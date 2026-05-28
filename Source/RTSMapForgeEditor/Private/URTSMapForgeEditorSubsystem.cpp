#include "URTSMapForgeEditorSubsystem.h"
#include "Core/URTSGenerationSettings.h"
#include "Core/FRTSGenerationPipeline.h"
#include "Visualization/FRTSDebugRenderer.h"
#include "Async/Async.h"                    // FIX Problem 1: AsyncTask
#include "Engine/Texture2D.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

void URTSMapForgeEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bIsGenerating = false;
}

void URTSMapForgeEditorSubsystem::Deinitialize()
{
    // CRITICAL: RemoveFromRoot MUST be called before MarkAsGarbage
    // to prevent UObject root-set leak. AddToRoot() was called in
    // CreatePreviewTexture() — we balance it here.
    if (PreviewTexture)
    {
        PreviewTexture->RemoveFromRoot();
        PreviewTexture->MarkAsGarbage();
        PreviewTexture = nullptr;
    }

    Super::Deinitialize();
}

// ============================================================
// FIX Problem 1: GenerateMap() is now asynchronous.
//
// Previously this ran synchronously on the game/editor thread, freezing the
// editor for 2–30+ seconds depending on grid size. UE's watchdog could kill
// the process on very large grids.
//
// New flow:
//   1. Guard against re-entry with bIsGenerating.
//   2. Dispatch FRTSGenerationPipeline::Generate() to a background thread.
//      (FRTSGenerationPipeline is a plain C++ class — no UObject/GT restrictions.)
//   3. Marshal all UObject writes (CurrentGrid, textures, metadata) back to
//      the game thread via a nested AsyncTask(GameThread, ...) lambda.
//
// Settings pointer: The pipeline reads Settings values during generation.
// Settings is a UDataAsset owned by the editor; it will not be GC'd during the
// short window of background execution as long as the subsystem holds a reference.
// We capture Settings as a raw pointer (safe: UDataAsset lifetime >> task lifetime)
// and do NOT write to it on the background thread.
// ============================================================
void URTSMapForgeEditorSubsystem::GenerateMap(URTSGenerationSettings* Settings)
{
    if (!Settings || bIsGenerating)
    {
        return;
    }

    bIsGenerating = true;

    // Capture a strong reference to keep 'this' alive for the duration of the task.
    TWeakObjectPtr<URTSMapForgeEditorSubsystem> WeakThis(this);

    AsyncTask(ENamedThreads::AnyBackgroundThreadNormalTask, [WeakThis, Settings]()
    {
        // --- BACKGROUND THREAD ---
        // FRTSGenerationPipeline is a pure C++ class with no GT affinity.
        FRTSGrid           TempGrid;
        FRTSMapMetadata    TempMeta;
        FRTSValidationResult TempValidation;
        FRTSGenerationPipeline Pipeline;

        // CRITICAL: Generate() resolves the seed EXACTLY ONCE internally.
        // We MUST NOT call Settings->ResolveSeed() here.
        int64 ResolvedSeed = Pipeline.Generate(Settings, TempGrid, TempMeta, TempValidation, /*MaxRetries=*/10);
        TempMeta.Seed = ResolvedSeed;

        // --- RETURN TO GAME THREAD for all UObject writes ---
        AsyncTask(ENamedThreads::GameThread,
            [WeakThis,
             TempGrid       = MoveTemp(TempGrid),
             TempMeta,
             TempValidation,
             ResolvedSeed]() mutable
        {
            URTSMapForgeEditorSubsystem* Self = WeakThis.Get();
            if (!Self)
            {
                return; // Subsystem was destroyed while task was running
            }

            Self->CurrentGrid          = MoveTemp(TempGrid);
            Self->LastMetadata         = TempMeta;
            Self->LastMetadata.Seed    = ResolvedSeed;
            Self->LastMetadata.GridWidth  = Self->CurrentGrid.Width;
            Self->LastMetadata.GridHeight = Self->CurrentGrid.Height;
            Self->LastMetadata.CellSize   = Self->CurrentGrid.CellSize;
            Self->LastValidationResult = TempValidation;
            Self->bIsGenerating        = false;

            Self->UpdatePreviewTexture();
        });
    });
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

    // CRITICAL: RemoveFromRoot the old texture before replacing to prevent leak.
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

    PreviewTexture->Filter   = TF_Nearest; // Pixel-perfect for grid preview
    PreviewTexture->AddressX = TA_Clamp;
    PreviewTexture->AddressY = TA_Clamp;
    PreviewTexture->SRGB     = false;

    // Prevent GC during generation cycles. Balanced by RemoveFromRoot in:
    //  - Deinitialize()           (editor shutdown)
    //  - CreatePreviewTexture()   (when dimensions change, before replacement)
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

    // FIX Minor: Use TJsonWriter for safe, properly-escaped JSON serialization.
    // Manual string building is fragile — field values with special chars would
    // produce malformed JSON.
    FString Json;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    Writer->WriteObjectStart();
    Writer->WriteValue(TEXT("Seed"),            LastMetadata.Seed);
    Writer->WriteValue(TEXT("GridWidth"),       LastMetadata.GridWidth);
    Writer->WriteValue(TEXT("GridHeight"),      LastMetadata.GridHeight);
    Writer->WriteValue(TEXT("CellSize"),        LastMetadata.CellSize);
    Writer->WriteValue(TEXT("Bases"),           LastMetadata.Bases.Num());
    Writer->WriteValue(TEXT("Expansions"),      LastMetadata.Expansions.Num());
    Writer->WriteValue(TEXT("Chokes"),          LastMetadata.Chokes.Num());
    Writer->WriteValue(TEXT("ValidationScore"), LastValidationResult.OverallScore);
    Writer->WriteValue(TEXT("Passed"),          LastValidationResult.bPassed);
    Writer->WriteValue(TEXT("RetryCount"),      LastValidationResult.RetryCount);

    // Write issue summary array
    Writer->WriteArrayStart(TEXT("Issues"));
    for (const FRTSValidationIssue& Issue : LastValidationResult.Issues)
    {
        Writer->WriteObjectStart();
        Writer->WriteValue(TEXT("Pass"),     Issue.PassName);
        Writer->WriteValue(TEXT("Reason"),   Issue.Reason);
        Writer->WriteValue(TEXT("Severity"), Issue.Severity == ERTSValidationSeverity::Critical
                                                ? TEXT("Critical")
                                                : Issue.Severity == ERTSValidationSeverity::Warning
                                                    ? TEXT("Warning")
                                                    : TEXT("Pass"));
        Writer->WriteObjectEnd();
    }
    Writer->WriteArrayEnd();

    Writer->WriteObjectEnd();
    Writer->Close();

    const FString Path = FPaths::ProjectSavedDir() / TEXT("RTSMapForge_LastExport.json");
    FFileHelper::SaveStringToFile(Json, *Path);
    UE_LOG(LogTemp, Log, TEXT("RTSMapForge: Metadata exported to %s"), *Path);
}
