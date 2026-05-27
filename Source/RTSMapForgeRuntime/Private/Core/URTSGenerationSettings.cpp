#include "Core/URTSGenerationSettings.h"

float URTSGenerationSettings::GetMapDiagonal() const
{
    const float W = static_cast<float>(GridWidth) * CellSize;
    const float H = static_cast<float>(GridHeight) * CellSize;
    return FMath::Sqrt(W * W + H * H);
}

int64 URTSGenerationSettings::ResolveSeed() const
{
    if (bRandomSeed)
    {
        return static_cast<int64>(FMath::Rand()) ^ (static_cast<int64>(FMath::Rand()) << 16) ^ (static_cast<int64>(FMath::Rand()) << 32);
    }
    return Seed;
}
