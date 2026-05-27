#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Validation/FRTSValidationResult.h"
#include "Core/URTSGenerationSettings.h"

/**
 * V1.5 Resource Accessibility Validation
 * 
 * Total resource value parity (V1) is necessary but not sufficient.
 * Two equal resource pools may still be strategically unequal if:
 *   - One is protected by chokes / distance from enemy
 *   - One requires crossing contested territory
 *   - One is accessible early, the other only late-game
 * 
 * This validator computes accessibility per player:
 *   - Path distance from base to each resource cluster
 *   - Number of choke points en route
 *   - River crossings required
 *   - Comparison between players
 */
class RTSMAPFORGERUNTIME_API FRTSResourceAccessibilityValidator
{
public:
    void Validate(
        const FRTSGrid& Grid,
        const FRTSMapMetadata& Metadata,
        FRTSValidationResult& OutResult,
        const URTSGenerationSettings* Settings
    ) const;

private:
    struct FResourceAccessibility
    {
        FVector2D Position;
        float ResourceValue = 0.0f;
        float PathCostFromBase = 0.0f;     // A* cost from nearest base
        int32 ChokesEnRoute = 0;           // Choke points on path
        int32 RiverCrossings = 0;         // RiverCrossing zones on path
        float SafetyScore = 0.0f;          // Distance from enemy base
    };

    TArray<FResourceAccessibility> GatherResources(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata) const;
    void ComputeAccessibility(FResourceAccessibility& Resource, const FRTSGrid& Grid, const FRTSMapMetadata& Metadata) const;
    void ComputeSafety(FResourceAccessibility& Resource, const FRTSGrid& Grid, const FRTSMapMetadata& Metadata) const;
    
    bool TracePathForObstacles(
        const FRTSGrid& Grid,
        FIntPoint Start,
        FIntPoint End,
        int32& OutChokes,
        int32& OutRiverCrossings,
        float& OutPathCost
    ) const;
};
