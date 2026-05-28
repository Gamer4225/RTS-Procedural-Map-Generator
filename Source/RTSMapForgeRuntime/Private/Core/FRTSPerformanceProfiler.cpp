#include "Core/FRTSPerformanceProfiler.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMemory.h"

void FRTSPerformanceProfiler::BeginPipeline(int32 W, int32 H) { PipelineGridWidth=W; PipelineGridHeight=H; PipelineStartTime=GetCurrentTimeMs(); StageProfiles.Empty(); TotalDurationMs=0.0; }
void FRTSPerformanceProfiler::EndPipeline() { TotalDurationMs = GetCurrentTimeMs() - PipelineStartTime; }

void FRTSPerformanceProfiler::BeginStage(const FString& StageName)
{
    FRTSStageProfile P; P.StageName=StageName; P.PeakMemoryBytes=GetCurrentMemoryBytes();
    CurrentStageIndex = StageProfiles.Add(P);
    StageProfiles[CurrentStageIndex].DurationMs = GetCurrentTimeMs();
}

void FRTSPerformanceProfiler::EndStage()
{
    if (!StageProfiles.IsValidIndex(CurrentStageIndex)) return;
    double End = GetCurrentTimeMs();
    StageProfiles[CurrentStageIndex].DurationMs = End - StageProfiles[CurrentStageIndex].DurationMs;
    StageProfiles[CurrentStageIndex].MemoryDeltaBytes = GetCurrentMemoryBytes() - StageProfiles[CurrentStageIndex].PeakMemoryBytes;
    CurrentStageIndex = INDEX_NONE;
}

bool FRTSPerformanceProfiler::IsWithinBenchmark() const { return TotalDurationMs <= GetBenchmarkTarget(PipelineGridWidth, PipelineGridHeight); }

FString FRTSPerformanceProfiler::GenerateReport() const
{
    FString R = FString::Printf(TEXT("=== RTSMapForge Perf: %dx%d | %.3f ms / %.3f ms target ===\n"),
        PipelineGridWidth, PipelineGridHeight, TotalDurationMs, GetBenchmarkTarget(PipelineGridWidth,PipelineGridHeight));
    for (const auto& S : StageProfiles)
        R += FString::Printf(TEXT("  %-28s: %8.3f ms\n"), *S.StageName, S.DurationMs);
    return R;
}

double FRTSPerformanceProfiler::GetCurrentTimeMs() const { return FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64()); }
int64  FRTSPerformanceProfiler::GetCurrentMemoryBytes() const { return 0; }

double FRTSPerformanceProfiler::GetBenchmarkTarget(int32 W, int32 H) const
{
    int32 C = W*H;
    if (C <= 128*128)  return 500.0;
    if (C <= 256*256)  return 2000.0;
    if (C <= 512*512)  return 8000.0;
    if (C <= 1024*1024) return 25000.0;
    return 60000.0;
}
