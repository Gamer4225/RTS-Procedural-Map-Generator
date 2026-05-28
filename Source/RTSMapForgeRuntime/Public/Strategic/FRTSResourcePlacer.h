#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Core/URTSGenerationSettings.h"
#include "Core/FRTSSeedManager.h"
class RTSMAPFORGERUNTIME_API FRTSResourcePlacer
{
public:
    void PlaceResources(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager);
private:
    struct FCandidate { FIntPoint Position; float Score; };
    TArray<FCandidate> GatherCandidates(FRTSGrid& Grid, const FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings) const;
    float ScoreCell(const FRTSGrid& Grid, int32 X, int32 Y, const FRTSMapMetadata& Metadata) const;
};
