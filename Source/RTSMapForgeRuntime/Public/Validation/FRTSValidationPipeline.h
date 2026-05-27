#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Validation/FRTSValidationResult.h"
#include "Core/URTSGenerationSettings.h"

/**
 * 6-Pass validation pipeline with explicit CRITICAL/WARNING severity.
 * 
 * NEW: Pass 1 now includes base-to-base A* traversal validation.
 * NEW: Pass 3 now includes resource parity (global economic balance).
 * 
 * On CRITICAL failure: generation retries with mutated seed (up to MaxRetries).
 * On WARNING: map accepted but flagged to user.
 */
class RTSMAPFORGERUNTIME_API FRTSValidationPipeline
{
public:
    void Validate(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings);

private:
    // Pass 1: All bases must be reachable from each other via A*
    // CRITICAL if any base pair is unreachable.
    void Pass1_Traversal(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) const;

    // Pass 2: Each base must have adequate flat buildable area
    // CRITICAL if any base lacks buildable radius.
    void Pass2_Spawn(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult) const;

    // Pass 3: Resource and expansion fairness within MaxFairnessError (10%)
    // WARNING if imbalance detected; flags to user.
    void Pass3_Economy(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) const;

    // Pass 4: Choke structure — at least 1 choke per player pair, not too many
    // WARNING if structure is weak or excessive.
    void Pass4_Choke(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult) const;

    // Pass 5: Navmesh traversal width — min choke width >= MinChokeWidth
    // CRITICAL if chokes are too narrow (units can't pass).
    void Pass5_Navmesh(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) const;

    // Pass 6: Overall strategic fairness score >= MinAcceptableScore
    // WARNING if below threshold; user decides.
    void Pass6_Fairness(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) const;
};
