#include "Core/FRTSGrid.h"

FRTSGrid::FRTSGrid(int32 InWidth, int32 InHeight, float InCellSize)
{
    Initialize(InWidth, InHeight, InCellSize);
}

void FRTSGrid::Initialize(int32 InWidth, int32 InHeight, float InCellSize)
{
    Width    = InWidth;
    Height   = InHeight;
    CellSize = InCellSize;

    const int32 TotalCells = Width * Height;
    Cells.Empty(TotalCells);
    Cells.SetNumZeroed(TotalCells);

    for (int32 Y = 0; Y < Height; ++Y)
    {
        for (int32 X = 0; X < Width; ++X)
        {
            int32 Idx = Y * Width + X;
            FRTSCell& Cell = Cells[Idx];
            Cell.GridCoord   = FVector2D(static_cast<float>(X), static_cast<float>(Y));
            Cell.WorldPosition = GridToWorld(X, Y);
            Cell.RegionID    = INDEX_NONE;
            Cell.BiomeID     = INDEX_NONE;
            Cell.ChunkID     = INDEX_NONE;
        }
    }
}

void FRTSGrid::GetNeighbors(int32 Index, bool bDiagonal, TArray<int32>& OutNeighbors) const
{
    OutNeighbors.Reset();
    if (!Cells.IsValidIndex(Index)) return;

    const int32 X = Index % Width;
    const int32 Y = Index / Width;

    if (X > 0)          OutNeighbors.Add(Index - 1);
    if (X < Width - 1)  OutNeighbors.Add(Index + 1);
    if (Y > 0)          OutNeighbors.Add(Index - Width);
    if (Y < Height - 1) OutNeighbors.Add(Index + Width);

    if (bDiagonal)
    {
        const bool bU = (Y > 0), bD = (Y < Height - 1), bL = (X > 0), bR = (X < Width - 1);
        if (bU && bL) OutNeighbors.Add(Index - Width - 1);
        if (bU && bR) OutNeighbors.Add(Index - Width + 1);
        if (bD && bL) OutNeighbors.Add(Index + Width - 1);
        if (bD && bR) OutNeighbors.Add(Index + Width + 1);
    }
}

int32 FRTSGrid::GetNeighborsFixed(int32 Index, bool bDiagonal, int32 OutFixedArray[8]) const
{
    if (!Cells.IsValidIndex(Index)) return 0;
    const int32 X = Index % Width;
    const int32 Y = Index / Width;
    int32 Count = 0;

    if (X > 0)          OutFixedArray[Count++] = Index - 1;
    if (X < Width - 1)  OutFixedArray[Count++] = Index + 1;
    if (Y > 0)          OutFixedArray[Count++] = Index - Width;
    if (Y < Height - 1) OutFixedArray[Count++] = Index + Width;

    if (bDiagonal)
    {
        const bool bU = (Y > 0), bD = (Y < Height - 1), bL = (X > 0), bR = (X < Width - 1);
        if (bU && bL) OutFixedArray[Count++] = Index - Width - 1;
        if (bU && bR) OutFixedArray[Count++] = Index - Width + 1;
        if (bD && bL) OutFixedArray[Count++] = Index + Width - 1;
        if (bD && bR) OutFixedArray[Count++] = Index + Width + 1;
    }
    return Count;
}

FVector FRTSGrid::GridToWorld(int32 X, int32 Y, float ZOffset) const
{
    return FVector(static_cast<float>(X) * CellSize, static_cast<float>(Y) * CellSize, ZOffset);
}

FIntPoint FRTSGrid::WorldToGrid(FVector WorldLocation) const
{
    return FIntPoint(FMath::FloorToInt(WorldLocation.X / CellSize), FMath::FloorToInt(WorldLocation.Y / CellSize));
}

SIZE_T FRTSGrid::GetAllocatedSize() const
{
    return sizeof(*this) + Cells.GetAllocatedSize() + (Cells.Num() * sizeof(FRTSCell));
}
