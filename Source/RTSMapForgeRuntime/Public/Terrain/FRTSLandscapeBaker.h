#pragma once

#include "CoreMinimal.h"
#include "Core/FRTSGrid.h"

class ALandscapeProxy;
class UWorld;

/**
 * Landscape Bake: Writes FRTSGrid height data into an existing Unreal Landscape.
 *
 * STATUS: Implemented for editor builds using the Landscape edit interface.
 *
 * DESIGN: One-way data flow — Grid heights → Landscape Heightmap.
 *         The Landscape is modified in the editor viewport, NOT at runtime.
 *
 * V1 REQUIREMENT: The user must create a Landscape actor manually in the editor
 *                 (Modes > Landscape > Manage > New Landscape) BEFORE calling
 *                 BakeToLandscape(). Auto-creation is NOT supported in V1 because
 *                 spawning ALandscapeProxy directly is incorrect in UE5 — proxies
 *                 are created by the landscape streaming system, not user code.
 *
 * Pipeline:
 *  1. Find existing ALandscapeProxy in current level (error if none found)
 *  2. Convert Cell.Height [0,1] to Landscape heightmap uint16 values [1024..64512]
 *  3. Write into Landscape heightmap via FLandscapeEditDataInterface
 *  4. Regenerate collision/navmesh if requested
 *
 * NOTE: Requires Editor module; Runtime module stores this header only.
 *       Actual bake call lives in EditorSubsystem or Editor module.
 */
class RTSMAPFORGERUNTIME_API FRTSLandscapeBaker
{
public:
    // Editor-only bake: writes Grid height data into an existing Landscape proxy.
    // Returns false with a descriptive log message if no landscape is found in World.
    static bool BakeToLandscape(const FRTSGrid& Grid, UWorld* World, bool bRegenerateCollision = true);

    // Utility: converts normalized height [0,1] to Landscape uint16 range [1024, 64512]
    static uint16 HeightToLandscape(float NormalizedHeight);

private:
    // FIX Minor: FindOrCreateLandscape() replaced by FindExistingLandscape().
    // V1 does not auto-create landscapes — user must create one manually.
#if WITH_EDITOR
    static ALandscapeProxy* FindExistingLandscape(UWorld* World);
    static bool WriteHeightmap(ALandscapeProxy* Landscape, const FRTSGrid& Grid);
#endif
};
