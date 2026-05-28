#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Validation/FRTSValidationResult.h"
#include "Core/URTSGenerationSettings.h"
class RTSMAPFORGERUNTIME_API FRTSResourceAccessibilityValidator
{
public:
    void Validate(const FRTSGrid&,const FRTSMapMetadata&,FRTSValidationResult&,const URTSGenerationSettings*) const;
private:
    struct FResourceAccessibility { FVector2D Position; float ResourceValue=0,PathCostFromBase=0,SafetyScore=0; int32 ChokesEnRoute=0,RiverCrossings=0; };
    TArray<FResourceAccessibility> GatherResources(const FRTSGrid&,const FRTSMapMetadata&) const;
    void ComputeAccessibility(FResourceAccessibility&,const FRTSGrid&,const FRTSMapMetadata&) const;
    void ComputeSafety(FResourceAccessibility&,const FRTSGrid&,const FRTSMapMetadata&) const;
    bool TracePathForObstacles(const FRTSGrid&,FIntPoint,FIntPoint,int32&,int32&,float&) const;
};
