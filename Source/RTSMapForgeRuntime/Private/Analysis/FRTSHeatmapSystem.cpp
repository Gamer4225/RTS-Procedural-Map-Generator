#include "Analysis/FRTSHeatmapSystem.h"
#include "Math/UnrealMathUtility.h"

void FRTSHeatmapSystem::GenerateAll(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    // V1: Populate cell-level heat values directly.
    // Future: export float arrays for GPU overlay rendering.

    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        FRTSCell& Cell = Grid.Cells[i];

        // Combat heat: high where control values are near zero (contested)
        Cell.VisibilityScore = FMath::Abs(Cell.ControlValue); // who dominates
        // Using ExposureScore as combat heat proxy for V1: low = contested, high = safe for one player
        Cell.ExposureScore = 1.0f - FMath::Abs(Cell.ControlValue);
    }

    // Traversal heat: mark A* paths from base to base by walking influence
    // V1 stub: traversal is implicitly covered by pathfinding stage.
}
