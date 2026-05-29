# RTSMapForge — Fix Changelog
**Date:** 2026-05-27  
**Scope:** All critical compile errors, design problems, and minor issues resolved.

---

## 🔴 Critical Fixes (Compile Errors / Crashes)

### Bug 1 — `FRTSNoiseGenerator` missing include in pipeline header
**File:** `Public/Core/FRTSGenerationPipeline.h`  
**Fix:** Added `#include "Terrain/FRTSNoiseGenerator.h"`.  
The pipeline had `FRTSNoiseGenerator NoiseGen` as a private member but no include for it — would fail to compile.

---

### Bug 2 — `"InstancedPlacements"` is not a valid UE5.3/5.4 module
**File:** `RTSMapForgeRuntime.Build.cs`  
**Fix:** Removed `"InstancedPlacements"` from `PublicDependencyModuleNames`.  
`HierarchicalInstancedStaticMeshComponent` lives in the `"Engine"` module, already listed. Also added `"LandscapeEditor"` under `bBuildEditor` so `FLandscapeEditDataInterface` resolves correctly.

---

### Bug 3 — `WorkspaceMenuStructure` double-include fails on most UE installs
**Files:** `RTSMapForgeEditor.Build.cs`, `Private/RTSMapForgeEditorModule.cpp`  
**Fix:** Removed both `WorkspaceMenuStructure` includes from the `.cpp` file and removed the module from `Build.cs`. It was never actually used — the tab spawner and toolbar button work without it.

---

### Bug 4 — `FRTSRiverGenerator.h` in wrong folder
**File:** `Public/Terrain/FRTSRiverGenerator.h` (moved from `Public/Strategic/`)  
**Fix:** Header relocated to `Public/Terrain/` to match its `.cpp` at `Private/Terrain/`. Updated the pipeline include to `"Terrain/FRTSRiverGenerator.h"`. Any other file that included the old `"Strategic/"` path must be updated to match.

---

### Bug 5 — `ResolveSeed()` used `FMath::Rand()` violating determinism rule
**File:** `Private/Core/URTSGenerationSettings.cpp`  
**Fix:** Replaced `FMath::Rand()` (global C `rand()`, not controlled by any `FRandomStream`) with `FPlatformTime::Cycles64()`. The high-resolution timer provides sufficient entropy without corrupting global rand state. The resolved seed is still stored once by the pipeline and fully reproducible from that point onwards.

---

### Bug 6 — `NewObject` missing template type for `UFRTSSeedManager`
**File:** `Private/Core/FRTSGenerationPipeline.cpp`  
**Fix:** Changed `NewObject(GetTransientPackage(), ...)` to `NewObject<UFRTSSeedManager>(GetTransientPackage(), ...)`.

---

## 🟡 Design Problem Fixes (Editor Freezes / Logic Errors)

### Problem 1 — `GenerateMap()` froze the editor thread
**Files:** `Public/URTSMapForgeEditorSubsystem.h`, `Private/URTSMapForgeEditorSubsystem.cpp`, `Private/SRTSMapGeneratorWindow.cpp`, `Public/SRTSMapGeneratorWindow.h`  
**Fix:** `GenerateMap()` now dispatches `FRTSGenerationPipeline::Generate()` to `ENamedThreads::AnyBackgroundThreadNormalTask` via `AsyncTask`. All UObject writes (`CurrentGrid`, textures, metadata) are marshalled back to the game thread via a nested `AsyncTask(GameThread, ...)`.
- Added `std::atomic<bool> bIsGenerating` guard to prevent double-generation.
- Added `IsGenerating()` UFUNCTION for Blueprint/UI polling.
- The Generate button in `SRTSMapGeneratorWindow` is disabled and shows "Generating…" while the job runs.
- `Tick()` detects async completion via the `bWasGenerating` → `!IsGenerating()` transition and calls `RefreshReadouts()` automatically.

---

### Problem 2 — A* open set O(n) linear scan
**Files:** `Public/Pathfinding/FRTSAStarSolver.h`, `Private/Pathfinding/FRTSAStarSolver.cpp`  
**Fix:**
- Best-node selection replaced with `Algo::MinElementBy` (same O(n) complexity, tighter constant, cleaner intent).
- Added a `TMap<int32, float> BestG` for per-index best-G tracking. This eliminates the inner O(n) linear scan over the open set when checking for duplicate nodes — the duplicate check is now O(1).
- Added `Open.Reserve(512)` and `Closed.Reserve(1024)` to reduce reallocation overhead.
- V2 target documented: replace `TArray<FAStarNode>` with a binary heap for true O(log n) extraction.

---

### Problem 3 — Test file compiled unconditionally (Fab certification risk)
**File:** `Private/Tests/RTSMapForgeDeterminismTests.cpp`  
**Fix:** Entire file wrapped in `#if WITH_DEV_AUTOMATION_TESTS / #endif`. Tests now only compile in Editor + non-shipping configurations and never ship with the packaged plugin.

---

## 🟢 Minor Fixes

### Rush distances not stored in metadata (double A* compute)
**Files:** `Public/Data/FRTSMapMetadata.h`, `Private/Pathfinding/FRTSAStarSolver.cpp`, `Private/Core/FRTSGenerationPipeline.cpp`  
**Fix:** Added `TMap<int64, float> RushDistances` to `FRTSMapMetadata`. Stage 12 now populates it (key = packed base-pair int64). Validation stages 16/16b can read cached costs without re-running A*.

### `SpawnProps()` missing `RF_Transient` on manager actor
**File:** `Private/Spawning/URTSPropSpawner.cpp`  
**Fix:** `SpawnProps()` now sets `RF_Transient` on the manager actor, consistent with `SpawnResourceNodes()`. Prevents unintentional serialization to level.

### `FRTSLandscapeBaker` — `ALandscapeProxy` auto-create removed
**Files:** `Public/Terrain/FRTSLandscapeBaker.h`, `Private/Terrain/FRTSLandscapeBaker.cpp`  
**Fix:** `FindOrCreateLandscape()` replaced by `FindExistingLandscape()`. V1 only bakes to a landscape the user creates manually. Attempting to `SpawnActor<ALandscapeProxy>` directly is incorrect in UE5 (proxies are managed by the streaming system). A clear `UE_LOG(Error, ...)` message guides the user to create a landscape first.

### `ExportMetadataToJSON()` — manual JSON string replaced with `TJsonWriter`
**File:** `Private/URTSMapForgeEditorSubsystem.cpp`  
**Fix:** Replaced hand-built string concatenation with `TJsonWriterFactory<>::Create` + typed `WriteValue`/`WriteArray` calls. Now correctly escapes all string values and includes a full issues array.

### V1 prop placement comment clarified
**File:** `Private/Spawning/URTSPropSpawner.cpp`  
**Fix:** Log message updated to clearly state "V1 — mesh assets not yet wired; no HISM instances placed" so testers understand no visible props in V1 is by design, not a bug.

---

## Files Changed Summary

| File | Change |
|------|--------|
| `RTSMapForgeRuntime.Build.cs` | Removed `InstancedPlacements`; added `LandscapeEditor` |
| `RTSMapForgeEditor.Build.cs` | Removed `WorkspaceMenuStructure` |
| `Public/Core/FRTSGenerationPipeline.h` | Added `FRTSNoiseGenerator.h` include |
| `Private/Core/FRTSGenerationPipeline.cpp` | Fixed `NewObject<>` template; corrected River include path |
| `Private/Core/URTSGenerationSettings.cpp` | Replaced `FMath::Rand()` with `FPlatformTime::Cycles64()` |
| `Public/Data/FRTSMapMetadata.h` | Added `RushDistances` TMap |
| `Public/Pathfinding/FRTSAStarSolver.h` | Updated doc comment |
| `Private/Pathfinding/FRTSAStarSolver.cpp` | `Algo::MinElementBy` + BestG map + rush distance storage |
| `Private/Spawning/URTSPropSpawner.cpp` | Added `RF_Transient` to `SpawnProps()` manager actor |
| `Public/Terrain/FRTSLandscapeBaker.h` | Replaced `FindOrCreateLandscape` with `FindExistingLandscape` |
| `Private/Terrain/FRTSLandscapeBaker.cpp` | Removed auto-create; V1 requires user-created landscape |
| `Public/Terrain/FRTSRiverGenerator.h` | Moved from `Strategic/` → `Terrain/` |
| `Public/URTSMapForgeEditorSubsystem.h` | Added `bIsGenerating`, `IsGenerating()` |
| `Private/URTSMapForgeEditorSubsystem.cpp` | Async `GenerateMap()`; `TJsonWriter` export |
| `Public/SRTSMapGeneratorWindow.h` | Added `bWasGenerating` tracking |
| `Private/SRTSMapGeneratorWindow.cpp` | Disabled button while generating; async-aware Tick |
| `Private/RTSMapForgeEditorModule.cpp` | Removed `WorkspaceMenuStructure` includes |
| `Private/Tests/RTSMapForgeDeterminismTests.cpp` | Wrapped in `#if WITH_DEV_AUTOMATION_TESTS` |
