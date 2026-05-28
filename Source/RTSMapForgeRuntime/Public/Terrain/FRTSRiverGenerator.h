#pragma once

// FIX Bug 4: Header moved from Public/Strategic/FRTSRiverGenerator.h to
//            Public/Terrain/FRTSRiverGenerator.h where it belongs conceptually
//            and physically (its .cpp lives in Private/Terrain/).
//            Any include of "Strategic/FRTSRiverGenerator.h" must be updated
//            to "Terrain/FRTSRiverGenerator.h".

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"
#include "Core/URTSGenerationSettings.h"
#include "Core/FRTSSeedManager.h"

/**
 * Generates rivers via gradient descent from high terrain toward water level.
 * Applies widening and lateral jitter for natural-looking waterways.
 *
 * CRITICAL: Uses SeedManager for all randomness — never calls ResolveSeed() internally.
 */
class RTSMAPFORGERUNTIME_API FRTSRiverGenerator
{
public:
    FRTSRiverGenerator() = default;

    void Generate(FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager);
};
