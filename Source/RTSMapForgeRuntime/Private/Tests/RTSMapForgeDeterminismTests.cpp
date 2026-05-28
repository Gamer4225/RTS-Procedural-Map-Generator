// FIX Problem 3: Entire test file is now wrapped in WITH_DEV_AUTOMATION_TESTS.
// Previously these tests compiled unconditionally, meaning they could be
// included in packaged game builds — a Fab certification concern.
// WITH_DEV_AUTOMATION_TESTS is only true in Editor + non-shipping builds,
// so the test binary never ships with the packaged plugin.
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Core/FRTSGrid.h"
#include "Core/FRTSSeedManager.h"
#include "Core/URTSGenerationSettings.h"
#include "Terrain/FRTSNoiseGenerator.h"
#include "Terrain/FRTSHeightmapGenerator.h"
#include "Core/FRTSGenerationPipeline.h"

// ============================================
// UE AUTOMATION TEST SUITE
// Run via: Editor → Tools → Session Frontend → Automation
//       or: RunUAT BuildPlugin ... -test
// ============================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSMapForge_Determinism_Noise,
    "RTSMapForge.Determinism.PerlinNoise_SameSeed_SameOutput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTSMapForge_Determinism_Noise::RunTest(const FString& Parameters)
{
    // Test: Same seed → identical permutation table → identical noise values
    FRTSNoiseGenerator NoiseA;
    FRTSNoiseGenerator NoiseB;

    const int32 TestSeed = 123456;
    NoiseA.Initialize(TestSeed);
    NoiseB.Initialize(TestSeed);

    for (float X = 0.0f; X <= 10.0f; X += 1.25f)
    {
        for (float Y = 0.0f; Y <= 10.0f; Y += 1.25f)
        {
            float ValA = NoiseA.PerlinNoise2D(X, Y);
            float ValB = NoiseB.PerlinNoise2D(X, Y);
            TestEqual(FString::Printf(TEXT("Perlin match at (%.2f, %.2f)"), X, Y), ValA, ValB);
        }
    }

    // Different seed → different values (with very high probability)
    FRTSNoiseGenerator NoiseC;
    NoiseC.Initialize(TestSeed + 1);
    float ValA = NoiseA.PerlinNoise2D(5.0f, 5.0f);
    float ValC = NoiseC.PerlinNoise2D(5.0f, 5.0f);
    TestFalse(TEXT("Different seed must produce different noise"), FMath::IsNearlyEqual(ValA, ValC, 0.0001f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSMapForge_Determinism_FBM,
    "RTSMapForge.Determinism.FBM_SameSeed_SameOutput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTSMapForge_Determinism_FBM::RunTest(const FString& Parameters)
{
    FRTSNoiseGenerator NoiseA;
    FRTSNoiseGenerator NoiseB;
    const int32 Seed = 7777;
    NoiseA.Initialize(Seed);
    NoiseB.Initialize(Seed);

    const int32 Octaves     = 6;
    const float Persistence = 0.5f;
    const float Lacunarity  = 2.0f;

    for (float X = 0.0f; X <= 5.0f; X += 0.5f)
    {
        for (float Y = 0.0f; Y <= 5.0f; Y += 0.5f)
        {
            float ValA = NoiseA.FBM(X, Y, Octaves, Persistence, Lacunarity);
            float ValB = NoiseB.FBM(X, Y, Octaves, Persistence, Lacunarity);
            TestEqual(FString::Printf(TEXT("FBM match at (%.2f, %.2f)"), X, Y), ValA, ValB);
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSMapForge_Determinism_SeedManager,
    "RTSMapForge.Determinism.SeedManager_StreamReplay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTSMapForge_Determinism_SeedManager::RunTest(const FString& Parameters)
{
    UFRTSSeedManager* SeedA = NewObject<UFRTSSeedManager>();
    UFRTSSeedManager* SeedB = NewObject<UFRTSSeedManager>();

    const int64 Seed = 424242;
    SeedA->Initialize(Seed);
    SeedB->Initialize(Seed);

    TArray<int32> SeqA;
    TArray<int32> SeqB;
    for (int32 i = 0; i < 1000; ++i)
    {
        SeqA.Add(SeedA->RandRange(0, 100000));
        SeqB.Add(SeedB->RandRange(0, 100000));
    }

    for (int32 i = 0; i < 1000; ++i)
    {
        TestEqual(FString::Printf(TEXT("Stream match at %d"), i), SeqA[i], SeqB[i]);
    }

    // Reset and replay must match
    SeedA->ResetStream();
    SeedB->ResetStream();
    for (int32 i = 0; i < 100; ++i)
    {
        TestEqual(FString::Printf(TEXT("Replay match at %d"), i), SeedA->RandFloat(), SeedB->RandFloat());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSMapForge_Determinism_FullPipeline,
    "RTSMapForge.Determinism.FullPipeline_SameSettings_SameGrid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTSMapForge_Determinism_FullPipeline::RunTest(const FString& Parameters)
{
    URTSGenerationSettings* SettingsA = NewObject<URTSGenerationSettings>();
    URTSGenerationSettings* SettingsB = NewObject<URTSGenerationSettings>();

    SettingsA->GridWidth      = 128;
    SettingsA->GridHeight     = 128;
    SettingsA->Seed           = 999888;
    SettingsA->bRandomSeed    = false;
    SettingsA->FBMOctaves     = 6;
    SettingsA->FBMPersistence = 0.5f;
    SettingsA->FBMLacunarity  = 2.0f;
    SettingsA->TerrainScale   = 1.0f;
    SettingsA->WaterLevel     = 0.25f;
    SettingsA->MountainLevel  = 0.75f;
    SettingsA->NumPlayers     = 2;

    *SettingsB = *SettingsA; // Deep copy

    FRTSGrid            GridA, GridB;
    FRTSMapMetadata     MetaA, MetaB;
    FRTSValidationResult ValA, ValB;

    FRTSGenerationPipeline PipeA;
    FRTSGenerationPipeline PipeB;

    PipeA.Generate(SettingsA, GridA, MetaA, ValA, /*MaxRetries=*/1);
    PipeB.Generate(SettingsB, GridB, MetaB, ValB, /*MaxRetries=*/1);

    TestEqual(TEXT("Grid dimensions match"), GridA.Cells.Num(), GridB.Cells.Num());
    TestEqual(TEXT("Grid width match"),      GridA.Width,       GridB.Width);
    TestEqual(TEXT("Grid height match"),     GridA.Height,      GridB.Height);

    int32 MismatchCount = 0;
    for (int32 i = 0; i < GridA.Cells.Num(); ++i)
    {
        if (!FMath::IsNearlyEqual(GridA.Cells[i].Height, GridB.Cells[i].Height, 0.0001f))
        {
            ++MismatchCount;
        }
    }

    TestEqual(TEXT("Zero height mismatches across full pipeline"), MismatchCount, 0);
    TestTrue(TEXT("Validation passed"), ValA.bPassed);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSMapForge_Memory_CellSize,
    "RTSMapForge.Memory.CellStructSize_Small",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTSMapForge_Memory_CellSize::RunTest(const FString& Parameters)
{
    const SIZE_T CellSize   = sizeof(FRTSCell);
    const SIZE_T MaxExpected = 160;

    UE_LOG(LogTemp, Log, TEXT("FRTSCell size: %llu bytes"), static_cast<uint64>(CellSize));
    TestTrue(TEXT("FRTSCell is reasonably small (< 160 bytes)"), CellSize <= MaxExpected);

    FRTSGrid Grid(256, 256, 200.0f);
    SIZE_T Allocated    = Grid.GetAllocatedSize();
    SIZE_T ExpectedApprox = static_cast<SIZE_T>(256 * 256) * CellSize;
    TestTrue(TEXT("Grid allocation matches cell count"), Allocated >= ExpectedApprox);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTSMapForge_Grid_BoundsSafety,
    "RTSMapForge.Grid.BoundsCheck_Debug",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FRTSMapForge_Grid_BoundsSafety::RunTest(const FString& Parameters)
{
    FRTSGrid Grid(32, 32, 100.0f);

    TestTrue(TEXT("Valid coord (0,0)"),     Grid.IsValidCoord(0, 0));
    TestTrue(TEXT("Valid coord (31,31)"),   Grid.IsValidCoord(31, 31));
    TestFalse(TEXT("Invalid coord (-1,0)"), Grid.IsValidCoord(-1, 0));
    TestFalse(TEXT("Invalid coord (32,0)"), Grid.IsValidCoord(32, 0));
    TestFalse(TEXT("Invalid index -1"),     Grid.IsValidIndex(-1));
    TestFalse(TEXT("Invalid index 1024"),   Grid.IsValidIndex(1024));
    TestTrue(TEXT("Valid index 0"),         Grid.IsValidIndex(0));
    TestTrue(TEXT("Valid index 1023"),      Grid.IsValidIndex(1023));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
