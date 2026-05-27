#pragma once

#include "CoreMinimal.h"
#include "FRTSSeedManager.generated.h"

/**
 * Deterministic seed manager. ALL randomness must flow through here.
 * Guarantees: same Seed + same Settings = identical map.
 */
UCLASS(BlueprintType)
class RTSMAPFORGERUNTIME_API UFRTSSeedManager : public UObject
{
    GENERATED_BODY()

public:
    void Initialize(int64 InSeed);

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Seed")
    void SetSeed(int64 InSeed);

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Seed")
    int64 GetCurrentSeed() const { return CurrentSeed; }

    // Deterministic random access
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Seed")
    int32 RandRange(int32 Min, int32 Max);

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Seed")
    float RandFloat(); // 0..1

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Seed")
    FVector2D RandPointInCircle(float Radius);

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Seed")
    void ShuffleArray(TArray<int32>& Array);

    // Re-seed from current to replay identical sequence
    void ResetStream();

private:
    UPROPERTY()
    int64 CurrentSeed = 0;

    FRandomStream Stream;
};
