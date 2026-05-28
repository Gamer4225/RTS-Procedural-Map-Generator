#pragma once
#include "CoreMinimal.h"
struct RTSMAPFORGERUNTIME_API FRTSStageProfile { FString StageName; double DurationMs=0; int64 MemoryDeltaBytes=0,PeakMemoryBytes=0; };
class RTSMAPFORGERUNTIME_API FRTSPerformanceProfiler
{
public:
    void BeginStage(const FString& StageName); void EndStage();
    void BeginPipeline(int32 W, int32 H); void EndPipeline();
    bool IsWithinBenchmark() const; FString GenerateReport() const;
    const TArray<FRTSStageProfile>& GetStageProfiles() const { return StageProfiles; }
    double GetTotalDurationMs() const { return TotalDurationMs; }
private:
    TArray<FRTSStageProfile> StageProfiles; int32 CurrentStageIndex=INDEX_NONE;
    double PipelineStartTime=0,TotalDurationMs=0; int32 PipelineGridWidth=0,PipelineGridHeight=0;
    double GetCurrentTimeMs() const; int64 GetCurrentMemoryBytes() const; double GetBenchmarkTarget(int32 W,int32 H) const;
};
class FRTSProfileScope { public: FRTSProfileScope(FRTSPerformanceProfiler& P,const FString& S):Profiler(P){P.BeginStage(S);} ~FRTSProfileScope(){Profiler.EndStage();} private: FRTSPerformanceProfiler& Profiler; };
