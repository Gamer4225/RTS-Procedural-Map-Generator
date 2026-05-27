#include "Strategic/FRTSRiverGenerator.h"
#include "Math/UnrealMathUtility.h"
#include "Core/URTSGenerationSettings.h"
#include "Core/FRTSGrid.h"
#include "Core/FRTSSeedManager.h"

void FRTSRiverGenerator::Generate(FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings || !SeedManager || Grid.Cells.Num() == 0)
    {
        return;
    }

    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    const float MountainLevel = Settings->MountainLevel;
    const float WaterLevel = Settings->WaterLevel;

    // Step 1: Find mountain peaks
    TArray<FIntPoint> Peaks = FindPeaks(Grid, MountainLevel);
    if (Peaks.Num() == 0)
    {
        return;
    }

    // Step 2: Deterministically select spaced source points
    int32 NumRivers = FMath::Clamp(Settings->NumPlayers * 2, 2, 6);
    float MinSourceDist = FMath::Min(W, H) * 0.15f;
    TArray<FIntPoint> Sources = SelectSources(Peaks, NumRivers, MinSourceDist * MinSourceDist, SeedManager);

    // Step 3: Trace each river downhill with widening and lateral jitter
    int32 NeighborBuffer[8];
    TArray<FRiverPath> RiverPaths;
    
    for (const FIntPoint& Source : Sources)
    {
        FRiverPath Path = TraceRiverWithWidening(Grid, Source, WaterLevel, NeighborBuffer, SeedManager);
        if (Path.Cells.Num() > 5) // Only keep meaningful rivers
        {
            RiverPaths.Add(Path);
        }
    }

    // Step 4: Carve terrain along rivers
    CarveRiverBeds(Grid, WaterLevel);

    // Step 5: Smooth riverbanks
    SmoothRiverbanks(Grid, WaterLevel);

    // Step 6: Re-classify traversal (rivers = water = blocked)
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        FRTSCell& Cell = Grid.Cells[i];
        if (Cell.bWater)
        {
            Cell.bWalkable = false;
            Cell.bBuildable = false;
            Cell.MovementCostMultiplier = 0.0f;
        }
    }

    // Step 7: Apply contextual strategic value based on river topology
    ApplyContextualStrategicValue(Grid, RiverPaths, Settings);
}

TArray<FIntPoint> FRTSRiverGenerator::FindPeaks(const FRTSGrid& Grid, float MountainLevel) const
{
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    TArray<FIntPoint> Peaks;

    for (int32 Y = 1; Y < H - 1; ++Y)
    {
        for (int32 X = 1; X < W - 1; ++X)
        {
            float HCenter = Grid.GetCell(X, Y).Height;
            if (HCenter < MountainLevel)
            {
                continue;
            }

            bool bIsPeak = true;
            for (int32 dy = -1; dy <= 1 && bIsPeak; ++dy)
            {
                for (int32 dx = -1; dx <= 1; ++dx)
                {
                    if (dx == 0 && dy == 0) continue;
                    if (Grid.GetCell(X + dx, Y + dy).Height > HCenter)
                    {
                        bIsPeak = false;
                        break;
                    }
                }
            }

            if (bIsPeak)
            {
                Peaks.Add(FIntPoint(X, Y));
            }
        }
    }

    return Peaks;
}

TArray<FIntPoint> FRTSRiverGenerator::SelectSources(
    const TArray<FIntPoint>& Peaks,
    int32 NumRivers,
    float MinDistSq,
    UFRTSSeedManager* SeedManager) const
{
    TArray<FIntPoint> Sources;
    
    // Deterministic seeded selection: use position hash to create deterministic order
    TArray<int32> Indices;
    Indices.Reserve(Peaks.Num());
    for (int32 i = 0; i < Peaks.Num(); ++i) Indices.Add(i);
    
    // Sort by seeded hash of position for deterministic but varied ordering
    Indices.Sort([](int32 A, int32 B) {
        uint32 HashA = static_cast<uint32>(Peaks[A].X * 73856093u ^ Peaks[A].Y * 19349663u);
        uint32 HashB = static_cast<uint32>(Peaks[B].X * 73856093u ^ Peaks[B].Y * 19349663u);
        return HashA < HashB;
    });

    for (int32 IdxIdx = 0; IdxIdx < Indices.Num() && Sources.Num() < NumRivers; ++IdxIdx)
    {
        const FIntPoint& Peak = Peaks[Indices[IdxIdx]];
        bool bTooClose = false;
        for (const FIntPoint& Existing : Sources)
        {
            if (FIntPoint::DistSquared(Peak, Existing) < MinDistSq)
            {
                bTooClose = true;
                break;
            }
        }
        if (!bTooClose)
        {
            Sources.Add(Peak);
        }
    }

    return Sources;
}

FRTSRiverGenerator::FRiverPath FRTSRiverGenerator::TraceRiverWithWidening(
    FRTSGrid& Grid,
    FIntPoint Source,
    float WaterLevel,
    int32 NeighborBuffer[8],
    UFRTSSeedManager* SeedManager)
{
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    const int32 MaxSteps = W + H;
    const int32 SeedForBias = SeedManager->RandRange(0, 100000);

    FRiverPath Path;
    Path.Cells.Add(Source);

    int32 CurrentX = Source.X;
    int32 CurrentY = Source.Y;
    int32 PrevX = CurrentX;
    int32 PrevY = CurrentY - 1; // Assume came from above initially

    // Mark source and immediate neighborhood as water (2-3 cell radius)
    for (int32 dy = -1; dy <= 1; ++dy)
    {
        for (int32 dx = -1; dx <= 1; ++dx)
        {
            int32 NX = CurrentX + dx;
            int32 NY = CurrentY + dy;
            if (Grid.IsValidCoord(NX, NY))
            {
                FRTSCell& Cell = Grid.GetCell(NX, NY);
                Cell.bWater = true;
                Cell.Height = FMath::Min(Cell.Height, WaterLevel - 0.02f);
            }
        }
    }

    for (int32 Step = 0; Step < MaxSteps; ++Step)
    {
        FRTSCell& Cell = Grid.GetCell(CurrentX, CurrentY);

        // Stop if we hit existing water (not our own)
        if (Cell.bWater && !(CurrentX == Source.X && CurrentY == Source.Y))
        {
            // Merge with existing river
            break;
        }

        // Mark center and widen to 2-3 cells
        // Core path: 3×3 block around current position
        for (int32 dy = -1; dy <= 1; ++dy)
        {
            for (int32 dx = -1; dx <= 1; ++dx)
            {
                int32 WX = CurrentX + dx;
                int32 WY = CurrentY + dy;
                if (Grid.IsValidCoord(WX, WY))
                {
                    FRTSCell& WCell = Grid.GetCell(WX, WY);
                    WCell.bWater = true;
                    WCell.Height = FMath::Min(WCell.Height, WaterLevel - 0.02f);
                }
            }
        }

        // Find lowest neighbor with DETERMINISTIC lateral bias
        int32 NumNeighbors = Grid.GetNeighborsFixed(Grid.ToIndex(CurrentX, CurrentY), true, NeighborBuffer);

        int32 BestIdx = INDEX_NONE;
        float BestScore = MAX_FLT;

        for (int32 n = 0; n < NumNeighbors; ++n)
        {
            int32 NIdx = NeighborBuffer[n];
            FIntPoint NCoord = Grid.ToCoord(NIdx);
            const FRTSCell& NCell = Grid.GetCell(NIdx);

            // Base score: lower height = preferred
            float Score = NCell.Height;
            
            // Prefer existing water (merge rivers)
            if (NCell.bWater)
            {
                Score -= 0.15f;
            }

            // DETERMINISTIC lateral jitter: nudge away from straight lines
            Score = ApplyLateralBias(FIntPoint(CurrentX, CurrentY), NCoord, FIntPoint(PrevX, PrevY), Score, Step, SeedForBias);

            if (Score < BestScore)
            {
                BestScore = Score;
                BestIdx = NIdx;
            }
        }

        if (BestIdx == INDEX_NONE)
        {
            break;
        }

        // Store previous position for lateral bias calculation
        PrevX = CurrentX;
        PrevY = CurrentY;

        FIntPoint BestCoord = Grid.ToCoord(BestIdx);
        CurrentX = BestCoord.X;
        CurrentY = BestCoord.Y;
        Path.Cells.Add(FIntPoint(CurrentX, CurrentY));

        // Stop at map edge
        if (CurrentX <= 1 || CurrentX >= W - 2 || CurrentY <= 1 || CurrentY >= H - 2)
        {
            break;
        }
    }

    return Path;
}

float FRTSRiverGenerator::ApplyLateralBias(
    FIntPoint Current,
    FIntPoint Candidate,
    FIntPoint Previous,
    float BaseScore,
    int32 StepIndex,
    int32 Seed) const
{
    // Direction of last movement
    int32 DirX = Current.X - Previous.X;
    int32 DirY = Current.Y - Previous.Y;
    
    // Candidate direction relative to current
    int32 CandDirX = Candidate.X - Current.X;
    int32 CandDirY = Candidate.Y - Current.Y;

    // If continuing in exact same direction, add small penalty
    if (CandDirX == DirX && CandDirY == DirY)
    {
        BaseScore += 0.008f; // Tiny nudge to prefer slight turns
    }

    // Deterministic sinusoidal lateral bias based on position and seed
    // Creates gentle meanders that are reproducible for same seed
    float MeanderPhase = static_cast<float>(Seed % 1000) * 0.01f + static_cast<float>(StepIndex) * 0.15f;
    float MeanderStrength = 0.012f * FMath::Sin(MeanderPhase + Current.X * 0.1f + Current.Y * 0.07f);
    
    // Apply meander perpendicular to flow direction
    if (DirX != 0 || DirY != 0)
    {
        // Perpendicular vector
        int32 PerpX = -DirY;
        int32 PerpY = DirX;
        
        // Dot product with candidate direction tells us if candidate goes "left" or "right"
        float Dot = static_cast<float>(CandDirX * PerpX + CandDirY * PerpY);
        BaseScore += MeanderStrength * Dot;
    }

    return BaseScore;
}

void FRTSRiverGenerator::CarveRiverBeds(FRTSGrid& Grid, float WaterLevel)
{
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    for (int32 Y = 0; Y < H; ++Y)
    {
        for (int32 X = 0; X < W; ++X)
        {
            FRTSCell& Cell = Grid.GetCell(X, Y);
            if (Cell.bWater)
            {
                // Carve bed: ensure it's below water level with some depth variation
                float Depth = 0.03f + 0.04f * FMath::Sin(X * 0.2f + Y * 0.15f);
                Cell.Height = FMath::Min(Cell.Height, WaterLevel - 0.05f - Depth);
            }
        }
    }
}

void FRTSRiverGenerator::SmoothRiverbanks(FRTSGrid& Grid, float WaterLevel)
{
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    // First pass: identify bank cells (non-water adjacent to water)
    TArray<bool> IsBank;
    IsBank.SetNumZeroed(Grid.Cells.Num());

    const int32 Dirs[4][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}};
    for (int32 Y = 1; Y < H - 1; ++Y)
    {
        for (int32 X = 1; X < W - 1; ++X)
        {
            if (Grid.GetCell(X, Y).bWater)
            {
                continue;
            }

            for (int32 d = 0; d < 4; ++d)
            {
                if (Grid.GetCell(X + Dirs[d][0], Y + Dirs[d][1]).bWater)
                {
                    IsBank[Grid.ToIndex(X, Y)] = true;
                    break;
                }
            }
        }
    }

    // Second pass: smooth bank heights and create gentle slope from bank to water
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        if (!IsBank[i])
        {
            continue;
        }

        FRTSCell& Cell = Grid.Cells[i];
        FIntPoint Coord = Grid.ToCoord(i);
        
        // Count adjacent water cells to determine "steepness" of bank
        int32 WaterNeighbors = 0;
        for (int32 d = 0; d < 4; ++d)
        {
            int32 NX = Coord.X + Dirs[d][0];
            int32 NY = Coord.Y + Dirs[d][1];
            if (Grid.IsValidCoord(NX, NY) && Grid.GetCell(NX, NY).bWater)
            {
                ++WaterNeighbors;
            }
        }

        // Steeper bank where more water neighbors (inside bends)
        float BankHeight = WaterLevel + 0.02f + 0.03f * WaterNeighbors;
        Cell.Height = FMath::Lerp(Cell.Height, BankHeight, 0.7f);
    }
}

void FRTSRiverGenerator::ApplyContextualStrategicValue(
    FRTSGrid& Grid,
    const TArray<FRiverPath>& RiverPaths,
    const URTSGenerationSettings* Settings)
{
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    
    // For each water cell, compute strategic value based on:
    // 1. Proximity to expansion zones (crossing pressure)
    // 2. Number of adjacent land regions (choke creation potential)
    // 3. Width of river (narrower = more strategic)

    for (int32 Y = 0; Y < H; ++Y)
    {
        for (int32 X = 0; X < W; ++X)
        {
            FRTSCell& Cell = Grid.GetCell(X, Y);
            if (!Cell.bWater)
            {
                continue;
            }

            // Count adjacent walkable land regions
            TSet<int32> AdjacentRegions;
            const int32 Dirs[8][2] = {{1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {-1,1}, {1,-1}, {-1,-1}};
            
            for (int32 d = 0; d < 8; ++d)
            {
                int32 NX = X + Dirs[d][0];
                int32 NY = Y + Dirs[d][1];
                if (Grid.IsValidCoord(NX, NY))
                {
                    const FRTSCell& NCell = Grid.GetCell(NX, NY);
                    if (NCell.bWalkable && NCell.RegionID != INDEX_NONE)
                    {
                        AdjacentRegions.Add(NCell.RegionID);
                    }
                }
            }

            // Strategic value based on:
            // - Base value for being water (barrier)
            // - Bonus for separating multiple regions (choke point potential)
            // - Bonus for narrow crossings (few adjacent land cells)
            float BaseValue = 0.25f;
            float RegionBonus = FMath::Clamp((AdjacentRegions.Num() - 1) * 0.15f, 0.0f, 0.4f);
            
            // Narrow river = more strategic (harder to cross)
            int32 LandNeighbors = 0;
            for (int32 d = 0; d < 4; ++d)
            {
                int32 NX = X + Dirs[d][0];
                int32 NY = Y + Dirs[d][1];
                if (Grid.IsValidCoord(NX, NY) && !Grid.GetCell(NX, NY).bWater)
                {
                    ++LandNeighbors;
                }
            }
            float NarrowBonus = (LandNeighbors >= 3) ? 0.15f : 0.0f; // Narrow crossing

            Cell.StrategicValue = FMath::Clamp(BaseValue + RegionBonus + NarrowBonus, 0.0f, 1.0f);
        }
    }
}
