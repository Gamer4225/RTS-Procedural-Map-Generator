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
 * 
 * Rules:
 *   - Only place on bWalkable && bBuildable cells
 *   - Respect biome prop density from URTSBiomeAsset
 *   - Scale/rotation from deterministic seeded pseudo-random
 *   - Z-position from Cell.Height + Cell.Slope
 */
UCLASS(BlueprintType)
class RTSMAPFORGERUNTIME_API URTSPropSpawner : public UObject
{
    GENERATED_BODY()

public:
    // Spawn props into World based on Grid + Metadata.
    // Returns true if any props spawned.
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning")
    bool SpawnProps(UWorld* World, const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, int64 Seed);

    // Clear all previously spawned HISM components.
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning")
    void ClearProps();

    // Spawn a specific resource mesh at grid positions marked ResourceCluster.
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning")
    bool SpawnResourceNodes(UWorld* World, const FRTSGrid& Grid, UStaticMesh* Mesh, UMaterialInterface* Material);

    // Returns number of currently instanced meshes.
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning", BlueprintPure)
    int32 GetTotalInstanceCount() const;

    // Cleanup: destroys all components and manager actor.
    UFUNCTION(BlueprintCallable, Category = "RTSMapForge|Spawning")
    void Deinitialize();

private:
    // Per-biome HISM components keyed by StaticMesh path
    UPROPERTY()
    TMap<TObjectPtr<UStaticMesh>, TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> HISMComponents;

    UPROPERTY()
    TObjectPtr<AActor> ManagerActor = nullptr;

    // Deterministic random helpers
    float DeterministicRandFloat(int64 Seed, int32 Index) const;
    FTransform ComputePropTransform(const FRTSCell& Cell, float CellSize, int64 Seed, int32 Index) const;
};
