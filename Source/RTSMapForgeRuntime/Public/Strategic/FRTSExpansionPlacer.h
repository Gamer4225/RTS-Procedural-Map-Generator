#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Core/FRTSSeedManager.h"
#include "Core/URTSGenerationSettings.h"
#include "Data/FRTSMapMetadata.h"

class RTSMAPFORGERUNTIME_API FRTSExpansionPlacer
{
public:
    void PlaceExpansions(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager);
};
