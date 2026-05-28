#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
class RTSMAPFORGERUNTIME_API FRTSTacticalZoneClassifier
{
public:
    void Classify(FRTSGrid& Grid, const FRTSMapMetadata& Metadata);
private:
    void FilterSmallRegions(FRTSGrid& Grid, TArray<bool>& Mask, int32 MinSize) const;
};
