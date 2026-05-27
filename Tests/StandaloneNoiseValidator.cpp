// ============================================================
// Standalone Noise Determinism Validator
// Compile: g++ -std=c++17 -O2 -o noise_validator StandaloneNoiseValidator.cpp
// Run: ./noise_validator
// ============================================================
// This validates the EXACT same algorithm logic used in the UE plugin,
// but with standard C++ types so you can test it without the engine.
// ============================================================

#include <cmath>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <string>

// --- Minimal math utilities (mimicking Unreal's FMath) ---
static inline float Sqrtf(float x) { return std::sqrt(x); }
static inline float Fabsf(float x) { return std::fabs(x); }
static inline float Floorf(float x) { return std::floor(x); }
static inline float Clampf(float x, float minv, float maxv) {
    return (x < minv) ? minv : ((x > maxv) ? maxv : x);
}
static inline int32_t FloorToInt(float x) { return static_cast<int32_t>(Floorf(x)); }
static inline int32_t RandRange(int32_t Min, int32_t Max, uint32_t& State) {
    // LCG
    State = State * 1103515245u + 12345u;
    return Min + static_cast<int32_t>(State % static_cast<uint32_t>(Max - Min + 1));
}
static inline float FRand(uint32_t& State) {
    State = State * 1103515245u + 12345u;
    return static_cast<float>(State) / 4294967296.0f;
}

// --- Perlin / FBM implementation (EXACT logic from FRTSNoiseGenerator) ---
class StandaloneNoiseGenerator {
public:
    int32_t Permutation[512];

    void Initialize(int32_t Seed) {
        int32_t BasePerm[256];
        for (int32_t i = 0; i < 256; ++i) BasePerm[i] = i;

        uint32_t LCGState = static_cast<uint32_t>(Seed);
        for (int32_t i = 255; i > 0; --i) {
            LCGState = LCGState * 1103515245u + 12345u;
            int32_t j = static_cast<int32_t>(LCGState % static_cast<uint32_t>(i + 1));
            int32_t tmp = BasePerm[i]; BasePerm[i] = BasePerm[j]; BasePerm[j] = tmp;
        }

        for (int32_t i = 0; i < 256; ++i) {
            Permutation[i] = BasePerm[i];
            Permutation[i + 256] = BasePerm[i];
        }
    }

    static float Fade(float t) {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }
    static float Lerp(float a, float b, float t) { return a + t * (b - a); }
    static float Grad(int32_t hash, float x, float y) {
        int32_t h = hash & 0x7;
        float u = h < 4 ? x : y;
        float v = h < 4 ? y : x;
        float su = (h & 1) ? -1.0f : 1.0f;
        float sv = (h & 2) ? -1.0f : 1.0f;
        return su * u + sv * v;
    }

    float PerlinNoise2D(float x, float y) const {
        int32_t X0 = FloorToInt(x) & 255;
        int32_t Y0 = FloorToInt(y) & 255;
        float xf = x - Floorf(x);
        float yf = y - Floorf(y);
        float u = Fade(xf);
        float v = Fade(yf);

        int32_t A = Permutation[X0] + Y0;
        int32_t B = Permutation[X0 + 1] + Y0;

        float N00 = Grad(Permutation[A], xf, yf);
        float N01 = Grad(Permutation[A + 1], xf, yf - 1.0f);
        float N10 = Grad(Permutation[B], xf - 1.0f, yf);
        float N11 = Grad(Permutation[B + 1], xf - 1.0f, yf - 1.0f);

        float X0Lerp = Lerp(N00, N10, u);
        float X1Lerp = Lerp(N01, N11, u);
        return Lerp(X0Lerp, X1Lerp, v);
    }

    float FBM(float x, float y, int32_t octaves, float persistence, float lacunarity) const {
        float total = 0.0f;
        float amplitude = 1.0f;
        float frequency = 1.0f;
        float maxValue = 0.0f;

        for (int32_t i = 0; i < octaves; ++i) {
            total += amplitude * PerlinNoise2D(x * frequency, y * frequency);
            maxValue += amplitude;
            amplitude *= persistence;
            frequency *= lacunarity;
        }
        if (maxValue > 1e-5f) total /= maxValue;
        return total; // roughly [-1, 1]
    }

    float SeededFBM(float x, float y, int32_t octaves, float persistence, float lacunarity, float offsetX, float offsetY) const {
        return FBM(x + offsetX, y + offsetY, octaves, persistence, lacunarity);
    }
};

// --- Deterministic Seed Manager (mimicking UFRTSSeedManager) ---
class StandaloneSeedManager {
public:
    uint32_t StreamState = 0;
    void Initialize(int64_t seed) {
        int32_t Seed32 = static_cast<int32_t>(seed ^ (seed >> 32));
        StreamState = static_cast<uint32_t>(Seed32);
    }
    int32_t RandRange(int32_t min, int32_t max) {
        return ::RandRange(min, max, StreamState);
    }
    float RandFloat() {
        return FRand(StreamState);
    }
};

// --- Grid / Cell (simplified from FRTSGrid / FRTSCell) ---
struct StandaloneCell {
    float height = 0.0f;
    float slope = 0.0f;
    bool walkable = true;
    bool buildable = true;
    bool water = false;
    bool cliff = false;
    int32_t regionID = -1;
    int32_t biomeID = -1;
};

struct StandaloneGrid {
    int32_t W = 0, H = 0;
    float cellSize = 200.0f;
    std::vector<StandaloneCell> cells;

    void Init(int32_t w, int32_t h, float cs) {
        W = w; H = h; cellSize = cs;
        cells.resize(static_cast<size_t>(w) * h);
    }

    StandaloneCell& at(int32_t x, int32_t y) { return cells[y * W + x]; }
    const StandaloneCell& at(int32_t x, int32_t y) const { return cells[y * W + x]; }
};

// --- Test suite ---
static int32_t g_passed = 0;
static int32_t g_failed = 0;

void CHECK(bool cond, const std::string& name) {
    if (cond) { g_passed++; printf("  [PASS] %s\n", name.c_str()); }
    else      { g_failed++; printf("  [FAIL] %s\n", name.c_str()); }
}

void CHECK_NEAR(float a, float b, float eps, const std::string& name) {
    bool ok = Fabsf(a - b) <= eps;
    if (ok) { g_passed++; printf("  [PASS] %s (%.6f vs %.6f)\n", name.c_str(), a, b); }
    else    { g_failed++; printf("  [FAIL] %s (%.6f vs %.6f, diff=%.6f)\n", name.c_str(), a, b, Fabsf(a-b)); }
}

int main(int argc, char** argv) {
    printf("========================================\n");
    printf("RTS MapForge — Standalone Determinism Validator\n");
    printf("========================================\n\n");

    // TEST 1: Same seed → same permutation → same noise
    printf("TEST 1: Perlin Noise Determinism\n");
    {
        StandaloneNoiseGenerator A, B;
        A.Initialize(123456);
        B.Initialize(123456);
        for (float x = 0.0f; x <= 10.0f; x += 1.25f) {
            for (float y = 0.0f; y <= 10.0f; y += 1.25f) {
                float va = A.PerlinNoise2D(x, y);
                float vb = B.PerlinNoise2D(x, y);
                CHECK_NEAR(va, vb, 0.00001f, std::string("Perlin at (") + std::to_string(x) + "," + std::to_string(y) + ")");
            }
        }

        // Different seed → different (sample multiple points to avoid zero-crossing false negatives)
        StandaloneNoiseGenerator C;
        C.Initialize(123457);
        float va = A.PerlinNoise2D(5.0f, 5.0f) + A.PerlinNoise2D(1.3f, 4.7f) + A.PerlinNoise2D(8.9f, 2.1f);
        float vc = C.PerlinNoise2D(5.0f, 5.0f) + C.PerlinNoise2D(1.3f, 4.7f) + C.PerlinNoise2D(8.9f, 2.1f);
        CHECK(!std::fabs(va - vc) < 0.0001f, "Different seed yields different noise");
    }

    // TEST 2: FBM Determinism
    printf("\nTEST 2: FBM Determinism\n");
    {
        StandaloneNoiseGenerator A, B;
        A.Initialize(7777); B.Initialize(7777);
        for (float x = 0.0f; x <= 5.0f; x += 0.5f) {
            for (float y = 0.0f; y <= 5.0f; y += 0.5f) {
                float va = A.FBM(x, y, 6, 0.5f, 2.0f);
                float vb = B.FBM(x, y, 6, 0.5f, 2.0f);
                CHECK_NEAR(va, vb, 0.00001f, "FBM match");
            }
        }
    }

    // TEST 3: Seed Manager Stream Replay
    printf("\nTEST 3: Seed Manager Replay\n");
    {
        StandaloneSeedManager A, B;
        A.Initialize(424242); B.Initialize(424242);
        bool match = true;
        for (int i = 0; i < 1000; ++i) {
            if (A.RandRange(0, 100000) != B.RandRange(0, 100000)) { match = false; break; }
        }
        CHECK(match, "1000 sequential random values match with same seed");

        // Reset and replay
        A.Initialize(424242); B.Initialize(424242);
        match = true;
        for (int i = 0; i < 100; ++i) {
            if (std::fabs(A.RandFloat() - B.RandFloat()) > 0.0001f) { match = false; break; }
        }
        CHECK(match, "Stream replay after re-initialization matches");
    }

    // TEST 4: Full Heightmap Generation Determinism (Mini Grid)
    printf("\nTEST 4: Full Heightmap Pipeline Determinism (64x64)\n");
    {
        const int32_t GRID_W = 64, GRID_H = 64;
        const float CellSize = 200.0f;
        const int32_t Octaves = 6;
        const float Persistence = 0.5f;
        const float Lacunarity = 2.0f;
        const float Scale = 1.0f;
        const float WaterLevel = 0.25f;
        const int64_t Seed = 999888;

        StandaloneGrid GridA, GridB;
        GridA.Init(GRID_W, GRID_H, CellSize);
        GridB.Init(GRID_W, GRID_H, CellSize);

        StandaloneNoiseGenerator NoiseA, NoiseB;
        NoiseA.Initialize(static_cast<int32_t>(Seed ^ (Seed >> 32)));
        NoiseB.Initialize(static_cast<int32_t>(Seed ^ (Seed >> 32)));

        // Offsets derived from seed (same logic as FRTSHeightmapGenerator)
        float OffsetX = static_cast<float>(Seed & 0xFFFF) * 0.01f;
        float OffsetY = static_cast<float>((Seed >> 16) & 0xFFFF) * 0.01f;

        for (int32_t Y = 0; Y < GRID_H; ++Y) {
            for (int32_t X = 0; X < GRID_W; ++X) {
                float NX = static_cast<float>(X) / static_cast<float>(GRID_W) * Scale;
                float NY = static_cast<float>(Y) / static_cast<float>(GRID_H) * Scale;

                float RawA = NoiseA.SeededFBM(NX, NY, Octaves, Persistence, Lacunarity, OffsetX, OffsetY);
                float RawB = NoiseB.SeededFBM(NX, NY, Octaves, Persistence, Lacunarity, OffsetX, OffsetY);

                float NormA = Clampf((RawA + 1.0f) * 0.5f, 0.0f, 1.0f);
                float NormB = Clampf((RawB + 1.0f) * 0.5f, 0.0f, 1.0f);

                GridA.at(X, Y).height = NormA;
                GridB.at(X, Y).height = NormB;
            }
        }

        int32_t mismatches = 0;
        for (int32_t i = 0; i < GRID_W * GRID_H; ++i) {
            if (std::fabs(GridA.cells[i].height - GridB.cells[i].height) > 0.0001f) {
                mismatches++;
            }
        }
        CHECK(mismatches == 0, std::string("Zero height mismatches across 64x64 grid (found ") + std::to_string(mismatches) + ")");

        // Water classification determinism
        int32_t waterA = 0, waterB = 0;
        for (int32_t i = 0; i < GRID_W * GRID_H; ++i) {
            if (GridA.cells[i].height < WaterLevel) { GridA.cells[i].water = true; GridA.cells[i].walkable = false; waterA++; }
            if (GridB.cells[i].height < WaterLevel) { GridB.cells[i].water = true; GridB.cells[i].walkable = false; waterB++; }
        }
        CHECK(waterA == waterB, "Water cell count identical");
    }

    // TEST 5: Cell struct size validation (no dynamic allocations)
    printf("\nTEST 5: Cell Size Validation\n");
    {
        size_t cellSize = sizeof(StandaloneCell);
        printf("  StandaloneCell size: %zu bytes\n", cellSize);
        CHECK(cellSize <= 64, "Cell struct is compact (< 64 bytes)");
    }

    printf("\n========================================\n");
    printf("Results: %d passed, %d failed\n", g_passed, g_failed);
    printf("========================================\n");

    return (g_failed > 0) ? 1 : 0;
}
