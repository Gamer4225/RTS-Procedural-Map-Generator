#include "Core/FRTSMemoryProfiler.h"
#include "HAL/PlatformMemory.h"

void FRTSMemoryProfiler::Snapshot(const FString& Label, const FRTSGrid& Grid)
{
    LastLabel = Label;
    Subsystems.Empty();
    
    // Grid memory: cells + any internal arrays
    GridMemory = Grid.GetAllocatedSize();
    RecordSubsystem(TEXT("FRTSGrid.Cells"), GridMemory, Grid.Cells.Num(), sizeof(FRTSCell));
    
    // Overlay memory: minimap bitmaps (RGBA8 = 4 bytes per cell)
    // Worst case: one active preview texture + one viewport overlay buffer
    int32 CellCount = Grid.Width * Grid.Height;
    SIZE_T BitmapSize = static_cast<SIZE_T>(CellCount) * sizeof(FColor) * 2; // preview + viewport
    OverlayMemory = BitmapSize;
    RecordSubsystem(TEXT("OverlayBitmaps"), OverlayMemory, CellCount * 2, sizeof(FColor));
    
    // Validation memory: A* open/closed sets worst case
    // A* can hold up to all cells in open set + closed set
    SIZE_T AStarNodeSize = sizeof(int32) + sizeof(float) * 2 + sizeof(int32); // Index + G + F + Parent
    SIZE_T MaxAStarOpen = static_cast<SIZE_T>(CellCount) * AStarNodeSize * 2; // open + closed
    ValidationMemory = MaxAStarOpen;
    RecordSubsystem(TEXT("AStarWorstCase"), ValidationMemory, CellCount * 2, static_cast<int32>(AStarNodeSize));
    
    // Region detection: visited array + stack (worst case all cells)
    SIZE_T FloodFillSize = static_cast<SIZE_T>(CellCount) * (sizeof(bool) + sizeof(int32));
    ValidationMemory += FloodFillSize;
    RecordSubsystem(TEXT("FloodFillWorstCase"), FloodFillSize, CellCount, sizeof(bool) + sizeof(int32));
    
    // Heatmap memory: any separate float arrays (combat, traversal, etc.)
    // Currently heat values live inside FRTSCell, so no extra array
    // But if we add GPU arrays later, account for them
    HeatmapMemory = 0;
    
    // Influence is inside FRTSCell.ControlValue, no extra
    InfluenceMemory = 0;
}

void FRTSMemoryProfiler::RecordSubsystem(const FString& Name, SIZE_T Bytes, int32 ElementCount, SIZE_T BytesPerElement)
{
    FRTSSubsystemMemory Sub;
    Sub.Name = Name;
    Sub.Bytes = Bytes;
    Sub.ElementCount = ElementCount;
    Sub.BytesPerElement = static_cast<SIZE_T>(BytesPerElement);
    Subsystems.Add(Sub);
}

FString FRTSMemoryProfiler::GenerateReport() const
{
    FString Report;
    Report += FString::Printf(TEXT("=== RTS MapForge Memory Report [%s] ===\n"), *LastLabel);
    
    SIZE_T Total = GetTotalMemory();
    Report += FString::Printf(TEXT("Total: %.2f MB\n"), Total / (1024.0 * 1024.0));
    Report += FString::Printf(TEXT("  Grid:      %.2f MB\n"), GridMemory / (1024.0 * 1024.0));
    Report += FString::Printf(TEXT("  Overlay:   %.2f MB\n"), OverlayMemory / (1024.0 * 1024.0));
    Report += FString::Printf(TEXT("  Validate:  %.2f MB\n"), ValidationMemory / (1024.0 * 1024.0));
    Report += FString::Printf(TEXT("  Heatmap:   %.2f MB\n"), HeatmapMemory / (1024.0 * 1024.0));
    Report += TEXT("\nPer-Subsystem:\n");
    
    for (const auto& Sub : Subsystems)
    {
        float MB = Sub.Bytes / (1024.0 * 1024.0);
        float KB = Sub.Bytes / 1024.0;
        if (MB >= 1.0f)
        {
            Report += FString::Printf(TEXT("  %-24s: %8.2f MB (%d elements, %llu bytes/elem)\n"),
                *Sub.Name, MB, Sub.ElementCount, static_cast<uint64>(Sub.BytesPerElement));
        }
        else
        {
            Report += FString::Printf(TEXT("  %-24s: %8.2f KB (%d elements, %llu bytes/elem)\n"),
                *Sub.Name, KB, Sub.ElementCount, static_cast<uint64>(Sub.BytesPerElement));
        }
    }
    
    Report += TEXT("================================\n");
    return Report;
}

bool FRTSMemoryProfiler::IsWithinTarget(int32 GridWidth, int32 GridHeight) const
{
    int32 CellCount = GridWidth * GridHeight;
    SIZE_T Target = GetTargetMemory(CellCount);
    return GetTotalMemory() <= Target;
}

SIZE_T FRTSMemoryProfiler::GetTotalMemory() const
{
    return GridMemory + OverlayMemory + ValidationMemory + HeatmapMemory + InfluenceMemory;
}

SIZE_T FRTSMemoryProfiler::GetTargetMemory(int32 CellCount) const
{
    // Target memory scales with cell count:
    // 128² = 16k cells  → ~3 MB  = 192 bytes/cell
    // 256² = 65k cells  → ~12 MB = 192 bytes/cell
    // 512² = 262k cells → ~48 MB = 192 bytes/cell
    // 1024² = 1M cells  → ~192 MB = 192 bytes/cell
    // 
    // This accounts for: grid cells (~64 bytes) + overlays + validation buffers
    return static_cast<SIZE_T>(CellCount) * 192;
}
