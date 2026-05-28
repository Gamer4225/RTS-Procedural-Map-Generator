#include "Core/FRTSMemoryProfiler.h"
#include "HAL/PlatformMemory.h"

void FRTSMemoryProfiler::Snapshot(const FString& Label, const FRTSGrid& Grid)
{
    LastLabel = Label;
    Subsystems.Empty();
    GridMemory = Grid.GetAllocatedSize();
    RecordSubsystem(TEXT("FRTSGrid.Cells"), GridMemory, Grid.Cells.Num(), sizeof(FRTSCell));
    int32 CellCount = Grid.Width * Grid.Height;
    OverlayMemory = static_cast<SIZE_T>(CellCount) * sizeof(FColor) * 2;
    RecordSubsystem(TEXT("OverlayBitmaps"), OverlayMemory, CellCount * 2, sizeof(FColor));
    SIZE_T AStarNodeSize = sizeof(int32) + sizeof(float) * 2 + sizeof(int32);
    ValidationMemory = static_cast<SIZE_T>(CellCount) * AStarNodeSize * 2;
    RecordSubsystem(TEXT("AStarWorstCase"), ValidationMemory, CellCount * 2, static_cast<SIZE_T>(AStarNodeSize));
    HeatmapMemory   = 0;
    InfluenceMemory = 0;
}

void FRTSMemoryProfiler::RecordSubsystem(const FString& Name, SIZE_T Bytes, int32 ElementCount, SIZE_T BytesPerElement)
{
    FRTSSubsystemMemory Sub; Sub.Name = Name; Sub.Bytes = Bytes;
    Sub.ElementCount = ElementCount; Sub.BytesPerElement = static_cast<SIZE_T>(BytesPerElement);
    Subsystems.Add(Sub);
}

FString FRTSMemoryProfiler::GenerateReport() const
{
    FString R;
    R += FString::Printf(TEXT("=== RTSMapForge Memory [%s] ===\nTotal: %.2f MB\n"), *LastLabel, GetTotalMemory() / (1024.0 * 1024.0));
    for (const auto& S : Subsystems)
        R += FString::Printf(TEXT("  %-24s: %.2f KB (%d elems)\n"), *S.Name, S.Bytes / 1024.0, S.ElementCount);
    return R;
}

bool FRTSMemoryProfiler::IsWithinTarget(int32 W, int32 H) const { return GetTotalMemory() <= GetTargetMemory(W * H); }
SIZE_T FRTSMemoryProfiler::GetTotalMemory() const { return GridMemory + OverlayMemory + ValidationMemory + HeatmapMemory + InfluenceMemory; }
SIZE_T FRTSMemoryProfiler::GetTargetMemory(int32 CellCount) const { return static_cast<SIZE_T>(CellCount) * 192; }
