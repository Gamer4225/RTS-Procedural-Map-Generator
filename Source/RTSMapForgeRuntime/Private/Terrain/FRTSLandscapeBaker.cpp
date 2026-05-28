#include "Terrain/FRTSLandscapeBaker.h"
#include "Core/FRTSGrid.h"

// ============================================================
// CRITICAL ARCHITECTURAL PRINCIPLE: Grid is Source of Truth
// ============================================================
// One-way data flow: FRTSGrid (simulation) → ALandscapeProxy (visual output)
//
// NEVER read heights back from Landscape.
// NEVER let Landscape edits modify the Grid.
// Landscape is PURELY an output visualization target.
//
// This prevents:
//  - Circular dependencies
//  - UE Landscape API fragility breaking core generation
//  - Version-specific bugs corrupting deterministic maps
// ============================================================

#if WITH_EDITOR
#include "Landscape.h"
#include "LandscapeProxy.h"
#include "LandscapeInfo.h"
#include "LandscapeComponent.h"
#include "LandscapeEdit.h"
#include "Engine/World.h"
#include "Editor.h"
#include "FileHelpers.h"
#endif

bool FRTSLandscapeBaker::BakeToLandscape(const FRTSGrid& Grid, UWorld* World, bool bRegenerateCollision)
{
#if WITH_EDITOR
    if (!World || Grid.Cells.Num() == 0 || Grid.Width <= 0 || Grid.Height <= 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("RTSMapForge LandscapeBake: Invalid input - World=%s, Cells=%d, WxH=%dx%d"),
            World ? TEXT("valid") : TEXT("null"), Grid.Cells.Num(), Grid.Width, Grid.Height);
        return false;
    }

    // SOURCE OF TRUTH CHECK: Grid must be authoritative.
    const int32 ExpectedCells = Grid.Width * Grid.Height;
    if (Grid.Cells.Num() != ExpectedCells)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSMapForge LandscapeBake: Grid cell count mismatch! Expected %d, got %d"),
            ExpectedCells, Grid.Cells.Num());
        return false;
    }

    // FIX Minor: We no longer attempt to auto-create ALandscapeProxy / ALandscape.
    // Auto-creation is fragile in UE5: ALandscapeProxy must be created by the
    // landscape system as a streaming level proxy, NOT spawned directly.
    // Spawning ALandscapeProxy directly would create an invisible actor with no
    // proper component initialization.
    //
    // V1 policy: Bake ONLY to an existing landscape that the user creates manually.
    // If no landscape is found, log a clear user-facing error and return false.
    // Document this in the QuickStart guide.
    ALandscapeProxy* Landscape = FindExistingLandscape(World);
    if (!Landscape)
    {
        UE_LOG(LogTemp, Error,
            TEXT("RTSMapForge LandscapeBake: No ALandscape found in the current level. ")
            TEXT("Please create a Landscape actor manually in the editor before baking. ")
            TEXT("(Modes > Landscape > Manage > New Landscape). ")
            TEXT("The bake will then overwrite its heightmap with the generated grid data."));
        return false;
    }

    ULandscapeInfo* LandscapeInfo = Landscape->CreateLandscapeInfo();
    if (!LandscapeInfo)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSMapForge LandscapeBake: CreateLandscapeInfo() returned null"));
        return false;
    }

    UE_LOG(LogTemp, Log, TEXT("RTSMapForge LandscapeBake: Baking %dx%d grid to Landscape '%s'"),
        Grid.Width, Grid.Height, *Landscape->GetName());

    // Write heightmap data using the landscape edit interface
    bool bSuccess = WriteHeightmap(Landscape, Grid);
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSMapForge LandscapeBake: Heightmap write failed"));
        return false;
    }

    // Mark landscape as modified (editor dirty flag)
    Landscape->MarkPackageDirty();
    LandscapeInfo->MarkPackageDirty();

    // Optionally regenerate collision / navmesh
    if (bRegenerateCollision)
    {
        UE_LOG(LogTemp, Log, TEXT("RTSMapForge LandscapeBake: Regenerating collision components..."));
        Landscape->RecreateCollisionComponents();
    }

    UE_LOG(LogTemp, Log, TEXT("RTSMapForge LandscapeBake: SUCCESS. Grid is source of truth. Landscape is output ONLY."));
    return true;
#else
    UE_LOG(LogTemp, Warning, TEXT("RTSMapForge LandscapeBake: Only available in Editor builds. Grid remains source of truth."));
    return false;
#endif
}

uint16 FRTSLandscapeBaker::HeightToLandscape(float NormalizedHeight)
{
    // UE Landscape uses uint16 range: 0 = lowest, 65535 = highest.
    // NormalizedHeight is [0, 1] from FRTSGrid.
    //
    // We map 0..1 to 1024..64512 to keep headroom:
    //  - Minimum 1024 (prevents landscape being completely flat at the bottom)
    //  - Maximum 64512 (prevents hitting the absolute uint16 ceiling)
    //
    // This allows users to manually sculpt ±~1.5% in Landscape editor
    // without the bake hitting uint16 boundaries.

    const uint16 MinValue = 1024;
    const uint16 MaxValue = 64512;
    const uint16 Range    = MaxValue - MinValue;

    float Clamped = FMath::Clamp(NormalizedHeight, 0.0f, 1.0f);
    return static_cast<uint16>(MinValue + static_cast<uint32>(Clamped * static_cast<float>(Range)));
}

#if WITH_EDITOR

// FIX Minor: Replaced FindOrCreateLandscape() with FindExistingLandscape().
// Auto-creating ALandscapeProxy directly via SpawnActor is incorrect in UE5 —
// proxies are created by the landscape streaming system, not user code.
// V1 requires the user to pre-create a landscape in the editor.
ALandscapeProxy* FRTSLandscapeBaker::FindExistingLandscape(UWorld* World)
{
    for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
    {
        ALandscapeProxy* Existing = *It;
        if (Existing && !Existing->IsPendingKillPending())
        {
            UE_LOG(LogTemp, Log, TEXT("RTSMapForge LandscapeBake: Found existing LandscapeProxy '%s'. ")
                TEXT("Grid heightmap will OVERWRITE its current height data. ")
                TEXT("Grid is source of truth — any manual sculpting will be lost."),
                *Existing->GetName());
            return Existing;
        }
    }
    return nullptr;
}

bool FRTSLandscapeBaker::WriteHeightmap(ALandscapeProxy* Landscape, const FRTSGrid& Grid)
{
    if (!Landscape || Grid.Cells.Num() == 0)
    {
        return false;
    }

    const int32 Width  = Grid.Width;
    const int32 Height = Grid.Height;

    // Prepare height data buffer (uint16, row-major)
    TArray<uint16> HeightData;
    HeightData.SetNumUninitialized(Width * Height);

    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            const FRTSCell& Cell = Grid.GetCell(X, Y);
            HeightData[Y * Width + X] = HeightToLandscape(Cell.Height);
        }
    }

    // Get landscape info for editor-side write access
    ULandscapeInfo* LandscapeInfo = Landscape->GetLandscapeInfo();
    if (!LandscapeInfo)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSMapForge LandscapeBake: GetLandscapeInfo() returned null"));
        return false;
    }

    FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo);
    LandscapeEdit.SetHeightData(
        0,
        0,
        Width - 1,
        Height - 1,
        HeightData.GetData(),
        Width,
        /*bUpdateCollision=*/true,
        nullptr,
        /*bUpdateBounds=*/true,
        nullptr,
        nullptr,
        /*bUpdateAddCollision=*/true,
        /*bMipmap=*/true,
        /*bInterpNormals=*/true);
    LandscapeEdit.Flush();

    // Mark modified components dirty
    TSet<ULandscapeComponent*> ModifiedComponents;
    LandscapeEdit.GetComponentsInRegion(0, 0, Width - 1, Height - 1, &ModifiedComponents);
    for (ULandscapeComponent* Component : ModifiedComponents)
    {
        if (Component)
        {
            Component->MarkPackageDirty();
        }
    }

    Landscape->MarkPackageDirty();
    LandscapeInfo->MarkPackageDirty();

    return true;
}

#endif // WITH_EDITOR
