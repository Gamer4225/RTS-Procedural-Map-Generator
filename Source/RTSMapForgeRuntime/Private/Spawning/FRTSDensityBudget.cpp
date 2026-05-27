#include "Spawning/FRTSDensityBudget.h"

void FRTSDensityBudget::Initialize(int32 GridCellCount)
{
    // Scale budgets by cell count relative to 256×256 baseline (65,536)
    const float BaselineCells = 65536.0f;
    const float ScaleFactor = FMath::Clamp(static_cast<float>(GridCellCount) / BaselineCells, 0.25f, 4.0f);
    
    // Base budgets for 256×256
    const int32 BaseProps = 2000;
    const int32 BaseResources = 64;
    const int32 BaseDecorative = 4000;
    const int32 BaseTotal = 6000;
    
    MaxProps = FMath::RoundToInt(BaseProps * ScaleFactor);
    MaxResources = FMath::RoundToInt(BaseResources * ScaleFactor);
    MaxDecorative = FMath::RoundToInt(BaseDecorative * ScaleFactor);
    MaxTotal = FMath::RoundToInt(BaseTotal * ScaleFactor);
    
    // Hard caps regardless of scale
    MaxTotal = FMath::Min(MaxTotal, 24000);        // Never exceed 24k total
    MaxProps = FMath::Min(MaxProps, 8000);         // Never exceed 8k props
    MaxDecorative = FMath::Min(MaxDecorative, 16000);
    
    // Reset counters
    PlacedProps = 0;
    PlacedResources = 0;
    PlacedDecorative = 0;
    PlacedTotal = 0;
}

bool FRTSDensityBudget::CanPlaceProp() const
{
    return PlacedProps < MaxProps && PlacedTotal < MaxTotal;
}

bool FRTSDensityBudget::CanPlaceResource() const
{
    return PlacedResources < MaxResources && PlacedTotal < MaxTotal;
}

bool FRTSDensityBudget::CanPlaceDecorative() const
{
    return PlacedDecorative < MaxDecorative && PlacedTotal < MaxTotal;
}

bool FRTSDensityBudget::RecordProp()
{
    if (!CanPlaceProp())
    {
        return false;
    }
    ++PlacedProps;
    ++PlacedTotal;
    return true;
}

bool FRTSDensityBudget::RecordResource()
{
    if (!CanPlaceResource())
    {
        return false;
    }
    ++PlacedResources;
    ++PlacedTotal;
    return true;
}

bool FRTSDensityBudget::RecordDecorative()
{
    if (!CanPlaceDecorative())
    {
        return false;
    }
    ++PlacedDecorative;
    ++PlacedTotal;
    return true;
}

FString FRTSDensityBudget::GenerateReport() const
{
    FString Report;
    Report += FString::Printf(TEXT("=== Density Budget Report ===\n"));
    Report += FString::Printf(TEXT("Props:      %d / %d (%.1f%%)\n"), PlacedProps, MaxProps, 
        MaxProps > 0 ? (PlacedProps * 100.0f / MaxProps) : 0.0f);
    Report += FString::Printf(TEXT("Resources:  %d / %d (%.1f%%)\n"), PlacedResources, MaxResources,
        MaxResources > 0 ? (PlacedResources * 100.0f / MaxResources) : 0.0f);
    Report += FString::Printf(TEXT("Decorative: %d / %d (%.1f%%)\n"), PlacedDecorative, MaxDecorative,
        MaxDecorative > 0 ? (PlacedDecorative * 100.0f / MaxDecorative) : 0.0f);
    Report += FString::Printf(TEXT("TOTAL:      %d / %d (%.1f%%)\n"), PlacedTotal, MaxTotal,
        MaxTotal > 0 ? (PlacedTotal * 100.0f / MaxTotal) : 0.0f);
    
    if (IsTotalBudgetExhausted())
    {
        Report += TEXT("⚠️ TOTAL BUDGET EXHAUSTED - further placements will be culled\n");
    }
    
    Report += TEXT("============================\n");
    return Report;
}
