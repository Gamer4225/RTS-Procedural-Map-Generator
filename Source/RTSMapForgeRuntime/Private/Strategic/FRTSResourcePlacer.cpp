#include "Strategic/FRTSResourcePlacer.h"
#include "Math/UnrealMathUtility.h"

void FRTSResourcePlacer::PlaceResources(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings || !SeedManager || Grid.Cells.Num() == 0)
    {
        return;
    }

    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    const float MinResourceDist = FMath::Max(W, H) * 0.06f; // Minimum spacing between resource nodes
    const float MinResourceDistSq = MinResourceDist * MinResourceDist;
    const int32 MaxResources = FMath::Clamp((W * H) / 200, 8, 64); // Scale with map size

    // Step 1: Gather candidates
    TArray<FCandidate> Candidates = GatherCandidates(Grid, Metadata, Settings);
    if (Candidates.Num() == 0)
    {
        return;
    }

    // Step 2: Deterministic shuffle using seed manager
    // Simple seeded sort: we sort by a seeded hash of position to get deterministic ordering
    Candidates.Sort([SeedManager](const FCandidate& A, const FCandidate& B) {
        // Deterministic pseudo-random score based on position
        uint32 HashA = static_cast<uint32>(A.Position.X * 73856093u ^ A.Position.Y * 19349663u);
        uint32 HashB = static_cast<uint32>(B.Position.X * 73856093u ^ B.Position.Y * 19349663u);
        return HashA < HashB;
    });

    // Step 3: Poisson-disk selection (greedy)
    TArray<FIntPoint> Placed;
    for (const FCandidate& Cand : Candidates)
    {
        if (Placed.Num() >= MaxResources)
        {
            break;
        }

        bool bTooClose = false;
        for (const FIntPoint& Existing : Placed)
        {
            if (FIntPoint::DistSquared(Cand.Position, Existing) < MinResourceDistSq)
            {
                bTooClose = true;
                break;
            }
        }
        if (bTooClose)
        {
            continue;
        }

        // Place resource
        Placed.Add(Cand.Position);
        FRTSCell& Cell = Grid.GetCell(Cand.Position.X, Cand.Position.Y);
        Cell.ResourceValue = FMath::Clamp(Cand.Score, 0.0f, 1.0f);
        
        // Mark resource cluster if score is high enough
        if (Cell.ResourceValue > 0.5f)
        {
            Cell.TacticalZone = ERTSTacticalZone::ResourceCluster;
        }
    }
}

TArray<FRTSResourcePlacer::FCandidate> FRTSResourcePlacer::GatherCandidates(
    FRTSGrid& Grid,
    const FRTSMapMetadata& Metadata,
    const URTSGenerationSettings* Settings) const
{
    TArray<FCandidate> Candidates;
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    for (int32 Y = 1; Y < H - 1; ++Y)
    {
        for (int32 X = 1; X < W - 1; ++X)
        {
            const FRTSCell& Cell = Grid.GetCell(X, Y);
            
            // Only place on walkable, buildable terrain that isn't water or cliff
            if (!Cell.bWalkable || !Cell.bBuildable || Cell.bWater || Cell.bCliff)
            {
                continue;
            }

            float Score = ScoreCell(Grid, X, Y, Metadata);
            if (Score > 0.2f) // Minimum threshold
            {
                Candidates.Add({FIntPoint(X, Y), Score});
            }
        }
    }

    return Candidates;
}

float FRTSResourcePlacer::ScoreCell(const FRTSGrid& Grid, int32 X, int32 Y, const FRTSMapMetadata& Metadata) const
{
    const FRTSCell& Cell = Grid.GetCell(X, Y);
    float Score = 0.0f;

    // Proximity to expansions is valuable (contested resources = good gameplay)
    float DistToNearestExp = MAX_FLT;
    for (const FRTSExpansionInfo& Exp : Metadata.Expansions)
    {
        float D = FVector2D::Distance(FVector2D(X, Y), Exp.GridPosition);
        if (D < DistToNearestExp)
        {
            DistToNearestExp = D;
        }
    }
    
    const float MapDiag = FMath::Sqrt(static_cast<float>(Grid.Width * Grid.Width + Grid.Height * Grid.Height));
    if (DistToNearestExp < MAX_FLT)
    {
        // Closer to expansions = higher score, but not TOO close
        float ProximityScore = 1.0f - FMath::Clamp(DistToNearestExp / (MapDiag * 0.3f), 0.0f, 1.0f);
        Score += ProximityScore * 0.4f;
    }

    // High ground bonus
    if (Cell.Height > 0.6f)
    {
        Score += 0.2f;
    }

    // Slight randomness for organic feel (deterministic: use position hash)
    uint32 PosHash = static_cast<uint32>(X * 73856093u ^ Y * 19349663u);
    float DeterministicJitter = static_cast<float>(PosHash % 1000) / 1000.0f;
    Score += DeterministicJitter * 0.1f;

    return Score;
}
