#include "Strategic/FRTSExpansionPlacer.h"
#include "Math/UnrealMathUtility.h"

void FRTSExpansionPlacer::PlaceExpansions(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings || Metadata.Bases.Num() == 0)
    {
        return;
    }

    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    const float MaxDist = FMath::Sqrt(static_cast<float>(W * W + H * H));
    const int32 NumExpansions = Settings->NumExpansions;

    for (const FRTSBaseInfo& Base : Metadata.Bases)
    {
        // Find candidates: buildable, not already a base, in same or adjacent region
        TArray<FIntPoint> Candidates;
        const int32 BaseX = FMath::FloorToInt(Base.GridPosition.X);
        const int32 BaseY = FMath::FloorToInt(Base.GridPosition.Y);

        for (int32 Y = 0; Y < H; ++Y)
        {
            for (int32 X = 0; X < W; ++X)
            {
                FRTSCell& Cell = Grid.GetCell(X, Y);
                if (!Cell.bBuildable || Cell.TacticalZone == ERTSTacticalZone::MainBase)
                {
                    continue;
                }

                float DistToBase = FVector2D::Distance(FVector2D(X, Y), Base.GridPosition);
                // Natural expansion: moderately close; Risky: farther / mid-map
                if (DistToBase > MaxDist * 0.15f && DistToBase < MaxDist * 0.5f)
                {
                    Candidates.Add(FIntPoint(X, Y));
                }
            }
        }

        // Pick NumExpansions best candidates by risk/interest
        for (int32 e = 0; e < NumExpansions && Candidates.Num() > 0; ++e)
        {
            int32 BestIdx = 0;
            float BestScore = -1.0f;

            for (int32 i = 0; i < Candidates.Num(); ++i)
            {
                const FIntPoint& Pos = Candidates[i];
                float DistToBase = FVector2D::Distance(FVector2D(Pos.X, Pos.Y), Base.GridPosition);
                float DistToCenter = FVector2D::Distance(FVector2D(Pos.X, Pos.Y), FVector2D(W * 0.5f, H * 0.5f));
                // Prefer expansions somewhat toward center for 2p symmetry; closer = safer
                float Score = (DistToBase * 0.3f) + (DistToCenter * 0.7f);
                if (Score > BestScore)
                {
                    BestScore = Score;
                    BestIdx = i;
                }
            }

            FIntPoint Pick = Candidates[BestIdx];
            Candidates.RemoveAt(BestIdx);

            FRTSCell& Cell = Grid.GetCell(Pick.X, Pick.Y);
            Cell.TacticalZone = ERTSTacticalZone::NatExpansion;

            FRTSExpansionInfo Exp;
            Exp.OwnerPlayerIndex = Base.PlayerIndex;
            Exp.GridPosition = FVector2D(Pick.X, Pick.Y);
            Exp.WorldPosition = Cell.WorldPosition;
            Exp.RiskScore = 1.0f - (FVector2D::Distance(Exp.GridPosition, Base.GridPosition) / (MaxDist * 0.5f));
            Exp.bContested = (Exp.RiskScore > 0.6f);
            Metadata.Expansions.Add(Exp);
        }
    }
}
