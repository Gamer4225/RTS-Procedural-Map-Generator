#include "Strategic/FRTSBridgeDetector.h"
#include "Math/UnrealMathUtility.h"

void FRTSBridgeDetector::DetectCrossings(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    if (Settings->NumPlayers<2) return;
    TArray<FCrossingCandidate> Cands=FindCandidates(Grid);
    for (FCrossingCandidate& C:Cands) { ScoreTraffic(C,Grid,Metadata); ScoreProximity(C,Metadata,Grid); C.OverallScore=(1-FMath::Clamp(C.WidthCells/6.0f,0.0f,1.0f))*0.4f+C.TrafficScore*0.35f+C.ProximityScore*0.25f; }
    Cands.Sort([](const FCrossingCandidate& A,const FCrossingCandidate& B){return A.OverallScore>B.OverallScore;});
    CommitCrossings(Grid,Metadata,Cands,FMath::Clamp(Settings->NumPlayers*2,2,8));
}

TArray<FRTSBridgeDetector::FCrossingCandidate> FRTSBridgeDetector::FindCandidates(const FRTSGrid& Grid) const
{
    TArray<FCrossingCandidate> Out;
    for (int32 Y=1;Y<Grid.Height-1;++Y) for (int32 X=1;X<Grid.Width-1;++X)
    {
        if (!Grid.GetCell(X,Y).bWater) continue;
        bool bH=(Grid.GetCell(X-1,Y).bWalkable&&Grid.GetCell(X+1,Y).bWalkable);
        bool bV=(Grid.GetCell(X,Y-1).bWalkable&&Grid.GetCell(X,Y+1).bWalkable);
        if (!bH&&!bV) continue;
        FCrossingCandidate C=EvaluateCrossing(Grid,X,Y);
        if (C.RegionA!=INDEX_NONE&&C.RegionB!=INDEX_NONE) Out.Add(C);
    }
    return Out;
}

FRTSBridgeDetector::FCrossingCandidate FRTSBridgeDetector::EvaluateCrossing(const FRTSGrid& Grid, int32 X, int32 Y) const
{
    FCrossingCandidate C; C.Position=FIntPoint(X,Y);
    auto L=Grid.GetCell(X-1,Y),R=Grid.GetCell(X+1,Y),U=Grid.GetCell(X,Y-1),D=Grid.GetCell(X,Y+1);
    if (L.bWalkable&&R.bWalkable&&L.RegionID!=R.RegionID) { C.RegionA=L.RegionID;C.RegionB=R.RegionID;C.WidthCells=MeasureWaterWidth(Grid,X,Y,0,1,8)+MeasureWaterWidth(Grid,X,Y,0,-1,8)-1;return C; }
    if (U.bWalkable&&D.bWalkable&&U.RegionID!=D.RegionID) { C.RegionA=U.RegionID;C.RegionB=D.RegionID;C.WidthCells=MeasureWaterWidth(Grid,X,Y,1,0,8)+MeasureWaterWidth(Grid,X,Y,-1,0,8)-1;return C; }
    C.RegionA=C.RegionB=INDEX_NONE; return C;
}

int32 FRTSBridgeDetector::MeasureWaterWidth(const FRTSGrid& Grid, int32 SX, int32 SY, int32 DX, int32 DY, int32 Max) const
{
    int32 Cnt=0,X=SX,Y=SY;
    while (Cnt<Max){X+=DX;Y+=DY;if(!Grid.IsValidCoord(X,Y)||!Grid.GetCell(X,Y).bWater)break;++Cnt;}
    return Cnt+1;
}

void FRTSBridgeDetector::ScoreTraffic(FCrossingCandidate& C, const FRTSGrid& Grid, const FRTSMapMetadata& Meta) const
{
    int32 CA=0,CB=0;
    for (const auto& B:Meta.Bases) { FIntPoint P(FMath::FloorToInt(B.GridPosition.X),FMath::FloorToInt(B.GridPosition.Y)); if (Grid.IsValidCoord(P.X,P.Y)) { int32 R=Grid.GetCell(P.X,P.Y).RegionID; if(R==C.RegionA)++CA;else if(R==C.RegionB)++CB; } }
    float T=(float)(CA+CB),Bal=(CA>0&&CB>0)?1-FMath::Abs(CA-CB)/T:0;
    C.TrafficScore=FMath::Clamp(T/4.0f,0.0f,1.0f)*(0.3f+0.7f*Bal);
}

void FRTSBridgeDetector::ScoreProximity(FCrossingCandidate& C, const FRTSMapMetadata& Meta, const FRTSGrid& Grid) const
{
    float MinB=MAX_FLT,MinE=MAX_FLT,Diag=FMath::Sqrt((float)(Grid.Width*Grid.Width+Grid.Height*Grid.Height));
    for (const auto& B:Meta.Bases) MinB=FMath::Min(MinB,FVector2D::Distance(FVector2D(C.Position.X,C.Position.Y),B.GridPosition));
    for (const auto& E:Meta.Expansions) MinE=FMath::Min(MinE,FVector2D::Distance(FVector2D(C.Position.X,C.Position.Y),E.GridPosition));
    C.ProximityScore=(1-FMath::Clamp(MinB/(Diag*0.3f),0.0f,1.0f))*0.6f+(1-FMath::Clamp(MinE/(Diag*0.25f),0.0f,1.0f))*0.4f;
}

void FRTSBridgeDetector::CommitCrossings(FRTSGrid& Grid, FRTSMapMetadata& Meta, TArray<FCrossingCandidate>& Cands, int32 Max) const
{
    float MinDSq=FMath::Square(FMath::Min(Grid.Width,Grid.Height)*0.08f);
    TArray<FIntPoint> Done;
    for (FCrossingCandidate& C:Cands)
    {
        if (Done.Num()>=Max) break;
        bool bClose=false; for (const FIntPoint& E:Done) if (FIntPoint::DistSquared(C.Position,E)<MinDSq){bClose=true;break;} if (bClose) continue;
        for (int32 dy=-1;dy<=1;++dy) for (int32 dx=-1;dx<=1;++dx) { int32 NX=C.Position.X+dx,NY=C.Position.Y+dy; if (Grid.IsValidCoord(NX,NY)&&Grid.GetCell(NX,NY).bWater) { Grid.GetCell(NX,NY).TacticalZone=ERTSTacticalZone::RiverCrossing; Grid.GetCell(NX,NY).StrategicValue=FMath::Max(Grid.GetCell(NX,NY).StrategicValue,0.6f); } }
        FRTSChokeInfo Ch; Ch.WidthCells=C.WidthCells; Ch.Cells.Add(C.Position); Ch.RegionA=C.RegionA; Ch.RegionB=C.RegionB; Ch.Hardness=1-FMath::Clamp(C.WidthCells/5.0f,0.0f,1.0f);
        Meta.Chokes.Add(Ch); Done.Add(C.Position);
    }
}
