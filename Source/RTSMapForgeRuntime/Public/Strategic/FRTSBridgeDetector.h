#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Core/URTSGenerationSettings.h"

/**
 * Bridge / Crossing Detection for V1.5
 * 
 * CRITICAL RTS FEATURE: Rivers create barriers, but CROSSINGS are where
 * battles happen. Natural fords and narrow river sections become:
 *   - Strategic control points
 *   - Battle hotspots
 *   - Expansion corridor gates
 * 
 * Algorithm:
 *   1. Scan water cells that border exactly 2+ distinct walkable regions
 *   2. Measure perpendicular water width at each candidate
 *   3. Score by: width (narrower = better), path traffic, proximity to bases/expansions
 *   4. Store in metadata; mark cells with RiverCrossing tactical zone
 *   5. Feed into strategic scoring (crossings = map readability)
 */
class RTSMAPFORGERUNTIME_API FRTSBridgeDetector
{
public:
    void DetectCrossings(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings);

private:
    struct FCrossingCandidate
    {
        FIntPoint Position;
        int32 WidthCells = 0;          // Perpendicular water width
        int32 RegionA = INDEX_NONE;    // One side
        int32 RegionB = INDEX_NONE;    // Other side
        float TrafficScore = 0.0f;     // How many A* paths would cross here
        float ProximityScore = 0.0f;   // Near bases/expansions
        float OverallScore = 0.0f;
    };

    TArray<FCrossingCandidate> FindCandidates(const FRTSGrid& Grid) const;
    
    // Measures water width scanning perpendicular to boundary direction
    // Returns count of contiguous water cells in scan direction
    int32 MeasureWaterWidth(const FRTSGrid& Grid, int32 StartX, int32 StartY, int32 DirX, int32 DirY, int32 MaxWidth) const;
    
    // Finds the narrowest water crossing between two regions near (X,Y)
    FCrossingCandidate EvaluateCrossing(const FRTSGrid& Grid, int32 X, int32 Y) const;
    
    void ScoreTraffic(FCrossingCandidate& Candidate, const FRTSGrid& Grid, const FRTSMapMetadata& Metadata) const;
    void ScoreProximity(FCrossingCandidate& Candidate, const FRTSMapMetadata& Metadata, const FRTSGrid& Grid) const;
    void CommitCrossings(FRTSGrid& Grid, FRTSMapMetadata& Metadata, TArray<FCrossingCandidate>& Candidates, int32 MaxCrossings) const;
};
