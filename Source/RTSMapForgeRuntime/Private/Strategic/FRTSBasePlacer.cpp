#include "Strategic/FRTSBasePlacer.h"
#include "Math/UnrealMathUtility.h"

void FRTSBasePlacer::PlaceBases(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings||!SeedManager) return;
    Metadata.Bases.Empty();
    const int32 W=Grid.Width,H=Grid.Height,NP=Settings->NumPlayers;
    TArray<FIntPoint> Cands=FindCandidateCells(Grid,64);
    if (Cands.Num()==0) return;
    bool bSym=(Settings->SymmetryStrength>=0.8f&&NP==2);
    if (bSym&&Cands.Num()>0)
    {
        FIntPoint P1=PickCandidate(Cands,SeedManager);
        int32 EdgeM=FMath::Max(W,H)/6, Att=0;
        while (Att<50&&!(P1.X>EdgeM&&P1.X<W-EdgeM&&P1.Y>EdgeM&&P1.Y<H-EdgeM)){P1=PickCandidate(Cands,SeedManager);++Att;}
        FIntPoint P2=ApplySymmetry(P1,W,H,NP,1);
        auto AddBase=[&](int32 PI,const FIntPoint& Pos)->bool
        {
            if (!Grid.IsValidCoord(Pos.X,Pos.Y)) return false;
            FRTSCell& C=Grid.GetCell(Pos.X,Pos.Y);
            if (!C.bWalkable||!C.bBuildable||!IsAreaBuildable(Grid,Pos.X,Pos.Y,5)) return false;
            FRTSBaseInfo I; I.PlayerIndex=PI; I.GridPosition=FVector2D(Pos.X,Pos.Y); I.WorldPosition=C.WorldPosition; I.RegionID=C.RegionID;
            Metadata.Bases.Add(I); C.TacticalZone=ERTSTacticalZone::MainBase; return true;
        };
        if (AddBase(0,P1)&&AddBase(1,P2)) return;
        Metadata.Bases.Empty();
    }
    TArray<FIntPoint> Used;
    float MinDSq=FMath::Square(FMath::Sqrt((float)(W*W+H*H))*Settings->MinRushDistance*0.5f);
    for (int32 p=0;p<NP;++p)
    {
        FIntPoint Best; float BestScore=-1; int32 Att=0;
        while (Att<200)
        {
            FIntPoint C=PickCandidate(Cands,SeedManager); bool bClose=false;
            for (const FIntPoint& E:Used) if ((float)FIntPoint::DistSquared(C,E)<MinDSq){bClose=true;break;}
            if (!bClose)
            {
                float DX=FMath::Abs(C.X-W/2)/(float)W, DY=FMath::Abs(C.Y-H/2)/(float)H, Score=1-(DX+DY);
                if (IsAreaBuildable(Grid,C.X,C.Y,5)&&Score>BestScore){BestScore=Score;Best=C;}
            }
            ++Att;
        }
        if (BestScore>=0)
        {
            Used.Add(Best); FRTSCell& C=Grid.GetCell(Best.X,Best.Y);
            FRTSBaseInfo I; I.PlayerIndex=p; I.GridPosition=FVector2D(Best.X,Best.Y); I.WorldPosition=C.WorldPosition; I.RegionID=C.RegionID;
            Metadata.Bases.Add(I); C.TacticalZone=ERTSTacticalZone::MainBase;
        }
    }
}

TArray<FIntPoint> FRTSBasePlacer::FindCandidateCells(const FRTSGrid& Grid, int32 MinRS) const
{
    TMap<int32,int32> RC;
    for (const FRTSCell& C:Grid.Cells) if (C.RegionID!=INDEX_NONE&&C.bWalkable&&C.bBuildable) RC.FindOrAdd(C.RegionID)++;
    TArray<FIntPoint> Out;
    for (int32 Y=0;Y<Grid.Height;++Y) for (int32 X=0;X<Grid.Width;++X)
    { const FRTSCell& C=Grid.GetCell(X,Y); if (C.bWalkable&&C.bBuildable&&RC.FindRef(C.RegionID)>=MinRS) Out.Add(FIntPoint(X,Y)); }
    return Out;
}

FIntPoint FRTSBasePlacer::PickCandidate(const TArray<FIntPoint>& C, UFRTSSeedManager* S) const { if (C.Num()==0) return FIntPoint::ZeroValue; return C[S->RandRange(0,C.Num()-1)]; }

bool FRTSBasePlacer::IsAreaBuildable(const FRTSGrid& Grid, int32 X, int32 Y, int32 R) const
{
    for (int32 dy=-R;dy<=R;++dy) for (int32 dx=-R;dx<=R;++dx) { if (!Grid.IsValidCoord(X+dx,Y+dy)||!Grid.GetCell(X+dx,Y+dy).bBuildable) return false; }
    return true;
}

FIntPoint FRTSBasePlacer::ApplySymmetry(const FIntPoint& P, int32 W, int32 H, int32 NP, int32 PI) const
{ if (NP==2&&PI==1) return FIntPoint(W-1-P.X,H-1-P.Y); return P; }
