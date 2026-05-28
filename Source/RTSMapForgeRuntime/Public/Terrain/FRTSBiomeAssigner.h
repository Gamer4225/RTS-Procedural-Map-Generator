#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Core/URTSGenerationSettings.h"
#include "Core/FRTSSeedManager.h"
class RTSMAPFORGERUNTIME_API FRTSBiomeAssigner
{
public:
    void Assign(FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager);
private:
    struct FBiomeSeed { FVector2D Position; int32 BiomeIndex=0; };
    TArray<FBiomeSeed> GenerateSeeds(const FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager) const;
    int32 FindNearestSeedIndex(const FRTSGrid& Grid, int32 X, int32 Y, const TArray<FBiomeSeed>& Seeds) const;
};
