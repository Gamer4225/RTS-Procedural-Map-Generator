#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Validation/FRTSValidationResult.h"
#include "Core/URTSGenerationSettings.h"
class RTSMAPFORGERUNTIME_API FRTSValidationPipeline
{
public:
    void Validate(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings);
private:
    void Pass1_Traversal(const FRTSGrid&,const FRTSMapMetadata&,FRTSValidationResult&,const URTSGenerationSettings*) const;
    void Pass2_Spawn(const FRTSGrid&,const FRTSMapMetadata&,FRTSValidationResult&) const;
    void Pass3_Economy(const FRTSGrid&,const FRTSMapMetadata&,FRTSValidationResult&,const URTSGenerationSettings*) const;
    void Pass4_Choke(const FRTSGrid&,const FRTSMapMetadata&,FRTSValidationResult&) const;
    void Pass5_Navmesh(const FRTSGrid&,const FRTSMapMetadata&,FRTSValidationResult&,const URTSGenerationSettings*) const;
    void Pass6_Fairness(const FRTSGrid&,const FRTSMapMetadata&,FRTSValidationResult&,const URTSGenerationSettings*) const;
};
