#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Core/FRTSSeedManager.h"
#include "Core/URTSGenerationSettings.h"
#include "Data/FRTSMapMetadata.h"
#include "Validation/FRTSValidationResult.h"
#include "Terrain/FRTSNoiseGenerator.h"   // FIX Bug 1: FRTSNoiseGenerator was used as a
                                           // member (NoiseGen) but never included. Added here.

/**
 * RAII guard to manage UObject root-set membership.
 * Automatically adds to root on construction and removes on destruction.
 * Safe against early returns or exceptions.
 */
class FRTSRootGuard
{
public:
    explicit FRTSRootGuard(UObject* InObject)
        : Object(InObject)
    {
        if (Object)
        {
            Object->AddToRoot();
        }
    }

    ~FRTSRootGuard()
    {
        if (Object)
        {
            Object->RemoveFromRoot();
        }
    }

    // Non-copyable, non-movable
    FRTSRootGuard(const FRTSRootGuard&) = delete;
    FRTSRootGuard(FRTSRootGuard&&) = delete;
    FRTSRootGuard& operator=(const FRTSRootGuard&) = delete;
    FRTSRootGuard& operator=(FRTSRootGuard&&) = delete;

private:
    UObject* Object = nullptr;
};

/**
 * Master generation pipeline orchestrator.
 *
 * CRITICAL ARCHITECTURAL RULES:
 * 1. Seed is resolved EXACTLY ONCE in Generate() and NEVER again anywhere.
 *    All generators receive the resolved seed as an explicit int64 parameter.
 * 2. SeedManager is AddToRoot/RemoveFromRoot scoped to Generate() call ONLY.
 * 3. All stages are deterministic given the same ResolvedSeed + same Settings.
 * 4. No subsystem, generator, or UI code may call Settings->ResolveSeed() except here.
 *
 * V1.5 Pipeline (20 stages):
 *  1.  Seed Init       → FRandomStream + Perlin permutation
 *  2.  Grid Alloc      → flat TArray[W*H]
 *  3.  Heightmap (FBM) → Perlin octaves → normalized height
 *  3b. Radial Falloff  → Island shaping
 *  4.  Terrain Classify→ Water, Cliff, Buildable
 *  5.  Biome Assignment→ Voronoi seeds per biome asset
 *  6.  River Generation→ Gradient descent + widening + lateral jitter
 *  6b. Reclassify Rivers→ Update traversal after water carving
 *  6c. Water Validation→ Connectivity check
 *  7.  Region Detection→ BFS flood fill
 *  8.  Base Placement  → 180° symmetry (2p) + Poisson fallback
 *  9.  Expansion Placement → Risk score by distance
 *  10. Choke Detection → Region boundary width analysis
 *  10b.Bridge/Crossing Detection ← NEW V1.5: Narrow river crossings
 *  10c.Resource Placement → Strategic Poisson-disk
 *  11. Tactical Zones  → Priority hierarchy classification
 *  12. A* Pathfinding  → Octile heuristic; rush distances stored in Metadata.RushDistances
 *  13. Influence Maps  → Inverse-square control
 *  14. Heatmaps        → Combat/traversal density
 *  15. Strategic Scoring → Balance, Rush, Choke, Overall
 *  16. Validation      → 6-pass + ResourceAccessibility + WaterConnectivity
 *  16b.Resource Accessibility Validation ← NEW V1.5: path cost + safety parity
 */
class RTSMAPFORGERUNTIME_API FRTSGenerationPipeline
{
public:
    FRTSGenerationPipeline();
    virtual ~FRTSGenerationPipeline();

    // Main entry point. Populates OutGrid and OutMetadata.
    // Returns the resolved seed that was used for this generation.
    // If bValidateAndRetry, will loop until validation passes or MaxRetries hit.
    // CRITICAL: Seed is resolved EXACTLY ONCE inside this function.
    int64 Generate(URTSGenerationSettings* Settings, FRTSGrid& OutGrid, FRTSMapMetadata& OutMetadata, FRTSValidationResult& OutValidation, int32 MaxRetries = 10);

    // Access the seed that was resolved during the last Generate() call.
    int64 GetLastResolvedSeed() const { return LastResolvedSeed; }

    // Access to latest seed manager for external queries
    UFRTSSeedManager* GetSeedManager() const { return SeedManager; }

    // Individual stages — exposed for debugging/testing
    void Stage1_SeedInit(URTSGenerationSettings* Settings, int64 ResolvedSeed);
    void Stage2_GridAlloc(FRTSGrid& OutGrid, URTSGenerationSettings* Settings);
    void Stage3_Heightmap(FRTSGrid& Grid, URTSGenerationSettings* Settings, int64 ResolvedSeed);
    void Stage3b_RadialFalloff(FRTSGrid& Grid, URTSGenerationSettings* Settings);
    void Stage4_ClassifyTerrain(FRTSGrid& Grid, URTSGenerationSettings* Settings);
    void Stage5_BiomeAssignment(FRTSGrid& Grid, URTSGenerationSettings* Settings);
    void Stage6_RiverGeneration(FRTSGrid& Grid, URTSGenerationSettings* Settings);
    void Stage6b_ReclassifyAfterRivers(FRTSGrid& Grid, URTSGenerationSettings* Settings);
    void Stage6c_WaterValidation(FRTSGrid& Grid, FRTSValidationResult& OutValidation);
    void Stage7_RegionDetection(FRTSGrid& Grid);
    void Stage8_BasePlacement(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings);
    void Stage9_ExpansionPlacement(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings);
    void Stage10_ChokeDetection(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings);
    void Stage10b_BridgeDetection(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings);
    void Stage10c_ResourcePlacement(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings);
    void Stage11_TacticalZones(FRTSGrid& Grid, FRTSMapMetadata& Metadata);
    void Stage12_Pathfinding(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings);
    void Stage13_InfluenceMaps(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings);
    void Stage14_Heatmaps(FRTSGrid& Grid, FRTSMapMetadata& Metadata, URTSGenerationSettings* Settings);
    void Stage15_StrategicScoring(FRTSGrid& Grid, FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, URTSGenerationSettings* Settings);
    void Stage16_Validation(FRTSGrid& Grid, FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, URTSGenerationSettings* Settings);
    void Stage16b_ResourceAccessibilityValidation(FRTSGrid& Grid, FRTSMapMetadata& Metadata, FRTSValidationResult& OutResult, URTSGenerationSettings* Settings);

private:
    TObjectPtr<UFRTSSeedManager> SeedManager;
    FRTSNoiseGenerator NoiseGen;
    URTSGenerationSettings* CurrentSettings = nullptr;
    int64 LastResolvedSeed = 0;

    void ResetForNewGeneration();
};
