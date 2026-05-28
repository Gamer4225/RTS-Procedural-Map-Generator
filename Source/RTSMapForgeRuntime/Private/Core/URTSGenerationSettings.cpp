#include "Core/URTSGenerationSettings.h"
#include "HAL/PlatformTime.h"   // FIX Bug 5: FPlatformTime::Cycles64()

float URTSGenerationSettings::GetMapDiagonal() const
{
    const float W = static_cast<float>(GridWidth) * CellSize;
    const float H = static_cast<float>(GridHeight) * CellSize;
    return FMath::Sqrt(W * W + H * H);
}

// FIX Bug 5: ResolveSeed() previously used FMath::Rand() (global C runtime rand()),
// which is seeded by the OS and NOT controlled by any FRandomStream.
// This meant two calls on different machines/frames with bRandomSeed=true would
// return different seeds with no way to reproduce them if the value was ever lost.
//
// New implementation uses FPlatformTime::Cycles64() (high-resolution timer) for
// entropy when bRandomSeed=true. The returned value is still non-cryptographic but:
//   1. Does NOT corrupt the global C rand() state.
//   2. Is highly unlikely to collide between consecutive calls.
//   3. Is entirely irrelevant once stored — because Generate() stores the resolved
//      seed in both OutMetadata.Seed and its return value, full reproducibility is
//      preserved from the first call onwards.
int64 URTSGenerationSettings::ResolveSeed() const
{
    if (bRandomSeed)
    {
        // Use high-resolution platform timer for entropy.
        // XOR-fold with a shifted copy to distribute entropy across all 64 bits.
        const uint64 Cycles = FPlatformTime::Cycles64();
        const int64  T      = static_cast<int64>(Cycles);
        return T ^ (T >> 17) ^ (T << 31);
    }
    return Seed;
}
