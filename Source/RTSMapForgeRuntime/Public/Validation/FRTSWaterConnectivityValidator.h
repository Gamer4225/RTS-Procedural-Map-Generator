#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Validation/FRTSValidationResult.h"

/**
 * V1 Water Validation: Ensures water bodies are properly connected
 * and do not create isolated unreachable pockets that break gameplay.
 * 
 * In RTS maps, rivers must be:
 *   - Traversal blockers (intentional)
 *   - NOT create isolated land regions (unintentional)
 *   - Reachable from map edges (rivers flow somewhere)
 */
class RTSMAPFORGERUNTIME_API FRTSWaterConnectivityValidator
{
public:
    // Returns true if all water is connected to at least one map edge
    // (meaning rivers flow off-map or into other water).
    // Sets bHasIsolatedWater = true if disconnected water pockets exist.
    bool ValidateWaterConnectivity(
        const FRTSGrid& Grid,
        FRTSValidationResult& OutResult,
        bool& bHasIsolatedWater
    ) const;

    // Counts land regions that are completely surrounded by water
    // (island pockets = bad for RTS unless intentional).
    int32 CountIsolatedLandRegions(const FRTSGrid& Grid) const;

private:
    void FloodFillWaterEdgeReachable(const FRTSGrid& Grid, TArray<bool>& OutReachable) const;
};
