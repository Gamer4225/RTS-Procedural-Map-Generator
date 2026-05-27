#include "Visualization/FRTSDebugRenderer.h"
#include "Math/UnrealMathUtility.h"
#include "Data/FRTSMapMetadata.h"

// === Minimap Bitmap Generation ===
void FRTSDebugRenderer::GenerateMinimapBitmap(
    const FRTSGrid& Grid,
    ERTSDebugOverlayMode Mode,
    TArray<FColor>& OutBitmap,
    int32& OutWidth,
    int32& OutHeight
) const
{
    const int32 Stride = GetVisualStride(Grid);
    OutWidth = FMath::Max(1, FMath::DivideAndRoundUp(Grid.Width, Stride));
    OutHeight = FMath::Max(1, FMath::DivideAndRoundUp(Grid.Height, Stride));
    OutBitmap.SetNumUninitialized(OutWidth * OutHeight);

    for (int32 Y = 0; Y < OutHeight; ++Y)
    {
        const int32 SourceY = FMath::Min(Y * Stride, Grid.Height - 1);
        for (int32 X = 0; X < OutWidth; ++X)
        {
            const int32 SourceX = FMath::Min(X * Stride, Grid.Width - 1);
            const FRTSCell& Cell = Grid.GetCell(SourceX, SourceY);
            FColor Color = FColor::Black;

            switch (Mode)
            {
                case ERTSDebugOverlayMode::Heightmap:
                    Color = GetHeightmapColor(Cell.Height);
                    break;
                case ERTSDebugOverlayMode::WaterCliff:
                    Color = GetWaterCliffColor(Cell.bWater, Cell.bCliff, Cell.bWalkable);
                    break;
                case ERTSDebugOverlayMode::Walkable:
                    Color = GetWalkableColor(Cell.bWalkable);
                    break;
                case ERTSDebugOverlayMode::Buildable:
                    Color = GetBuildableColor(Cell.bBuildable);
                    break;
                case ERTSDebugOverlayMode::Slope:
                    Color = GetSlopeColor(Cell.Slope);
                    break;
                case ERTSDebugOverlayMode::Regions:
                    Color = GetRegionColor(Cell.RegionID);
                    break;
                case ERTSDebugOverlayMode::Biomes:
                    Color = GetBiomeColor(Cell.BiomeID);
                    break;
                case ERTSDebugOverlayMode::TacticalZones:
                    Color = GetTacticalZoneColor(Cell.TacticalZone);
                    break;
                case ERTSDebugOverlayMode::Influence:
                    Color = GetInfluenceColor(Cell.ControlValue);
                    break;
                case ERTSDebugOverlayMode::CombatHeat:
                    Color = GetCombatHeatColor(Cell.ExposureScore);
                    break;
                case ERTSDebugOverlayMode::ChokePoints:
                    Color = GetChokeColor(Cell.TacticalZone == ERTSTacticalZone::ChokePoint);
                    break;
                default:
                    Color = FColor::Black;
                    break;
            }

            OutBitmap[Y * OutWidth + X] = Color;
        }
    }
}

// === PDI Rendering (Editor Viewport) ===
void FRTSDebugRenderer::RenderOverlay(
    const FRTSGrid& Grid,
    ERTSDebugOverlayMode Mode,
    FPrimitiveDrawInterface* PDI,
    const FMatrix& LocalToWorld,
    float ZOffset
) const
{
    if (!PDI || Mode == ERTSDebugOverlayMode::None)
    {
        return;
    }

    const int32 Stride = GetVisualStride(Grid);
    const float CellSize = Grid.CellSize;
    const FVector ZLift(0.0f, 0.0f, ZOffset);

    for (int32 Y = 0; Y < Grid.Height; Y += Stride)
    {
        for (int32 X = 0; X < Grid.Width; X += Stride)
        {
            const FRTSCell& Cell = Grid.GetCell(X, Y);
            FLinearColor LinearColor = FLinearColor::Black;

            switch (Mode)
            {
                case ERTSDebugOverlayMode::Heightmap:
                    LinearColor = FLinearColor(GetHeightmapColor(Cell.Height));
                    break;
                case ERTSDebugOverlayMode::WaterCliff:
                    LinearColor = FLinearColor(GetWaterCliffColor(Cell.bWater, Cell.bCliff, Cell.bWalkable));
                    break;
                case ERTSDebugOverlayMode::Walkable:
                    LinearColor = FLinearColor(GetWalkableColor(Cell.bWalkable));
                    break;
                case ERTSDebugOverlayMode::Buildable:
                    LinearColor = FLinearColor(GetBuildableColor(Cell.bBuildable));
                    break;
                case ERTSDebugOverlayMode::Slope:
                    LinearColor = FLinearColor(GetSlopeColor(Cell.Slope));
                    break;
                case ERTSDebugOverlayMode::Regions:
                    LinearColor = FLinearColor(GetRegionColor(Cell.RegionID));
                    break;
                case ERTSDebugOverlayMode::Biomes:
                    LinearColor = FLinearColor(GetBiomeColor(Cell.BiomeID));
                    break;
                case ERTSDebugOverlayMode::TacticalZones:
                    LinearColor = FLinearColor(GetTacticalZoneColor(Cell.TacticalZone));
                    break;
                case ERTSDebugOverlayMode::Influence:
                    LinearColor = FLinearColor(GetInfluenceColor(Cell.ControlValue));
                    break;
                case ERTSDebugOverlayMode::CombatHeat:
                    LinearColor = FLinearColor(GetCombatHeatColor(Cell.ExposureScore));
                    break;
                case ERTSDebugOverlayMode::ChokePoints:
                    LinearColor = FLinearColor(GetChokeColor(Cell.TacticalZone == ERTSTacticalZone::ChokePoint));
                    break;
                default:
                    break;
            }

            FVector Center = Cell.WorldPosition + ZLift;
            FVector Extent(CellSize * 0.45f * static_cast<float>(Stride), CellSize * 0.45f * static_cast<float>(Stride), 0.0f);
            FColor DrawColor = LinearColor.ToFColor(false);

            PDI->DrawLine(
                Center + FVector(-Extent.X, -Extent.Y, 0.0f),
                Center + FVector(Extent.X, -Extent.Y, 0.0f),
                DrawColor,
                SDPG_Foreground,
                1.0f
            );
            PDI->DrawLine(
                Center + FVector(Extent.X, -Extent.Y, 0.0f),
                Center + FVector(Extent.X, Extent.Y, 0.0f),
                DrawColor,
                SDPG_Foreground,
                1.0f
            );
            PDI->DrawLine(
                Center + FVector(Extent.X, Extent.Y, 0.0f),
                Center + FVector(-Extent.X, Extent.Y, 0.0f),
                DrawColor,
                SDPG_Foreground,
                1.0f
            );
            PDI->DrawLine(
                Center + FVector(-Extent.X, Extent.Y, 0.0f),
                Center + FVector(-Extent.X, -Extent.Y, 0.0f),
                DrawColor,
                SDPG_Foreground,
                1.0f
            );
        }
    }
}

int32 FRTSDebugRenderer::GetVisualStride(const FRTSGrid& Grid) const
{
    const int32 TotalCells = Grid.Width * Grid.Height;
    const int32 MaxVisualCells = 16384;

    if (TotalCells <= MaxVisualCells || TotalCells <= 0)
    {
        return 1;
    }

    const float Ratio = static_cast<float>(TotalCells) / static_cast<float>(MaxVisualCells);
    return FMath::Clamp(FMath::CeilToInt(FMath::Sqrt(Ratio)), 1, FMath::Max(Grid.Width, Grid.Height));
}

// === Color Helpers ===
FColor FRTSDebugRenderer::GetHeightmapColor(float Height) const
{
    uint8 V = FMath::Clamp(static_cast<int32>(Height * 255.0f), 0, 255);
    return FColor(V, V, V, 255);
}

FColor FRTSDebugRenderer::GetWaterCliffColor(bool bWater, bool bCliff, bool bWalkable) const
{
    if (bWater)  return FColor(0, 64, 255, 255);     // Deep blue
    if (bCliff)  return FColor(128, 128, 128, 255);  // Gray rock
    if (bWalkable) return FColor(34, 139, 34, 255);  // Forest green
    return FColor(160, 82, 45, 255);                 // Brown (mud / unbuildable)
}

FColor FRTSDebugRenderer::GetWalkableColor(bool bWalkable) const
{
    return bWalkable ? FColor(0, 255, 0, 255) : FColor(255, 0, 0, 255);
}

FColor FRTSDebugRenderer::GetBuildableColor(bool bBuildable) const
{
    return bBuildable ? FColor(0, 200, 0, 255) : FColor(200, 0, 0, 255);
}

FColor FRTSDebugRenderer::GetSlopeColor(float SlopeRadians) const
{
    float Deg = FMath::RadiansToDegrees(SlopeRadians);
    uint8 V = FMath::Clamp(static_cast<int32>((Deg / 90.0f) * 255.0f), 0, 255);
    return FColor(V, 255 - V, 0, 255);
}

FColor FRTSDebugRenderer::GetRegionColor(int32 RegionID) const
{
    if (RegionID == INDEX_NONE) return FColor::Black;
    int32 R = ((RegionID * 7919) % 256);
    int32 G = ((RegionID * 104729) % 256);
    int32 B = ((RegionID * 1299709) % 256);
    return FColor(R, G, B, 255);
}

FColor FRTSDebugRenderer::GetBiomeColor(int32 BiomeID) const
{
    switch (BiomeID)
    {
        case 0: return FColor::Green;
        case 1: return FColor::Yellow;
        case 2: return FColor::White;
        case 3: return FColor::Orange;
        default: return FColor::Cyan;
    }
}

FColor FRTSDebugRenderer::GetTacticalZoneColor(ERTSTacticalZone Zone) const
{
    switch (Zone)
    {
        case ERTSTacticalZone::MainBase:          return FColor(0, 255, 0, 255);
        case ERTSTacticalZone::NatExpansion:      return FColor(0, 200, 100, 255);
        case ERTSTacticalZone::ContestedExp:      return FColor(255, 165, 0, 255);
        case ERTSTacticalZone::ChokePoint:         return FColor(255, 0, 0, 255);
        case ERTSTacticalZone::RiverCrossing:     return FColor(255, 255, 0, 255);   // NEW V1.5: Yellow for crossings
        case ERTSTacticalZone::OpenBattlefield:    return FColor(200, 200, 200, 255);
        case ERTSTacticalZone::HighGround:         return FColor(139, 69, 19, 255);
        case ERTSTacticalZone::FlankRoute:         return FColor(128, 0, 128, 255);
        case ERTSTacticalZone::VisionControl:      return FColor(0, 255, 255, 255);
        case ERTSTacticalZone::ResourceCluster:    return FColor(255, 215, 0, 255);
        default:                                   return FColor::Black;
    }
}

FColor FRTSDebugRenderer::GetInfluenceColor(float ControlValue) const
{
    if (ControlValue < 0.0f)
    {
        uint8 Intensity = FMath::Clamp(static_cast<int32>(-ControlValue * 255.0f), 0, 255);
        return FColor(Intensity, 0, 0, 255);
    }
    else
    {
        uint8 Intensity = FMath::Clamp(static_cast<int32>(ControlValue * 255.0f), 0, 255);
        return FColor(0, 0, Intensity, 255);
    }
}

FColor FRTSDebugRenderer::GetCombatHeatColor(float ExposureScore) const
{
    uint8 V = FMath::Clamp(static_cast<int32>(ExposureScore * 255.0f), 0, 255);
    return FColor(V, 0, 255 - V, 255);
}

FColor FRTSDebugRenderer::GetChokeColor(bool bIsChoke) const
{
    return bIsChoke ? FColor(255, 0, 0, 255) : FColor(0, 0, 0, 0);
}
