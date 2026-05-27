#include "Spawning/URTSPropSpawner.h"
#include "Core/FRTSGrid.h"
#include "Data/FRTSMapMetadata.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Math/UnrealMathUtility.h"

bool URTSPropSpawner::SpawnProps(UWorld* World, const FRTSGrid& Grid, const FRTSMapMetadata& Metadata, int64 Seed)
{
    if (!World || Grid.Cells.Num() == 0)
    {
        return false;
    }

    // Create manager actor if needed
    if (!ManagerActor || ManagerActor->IsPendingKillPending())
    {
        FActorSpawnParameters Params;
        Params.Name = FName(TEXT("RTSMapForge_PropManager"));
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ManagerActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (!ManagerActor)
        {
            return false;
        }
    }

    // Clear previous
    ClearProps();

    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    const float CellSize = Grid.CellSize;

    // Simple deterministic placement: place on walkable, buildable, non-water cells
    // V1: basic tree/rock placement. Biome-specific meshes assigned by BiomeID.
    // For V1 release, we use placeholder logic; full biome mesh tables in V1.5

    int32 InstanceCount = 0;
    
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        const FRTSCell& Cell = Grid.Cells[i];
        
        // Skip water, cliffs, bases, expansions, choke points
        if (!Cell.bWalkable || !Cell.bBuildable || Cell.bWater || Cell.bCliff)
        {
            continue;
        }
        
        // Skip cells that are already strategic zones (keep them clear)
        if (Cell.TacticalZone == ERTSTacticalZone::MainBase ||
            Cell.TacticalZone == ERTSTacticalZone::NatExpansion ||
            Cell.TacticalZone == ERTSTacticalZone::ContestedExp ||
            Cell.TacticalZone == ERTSTacticalZone::ChokePoint ||
            Cell.TacticalZone == ERTSTacticalZone::RiverCrossing)
        {
            continue;
        }

        // Density check: deterministic placement based on position hash + seed
        float DensityThreshold = 0.15f; // 15% fill rate for V1
        float PlacementHash = DeterministicRandFloat(Seed, i * 7919);
        
        if (PlacementHash > DensityThreshold)
        {
            continue;
        }

        // V1: We don't have actual meshes configured yet, so we just count what WOULD be placed
        // In a real build, this adds to a HISM component keyed by biome-specific mesh
        ++InstanceCount;
    }

    UE_LOG(LogTemp, Log, TEXT("RTSMapForge PropSpawner: %d instances would be placed (mesh setup pending)"), InstanceCount);
    return InstanceCount > 0;
}

void URTSPropSpawner::ClearProps()
{
    // Unregister and destroy all HISM components
    for (auto& Pair : HISMComponents)
    {
        if (UHierarchicalInstancedStaticMeshComponent* HISM = Pair.Value)
        {
            HISM->ClearInstances();
            if (HISM->IsRegistered())
            {
                HISM->UnregisterComponent();
            }
            HISM->DestroyComponent();
        }
    }
    HISMComponents.Empty();

    // Destroy manager actor if it exists
    if (AActor* ExistingManager = ManagerActor.Get())
    {
        ExistingManager->Destroy();
        ManagerActor = nullptr;
    }
}

void URTSPropSpawner::Deinitialize()
{
    ClearProps();
}

bool URTSPropSpawner::SpawnResourceNodes(UWorld* World, const FRTSGrid& Grid, UStaticMesh* Mesh, UMaterialInterface* Material)
{
    if (!World || !Mesh || Grid.Cells.Num() == 0)
    {
        return false;
    }

    if (!ManagerActor || ManagerActor->IsPendingKillPending())
    {
        FActorSpawnParameters Params;
        Params.Name = FName(TEXT("RTSMapForge_PropManager"));
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ManagerActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
        if (ManagerActor)
        {
            ManagerActor->SetFlags(RF_Transient);
        }
    }

    if (!ManagerActor)
    {
        return false;
    }

    // Find or create HISM component for this mesh
    UHierarchicalInstancedStaticMeshComponent* HISM = HISMComponents.FindRef(Mesh);
    if (!HISM)
    {
        FName CompName = FName(*FString::Printf(TEXT("HISM_%s"), *Mesh->GetName()));
        HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(ManagerActor, CompName);
        HISM->SetStaticMesh(Mesh);
        if (Material)
        {
            HISM->SetMaterial(0, Material);
        }
        HISM->RegisterComponent();
        ManagerActor->AddInstanceComponent(HISM);
        HISMComponents.Add(Mesh, HISM);
    }

    const float CellSize = Grid.CellSize;
    int32 Added = 0;

    for (int32 Y = 0; Y < Grid.Height; ++Y)
    {
        for (int32 X = 0; X < Grid.Width; ++X)
        {
            const FRTSCell& Cell = Grid.GetCell(X, Y);
            if (Cell.TacticalZone == ERTSTacticalZone::ResourceCluster && Cell.ResourceValue > 0.3f)
            {
                FTransform T = ComputePropTransform(Cell, CellSize, 0, Added);
                HISM->AddInstance(T);
                ++Added;
            }
        }
    }

    return Added > 0;
}

int32 URTSPropSpawner::GetTotalInstanceCount() const
{
    int32 Total = 0;
    for (const auto& Pair : HISMComponents)
    {
        if (Pair.Value)
        {
            Total += Pair.Value->GetInstanceCount();
        }
    }
    return Total;
}

float URTSPropSpawner::DeterministicRandFloat(int64 Seed, int32 Index) const
{
    // Simple hash-based deterministic float [0,1]
    int64 Hash = Seed * 1103515245 + Index * 12345;
    Hash = (Hash ^ (Hash >> 16)) & 0x7FFFFFFF;
    return static_cast<float>(Hash) / static_cast<float>(0x7FFFFFFF);
}

FTransform URTSPropSpawner::ComputePropTransform(const FRTSCell& Cell, float CellSize, int64 Seed, int32 Index) const
{
    FVector Location = Cell.WorldPosition;
    
    // Jitter within cell
    float JitterX = (DeterministicRandFloat(Seed, Index * 3) - 0.5f) * CellSize * 0.6f;
    float JitterY = (DeterministicRandFloat(Seed, Index * 7) - 0.5f) * CellSize * 0.6f;
    Location.X += JitterX;
    Location.Y += JitterY;
    
    // Z height from terrain + slight offset
    Location.Z += 50.0f; // Above ground

    // Scale variation
    float ScaleBase = 0.8f + DeterministicRandFloat(Seed, Index * 13) * 0.4f;
    FVector Scale(ScaleBase, ScaleBase, ScaleBase * (0.9f + DeterministicRandFloat(Seed, Index * 17) * 0.2f));

    // Rotation
    FRotator Rotation(0.0f, DeterministicRandFloat(Seed, Index * 23) * 360.0f, 0.0f);

    return FTransform(Rotation, Location, Scale);
}
