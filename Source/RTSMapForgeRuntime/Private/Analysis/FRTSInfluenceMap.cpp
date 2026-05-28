#include "Analysis/FRTSInfluenceMap.h"
#include "Math/UnrealMathUtility.h"

void FRTSInfluenceMap::Generate(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    const int32 W = Grid.Width, H = Grid.Height;
    if (Metadata.Bases.Num() < 2) return;
    FVector2D PosA = Metadata.Bases[0].GridPosition, PosB = Metadata.Bases[1].GridPosition;
    for (int32 Y = 0; Y < H; ++Y)
    for (int32 X = 0; X < W; ++X)
    {
        int32 Idx = Grid.ToIndex(X, Y);
        FVector2D Pos(static_cast<float>(X), static_cast<float>(Y));
        float DA = FVector2D::Distance(Pos, PosA) + 1.0f;
        float DB = FVector2D::Distance(Pos, PosB) + 1.0f;
        float IA = 1.0f / (DA * DA), IB = 1.0f / (DB * DB);
        float Total = IA + IB;
        if (Total > KINDA_SMALL_NUMBER)
            Grid.Cells[Idx].ControlValue = (IB - IA) / Total;
    }
}
