#include "Core/FRTSPerformanceProfiler.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"

void FRTSPerformanceProfiler::BeginPipeline(int32 GridWidth, int32 GridHeight)
{
    PipelineGridWidth = GridWidth;
    PipelineGridHeight = GridHeight;
    PipelineStartTime = GetCurrentTimeMs();
    StageProfiles.Empty();
    TotalDurationMs = 0.0;
}

void FRTSPerformanceProfiler::EndPipeline()
{
    TotalDurationMs = GetCurrentTimeMs() - PipelineStartTime;
}

void FRTSPerformanceProfiler::BeginStage(const FString& StageName)
{
    FRTSStageProfile Profile;
    Profile.StageName = StageName;
    Profile.PeakMemoryBytes = GetCurrentMemoryBytes();
    
    CurrentStageIndex = StageProfiles.Add(Profile);
    StageProfiles[CurrentStageIndex].DurationMs = GetCurrentTimeMs(); // Store start time temporarily
}

void FRTSPerformanceProfiler::EndStage()
{
    if (!StageProfiles.IsValidIndex(CurrentStageIndex))
    {
        return;
    }
    
    double EndTime = GetCurrentTimeMs();
    double StartTime = StageProfiles[CurrentStageIndex].DurationMs; // Was stored as start
    StageProfiles[CurrentStageIndex].DurationMs = EndTime - StartTime;
    StageProfiles[CurrentStageIndex].MemoryDeltaBytes = GetCurrentMemoryBytes() - StageProfiles[CurrentStageIndex].PeakMemoryBytes;
    CurrentStageIndex = INDEX_NONE;
}

bool FRTSPerformanceProfiler::IsWithinBenchmark() const
{
    double Target = GetBenchmarkTarget(PipelineGridWidth, PipelineGridHeight);
    return TotalDurationMs <= Target;
}

FString FRTSPerformanceProfiler::GenerateReport() const
{
    FString Report;
    Report += FString::Printf(TEXT("=== RTS MapForge Performance Report ===\n"));
    Report += FString::Printf(TEXT("Grid: %dx%d | Total: %.3f ms | Target: %.3f ms\n"),
        PipelineGridWidth, PipelineGridHeight, TotalDurationMs,
        GetBenchmarkTarget(PipelineGridWidth, PipelineGridHeight));
    
    if (IsWithinBenchmark())
    {
        Report += TEXT("STATUS: WITHIN BENCHMARK ✅\n");
    }
    else
    {
        Report += TEXT("STATUS: EXCEEDS BENCHMARK ⚠️\n");
    }
    
    Report += TEXT("\nPer-Stage Breakdown:\n");
    
    // Sort by duration descending
    TArray<FRTSStageProfile> Sorted = StageProfiles;
    Sorted.Sort([](const FRTSStageProfile& A, const FRTSStageProfile& B) {
        return A.DurationMs > B.DurationMs;
    });
    
    for (const FRTSStageProfile& Stage : Sorted)
    {
        float Percent = TotalDurationMs > 0.0f ? (Stage.DurationMs / TotalDurationMs * 100.0f) : 0.0f;
        Report += FString::Printf(TEXT("  %-24s : %8.3f ms (%5.1f%%)\n"),
            *Stage.StageName, Stage.DurationMs, Percent);
    }
    
    Report += TEXT("=====================================\n");
    return Report;
}

double FRTSPerformanceProfiler::GetCurrentTimeMs() const
{
    return FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64());
}

int64 FRTSPerformanceProfiler::GetCurrentMemoryBytes() const
{
#if ENABLE_MEMORY_PROFILER
    return static_cast<int64>(FPlatformMemory::GetBytesAllocated());
#else
    return 0;
#endif
}

double FRTSPerformanceProfiler::GetBenchmarkTarget(int32 Width, int32 Height) const
{
    int32 CellCount = Width * Height;
    
    if (CellCount <= 128 * 128)
    {
        return 500.0; // < 0.5 sec
    }
    else if (CellCount <= 256 * 256)
    {
        return 2000.0; // < 2.0 sec
    }
    else if (CellCount <= 512 * 512)
    {
        return 8000.0; // < 8.0 sec
    }
    else if (CellCount <= 1024 * 1024)
    {
        return 25000.0; // < 25 sec
    }
    else
    {
        return 60000.0; // < 60 sec for very large maps
    }
}
