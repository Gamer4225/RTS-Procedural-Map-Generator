#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"

/**
 * Debug overlay rendering modes for the editor viewport.
 * 
 * DESIGN: Stateless — consumes FRTSGrid, produces debug primitives.
 * This keeps rendering entirely separate from simulation data.
 */
UENUM(BlueprintType)
enum class ERTSDebugOverlayMode : uint8
{
    None           UMETA(DisplayName = "None"),
    Heightmap      UMETA(DisplayName = "Heightmap"),
    WaterCliff     UMETA(DisplayName = "Water & Cliffs"),
    Walkable       UMETA(DisplayName = "Walkable"),
    Buildable      UMETA(DisplayName = "Buildable"),
    Slope          UMETA(DisplayName = "Slope"),
    Regions        UMETA(DisplayName = "Regions"),
    Biomes         UMETA(DisplayName = "Biomes"),
    TacticalZones  UMETA(DisplayName = "Tactical Zones"),
    Influence      UMETA(DisplayName = "Influence Map"),
    CombatHeat     UMETA(DisplayName = "Combat Heat"),
    BasePlacement  UMETA(DisplayName = "Base & Expansion"),
    ChokePoints    UMETA(DisplayName = "Choke Points"),
};

class RTSMAPFORGERUNTIME_API FRTSDebugRenderer
{
public:
    // Draws the grid as colored quads at world positions.
    // This is designed to be called from an EditorViewportClient Draw() override
    // or from a custom SceneProxy tick.
    void RenderOverlay(
        const FRTSGrid& Grid,
        ERTSDebugOverlayMode Mode,
        FPrimitiveDrawInterface* PDI,
        const FMatrix& LocalToWorld,
        float ZOffset = 10.0f // Slight lift above terrain
    ) const;

    // Generates a flat TArray<FColor> bitmap for use with UTexture2D or Slate ImageBrush.
    // Width x Height RGBA8 image. Caller owns the array.
    void GenerateMinimapBitmap(
        const FRTSGrid& Grid,
        ERTSDebugOverlayMode Mode,
        TArray<FColor>& OutBitmap,
        int32& OutWidth,
        int32& OutHeight
    ) const;

private:
    int32 GetVisualStride(const FRTSGrid& Grid) const;

    FColor GetHeightmapColor(float Height) const;
    FColor GetWaterCliffColor(bool bWater, bool bCliff, bool bWalkable) const;
    FColor GetWalkableColor(bool bWalkable) const;
    FColor GetBuildableColor(bool bBuildable) const;
    FColor GetSlopeColor(float SlopeRadians) const;
    FColor GetRegionColor(int32 RegionID) const;
    FColor GetBiomeColor(int32 BiomeID) const;
    FColor GetTacticalZoneColor(ERTSTacticalZone Zone) const;
    FColor GetInfluenceColor(float ControlValue) const;
    FColor GetCombatHeatColor(float ExposureScore) const;
    FColor GetBasePlacementColor(const FRTSGrid& Grid, int32 X, int32 Y, const TArray<FIntPoint>& Bases, const TArray<FIntPoint>& Expansions) const;
    FColor GetChokeColor(bool bIsChoke) const;
};
