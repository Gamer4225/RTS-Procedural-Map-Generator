#include "URTSMapForgeEditorSubsystem.h"
#include "Core/URTSGenerationSettings.h"
#include "Core/FRTSGenerationPipeline.h"
#include "Engine/Texture2D.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

void URTSMapForgeEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void URTSMapForgeEditorSubsystem::Deinitialize()
{
    // CRITICAL: RemoveFromRoot MUST be called before MarkAsGarbage
    // to prevent UObject root set leak. AddToRoot() was called in
    // CreatePreviewTexture — we balance it here.
    if (PreviewTexture)
    {
        PreviewTexture->RemoveFromRoot();
        PreviewTexture->MarkAsGarbage();
        PreviewTexture = nullptr;
    }
    
    Super::Deinitialize();
}

void URTSMapForgeEditorSubsystem::GenerateMap(URTSGenerationSettings* Settings)
{
    if (!Settings)
    {
        return;
    }

    FRTSGenerationPipeline Pipeline;
    
    // CRITICAL: Generate() resolves the seed EXACTLY ONCE internally.
    // The returned int64 is the resolved seed for THIS generation.
    // We MUST NOT call Settings->ResolveSeed() again here.
    int64 ResolvedSeed = Pipeline.Generate(Settings, CurrentGrid, LastMetadata, LastValidationResult, /*MaxRetries=*/10);

    // Use the seed returned by the pipeline — do NOT re-resolve.
    LastMetadata.Seed = ResolvedSeed;
    LastMetadata.GridWidth = CurrentGrid.Width;
    LastMetadata.GridHeight = CurrentGrid.Height;
    LastMetadata.CellSize = CurrentGrid.CellSize;

    UpdatePreviewTexture();
}

bool URTSMapForgeEditorSubsystem::HasValidGrid() const
{
    return CurrentGrid.Cells.Num() > 0 && CurrentGrid.Width > 0 && CurrentGrid.Height > 0;
}

void URTSMapForgeEditorSubsystem::SetOverlayMode(ERTSDebugOverlayMode Mode)
{
    CurrentOverlayMode = Mode;
    UpdatePreviewTexture();
}

void URTSMapForgeEditorSubsystem::CycleOverlayMode()
{
    uint8 Next = static_cast<uint8>(CurrentOverlayMode) + 1;
    if (Next > static_cast<uint8>(ERTSDebugOverlayMode::ChokePoints))
    {
        Next = 0;
    }
    CurrentOverlayMode = static_cast<ERTSDebugOverlayMode>(Next);
    UpdatePreviewTexture();
}

void URTSMapForgeEditorSubsystem::CreatePreviewTexture(int32 Width, int32 Height)
{
    // REUSE existing texture if dimensions match — avoids GC churn
    if (PreviewTexture && PreviewTexture->GetSizeX() == Width && PreviewTexture->GetSizeY() == Height)
    {
        return;
    }

    // Only create new texture when dimensions change.
    // CRITICAL: RemoveFromRoot the old texture to prevent leak before replacing.
    if (PreviewTexture)
    {
        PreviewTexture->RemoveFromRoot();
        PreviewTexture->MarkAsGarbage();
        PreviewTexture = nullptr;
    }

    PreviewTexture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
    if (!PreviewTexture)
    {
        return;
    }

    PreviewTexture->Filter = TF_Nearest;      // Pixel-perfect for grid preview
    PreviewTexture->AddressX = TA_Clamp;
    PreviewTexture->AddressY = TA_Clamp;
    PreviewTexture->SRGB = false;

    // Prevent GC during generation cycles. Balanced by RemoveFromRoot in:
    //   - Deinitialize()  (editor shutdown)
    //   - CreatePreviewTexture() (when dimensions change, before replacement)
    PreviewTexture->AddToRoot();
}

void URTSMapForgeEditorSubsystem::UpdatePreviewTexture()
{
    if (!HasValidGrid())
    {
        return;
    }

    TArray<FColor> Bitmap;
    int32 W = 0, H = 0;

    FRTSDebugRenderer Renderer;
    Renderer.GenerateMinimapBitmap(CurrentGrid, CurrentOverlayMode, Bitmap, W, H);

    if (Bitmap.Num() == 0)
    {
        return;
    }

    CreatePreviewTexture(W, H);
    if (!PreviewTexture)
    {
        return;
    }

    FTexturePlatformData* PlatformData = PreviewTexture->GetPlatformData();
    if (!PlatformData || PlatformData->Mips.Num() == 0)
    {
        return;
    }

    FTexture2DMipMap& Mip = PlatformData->Mips[0];
    void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
    if (Data)
    {
        FMemory::Memcpy(Data, Bitmap.GetData(), Bitmap.Num() * sizeof(FColor));
        Mip.BulkData.Unlock();
    }
    else
    {
        Mip.BulkData.Unlock();
        return;
    }

    PreviewTexture->UpdateResource();
}

void URTSMapForgeEditorSubsystem::ExportMetadataToJSON()
{
    if (!HasValidGrid())
    {
        return;
    }

    FString Json;
    Json += TEXT("{\n");
    Json += FString::Printf(TEXT("  \"Seed\": %lld,\n"), LastMetadata.Seed);
    Json += FString::Printf(TEXT("  \"GridWidth\": %d,\n"), LastMetadata.GridWidth);
    Json += FString::Printf(TEXT("  \"GridHeight\": %d,\n"), LastMetadata.GridHeight);
    Json += FString::Printf(TEXT("  \"CellSize\": %.2f,\n"), LastMetadata.CellSize);
    Json += FString::Printf(TEXT("  \"Bases\": %d,\n"), LastMetadata.Bases.Num());
    Json += FString::Printf(TEXT("  \"Expansions\": %d,\n"), LastMetadata.Expansions.Num());
    Json += FString::Printf(TEXT("  \"Chokes\": %d,\n"), LastMetadata.Chokes.Num());
    Json += FString::Printf(TEXT("  \"ValidationScore\": %.2f,\n"), LastValidationResult.OverallScore);
    Json += FString::Printf(TEXT("  \"Passed\": %s\n"), LastValidationResult.bPassed ? TEXT("true") : TEXT("false"));
    Json += TEXT("}\n");

    const FString Path = FPaths::ProjectSavedDir() / TEXT("RTSMapForge_LastExport.json");
    FFileHelper::SaveStringToFile(Json, *Path);
}
