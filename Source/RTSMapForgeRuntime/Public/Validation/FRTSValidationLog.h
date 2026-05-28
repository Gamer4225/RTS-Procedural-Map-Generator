#pragma once
#include "CoreMinimal.h"
#include "Validation/FRTSValidationResult.h"
#include "FRTSValidationLog.generated.h"
UENUM()
enum class ERTSFailureClass : uint8 { None, TraversalBlocked, BaseUnreachable, ExpansionIsolated, NoChokes, TooManyChokes, SpawnAreaTooSmall, ResourceImbalance, RushTooShort, RushTooLong, OverallScoreLow, Unknown };
struct RTSMAPFORGERUNTIME_API FRTSFailureLogEntry
{
    ERTSFailureClass FailureClass=ERTSFailureClass::None; FString PassName,Reason;
    ERTSValidationSeverity Severity=ERTSValidationSeverity::Pass; int32 RetryAttempt=0; float ScoreAtFailure=0;
    FString ToLogString() const; static ERTSFailureClass Classify(const FRTSValidationIssue& Issue);
};
class RTSMAPFORGERUNTIME_API FRTSValidationLog
{
public:
    void LogFailure(const FRTSValidationIssue& Issue, int32 RetryAttempt, float ScoreAtFailure);
    void LogSuccess(int32 RetryAttempt, float FinalScore);
    ERTSFailureClass GetDominantFailureClass() const;
    FString GetRetryRecommendation() const;
    FString GenerateFullLog() const;
    bool HasCriticalFailures() const;
    int32 GetRetryCount() const { return Entries.Num(); }
    void Clear() { Entries.Empty(); }
    static FString FailureClassToString(ERTSFailureClass Class);
private:
    TArray<FRTSFailureLogEntry> Entries;
};
