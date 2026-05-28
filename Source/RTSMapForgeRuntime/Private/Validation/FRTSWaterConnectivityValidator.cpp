#include "Validation/FRTSWaterConnectivityValidator.h"

bool FRTSWaterConnectivityValidator::ValidateWaterConnectivity(const FRTSGrid& Grid, FRTSValidationResult& OutResult, bool& bHasIsolated) const
{
    bHasIsolated=false;
    TArray<bool> Reachable; FloodFillWaterEdgeReachable(Grid, Reachable);
    int32 Cnt=0; for (int32 i=0;i<Grid.Cells.Num();++i) if (Grid.Cells[i].bWater&&!Reachable[i]) ++Cnt;
    if (Cnt>0) { bHasIsolated=true; OutResult.Issues.Add({TEXT("WaterConnectivity"),FString::Printf(TEXT("%d isolated water cells"),Cnt),ERTSValidationSeverity::Warning}); }
    return !bHasIsolated;
}

int32 FRTSWaterConnectivityValidator::CountIsolatedLandRegions(const FRTSGrid& Grid) const
{
    TArray<bool> Visited; Visited.SetNumZeroed(Grid.Cells.Num()); int32 Count=0;
    for (int32 i=0;i<Grid.Cells.Num();++i)
    {
        if (Visited[i]||Grid.Cells[i].bWater) continue;
        TArray<int32> Stack; Stack.Push(i); Visited[i]=true; bool bEdge=false; int32 Size=0;
        while (Stack.Num()>0) { int32 C=Stack.Pop(); ++Size; FIntPoint Coord=Grid.ToCoord(C); if (Coord.X==0||Coord.X==Grid.Width-1||Coord.Y==0||Coord.Y==Grid.Height-1) bEdge=true; int32 NB[8]; int32 N=Grid.GetNeighborsFixed(C,false,NB); for (int32 n=0;n<N;++n) { int32 NIdx=NB[n]; if (!Visited[NIdx]&&!Grid.Cells[NIdx].bWater) { Visited[NIdx]=true; Stack.Push(NIdx); } } }
        if (!bEdge && Size<(Grid.Width*Grid.Height)/20) ++Count;
    }
    return Count;
}

void FRTSWaterConnectivityValidator::FloodFillWaterEdgeReachable(const FRTSGrid& Grid, TArray<bool>& Out) const
{
    Out.SetNumZeroed(Grid.Cells.Num()); TArray<int32> Stack;
    auto Push=[&](int32 i){ if (Grid.Cells[i].bWater&&!Out[i]) { Out[i]=true; Stack.Push(i); } };
    for (int32 X=0;X<Grid.Width;++X) { Push(Grid.ToIndex(X,0)); Push(Grid.ToIndex(X,Grid.Height-1)); }
    for (int32 Y=1;Y<Grid.Height-1;++Y) { Push(Grid.ToIndex(0,Y)); Push(Grid.ToIndex(Grid.Width-1,Y)); }
    int32 NB[8];
    while (Stack.Num()>0) { int32 C=Stack.Pop(); int32 N=Grid.GetNeighborsFixed(C,false,NB); for (int32 n=0;n<N;++n) { int32 NIdx=NB[n]; if (!Out[NIdx]&&Grid.Cells[NIdx].bWater) { Out[NIdx]=true; Stack.Push(NIdx); } } }
}
