#include "Terrain/FRTSHeightmapGenerator.h"
#include "Math/UnrealMathUtility.h"

void FRTSHeightmapGenerator::Generate(FRTSGrid& Grid, const URTSGenerationSettings* Settings, const FRTSNoiseGenerator& Noise, int64 InSeed)
{
    if (!Settings || Grid.Cells.Num() == 0)
    {
        return;
    }

    const int32 W           = Grid.Width;
    const int32 H           = Grid.Height;
    const float Scale       = Settings->TerrainScale;
    const int32 Octaves     = Settings->FBMOctaves;
    const float Persistence = Settings->FBMPersistence;
    const float Lacunarity  = Settings->FBMLacunarity;

    // Use deterministic offsets derived from the ALREADY-RESOLVED seed.
    // CRITICAL: Generators must NEVER call Settings->ResolveSeed() internally.
    // Seed resolution is owned exclusively by the pipeline entry point.
    const float OffsetX = static_cast<float>(InSeed & 0xFFFF) * 0.01f;
    const float OffsetY = static_cast<float>((InSeed >> 16) & 0xFFFF) * 0.01f;

    for (int32 Y = 0; Y < H; ++Y)
    {
        for (int32 X = 0; X < W; ++X)
        {
            float NX = static_cast<float>(X) / static_cast<float>(W) * Scale;
            float NY = static_cast<float>(Y) / static_cast<float>(H) * Scale;

            float RawHeight = Noise.SeededFBM(NX, NY, Octaves, Persistence, Lacunarity, OffsetX, OffsetY);

            // Map from [-1, 1] -> [0, 1]
            float Normalized = (RawHeight + 1.0f) * 0.5f;
            Normalized = FMath::Clamp(Normalized, 0.0f, 1.0f);

            FRTSCell& Cell = Grid.GetCell(X, Y);
            Cell.Height = Normalized;
        }
    }

    ComputeSlopes(Grid);
}

void FRTSHeightmapGenerator::ApplyRadialFalloff(FRTSGrid& Grid, float FalloffStrength)
{
    if (FalloffStrength <= 0.0f || Grid.Cells.Num() == 0)
    {
        return;
    }

    const int32 W        = Grid.Width;
    const int32 H        = Grid.Height;
    const float CenterX  = (W - 1) * 0.5f;
    const float CenterY  = (H - 1) * 0.5f;
    const float MaxDist  = FMath::Sqrt(CenterX * CenterX + CenterY * CenterY);

    for (int32 Y = 0; Y < H; ++Y)
    {
        for (int32 X = 0; X < W; ++X)
        {
            float DX   = static_cast<float>(X) - CenterX;
            float DY   = static_cast<float>(Y) - CenterY;
            float Dist = FMath::Sqrt(DX * DX + DY * DY) / MaxDist;

            // Smoothstep falloff from edges
            float Falloff = Dist * Dist * Dist * (Dist * (Dist * 6.0f - 15.0f) + 10.0f);
            Falloff *= FalloffStrength;

            FRTSCell& Cell = Grid.GetCell(X, Y);
            Cell.Height = FMath::Clamp(Cell.Height - Falloff, 0.0f, 1.0f);
        }
    }

    ComputeSlopes(Grid);
}

void FRTSHeightmapGenerator::ComputeSlopes(FRTSGrid& Grid)
{
    for (int32 Y = 0; Y < Grid.Height; ++Y)
    {
        for (int32 X = 0; X < Grid.Width; ++X)
        {
            FRTSCell& Cell = Grid.GetCell(X, Y);
            Cell.Slope = ComputeSlopeAt(Grid, X, Y);
        }
    }
}

float FRTSHeightmapGenerator::ComputeSlopeAt(const FRTSGrid& Grid, int32 X, int32 Y) const
{
    const int32 W        = Grid.Width;
    const int32 H        = Grid.Height;
    const float CellSize = Grid.CellSize;

    float HCenter = Grid.GetCell(X, Y).Height;
    float HLeft   = (X > 0)     ? Grid.GetCell(X - 1, Y).Height : HCenter;
    float HRight  = (X < W - 1) ? Grid.GetCell(X + 1, Y).Height : HCenter;
    float HUp     = (Y > 0)     ? Grid.GetCell(X, Y - 1).Height : HCenter;
    float HDown   = (Y < H - 1) ? Grid.GetCell(X, Y + 1).Height : HCenter;

    float dX = (HRight - HLeft) * 0.5f;
    float dY = (HDown  - HUp  ) * 0.5f;

    float Gradient = FMath::Sqrt(dX * dX + dY * dY) / CellSize;
    return FMath::Atan(Gradient);
}
