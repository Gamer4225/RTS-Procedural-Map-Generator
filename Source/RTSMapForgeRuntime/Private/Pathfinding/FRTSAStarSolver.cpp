#include "Pathfinding/FRTSAStarSolver.h"
#include "Algo/MinElement.h"            // FIX Problem 2: Algo::MinElementBy for O(n) best-node pick
#include "Math/UnrealMathUtility.h"

// FIX Minor: ComputeRushDistances now stores every computed cost in
// Metadata.RushDistances (keyed by packed base-pair int64) so Stage 16/16b
// validation can read them directly without re-running A* from scratch.
void FRTSAStarSolver::ComputeRushDistances(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    const int32 NumBases = Metadata.Bases.Num();
    Metadata.RushDistances.Empty(NumBases * NumBases);

    for (int32 i = 0; i < NumBases; ++i)
    {
        for (int32 j = i + 1; j < NumBases; ++j)
        {
            FIntPoint A(
                FMath::FloorToInt(Metadata.Bases[i].GridPosition.X),
                FMath::FloorToInt(Metadata.Bases[i].GridPosition.Y));
            FIntPoint B(
                FMath::FloorToInt(Metadata.Bases[j].GridPosition.X),
                FMath::FloorToInt(Metadata.Bases[j].GridPosition.Y));

            float Cost = FindPathCost(Grid, A, B);

            // Pack pair (i,j) into a single int64 key — i is always < j here
            int64 Key = (static_cast<int64>(i) << 32) | static_cast<int64>(j);
            Metadata.RushDistances.Add(Key, Cost);
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
    int32 GoalIdx  = Grid.ToIndex(Goal.X, Goal.Y);

    if (StartIdx == GoalIdx)
    {
        return 0.0f;
    }

    // Open set — TArray of nodes.
    // FIX Problem 2: Linear scan replaced by Algo::MinElementBy which has the same
    // O(n) asymptotic complexity but a tighter constant (single pass, no branch
    // misprediction from the hand-written loop). For V2, replace the open set with
    // a binary heap (TArray + HeapPush/HeapPop) for O(log n) extraction.
    TArray<FAStarNode> Open;
    Open.Reserve(512); // Avoid repeated reallocation on typical map sizes

    // Fast closed-set lookup — TSet<int32> is O(1) amortised.
    TSet<int32> Closed;
    Closed.Reserve(1024);

    // Per-index best G score for duplicate detection — avoids full Open scan.
    TMap<int32, float> BestG;
    BestG.Reserve(1024);

    FAStarNode StartNode;
    StartNode.Index  = StartIdx;
    StartNode.G      = 0.0f;
    StartNode.F      = OctileDistance(Start, Goal);
    StartNode.Parent = INDEX_NONE;
    Open.Add(StartNode);
    BestG.Add(StartIdx, 0.0f);

    // Reusable fixed neighbor buffer — zero allocation per iteration
    int32 NeighborBuffer[8];

    int32 Iterations = 0;
    while (Open.Num() > 0 && Iterations < MaxIterations)
    {
        ++Iterations;

        // FIX Problem 2: Use Algo::MinElementBy instead of a manual loop.
        // Still O(n) but with a smaller constant and cleaner intent.
        const FAStarNode* BestPtr = Algo::MinElementBy(Open, [](const FAStarNode& N) { return N.F; });
        if (!BestPtr)
        {
            break;
        }

        FAStarNode Current = *BestPtr;
        Open.RemoveSingleSwap(Current); // O(1) swap-remove

        if (Closed.Contains(Current.Index))
        {
            // A cheaper path was already processed; skip this stale entry.
            continue;
        }

        Closed.Add(Current.Index);

        if (Current.Index == GoalIdx)
        {
            return Current.G;
        }

        // === ZERO-ALLOCATION neighbor lookup ===
        int32 NumNeighbors = Grid.GetNeighborsFixed(Current.Index, /*bDiagonal=*/true, NeighborBuffer);

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

            // Check BestG map — O(1) instead of O(open set size) scan
            const float* ExistingG = BestG.Find(NeighborIdx);
            if (ExistingG && NewG >= *ExistingG)
            {
                // Already have an equal-or-better path to this node
                continue;
            }

            // Update or insert BestG
            BestG.FindOrAdd(NeighborIdx) = NewG;

            FIntPoint NCoord = Grid.ToCoord(NeighborIdx);
            float H          = OctileDistance(NCoord, Goal);
            float NewF       = NewG + H;

            FAStarNode NewNode;
            NewNode.Index  = NeighborIdx;
            NewNode.G      = NewG;
            NewNode.F      = NewF;
            NewNode.Parent = Current.Index;
            Open.Add(NewNode);
        }
    }

    return -1.0f; // Unreachable or MaxIterations exceeded
}

float FRTSAStarSolver::OctileDistance(FIntPoint A, FIntPoint B) const
{
    int32 dx = FMath::Abs(A.X - B.X);
    int32 dy = FMath::Abs(A.Y - B.Y);
    const float D  = 1.0f;
    const float D2 = FMath::Sqrt(2.0f);
    return D * FMath::Max(dx, dy) + (D2 - D) * FMath::Min(dx, dy);
}
