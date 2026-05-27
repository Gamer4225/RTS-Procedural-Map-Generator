#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/URTSGenerationSettings.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Validation/FRTSValidationResult.h"
#include "URTSMapForgeSubsystem.generated.h"

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnMapGenComplete, const FRTSValidationResult&, ValidationResult);

/**
 * Public Blueprint-facing API for the RTS MapForge generator.
 * Can be used from Editor utilities or runtime (though V1 targets editor-time generation).
 */
UCLASS(BlueprintType)
class RTSMAPFORGERUNTIME_API URTSMapForgeSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // === Generation ===
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Generation")
    void GenerateMap(URTSGenerationSettings* Settings);

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Generation")
    void GenerateMapAsync(URTSGenerationSettings* Settings, const FOnMapGenComplete& OnComplete);

    // === Queries ===
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSMapForge|Query")
    FRTSCell GetCellAtWorldLocation(FVector WorldLocation) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSMapForge|Query")
    ERTSTacticalZone GetZoneAtLocation(FVector Location) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSMapForge|Query")
    FRTSValidationResult GetLastValidationResult() const { return LastValidationResult; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RTSMapForge|Query")
    FRTSMapMetadata GetLastMetadata() const { return LastMetadata; }

    // === Export ===
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Export")
    bool ExportMetadataToJSON(FString FilePath) const;

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Export")
    void BakeToLevel();

    // === Access to raw grid (advanced C++ use) ===
    FORCEINLINE FRTSGrid& GetGrid() { return CurrentGrid; }
    FORCEINLINE const FRTSGrid& GetGrid() const { return CurrentGrid; }

private:
    UPROPERTY()
    FRTSGrid CurrentGrid;

    UPROPERTY()
    FRTSMapMetadata LastMetadata;

    UPROPERTY()
    FRTSValidationResult LastValidationResult;
};
