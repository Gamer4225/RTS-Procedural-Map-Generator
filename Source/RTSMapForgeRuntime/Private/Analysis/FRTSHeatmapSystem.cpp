#include "Analysis/FRTSHeatmapSystem.h"
#include "Math/UnrealMathUtility.h"

void FRTSHeatmapSystem::GenerateAll(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        FRTSCell& Cell = Grid.Cells[i];
        Cell.VisibilityScore = FMath::Abs(Cell.ControlValue);
        Cell.ExposureScore   = 1.0f - FMath::Abs(Cell.ControlValue);
    }
}
