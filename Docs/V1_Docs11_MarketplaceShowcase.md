# RTS MapForge — Marketplace Showcase & Sales Materials

## Product Identity

**RTS MapForge** — *Strategic Battlefield Generator for Unreal Engine 5*

> Not a terrain generator. An automated RTS map designer.

Your plugin fills the gap between procedural terrain tools and RTS-specific gameplay design. Other plugins generate noise. RTS MapForge generates **strategically valid, competitively balanced battlefields** with metadata AI systems can read.

---

## Key Selling Points (Commercial Copy)

### For Indie RTS Developers
- Generate playable maps in **seconds**, not weeks
- No level designer required for prototypes or roguelike campaigns
- Deterministic seeds — share maps with a single number

### For Multiplayer RTS Studios
- **Symmetric base placement** guarantees competitive fairness
- **Rush distance validation** — maps can't be broken by instant attacks
- **Choke point detection** — every map has strategic depth
- **Resource accessibility parity** — no player gets an economic advantage

### For Tactical/MOBA Studios
- River barriers and crossing hotspots create natural lane-like structures
- High ground and cover zones generate tactical positioning
- Influence maps show territorial control at a glance

### For Tower Defense Developers
- Pathfinding analysis identifies natural enemy routes
- Choke detection finds ideal tower placement points
- Walkable/buildable validation ensures playable paths

### For Military Sim Developers
- Realistic terrain with FBM noise, river networks, biome regions
- Slope analysis for vehicle mobility, line-of-sight
- Strategic scoring for scenario design

### For Game Jams
- Generate 20 maps, pick the best 3 in under a minute
- Same seed = same map for multiplayer tournament use
- No external tools — everything inside Unreal Editor

---

## Feature Comparison Matrix

| Feature | RTS MapForge | Generic Terrain Tool | Manual Design |
|---|---|---|---|
| Procedural generation | ✅ Yes | ✅ Yes | ❌ No |
| Symmetric base placement | ✅ Automatic | ❌ Manual | ✅ Possible |
| Rush distance validation | ✅ Automatic | ❌ Not checked | ✅ Designer test |
| Choke point detection | ✅ Algorithmic | ❌ Visual only | ✅ Experience |
| Resource parity | ✅ Accessibility scoring | ❌ Not checked | ✅ Playtest |
| Deterministic seeds | ✅ Guaranteed | ⚠️ Sometimes | ✅ Yes |
| Strategic overlays | ✅ 12 modes | ❌ Height only | ❌ None |
| A* pathfinding analysis | ✅ Built-in | ❌ External | ⚠️ Designer test |
| Biome system | ✅ 4 presets + custom | ⚠️ Limited | ❌ Manual |
| Editor integration | ✅ Full Slate UI | ⚠️ Partial | ✅ Native |
| Export to Landscape | ✅ One-click bake | ❌ Manual | ✅ Native |
| JSON metadata | ✅ Full strategic data | ❌ Not available | ❌ Not available |
| Validation/retry | ✅ 6-pass automatic | ❌ Not available | ❌ Not available |
| Price | **$149** | $50–$200 | Salary + time |

---

## Screenshot Checklist (Required for Fab Submission)

### 1. Generator Window (Primary Hero Image)
- 1920×1080, Unreal Editor interface
- Show full Slate window: seed input, size slider, generate button
- Minimap preview showing a generated island map
- Score readout: "Score: 78.4 | Bases: 2 | Expansions: 6 | Chokes: 4"
- Validation: "PASS"
- Caption: *"Generate playable RTS battlefields in seconds"*

### 2. Overlay Gallery (6 Images)

**Heightmap Overlay**
- Grayscale island terrain, visible mountains and coastlines
- Caption: *"FBM noise + radial falloff = natural island maps"*

**Water & Cliff Overlay**
- Blue rivers, gray cliffs, green walkable areas
- Show 2-3 cell wide rivers meandering from peaks to coast
- Caption: *"River networks create natural traversal barriers"*

**Tactical Zones Overlay**
- Green base dots, red choke lines, gold resource clusters, yellow river crossings
- Caption: *"Strategic zones: bases, chokes, crossings, resources — algorithmically identified"*

**Walkable Overlay**
- Green paths connecting bases, red rivers blocking
- Caption: *"Pathfinding validation ensures all bases are reachable"*

**Influence Map Overlay**
- Red/Blue territorial control with sharp boundaries at rivers
- Caption: *"Per-player influence shows territorial balance"*

**Combat Heat Overlay**
- Purple contested zones at chokes and crossings
- Caption: *"Predict battle hotspots before a single unit spawns"*

### 3. Determinism Proof (1 Image)

Two side-by-side minimaps labeled "Seed: 48291"
- Identical pixel-for-pixel
- Caption: *"Same seed = identical map. Every time. Guaranteed."*

### 4. Biome Presets (1 Image)

Four-panel split:
- Temperate (green forest)
- Desert (sand dunes)
- Snow (white ice)
- Lava (red volcanic)
- Caption: *"4 built-in biomes + unlimited custom DataAssets"*

### 5. Validation Panel (1 Image)

Show validation readout with specific issues:
```
Score: 71.0 | Bases: 2 | Expansions: 6 | Chokes: 4
Validation: FAIL — 2 issue(s)
• [WARN] Resource accessibility imbalance: 18%
• [WARN] No choke points detected for 2 players
```
Caption: *"Bad maps are rejected automatically. No broken multiplayer."*

---

## Video Trailer Script (60–90 seconds)

**0:00–0:05** — Opening shot: Black screen, text fades in:
> "Most procedural generators make noise."
> "RTS MapForge makes strategy."

**0:05–0:12** — Show editor. User clicks MapForge toolbar button. Window opens. Clicks GENERATE.
- Real-time generation progress (256×256, ~1 second)
- Minimap preview fills with terrain

**0:12–0:20** — Overlay toggle montage:
- Heightmap → Water/Cliff → Tactical Zones → Walkable → Influence → Combat Heat
- Each overlay reveals new strategic information

**0:20–0:30** — Strategic details:
- Zoom into Tactical Zones: green base, red choke, yellow crossing
- Zoom into river: 3-cell wide meandering water with smooth banks
- Text overlay: *"Rivers, chokes, crossings — all algorithmically placed"*

**0:30–0:40** — Validation demo:
- Show PASS readout with score 78.4
- Show FAIL readout with specific warnings
- Text: *"Automatic quality control. No broken maps."*

**0:40–0:50** — Determinism proof:
- Split screen: two identical minimaps
- Text: *"Seed 48291. Same map. Every time."*

**0:50–0:60** — Biome montage:
- Four maps in split screen: Temperate, Desert, Snow, Lava
- Text: *"Temperate. Desert. Snow. Lava. Unlimited custom biomes."*

**0:60–0:75** — Export and bake:
- Click Export JSON
- Click Bake to Landscape
- Landscape appears in viewport with generated terrain
- Text: *"One-click landscape bake. Ready to play."*

**0:75–0:90** — Closing:
- RTS MapForge logo
- Price: $149
- CTA: "Available now on Fab Marketplace"
- *"Generate strategy. Not just terrain."*

---

## Quick Start Tutorial (YouTube / Documentation)

### Step 1: Install (30 seconds)
1. Copy RTSMapForge to Plugins/
2. Regenerate project files
3. Launch editor
4. Enable: Edit → Plugins → Procedural → RTS MapForge

### Step 2: Generate First Map (60 seconds)
1. Click "MapForge" in Level Editor toolbar
2. Set Seed: 1337
3. Set Size: 256
4. Set Players: 2
5. Click GENERATE MAP
6. Watch minimap fill with terrain

### Step 3: Explore Overlays (90 seconds)
1. Switch Overlay to "Water & Cliff" — see rivers
2. Switch to "Tactical Zones" — see bases, chokes, resources
3. Switch to "Influence Map" — see territorial control
4. Switch to "Walkable" — verify bases connect

### Step 4: Export and Play (30 seconds)
1. Click Export JSON — metadata saved to Saved/
2. Check validation: should say PASS
3. If PASS: click Bake to Landscape (optional)

---

## FAQ (For Product Page)

**Q: Does this work at runtime?**
A: V1 is editor-time generation. V2 will add runtime generation for roguelikes and dynamic campaigns.

**Q: Can I customize biomes?**
A: Yes. Create URTSBiomeAsset Data Assets. Adjust terrain rules, movement costs, resource types, and debug colors.

**Q: Is it deterministic for multiplayer?**
A: Yes. Same seed + same settings = identical map, pixel-for-pixel. Guaranteed by 208 automated tests.

**Q: What map sizes are supported?**
A: 128×128 (MOBA-scale) up to 1024×1024 (planetary warfare). Default is 256×256 (StarCraft-scale). Generation times: 128² <0.5s, 256² <2s, 512² <8s, 1024² <25s.

**Q: Does it work with existing landscapes?**
A: Yes. Bake to an existing ALandscapeProxy in your level, or generate a new one.

**Q: Can I use this for non-RTS games?**
A: Yes. Tower defense, survival, tactical RPG, military simulation — any game needing strategic terrain analysis.

**Q: Does it require coding?**
A: No. Full Slate UI inside Unreal Editor. Blueprint API also available. C++ source included for advanced users.

**Q: Is the source code included?**
A: Yes. Full C++ source with modular architecture. Extend via plugin modules, add custom generation stages, create new validation rules.

**Q: What's included?**
A: Runtime module (algorithms, pathfinding, validation), Editor module (Slate UI, viewport overlay), 4 default biomes, Quick Start guide, API reference.

---

## Pricing & Updates

| Version | Price | Timeline | What's New |
|---|---|---|---|
| **V1.0** | **$149** | Launch | Core generator, strategic analysis, editor UI |
| V1.5 | $179 | +3 months | River polish, biome packs, landscape bake, mesh spawn |
| **V2.0** | **$249** | +6 months | Strategic archetypes, bitmask tactical tags, traffic-weighted chokes, terrain-aware influence, async generation |
| V2.5 | $299 | +9 months | Monte Carlo AI simulation, runtime generation |
| V3.0 | $399 | +12 months | ML map optimization, campaign tools |

**Bundle opportunity:** Partner with RTSUnitTemplate (your complementary plugin) for joint bundle pricing.

---

## Technical Specifications (For Fab Page)

- **Engine Version:** UE 5.3+
- **Supported Platforms:** Windows, Linux, macOS (Editor); Windows, Linux (Runtime — V2+)
- **C++ Standard:** C++17
- **Module Count:** 2 (Runtime + Editor)
- **Blueprint Support:** Yes (Settings, Subsystem API)
- **Network Replicated:** No (Editor-time tool; V2 adds deterministic runtime for multiplayer)
- **Included Files:** 72 source files, 7 documentation files, 4 default biome presets
- **Dependencies:** Core, CoreUObject, Engine, NavigationSystem, AIModule, Landscape (Editor), Slate (Editor)
