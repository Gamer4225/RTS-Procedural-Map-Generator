#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"

class ALandscapeProxy;
class UWorld;

/**
 * Landscape Bake: Writes FRTSGrid height data into an Unreal Landscape.
 *
 * STATUS: Implemented for editor builds using the Landscape edit interface.
 *
 * DESIGN: One-way data flow — Grid heights → Landscape Heightmap.
 * The Landscape is created/modified in the editor viewport, NOT at runtime.
 *
 * Pipeline:
 *   1. Create or find ALandscapeProxy in current level
 *   2. Convert Cell.Height [0,1] to Landscape heightmap uint16 values
 *   3. Write into Landscape heightmap texture
 *   4. Regenerate collision/navmesh if requested
 *
 * NOTE: Requires Editor module; Runtime module stores this header only.
 * Actual bake call lives in EditorSubsystem or Editor module.
 */
class RTSMAPFORGERUNTIME_API FRTSLandscapeBaker
{
public:
    // Editor-only bake: writes Grid height data into a Landscape proxy using the Landscape edit interface.
    static bool BakeToLandscape(const FRTSGrid& Grid, UWorld* World, bool bRegenerateCollision = true);

    // Utility: converts normalized height [0,1] to Landscape uint16 range [0, 65535]
    static uint16 HeightToLandscape(float NormalizedHeight);

private:
    static ALandscapeProxy* FindOrCreateLandscape(UWorld* World, int32 Width, int32 Height, float CellSize);
    static bool WriteHeightmap(ALandscapeProxy* Landscape, const FRTSGrid& Grid);
};
