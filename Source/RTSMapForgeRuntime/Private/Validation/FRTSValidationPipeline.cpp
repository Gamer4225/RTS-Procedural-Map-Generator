#include "Validation/FRTSValidationPipeline.h"
#include "Pathfinding/FRTSAStarSolver.h"
#include "Math/UnrealMathUtility.h"

void FRTSValidationPipeline::Validate(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings)
{
    OutResult.Issues.Empty(); OutResult.bPassed=false;
    Pass1_Traversal(Grid,Metadata,OutResult,Settings);
    Pass2_Spawn(Grid,Metadata,OutResult);
    Pass3_Economy(Grid,Metadata,OutResult,Settings);
    Pass4_Choke(Grid,Metadata,OutResult);
    Pass5_Navmesh(Grid,Metadata,OutResult,Settings);
    Pass6_Fairness(Grid,Metadata,OutResult,Settings);
    OutResult.bPassed = !OutResult.HasCriticalFailure();
}

void FRTSValidationPipeline::Pass1_Traversal(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) const
{
    FRTSAStarSolver Solver;
    const int32 NB = Metadata.Bases.Num();
    for (int32 i=0;i<NB;++i) for (int32 j=i+1;j<NB;++j)
    {
        FIntPoint A(FMath::FloorToInt(Metadata.Bases[i].GridPosition.X),FMath::FloorToInt(Metadata.Bases[i].GridPosition.Y));
        FIntPoint B(FMath::FloorToInt(Metadata.Bases[j].GridPosition.X),FMath::FloorToInt(Metadata.Bases[j].GridPosition.Y));
        float Cost=Solver.FindPathCost(Grid,A,B,50000);
        if (Cost<0) OutResult.Issues.Add({TEXT("Traversal"),FString::Printf(TEXT("Base %d UNREACHABLE from Base %d"),i,j),ERTSValidationSeverity::Critical});
        else
        {
            float Diag=FMath::Sqrt((float)(Grid.Width*Grid.Width+Grid.Height*Grid.Height));
            if (Cost/Diag < Settings->MinRushDistance*0.5f)
                OutResult.Issues.Add({TEXT("Traversal"),FString::Printf(TEXT("Base %d→%d rush distance %.2f very short"),i,j,Cost/Diag),ERTSValidationSeverity::Warning});
        }
    }
    for (const FRTSExpansionInfo& Exp : Metadata.Expansions)
    {
        FIntPoint EP(FMath::FloorToInt(Exp.GridPosition.X),FMath::FloorToInt(Exp.GridPosition.Y));
        bool bR=false;
        for (const FRTSBaseInfo& Base : Metadata.Bases) { FIntPoint BP(FMath::FloorToInt(Base.GridPosition.X),FMath::FloorToInt(Base.GridPosition.Y)); if (Solver.FindPathCost(Grid,BP,EP,30000)>=0) { bR=true; break; } }
        if (!bR) OutResult.Issues.Add({TEXT("Traversal"),FString::Printf(TEXT("Expansion (%.0f,%.0f) UNREACHABLE"),Exp.GridPosition.X,Exp.GridPosition.Y),ERTSValidationSeverity::Warning});
    }
}

void FRTSValidationPipeline::Pass2_Spawn(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult) const
{
    for (const FRTSBaseInfo& Base : Metadata.Bases)
    {
        int32 X=FMath::FloorToInt(Base.GridPosition.X), Y=FMath::FloorToInt(Base.GridPosition.Y), Blocked=0; bool bOK=true;
        for (int32 dy=-5;dy<=5&&bOK;++dy) for (int32 dx=-5;dx<=5;++dx)
        { if (!Grid.IsValidCoord(X+dx,Y+dy)){bOK=false;break;} if (!Grid.GetCell(X+dx,Y+dy).bBuildable&&++Blocked>8){bOK=false;break;} }
        if (!bOK) OutResult.Issues.Add({TEXT("Spawn"),FString::Printf(TEXT("Base %d lacks buildable area"),Base.PlayerIndex),ERTSValidationSeverity::Critical});
    }
}

void FRTSValidationPipeline::Pass3_Economy(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) const
{
    TMap<int32,int32> EC; for (const auto& E:Metadata.Expansions) EC.FindOrAdd(E.OwnerPlayerIndex)++;
    float MaxE=0,MinE=MAX_FLT; for (const auto& P:EC){MaxE=FMath::Max(MaxE,(float)P.Value);MinE=FMath::Min(MinE,(float)P.Value);}
    if (MaxE>KINDA_SMALL_NUMBER&&MinE<MAX_FLT) { float D=(MaxE-MinE)/MaxE; if (D>Settings->MaxFairnessError) OutResult.Issues.Add({TEXT("Economy"),FString::Printf(TEXT("Expansion imbalance %.0f%%"),D*100),ERTSValidationSeverity::Warning}); }
}

void FRTSValidationPipeline::Pass4_Choke(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult) const
{
    int32 NP=Metadata.Bases.Num(), NC=Metadata.Chokes.Num();
    if (NC==0&&NP>=2) OutResult.Issues.Add({TEXT("Choke"),FString::Printf(TEXT("No chokes for %d players"),NP),ERTSValidationSeverity::Warning});
    else if (NC<FMath::Max(1,NP-1)) OutResult.Issues.Add({TEXT("Choke"),FString::Printf(TEXT("Only %d chokes for %d players"),NC,NP),ERTSValidationSeverity::Warning});
}

void FRTSValidationPipeline::Pass5_Navmesh(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) const
{
    for (const FRTSChokeInfo& Ch : Metadata.Chokes)
        if (Ch.WidthCells < Settings->MinChokeWidth)
            OutResult.Issues.Add({TEXT("Navmesh"),FString::Printf(TEXT("Choke width %d < min %.0f — units may be stuck"),Ch.WidthCells,(float)Settings->MinChokeWidth),ERTSValidationSeverity::Critical});
}

void FRTSValidationPipeline::Pass6_Fairness(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings) const
{
    if (OutResult.OverallScore < Settings->MinAcceptableScore)
        OutResult.Issues.Add({TEXT("Fairness"),FString::Printf(TEXT("Score %.1f below threshold %.1f"),OutResult.OverallScore,Settings->MinAcceptableScore),ERTSValidationSeverity::Warning});
}
