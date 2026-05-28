#include "Strategic/FRTSTacticalZoneClassifier.h"
#include "Math/UnrealMathUtility.h"

void FRTSTacticalZoneClassifier::Classify(FRTSGrid& Grid, const FRTSMapMetadata& Metadata)
{
    TArray<bool> HGCand; HGCand.SetNumZeroed(Grid.Cells.Num());
    TArray<float> AvgH; AvgH.SetNumZeroed(Grid.Cells.Num());
    for (int32 i=0;i<Grid.Cells.Num();++i)
    {
        FIntPoint Coord=Grid.ToCoord(i); float Sum=0; int32 Cnt=0;
        for (int32 dy=-1;dy<=1;++dy) for (int32 dx=-1;dx<=1;++dx)
        { if (dx==0&&dy==0) continue; if (Grid.IsValidCoord(Coord.X+dx,Coord.Y+dy)) { Sum+=Grid.GetCell(Coord.X+dx,Coord.Y+dy).Height; ++Cnt; } }
        if (Cnt>0) AvgH[i]=Sum/Cnt;
    }
    for (int32 i=0;i<Grid.Cells.Num();++i)
    { const FRTSCell& C=Grid.Cells[i]; if (C.bWalkable&&!C.bWater&&C.Height>0.65f&&C.Height>AvgH[i]+0.04f) HGCand[i]=true; }
    FilterSmallRegions(Grid, HGCand, 12);
    for (int32 i=0;i<Grid.Cells.Num();++i)
    {
        FRTSCell& C=Grid.Cells[i];
        if (!C.bWalkable) { C.TacticalZone=ERTSTacticalZone::Unclassified; continue; }
        if (C.TacticalZone!=ERTSTacticalZone::Unclassified) continue;
        if (HGCand[i]) { C.TacticalZone=ERTSTacticalZone::HighGround; continue; }
        C.TacticalZone=ERTSTacticalZone::OpenBattlefield;
    }
}

void FRTSTacticalZoneClassifier::FilterSmallRegions(FRTSGrid& Grid, TArray<bool>& Mask, int32 MinSize) const
{
    TArray<bool> Visited; Visited.SetNumZeroed(Grid.Cells.Num()); int32 NB[8];
    for (int32 i=0;i<Grid.Cells.Num();++i)
    {
        if (!Mask[i]||Visited[i]) continue;
        TArray<int32> Stack,Region; Stack.Push(i); Visited[i]=true;
        while (Stack.Num()>0) { int32 C=Stack.Pop(); Region.Add(C); int32 N=Grid.GetNeighborsFixed(C,false,NB); for (int32 n=0;n<N;++n) { int32 NIdx=NB[n]; if (!Visited[NIdx]&&Mask[NIdx]) { Visited[NIdx]=true; Stack.Push(NIdx); } } }
        if (Region.Num()<MinSize) for (int32 Idx:Region) Mask[Idx]=false;
    }
}
