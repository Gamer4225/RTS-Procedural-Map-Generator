#include "Data/URTSBiomeAsset.h"

URTSBiomeAsset* URTSBiomeAsset::CreateDefaultTemperate(UObject* Outer)
{
    URTSBiomeAsset* Biome = NewObject<URTSBiomeAsset>(Outer);
    Biome->BiomeName = FName(TEXT("Temperate"));
    Biome->DebugColor = FLinearColor(0.13f, 0.55f, 0.13f, 1.0f); // Forest green
    Biome->HeightBias = 0.0f;
    Biome->RoughnessMultiplier = 1.0f;
    Biome->CliffThreshold = 45.0f;
    Biome->BaseMovementCost = 1.0f;
    Biome->bAllowBuilding = true;
    Biome->ResourceDensity = 0.5f;
    Biome->AllowedResources = { FName(TEXT("Wood")), FName(TEXT("Stone")) };
    Biome->PropDensity = 0.3f;
    return Biome;
}

URTSBiomeAsset* URTSBiomeAsset::CreateDefaultDesert(UObject* Outer)
{
    URTSBiomeAsset* Biome = NewObject<URTSBiomeAsset>(Outer);
    Biome->BiomeName = FName(TEXT("Desert"));
    Biome->DebugColor = FLinearColor(0.76f, 0.70f, 0.50f, 1.0f); // Sand
    Biome->HeightBias = -0.05f; // Slightly lower
    Biome->RoughnessMultiplier = 1.8f; // Dunes
    Biome->CliffThreshold = 50.0f;
    Biome->BaseMovementCost = 1.3f; // Sand slows units
    Biome->bAllowBuilding = true;
    Biome->ResourceDensity = 0.35f;
    Biome->AllowedResources = { FName(TEXT("Oil")), FName(TEXT("Gold")) };
    Biome->PropDensity = 0.1f;
    return Biome;
}

URTSBiomeAsset* URTSBiomeAsset::CreateDefaultSnow(UObject* Outer)
{
    URTSBiomeAsset* Biome = NewObject<URTSBiomeAsset>(Outer);
    Biome->BiomeName = FName(TEXT("Snow"));
    Biome->DebugColor = FLinearColor(0.90f, 0.95f, 1.0f, 1.0f); // White-blue
    Biome->HeightBias = 0.1f; // Higher elevation bias
    Biome->RoughnessMultiplier = 0.6f; // Flatter ice
    Biome->CliffThreshold = 40.0f;
    Biome->BaseMovementCost = 1.5f; // Snow slows units more
    Biome->bAllowBuilding = true;
    Biome->ResourceDensity = 0.4f;
    Biome->AllowedResources = { FName(TEXT("Crystals")), FName(TEXT("Gas")) };
    Biome->PropDensity = 0.05f;
    return Biome;
}

URTSBiomeAsset* URTSBiomeAsset::CreateDefaultLava(UObject* Outer)
{
    URTSBiomeAsset* Biome = NewObject<URTSBiomeAsset>(Outer);
    Biome->BiomeName = FName(TEXT("Lava"));
    Biome->DebugColor = FLinearColor(0.8f, 0.2f, 0.05f, 1.0f); // Red-orange
    Biome->HeightBias = -0.15f; // Lower, volcanic rifts
    Biome->RoughnessMultiplier = 2.5f; // Jagged volcanic rock
    Biome->CliffThreshold = 35.0f; // More cliffs
    Biome->BaseMovementCost = 2.0f; // Very slow
    Biome->bAllowBuilding = false; // Too hazardous
    Biome->ResourceDensity = 0.7f; // Rich in rare minerals
    Biome->AllowedResources = { FName(TEXT("Minerals")), FName(TEXT("Magma")) };
    Biome->PropDensity = 0.05f;
    return Biome;
}
