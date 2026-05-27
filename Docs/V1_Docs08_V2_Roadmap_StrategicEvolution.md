# RTS MapForge — V2 Roadmap: Strategic Evolution

## V1 Status: Strategic Foundation Complete

V1 generates **emergent strategy** from terrain:
- Noise + falloff creates island shape
- Rivers create barriers and crossings
- Symmetric placement ensures fairness
- Chokes emerge from region boundaries
- Resources cluster near objectives

**V2 will generate intentional strategy.** Maps will have **strategic identity**: defensive, aggressive, macro-focused, mobility-heavy, artillery-centric.

---

## V2 Feature: Strategic Archetypes / Templates

### The Problem

Every V1 map feels similar. Roughly equal chance of any strategic pattern. Customers want:
- **Defensive maps** for turtle players
- **Macro maps** for economic expansion
- **Mobility maps** for flanking armies
- **Artillery maps** for ranged siege
- **Naval maps** for amphibious warfare

### The Solution: Strategic Archetype Presets

Each archetype is a **parameter profile** that biases the 20-stage pipeline toward a specific strategic character.

#### 1. Defensive Archetype ("The Fortress")

| Pipeline Parameter | Biased Value | Effect |
|---|---|---|
| `WaterLevel` | Lower (0.18) | More land, more buildable area |
| `MountainLevel` | Lower (0.65) | Fewer peaks, flatter terrain |
| `FBMPersistence` | Higher (0.7) | Smoother, more predictable terrain |
| `NumExpansions` | Higher (6) | More economic points to defend |
| `MinChokeWidth` | Narrower (2) | Tighter bottlenecks |
| `MaxChokeWidth` | Narrower (8) | All chokes are tight |
| `MinRushDistance` | Higher (0.5) | Bases farther apart |
| `SymmetryStrength` | Higher (1.0) | Perfect mirror defense |
| `River count` | More (8-10) | Multiple defensive moats |
| `River width` | Narrower (2-cell) | Crossable but costly |
| Strategic scoring | `ChokeQuality` weighted 40% | Reward choke density |

**Result:** Map with multiple tight chokes, natural defensive lines, long rush distances. Encourages turtling, siege, positional warfare.

#### 2. Aggressive / Rush Archetype ("The Colosseum")

| Pipeline Parameter | Biased Value | Effect |
|---|---|---|
| `WaterLevel` | Higher (0.35) | Less land, more contested |
| `MountainLevel` | Higher (0.8) | Peaks create arena walls |
| `FBMPersistence` | Lower (0.3) | Chaotic, broken terrain |
| `NumExpansions` | Lower (2) | Few safe places, more fighting |
| `MinChokeWidth` | Wider (5) | Chokes are passable |
| `MaxChokeWidth` | Wider (15) | Most passages are open |
| `MinRushDistance` | Lower (0.2) | Bases close |
| `SymmetryStrength` | Lower (0.3) | Asymmetric for uneven fights |
| `River count` | Fewer (2-3) | Few barriers |
| `River width` | Wider (4-cell) | Easy crossings |
| Strategic scoring | `RushDistanceScore` weighted 40% | Reward close bases |

**Result:** Fast rushes, open engagements, chaotic terrain. Encourages aggressive play, early pressure, skirmishes.

#### 3. Macro / Economic Archetype ("The Empire")

| Pipeline Parameter | Biased Value | Effect |
|---|---|---|
| `GridWidth/Height` | Larger (512) | More space to expand |
| `NumExpansions` | Higher (8-12) | Many expansion points |
| `ResourceDensity` | Higher (0.8) | Rich resource fields |
| `NumPlayers` | Higher (4-6) | FFA scramble |
| `MinRushDistance` | Higher (0.5) | Safe early game |
| `WaterLevel` | Lower (0.15) | Maximum buildable land |
| `SymmetryStrength` | Moderate (0.6) | Partial symmetry |
| Strategic scoring | `ExpansionSafety` weighted 40% | Reward protected expansions |

**Result:** Huge map, many resources, safe early game. Encourages greedy play, economic war, late-game armies.

#### 4. Mobility / Flanking Archetype ("The Maze")

| Pipeline Parameter | Biased Value | Effect |
|---|---|---|
| `FBMLacunarity` | Higher (3.0) | Chaotic terrain variation |
| `FBMOctaves` | Higher (8) | Very detailed terrain |
| `WaterLevel` | Moderate (0.25) | Rivers as highways |
| `River width` | Narrow (1-2 cell) | Fast amphibious crossings |
| `MountainLevel` | Moderate (0.7) | Hills create cover |
| `NumExpansions` | Moderate (4) | Several flanking routes |
| Strategic scoring | `PathDiversity` weighted 40% | Reward multiple routes |

**Result:** Complex terrain with multiple paths between any two points. Encourages hit-and-run, flanking, pincer movements.

#### 5. Artillery / Siege Archetype ("The Ridge")

| Pipeline Parameter | Biased Value | Effect |
|---|---|---|
| `MountainLevel` | Lower (0.55) | Many elevated areas |
| `WaterLevel` | Low (0.15) | Open sightlines |
| `Height bias per biome` | +0.15 | Elevated terrain |
| `HighGround threshold` | Lower (0.55) | More high ground cells |
| Strategic scoring | `CoverValue` weighted 30% | Reward cover |

**Result:** Elevated ridges and valleys create natural artillery positions. Encourages siege warfare, ranged dominance, positional control.

---

## V2 Feature: Bitmask Tactical Tags (Replacing Single-Layer Enum)

### The Problem

`enum class ERTSTacticalZone` allows ONE zone per cell. A cell cannot be:
- Both HighGround AND ChokePoint
- Both ResourceCluster AND FlankRoute
- Both MainBase AND Cover position

### The Solution: `FRTSTacticalTags` Bitmask

```cpp
USTRUCT(BlueprintType)
struct FRTSTacticalTags
{
    GENERATED_BODY()
    
    uint32 Tags = 0;
    
    // Tag bits
    static constexpr uint32 MainBase        = 1 << 0;
    static constexpr uint32 Expansion       = 1 << 1;
    static constexpr uint32 ChokePoint      = 1 << 2;
    static constexpr uint32 RiverCrossing   = 1 << 3;
    static constexpr uint32 HighGround      = 1 << 4;
    static constexpr uint32 ResourceCluster  = 1 << 5;
    static constexpr uint32 OpenBattlefield = 1 << 6;
    static constexpr uint32 FlankRoute      = 1 << 7;
    static constexpr uint32 Cover           = 1 << 8;      // NEW: Ambush/stealth viable
    static constexpr uint32 ArtilleryPosition = 1 << 9;    // NEW: Elevated + open sightlines
    static constexpr uint32 DropZone        = 1 << 10;   // NEW: Air/teleport viable
    static constexpr uint32 SupplyLine      = 1 << 11;   // NEW: Critical path between bases
    
    bool HasTag(uint32 Tag) const { return (Tags & Tag) != 0; }
    void AddTag(uint32 Tag) { Tags |= Tag; }
    void RemoveTag(uint32 Tag) { Tags &= ~Tag; }
};
```

### Benefits

| Scenario | V1 (Enum) | V2 (Bitmask) |
|---|---|---|
| High ground choke with resources | `ChokePoint` (loses resource info) | `ChokePoint + HighGround + ResourceCluster` |
| Base on elevated ridge | `MainBase` (loses elevation info) | `MainBase + HighGround + ArtilleryPosition` |
| River ford on flank route | `RiverCrossing` (loses flank info) | `RiverCrossing + FlankRoute` |
| Expansion near ambush cover | `NatExpansion` (loses cover info) | `NatExpansion + Cover` |

**AI Benefits:** An AI seeing `ChokePoint + HighGround + Cover` knows: "Defend this with ranged units in cover."

---

## V2 Feature: Traffic-Weighted Choke/Crossing Scoring

### The Problem

Current choke scoring is geometry-only: width, region adjacency. It doesn't know which chokes are actually important.

### The Solution: Betweenness Centrality via A*

```
For each player base pair (A, B):
    Run A* from A to B
    Record every choke/crossing cell traversed
    Increment traffic counter per cell

Choke strategic value = GeometryScore × (1 + log(TrafficCount + 1))
```

**Result:** A choke on the ONLY path between two bases becomes 10× more important than a choke on one of 5 alternate routes.

---

## V2 Feature: Terrain-Aware Influence Propagation

### The Problem

Current influence uses Euclidean distance: `I(x,y) = S / (d² + 1)`. Rivers and cliffs don't block influence.

### The Solution: Path-Cost Propagation

```
For each base B:
    Dijkstra/BFS from B with cost = MovementCostMultiplier
    Influence at cell C = S / (path_cost(B→C)² + 1)
```

**Result:** Influence naturally stops at rivers, goes around cliffs, slows through swamps. Maps with rivers show sharp territorial boundaries instead of smooth gradients.

---

## V2 Feature: Async Generation (UE Tasks)

### Architecture

```
[Editor Thread]                         [Background Task]
    │ Launch AsyncTask                     │
    │                                    Stage 1-15 (pure compute)
    │                                    (Grid mutation, no UObject)
    │                                      │
    │ <--- FOnMapGenComplete callback ---  │
    │                                    Stage 16 (UObject bake)
    │                                    (Landscape write, actor spawn)
```

**Requirements:**
- All stages 1-15 use ONLY `FRTSGrid`, `FRTSMapMetadata`, raw float arrays
- NO `UObject` access in background thread
- Callback marshals to Game Thread for:
  - `UTexture2D` preview update
  - `ALandscapeProxy` bake
  - `AEditorTickActor` viewport refresh

---

## V2 Feature: Monte Carlo AI Simulation

### Concept

Simulate 100-1000 rapid RTS matches on the generated grid to score map quality:

```
For N simulated matches:
    Place 2 AI agents at base positions
    AI uses simplified rules:
      - Rush if distance < threshold
      - Expand if resources nearby
      - Control chokes with cheapest units
    Score:
      - Win rate balance (50/50 = perfect)
      - Average game duration (10-20 min target)
      - Expansion timing similarity
      - Choke battle frequency

MapScore += MatchSimulationScore / N
```

**Benefit:** Direct evidence that "this map produces good games." Not heuristic scoring — actual simulated play.

---

## V2 Development Order

| Priority | Feature | Est. Effort | Impact |
|---|---|---|---|
| **P0** | Bitmask tactical tags | 2 weeks | Enables all other AI features |
| **P0** | Strategic archetype presets | 1 week | Immediate commercial differentiation |
| **P1** | Traffic-weighted chokes | 2 weeks | Massive strategic scoring improvement |
| **P1** | Terrain-aware influence | 2 weeks | Correct territorial analysis |
| **P1** | Async generation | 2 weeks | Scales to 1024×1024+ |
| **P2** | Monte Carlo simulation | 3 weeks | Elite-tier validation |
| **P2** | Runtime generation API | 1 week | Multiplayer/dynamic campaign support |
| **P3** | ML optimization | 4 weeks | Future-proof research feature |

---

## V2 Pricing

| Version | Price | New Features |
|---|---|---|
| V1.0 | **$149** | Core generator + strategic analysis + editor |
| V1.5 | $179 | River polish + biome packs + landscape bake |
| **V2.0** | **$249** | Archetypes + bitmask tags + traffic chokes + terrain influence + async |
| V2.5 | $299 | Monte Carlo simulation + runtime generation |
| V3.0 | $399 | ML map optimization + campaign tools |
