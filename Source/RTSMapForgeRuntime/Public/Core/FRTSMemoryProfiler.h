#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
struct RTSMAPFORGERUNTIME_API FRTSSubsystemMemory { FString Name; SIZE_T Bytes=0; int32 ElementCount=0; SIZE_T BytesPerElement=0; };
class RTSMAPFORGERUNTIME_API FRTSMemoryProfiler
{
public:
    void Snapshot(const FString& Label, const FRTSGrid& Grid);
    void RecordSubsystem(const FString& Name, SIZE_T Bytes, int32 ElementCount, SIZE_T BytesPerElement);
    FString GenerateReport() const;
    bool IsWithinTarget(int32 GridWidth, int32 GridHeight) const;
    SIZE_T GetTotalMemory() const;
    SIZE_T GetGridMemory() const { return GridMemory; }
    SIZE_T GetOverlayMemory() const { return OverlayMemory; }
    SIZE_T GetValidationMemory() const { return ValidationMemory; }
private:
    FString LastLabel; SIZE_T GridMemory=0,OverlayMemory=0,ValidationMemory=0,InfluenceMemory=0,HeatmapMemory=0;
    TArray<FRTSSubsystemMemory> Subsystems;
    SIZE_T GetTargetMemory(int32 CellCount) const;
};
