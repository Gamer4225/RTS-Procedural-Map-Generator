#pragma once

#include "CoreMinimal.h"

/**
 * V1 Density Budget System: Prevents HISM/ISM spawning from destroying editor performance.
 * 
 * CRITICAL: RTS maps can easily spawn 10,000+ instances if uncontrolled.
 * This system enforces hard caps per category, scaled by map cell count.
 * 
 * Budget tiers (per 256×256 = 65,536 cells):
 *   Props (trees/rocks):     2,000 instances max (3% density)
 *   Resources:               64 clusters max
 *   Decorative (grass/etc.):   4,000 instances max (6% density)
 *   Total combined:          6,000 instances max
 * 
 * Enforcement: If budget exceeded, deterministic culling (nearest-to-seed-point kept).
 */
struct RTSMAPFORGERUNTIME_API FRTSDensityBudget
{
    int32 MaxProps = 0;
    int32 MaxResources = 0;
    int32 MaxDecorative = 0;
    int32 MaxTotal = 0;
    
    int32 PlacedProps = 0;
    int32 PlacedResources = 0;
    int32 PlacedDecorative = 0;
    int32 PlacedTotal = 0;
    
    // Initialize budgets scaled by grid cell count
    void Initialize(int32 GridCellCount);
    
    // Check if placement allowed. Returns true if within budget.
    bool CanPlaceProp() const;
    bool CanPlaceResource() const;
    bool CanPlaceDecorative() const;
    
    // Record placement. Returns false if over budget (caller should cull).
    bool RecordProp();
    bool RecordResource();
    bool RecordDecorative();
    
    // Get remaining slots
    int32 GetRemainingProps() const { return FMath::Max(0, MaxProps - PlacedProps); }
    int32 GetRemainingResources() const { return FMath::Max(0, MaxResources - PlacedResources); }
    int32 GetRemainingTotal() const { return FMath::Max(0, MaxTotal - PlacedTotal); }
    
    // Total instance count cap enforcement
    bool IsTotalBudgetExhausted() const { return PlacedTotal >= MaxTotal; }
    
    FString GenerateReport() const;
};
