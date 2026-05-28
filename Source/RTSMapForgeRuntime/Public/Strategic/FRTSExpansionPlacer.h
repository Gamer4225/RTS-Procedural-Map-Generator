#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Core/URTSGenerationSettings.h"
#include "Core/FRTSSeedManager.h"
class RTSMAPFORGERUNTIME_API FRTSExpansionPlacer
{
public:
    void PlaceExpansions(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager);
};
