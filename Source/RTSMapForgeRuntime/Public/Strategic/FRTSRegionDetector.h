#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
class RTSMAPFORGERUNTIME_API FRTSRegionDetector
{
public:
    void DetectRegions(FRTSGrid& Grid);
    int32 GetRegionCellCount(int32 RegionID) const;
private:
    TMap<int32,int32> RegionSizes;
};
