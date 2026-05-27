#include "Validation/FRTSWaterConnectivityValidator.h"

bool FRTSWaterConnectivityValidator::ValidateWaterConnectivity(
    const FRTSGrid& Grid,
    FRTSValidationResult& OutResult,
    bool& bHasIsolatedWater
) const
{
    bHasIsolatedWater = false;
    
    TArray<bool> WaterReachable;
    FloodFillWaterEdgeReachable(Grid, WaterReachable);

    int32 IsolatedCount = 0;
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        if (Grid.Cells[i].bWater && !WaterReachable[i])
        {
            ++IsolatedCount;
        }
    }

    if (IsolatedCount > 0)
    {
        bHasIsolatedWater = true;
        OutResult.Issues.Add(FRTSValidationIssue{
            TEXT("WaterConnectivity"),
            FString::Printf(TEXT("%d isolated water cells (not connected to map edge)"), IsolatedCount),
            ERTSValidationSeverity::Warning
        });
    }

    return !bHasIsolatedWater;
}

int32 FRTSWaterConnectivityValidator::CountIsolatedLandRegions(const FRTSGrid& Grid) const
{
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    
    TArray<bool> Visited;
    Visited.SetNumZeroed(Grid.Cells.Num());
    
    int32 RegionCount = 0;
    
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        if (Visited[i] || Grid.Cells[i].bWater)
        {
            continue;
        }

        // BFS this land region
        TArray<int32> Stack;
        Stack.Push(i);
        Visited[i] = true;
        bool bTouchesEdge = false;
        int32 RegionSize = 0;

        while (Stack.Num() > 0)
        {
            int32 Current = Stack.Pop();
            ++RegionSize;
            
            FIntPoint Coord = Grid.ToCoord(Current);
            if (Coord.X == 0 || Coord.X == W - 1 || Coord.Y == 0 || Coord.Y == H - 1)
            {
                bTouchesEdge = true;
            }

            int32 Neighbors[8];
            int32 NumNeighbors = Grid.GetNeighborsFixed(Current, false, Neighbors);
            for (int32 n = 0; n < NumNeighbors; ++n)
            {
                int32 NIdx = Neighbors[n];
                if (!Visited[NIdx] && !Grid.Cells[NIdx].bWater)
                {
                    Visited[NIdx] = true;
                    Stack.Push(NIdx);
                }
            }
        }

        // If land region doesn't touch edge and is small, it's isolated
        if (!bTouchesEdge && RegionSize < (W * H) / 20) // Less than 5% of map
        {
            ++RegionCount;
        }
    }

    return RegionCount;
}

void FRTSWaterConnectivityValidator::FloodFillWaterEdgeReachable(const FRTSGrid& Grid, TArray<bool>& OutReachable) const
{
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    
    OutReachable.SetNumZeroed(Grid.Cells.Num());
    TArray<int32> Stack;

    // Seed from all water cells on the map edges
    for (int32 X = 0; X < W; ++X)
    {
        if (Grid.Cells[Grid.ToIndex(X, 0)].bWater && !OutReachable[Grid.ToIndex(X, 0)])
        {
            Stack.Push(Grid.ToIndex(X, 0));
            OutReachable[Grid.ToIndex(X, 0)] = true;
        }
        if (Grid.Cells[Grid.ToIndex(X, H - 1)].bWater && !OutReachable[Grid.ToIndex(X, H - 1)])
        {
            Stack.Push(Grid.ToIndex(X, H - 1));
            OutReachable[Grid.ToIndex(X, H - 1)] = true;
        }
    }
    for (int32 Y = 1; Y < H - 1; ++Y)
    {
        if (Grid.Cells[Grid.ToIndex(0, Y)].bWater && !OutReachable[Grid.ToIndex(0, Y)])
        {
            Stack.Push(Grid.ToIndex(0, Y));
            OutReachable[Grid.ToIndex(0, Y)] = true;
        }
        if (Grid.Cells[Grid.ToIndex(W - 1, Y)].bWater && !OutReachable[Grid.ToIndex(W - 1, Y)])
        {
            Stack.Push(Grid.ToIndex(W - 1, Y));
            OutReachable[Grid.ToIndex(W - 1, Y)] = true;
        }
    }

    // BFS water connectivity (4-dir for water bodies)
    int32 NeighborBuffer[8];
    while (Stack.Num() > 0)
    {
        int32 Current = Stack.Pop();
        int32 NumNeighbors = Grid.GetNeighborsFixed(Current, false, NeighborBuffer);
        
        for (int32 n = 0; n < NumNeighbors; ++n)
        {
            int32 NIdx = NeighborBuffer[n];
            if (!OutReachable[NIdx] && Grid.Cells[NIdx].bWater)
            {
                OutReachable[NIdx] = true;
                Stack.Push(NIdx);
            }
        }
    }
}
