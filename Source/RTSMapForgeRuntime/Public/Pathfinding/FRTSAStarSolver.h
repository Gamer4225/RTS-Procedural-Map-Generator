#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Core/URTSGenerationSettings.h"

class RTSMAPFORGERUNTIME_API FRTSAStarSolver
{
public:
    void ComputeRushDistances(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings);

    // Returns path cost (grid units) between two points, or -1 if unreachable.
    float FindPathCost(const FRTSGrid& Grid, FIntPoint Start, FIntPoint Goal, int32 MaxIterations = 50000) const;

private:
    struct FAStarNode
    {
        int32 Index = INDEX_NONE;
        float G = 0.0f;
        float F = 0.0f;
        int32 Parent = INDEX_NONE;

        bool operator==(const FAStarNode& Other) const { return Index == Other.Index; }
    };

    float OctileDistance(FIntPoint A, FIntPoint B) const;
};
