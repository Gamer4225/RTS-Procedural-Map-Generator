#pragma once
#include "CoreMinimal.h"
#include "Strategic/ERTSTacticalZone.h"
#include "FRTSCell.generated.h"

USTRUCT(BlueprintType)
struct RTSMAPFORGERUNTIME_API FRTSCell
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly, Category="Spatial")  FVector2D GridCoord = FVector2D::ZeroVector;
    FVector WorldPosition = FVector::ZeroVector;
    float Height = 0.0f;
    float Slope  = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") float MovementCostMultiplier = 1.0f;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") bool bWalkable  = true;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") bool bBuildable = true;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") bool bWater     = false;
    UPROPERTY(BlueprintReadOnly, Category="Traversal") bool bCliff     = false;
    UPROPERTY(BlueprintReadOnly, Category="Gameplay")  float CoverValue       = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Gameplay")  float VisibilityScore  = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Gameplay")  float ExposureScore    = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Strategic") float StrategicValue   = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Strategic") float ResourceValue    = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Strategic") float ControlValue     = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category="Strategic") ERTSTacticalZone TacticalZone = ERTSTacticalZone::Unclassified;
    int32 RegionID = INDEX_NONE;
    int32 BiomeID  = INDEX_NONE;
    int32 ChunkID  = INDEX_NONE;
    FRTSCell() = default;
};
