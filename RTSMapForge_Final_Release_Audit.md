# RTSMapForge — Final Production Readiness Audit

Generated: 2026-05-26

Post-fix validation summary:
- `get_errors` returned no errors for the edited runtime files and headers.
- Source sweeps no longer show stale landscape TODO / NOT IMPLEMENTED markers.
- Landscape bake is now implemented through the editor-side landscape edit interface.
- Overlay rendering now adapts its stride for large maps instead of doing full per-cell work.

**Critical Issues ✅**

No blocking compile or determinism defects remain in the audited slices.

The seed retry path now resolves seed once at pipeline entry, uses the effective retry seed for generation, stores that effective seed in metadata, and returns that effective seed to callers. The caller's settings are restored before exit, so `Generate()` no longer leaves hidden state behind.

Relevant files:
- [Source/RTSMapForgeRuntime/Private/Core/FRTSGenerationPipeline.cpp](RTSMapForge/Source/RTSMapForgeRuntime/Private/Core/FRTSGenerationPipeline.cpp)
- [Source/RTSMapForgeRuntime/Public/Core/FRTSGenerationPipeline.h](RTSMapForge/Source/RTSMapForgeRuntime/Public/Core/FRTSGenerationPipeline.h)

**High Priority Warnings ✅**

The prop spawner lifecycle is now hardened:
- HISM components are explicitly unregistered and destroyed.
- The manager actor is destroyed during cleanup.
- The manager actor is marked transient on spawn.
- `Deinitialize()` delegates to `ClearProps()` so teardown is centralized.

Relevant files:
- [Source/RTSMapForgeRuntime/Private/Spawning/URTSPropSpawner.cpp](RTSMapForge/Source/RTSMapForgeRuntime/Private/Spawning/URTSPropSpawner.cpp)
- [Source/RTSMapForgeRuntime/Public/Spawning/URTSPropSpawner.h](RTSMapForge/Source/RTSMapForgeRuntime/Public/Spawning/URTSPropSpawner.h)

**Marketplace Risks ✅**

The previous marketplace-facing blocker was the incomplete landscape bake path. That path is now implemented for editor builds using the landscape edit interface, and the stale V1.0 placeholder comments were removed from source.

Relevant files:
- [Source/RTSMapForgeRuntime/Private/Terrain/FRTSLandscapeBaker.cpp](RTSMapForge/Source/RTSMapForgeRuntime/Private/Terrain/FRTSLandscapeBaker.cpp)
- [Source/RTSMapForgeRuntime/Public/Terrain/FRTSLandscapeBaker.h](RTSMapForge/Source/RTSMapForgeRuntime/Public/Terrain/FRTSLandscapeBaker.h)

**Determinism Verification ✅**

Verified post-fix behavior:
- `Settings->ResolveSeed()` is called only once at pipeline entry.
- Generators receive an explicit resolved seed parameter.
- Retry iterations use an explicit effective seed derived from the original resolved seed.
- The returned seed and `OutMetadata.Seed` match the actual generation result.

Relevant files:
- [Source/RTSMapForgeRuntime/Private/Core/FRTSGenerationPipeline.cpp](RTSMapForge/Source/RTSMapForgeRuntime/Private/Core/FRTSGenerationPipeline.cpp)
- [Source/RTSMapForgeRuntime/Private/Terrain/FRTSHeightmapGenerator.cpp](RTSMapForge/Source/RTSMapForgeRuntime/Private/Terrain/FRTSHeightmapGenerator.cpp)

**Memory/Lifecycle Problems ✅**

The root-set and ownership issues addressed in the prior audit are now controlled in the fixed slices:
- `SeedManager` uses a scoped root guard during generation.
- `PreviewTexture` remains balanced with `AddToRoot()` / `RemoveFromRoot()`.
- The prop spawner teardown now cleans up its owned UObject/actor graph explicitly.

Relevant files:
- [Source/RTSMapForgeRuntime/Private/Core/FRTSGenerationPipeline.cpp](RTSMapForge/Source/RTSMapForgeRuntime/Private/Core/FRTSGenerationPipeline.cpp)
- [Source/RTSMapForgeEditor/Private/URTSMapForgeEditorSubsystem.cpp](RTSMapForge/Source/RTSMapForgeEditor/Private/URTSMapForgeEditorSubsystem.cpp)
- [Source/RTSMapForgeRuntime/Private/Spawning/URTSPropSpawner.cpp](RTSMapForge/Source/RTSMapForgeRuntime/Private/Spawning/URTSPropSpawner.cpp)

**Packaging Risks ✅**

Packaging risks tied to the landscape bake TODO and fake-success behavior have been removed in the audited source slices. The plugin/module split remains correct, and the editor-only bake path is now implemented without leaving a release-blocking placeholder.

Relevant files:
- [RTSMapForge.uplugin](RTSMapForge/RTSMapForge.uplugin)
- [Source/RTSMapForgeRuntime/Private/Terrain/FRTSLandscapeBaker.cpp](RTSMapForge/Source/RTSMapForgeRuntime/Private/Terrain/FRTSLandscapeBaker.cpp)

**Performance Risks ✅**

The editor overlay bottleneck was mitigated by adaptive stride sampling. Large grids no longer require a full per-cell render/update pass for both the overlay and the minimap bitmap path.

Relevant files:
- [Source/RTSMapForgeRuntime/Private/Visualization/FRTSDebugRenderer.cpp](RTSMapForge/Source/RTSMapForgeRuntime/Private/Visualization/FRTSDebugRenderer.cpp)

**Final Release Verdict ✅**

No source-level blockers remain in the audited areas.

Current state:
- Determinism seed handling is correct.
- Object lifetime / cleanup is explicit.
- Landscape bake is implemented for editor builds.
- Overlay rendering is throttled for large grids.

Remaining practical step before Fab submission:
- Run a full Unreal Editor build and a live bake/overlay smoke test in the target UE version you plan to ship.
