#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Core/URTSGenerationSettings.h"

class RTSMAPFORGERUNTIME_API FRTSChokeDetector
{
public:
    void DetectChokes(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings);
};
