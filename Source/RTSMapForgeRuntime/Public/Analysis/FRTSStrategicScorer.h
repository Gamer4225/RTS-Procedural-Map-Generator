#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Validation/FRTSValidationResult.h"
#include "Core/URTSGenerationSettings.h"

class RTSMAPFORGERUNTIME_API FRTSStrategicScorer
{
public:
    void Score(FRTSGrid& Grid, FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings);
};
