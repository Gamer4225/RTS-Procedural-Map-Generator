# RTS MapForge — Quick Start Guide

## Installation

1. Copy the `RTSMapForge` folder into your Unreal Engine 5 project's `Plugins/` directory.
2. Regenerate project files (right-click `.uproject` → Generate Visual Studio project files).
3. Build your project.
4. Launch the editor.

## First Map Generation

1. In the Level Editor toolbar, click the **"MapForge"** button (next to Play).
2. The **RTS MapForge** dockable window opens.
3. Set parameters:
   - **Seed**: `1337` (use same seed for reproducible maps)
   - **Size**: `256` (256×256 cells)
   - **Players**: `2`
   - **Symmetry**: `1.0` (full 180° symmetry for 2 players)
4. Click **GENERATE MAP**.
5. Observe the **minimap preview** update to show terrain.

## Understanding the Overlays

Switch the **Overlay** dropdown to visualize different map layers:

| Overlay | What It Shows | Why It Matters |
|---|---|---|
| **Heightmap** | Grayscale terrain elevation | Verify island shape, mountains, valleys |
| **Water & Cliffs** | Blue = water, Gray = cliffs, Green = walkable | Validate unit traversal paths |
| **Walkable** | Green = passable, Red = blocked | Quick pass/fail on base connectivity |
| **Buildable** | Green = buildable, Red = no-build | Check base/expansion placement |
| **Tactical Zones** | Strategic map grammar | See where battles will happen |
| **Influence Map** | Red = Player A, Blue = Player B | Check territorial balance |
| **Combat Heat** | Purple = contested zones | Identify battle hotspots |

## Reading the Validation Panel

After generation, the panel shows:

```
Score: 72.4 | Bases: 2 | Expansions: 6 | Chokes: 4
Validation: PASS (0/0 issues)
```

**Score** — Overall strategic quality (0-100). Target: 65+

**Bases** — Starting positions placed.

**Expansions** — Economic expansion points.

**Chokes** — Bottlenecks detected (includes river crossings).

### If Validation Shows Warnings

```
Validation: FAIL — 2 issue(s)
• [WARN] Resource accessibility imbalance: 18%
• [WARN] No choke points detected for 2 players
```

The generator will **auto-retry** up to 10 times with a new seed. If warnings persist, the map is accepted but flagged. You may want to:
- Increase map size
- Adjust symmetry
- Change seed manually

## Exporting Maps

Click **Export JSON** to save map metadata to:
```
YourProject/Saved/RTSMapForge_LastExport.json
```

This contains:
- Seed, grid dimensions, cell size
- Base positions
- Expansion positions
- Choke points
- Validation score and pass/fail status

## Baking to Landscape (Editor Only)

After generating a satisfactory map:
1. Switch to **Heightmap** overlay to verify terrain shape.
2. In the editor console, run:
   ```
   RTSMapForge.BakeToLandscape
   ```
3. The grid heightmap is written to a `ALandscapeProxy` in the current level.
4. Adjust `CellSize` in settings before generating to control world scale.

## Determinism — Same Seed, Same Map

RTS MapForge is **fully deterministic**. Same seed + same settings = identical map, every time.

This is critical for:
- Multiplayer synchronization
- Map sharing between players
- Tournament use
- Regression testing

To verify: Generate with Seed=1337. Generate again. Minimap should be pixel-identical.

## Biome Presets

The plugin ships with four default biome types:

| Biome | Color | Movement | Building | Resources |
|---|---|---|---|---|
| **Temperate** | Green forest | Normal | Yes | Wood, Stone |
| **Desert** | Sand dunes | Slow (1.3x) | Yes | Oil, Gold |
| **Snow** | White ice | Very slow (1.5x) | Yes | Crystals, Gas |
| **Lava** | Volcanic rock | Crawl (2.0x) | **No** | Minerals, Magma |

Biomes affect:
- Unit movement speed
- Where resources spawn
- Whether buildings can be placed
- Visual mesh spawning (V1.5+)

## Performance Expectations

| Grid Size | Generation Time | Memory |
|---|---|---|
| 128×128 | < 0.5 sec | ~2 MB |
| 256×256 | < 2 sec | ~8 MB |
| 512×512 | < 8 sec | ~32 MB |
| 1024×1024 | < 25 sec | ~128 MB |

Times measured on mid-range hardware (2023 CPU). Actual times depend on settings and validation retries.

## Tips for Best Results

1. **Start with 256×256** — good balance of detail and generation speed.
2. **Use symmetry for 2-4 players** — creates fair, competitive maps.
3. **Check Walkable overlay first** — ensures bases are connected.
4. **Check Tactical Zones overlay** — confirms strategic depth.
5. **Save presets** — click "Save Preset" to store favorite configurations.
6. **Share seeds** — tell teammates "Map 48291" and they get the exact same map.

## Troubleshooting

**Minimap stays white / blank**
→ Click GENERATE MAP. The preview updates after generation completes.

**"Subsystem not ready" error**
→ Close and reopen the MapForge window. The EditorSubsystem initializes on first open.

**Viewport overlay doesn't appear**
→ Check **"Show Viewport Overlay"** checkbox in the MapForge window. Toggle off/on to refresh.

**Generation takes too long**
→ Reduce grid size to 128×128 for rapid iteration. Increase to 512×512 only for final export.

**Validation always fails**
→ Try different seeds, or increase map size. Very small maps (128×128) with 4+ players often fail.

## Next Steps

- Read `API.md` for Blueprint API documentation.
- See `Showcase.md` for example map screenshots and strategic analysis.
- Join the community for custom biome sharing and template packs.
