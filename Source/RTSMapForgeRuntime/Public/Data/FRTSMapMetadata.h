#pragma once

#include "CoreMinimal.h"
#include "FRTSMapMetadata.generated.h"

USTRUCT(BlueprintType)
struct RTSMAPFORGERUNTIME_API FRTSBaseInfo
{
    GENERATED_BODY()

    UPROPERTY()
    int32 PlayerIndex = INDEX_NONE;

    UPROPERTY()
    FVector2D GridPosition = FVector2D::ZeroVector;

    UPROPERTY()
    FVector WorldPosition = FVector::ZeroVector;

    UPROPERTY()
    int32 RegionID = INDEX_NONE;
};

USTRUCT(BlueprintType)
struct RTSMAPFORGERUNTIME_API FRTSExpansionInfo
{
    GENERATED_BODY()

    UPROPERTY()
    int32 OwnerPlayerIndex = INDEX_NONE;

    UPROPERTY()
    FVector2D GridPosition = FVector2D::ZeroVector;

    UPROPERTY()
    FVector WorldPosition = FVector::ZeroVector;

    UPROPERTY()
    float RiskScore = 0.0f; // 0 = safe, 1 = very risky

    UPROPERTY()
    bool bContested = false;
};

USTRUCT(BlueprintType)
struct RTSMAPFORGERUNTIME_API FRTSChokeInfo
{
    GENERATED_BODY()

    UPROPERTY()
    int32 WidthCells = 0;

    UPROPERTY()
    TArray<FIntPoint> Cells;

    UPROPERTY()
    int32 RegionA = INDEX_NONE;

    UPROPERTY()
    int32 RegionB = INDEX_NONE;

    UPROPERTY()
    float Hardness = 0.0f; // 0..1, derived from width
};

USTRUCT(BlueprintType)
struct RTSMAPFORGERUNTIME_API FRTSMapMetadata
{
    GENERATED_BODY()

    UPROPERTY()
    int64 Seed = 0;

    UPROPERTY()
    int32 GridWidth = 0;

    UPROPERTY()
    int32 GridHeight = 0;

    UPROPERTY()
    float CellSize = 200.0f;

    UPROPERTY()
    TArray<FRTSBaseInfo> Bases;

    UPROPERTY()
    TArray<FRTSExpansionInfo> Expansions;

    UPROPERTY()
    TArray<FRTSChokeInfo> Chokes;

    UPROPERTY()
    TArray<FIntPoint> HighGroundCells;

    UPROPERTY()
    TMap<int32, float> RegionSizes; // RegionID -> cell count
};
