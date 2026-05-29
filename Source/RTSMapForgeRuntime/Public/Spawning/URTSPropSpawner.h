#pragma once
#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "URTSPropSpawner.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

/**
 * V1 Prop Spawning: Hierarchical Instanced Static Mesh (HISM) placement.
 *
 * DESIGN: Avoids Actor hell. Uses HISM components attached to a single manager actor.
 * Each biome prop type (tree, rock, resource node) gets one HISM component.
 * Placement is deterministic from grid metadata.
 */
UCLASS(BlueprintType)
class RTSMAPFORGERUNTIME_API URTSPropSpawner : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning")
    bool SpawnProps(UWorld* World, const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, int64 Seed);

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning")
    void ClearProps();

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning")
    bool SpawnResourceNodes(UWorld* World, const FRTSGrid& Grid, UStaticMesh* Mesh, UMaterialInterface* Material);

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning", BlueprintPure)
    int32 GetTotalInstanceCount() const;

    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning")
    void Deinitialize();

private:
    UPROPERTY()
    TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMComponents;

    UPROPERTY()
    TObjectPtr<AActor> ManagerActor = nullptr;

    float DeterministicRandFloat(int64 Seed, int32 Index) const;
    FTransform ComputePropTransform(const FRTSCell& Cell, float CellSize, int64 Seed, int32 Index) const;
};
