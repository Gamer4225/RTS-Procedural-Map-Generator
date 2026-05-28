#include "Analysis/FRTSStrategicScorer.h"
#include "Math/UnrealMathUtility.h"

void FRTSStrategicScorer::Score(FRTSGrid& Grid, FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings)
{
    float Balance=100,Rush=100,Choke=100,Path=50;
    if (Metadata.Bases.Num() >= 2)
    {
        TMap<int32,int32> ExpCounts;
        for (const auto& E : Metadata.Expansions) ExpCounts.FindOrAdd(E.OwnerPlayerIndex)++;
        float MaxE=1,MinE=MAX_FLT;
        for (const auto& P : ExpCounts) { MaxE=FMath::Max(MaxE,(float)P.Value); MinE=FMath::Min(MinE,(float)P.Value); }
        if (MaxE > KINDA_SMALL_NUMBER) Balance = FMath::Clamp(100.0f - (MaxE-FMath::Max(MinE,0.0f))/MaxE*100.0f, 0.0f, 100.0f);
        float Dist = FVector2D::Distance(Metadata.Bases[0].GridPosition, Metadata.Bases[1].GridPosition);
        float Diag = FMath::Sqrt((float)(Grid.Width*Grid.Width+Grid.Height*Grid.Height));
        Rush = FMath::Clamp(100.0f - (FMath::Abs(Dist/Diag - 0.5f)/0.5f)*100.0f, 0.0f, 100.0f);
        int32 NC = Metadata.Chokes.Num();
        Choke = (NC==0)?30:(NC<=5)?100:FMath::Clamp(100.0f-(NC-5)*5.0f,0.0f,100.0f);
    }
    Path = Choke * 0.7f;
    OutResult.OverallScore = Balance*0.35f + Rush*0.25f + Choke*0.25f + Path*0.15f;
}
