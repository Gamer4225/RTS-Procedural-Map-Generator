#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Core/URTSGenerationSettings.h"

class RTSMAPFORGERUNTIME_API FRTSAStarSolver
{
public:
    // Computes rush distances between all base pairs.
    // FIX Minor: Results are now stored in Metadata.RushDistances (TMap<int64, float>)
    // so the validation pass can read cached costs without re-running A*.
    void ComputeRushDistances(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings);

    // Returns path cost (grid units) between two points, or -1 if unreachable.
    // FIX Problem 2: Open-set best-node selection now uses Algo::MinElementBy
    // (tighter constant than hand-written linear scan) + a BestG TMap to avoid
    // duplicate open-set entries. V2 target: binary heap for O(log n) extraction.
    float FindPathCost(const FRTSGrid& Grid, FIntPoint Start, FIntPoint Goal, int32 MaxIterations = 50000) const;

private:
    struct FAStarNode
    {
        int32 Index  = INDEX_NONE;
        float G      = 0.0f;
        float F      = 0.0f;
        int32 Parent = INDEX_NONE;

        bool operator==(const FAStarNode& Other) const { return Index == Other.Index; }
    };

    float OctileDistance(FIntPoint A, FIntPoint B) const;
};
