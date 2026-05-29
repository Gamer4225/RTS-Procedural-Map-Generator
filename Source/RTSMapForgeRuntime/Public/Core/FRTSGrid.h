#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSCell.h"
#include "FRTSGrid.generated.h"

/**
 * Container for the entire map grid. Flat array storage for cache locality.
 * All indexing is [Y * Width + X].
 *
 * CRITICAL DESIGN: No per-cell dynamic allocations. Neighbors are computed on-the-fly.
 * Bounds-checked in Debug builds via checkf() macros.
 */
USTRUCT(BlueprintType)
struct RTSMAPFORGERUNTIME_API FRTSGrid
{
    GENERATED_BODY()

    UPROPERTY()
    int32 Width = 0;

    UPROPERTY()
    int32 Height = 0;

    UPROPERTY()
    float CellSize = 200.0f; // World units per cell (cm)

    UPROPERTY()
    TArray<FRTSCell> Cells;

    FRTSGrid() = default;
    FRTSGrid(int32 InWidth, int32 InHeight, float InCellSize);

    void Initialize(int32 InWidth, int32 InHeight, float InCellSize);

    // === Fast Access (Bounds-checked in Debug) ===
    FORCEINLINE FRTSCell& GetCell(int32 X, int32 Y)
    {
        checkf(IsValidCoord(X, Y), TEXT("FRTSGrid::GetCell out of bounds: (%d, %d) vs Grid(%d, %d)"), X, Y, Width, Height);
        return Cells[Y * Width + X];
    }

    FORCEINLINE const FRTSCell& GetCell(int32 X, int32 Y) const
    {
        checkf(IsValidCoord(X, Y), TEXT("FRTSGrid::GetCell out of bounds: (%d, %d) vs Grid(%d, %d)"), X, Y, Width, Height);
        return Cells[Y * Width + X];
    }

    FORCEINLINE FRTSCell& GetCell(int32 Index)
    {
        checkf(Cells.IsValidIndex(Index), TEXT("FRTSGrid::GetCell index out of bounds: %d vs Num=%d"), Index, Cells.Num());
        return Cells[Index];
    }

    FORCEINLINE const FRTSCell& GetCell(int32 Index) const
    {
        checkf(Cells.IsValidIndex(Index), TEXT("FRTSGrid::GetCell index out of bounds: %d vs Num=%d"), Index, Cells.Num());
        return Cells[Index];
    }

    FORCEINLINE int32 ToIndex(int32 X, int32 Y) const
    {
        checkf(IsValidCoord(X, Y), TEXT("FRTSGrid::ToIndex out of bounds: (%d, %d) vs Grid(%d, %d)"), X, Y, Width, Height);
        return Y * Width + X;
    }

    FORCEINLINE FIntPoint ToCoord(int32 Index) const
    {
        checkf(Cells.IsValidIndex(Index), TEXT("FRTSGrid::ToCoord index out of bounds: %d vs Num=%d"), Index, Cells.Num());
        return FIntPoint(Index % Width, Index / Width);
    }

    FORCEINLINE bool IsValidCoord(int32 X, int32 Y) const
    {
        return X >= 0 && X < Width && Y >= 0 && Y < Height;
    }

    FORCEINLINE bool IsValidIndex(int32 Index) const
    {
        return Cells.IsValidIndex(Index);
    }

    // Neighbor lookup (4-dir or 8-dir). Results stored in OutNeighbors (caller-owned TArray).
    // ZERO allocations from this function — it only appends to the provided array.
    void GetNeighbors(int32 Index, bool bDiagonal, TArray<int32>& OutNeighbors) const;

    // Batch neighbor compute — writes up to 8 int32s into OutFixedArray, returns count.
    // Use this for hot loops where you want zero TArray overhead.
    int32 GetNeighborsFixed(int32 Index, bool bDiagonal, int32 OutFixedArray[8]) const;

    // World-space conversion
    FVector GridToWorld(int32 X, int32 Y, float ZOffset = 0.0f) const;
    FIntPoint WorldToGrid(FVector WorldLocation) const;

    // Memory stats
    SIZE_T GetAllocatedSize() const;
};
