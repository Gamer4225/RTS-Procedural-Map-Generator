#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Core/FRTSSeedManager.h"
#include "Core/URTSGenerationSettings.h"
#include "Data/FRTSMapMetadata.h"

/**
 * Places starting bases using Poisson disk constraints + symmetry rules.
 * Ensures adequate buildable area and minimum rush distances.
 */
class RTSMAPFORGERUNTIME_API FRTSBasePlacer
{
public:
    void PlaceBases(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager);

private:
    TArray<FIntPoint> FindCandidateCells(const FRTSGrid& Grid, int32 MinRegionSize) const;
    FIntPoint PickCandidate(const TArray<FIntPoint>& Candidates, UFRTSSeedManager* SeedManager) const;
    bool IsAreaBuildable(const FRTSGrid& Grid, int32 X, int32 Y, int32 RadiusCells) const;
    FIntPoint ApplySymmetry(const FIntPoint& Point, int32 W, int32 H, int32 NumPlayers, int32 PlayerIndex) const;
};
