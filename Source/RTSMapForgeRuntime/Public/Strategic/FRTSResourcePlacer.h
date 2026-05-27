#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Core/FRTSSeedManager.h"
#include "Core/URTSGenerationSettings.h"
#include "Data/FRTSMapMetadata.h"

/**
 * V1 Resource Placement: Deterministic Poisson-disk scatter near expansions,
 * high ground, and resource clusters.
 * 
 * Integrates into FRTSCell::ResourceValue and marks ResourceCluster tactical zones.
 */
class RTSMAPFORGERUNTIME_API FRTSResourcePlacer
{
public:
    void PlaceResources(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager);

private:
    struct FCandidate
    {
        FIntPoint Position;
        float Score;
    };

    TArray<FCandidate> GatherCandidates(FRTSGrid& Grid, const FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings) const;
    float ScoreCell(const FRTSGrid& Grid, int32 X, int32 Y, const FRTSMapMetadata& Metadata) const;
};
