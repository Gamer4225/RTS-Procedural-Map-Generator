#pragma once

#include "CoreMinimal.h"
#include "Strategic/ERTSTacticalZone.h"
#include "FRTSCell.generated.h"

/**
 * Atomic gameplay-aware cell data for the RTS map grid.
 * 
 * DESIGN: Lightweight, data-oriented struct. NO dynamic allocations per cell.
 * Neighbor indices are computed dynamically via FRTSGrid::GetNeighbors().
 * 
 * Blueprint exposure is MINIMAL — only query-friendly properties.
 * Internal runtime data (RegionID, ChunkID, slope) is kept as plain members
 * to avoid reflection overhead and serialization bloat.
 */
USTRUCT(BlueprintType)
struct RTSMAPFORGERUNTIME_API FRTSCell
{
    GENERATED_BODY()

    // === SPATIAL (Minimal Blueprint exposure) ===
    UPROPERTY(BlueprintReadOnly, Category = "Spatial")
    FVector2D GridCoord = FVector2D::ZeroVector;

    FVector WorldPosition = FVector::ZeroVector;
    float Height = 0.0f;       // Normalized 0..1
    float Slope = 0.0f;        // Radians from flat

    // === TRAVERSAL (Blueprint-read for designers) ===
    UPROPERTY(BlueprintReadOnly, Category = "Traversal")
    float MovementCostMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Traversal")
    bool bWalkable = true;

    UPROPERTY(BlueprintReadOnly, Category = "Traversal")
    bool bBuildable = true;

    UPROPERTY(BlueprintReadOnly, Category = "Traversal")
    bool bWater = false;

    UPROPERTY(BlueprintReadOnly, Category = "Traversal")
    bool bCliff = false;

    // === GAMEPLAY (Designer-visible) ===
    UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
    float CoverValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
    float VisibilityScore = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Gameplay")
    float ExposureScore = 0.0f;

    // === STRATEGIC (Designer-visible) ===
    UPROPERTY(BlueprintReadOnly, Category = "Strategic")
    float StrategicValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Strategic")
    float ResourceValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Strategic")
    float ControlValue = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category = "Strategic")
    ERTSTacticalZone TacticalZone = ERTSTacticalZone::Unclassified;

    // === REGION / BIOME (Internal runtime — NO Blueprint reflection) ===
    int32 RegionID = INDEX_NONE;
    int32 BiomeID = INDEX_NONE;
    int32 ChunkID = INDEX_NONE;

    // === NAVIGATION (Internal — computed dynamically) ===
    // REMOVED: TArray<int32> NeighborIndices;
    // Use FRTSGrid::GetNeighbors(Index, ...) instead to avoid per-cell allocation.

    FRTSCell() = default;
};
