#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"

/**
 * Tactical zone classification with STRICT priority rules.
 * 
 * PRIORITY (highest → lowest):
 *   1. MainBase        ← Set by base placer, NEVER overridden
 *   2. ChokePoint      ← Set by choke detector
 *   3. NatExpansion    ← Set by expansion placer
 *   4. ContestedExp    ← Set by expansion placer (high risk)
 *   5. ResourceCluster  ← Set by resource placer
 *   6. HighGround      ← Elevation-based, filters small pockets
 *   7. OpenBattlefield ← Default walkable
 *   8. Unclassified    ← Default unwalkable
 * 
 * Earlier pipeline stages (Base, Expansion, Choke, Resource placement) assign
 * their zones DIRECTLY to cells. This classifier ONLY fills in remaining
 * unclassified cells, NEVER overwriting existing strategic assignments.
 */
class RTSMAPFORGERUNTIME_API FRTSTacticalZoneClassifier
{
public:
    void Classify(FRTSGrid& Grid, const FRTSMapMetadata& Metadata);

private:
    // Remove high-ground candidates that are too small (isolated peaks)
    void FilterSmallRegions(FRTSGrid& Grid, TArray<bool>& CandidateMask, int32 MinSize) const;
};
