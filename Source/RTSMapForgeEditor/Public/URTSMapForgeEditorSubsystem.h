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
 */
UCLASS()
class RTSMAPFORGEEDITOR_API URTSMapForgeEditorSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // === Generation ===
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Editor")
    void GenerateMap(class URTSGenerationSettings* Settings);

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
    FRTSValidationResult GetLastValidation() const { return LastValidation; }

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
    FRTSValidationResult LastValidation;

    UPROPERTY()
    TObjectPtr<UTexture2D> PreviewTexture;

    ERTSDebugOverlayMode CurrentOverlayMode = ERTSDebugOverlayMode::Heightmap;

    void CreatePreviewTexture(int32 Width, int32 Height);
};
