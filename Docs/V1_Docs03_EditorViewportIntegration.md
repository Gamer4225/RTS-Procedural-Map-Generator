# RTS MapForge V1 — Editor Viewport Integration

## What Was Built

A complete **in-editor Slate UI + Viewport Overlay** system that allows you to generate maps, preview them as a minimap, and visualize strategic data directly in the Unreal Editor viewport.

---

## Editor Module Files

```
RTSMapForge/Source/RTSMapForgeEditor/
├── Public/
│   ├── URTSMapForgeEditorSubsystem.h      ← EditorSubsystem: owns grid, preview texture, overlay mode
│   ├── SRTSMapGeneratorWindow.h           ← Slate window: controls + minimap + validation readout
│   ├── FRTSMapForgeEdMode.h               ← FEdMode: renders colored wireframe grid in viewport
│   └── FRTSMapForgeEditorCommands.h       ← UI command definition (toolbar button)
└── Private/
    ├── URTSMapForgeEditorSubsystem.cpp    ← Generation, texture update, JSON export
    ├── SRTSMapGeneratorWindow.cpp         ← Full Slate layout: seed, size, players, generate, overlay combo, preview, scores
    ├── FRTSMapForgeEdMode.cpp             ← PDI overlay rendering from subsystem grid
    ├── FRTSMapForgeEditorCommands.cpp     ← Command registration
    └── RTSMapForgeEditorModule.cpp        ← Module: registers EdMode, tab spawner, toolbar button
```

---

## User Workflow

### 1. Open the Generator
Click the **"MapForge"** toolbar button in the Level Editor toolbar (next to Play).
This opens the **RTS MapForge** dockable tab:

```
┌─────────────────────────────────────┐
│  RTS MapForge — Battlefield Generator│
├─────────────────────────────────────┤
│  Seed: [1337_______] [🎲]           │
│  Size W/H: [256]                    │
│  Players: [2]   Symmetry: [1.0]     │
│  [        GENERATE MAP        ]     │
├─────────────────────────────────────┤
│  Overlay: [Heightmap ▼]             │
│  ☑ Show Viewport Overlay            │
├─────────────────────────────────────┤
│  ┌─────────────────────────────┐   │
│  │      [Minimap Preview]      │   │
│  │  (live 256×256 texture)     │   │
│  └─────────────────────────────┘   │
├─────────────────────────────────────┤
│  Score: 71.0 | Bases: 2 | Exp: 6    │
│  Validation: PASS (0/0 issues)      │
├─────────────────────────────────────┤
│  [Export JSON] [Cycle Overlay]      │
└─────────────────────────────────────┘
```

### 2. Generate
Click **GENERATE MAP**. The pipeline runs through all 16 stages:
- Seed init → Grid allocation → Heightmap (FBM) → Classification → Biomes → Regions → Bases → Expansions → Chokes → Tactical zones → A* pathfinding → Influence maps → Heatmaps → Scoring → Validation

### 3. Preview
- **Minimap**: live `UTexture2D` updated from `FRTSDebugRenderer::GenerateMinimapBitmap()`
- **Viewport Overlay**: colored wireframe quads drawn in world space at each cell position

### 4. Toggle Overlays
- Dropdown selects mode: Heightmap, Water/Cliff, Walkable, Buildable, Slope, Regions, Biomes, Tactical Zones, Influence Map, Combat Heat, Choke Points
- "Show Viewport Overlay" checkbox toggles the EdMode on/off in the active viewport
- "Cycle Overlay" button rotates through modes for quick inspection

### 5. Export
**Export JSON** saves `RTSMapForge_LastExport.json` to `Saved/` with:
- Seed, grid dimensions, cell size
- Base/expansion/choke counts
- Overall strategic score
- Pass/fail status

---

## Architecture: How It All Connects

```
User clicks GENERATE
    ↓
SRTSMapGeneratorWindow::OnGenerateClicked()
    ↓
URTSMapForgeEditorSubsystem::GenerateMap(Settings)
    ↓
FRTSGenerationPipeline::Generate() [Runtime module, 16 stages]
    ↓
URTSMapForgeEditorSubsystem::UpdatePreviewTexture()
    ↓
FRTSDebugRenderer::GenerateMinimapBitmap() → TArray<FColor>
    ↓
Memcpy into UTexture2D transient → Slate SImage refresh

User toggles "Show Viewport Overlay"
    ↓
GEditor->GetEditorModeManager().ActivateMode(EM_RTSMapForge)
    ↓
FRTSMapForgeEdMode::Render(PDI)
    ↓
FRTSDebugRenderer::RenderOverlay(Grid, CurrentMode, PDI, ...)
    ↓
Colored wireframe quads drawn at each Cell.WorldPosition
```

---

## Key Technical Details

### Preview Texture (`URTSMapForgeEditorSubsystem`)
- `UTexture2D::CreateTransient(W, H, PF_B8G8R8A8)` — no DDC, instant update
- `TF_Nearest` filter for pixel-perfect grid preview
- `FTexturePlatformData` locked, color bitmap memcopied, unlocked, `UpdateResource()` called
- Texture is owned by EditorSubsystem (survives PIE)

### Viewport Overlay (`FRTSMapForgeEdMode`)
- Standard `FEdMode` registered in `FEditorModeRegistry`
- `Render()` gets `FPrimitiveDrawInterface*` — draws `PDI->DrawLine()` quads per cell
- ZOffset = 10cm above world origin; adjust if your terrain/Landscape sits at different Z
- Does NOT interfere with selection — `AllowWidgetMove=false`, `ShouldDrawWidget=false`

### Slate Window (`SRTSMapGeneratorWindow`)
- Bound to `URTSMapForgeEditorSubsystem` via `GEditor->GetEditorSubsystem<>()`
- Lambda-heavy binding for live readout updates without custom delegates
- `Tick()` watches for grid state changes and refreshes score text
- Minimap is `SImage` with `FSlateBrush` bound to subsystem's transient texture

### Toolbar Integration
- `FLevelEditorModule::GetToolBarExtensibilityManager()->AddExtender()`
- Adds button after "Play" group
- Opens dockable tab via `FGlobalTabmanager::TryInvokeTab()`

---

## Overlay Modes (12 Total)

| Mode | Color Logic | Use Case |
|---|---|---|
| **Heightmap** | Grayscale (0=black, 1=white) | Verify terrain shape |
| **Water & Cliffs** | Blue=water, Gray=cliff, Green=walkable | Validate traversal |
| **Walkable** | Green=walkable, Red=blocked | Quick pass/fail check |
| **Buildable** | Green=buildable, Red=no-build | Base placement sanity |
| **Slope** | Green=0°, Red=90° | Cliff threshold tuning |
| **Regions** | Random per-RegionID | Flood fill debug |
| **Biomes** | Fixed per-BiomeID | Voronoi seed verification |
| **Tactical Zones** | Green=Base, Red=Choke, Brown=HighGround | Strategic readability |
| **Influence Map** | Red=Player A, Blue=Player B | Symmetry/fairness check |
| **Combat Heat** | Purple=contested, Black=safe | Conflict prediction |
| **Choke Points** | Red=choke cell, Transparent=open | Choke quality |
| **None** | Black | Reset |

---

## How to Compile & Test

### 1. Drop into Project
```
YourProject/Plugins/
    RTSMapForge/
        Source/
        RTSMapForge.uplugin
```

### 2. Regenerate Project Files
Right-click `.uproject` → **Generate Visual Studio project files**

### 3. Build
Build target: `YourProjectEditor`
This compiles both `RTSMapForgeRuntime` and `RTSMapForgeEditor`

### 4. Launch Editor
Plugin auto-loads. Verify in **Edit → Plugins → Procedural → RTS MapForge**

### 5. Open the Tool
Click **MapForge** in the main toolbar.

### 6. Generate & Verify
- Set Seed = `1337`
- Set Size = `256`
- Click **GENERATE**
- Observe minimap turns into a grayscale heightmap
- Check **Show Viewport Overlay**
- Observe colored grid in active viewport
- Change Overlay to **Water & Cliffs** — verify blue patches and gray edges
- Change Overlay to **Tactical Zones** — verify green base dots and red choke lines

### 7. Determinism Check
- Regenerate with same seed
- Minimap must be pixel-identical
- This proves `FRTSSeedManager` and `FRTSNoiseGenerator` are deterministic

---

## Next Step: Milestone 1 (Terrain Polish)

Now that the grid foundation + visualization framework is rock-solid, the next correct step is:

| Task | Description |
|---|---|
| **River Generation** | Gradient descent from `MountainLevel` peaks toward water; carve terrain; mark water |
| **Island/Radial Falloff** | Optional `ApplyRadialFalloff()` in pipeline; push edges to water |
| **Default Biome Assets** | Create `URTSBiomeAsset` Data Assets: Temperate, Desert, Snow, Lava |
| **Resource Node Scatter** | Poisson-disk place resources near expansions / high-ground |
| **N-Player Symmetry** | Extend base placer to 3-way 120°, 4-way 90°, FFA |

With the debug renderer in place, each new feature is **immediately visible** — no more blind data generation.
