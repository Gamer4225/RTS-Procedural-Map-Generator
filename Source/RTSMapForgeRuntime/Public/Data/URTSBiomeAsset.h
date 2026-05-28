#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "URTSBiomeAsset.generated.h"
UCLASS(BlueprintType)
class RTSMAPFORGERUNTIME_API URTSBiomeAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Biome") FName BiomeName=FName(TEXT("Default"));
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Debug") FLinearColor DebugColor=FLinearColor::White;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Terrain",meta=(ClampMin="-1",ClampMax="1")) float HeightBias=0;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Terrain",meta=(ClampMin="0")) float RoughnessMultiplier=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Terrain",meta=(ClampMin="0",ClampMax="90")) float CliffThreshold=45;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Traversal",meta=(ClampMin="0")) float BaseMovementCost=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Traversal") bool bAllowBuilding=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Resources",meta=(ClampMin="0",ClampMax="1")) float ResourceDensity=0.5f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Resources") TArray<FName> AllowedResources;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Visuals") TArray<TSoftObjectPtr<UStaticMesh>> PropMeshes;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Visuals",meta=(ClampMin="0",ClampMax="1")) float PropDensity=0.2f;
    static URTSBiomeAsset* CreateDefaultTemperate(UObject* Outer);
    static URTSBiomeAsset* CreateDefaultDesert(UObject* Outer);
    static URTSBiomeAsset* CreateDefaultSnow(UObject* Outer);
    static URTSBiomeAsset* CreateDefaultLava(UObject* Outer);
};
