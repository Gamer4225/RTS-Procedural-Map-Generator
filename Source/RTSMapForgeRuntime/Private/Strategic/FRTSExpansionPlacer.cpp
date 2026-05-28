#include "Strategic/FRTSExpansionPlacer.h"
#include "Math/UnrealMathUtility.h"

void FRTSExpansionPlacer::PlaceExpansions(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings||Metadata.Bases.Num()==0) return;
    const int32 W=Grid.Width,H=Grid.Height;
    const float MaxD=FMath::Sqrt((float)(W*W+H*H));
    for (const FRTSBaseInfo& Base:Metadata.Bases)
    {
        TArray<FIntPoint> Cands;
        for (int32 Y=0;Y<H;++Y) for (int32 X=0;X<W;++X)
        {
            FRTSCell& C=Grid.GetCell(X,Y);
            if (!C.bBuildable||C.TacticalZone==ERTSTacticalZone::MainBase) continue;
            float D=FVector2D::Distance(FVector2D(X,Y),Base.GridPosition);
            if (D>MaxD*0.15f&&D<MaxD*0.5f) Cands.Add(FIntPoint(X,Y));
        }
        for (int32 e=0;e<Settings->NumExpansions&&Cands.Num()>0;++e)
        {
            int32 BestIdx=0; float BestScore=-1;
            for (int32 i=0;i<Cands.Num();++i)
            {
                float DB=FVector2D::Distance(FVector2D(Cands[i].X,Cands[i].Y),Base.GridPosition);
                float DC=FVector2D::Distance(FVector2D(Cands[i].X,Cands[i].Y),FVector2D(W*0.5f,H*0.5f));
                float Score=DB*0.3f+DC*0.7f;
                if (Score>BestScore){BestScore=Score;BestIdx=i;}
            }
            FIntPoint Pick=Cands[BestIdx]; Cands.RemoveAt(BestIdx);
            FRTSCell& C=Grid.GetCell(Pick.X,Pick.Y); C.TacticalZone=ERTSTacticalZone::NatExpansion;
            FRTSExpansionInfo Exp; Exp.OwnerPlayerIndex=Base.PlayerIndex; Exp.GridPosition=FVector2D(Pick.X,Pick.Y);
            Exp.WorldPosition=C.WorldPosition; Exp.RiskScore=1-FVector2D::Distance(Exp.GridPosition,Base.GridPosition)/(MaxD*0.5f);
            Exp.bContested=(Exp.RiskScore>0.6f); Metadata.Expansions.Add(Exp);
        }
    }
}
