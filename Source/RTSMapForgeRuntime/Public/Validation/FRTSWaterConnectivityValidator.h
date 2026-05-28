#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Validation/FRTSValidationResult.h"
class RTSMAPFORGERUNTIME_API FRTSWaterConnectivityValidator
{
public:
    bool ValidateWaterConnectivity(const FRTSGrid& Grid, FRTSValidationResult& OutResult, bool& bHasIsolatedWater) const;
    int32 CountIsolatedLandRegions(const FRTSGrid& Grid) const;
private:
    void FloodFillWaterEdgeReachable(const FRTSGrid& Grid, TArray<bool>& OutReachable) const;
};
