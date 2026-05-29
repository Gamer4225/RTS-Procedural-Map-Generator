#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "URTSGenerationSettings.generated.h"

UCLASS(BlueprintType, Config = RTSMapForge)
class RTSMAPFORGERUNTIME_API URTSGenerationSettings : public UDataAsset
{
    GENERATED_BODY()

public:
    // === MAP SIZE ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ClampMin = "16", ClampMax = "2048"))
    int32 GridWidth = 256;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ClampMin = "16", ClampMax = "2048"))
    int32 GridHeight = 256;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map", meta = (ClampMin = "10.0"))
    float CellSize = 200.0f; // cm

    // === SEED ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seed")
    int64 Seed = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Seed")
    bool bRandomSeed = true;

    // === TERRAIN ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "1", ClampMax = "16"))
    int32 FBMOctaves = 6;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float FBMPersistence = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "1.0", ClampMax = "4.0"))
    float FBMLacunarity = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.001"))
    float TerrainScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WaterLevel = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MountainLevel = 0.75f;

    // === STRATEGIC ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strategic", meta = (ClampMin = "2", ClampMax = "12"))
    int32 NumPlayers = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strategic", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinRushDistance = 0.35f; // fraction of map diagonal

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strategic", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SymmetryStrength = 1.0f; // 0=none, 1=full

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Strategic", meta = (ClampMin = "0", ClampMax = "16"))
    int32 NumExpansions = 3;

    // === BIOMES ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biomes")
    TArray<TSoftObjectPtr<class URTSBiomeAsset>> Biomes;

    // === VALIDATION RULES ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation", meta = (ClampMin = "1"))
    float MinChokeWidth = 3.0f; // cells

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation", meta = (ClampMin = "1"))
    float MaxChokeWidth = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinBuildableRatio = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MaxFairnessError = 0.10f; // 10%

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation", meta = (ClampMin = "0", ClampMax = "100"))
    float MinAcceptableScore = 65.0f;

    // === DERIVED HELPERS ===
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Settings")
    float GetMapDiagonal() const;

    // CRITICAL: Called EXACTLY ONCE per generation, inside FRTSGenerationPipeline::Generate().
    // No other code may call this.
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Settings")
    int64 ResolveSeed() const;
};
