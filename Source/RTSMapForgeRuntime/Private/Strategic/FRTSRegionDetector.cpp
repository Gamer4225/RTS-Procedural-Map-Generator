#include "Strategic/FRTSRegionDetector.h"

void FRTSRegionDetector::DetectRegions(FRTSGrid& Grid)
{
    RegionSizes.Reset();
    const int32 TotalCells = Grid.Cells.Num();

    TArray<bool> Visited;
    Visited.SetNumZeroed(TotalCells);

    int32 CurrentRegionID = 0;

    // Reusable fixed neighbor buffer (zero allocation per iteration)
    int32 NeighborBuffer[8];

    for (int32 i = 0; i < TotalCells; ++i)
    {
        if (Visited[i] || !Grid.Cells[i].bWalkable)
        {
            continue;
        }

        // BFS flood fill using a stack array
        TArray<int32> Stack;
        Stack.Reserve(64);
        Stack.Push(i);
        Visited[i] = true;
        int32 RegionCellCount = 0;

        while (Stack.Num() > 0)
        {
            int32 Current = Stack.Pop();
            Grid.Cells[Current].RegionID = CurrentRegionID;
            ++RegionCellCount;

            // === ZERO-ALLOCATION neighbor lookup (4-dir) ===
            int32 NumNeighbors = Grid.GetNeighborsFixed(Current, false, NeighborBuffer);

            for (int32 n = 0; n < NumNeighbors; ++n)
            {
                int32 Neighbor = NeighborBuffer[n];
                if (!Visited[Neighbor] && Grid.Cells[Neighbor].bWalkable)
                {
                    Visited[Neighbor] = true;
                    Stack.Push(Neighbor);
                }
            }
        }

        RegionSizes.Add(CurrentRegionID, RegionCellCount);
        ++CurrentRegionID;
    }
}

int32 FRTSRegionDetector::GetRegionCellCount(int32 RegionID) const
{
    if (const int32* Count = RegionSizes.Find(RegionID))
    {
        return *Count;
    }
    return 0;
}
