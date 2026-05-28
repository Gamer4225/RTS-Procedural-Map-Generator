#include "Strategic/FRTSResourcePlacer.h"
#include "Math/UnrealMathUtility.h"

void FRTSResourcePlacer::PlaceResources(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings||!SeedManager||Grid.Cells.Num()==0) return;
    const float MinDist = FMath::Max(Grid.Width,Grid.Height)*0.06f, MinDistSq=MinDist*MinDist;
    const int32 MaxRes = FMath::Clamp((Grid.Width*Grid.Height)/200, 8, 64);
    TArray<FCandidate> Cands = GatherCandidates(Grid, Metadata, Settings);
    if (Cands.Num()==0) return;
    Cands.Sort([](const FCandidate& A, const FCandidate& B){
        uint32 HA=(uint32)(A.Position.X*73856093u^A.Position.Y*19349663u);
        uint32 HB=(uint32)(B.Position.X*73856093u^B.Position.Y*19349663u);
        return HA<HB;
    });
    TArray<FIntPoint> Placed;
    for (const FCandidate& C : Cands)
    {
        if (Placed.Num()>=MaxRes) break;
        bool bClose=false;
        for (const FIntPoint& E : Placed) if (FIntPoint::DistSquared(C.Position,E)<MinDistSq){bClose=true;break;}
        if (bClose) continue;
        Placed.Add(C.Position);
        FRTSCell& Cell = Grid.GetCell(C.Position.X,C.Position.Y);
        Cell.ResourceValue = FMath::Clamp(C.Score,0.0f,1.0f);
        if (Cell.ResourceValue>0.5f) Cell.TacticalZone=ERTSTacticalZone::ResourceCluster;
    }
}

TArray<FRTSResourcePlacer::FCandidate> FRTSResourcePlacer::GatherCandidates(FRTSGrid& Grid, const FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings) const
{
    TArray<FCandidate> Out;
    for (int32 Y=1;Y<Grid.Height-1;++Y)
    for (int32 X=1;X<Grid.Width-1;++X)
    {
        const FRTSCell& C=Grid.GetCell(X,Y);
        if (!C.bWalkable||!C.bBuildable||C.bWater||C.bCliff) continue;
        float Score=ScoreCell(Grid,X,Y,Metadata);
        if (Score>0.2f) Out.Add({FIntPoint(X,Y),Score});
    }
    return Out;
}

float FRTSResourcePlacer::ScoreCell(const FRTSGrid& Grid, int32 X, int32 Y, const FRTSMapMetadata& Metadata) const
{
    float Score=0, BestD=MAX_FLT;
    float Diag=FMath::Sqrt((float)(Grid.Width*Grid.Width+Grid.Height*Grid.Height));
    for (const auto& E : Metadata.Expansions) BestD=FMath::Min(BestD,FVector2D::Distance(FVector2D(X,Y),E.GridPosition));
    if (BestD<MAX_FLT) Score+=(1.0f-FMath::Clamp(BestD/(Diag*0.3f),0.0f,1.0f))*0.4f;
    if (Grid.GetCell(X,Y).Height>0.6f) Score+=0.2f;
    uint32 H=(uint32)(X*73856093u^Y*19349663u);
    Score+=((float)(H%1000)/1000.0f)*0.1f;
    return Score;
}
