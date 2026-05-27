#include "Validation/FRTSValidationPipeline.h"
#include "Pathfinding/FRTSAStarSolver.h"
#include "Math/UnrealMathUtility.h"

void FRTSValidationPipeline::Validate(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, const URTSGenerationSettings* Settings)
{
    OutResult.Issues.Empty();
    OutResult.bPassed = false;

    Pass1_Traversal(Grid, Metadata, OutResult, Settings);
    Pass2_Spawn(Grid, Metadata, OutResult);
    Pass3_Economy(Grid, Metadata, OutResult, Settings);
    Pass4_Choke(Grid, Metadata, OutResult);
    Pass5_Navmesh(Grid, Metadata, OutResult, Settings);
    Pass6_Fairness(Grid, Metadata, OutResult, Settings);

    OutResult.bPassed = !OutResult.HasCriticalFailure();
}

// ========================= PASS 1: TRAVERSAL =========================
void FRTSValidationPipeline::Pass1_Traversal(
    const FRTSGrid& Grid,
    const FRTSMapMetadata& Metadata,
    FRTSValidationResult& OutResult,
    const URTSGenerationSettings* Settings) const
{
    // CRITICAL: Every base must reach every other base via A*.
    // This is fundamental for RTS: if players cannot attack each other, the map is broken.
    FRTSAStarSolver Solver;
    const int32 NumBases = Metadata.Bases.Num();

    // Validate each pair
    for (int32 i = 0; i < NumBases; ++i)
    {
        for (int32 j = i + 1; j < NumBases; ++j)
        {
            FIntPoint A(
                FMath::FloorToInt(Metadata.Bases[i].GridPosition.X),
                FMath::FloorToInt(Metadata.Bases[i].GridPosition.Y)
            );
            FIntPoint B(
                FMath::FloorToInt(Metadata.Bases[j].GridPosition.X),
                FMath::FloorToInt(Metadata.Bases[j].GridPosition.Y)
            );

            float Cost = Solver.FindPathCost(Grid, A, B, /*MaxIterations=*/50000);
            if (Cost < 0.0f)
            {
                OutResult.Issues.Add(FRTSValidationIssue{
                    TEXT("Traversal"),
                    FString::Printf(TEXT("Base %d UNREACHABLE from Base %d — map is broken for multiplayer"), i, j),
                    ERTSValidationSeverity::Critical
                });
            }
            else
            {
                // Also warn if rush distance is suspiciously short/long
                float MapDiag = FMath::Sqrt(static_cast<float>(Grid.Width * Grid.Width + Grid.Height * Grid.Height));
                float NormalizedDist = Cost / MapDiag;
                if (NormalizedDist < Settings->MinRushDistance * 0.5f)
                {
                    OutResult.Issues.Add(FRTSValidationIssue{
                        TEXT("Traversal"),
                        FString::Printf(TEXT("Base %d to Base %d rush distance %.2f is very short (map may be too small for %d players)"), i, j, NormalizedDist),
                        ERTSValidationSeverity::Warning
                    });
                }
            }
        }
    }

    // Also validate: each expansion must be reachable from at least one base
    for (const FRTSExpansionInfo& Exp : Metadata.Expansions)
    {
        FIntPoint ExpPos(FMath::FloorToInt(Exp.GridPosition.X), FMath::FloorToInt(Exp.GridPosition.Y));
        bool bReachable = false;
        
        for (const FRTSBaseInfo& Base : Metadata.Bases)
        {
            FIntPoint BasePos(FMath::FloorToInt(Base.GridPosition.X), FMath::FloorToInt(Base.GridPosition.Y));
            float Cost = Solver.FindPathCost(Grid, BasePos, ExpPos, /*MaxIterations=*/30000);
            if (Cost >= 0.0f)
            {
                bReachable = true;
                break;
            }
        }

        if (!bReachable)
        {
            OutResult.Issues.Add(FRTSValidationIssue{
                TEXT("Traversal"),
                FString::Printf(TEXT("Expansion at (%.0f, %.0f) is UNREACHABLE from any base — blocked by water/cliffs"), Exp.GridPosition.X, Exp.GridPosition.Y),
                ERTSValidationSeverity::Warning
            });
        }
    }
}

// ========================= PASS 2: SPAWN AREA =========================
void FRTSValidationPipeline::Pass2_Spawn(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult) const
{
    const int32 MinRadius = 5; // Minimum 5-cell radius for building space
    
    for (const FRTSBaseInfo& Base : Metadata.Bases)
    {
        int32 X = FMath::FloorToInt(Base.GridPosition.X);
        int32 Y = FMath::FloorToInt(Base.GridPosition.Y);
        bool bOK = true;
        int32 BlockedCount = 0;
        
        for (int32 dy = -MinRadius; dy <= MinRadius && bOK; ++dy)
        {
            for (int32 dx = -MinRadius; dx <= MinRadius; ++dx)
            {
                if (!Grid.IsValidCoord(X + dx, Y + dy))
                {
                    bOK = false;
                    break;
                }
                if (!Grid.GetCell(X + dx, Y + dy).bBuildable)
                {
                    ++BlockedCount;
                    if (BlockedCount > 8) // Allow a few blocked cells (minor terrain variation)
                    {
                        bOK = false;
                        break;
                    }
                }
            }
        }
        
        if (!bOK)
        {
            OutResult.Issues.Add(FRTSValidationIssue{
                TEXT("Spawn"),
                FString::Printf(TEXT("Base %d lacks adequate buildable area (%d blocked cells in %dx%d radius)"), Base.PlayerIndex, BlockedCount, MinRadius * 2 + 1, MinRadius * 2 + 1),
                ERTSValidationSeverity::Critical
            });
        }
    }
}

// ========================= PASS 3: ECONOMY FAIRNESS =========================
void FRTSValidationPipeline::Pass3_Economy(
    const FRTSGrid& Grid,
    const FRTSMapMetadata& Metadata,
    FRTSValidationResult& OutResult,
    const URTSGenerationSettings* Settings) const
{
    // Validate: expansion count per player is within 10% of each other
    TMap<int32, int32> ExpansionCounts;
    for (const FRTSExpansionInfo& Exp : Metadata.Expansions)
    {
        ExpansionCounts.FindOrAdd(Exp.OwnerPlayerIndex)++;
    }
    
    float MaxExp = 0.0f;
    float MinExp = MAX_FLT;
    for (const auto& Pair : ExpansionCounts)
    {
        MaxExp = FMath::Max(MaxExp, static_cast<float>(Pair.Value));
        MinExp = FMath::Min(MinExp, static_cast<float>(Pair.Value));
    }
    
    if (MaxExp > KINDA_SMALL_NUMBER && MinExp < MAX_FLT)
    {
        float Delta = (MaxExp - MinExp) / MaxExp;
        if (Delta > Settings->MaxFairnessError)
        {
            OutResult.Issues.Add(FRTSValidationIssue{
                TEXT("Economy"),
                FString::Printf(TEXT("Expansion imbalance: %.0f%% difference (max=%d, min=%d, threshold=%.0f%%)"), 
                    Delta * 100.0f, static_cast<int32>(MaxExp), static_cast<int32>(MinExp), Settings->MaxFairnessError * 100.0f),
                ERTSValidationSeverity::Warning
            });
        }
    }

    // NEW: Validate resource parity per player
    // Calculate total resource value within influence radius of each base
    TMap<int32, float> PlayerResourceValue;
    float TotalResourceValue = 0.0f;
    
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        if (Grid.Cells[i].ResourceValue > 0.0f)
        {
            TotalResourceValue += Grid.Cells[i].ResourceValue;
            
            // Attribute to nearest base owner
            float BestDist = MAX_FLT;
            int32 BestPlayer = INDEX_NONE;
            FIntPoint CellCoord = Grid.ToCoord(i);
            
            for (const FRTSBaseInfo& Base : Metadata.Bases)
            {
                float Dist = FVector2D::Distance(FVector2D(CellCoord.X, CellCoord.Y), Base.GridPosition);
                if (Dist < BestDist)
                {
                    BestDist = Dist;
                    BestPlayer = Base.PlayerIndex;
                }
            }
            
            if (BestPlayer != INDEX_NONE)
            {
                PlayerResourceValue.FindOrAdd(BestPlayer) += Grid.Cells[i].ResourceValue;
            }
        }
    }
    
    if (PlayerResourceValue.Num() > 1 && TotalResourceValue > KINDA_SMALL_NUMBER)
    {
        float MaxRes = 0.0f;
        float MinRes = MAX_FLT;
        for (const auto& Pair : PlayerResourceValue)
        {
            MaxRes = FMath::Max(MaxRes, Pair.Value);
            MinRes = FMath::Min(MinRes, Pair.Value);
        }
        
        if (MinRes > KINDA_SMALL_NUMBER)
        {
            float ResourceDelta = (MaxRes - MinRes) / MaxRes;
            if (ResourceDelta > Settings->MaxFairnessError)
            {
                OutResult.Issues.Add(FRTSValidationIssue{
                    TEXT("Economy"),
                    FString::Printf(TEXT("Resource parity imbalance: %.0f%% difference (max=%.1f, min=%.1f, threshold=%.0f%%) — late-game economy may be unfair"),
                        ResourceDelta * 100.0f, MaxRes, MinRes, Settings->MaxFairnessError * 100.0f),
                    ERTSValidationSeverity::Warning
                });
            }
        }
    }
}

// ========================= PASS 4: CHOKE STRUCTURE =========================
void FRTSValidationPipeline::Pass4_Choke(const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult) const
{
    int32 NumPlayers = Metadata.Bases.Num();
    int32 MinExpectedChokes = FMath::Max(1, NumPlayers - 1); // At least 1 choke for 2p, more for FFA
    
    if (Metadata.Chokes.Num() == 0 && NumPlayers >= 2)
    {
        OutResult.Issues.Add(FRTSValidationIssue{
            TEXT("Choke"),
            FString::Printf(TEXT("No choke points detected for %d players — map lacks strategic depth"), NumPlayers),
            ERTSValidationSeverity::Warning
        });
    }
    else if (Metadata.Chokes.Num() < MinExpectedChokes)
    {
        OutResult.Issues.Add(FRTSValidationIssue{
            TEXT("Choke"),
            FString::Printf(TEXT("Only %d chokes for %d players (recommend %d+) — map may feel too open"), Metadata.Chokes.Num(), NumPlayers, MinExpectedChokes),
            ERTSValidationSeverity::Warning
        });
    }
    
    if (Metadata.Chokes.Num() > 12)
    {
        OutResult.Issues.Add(FRTSValidationIssue{
            TEXT("Choke"),
            FString::Printf(TEXT("Excessive choke count: %d (max recommended 12) — map may feel too fragmented"), Metadata.Chokes.Num()),
            ERTSValidationSeverity::Warning
        });
    }
}

// ========================= PASS 5: NAVMESH WIDTH =========================
void FRTSValidationPipeline::Pass5_Navmesh(
    const FRTSGrid& Grid,
    const FRTSMapMetadata& Metadata,
    FRTSValidationResult& OutResult,
    const URTSGenerationSettings* Settings) const
{
    for (const FRTSChokeInfo& Choke : Metadata.Chokes)
    {
        if (Choke.WidthCells < Settings->MinChokeWidth)
        {
            OutResult.Issues.Add(FRTSValidationIssue{
                TEXT("Navmesh"),
                FString::Printf(TEXT("Choke between regions %d and %d too narrow: %d cells (min=%.0f) — units may get stuck"),
                    Choke.RegionA, Choke.RegionB, Choke.WidthCells, Settings->MinChokeWidth),
                ERTSValidationSeverity::Critical
            });
        }
        
        if (Choke.WidthCells > Settings->MaxChokeWidth)
        {
            OutResult.Issues.Add(FRTSValidationIssue{
                TEXT("Navmesh"),
                FString::Printf(TEXT("Choke between regions %d and %d very wide: %d cells (max recommended %.0f) — may not function as choke"),
                    Choke.RegionA, Choke.RegionB, Choke.WidthCells, Settings->MaxChokeWidth),
                ERTSValidationSeverity::Warning
            });
        }
    }
}

// ========================= PASS 6: STRATEGIC FAIRNESS =========================
void FRTSValidationPipeline::Pass6_Fairness(
    const FRTSGrid& Grid,
    const FRTSMapMetadata& Metadata,
    FRTSValidationResult& OutResult,
    const URTSGenerationSettings* Settings) const
{
    float MinScore = Settings->MinAcceptableScore;
    if (OutResult.OverallScore < MinScore)
    {
        OutResult.Issues.Add(FRTSValidationIssue{
            TEXT("Fairness"),
            FString::Printf(TEXT("Overall strategic score %.1f below threshold %.1f — map may be unbalanced or strategically shallow"),
                OutResult.OverallScore, MinScore),
            ERTSValidationSeverity::Warning
        });
    }
    
    // Also warn if no bases were placed at all (pipeline failure)
    if (Metadata.Bases.Num() == 0)
    {
        OutResult.Issues.Add(FRTSValidationIssue{
            TEXT("Fairness"),
            TEXT("No bases placed — generation pipeline failed completely"),
            ERTSValidationSeverity::Critical
        });
    }
}
