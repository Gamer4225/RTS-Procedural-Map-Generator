#include "Analysis/FRTSInfluenceMap.h"
#include "Math/UnrealMathUtility.h"

void FRTSInfluenceMap::Generate(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    const float CellSize = Grid.CellSize;

    // Inverse-square influence from each base position
    // ControlValue = which player's influence dominates
    TArray<float> InfluenceA;
    TArray<float> InfluenceB;
    InfluenceA.SetNumZeroed(Grid.Cells.Num());
    InfluenceB.SetNumZeroed(Grid.Cells.Num());

    // V1: Simple 2-player influence. Extend for N players later.
    if (Metadata.Bases.Num() >= 2)
    {
        FVector2D PosA = Metadata.Bases[0].GridPosition;
        FVector2D PosB = Metadata.Bases[1].GridPosition;

        for (int32 Y = 0; Y < H; ++Y)
        {
            for (int32 X = 0; X < W; ++X)
            {
                int32 Idx = Grid.ToIndex(X, Y);
                FVector2D Pos(static_cast<float>(X), static_cast<float>(Y));
                float DistA = FVector2D::Distance(Pos, PosA) + 1.0f;
                float DistB = FVector2D::Distance(Pos, PosB) + 1.0f;

                InfluenceA[Idx] = 1.0f / (DistA * DistA);
                InfluenceB[Idx] = 1.0f / (DistB * DistB);

                float Total = InfluenceA[Idx] + InfluenceB[Idx];
                if (Total > KINDA_SMALL_NUMBER)
                {
                    // ControlValue: -1 = Player 0, +1 = Player 1
                    Grid.Cells[Idx].ControlValue = (InfluenceB[Idx] - InfluenceA[Idx]) / Total;
                }
            }
        }
    }
}
