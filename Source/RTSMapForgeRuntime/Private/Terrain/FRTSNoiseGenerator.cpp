#include "Terrain/FRTSNoiseGenerator.h"
#include "Math/UnrealMathUtility.h"
#include "Misc/DateTime.h"

FRTSNoiseGenerator::FRTSNoiseGenerator()
{
    // Zero-initialize until seeded
    FMemory::Memzero(Permutation, sizeof(Permutation));
}

void FRTSNoiseGenerator::Initialize(int32 Seed)
{
    // Deterministic permutation table generation
    int32 BasePerm[256];
    for (int32 i = 0; i < 256; ++i)
    {
        BasePerm[i] = i;
    }

    // Shuffle using a simple LCG seeded by Seed
    int32 State = Seed;
    for (int32 i = 255; i > 0; --i)
    {
        // LCG step
        State = (State * 1103515245 + 12345) & 0x7fffffff;
        int32 j = State % (i + 1);
        Swap(BasePerm[i], BasePerm[j]);
    }

    for (int32 i = 0; i < 256; ++i)
    {
        Permutation[i] = BasePerm[i];
        Permutation[i + 256] = BasePerm[i];
    }
}

float FRTSNoiseGenerator::Fade(float T)
{
    // 6t^5 - 15t^4 + 10t^3
    return T * T * T * (T * (T * 6.0f - 15.0f) + 10.0f);
}

float FRTSNoiseGenerator::Lerp(float A, float B, float T)
{
    return A + T * (B - A);
}

float FRTSNoiseGenerator::Grad(int32 Hash, float X, float Y)
{
    // Convert low 3 bits of hash code into 8 gradient directions
    int32 H = Hash & 0x7;
    float U = H < 4 ? X : Y;
    float V = H < 4 ? Y : X;
    float SignU = (H & 1) ? -1.0f : 1.0f;
    float SignV = (H & 2) ? -1.0f : 1.0f;
    return SignU * U + SignV * V;
}

float FRTSNoiseGenerator::PerlinNoise2D(float X, float Y) const
{
    int32 X0 = FMath::FloorToInt(X) & 255;
    int32 Y0 = FMath::FloorToInt(Y) & 255;

    float Xf = X - FMath::FloorToInt(X);
    float Yf = Y - FMath::FloorToInt(Y);

    float U = Fade(Xf);
    float V = Fade(Yf);

    int32 A = Permutation[X0] + Y0;
    int32 B = Permutation[X0 + 1] + Y0;

    float N00 = Grad(Permutation[A], Xf, Yf);
    float N01 = Grad(Permutation[A + 1], Xf, Yf - 1.0f);
    float N10 = Grad(Permutation[B], Xf - 1.0f, Yf);
    float N11 = Grad(Permutation[B + 1], Xf - 1.0f, Yf - 1.0f);

    float X0Lerp = Lerp(N00, N10, U);
    float X1Lerp = Lerp(N01, N11, U);

    return Lerp(X0Lerp, X1Lerp, V);
}

float FRTSNoiseGenerator::FBM(float X, float Y, int32 Octaves, float Persistence, float Lacunarity) const
{
    float Total = 0.0f;
    float Amplitude = 1.0f;
    float Frequency = 1.0f;
    float MaxValue = 0.0f;

    for (int32 i = 0; i < Octaves; ++i)
    {
        Total += Amplitude * PerlinNoise2D(X * Frequency, Y * Frequency);
        MaxValue += Amplitude;
        Amplitude *= Persistence;
        Frequency *= Lacunarity;
    }

    if (MaxValue > KINDA_SMALL_NUMBER)
    {
        Total /= MaxValue;
    }

    return Total; // roughly [-1, 1]
}

float FRTSNoiseGenerator::SeededFBM(float X, float Y, int32 Octaves, float Persistence, float Lacunarity, float OffsetX, float OffsetY) const
{
    return FBM(X + OffsetX, Y + OffsetY, Octaves, Persistence, Lacunarity);
}
