# RTS MapForge V1 — Milestone 1: Strategic Terrain Systems

## What Was Built

**Focus: Strategic terrain shaping, NOT more tooling.**

This milestone transforms "pure noise terrain" into **playable RTS battlefields** with:
- Rivers as traversal blockers and tactical separators
- Island-shaped maps via radial falloff
- Water connectivity validation (no broken maps)
- Strategic resource placement near expansions/high-ground
- All features immediately visible in debug overlays

---

## New Systems (6 Implemented)

### 1. River Generation (`FRTSRiverGenerator`)

**Design:** Intentionally simple gradient descent. NO fluid simulation, NO erosion, NO hydraulic models.

**Pipeline:**
```
Find local maxima above MountainLevel (peaks)
    ↓
Select N spaced source points (deterministic, Poisson-like via index shuffle)
    ↓
Trace downhill: at each step, move to lowest 8-neighbor
    ↓
Mark path cells as water, carve height below WaterLevel
    ↓
Carve river beds: ensure water cells are below water level
    ↓
Smooth riverbanks: raise adjacent land to create natural edges
    ↓
Update traversal: water = unwalkable, unbuildable
    ↓
Increase StrategicValue: rivers are natural tactical barriers
```

**Why this matters for RTS:**
- Rivers create **choke points** where crossings are narrow
- Rivers **separate player territories** early-game
- Rivers **block rushes** until amphibious/air units
- Rivers **define expansion corridors** (cross here, expand there)

**Parameters:**
- `NumRivers = clamp(NumPlayers * 2, 2, 6)`
- Sources spaced by `0.15 * min(W, H)` cells
- Max trace steps: `W + H`
- Bank height: `WaterLevel + 0.02–0.08`
- River bed: `WaterLevel - 0.05`

**Integration into gameplay data:**
```cpp
if (Cell.bWater) {
    Cell.bWalkable = false;
    Cell.bBuildable = false;
    Cell.MovementCostMultiplier = 0.0f;
    Cell.StrategicValue += 0.3f; // Tactical barrier
}
```

---

### 2. Radial Island Falloff (`FRTSHeightmapGenerator::ApplyRadialFalloff`)

**Formula:** `h' = h - k * (d/r)^n`
- `d` = distance from center
- `r` = max radius (map diagonal/2)
- `k` = falloff strength (default 0.3)
- `n` = smoothstep curve (cubic)

**Why this matters for RTS:**
- Creates **natural island maps** — no awkward map edges
- **Cleaner strategic flow** — players fight toward center
- **Readable coastlines** — water defines natural territory boundaries
- **Easier AI pathfinding** — no infinite open edges

**Applied in pipeline Stage 3b, after heightmap, before classification.**

---

### 3. Terrain Carving (integrated in river generation)

**River bed carving:** All water cells pushed to `WaterLevel - 0.05`
**Bank smoothing:** Adjacent land cells raised to `WaterLevel + 0.02–0.08`

This creates:
- **Visually distinct rivers** (not just flat water)
- **Natural crossing difficulty** (banks are slightly elevated)
- **Terrain readability** — players can SEE where rivers are

---

### 4. Water Connectivity Validation (`FRTSWaterConnectivityValidator`)

**Problem:** Random noise + rivers can create:
- Isolated water pockets (ponds in middle of land)
- Land regions completely surrounded by water (unreachable)
- Broken pathfinding where A* cannot find routes

**Validation checks:**
1. **Flood-fill from all water on map edges** → mark all water reachable from edge
2. **Flag isolated water cells** as warnings (not critical — ponds are okay)
3. **Count isolated land regions** (land completely surrounded by water, no edge contact)
4. **Warn if isolated land regions exist** — these can break base placement / pathfinding

**Result:** Map either passes, or flags specific water/land connectivity issues for retry.

---

### 5. Resource Placement (`FRTSResourcePlacer`)

**Design:** Deterministic Poisson-disk scatter. Resources are **strategic objectives**, not random loot.

**Scoring formula per cell:**
```
Score = 0.4 * ProximityToNearestExpansion + 0.2 * HighGroundBonus + 0.1 * DeterministicJitter
```

**Placement rules:**
- Only on walkable, buildable, non-water, non-cliff cells
- Minimum spacing: `0.06 * max(W, H)` cells
- Max resources: `clamp(W*H / 200, 8, 64)` (scales with map size)
- **High ground bonus:** resources on elevated terrain = harder to contest
- **Expansion proximity:** resources near expansions = early-game economy tension
- **Deterministic jitter:** prevents perfect grid patterns

**Integration into gameplay data:**
```cpp
Cell.ResourceValue = Score;        // 0..1 richness
if (Score > 0.5) {
    Cell.TacticalZone = ResourceCluster;  // Strategic overlay shows gold/resource color
}
```

---

### 6. Texture Reuse (Editor Subsystem Fix)

**Problem:** `UTexture2D::CreateTransient()` every generation = GC churn, editor hitching.

**Fix:** Reuse existing texture if dimensions match. Only recreate when grid size changes.

```cpp
void CreatePreviewTexture(int32 Width, int32 Height) {
    if (PreviewTexture && PreviewTexture->GetSizeX() == Width && PreviewTexture->GetSizeY() == Height) {
        return; // REUSE — no recreation
    }
    // ... only create new when needed
    PreviewTexture->AddToRoot(); // Prevent GC during generation cycles
}
```

---

## Updated Pipeline (18 Stages)

```
Stage  1: Seed Init           → FRandomStream + Perlin permutation table
Stage  2: Grid Alloc          → flat TArray<FRTSCell>[W*H]
Stage  3: Heightmap (FBM)     → Perlin octaves → normalized height
Stage 3b: Radial Falloff       → Island shaping: h' = h - k*(d/r)^n
Stage  4: Terrain Classify     → Water, Cliff, Buildable
Stage  5: Biome Assignment     → Voronoi seeds per biome asset
Stage  6: River Generation     → Gradient descent from peaks; carve + smooth
Stage 6b: Reclassify Rivers   → Update traversal/strategic after water carving
Stage 6c: Water Validation     → Ensure connectivity; no isolated land pockets
Stage  7: Region Detection     → BFS flood fill walkable regions
Stage  8: Base Placement       → 180° symmetry (2p) + Poisson fallback
Stage  9: Expansion Placement  → Risk score = dist_enemy / dist_base
Stage 10: Choke Detection      → Region boundary width analysis
Stage10b: Resource Placement   → Poisson-disk near expansions/high-ground
Stage 11: Tactical Zones       → Decision tree classification
Stage 12: A* Pathfinding       → Octile heuristic; rush distances
Stage 13: Influence Maps       → Inverse-square control values
Stage 14: Heatmaps             → Combat/traversal density
Stage 15: Strategic Scoring   → Balance, Rush, Choke, Overall
Stage 16: Validation           → 6-pass checks → retry or accept
```

---

## How to Verify (In Editor)

### 1. Generate with Default Settings
```
Seed: 1337
Size: 256×256
Players: 2
Symmetry: 1.0
```

### 2. Check Heightmap Overlay
- Islands should be visible (coastlines on edges)
- Center should be higher than edges

### 3. Switch to Water & Cliffs Overlay
- **Rivers should appear** as thin blue lines from mountains toward coast
- Rivers should merge naturally (tributaries)
- Banks should be visible as thin gray edges
- No isolated blue ponds in land (or minimal = warning)

### 4. Switch to Tactical Zones Overlay
- Green = Main Bases (2 dots, symmetric)
- Light green = Expansions
- Red = Choke Points (often where rivers cross paths)
- Gold = Resource Clusters (near expansions)
- Brown = High Ground
- Gray = Open Battlefield

### 5. Switch to Walkable Overlay
- Blue rivers should be RED (blocked)
- Base areas should be GREEN
- All bases should connect via some green path

### 6. Switch to Influence Map Overlay
- Red = Player A territory
- Blue = Player B territory
- Purple/white = Contested (where rivers create barriers)
- Rivers should create visible influence boundaries

### 7. Export JSON
- Check `Saved/RTSMapForge_LastExport.json`
- Verify `Chokes` count is > 0 (rivers create chokes)
- Verify `Passed: true`

### 8. Determinism Check
- Regenerate with SAME seed
- Minimap must be pixel-identical
- Rivers must trace identical paths
- Resource clusters must be in identical positions

---

## Known Limitations (V1, Acceptable)

| Limitation | Reason | When to Fix |
|---|---|---|
| Rivers are thin (1-cell wide) | Gradient descent traces single path | V1.5: widen with perpendicular carve |
| No bridge generation | Not yet needed for V1 scoring | V2: explicit crossing placement |
| Resource placement doesn't mirror | Only placed near expansions, which are symmetric | V2: enforce symmetry |
| No actual resource prefab spawning | Only metadata markers | V1.5: bake to level with StaticMesh |
| Falloff strength hardcoded to 0.3 | Sufficient for island feel | V1.5: expose in URTSGenerationSettings |
| Water validation only warns | Isolated ponds don't break gameplay | V2: auto-merge small water bodies |

---

## Architecture Principles Enforced

1. **Rivers are gameplay first, visuals second** — They block movement, create chokes, separate territories before any mesh is spawned.
2. **Validation validates rivers** — Water connectivity checks ensure rivers don't accidentally isolate bases.
3. **Resources are strategic objectives** — Placement score uses expansion proximity and high ground, not random scatter.
4. **Texture reuse prevents GC** — Editor stays responsive during rapid regeneration cycles.
5. **Determinism is maintained** — All new systems use `SeedManager` exclusively. Same seed = identical rivers, resources, falloff.

---

## Next Milestone (V1.5): Terrain Polish & Bake

| Task | Description |
|---|---|
| **Widen rivers** | Trace with 2-3 cell width + perpendicular carve |
| **Bridge/crossing detection** | Find narrow river crossings = choke points |
| **Exposed falloff strength** | Add `FalloffStrength` to `URTSGenerationSettings` |
| **Default Biome Assets** | Create Data Assets: Temperate, Desert, Snow, Lava |
| **Biome-specific resource tables** | Desert = oil, Snow = crystals, etc. |
| **Landscape heightmap bake** | Write generated heights to UE `ALandscapeProxy` |
| **Static mesh spawning** | Place trees/rocks/resources as actors from metadata |
| **Resource symmetry** | Mirror resource clusters for 2p maps |

The terrain now has **strategic grammar**: rivers create barriers, islands define edges, resources create objectives, and all of it is visible in the editor overlays.
