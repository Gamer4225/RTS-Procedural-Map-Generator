#include "Validation/FRTSResourceAccessibilityValidator.h"
#include "Pathfinding/FRTSAStarSolver.h"
#include "Math/UnrealMathUtility.h"

void FRTSResourceAccessibilityValidator::Validate(
    const FRTSGrid& Grid,
    const FRTSMapMetadata& Metadata,
    FRTSValidationResult& OutResult,
    const URTSGenerationSettings* Settings
) const
{
    if (Metadata.Bases.Num() < 2)
    {
        return;
    }

    TArray<FResourceAccessibility> Resources = GatherResources(Grid, Metadata);
    if (Resources.Num() == 0)
    {
        return;
    }

    // Compute accessibility for each resource
    for (FResourceAccessibility& Res : Resources)
    {
        ComputeAccessibility(Res, Grid, Metadata);
        ComputeSafety(Res, Grid, Metadata);
    }

    // Aggregate per-player scores
    // Attribution: resource goes to nearest base owner
    TMap<int32, float> PlayerAccessibilityScore;
    TMap<int32, float> PlayerSafetyScore;
    TMap<int32, int32> PlayerResourceCount;

    for (const FResourceAccessibility& Res : Resources)
    {
        // Find nearest base to determine owner
        float BestDist = MAX_FLT;
        int32 BestPlayer = INDEX_NONE;
        
        for (const FRTSBaseInfo& Base : Metadata.Bases)
        {
            float D = FVector2D::Distance(Res.Position, Base.GridPosition);
            if (D < BestDist)
            {
                BestDist = D;
                BestPlayer = Base.PlayerIndex;
            }
        }

        if (BestPlayer == INDEX_NONE)
        {
            continue;
        }

        // Composite accessibility: lower path cost = higher accessibility
        // Higher safety (distance from enemy) = higher accessibility
        // Fewer chokes/crossings = higher accessibility
        float Accessibility = 
            (1.0f / (1.0f + Res.PathCostFromBase * 0.01f)) * 0.4f +
            Res.SafetyScore * 0.35f +
            (1.0f / (1.0f + Res.ChokesEnRoute + Res.RiverCrossings)) * 0.25f;

        PlayerAccessibilityScore.FindOrAdd(BestPlayer) += Accessibility * Res.ResourceValue;
        PlayerSafetyScore.FindOrAdd(BestPlayer) += Res.SafetyScore * Res.ResourceValue;
        PlayerResourceCount.FindOrAdd(BestPlayer)++;
    }

    // Check parity
    if (PlayerAccessibilityScore.Num() < 2)
    {
        return;
    }

    float MaxAccessibility = 0.0f;
    float MinAccessibility = MAX_FLT;
    float MaxSafety = 0.0f;
    float MinSafety = MAX_FLT;

    for (const auto& Pair : PlayerAccessibilityScore)
    {
        MaxAccessibility = FMath::Max(MaxAccessibility, Pair.Value);
        MinAccessibility = FMath::Min(MinAccessibility, Pair.Value);
    }
    for (const auto& Pair : PlayerSafetyScore)
    {
        MaxSafety = FMath::Max(MaxSafety, Pair.Value);
        MinSafety = FMath::Min(MinSafety, Pair.Value);
    }

    // Normalize by resource count to avoid penalizing players with fewer but richer nodes
    float AccessibilityDelta = (MaxAccessibility - MinAccessibility) / FMath::Max(MaxAccessibility, 1.0f);
    float SafetyDelta = (MaxSafety - MinSafety) / FMath::Max(MaxSafety, 1.0f);

    if (AccessibilityDelta > Settings->MaxFairnessError)
    {
        OutResult.Issues.Add(FRTSValidationIssue{
            TEXT("ResourceAccessibility"),
            FString::Printf(TEXT("Resource accessibility imbalance: %.0f%% (path cost / choke / crossing differences). One player's resources are harder to reach/protect."),
                AccessibilityDelta * 100.0f),
            ERTSValidationSeverity::Warning
        });
    }

    if (SafetyDelta > Settings->MaxFairnessError)
    {
        OutResult.Issues.Add(FRTSValidationIssue{
            TEXT("ResourceSafety"),
            FString::Printf(TEXT("Resource safety imbalance: %.0f%%. One player's resources are more exposed to enemy attack."),
                SafetyDelta * 100.0f),
            ERTSValidationSeverity::Warning
        });
    }
}

TArray<FRTSResourceAccessibilityValidator::FResourceAccessibility> 
FRTSResourceAccessibilityValidator::GatherResources(
    const FRTSGrid& Grid,
    const FRTSMapMetadata& Metadata
) const
{
    TArray<FResourceAccessibility> Result;
    
    // Gather all resource-cluster cells
    for (int32 Y = 0; Y < Grid.Height; ++Y)
    {
        for (int32 X = 0; X < Grid.Width; ++X)
        {
            const FRTSCell& Cell = Grid.GetCell(X, Y);
            if (Cell.ResourceValue > 0.1f)
            {
                FResourceAccessibility Res;
                Res.Position = FVector2D(static_cast<float>(X), static_cast<float>(Y));
                Res.ResourceValue = Cell.ResourceValue;
                Result.Add(Res);
            }
        }
    }

    return Result;
}

void FRTSResourceAccessibilityValidator::ComputeAccessibility(
    FResourceAccessibility& Resource,
    const FRTSGrid& Grid,
    const FRTSMapMetadata& Metadata
) const
{
    // Find nearest base and compute path
    float BestCost = MAX_FLT;
    int32 BestBaseIdx = INDEX_NONE;
    FIntPoint ResPos(FMath::FloorToInt(Resource.Position.X), FMath::FloorToInt(Resource.Position.Y));

    FRTSAStarSolver Solver;
    for (int32 i = 0; i < Metadata.Bases.Num(); ++i)
    {
        FIntPoint BasePos(
            FMath::FloorToInt(Metadata.Bases[i].GridPosition.X),
            FMath::FloorToInt(Metadata.Bases[i].GridPosition.Y)
        );
        float Cost = Solver.FindPathCost(Grid, ResPos, BasePos, 30000);
        if (Cost >= 0.0f && Cost < BestCost)
        {
            BestCost = Cost;
            BestBaseIdx = i;
        }
    }

    if (BestBaseIdx == INDEX_NONE)
    {
        Resource.PathCostFromBase = MAX_FLT;
        return;
    }

    Resource.PathCostFromBase = BestCost;

    // Count obstacles on path
    FIntPoint BasePos(
        FMath::FloorToInt(Metadata.Bases[BestBaseIdx].GridPosition.X),
        FMath::FloorToInt(Metadata.Bases[BestBaseIdx].GridPosition.Y)
    );
    TracePathForObstacles(Grid, BasePos, ResPos, Resource.ChokesEnRoute, Resource.RiverCrossings, BestCost);
}

void FRTSResourceAccessibilityValidator::ComputeSafety(
    FResourceAccessibility& Resource,
    const FRTSGrid& Grid,
    const FRTSMapMetadata& Metadata
) const
{
    // Safety = average distance from ENEMY bases (not nearest)
    // A resource near your base but also near enemy base = low safety
    // A resource near your base and far from all enemies = high safety
    
    int32 OwnerPlayer = INDEX_NONE;
    float BestDist = MAX_FLT;
    
    for (const FRTSBaseInfo& Base : Metadata.Bases)
    {
        float D = FVector2D::Distance(Resource.Position, Base.GridPosition);
        if (D < BestDist)
        {
            BestDist = D;
            OwnerPlayer = Base.PlayerIndex;
        }
    }

    if (OwnerPlayer == INDEX_NONE)
    {
        Resource.SafetyScore = 0.0f;
        return;
    }

    float EnemyDistSum = 0.0f;
    int32 EnemyCount = 0;
    
    for (const FRTSBaseInfo& Base : Metadata.Bases)
    {
        if (Base.PlayerIndex != OwnerPlayer)
        {
            EnemyDistSum += FVector2D::Distance(Resource.Position, Base.GridPosition);
            ++EnemyCount;
        }
    }

    if (EnemyCount == 0)
    {
        Resource.SafetyScore = 1.0f; // FFA or no enemies = fully safe
        return;
    }

    float AvgEnemyDist = EnemyDistSum / EnemyCount;
    float FriendlyDist = BestDist;
    
    // Safety ratio: higher when enemy is farther relative to friend
    float Ratio = AvgEnemyDist / FMath::Max(FriendlyDist, 1.0f);
    Resource.SafetyScore = FMath::Clamp(Ratio / 3.0f, 0.0f, 1.0f); // Normalize: 3x distance = max safety
}

bool FRTSResourceAccessibilityValidator::TracePathForObstacles(
    const FRTSGrid& Grid,
    FIntPoint Start,
    FIntPoint End,
    int32& OutChokes,
    int32& OutRiverCrossings,
    float& OutPathCost
) const
{
    OutChokes = 0;
    OutRiverCrossings = 0;
    OutPathCost = -1.0f;

    // Simplified: Bresenham line walk from Start toward End
    // Counts choke and river-crossing zones touched
    int32 X0 = Start.X, Y0 = Start.Y;
    int32 X1 = End.X, Y1 = End.Y;
    
    int32 Dx = FMath::Abs(X1 - X0);
    int32 Dy = FMath::Abs(Y1 - Y0);
    int32 Sx = (X0 < X1) ? 1 : -1;
    int32 Sy = (Y0 < Y1) ? 1 : -1;
    int32 Err = Dx - Dy;

    TSet<FIntPoint> CountedChokes;
    TSet<FIntPoint> CountedCrossings;

    int32 Steps = 0;
    const int32 MaxSteps = Grid.Width + Grid.Height;

    while (Steps < MaxSteps)
    {
        if (!Grid.IsValidCoord(X0, Y0))
        {
            return false;
        }

        const FRTSCell& Cell = Grid.GetCell(X0, Y0);
        FIntPoint Pos(X0, Y0);

        if (Cell.TacticalZone == ERTSTacticalZone::ChokePoint && !CountedChokes.Contains(Pos))
        {
            ++OutChokes;
            CountedChokes.Add(Pos);
        }
        if (Cell.TacticalZone == ERTSTacticalZone::RiverCrossing && !CountedCrossings.Contains(Pos))
        {
            ++OutRiverCrossings;
            CountedCrossings.Add(Pos);
        }

        if (X0 == X1 && Y0 == Y1)
        {
            break;
        }

        int32 E2 = 2 * Err;
        if (E2 > -Dy) { Err -= Dy; X0 += Sx; }
        if (E2 < Dx)  { Err += Dx; Y0 += Sy; }
        ++Steps;
    }

    return true;
}
