#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Core/FRTSSeedManager.h"
#include "Core/URTSGenerationSettings.h"

/**
 * Assigns biome IDs to grid cells using deterministic Voronoi regions.
 * Supports Poisson disk sampled seed points per biome.
 */
class RTSMAPFORGERUNTIME_API FRTSBiomeAssigner
{
public:
    void Assign(FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager);

private:
    struct FBiomeSeed
    {
        int32 BiomeIndex = INDEX_NONE;
        FVector2D Position; // Grid space
    };

    TArray<FBiomeSeed> GenerateSeeds(const FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager) const;
    int32 FindNearestSeedIndex(const FRTSGrid& Grid, int32 X, int32 Y, const TArray<FBiomeSeed>& Seeds) const;
};
