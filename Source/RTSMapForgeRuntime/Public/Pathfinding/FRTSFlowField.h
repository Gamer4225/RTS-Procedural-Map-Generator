#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
// Flow field stub for V1 — used for unit movement in V2
class RTSMAPFORGERUNTIME_API FRTSFlowField
{
public:
    void Generate(const FRTSGrid& Grid, FIntPoint Goal) {}
};
