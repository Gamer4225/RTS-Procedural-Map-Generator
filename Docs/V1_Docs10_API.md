# RTS MapForge — API Reference

## Blueprint API

### URTSMapForgeSubsystem (Game Instance / Runtime)

Accessible from any Blueprint via `Get Game Instance` → `Get Subsystem (URTSMapForgeSubsystem)`.

#### Generate Map

```
GenerateMap(Settings: URTSGenerationSettings)
```

Generates a complete RTS battlefield. **Synchronous** (blocks until complete). For editor-time generation, use `URTSMapForgeEditorSubsystem`.

#### Query Cell Data

```
GetCellAtWorldLocation(Location: Vector) → FRTSCell
GetZoneAtLocation(Location: Vector) → ERTSTacticalZone
```

Query any world position for grid cell data and tactical zone classification.

#### Get Scores

```
GetMapMetrics() → FRTSStrategicMetrics
GetCombatHeatAt(Location: Vector) → float
```

Retrieve overall strategic scoring and localized combat heat values.

#### Export

```
ExportMetadataToJSON(FilePath: String) → bool
```

Saves map metadata to JSON file.

---

### URTSMapForgeEditorSubsystem (Editor Only)

Accessible in editor utility Blueprints and Slate widgets via `GEditor->GetEditorSubsystem<URTSMapForgeEditorSubsystem>()`.

#### Generate and Preview

```
GenerateMap(Settings: URTSGenerationSettings)
SetOverlayMode(Mode: ERTSDebugOverlayMode)
CycleOverlayMode()
GetPreviewTexture() → UTexture2D
```

Generate a map and update the minimap preview texture. Change overlay mode to visualize different strategic layers.

#### Export

```
ExportMetadataToJSON()
```

Exports to `Saved/RTSMapForge_LastExport.json`.

---

## Data Structures

### FRTSCell (Per-Grid-Cell Data)

| Field | Type | Blueprint? | Description |
|---|---|---|---|
| GridCoord | FVector2D | Read | Grid (X,Y) position |
| WorldPosition | FVector | Read | World-space location |
| Height | float | — | Normalized terrain height [0,1] |
| Slope | float | — | Slope angle in radians |
| bWalkable | bool | Read | Can units traverse? |
| bBuildable | bool | Read | Can structures be built? |
| bWater | bool | Read | Is this water? |
| bCliff | bool | Read | Is this a cliff? |
| MovementCostMultiplier | float | Read | 1.0 = normal, 0.0 = blocked |
| StrategicValue | float | Read | Computed tactical importance [0,1] |
| ResourceValue | float | Read | Resource richness [0,1] |
| ControlValue | float | Read | Influence map [-1, +1] |
| TacticalZone | ERTSTacticalZone | Read | Zone classification |
| CoverValue | float | Read | Ambush/cover potential [0,1] |
| VisibilityScore | float | Read | Line-of-sight quality [0,1] |

### FRTSMapMetadata (Map Summary)

| Field | Type | Description |
|---|---|---|
| Seed | int64 | Deterministic generation seed |
| GridWidth | int32 | Map width in cells |
| GridHeight | int32 | Map height in cells |
| CellSize | float | World units per cell (cm) |
| Bases | TArray<FRTSBaseInfo> | Starting positions |
| Expansions | TArray<FRTSExpansionInfo> | Economic expansion points |
| Chokes | TArray<FRTSChokeInfo> | Bottlenecks and crossings |

### FRTSValidationResult (Quality Check)

| Field | Type | Description |
|---|---|---|
| bPassed | bool | Overall pass/fail |
| OverallScore | float | 0-100 strategic quality score |
| Issues | TArray<FRTSValidationIssue> | Detailed issue list |
| RetryCount | int32 | How many retries before pass |

### URTSGenerationSettings (Configuration)

| Category | Property | Default | Range |
|---|---|---|---|
| Map | GridWidth / GridHeight | 256 | 16–2048 |
| Map | CellSize | 200.0 cm | 10+ |
| Seed | Seed | 0 | Any int64 |
| Seed | bRandomSeed | true | Boolean |
| Terrain | FBMOctaves | 6 | 1–16 |
| Terrain | FBMPersistence | 0.5 | 0–1 |
| Terrain | FBMLacunarity | 2.0 | 1–4 |
| Terrain | TerrainScale | 1.0 | 0.001+ |
| Terrain | WaterLevel | 0.25 | 0–1 |
| Terrain | MountainLevel | 0.75 | 0–1 |
| Strategic | NumPlayers | 2 | 2–12 |
| Strategic | MinRushDistance | 0.35 | 0–1 |
| Strategic | SymmetryStrength | 1.0 | 0–1 |
| Strategic | NumExpansions | 3 | 0–16 |
| Validation | MinChokeWidth | 3 | 1+ |
| Validation | MaxChokeWidth | 12 | 1+ |
| Validation | MinBuildableRatio | 0.25 | 0–1 |
| Validation | MaxFairnessError | 0.10 | 0–1 |
| Validation | MinAcceptableScore | 65.0 | 0–100 |

---

## Debug Overlay Modes (ERTSDebugOverlayMode)

| Mode | Use Case |
|---|---|
| None | Reset/clear |
| Heightmap | Verify terrain silhouette |
| WaterCliff | Check traversal barriers |
| Walkable | Quick connectivity test |
| Buildable | Verify base placement |
| Slope | Cliff threshold tuning |
| Regions | Flood-fill debug |
| Biomes | Voronoi cell debug |
| TacticalZones | Strategic grammar review |
| InfluenceMap | Territorial balance |
| CombatHeat | Battle prediction |
| ChokePoints | Bottleneck identification |

---

## C++ Extension Points

### Adding a New Generation Stage

1. Add stage method to `FRTSGenerationPipeline.h`:
   ```cpp
   void StageXX_MyNewStage(FRTSGrid& Grid, ...);
   ```

2. Implement in `.cpp`, following I/O contract.

3. Call from `Generate()` between appropriate existing stages.

4. Add automation test proving deterministic output.

5. Update `Docs/Stage_IO_Contracts.md`.

### Custom Biome

Create a `URTSBiomeAsset` Data Asset in editor:
- Set terrain rules, movement costs, resource types
- Assign debug color
- Add to `URTSGenerationSettings::Biomes` array

### Custom Validation Rule

Add a new pass in `FRTSValidationPipeline::Validate()`:
```cpp
void PassN_MyRule(...) const;
```

Use `OutResult.Issues.Add()` with appropriate `ERTSValidationSeverity`.

---

## Performance Profiling

Enable profiling in `URTSGenerationSettings` or call from C++:

```cpp
FRTSPerformanceProfiler Profiler;
Profiler.BeginPipeline(256, 256);
// ... run pipeline ...
Profiler.EndPipeline();
FString Report = Profiler.GenerateReport();
UE_LOG(LogTemp, Log, TEXT("%s"), *Report);
```

Benchmark targets:
- 128×128: < 0.5 sec
- 256×256: < 2 sec
- 512×512: < 8 sec
- 1024×1024: < 25 sec
