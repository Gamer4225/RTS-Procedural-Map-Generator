#pragma once

#include "CoreMinimal.h"
#include "Misc/DateTime.h"

/**
 * V1 Performance Profiler: Tracks per-stage timing and memory.
 * 
 * TARGET BENCHMARKS:
 *   128×128  : < 0.5 sec
 *   256×256  : < 2.0 sec  
 *   512×512  : < 8.0 sec
 *   1024×1024: < 25 sec
 * 
 * Usage:
 *   FRTSPerformanceProfiler Profiler;
 *   {
 *       FRTSProfileScope Scope(Profiler, TEXT("Stage3_Heightmap"));
 *       // ... generation code ...
 *   }
 *   FString Report = Profiler.GenerateReport();
 */
struct RTSMAPFORGERUNTIME_API FRTSStageProfile
{
    FString StageName;
    double DurationMs = 0.0;
    int64 MemoryDeltaBytes = 0;
    int64 PeakMemoryBytes = 0;
};

class RTSMAPFORGERUNTIME_API FRTSPerformanceProfiler
{
public:
    void BeginStage(const FString& StageName);
    void EndStage();
    
    void BeginPipeline(int32 GridWidth, int32 GridHeight);
    void EndPipeline();
    
    // Validates against benchmark targets
    bool IsWithinBenchmark() const;
    FString GenerateReport() const;
    
    // Access raw data
    const TArray<FRTSStageProfile>& GetStageProfiles() const { return StageProfiles; }
    double GetTotalDurationMs() const { return TotalDurationMs; }

private:
    TArray<FRTSStageProfile> StageProfiles;
    int32 CurrentStageIndex = INDEX_NONE;
    
    double PipelineStartTime = 0.0;
    double TotalDurationMs = 0.0;
    int32 PipelineGridWidth = 0;
    int32 PipelineGridHeight = 0;
    
    double GetCurrentTimeMs() const;
    int64 GetCurrentMemoryBytes() const;
    
    double GetBenchmarkTarget(int32 Width, int32 Height) const;
};

// RAII scope timer
class FRTSProfileScope
{
public:
    FRTSProfileScope(FRTSPerformanceProfiler& InProfiler, const FString& StageName)
        : Profiler(InProfiler)
    {
        Profiler.BeginStage(StageName);
    }
    
    ~FRTSProfileScope()
    {
        Profiler.EndStage();
    }
    
private:
    FRTSPerformanceProfiler& Profiler;
};
