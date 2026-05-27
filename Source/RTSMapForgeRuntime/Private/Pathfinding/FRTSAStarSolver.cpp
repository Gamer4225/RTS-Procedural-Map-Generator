#include "Pathfinding/FRTSAStarSolver.h"
#include "Math/UnrealMathUtility.h"

void FRTSAStarSolver::ComputeRushDistances(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    const int32 NumBases = Metadata.Bases.Num();
    for (int32 i = 0; i < NumBases; ++i)
    {
        for (int32 j = i + 1; j < NumBases; ++j)
        {
            FIntPoint A(FMath::FloorToInt(Metadata.Bases[i].GridPosition.X), FMath::FloorToInt(Metadata.Bases[i].GridPosition.Y));
            FIntPoint B(FMath::FloorToInt(Metadata.Bases[j].GridPosition.X), FMath::FloorToInt(Metadata.Bases[j].GridPosition.Y));
            float Cost = FindPathCost(Grid, A, B);
            // Store could go into metadata or heatmap later
        }
    }
}

float FRTSAStarSolver::FindPathCost(const FRTSGrid& Grid, FIntPoint Start, FIntPoint Goal, int32 MaxIterations) const
{
    if (!Grid.IsValidCoord(Start.X, Start.Y) || !Grid.IsValidCoord(Goal.X, Goal.Y))
    {
        return -1.0f;
    }

    int32 StartIdx = Grid.ToIndex(Start.X, Start.Y);
    int32 GoalIdx = Grid.ToIndex(Goal.X, Goal.Y);

    if (StartIdx == GoalIdx)
    {
        return 0.0f;
    }

    // Open set as array with linear search (V1: grids are moderate; upgrade to heap for large maps)
    TArray<FAStarNode> Open;
    TSet<int32> Closed;

    FAStarNode StartNode;
    StartNode.Index = StartIdx;
    StartNode.G = 0.0f;
    StartNode.F = OctileDistance(Start, Goal);
    StartNode.Parent = INDEX_NONE;
    Open.Add(StartNode);

    // Reusable fixed neighbor buffer (zero allocation per iteration)
    int32 NeighborBuffer[8];

    int32 Iterations = 0;
    while (Open.Num() > 0 && Iterations < MaxIterations)
    {
        ++Iterations;

        // Find best node
        int32 BestIdx = 0;
        float BestF = Open[0].F;
        for (int32 i = 1; i < Open.Num(); ++i)
        {
            if (Open[i].F < BestF)
            {
                BestF = Open[i].F;
                BestIdx = i;
            }
        }

        FAStarNode Current = Open[BestIdx];
        Open.RemoveAtSwap(BestIdx);
        Closed.Add(Current.Index);

        if (Current.Index == GoalIdx)
        {
            return Current.G;
        }

        // === ZERO-ALLOCATION neighbor lookup ===
        int32 NumNeighbors = Grid.GetNeighborsFixed(Current.Index, true, NeighborBuffer);

        for (int32 n = 0; n < NumNeighbors; ++n)
        {
            int32 NeighborIdx = NeighborBuffer[n];
            if (Closed.Contains(NeighborIdx))
            {
                continue;
            }
            const FRTSCell& NeighborCell = Grid.GetCell(NeighborIdx);
            if (!NeighborCell.bWalkable)
            {
                continue;
            }

            float StepCost = NeighborCell.MovementCostMultiplier;
            if (StepCost <= 0.0f)
            {
                continue;
            }

            float NewG = Current.G + StepCost;
            FIntPoint NCoord = Grid.ToCoord(NeighborIdx);
            float H = OctileDistance(NCoord, Goal);
            float NewF = NewG + H;

            bool bInOpen = false;
            for (FAStarNode& Node : Open)
            {
                if (Node.Index == NeighborIdx)
                {
                    bInOpen = true;
                    if (NewG < Node.G)
                    {
                        Node.G = NewG;
                        Node.F = NewF;
                        Node.Parent = Current.Index;
                    }
                    break;
                }
            }

            if (!bInOpen)
            {
                FAStarNode NewNode;
                NewNode.Index = NeighborIdx;
                NewNode.G = NewG;
                NewNode.F = NewF;
                NewNode.Parent = Current.Index;
                Open.Add(NewNode);
            }
        }
    }

    return -1.0f; // Unreachable
}

float FRTSAStarSolver::OctileDistance(FIntPoint A, FIntPoint B) const
{
    int32 dx = FMath::Abs(A.X - B.X);
    int32 dy = FMath::Abs(A.Y - B.Y);
    const float D = 1.0f;
    const float D2 = FMath::Sqrt(2.0f);
    return D * FMath::Max(dx, dy) + (D2 - D) * FMath::Min(dx, dy);
}
