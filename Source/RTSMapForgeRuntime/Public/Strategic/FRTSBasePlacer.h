#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Core/URTSGenerationSettings.h"
#include "Core/FRTSSeedManager.h"
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
