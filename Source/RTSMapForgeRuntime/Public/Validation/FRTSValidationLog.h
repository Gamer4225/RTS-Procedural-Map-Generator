#pragma once

#include "CoreMinimal.h"
#include "Validation/FRTSValidationResult.h"

/**
 * V1 Validation Failure Classification Log
 * 
 * Prevents validation retry spirals by classifying WHY maps fail,
 * enabling targeted retry strategies instead of blind seed mutation.
 * 
 * Failure Classes:
 *   FC_TraversalBlocked    → Retry with lower water level, wider rivers, fewer chokes
 *   FC_BaseUnreachable     → Retry with different base placement algorithm or smaller map
 *   FC_ExpansionIsolated   → Retry with more generous expansion placement radius
 *   FC_NoChokes           → Retry with different terrain roughness or mountain level
 *   FC_TooManyChokes      → Retry with smoother terrain, fewer rivers
 *   FC_SpawnAreaTooSmall  → Retry with flatter terrain, larger buildable threshold
 *   FC_ResourceImbalance  → Retry with symmetric resource placement
 *   FC_RushTooShort       → Retry with larger map or forced minimum distance
 *   FC_RushTooLong        → Retry with smaller map or more open terrain
 *   FC_OverallScoreLow    → Retry with adjusted scoring weights or different seed
 *   FC_Unknown            → Generic seed mutation (last resort)
 * 
 * Logging format enables customer support to diagnose generation failures
 * from log output without reproducing locally.
 */
UENUM()
enum class ERTSFailureClass : uint8
{
    None               UMETA(DisplayName = "None"),
    TraversalBlocked   UMETA(DisplayName = "Traversal Blocked"),
    BaseUnreachable    UMETA(DisplayName = "Base Unreachable"),
    ExpansionIsolated  UMETA(DisplayName = "Expansion Isolated"),
    NoChokes           UMETA(DisplayName = "No Choke Points"),
    TooManyChokes      UMETA(DisplayName = "Too Many Choke Points"),
    SpawnAreaTooSmall  UMETA(DisplayName = "Spawn Area Too Small"),
    ResourceImbalance  UMETA(DisplayName = "Resource Imbalance"),
    RushTooShort       UMETA(DisplayName = "Rush Distance Too Short"),
    RushTooLong        UMETA(DisplayName = "Rush Distance Too Long"),
    OverallScoreLow    UMETA(DisplayName = "Overall Score Too Low"),
    Unknown            UMETA(DisplayName = "Unknown"),
};

struct RTSMAPFORGERUNTIME_API FRTSFailureLogEntry
{
    ERTSFailureClass FailureClass = ERTSFailureClass::None;
    FString PassName;
    FString Reason;
    ERTSValidationSeverity Severity = ERTSValidationSeverity::Pass;
    int32 RetryAttempt = 0;
    float ScoreAtFailure = 0.0f;
    
    FString ToLogString() const;
    static ERTSFailureClass Classify(const FRTSValidationIssue& Issue);
};

class RTSMAPFORGERUNTIME_API FRTSValidationLog
{
public:
    void LogFailure(const FRTSValidationIssue& Issue, int32 RetryAttempt, float ScoreAtFailure);
    void LogSuccess(int32 RetryAttempt, float FinalScore);
    
    // Returns the dominant failure class across all retries
    ERTSFailureClass GetDominantFailureClass() const;
    
    // Returns recommended parameter adjustments for next retry
    FString GetRetryRecommendation() const;
    
    // Full log for customer support / debugging
    FString GenerateFullLog() const;
    
    bool HasCriticalFailures() const;
    int32 GetRetryCount() const { return Entries.Num(); }
    
    void Clear() { Entries.Empty(); }
    
private:
    TArray<FRTSFailureLogEntry> Entries;
    
    static FString FailureClassToString(ERTSFailureClass Class);
};
