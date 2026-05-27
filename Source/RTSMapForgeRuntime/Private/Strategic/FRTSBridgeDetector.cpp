#include "Strategic/FRTSBridgeDetector.h"
#include "Math/UnrealMathUtility.h"
#include "Pathfinding/FRTSAStarSolver.h"

void FRTSBridgeDetector::DetectCrossings(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    if (Settings->NumPlayers < 2)
    {
        return;
    }

    // Step 1: Find all candidate water cells that separate regions
    TArray<FCrossingCandidate> Candidates = FindCandidates(Grid);
    if (Candidates.Num() == 0)
    {
        return;
    }

    // Step 2: Score candidates
    for (FCrossingCandidate& Cand : Candidates)
    {
        ScoreTraffic(Cand, Grid, Metadata);
        ScoreProximity(Cand, Metadata, Grid);
        
        // Overall: narrow + high traffic + near objectives
        Cand.OverallScore = 
            (1.0f - FMath::Clamp(Cand.WidthCells / 6.0f, 0.0f, 1.0f)) * 0.4f +  // Narrower = better
            Cand.TrafficScore * 0.35f +
            Cand.ProximityScore * 0.25f;
    }

    // Step 3: Sort by score, commit top N spaced apart
    Candidates.Sort([](const FCrossingCandidate& A, const FCrossingCandidate& B) {
        return A.OverallScore > B.OverallScore;
    });

    int32 MaxCrossings = FMath::Clamp(Settings->NumPlayers * 2, 2, 8);
    CommitCrossings(Grid, Metadata, Candidates, MaxCrossings);
}

TArray<FRTSBridgeDetector::FCrossingCandidate> FRTSBridgeDetector::FindCandidates(const FRTSGrid& Grid) const
{
    TArray<FCrossingCandidate> Candidates;
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    // Scan all water cells
    for (int32 Y = 1; Y < H - 1; ++Y)
    {
        for (int32 X = 1; X < W - 1; ++X)
        {
            if (!Grid.GetCell(X, Y).bWater)
            {
                continue;
            }

            // Check 4-directional for walkable land on opposing sides
            // A crossing candidate must have land on at least 2 opposite sides
            const FRTSCell& Left   = Grid.GetCell(X - 1, Y);
            const FRTSCell& Right  = Grid.GetCell(X + 1, Y);
            const FRTSCell& Up     = Grid.GetCell(X, Y - 1);
            const FRTSCell& Down   = Grid.GetCell(X, Y + 1);

            bool bHorizontalLand = (Left.bWalkable && Right.bWalkable);
            bool bVerticalLand   = (Up.bWalkable && Down.bWalkable);

            if (!bHorizontalLand && !bVerticalLand)
            {
                continue;
            }

            // Evaluate this as a crossing
            FCrossingCandidate Cand = EvaluateCrossing(Grid, X, Y);
            if (Cand.RegionA != INDEX_NONE && Cand.RegionB != INDEX_NONE && Cand.WidthCells > 0)
            {
                Candidates.Add(Cand);
            }
        }
    }

    return Candidates;
}

FRTSBridgeDetector::FCrossingCandidate FRTSBridgeDetector::EvaluateCrossing(const FRTSGrid& Grid, int32 X, int32 Y) const
{
    FCrossingCandidate Cand;
    Cand.Position = FIntPoint(X, Y);

    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    // Try horizontal crossing first (scan vertical width)
    const FRTSCell& Left  = Grid.GetCell(X - 1, Y);
    const FRTSCell& Right = Grid.GetCell(X + 1, Y);

    if (Left.bWalkable && Right.bWalkable && Left.RegionID != Right.RegionID)
    {
        Cand.RegionA = Left.RegionID;
        Cand.RegionB = Right.RegionID;
        Cand.WidthCells = MeasureWaterWidth(Grid, X, Y, 0, 1, 8) + MeasureWaterWidth(Grid, X, Y, 0, -1, 8) - 1;
        return Cand;
    }

    // Try vertical crossing (scan horizontal width)
    const FRTSCell& Up   = Grid.GetCell(X, Y - 1);
    const FRTSCell& Down = Grid.GetCell(X, Y + 1);

    if (Up.bWalkable && Down.bWalkable && Up.RegionID != Down.RegionID)
    {
        Cand.RegionA = Up.RegionID;
        Cand.RegionB = Down.RegionID;
        Cand.WidthCells = MeasureWaterWidth(Grid, X, Y, 1, 0, 8) + MeasureWaterWidth(Grid, X, Y, -1, 0, 8) - 1;
        return Cand;
    }

    // No valid crossing
    Cand.RegionA = INDEX_NONE;
    Cand.RegionB = INDEX_NONE;
    return Cand;
}

int32 FRTSBridgeDetector::MeasureWaterWidth(const FRTSGrid& Grid, int32 StartX, int32 StartY, int32 DirX, int32 DirY, int32 MaxWidth) const
{
    int32 Count = 0;
    int32 X = StartX;
    int32 Y = StartY;

    while (Count < MaxWidth)
    {
        X += DirX;
        Y += DirY;
        if (!Grid.IsValidCoord(X, Y))
        {
            break;
        }
        if (!Grid.GetCell(X, Y).bWater)
        {
            break;
        }
        ++Count;
    }

    return Count + 1; // Include start cell
}

void FRTSBridgeDetector::ScoreTraffic(FCrossingCandidate& Candidate, const FRTSGrid& Grid, const FRTSMapMetadata& Metadata) const
{
    // Traffic score: estimate how many base-to-base / base-to-expansion paths
    // would logically cross between these two regions
    // Simplified: count bases/expansions in each region
    int32 CountA = 0;
    int32 CountB = 0;

    for (const FRTSBaseInfo& Base : Metadata.Bases)
    {
        FIntPoint Pos(FMath::FloorToInt(Base.GridPosition.X), FMath::FloorToInt(Base.GridPosition.Y));
        if (Grid.IsValidCoord(Pos.X, Pos.Y))
        {
            int32 R = Grid.GetCell(Pos.X, Pos.Y).RegionID;
            if (R == Candidate.RegionA) ++CountA;
            else if (R == Candidate.RegionB) ++CountB;
        }
    }

    for (const FRTSExpansionInfo& Exp : Metadata.Expansions)
    {
        FIntPoint Pos(FMath::FloorToInt(Exp.GridPosition.X), FMath::FloorToInt(Exp.GridPosition.Y));
        if (Grid.IsValidCoord(Pos.X, Pos.Y))
        {
            int32 R = Grid.GetCell(Pos.X, Pos.Y).RegionID;
            if (R == Candidate.RegionA) ++CountA;
            else if (R == Candidate.RegionB) ++CountB;
        }
    }

    // More objectives on both sides = more traffic
    float Total = static_cast<float>(CountA + CountB);
    float Balance = (CountA > 0 && CountB > 0) ? 
        (1.0f - FMath::Abs(CountA - CountB) / Total) : 0.0f;
    
    Candidate.TrafficScore = FMath::Clamp(Total / 4.0f, 0.0f, 1.0f) * (0.3f + 0.7f * Balance);
}

void FRTSBridgeDetector::ScoreProximity(FCrossingCandidate& Candidate, const FRTSMapMetadata& Metadata, const FRTSGrid& Grid) const
{
    // Proximity to bases and expansions (crossings near objectives = more important)
    float MinBaseDist = MAX_FLT;
    float MinExpDist = MAX_FLT;

    for (const FRTSBaseInfo& Base : Metadata.Bases)
    {
        float D = FVector2D::Distance(FVector2D(Candidate.Position.X, Candidate.Position.Y), Base.GridPosition);
        MinBaseDist = FMath::Min(MinBaseDist, D);
    }

    for (const FRTSExpansionInfo& Exp : Metadata.Expansions)
    {
        float D = FVector2D::Distance(FVector2D(Candidate.Position.X, Candidate.Position.Y), Exp.GridPosition);
        MinExpDist = FMath::Min(MinExpDist, D);
    }

    float MapDiag = FMath::Sqrt(static_cast<float>(Grid.Width * Grid.Width + Grid.Height * Grid.Height));
    float BaseScore = 1.0f - FMath::Clamp(MinBaseDist / (MapDiag * 0.3f), 0.0f, 1.0f);
    float ExpScore  = 1.0f - FMath::Clamp(MinExpDist / (MapDiag * 0.25f), 0.0f, 1.0f);

    Candidate.ProximityScore = BaseScore * 0.6f + ExpScore * 0.4f;
}

void FRTSBridgeDetector::CommitCrossings(FRTSGrid& Grid, FRTSMapMetadata& Metadata, TArray<FCrossingCandidate>& Candidates, int32 MaxCrossings) const
{
    const float MinDistSq = FMath::Square(FMath::Min(Grid.Width, Grid.Height) * 0.08f);
    TArray<FIntPoint> Committed;

    for (FCrossingCandidate& Cand : Candidates)
    {
        if (Committed.Num() >= MaxCrossings)
        {
            break;
        }

        // Spacing check: don't commit crossings too close to each other
        bool bTooClose = false;
        for (const FIntPoint& Existing : Committed)
        {
            if (FIntPoint::DistSquared(Cand.Position, Existing) < MinDistSq)
            {
                bTooClose = true;
                break;
            }
        }
        if (bTooClose)
        {
            continue;
        }

        // Commit: mark water cells in the crossing width as special
        // We mark the center and a small radius as RiverCrossing zone
        // Note: RiverCrossing is a water cell zone - the tactical zone sits on water
        // This is an exception to the "zones are on walkable land" rule
        for (int32 dy = -1; dy <= 1; ++dy)
        {
            for (int32 dx = -1; dx <= 1; ++dx)
            {
                int32 NX = Cand.Position.X + dx;
                int32 NY = Cand.Position.Y + dy;
                if (Grid.IsValidCoord(NX, NY) && Grid.GetCell(NX, NY).bWater)
                {
                    Grid.GetCell(NX, NY).TacticalZone = ERTSTacticalZone::RiverCrossing;
                    // Crossings are strategically very valuable
                    Grid.GetCell(NX, NY).StrategicValue = FMath::Max(Grid.GetCell(NX, NY).StrategicValue, 0.6f);
                }
            }
        }

        // Add to metadata
        FRTSChokeInfo Crossing;
        Crossing.WidthCells = Cand.WidthCells;
        Crossing.Cells.Add(Cand.Position);
        Crossing.RegionA = Cand.RegionA;
        Crossing.RegionB = Cand.RegionB;
        Crossing.Hardness = 1.0f - FMath::Clamp(Cand.WidthCells / 5.0f, 0.0f, 1.0f); // Narrow = harder to cross
        Metadata.Chokes.Add(Crossing); // Store crossings alongside chokes for unified strategic view

        Committed.Add(Cand.Position);
    }
}
