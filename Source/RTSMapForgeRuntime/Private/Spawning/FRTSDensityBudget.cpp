#include "Spawning/FRTSDensityBudget.h"

void FRTSDensityBudget::Initialize(int32 GridCellCount)
{
    const float Scale = FMath::Clamp(static_cast<float>(GridCellCount) / 65536.0f, 0.25f, 4.0f);
    MaxProps      = FMath::Min(FMath::RoundToInt(2000 * Scale), 8000);
    MaxResources  = FMath::RoundToInt(64   * Scale);
    MaxDecorative = FMath::Min(FMath::RoundToInt(4000 * Scale), 16000);
    MaxTotal      = FMath::Min(FMath::RoundToInt(6000 * Scale), 24000);
    PlacedProps = PlacedResources = PlacedDecorative = PlacedTotal = 0;
}

bool FRTSDensityBudget::CanPlaceProp()       const { return PlacedProps      < MaxProps      && PlacedTotal < MaxTotal; }
bool FRTSDensityBudget::CanPlaceResource()   const { return PlacedResources  < MaxResources  && PlacedTotal < MaxTotal; }
bool FRTSDensityBudget::CanPlaceDecorative() const { return PlacedDecorative < MaxDecorative && PlacedTotal < MaxTotal; }

bool FRTSDensityBudget::RecordProp()       { if (!CanPlaceProp())       return false; ++PlacedProps;      ++PlacedTotal; return true; }
bool FRTSDensityBudget::RecordResource()   { if (!CanPlaceResource())   return false; ++PlacedResources;  ++PlacedTotal; return true; }
bool FRTSDensityBudget::RecordDecorative() { if (!CanPlaceDecorative()) return false; ++PlacedDecorative; ++PlacedTotal; return true; }

FString FRTSDensityBudget::GenerateReport() const
{
    FString R;
    R += TEXT("=== Density Budget ===\n");
    R += FString::Printf(TEXT("Props:      %d / %d\n"), PlacedProps,      MaxProps);
    R += FString::Printf(TEXT("Resources:  %d / %d\n"), PlacedResources,  MaxResources);
    R += FString::Printf(TEXT("Decorative: %d / %d\n"), PlacedDecorative, MaxDecorative);
    R += FString::Printf(TEXT("TOTAL:      %d / %d\n"), PlacedTotal,      MaxTotal);
    if (IsTotalBudgetExhausted()) R += TEXT("WARNING: Total budget exhausted.\n");
    return R;
}
