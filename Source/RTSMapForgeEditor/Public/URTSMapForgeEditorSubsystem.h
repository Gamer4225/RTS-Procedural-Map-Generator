#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Validation/FRTSValidationResult.h"
#include "Visualization/FRTSDebugRenderer.h"
#include "URTSMapForgeEditorSubsystem.generated.h"

/**
 * Editor-only subsystem that owns the generated grid state,
 * preview texture, and overlay mode. Survives between PIE sessions.
 *
 * FIX Problem 1: GenerateMap() is now async — it dispatches generation to a
 * background thread and marshals UObject writes back to the game thread.
 * bIsGenerating prevents double-generation while a job is running.
 */
UCLASS()
class RTSMAPFORGEEDITOR_API URTSMapForgeEditorSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // === Generation ===

    // FIX Problem 1: Now async. Returns immediately; listen for bIsGenerating
    // transitioning false (or poll HasValidGrid()) to know when results are ready.
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    void GenerateMap(class URTSGenerationSettings* Settings);

    // Returns true if a generation job is currently in flight.
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor", BlueprintPure)
    bool IsGenerating() const { return bIsGenerating; }

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    bool HasValidGrid() const;

    // === Overlay ===
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    void SetOverlayMode(ERTSDebugOverlayMode Mode);

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    ERTSDebugOverlayMode GetOverlayMode() const { return CurrentOverlayMode; }

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    void CycleOverlayMode();

    // === Preview Texture ===
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    UTexture2D* GetPreviewTexture() const { return PreviewTexture; }

    void UpdatePreviewTexture();

    // === Query ===
    FORCEINLINE FRTSGrid& GetGrid() { return CurrentGrid; }
    FORCEINLINE const FRTSGrid& GetGrid() const { return CurrentGrid; }

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    FRTSValidationResult GetLastValidation() const { return LastValidationResult; }

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    FRTSMapMetadata GetLastMetadata() const { return LastMetadata; }

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    void ExportMetadataToJSON();

private:
    UPROPERTY()
    FRTSGrid CurrentGrid;

    UPROPERTY()
    FRTSMapMetadata LastMetadata;

    UPROPERTY()
    FRTSValidationResult LastValidationResult;

    UPROPERTY()
    TObjectPtr<UTexture2D> PreviewTexture;

    ERTSDebugOverlayMode CurrentOverlayMode = ERTSDebugOverlayMode::Heightmap;

    // FIX Problem 1: Guards against double-generation while async task is running.
    std::atomic<bool> bIsGenerating{ false };

    void CreatePreviewTexture(int32 Width, int32 Height);
};
