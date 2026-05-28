#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Core/URTSGenerationSettings.h"
class RTSMAPFORGERUNTIME_API FRTSBridgeDetector
{
public:
    void DetectCrossings(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings);
private:
    struct FCrossingCandidate { FIntPoint Position; int32 RegionA=INDEX_NONE,RegionB=INDEX_NONE,WidthCells=0; float TrafficScore=0,ProximityScore=0,OverallScore=0; };
    TArray<FCrossingCandidate> FindCandidates(const FRTSGrid& Grid) const;
    FCrossingCandidate EvaluateCrossing(const FRTSGrid& Grid, int32 X, int32 Y) const;
    int32 MeasureWaterWidth(const FRTSGrid& Grid, int32 SX, int32 SY, int32 DX, int32 DY, int32 MaxW) const;
    void ScoreTraffic(FCrossingCandidate& C, const FRTSGrid& Grid, const FRTSMapMetadata& Meta) const;
    void ScoreProximity(FCrossingCandidate& C, const FRTSMapMetadata& Meta, const FRTSGrid& Grid) const;
    void CommitCrossings(FRTSGrid& Grid, FRTSMapMetadata& Meta, TArray<FCrossingCandidate>& Cands, int32 Max) const;
};
