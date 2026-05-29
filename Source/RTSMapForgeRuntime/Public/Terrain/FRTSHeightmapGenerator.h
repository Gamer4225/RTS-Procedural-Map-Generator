#pragma once
#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Terrain/FRTSNoiseGenerator.h"
#include "Core/URTSGenerationSettings.h"

/**
 * Generates normalized heightmap data (0..1) into FRTSGrid using FBM noise.
 * Supports optional island-style radial falloff.
 *
 * CRITICAL: Seed offsets are passed explicitly (int64 InSeed).
 * Generators must NEVER call Settings->ResolveSeed() internally.
 */
class RTSMAPFORGERUNTIME_API FRTSHeightmapGenerator
{
public:
    FRTSHeightmapGenerator() = default;

    // Generates height into Grid.Cells[].Height and computes slope.
    // InSeed: the generation seed (already resolved by pipeline).
    void Generate(FRTSGrid& Grid, const URTSGenerationSettings* Settings, const FRTSNoiseGenerator& Noise, int64 InSeed);

    // Optional: apply radial falloff to push edges toward water
    void ApplyRadialFalloff(FRTSGrid& Grid, float FalloffStrength);

private:
    void ComputeSlopes(FRTSGrid& Grid);
    float ComputeSlopeAt(const FRTSGrid& Grid, int32 X, int32 Y) const;
};
