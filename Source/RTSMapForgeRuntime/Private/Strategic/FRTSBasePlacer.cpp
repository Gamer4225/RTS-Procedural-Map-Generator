#include "Strategic/FRTSBasePlacer.h"
#include "Math/UnrealMathUtility.h"

void FRTSBasePlacer::PlaceBases(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings || !SeedManager)
    {
        return;
    }

    Metadata.Bases.Empty();
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    const int32 NumPlayers = Settings->NumPlayers;
    const float SymmetryStrength = Settings->SymmetryStrength;

    // Gather candidates: large flat regions, away from edges
    TArray<FIntPoint> Candidates = FindCandidateCells(Grid, /*MinRegionSize=*/64);
    if (Candidates.Num() == 0)
    {
        return;
    }

    // For V1: if symmetry strength is high and NumPlayers == 2, place using 180 rotation
    bool bUseSymmetry = (SymmetryStrength >= 0.8f && NumPlayers == 2);

    if (bUseSymmetry && Candidates.Num() > 0)
    {
        FIntPoint P1 = PickCandidate(Candidates, SeedManager);
        // Pick one that isn't too close to edges
        int32 EdgeMargin = FMath::Max(W, H) / 6;
        int32 Attempts = 0;
        while (Attempts < 50)
        {
            if (P1.X > EdgeMargin && P1.X < W - EdgeMargin && P1.Y > EdgeMargin && P1.Y < H - EdgeMargin)
            {
                break;
            }
            P1 = PickCandidate(Candidates, SeedManager);
            ++Attempts;
        }

        FIntPoint P2 = ApplySymmetry(P1, W, H, NumPlayers, 1);

        // Verify both are walkable and buildable
        auto AddBase = [&](int32 PlayerIdx, const FIntPoint& Pos)
        {
            if (!Grid.IsValidCoord(Pos.X, Pos.Y))
            {
                return false;
            }
            FRTSCell& Cell = Grid.GetCell(Pos.X, Pos.Y);
            if (!Cell.bWalkable || !Cell.bBuildable)
            {
                return false;
            }
            if (!IsAreaBuildable(Grid, Pos.X, Pos.Y, 5))
            {
                return false;
            }

            FRTSBaseInfo Info;
            Info.PlayerIndex = PlayerIdx;
            Info.GridPosition = FVector2D(Pos.X, Pos.Y);
            Info.WorldPosition = Cell.WorldPosition;
            Info.RegionID = Cell.RegionID;
            Metadata.Bases.Add(Info);
            Cell.TacticalZone = ERTSTacticalZone::MainBase;
            return true;
        };

        if (AddBase(0, P1) && AddBase(1, P2))
        {
            return;
        }
        // If symmetry placement failed, fall through to random placement below
        Metadata.Bases.Empty();
    }

    // Fallback / general N-player placement: Poisson-disk-like spacing
    TArray<FIntPoint> Used;
    const float MinDistCells = FMath::Sqrt(static_cast<float>(W * H)) * Settings->MinRushDistance * 0.5f;
    const float MinDistSq = MinDistCells * MinDistCells;

    for (int32 p = 0; p < NumPlayers; ++p)
    {
        int32 Attempts = 0;
        FIntPoint Best;
        float BestScore = -1.0f;

        while (Attempts < 200)
        {
            FIntPoint Cand = PickCandidate(Candidates, SeedManager);

            // Check spacing against existing bases
            bool bTooClose = false;
            for (const FIntPoint& Existing : Used)
            {
                float Dsq = static_cast<float>(FIntPoint::DistSquared(Cand, Existing));
                if (Dsq < MinDistSq)
                {
                    bTooClose = true;
                    break;
                }
            }
            if (bTooClose)
            {
                ++Attempts;
                continue;
            }

            // Score by distance from edges (center-bias)
            float DX = FMath::Abs(Cand.X - W / 2) / static_cast<float>(W);
            float DY = FMath::Abs(Cand.Y - H / 2) / static_cast<float>(H);
            float Score = 1.0f - (DX + DY);

            if (IsAreaBuildable(Grid, Cand.X, Cand.Y, 5) && Score > BestScore)
            {
                BestScore = Score;
                Best = Cand;
            }
            ++Attempts;
        }

        if (BestScore >= 0.0f)
        {
            Used.Add(Best);
            FRTSCell& Cell = Grid.GetCell(Best.X, Best.Y);
            FRTSBaseInfo Info;
            Info.PlayerIndex = p;
            Info.GridPosition = FVector2D(Best.X, Best.Y);
            Info.WorldPosition = Cell.WorldPosition;
            Info.RegionID = Cell.RegionID;
            Metadata.Bases.Add(Info);
            Cell.TacticalZone = ERTSTacticalZone::MainBase;
        }
    }
}

TArray<FIntPoint> FRTSBasePlacer::FindCandidateCells(const FRTSGrid& Grid, int32 MinRegionSize) const
{
    TMap<int32, int32> RegionCounts;
    for (const FRTSCell& Cell : Grid.Cells)
    {
        if (Cell.RegionID != INDEX_NONE && Cell.bWalkable && Cell.bBuildable)
        {
            int32& Count = RegionCounts.FindOrAdd(Cell.RegionID);
            ++Count;
        }
    }

    TArray<FIntPoint> Candidates;
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    for (int32 Y = 0; Y < H; ++Y)
    {
        for (int32 X = 0; X < W; ++X)
        {
            const FRTSCell& Cell = Grid.GetCell(X, Y);
            if (Cell.bWalkable && Cell.bBuildable)
            {
                int32 Count = RegionCounts.FindRef(Cell.RegionID);
                if (Count >= MinRegionSize)
                {
                    Candidates.Add(FIntPoint(X, Y));
                }
            }
        }
    }
    return Candidates;
}

FIntPoint FRTSBasePlacer::PickCandidate(const TArray<FIntPoint>& Candidates, UFRTSSeedManager* SeedManager) const
{
    if (Candidates.Num() == 0)
    {
        return FIntPoint::ZeroValue;
    }
    int32 Idx = SeedManager->RandRange(0, Candidates.Num() - 1);
    return Candidates[Idx];
}

bool FRTSBasePlacer::IsAreaBuildable(const FRTSGrid& Grid, int32 X, int32 Y, int32 RadiusCells) const
{
    for (int32 dy = -RadiusCells; dy <= RadiusCells; ++dy)
    {
        for (int32 dx = -RadiusCells; dx <= RadiusCells; ++dx)
        {
            if (!Grid.IsValidCoord(X + dx, Y + dy))
            {
                return false;
            }
            if (!Grid.GetCell(X + dx, Y + dy).bBuildable)
            {
                return false;
            }
        }
    }
    return true;
}

FIntPoint FRTSBasePlacer::ApplySymmetry(const FIntPoint& Point, int32 W, int32 H, int32 NumPlayers, int32 PlayerIndex) const
{
    // For 2 players: 180-degree rotational symmetry around center
    if (NumPlayers == 2 && PlayerIndex == 1)
    {
        return FIntPoint(W - 1 - Point.X, H - 1 - Point.Y);
    }
    return Point;
}
