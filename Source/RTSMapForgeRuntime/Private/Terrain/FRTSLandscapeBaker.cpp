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
//   - Circular dependencies
//   - UE Landscape API fragility breaking core generation
//   - Version-specific bugs corrupting deterministic maps
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

    // SOURCE OF TRUTH CHECK: Grid must be the authoritative data.
    const int32 ExpectedCells = Grid.Width * Grid.Height;
    if (Grid.Cells.Num() != ExpectedCells)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSMapForge LandscapeBake: Grid cell count mismatch! Expected %d, got %d"),
            ExpectedCells, Grid.Cells.Num());
        return false;
    }

    // Find or create landscape proxy
    ALandscapeProxy* Landscape = FindOrCreateLandscape(World, Grid.Width, Grid.Height, Grid.CellSize);
    if (!Landscape)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSMapForge LandscapeBake: Failed to create or find LandscapeProxy"));
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

    // Write heightmap data
    bool bSuccess = WriteHeightmap(Landscape, Grid);
    if (!bSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSMapForge LandscapeBake: Heightmap write failed"));
        return false;
    }

    // Mark landscape as modified (editor dirty flag)
    Landscape->MarkPackageDirty();
    LandscapeInfo->MarkPackageDirty();

    // Regenerate collision/navmesh if requested
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
    // UE Landscape uses uint16 range: 0 = lowest, 65535 = highest
    // NormalizedHeight is [0, 1] from FRTSGrid
    // 
    // We map 0..1 to 1024..64512 to keep headroom:
    //   - Minimum 1024 (prevents landscape being completely flat at bottom)
    //   - Maximum 64512 (prevents hitting absolute ceiling)
    // 
    // This allows users to manually sculpt ±5% in Landscape editor
    // without the bake hitting uint16 boundaries.
    
    const uint16 MinValue = 1024;
    const uint16 MaxValue = 64512;
    const uint16 Range = MaxValue - MinValue;
    
    float Clamped = FMath::Clamp(NormalizedHeight, 0.0f, 1.0f);
    uint16 Result = static_cast<uint16>(MinValue + static_cast<uint16>(Clamped * static_cast<float>(Range)));
    
    return Result;
}

#if WITH_EDITOR
ALandscapeProxy* FRTSLandscapeBaker::FindOrCreateLandscape(UWorld* World, int32 Width, int32 Height, float CellSize)
{
    // Try to find existing landscape in world
    for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
    {
        ALandscapeProxy* Existing = *It;
        if (Existing && !Existing->IsPendingKillPending())
        {
            UE_LOG(LogTemp, Log, TEXT("RTSMapForge LandscapeBake: Reusing existing LandscapeProxy '%s'"),
                *Existing->GetName());
            // NOTE: Reusing existing landscape OVERWRITES its height data.
            // Grid remains source of truth. Any previous edits are lost.
            // This is correct by design: landscape is OUTPUT, not state.
            return Existing;
        }
    }

    // No existing landscape - create one
    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = FName(TEXT("RTSMapForge_Landscape"));
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    ALandscapeProxy* Landscape = World->SpawnActor<ALandscapeProxy>(
        FVector::ZeroVector,
        FRotator::ZeroRotator,
        SpawnParams
    );

    if (!Landscape)
    {
        UE_LOG(LogTemp, Error, TEXT("RTSMapForge LandscapeBake: SpawnActor<ALandscapeProxy> failed"));
        return nullptr;
    }

    // Calculate landscape component setup
    // UE Landscape uses quads per section and sections per component
    // Standard UE landscape: 63 quads per section, 1 section per component
    const int32 QuadsPerSection = 63;
    const int32 SectionsPerComponent = 1;
    const int32 ComponentSizeQuads = QuadsPerSection * SectionsPerComponent;
    
    // Calculate needed components
    int32 ComponentsX = FMath::CeilToInt(static_cast<float>(Width) / ComponentSizeQuads);
    int32 ComponentsY = FMath::CeilToInt(static_cast<float>(Height) / ComponentSizeQuads);
    
    // Total vertex size must align to component boundaries
    int32 TotalSizeX = ComponentsX * ComponentSizeQuads + 1;
    int32 TotalSizeY = ComponentsY * ComponentSizeQuads + 1;

    UE_LOG(LogTemp, Log, TEXT("RTSMapForge LandscapeBake: Creating Landscape %dx%d vertices (%dx%d components)"),
        TotalSizeX, TotalSizeY, ComponentsX, ComponentsY);

    Landscape->SetActorLocation(FVector::ZeroVector);
    Landscape->ComponentSizeQuads = ComponentSizeQuads;
    Landscape->SubsectionSizeQuads = QuadsPerSection;
    Landscape->NumSubsections = SectionsPerComponent;
    Landscape->CreateLandscapeInfo();

    return Landscape;
}

bool FRTSLandscapeBaker::WriteHeightmap(ALandscapeProxy* Landscape, const FRTSGrid& Grid)
{
    if (!Landscape || Grid.Cells.Num() == 0)
    {
        return false;
    }

    const int32 Width = Grid.Width;
    const int32 Height = Grid.Height;

    // Prepare height data buffer
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
        true,
        nullptr,
        true,
        nullptr,
        nullptr,
        true,
        true,
        true);
    LandscapeEdit.Flush();

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
#endif
