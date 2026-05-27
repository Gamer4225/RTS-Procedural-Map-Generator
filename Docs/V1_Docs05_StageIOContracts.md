# RTS MapForge — Stage Input/Output Contracts

## Purpose

As the pipeline grows beyond 18 stages, **implicit data dependencies** become the primary source of bugs. This document defines explicit contracts for each stage: what it reads, what it writes, and what it guarantees.

**Rule:** A stage may ONLY write to fields it owns. It may read from fields written by earlier stages. NEVER write to fields "owned" by other stages.

---

## Data Ownership Matrix

| Field/Structure | Owner Stage | Written By | Read By |
|---|---|---|---|
| `FRTSGrid::Cells` array allocation | Stage 2 | Stage 2 | All subsequent |
| `Cell.Height` | Stage 3 | Stage 3, Stage 3b, Stage 6 | Stage 4, Stage 6, Stage 7, Stage 8, tactical |
| `Cell.Slope` | Stage 3 | Stage 3 | Stage 4, Stage 11 |
| `Cell.bWater` | Stage 4 | Stage 4, Stage 6 | Stage 7, Stage 12, validation |
| `Cell.bCliff` | Stage 4 | Stage 4 | Stage 7, Stage 12, tactical |
| `Cell.bWalkable` | Stage 4 | Stage 4, Stage 6b | Stage 7, Stage 8, Stage 10, Stage 12 |
| `Cell.bBuildable` | Stage 4 | Stage 4, Stage 6b | Stage 8, Stage 10b |
| `Cell.MovementCostMultiplier` | Stage 4 | Stage 4, Stage 6b | Stage 12 (A*) |
| `Cell.BiomeID` | Stage 5 | Stage 5 | Stage 11, debug render |
| `Cell.RegionID` | Stage 7 | Stage 7 | Stage 8, Stage 10, Stage 11 |
| `Cell.TacticalZone` | Stage 8-11 | Stage 8 (Base), Stage 9 (Exp), Stage 10 (Choke), Stage 10b (Resource), Stage 11 (HighGround/Open) | Stage 13, Stage 14, debug render |
| `Cell.StrategicValue` | Stage 6, 11 | Stage 6 (river barriers), Stage 11 (contextual) | Stage 14, Stage 15 |
| `Cell.ResourceValue` | Stage 10b | Stage 10b | Stage 15, validation |
| `Cell.ControlValue` | Stage 13 | Stage 13 | Stage 14, debug render |
| `Cell.VisibilityScore` | Stage 14 | Stage 14 | Debug render |
| `Cell.ExposureScore` | Stage 14 | Stage 14 | Debug render |
| `FRTSMapMetadata::Bases` | Stage 8 | Stage 8 | Stage 9, 10, 12, 13, validation |
| `FRTSMapMetadata::Expansions` | Stage 9 | Stage 9 | Stage 10b, 12, validation |
| `FRTSMapMetadata::Chokes` | Stage 10 | Stage 10 | Stage 11, validation |
| `FRTSMapMetadata::RegionSizes` | Stage 7 | Stage 7 | Validation, debug |

---

## Per-Stage Contracts

### Stage 1: Seed Init
- **Input:** `URTSGenerationSettings::Seed`, `bRandomSeed`
- **Output:** `UFRTSSeedManager` initialized, `FRTSNoiseGenerator` permutation table ready
- **Guarantee:** All subsequent randomness is deterministic for same seed

### Stage 2: Grid Allocation
- **Input:** `GridWidth`, `GridHeight`, `CellSize`
- **Output:** `FRTSGrid` allocated; all cells zero-initialized with `GridCoord`, `WorldPosition`
- **Guarantee:** `Cells.Num() == Width * Height`; `IsValidCoord` true for all cells

### Stage 3: Heightmap Generation (FBM)
- **Input:** `FRTSGrid` (allocated), `Settings` (FBM params), `FRTSNoiseGenerator` (seeded)
- **Output:** `Cell.Height` normalized [0,1], `Cell.Slope` computed
- **Guarantee:** Heights are deterministic for same seed; slopes are world-space radians

### Stage 3b: Radial Falloff (Optional)
- **Input:** `Cell.Height`, `Grid` dimensions
- **Output:** `Cell.Height` modified by `h' = h - k*(d/r)^n`
- **Guarantee:** Edge heights are lower; center is preserved relative to edges

### Stage 4: Terrain Classification
- **Input:** `Cell.Height`, `Cell.Slope`, `WaterLevel`, implicit cliff thresholds
- **Output:** `bWater`, `bCliff`, `bWalkable`, `bBuildable`, `MovementCostMultiplier`
- **Guarantee:** 
  - `bWater` → `!bWalkable && !bBuildable`
  - `bCliff` → `!bWalkable && !bBuildable`
  - `bBuildable` → `bWalkable` (subset)

### Stage 5: Biome Assignment
- **Input:** `Cell.Height`, `Biomes[]`, `SeedManager`
- **Output:** `Cell.BiomeID`
- **Guarantee:** Every cell has a biome; biome count ≤ `Biomes.Num()`

### Stage 6: River Generation
- **Input:** `Cell.Height`, `MountainLevel`, `WaterLevel`, `SeedManager`
- **Output:** `Cell.bWater` (new water cells), `Cell.Height` (carved), `Cell.StrategicValue` (barrier bonus)
- **Guarantee:** Rivers trace from peaks to edges/existing water; 2-3 cell width; deterministic paths

### Stage 6b: Reclassify After Rivers
- **Input:** `Cell.bWater` (post-river)
- **Output:** `bWalkable`, `bBuildable`, `MovementCostMultiplier`
- **Guarantee:** All water cells are unwalkable; traversal state is consistent

### Stage 6c: Water Connectivity Validation
- **Input:** `Cell.bWater`, `FRTSGrid` dimensions
- **Output:** Validation issues (isolated water/land warnings)
- **Guarantee:** No isolated land pockets break base placement; warns on disconnected water

### Stage 7: Region Detection
- **Input:** `Cell.bWalkable`
- **Output:** `Cell.RegionID`, `FRTSMapMetadata::RegionSizes`
- **Guarantee:** Every walkable cell has a non-negative `RegionID`; contiguous walkable areas share ID

### Stage 8: Base Placement
- **Input:** `Cell.bWalkable`, `Cell.bBuildable`, `Cell.RegionID`, `NumPlayers`, `SymmetryStrength`
- **Output:** `FRTSMapMetadata::Bases`, `Cell.TacticalZone = MainBase`
- **Guarantee:** 
  - Bases are on walkable, buildable, large-region cells
  - 2p symmetry: 180° rotation if `SymmetryStrength >= 0.8`
  - Minimum distance enforced

### Stage 9: Expansion Placement
- **Input:** `Cell.bBuildable`, `Metadata.Bases[]`, `NumExpansions`
- **Output:** `FRTSMapMetadata::Expansions`, `Cell.TacticalZone = NatExpansion/ContestedExp`
- **Guarantee:** Expansions are buildable; risk score computed; natural expansions are closer, contested are mid-map

### Stage 10: Choke Detection
- **Input:** `Cell.RegionID`, `Cell.bWalkable`
- **Output:** `FRTSMapMetadata::Chokes`, `Cell.TacticalZone = ChokePoint`
- **Guarantee:** Choke width measured per boundary; stored in `MinChokeWidth–MaxChokeWidth` range

### Stage 10b: Resource Placement
- **Input:** `Cell.bWalkable`, `Cell.bBuildable`, `Metadata.Expansions[]`, `Metadata.Bases[]`
- **Output:** `Cell.ResourceValue`, `Cell.TacticalZone = ResourceCluster` (if score > 0.5)
- **Guarantee:** Resources are on valid terrain; spaced by Poisson distance; deterministic positions

### Stage 11: Tactical Zone Classification
- **Input:** `Cell` state from all prior stages; `Metadata` structures
- **Output:** `Cell.TacticalZone` for ALL unclassified cells
- **Guarantee:** **Priority enforcement** — earlier assignments (Base, Choke, Expansion, Resource) are NEVER overwritten. Only fills `Unclassified` cells with `HighGround` or `OpenBattlefield`.

### Stage 12: A* Pathfinding
- **Input:** `Cell.bWalkable`, `Cell.MovementCostMultiplier`, `Metadata.Bases[]`
- **Output:** Rush distances computed; path costs validated
- **Guarantee:** A* finds path or returns -1 (unreachable); costs accumulated from `MovementCostMultiplier`

### Stage 13: Influence Maps
- **Input:** `Metadata.Bases[]`, `Grid` dimensions
- **Output:** `Cell.ControlValue` [-1, +1], `Cell.StrategicValue` (may augment)
- **Guarantee:** Inverse-square decay; sum-normalized; deterministic for same base positions

### Stage 14: Heatmaps
- **Input:** `Cell.ControlValue`, `Cell.bWalkable`, path data from Stage 12
- **Output:** `Cell.VisibilityScore`, `Cell.ExposureScore`
- **Guarantee:** Combat heat based on contested zones (control near zero)

### Stage 15: Strategic Scoring
- **Input:** `Metadata` (all fields), `Cell.ResourceValue`, `Cell.TacticalZone`
- **Output:** `FRTSValidationResult::OverallScore`
- **Guarantee:** Score is [0, 100]; balance, rush, choke, diversity components

### Stage 16: Validation
- **Input:** All grid data, all metadata, scoring result
- **Output:** `FRTSValidationResult::Issues[]`, `bPassed`
- **Guarantee:** 
  - CRITICAL issues trigger retry
  - WARNING issues accepted but flagged
  - `bPassed == !HasCriticalFailure()`

---

## Contract Enforcement Rules

1. **Never Overwrite TacticalZone if already set.** Stage 11 is the ONLY stage allowed to write `TacticalZone` on previously-unclassified cells. Earlier stages (8, 9, 10, 10b) write directly and are protected.

2. **Height is owned by terrain stages.** Only Stages 3, 3b, 6 (river carving), and 6 smoothing may modify `Cell.Height`.

3. **Traversal flags are owned by Stages 4 and 6b.** No other stage may modify `bWalkable`, `bBuildable`, `MovementCostMultiplier`.

4. **RegionID is owned by Stage 7.** Immutable after detection (re-detection requires resetting).

5. **ResourceValue is owned by Stage 10b.** No other stage may modify after placement.

6. **Metadata structures append-only.** Stages may ADD to `Metadata.Bases[]`, `Expansions[]`, `Chokes[]`. They may NEVER remove or reorder existing entries.

---

## What This Prevents

| Risk | Contract Prevention |
|---|---|
| Stage 11 overwriting Base zones | Priority rule: `Unclassified` only |
| River carving breaking RegionID | Stage 6 modifies `Height`/`bWater`, Stage 7 re-detects regions AFTER |
| Resource placement on invalid terrain | Stage 10b reads `bWalkable` + `bBuildable` from Stage 4/6b |
| Choke detection on stale regions | Stage 10 reads `RegionID` from Stage 7, which ran AFTER rivers |
| Validation passing broken maps | Pass 1 validates A* base-to-base AFTER all placement and traversal updates |
| Tactical zone ambiguity | Explicit priority hierarchy enforced in Stage 11 |

---

## Adding a New Stage (Checklist)

When adding Stage X between existing stages N and M:

- [ ] Document **input fields** (what earlier stages must have written)
- [ ] Document **output fields** (what this stage writes)
- [ ] Verify no **conflict with existing ownership** (don't write to Stage 4's traversal flags)
- [ ] Add **guarantee clause** (what downstream stages can depend on)
- [ ] Update **Data Ownership Matrix** above
- [ ] Add **automation test** proving deterministic output for same input

This document is version-controlled alongside the code. Update it before every stage addition.
