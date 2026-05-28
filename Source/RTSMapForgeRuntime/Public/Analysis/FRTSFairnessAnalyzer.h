#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Validation/FRTSValidationResult.h"
#include "Core/URTSGenerationSettings.h"
class RTSMAPFORGERUNTIME_API FRTSFairnessAnalyzer
{
public:
    void Analyze(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) {}
};
