#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"

/**
 * V1 Memory Profiler: Tracks actual memory consumption across all subsystems.
 * 
 * CRITICAL for scaling to 512×512 (262k cells) and 1024×1024 (1M+ cells).
 * 
 * Memory targets:
 *   128×128  : ~  2 MB grid +  1 MB overlays =   3 MB total
 *   256×256  : ~  8 MB grid +  4 MB overlays =  12 MB total
 *   512×512  : ~ 32 MB grid + 16 MB overlays =  48 MB total
 *   1024×1024: ~128 MB grid + 64 MB overlays = 192 MB total
 * 
 * Usage:
 *   FRTSMemoryProfiler Profiler;
 *   Profiler.Snapshot(TEXT("PostGeneration"), Grid);
 *   FString Report = Profiler.GenerateReport();
 */
struct RTSMAPFORGERUNTIME_API FRTSSubsystemMemory
{
    FString Name;
    SIZE_T Bytes = 0;
    int32 ElementCount = 0;
    SIZE_T BytesPerElement = 0;
};

class RTSMAPFORGERUNTIME_API FRTSMemoryProfiler
{
public:
    void Snapshot(const FString& Label, const FRTSGrid& Grid);
    void RecordSubsystem(const FString& Name, SIZE_T Bytes, int32 ElementCount, SIZE_T BytesPerElement);
    
    FString GenerateReport() const;
    bool IsWithinTarget(int32 GridWidth, int32 GridHeight) const;
    
    SIZE_T GetTotalMemory() const;
    SIZE_T GetGridMemory() const { return GridMemory; }
    SIZE_T GetOverlayMemory() const { return OverlayMemory; }
    SIZE_T GetValidationMemory() const { return ValidationMemory; }

private:
    FString LastLabel;
    SIZE_T GridMemory = 0;
    SIZE_T OverlayMemory = 0;      // Debug renderer bitmaps, PDI buffers
    SIZE_T ValidationMemory = 0;   // A* open sets, flood fill queues
    SIZE_T InfluenceMemory = 0;    // Per-cell control values (already in grid)
    SIZE_T HeatmapMemory = 0;      // Any separate float arrays
    TArray<FRTSSubsystemMemory> Subsystems;
    
    SIZE_T GetTargetMemory(int32 CellCount) const;
};
