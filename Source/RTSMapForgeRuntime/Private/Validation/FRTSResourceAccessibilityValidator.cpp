#include "Validation/FRTSResourceAccessibilityValidator.h"
#include "Pathfinding/FRTSAStarSolver.h"
#include "Math/UnrealMathUtility.h"

void FRTSResourceAccessibilityValidator::Validate(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) const
{
    if (Metadata.Bases.Num()<2) return;
    TArray<FResourceAccessibility> Resources=GatherResources(Grid,Metadata);
    if (Resources.Num()==0) return;
    for (FResourceAccessibility& R:Resources) { ComputeAccessibility(R,Grid,Metadata); ComputeSafety(R,Grid,Metadata); }
    TMap<int32,float> AccScore,SafScore; TMap<int32,int32> Cnt;
    for (const FResourceAccessibility& R:Resources)
    {
        float BD=MAX_FLT; int32 BP=INDEX_NONE;
        for (const FRTSBaseInfo& B:Metadata.Bases) { float D=FVector2D::Distance(R.Position,B.GridPosition); if (D<BD){BD=D;BP=B.PlayerIndex;} }
        if (BP==INDEX_NONE) continue;
        float A=(1.0f/(1.0f+R.PathCostFromBase*0.01f))*0.4f+R.SafetyScore*0.35f+(1.0f/(1.0f+R.ChokesEnRoute+R.RiverCrossings))*0.25f;
        AccScore.FindOrAdd(BP)+=A*R.ResourceValue; SafScore.FindOrAdd(BP)+=R.SafetyScore*R.ResourceValue; Cnt.FindOrAdd(BP)++;
    }
    if (AccScore.Num()<2) return;
    float MaxA=0,MinA=MAX_FLT,MaxS=0,MinS=MAX_FLT;
    for (const auto& P:AccScore){MaxA=FMath::Max(MaxA,P.Value);MinA=FMath::Min(MinA,P.Value);}
    for (const auto& P:SafScore){MaxS=FMath::Max(MaxS,P.Value);MinS=FMath::Min(MinS,P.Value);}
    float DA=(MaxA-MinA)/FMath::Max(MaxA,1.0f), DS=(MaxS-MinS)/FMath::Max(MaxS,1.0f);
    if (DA>Settings->MaxFairnessError) OutResult.Issues.Add({TEXT("ResourceAccessibility"),FString::Printf(TEXT("Resource accessibility imbalance %.0f%%"),DA*100),ERTSValidationSeverity::Warning});
    if (DS>Settings->MaxFairnessError) OutResult.Issues.Add({TEXT("ResourceSafety"),FString::Printf(TEXT("Resource safety imbalance %.0f%%"),DS*100),ERTSValidationSeverity::Warning});
}

TArray<FRTSResourceAccessibilityValidator::FResourceAccessibility> FRTSResourceAccessibilityValidator::GatherResources(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata) const
{
    TArray<FResourceAccessibility> Out;
    for (int32 Y=0;Y<Grid.Height;++Y) for (int32 X=0;X<Grid.Width;++X)
    { const FRTSCell& C=Grid.GetCell(X,Y); if (C.ResourceValue>0.1f) { FResourceAccessibility R; R.Position=FVector2D(X,Y); R.ResourceValue=C.ResourceValue; Out.Add(R); } }
    return Out;
}

void FRTSResourceAccessibilityValidator::ComputeAccessibility(FResourceAccessibility& R, const FRTSGrid& Grid, const FRTSMapMetadata& Metadata) const
{
    FRTSAStarSolver Solver; float Best=MAX_FLT; int32 BestIdx=INDEX_NONE;
    FIntPoint RP(FMath::FloorToInt(R.Position.X),FMath::FloorToInt(R.Position.Y));
    for (int32 i=0;i<Metadata.Bases.Num();++i)
    { FIntPoint BP(FMath::FloorToInt(Metadata.Bases[i].GridPosition.X),FMath::FloorToInt(Metadata.Bases[i].GridPosition.Y)); float C=Solver.FindPathCost(Grid,RP,BP,30000); if (C>=0&&C<Best){Best=C;BestIdx=i;} }
    R.PathCostFromBase=(BestIdx==INDEX_NONE)?MAX_FLT:Best;
}

void FRTSResourceAccessibilityValidator::ComputeSafety(FResourceAccessibility& R, const FRTSGrid& Grid, const FRTSMapMetadata& Metadata) const
{
    float BD=MAX_FLT; int32 Owner=INDEX_NONE;
    for (const FRTSBaseInfo& B:Metadata.Bases) { float D=FVector2D::Distance(R.Position,B.GridPosition); if (D<BD){BD=D;Owner=B.PlayerIndex;} }
    if (Owner==INDEX_NONE){R.SafetyScore=0;return;}
    float ESum=0; int32 EC=0;
    for (const FRTSBaseInfo& B:Metadata.Bases) if (B.PlayerIndex!=Owner){ESum+=FVector2D::Distance(R.Position,B.GridPosition);++EC;}
    R.SafetyScore=(EC==0)?1.0f:FMath::Clamp((ESum/EC)/FMath::Max(BD,1.0f)/3.0f,0.0f,1.0f);
}

bool FRTSResourceAccessibilityValidator::TracePathForObstacles(const FRTSGrid& Grid, FIntPoint Start, FIntPoint End, int32& OutChokes, int32& OutCross, float& OutCost) const
{
    OutChokes=OutCross=0; OutCost=-1;
    int32 X0=Start.X,Y0=Start.Y,X1=End.X,Y1=End.Y;
    int32 Dx=FMath::Abs(X1-X0),Dy=FMath::Abs(Y1-Y0),Sx=(X0<X1)?1:-1,Sy=(Y0<Y1)?1:-1,Err=Dx-Dy,Steps=0;
    TSet<FIntPoint> CC,CR;
    while (Steps<Grid.Width+Grid.Height)
    {
        if (!Grid.IsValidCoord(X0,Y0)) return false;
        const FRTSCell& C=Grid.GetCell(X0,Y0); FIntPoint P(X0,Y0);
        if (C.TacticalZone==ERTSTacticalZone::ChokePoint&&!CC.Contains(P)){++OutChokes;CC.Add(P);}
        if (C.TacticalZone==ERTSTacticalZone::RiverCrossing&&!CR.Contains(P)){++OutCross;CR.Add(P);}
        if (X0==X1&&Y0==Y1) break;
        int32 E2=2*Err; if (E2>-Dy){Err-=Dy;X0+=Sx;} if (E2<Dx){Err+=Dx;Y0+=Sy;} ++Steps;
    }
    return true;
}
