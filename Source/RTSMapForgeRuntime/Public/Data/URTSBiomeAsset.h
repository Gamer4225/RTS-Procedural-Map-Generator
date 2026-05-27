#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "URTSBiomeAsset.generated.h"

/**
 * Data-driven biome configuration. Designers create these as Data Assets.
 * Controls terrain behavior, traversal, resources, and future visuals.
 * 
 * DEFAULT BIOMES (shipped with plugin):
 *   - Temperate: balanced, green, standard movement
 *   - Desert: sandy, higher roughness, slower movement, oil resources
 *   - Snow: white, flat areas, slow movement, crystals
 *   - Lava: hazardous, high roughness, blocked building, rare minerals
 */
UCLASS(BlueprintType)
class RTSMAPFORGERUNTIME_API URTSBiomeAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Biome")
    FName BiomeName = FName(TEXT("Default"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FLinearColor DebugColor = FLinearColor::White;

    // === TERRAIN RULES ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "-1.0", ClampMax = "1.0"))
    float HeightBias = 0.0f; // Push terrain up/down

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0"))
    float RoughnessMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain", meta = (ClampMin = "0.0", ClampMax = "90.0"))
    float CliffThreshold = 45.0f; // Degrees slope

    // === TRAVERSAL RULES ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal", meta = (ClampMin = "0.0"))
    float BaseMovementCost = 1.0f; // 1.0 = normal

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traversal")
    bool bAllowBuilding = true;

    // === RESOURCE RULES ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ResourceDensity = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resources")
    TArray<FName> AllowedResources;

    // === VISUAL RULES (Future mesh placement) ===
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
    TArray<TSoftObjectPtr<UStaticMesh>> PropMeshes;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PropDensity = 0.2f;

    // === STATIC DEFAULT FACTORY ===
    // Returns built-in default biome instances for common types.
    // These are NOT DataAssets - they're runtime UObject instances.
    static URTSBiomeAsset* CreateDefaultTemperate(UObject* Outer);
    static URTSBiomeAsset* CreateDefaultDesert(UObject* Outer);
    static URTSBiomeAsset* CreateDefaultSnow(UObject* Outer);
    static URTSBiomeAsset* CreateDefaultLava(UObject* Outer);
};
