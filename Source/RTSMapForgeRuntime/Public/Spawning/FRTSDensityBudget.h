#pragma once
#include "CoreMinimal.h"

/**
 * V1 Density Budget System: Prevents HISM/ISM spawning from destroying editor performance.
 *
 * Budget tiers (per 256x256 = 65,536 cells):
 *   Props (trees/rocks):    2,000 instances max
 *   Resources:                 64 clusters max
 *   Decorative (grass/etc): 4,000 instances max
 *   Total combined:          6,000 instances max
 */
struct RTSMAPFORGERUNTIME_API FRTSDensityBudget
{
    int32 MaxProps = 0;
    int32 MaxResources = 0;
    int32 MaxDecorative = 0;
    int32 MaxTotal = 0;

    int32 PlacedProps = 0;
    int32 PlacedResources = 0;
    int32 PlacedDecorative = 0;
    int32 PlacedTotal = 0;

    void Initialize(int32 GridCellCount);

    bool CanPlaceProp() const;
    bool CanPlaceResource() const;
    bool CanPlaceDecorative() const;

    bool RecordProp();
    bool RecordResource();
    bool RecordDecorative();

    int32 GetRemainingProps() const       { return FMath::Max(0, MaxProps - PlacedProps); }
    int32 GetRemainingResources() const   { return FMath::Max(0, MaxResources - PlacedResources); }
    int32 GetRemainingTotal() const       { return FMath::Max(0, MaxTotal - PlacedTotal); }
    bool  IsTotalBudgetExhausted() const  { return PlacedTotal >= MaxTotal; }

    FString GenerateReport() const;
};
