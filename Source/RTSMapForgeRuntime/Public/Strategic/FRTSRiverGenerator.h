#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"

class URTSGenerationSettings;
class UFRTSSeedManager;

/**
 * V1 River Generation: Gradient descent from mountain peaks with
 * DETERMINISTIC lateral jitter and multi-cell widening.
 * 
 * CRITICAL DESIGN: Intentionally simple — NO fluid simulation.
 * Rivers are gameplay features first: traversal blockers, tactical separators, choke creators.
 * 
 * Pipeline:
 *   1. Find local maxima above MountainLevel
 *   2. Deterministically select spaced source points
 *   3. Trace downhill with DETERMINISTIC lateral bias (NOT random — prevents straight artificial lines)
 *   4. WIDEN to 2-3 cells for RTS readability
 *   5. Carve river beds below water level
 *   6. Smooth adjacent banks
 *   7. Update traversal: water = blocked
 *   8. Contextual StrategicValue based on crossing pressure and expansion proximity
 */
class RTSMAPFORGERUNTIME_API FRTSRiverGenerator
{
public:
    void Generate(FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager);

private:
    struct FRiverPath
    {
        TArray<FIntPoint> Cells;
    };

    TArray<FIntPoint> FindPeaks(const FRTSGrid& Grid, float MountainLevel) const;
    TArray<FIntPoint> SelectSources(const TArray<FIntPoint>& Peaks, int32 NumRivers, float MinDistSq, UFRTSSeedManager* SeedManager) const;
    
    FRiverPath TraceRiverWithWidening(FRTSGrid& Grid, FIntPoint Source, float WaterLevel, int32 NeighborBuffer[8], UFRTSSeedManager* SeedManager);
    
    void CarveRiverBeds(FRTSGrid& Grid, float WaterLevel);
    void SmoothRiverbanks(FRTSGrid& Grid, float WaterLevel);
    void ApplyContextualStrategicValue(FRTSGrid& Grid, const TArray<FRiverPath>& RiverPaths, const URTSGenerationSettings* Settings);
    
    // Deterministic lateral bias: nudges river away from perfectly straight downhill paths
    // Returns modified neighbor score; lower = preferred
    float ApplyLateralBias(FIntPoint Current, FIntPoint Candidate, FIntPoint Previous, float BaseScore, int32 StepIndex, int32 Seed) const;
};
