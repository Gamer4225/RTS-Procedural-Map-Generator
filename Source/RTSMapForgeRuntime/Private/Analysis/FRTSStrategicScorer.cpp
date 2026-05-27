#include "Analysis/FRTSStrategicScorer.h"
#include "Math/UnrealMathUtility.h"

void FRTSStrategicScorer::Score(FRTSGrid& Grid, FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings)
{
    // V1 scoring: balance, rush distance viability, path diversity proxy via choke count
    float BalanceScore = 100.0f;
    float RushScore = 100.0f;
    float ChokeScore = 100.0f;
    float PathDiversityScore = 50.0f;

    const int32 NumBases = Metadata.Bases.Num();
    if (NumBases >= 2)
    {
        // Balance based on expansion counts per player (proxy)
        TMap<int32, int32> ExpansionCounts;
        for (const FRTSExpansionInfo& Exp : Metadata.Expansions)
        {
            ExpansionCounts.FindOrAdd(Exp.OwnerPlayerIndex)++;
        }

        float MaxExp = 1.0f;
        float MinExp = MAX_FLT;
        for (const auto& Pair : ExpansionCounts)
        {
            MaxExp = FMath::Max(MaxExp, static_cast<float>(Pair.Value));
            MinExp = FMath::Min(MinExp, static_cast<float>(Pair.Value));
        }
        if (MinExp < KINDA_SMALL_NUMBER)
        {
            MinExp = 0.0f;
        }
        if (MaxExp > KINDA_SMALL_NUMBER)
        {
            float Delta = (MaxExp - MinExp) / MaxExp;
            BalanceScore = FMath::Clamp(100.0f - Delta * 100.0f, 0.0f, 100.0f);
        }

        // Rush distance: compute approximate diagonal fraction between first two bases
        FVector2D B0 = Metadata.Bases[0].GridPosition;
        FVector2D B1 = Metadata.Bases[1].GridPosition;
        float Dist = FVector2D::Distance(B0, B1);
        float MapDiag = FMath::Sqrt(static_cast<float>(Grid.Width * Grid.Width + Grid.Height * Grid.Height));
        float Fraction = Dist / MapDiag;
        float Target = Settings->MinRushDistance;
        // Score peaks at around 0.5 map diagonal
        float Ideal = 0.5f;
        RushScore = FMath::Clamp(100.0f - (FMath::Abs(Fraction - Ideal) / Ideal) * 100.0f, 0.0f, 100.0f);

        // Choke quality: at least 1-3 chokes expected for interesting strategy
        int32 ChokeCount = Metadata.Chokes.Num();
        if (ChokeCount == 0)
        {
            ChokeScore = 30.0f;
        }
        else if (ChokeCount <= 5)
        {
            ChokeScore = 100.0f;
        }
        else
        {
            ChokeScore = FMath::Clamp(100.0f - (ChokeCount - 5) * 5.0f, 0.0f, 100.0f);
        }
    }

    // Path diversity: stub for V1; will use actual alternate path counts in V2
    PathDiversityScore = ChokeScore * 0.7f; // proxy

    float Overall = BalanceScore * 0.35f + RushScore * 0.25f + ChokeScore * 0.25f + PathDiversityScore * 0.15f;
    OutResult.OverallScore = Overall;
}
