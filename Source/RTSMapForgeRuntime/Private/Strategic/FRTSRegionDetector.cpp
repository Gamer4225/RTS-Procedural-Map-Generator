#include "Strategic/FRTSRegionDetector.h"

void FRTSRegionDetector::DetectRegions(FRTSGrid& Grid)
{
    RegionSizes.Reset();
    TArray<bool> Visited; Visited.SetNumZeroed(Grid.Cells.Num());
    int32 ID = 0, NeighborBuffer[8];
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        if (Visited[i] || !Grid.Cells[i].bWalkable) continue;
        TArray<int32> Stack; Stack.Reserve(64); Stack.Push(i); Visited[i] = true; int32 Count = 0;
        while (Stack.Num() > 0)
        {
            int32 C = Stack.Pop(); Grid.Cells[C].RegionID = ID; ++Count;
            int32 N = Grid.GetNeighborsFixed(C, false, NeighborBuffer);
            for (int32 n = 0; n < N; ++n) { int32 NIdx = NeighborBuffer[n]; if (!Visited[NIdx] && Grid.Cells[NIdx].bWalkable) { Visited[NIdx]=true; Stack.Push(NIdx); } }
        }
        RegionSizes.Add(ID++, Count);
    }
}

int32 FRTSRegionDetector::GetRegionCellCount(int32 RegionID) const
{
    if (const int32* C = RegionSizes.Find(RegionID)) return *C; return 0;
}
