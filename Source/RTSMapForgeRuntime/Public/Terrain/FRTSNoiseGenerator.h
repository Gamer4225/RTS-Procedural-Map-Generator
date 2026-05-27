#pragma once

#include "CoreMinimal.h"

/**
 * Deterministic noise generator using classic Perlin noise with FBM compositing.
 * All random permutation table generation is seeded externally.
 */
class RTSMAPFORGERUNTIME_API FRTSNoiseGenerator
{
public:
    FRTSNoiseGenerator();

    // Initialize permutation table with a deterministic seed
    void Initialize(int32 Seed);

    // Classic 2D Perlin noise, returns [-1, 1]
    float PerlinNoise2D(float X, float Y) const;

    // Fractal Brownian Motion: layered Perlin noise
    // Returns roughly [-1, 1], caller should normalize to [0,1] if desired
    float FBM(float X, float Y, int32 Octaves, float Persistence, float Lacunarity) const;

    // Seeded FBM with internal offset derived from seed
    float SeededFBM(float X, float Y, int32 Octaves, float Persistence, float Lacunarity, float OffsetX, float OffsetY) const;

private:
    // Permutation table (256 + 256 for overflow wrap)
    int32 Permutation[512];

    static float Fade(float T);
    static float Lerp(float A, float B, float T);
    static float Grad(int32 Hash, float X, float Y);
};
