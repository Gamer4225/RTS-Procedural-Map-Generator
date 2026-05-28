#include "Data/URTSBiomeAsset.h"

URTSBiomeAsset* URTSBiomeAsset::CreateDefaultTemperate(UObject* Outer)
{
    URTSBiomeAsset* B = NewObject<URTSBiomeAsset>(Outer);
    B->BiomeName="Temperate"; B->DebugColor=FLinearColor(0.13f,0.55f,0.13f,1); B->HeightBias=0; B->RoughnessMultiplier=1; B->CliffThreshold=45; B->BaseMovementCost=1; B->bAllowBuilding=true; B->ResourceDensity=0.5f; B->PropDensity=0.3f;
    B->AllowedResources={FName("Wood"),FName("Stone")}; return B;
}
URTSBiomeAsset* URTSBiomeAsset::CreateDefaultDesert(UObject* Outer)
{
    URTSBiomeAsset* B = NewObject<URTSBiomeAsset>(Outer);
    B->BiomeName="Desert"; B->DebugColor=FLinearColor(0.76f,0.70f,0.50f,1); B->HeightBias=-0.05f; B->RoughnessMultiplier=1.8f; B->CliffThreshold=50; B->BaseMovementCost=1.3f; B->bAllowBuilding=true; B->ResourceDensity=0.35f; B->PropDensity=0.1f;
    B->AllowedResources={FName("Oil"),FName("Gold")}; return B;
}
URTSBiomeAsset* URTSBiomeAsset::CreateDefaultSnow(UObject* Outer)
{
    URTSBiomeAsset* B = NewObject<URTSBiomeAsset>(Outer);
    B->BiomeName="Snow"; B->DebugColor=FLinearColor(0.90f,0.95f,1.0f,1); B->HeightBias=0.1f; B->RoughnessMultiplier=0.6f; B->CliffThreshold=40; B->BaseMovementCost=1.5f; B->bAllowBuilding=true; B->ResourceDensity=0.4f; B->PropDensity=0.05f;
    B->AllowedResources={FName("Crystals"),FName("Gas")}; return B;
}
URTSBiomeAsset* URTSBiomeAsset::CreateDefaultLava(UObject* Outer)
{
    URTSBiomeAsset* B = NewObject<URTSBiomeAsset>(Outer);
    B->BiomeName="Lava"; B->DebugColor=FLinearColor(0.8f,0.2f,0.05f,1); B->HeightBias=-0.15f; B->RoughnessMultiplier=2.5f; B->CliffThreshold=35; B->BaseMovementCost=2.0f; B->bAllowBuilding=false; B->ResourceDensity=0.7f; B->PropDensity=0.05f;
    B->AllowedResources={FName("Minerals"),FName("Magma")}; return B;
}
