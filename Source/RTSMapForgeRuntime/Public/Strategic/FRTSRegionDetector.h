#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"

/**
 * Detects connected walkable regions via flood fill (BFS).
 * Assigns Cell.RegionID and computes metadata.
 */
class RTSMAPFORGERUNTIME_API FRTSRegionDetector
{
public:
    void DetectRegions(FRTSGrid& Grid);

    int32 GetNumRegions() const { return RegionSizes.Num(); }
    int32 GetRegionCellCount(int32 RegionID) const;

private:
    TMap<int32, int32> RegionSizes;
};
