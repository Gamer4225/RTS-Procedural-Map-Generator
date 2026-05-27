#include "Core/FRTSGenerationPipeline.h"
#include "Terrain/FRTSHeightmapGenerator.h"
#include "Terrain/FRTSBiomeAssigner.h"
#include "Strategic/FRTSRegionDetector.h"
#include "Strategic/FRTSBasePlacer.h"
#include "Strategic/FRTSExpansionPlacer.h"
#include "Strategic/FRTSChokeDetector.h"
#include "Strategic/FRTSBridgeDetector.h"
#include "Strategic/FRTSResourcePlacer.h"
#include "Strategic/FRTSTacticalZoneClassifier.h"
#include "Pathfinding/FRTSAStarSolver.h"
#include "Pathfinding/FRTSFlowField.h"
#include "Analysis/FRTSInfluenceMap.h"
#include "Analysis/FRTSHeatmapSystem.h"
#include "Analysis/FRTSStrategicScorer.h"
#include "Analysis/FRTSFairnessAnalyzer.h"
#include "Validation/FRTSValidationPipeline.h"
#include "Validation/FRTSWaterConnectivityValidator.h"
#include "Validation/FRTSResourceAccessibilityValidator.h"

FRTSGenerationPipeline::FRTSGenerationPipeline()
{
    // SeedManager is a UObject subclass — create in transient package.
    // Do NOT AddToRoot() here; that creates a permanent leak.
    // Scoped AddToRoot/RemoveFromRoot happens inside Generate().
    SeedManager = NewObject<UFRTSSeedManager>(GetTransientPackage(), FName(TEXT("RTSMapForgeSeedManager")));
}

FRTSGenerationPipeline::~FRTSGenerationPipeline()
{
    // SeedManager is a UPROPERTY and will be garbage collected automatically.
    // Rooting is handled by RAII guard in Generate(), not here.
    SeedManager = nullptr;
}

void FRTSGenerationPipeline::ResetForNewGeneration()
{
    NoiseGen = FRTSNoiseGenerator();
}

int64 FRTSGenerationPipeline::Generate(
    URTSGenerationSettings* Settings,
    FRTSGrid& OutGrid,
    FRTSMapMetadata& OutMetadata,
    FRTSValidationResult& OutValidation,
    int32 MaxRetries)
{
    OutValidation.Issues.Empty();
    OutValidation.bPassed = false;
    LastResolvedSeed = 0;
    int64 EffectiveSeed = 0;
    const int64 OriginalSeed = Settings ? Settings->Seed : 0;
    const bool OriginalRandomSeed = Settings ? Settings->bRandomSeed : false;

    if (!Settings)
    {
        OutValidation.Issues.Add(FRTSValidationIssue{ TEXT("Init"), TEXT("Settings null"), ERTSValidationSeverity::Critical });
        return 0;
    }

    CurrentSettings = Settings;

    // ================================================================
    // CRITICAL: Resolve seed EXACTLY ONCE here.
    // This is the ONLY place in the entire codebase that may call ResolveSeed().
    // All generators, UI, and metadata systems receive the resolved seed explicitly.
    // ================================================================
    LastResolvedSeed = Settings->ResolveSeed();
    EffectiveSeed = LastResolvedSeed;

    // RAII guard to manage SeedManager root-set membership (safe against early returns)
    FRTSRootGuard SeedManagerRootGuard(SeedManager);

    for (int32 Retry = 0; Retry < MaxRetries; ++Retry)
    {
        ResetForNewGeneration();

        // Deterministic mutation: InitialSeed + Retry, NOT another ResolveSeed() call
        int64 CurrentSeed = LastResolvedSeed + Retry;
        EffectiveSeed = CurrentSeed;

        // Force deterministic mode for all retries
        Settings->Seed = CurrentSeed;
        Settings->bRandomSeed = false;

        Stage1_SeedInit(Settings, CurrentSeed);
        Stage2_GridAlloc(OutGrid, Settings);
        Stage3_Heightmap(OutGrid, Settings, CurrentSeed);
        Stage3b_RadialFalloff(OutGrid, Settings);
        Stage4_ClassifyTerrain(OutGrid, Settings);
        Stage5_BiomeAssignment(OutGrid, Settings);
        Stage6_RiverGeneration(OutGrid, Settings);
        Stage6b_ReclassifyAfterRivers(OutGrid, Settings);
        Stage6c_WaterValidation(OutGrid, OutValidation);
        Stage7_RegionDetection(OutGrid);
        Stage8_BasePlacement(OutGrid, OutMetadata, Settings);
        Stage9_ExpansionPlacement(OutGrid, OutMetadata, Settings);
        Stage10_ChokeDetection(OutGrid, OutMetadata, Settings);
        Stage10b_BridgeDetection(OutGrid, OutMetadata, Settings);
        Stage10c_ResourcePlacement(OutGrid, OutMetadata, Settings);
        Stage11_TacticalZones(OutGrid, OutMetadata);
        Stage12_Pathfinding(OutGrid, OutMetadata, Settings);
        Stage13_InfluenceMaps(OutGrid, OutMetadata, Settings);
        Stage14_Heatmaps(OutGrid, OutMetadata, Settings);
        Stage15_StrategicScoring(OutGrid, OutMetadata, OutValidation, Settings);
        Stage16_Validation(OutGrid, OutMetadata, OutValidation, Settings);
        Stage16b_ResourceAccessibilityValidation(OutGrid, OutMetadata, OutValidation, Settings);

        OutValidation.RetryCount = Retry;

        if (!OutValidation.HasCriticalFailure())
        {
            OutValidation.bPassed = true;
            break;
        }
    }

    // RAII guard automatically removes SeedManager from root here (destructor called)

    // Store effective seed (including retry offset) in metadata for downstream consumers
    OutMetadata.Seed = EffectiveSeed;

    // Restore caller-owned settings so Generate() does not leave side effects behind.
    Settings->Seed = OriginalSeed;
    Settings->bRandomSeed = OriginalRandomSeed;
    CurrentSettings = nullptr;

    return EffectiveSeed;
}

// ========================= STAGE 1 =========================
void FRTSGenerationPipeline::Stage1_SeedInit(URTSGenerationSettings* Settings, int64 ResolvedSeed)
{
    SeedManager->Initialize(ResolvedSeed);
    int32 NoiseSeed = static_cast<int32>(ResolvedSeed ^ (ResolvedSeed >> 32));
    NoiseGen.Initialize(NoiseSeed);
}

// ========================= STAGE 2 =========================
void FRTSGenerationPipeline::Stage2_GridAlloc(FRTSGrid& OutGrid, URTSGenerationSettings* Settings)
{
    OutGrid.Initialize(Settings->GridWidth, Settings->GridHeight, Settings->CellSize);
}

// ========================= STAGE 3 =========================
// CRITICAL: Pass ResolvedSeed explicitly. Generator must NOT call ResolveSeed().
void FRTSGenerationPipeline::Stage3_Heightmap(FRTSGrid& Grid, URTSGenerationSettings* Settings, int64 ResolvedSeed)
{
    FRTSHeightmapGenerator HeightGen;
    HeightGen.Generate(Grid, Settings, NoiseGen, ResolvedSeed);
}

// ========================= STAGE 3b =========================
void FRTSGenerationPipeline::Stage3b_RadialFalloff(FRTSGrid& Grid, URTSGenerationSettings* Settings)
{
    float FalloffStrength = 0.3f;
    FRTSHeightmapGenerator HeightGen;
    HeightGen.ApplyRadialFalloff(Grid, FalloffStrength);
}

// ========================= STAGE 4 =========================
void FRTSGenerationPipeline::Stage4_ClassifyTerrain(FRTSGrid& Grid, URTSGenerationSettings* Settings)
{
    const float WaterLevel = Settings->WaterLevel;

    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        FRTSCell& Cell = Grid.Cells[i];

        if (Cell.Height < WaterLevel)
        {
            Cell.bWater = true;
            Cell.bWalkable = false;
            Cell.bBuildable = false;
            Cell.MovementCostMultiplier = 0.0f;
        }
        else
        {
            Cell.bWater = false;
            float SlopeDegrees = FMath::RadiansToDegrees(Cell.Slope);
            if (SlopeDegrees > 45.0f)
            {
                Cell.bCliff = true;
                Cell.bWalkable = false;
                Cell.bBuildable = false;
                Cell.MovementCostMultiplier = 0.0f;
            }
            else
            {
                Cell.bCliff = false;
                Cell.bWalkable = true;
                Cell.bBuildable = (SlopeDegrees < 15.0f);
                Cell.MovementCostMultiplier = 1.0f + (SlopeDegrees / 45.0f);
            }
        }
    }
}

// ========================= STAGE 5 =========================
void FRTSGenerationPipeline::Stage5_BiomeAssignment(FRTSGrid& Grid, URTSGenerationSettings* Settings)
{
    FRTSBiomeAssigner Assigner;
    Assigner.Assign(Grid, Settings, SeedManager);
}

// ========================= STAGE 6 =========================
void FRTSGenerationPipeline::Stage6_RiverGeneration(FRTSGrid& Grid, URTSGenerationSettings* Settings)
{
    FRTSRiverGenerator Rivers;
    Rivers.Generate(Grid, Settings, SeedManager);
}

// ========================= STAGE 6b =========================
void FRTSGenerationPipeline::Stage6b_ReclassifyAfterRivers(FRTSGrid& Grid, URTSGenerationSettings* Settings)
{
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
}

// ========================= STAGE 6c =========================
void FRTSGenerationPipeline::Stage6c_WaterValidation(FRTSGrid& Grid, FRTSValidationResult& OutValidation)
{
    FRTSWaterConnectivityValidator WaterValidator;
    bool bHasIsolated = false;
    WaterValidator.ValidateWaterConnectivity(Grid, OutValidation, bHasIsolated);

    int32 IsolatedLand = WaterValidator.CountIsolatedLandRegions(Grid);
    if (IsolatedLand > 0)
    {
        OutValidation.Issues.Add(FRTSValidationIssue{
            TEXT("WaterConnectivity"),
            FString::Printf(TEXT("%d isolated land region(s) surrounded by water"), IsolatedLand),
            ERTSValidationSeverity::Warning
        });
    }
}

// ========================= STAGE 7 =========================
void FRTSGenerationPipeline::Stage7_RegionDetection(FRTSGrid& Grid)
{
    FRTSRegionDetector Detector;
    Detector.DetectRegions(Grid);
}

// ========================= STAGE 8 =========================
void FRTSGenerationPipeline::Stage8_BasePlacement(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings)
{
    FRTSBasePlacer Placer;
    Placer.PlaceBases(Grid, Metadata, Settings, SeedManager);
}

// ========================= STAGE 9 =========================
void FRTSGenerationPipeline::Stage9_ExpansionPlacement(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings)
{
    FRTSExpansionPlacer Placer;
    Placer.PlaceExpansions(Grid, Metadata, Settings, SeedManager);
}

// ========================= STAGE 10 =========================
void FRTSGenerationPipeline::Stage10_ChokeDetection(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings)
{
    FRTSChokeDetector Detector;
    Detector.DetectChokes(Grid, Metadata, Settings);
}

// ========================= STAGE 10b =========================
void FRTSGenerationPipeline::Stage10b_BridgeDetection(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings)
{
    FRTSBridgeDetector Bridges;
    Bridges.DetectCrossings(Grid, Metadata, Settings);
}

// ========================= STAGE 10c =========================
void FRTSGenerationPipeline::Stage10c_ResourcePlacement(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings)
{
    FRTSResourcePlacer Resources;
    Resources.PlaceResources(Grid, Metadata, Settings, SeedManager);
}

// ========================= STAGE 11 =========================
void FRTSGenerationPipeline::Stage11_TacticalZones(FRTSGrid& Grid, FRTSMapMetadata& Metadata)
{
    FRTSTacticalZoneClassifier Classifier;
    Classifier.Classify(Grid, Metadata);
}

// ========================= STAGE 12 =========================
void FRTSGenerationPipeline::Stage12_Pathfinding(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings)
{
    FRTSAStarSolver Solver;
    Solver.ComputeRushDistances(Grid, Metadata, Settings);
}

// ========================= STAGE 13 =========================
void FRTSGenerationPipeline::Stage13_InfluenceMaps(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings)
{
    FRTSInfluenceMap Influence;
    Influence.Generate(Grid, Metadata, Settings);
}

// ========================= STAGE 14 =========================
void FRTSGenerationPipeline::Stage14_Heatmaps(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings)
{
    FRTSHeatmapSystem Heatmaps;
    Heatmaps.GenerateAll(Grid, Metadata, Settings);
}

// ========================= STAGE 15 =========================
void FRTSGenerationPipeline::Stage15_StrategicScoring(FRTSGrid& Grid, FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, URTSGenerationSettings* Settings)
{
    FRTSStrategicScorer Scorer;
    Scorer.Score(Grid, Metadata, OutResult, Settings);
}

// ========================= STAGE 16 =========================
void FRTSGenerationPipeline::Stage16_Validation(FRTSGrid& Grid, FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, URTSGenerationSettings* Settings)
{
    FRTSValidationPipeline Validator;
    Validator.Validate(Grid, Metadata, OutResult, Settings);
}

// ========================= STAGE 16b =========================
void FRTSGenerationPipeline::Stage16b_ResourceAccessibilityValidation(
    FRTSGrid& Grid,
    FRTSMapMetadata& Metadata,
    FRTSValidationResult& OutResult,
    URTSGenerationSettings* Settings)
{
    FRTSResourceAccessibilityValidator AccessibilityValidator;
    AccessibilityValidator.Validate(Grid, Metadata, OutResult, Settings);
}
