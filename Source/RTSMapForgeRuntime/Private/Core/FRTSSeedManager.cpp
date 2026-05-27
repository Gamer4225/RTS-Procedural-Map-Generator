#include "Core/FRTSSeedManager.h"

void UFRTSSeedManager::Initialize(int64 InSeed)
{
    CurrentSeed = InSeed;
    ResetStream();
}

void UFRTSSeedManager::SetSeed(int64 InSeed)
{
    CurrentSeed = InSeed;
    ResetStream();
}

void UFRTSSeedManager::ResetStream()
{
    // FRandomStream takes int32 seed; hash int64 down deterministically
    int32 Seed32 = static_cast<int32>(CurrentSeed ^ (CurrentSeed >> 32));
    Stream = FRandomStream(Seed32);
}

int32 UFRTSSeedManager::RandRange(int32 Min, int32 Max)
{
    if (Min >= Max) return Min;
    return Stream.RandRange(Min, Max);
}

float UFRTSSeedManager::RandFloat()
{
    return Stream.FRand();
}

FVector2D UFRTSSeedManager::RandPointInCircle(float Radius)
{
    // Uniform distribution inside circle using sqrt method
    float Angle = Stream.FRand() * 2.0f * PI;
    float R = FMath::Sqrt(Stream.FRand()) * Radius;
    return FVector2D(FMath::Cos(Angle) * R, FMath::Sin(Angle) * R);
}

void UFRTSSeedManager::ShuffleArray(TArray<int32>& Array)
{
    // Fisher-Yates shuffle using deterministic stream
    for (int32 i = Array.Num() - 1; i > 0; --i)
    {
        int32 j = Stream.RandRange(0, i);
        Array.Swap(i, j);
    }
}
