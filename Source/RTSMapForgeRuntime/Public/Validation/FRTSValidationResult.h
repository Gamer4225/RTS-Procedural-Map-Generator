#pragma once

#include "CoreMinimal.h"
#include "FRTSValidationResult.generated.h"

UENUM(BlueprintType)
enum class ERTSValidationSeverity : uint8
{
    Pass     UMETA(DisplayName = "Pass"),
    Warning  UMETA(DisplayName = "Warning"),
    Critical UMETA(DisplayName = "Critical")
};

USTRUCT(BlueprintType)
struct RTSMAPFORGERUNTIME_API FRTSValidationIssue
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FString PassName;

    UPROPERTY(BlueprintReadOnly)
    FString Reason;

    UPROPERTY(BlueprintReadOnly)
    ERTSValidationSeverity Severity = ERTSValidationSeverity::Pass;
};

USTRUCT(BlueprintType)
struct RTSMAPFORGERUNTIME_API FRTSValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bPassed = false;

    UPROPERTY(BlueprintReadOnly)
    float OverallScore = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    TArray<FRTSValidationIssue> Issues;

    UPROPERTY(BlueprintReadOnly)
    int32 RetryCount = 0;

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Validation")
    bool HasCriticalFailure() const
    {
        for (const auto& Issue : Issues)
        {
            if (Issue.Severity == ERTSValidationSeverity::Critical)
            {
                return true;
            }
        }
        return false;
    }
};
