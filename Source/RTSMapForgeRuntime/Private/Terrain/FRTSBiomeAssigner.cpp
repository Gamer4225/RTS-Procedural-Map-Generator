#include "Terrain/FRTSBiomeAssigner.h"
#include "Data/URTSBiomeAsset.h"
#include "Math/UnrealMathUtility.h"

void FRTSBiomeAssigner::Assign(FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings || !SeedManager || Settings->Biomes.Num() == 0)
    {
        return;
    }

    TArray<FBiomeSeed> Seeds = GenerateSeeds(Grid, Settings, SeedManager);
    if (Seeds.Num() == 0)
    {
        return;
    }

    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    for (int32 Y = 0; Y < H; ++Y)
    {
        for (int32 X = 0; X < W; ++X)
        {
            int32 NearestIdx = FindNearestSeedIndex(Grid, X, Y, Seeds);
            FRTSCell& Cell = Grid.GetCell(X, Y);
            Cell.BiomeID = Seeds[NearestIdx].BiomeIndex;

            // Apply biome terrain modifiers
            if (Settings->Biomes.IsValidIndex(Cell.BiomeID))
            {
                TSoftObjectPtr<URTSBiomeAsset> BiomePtr = Settings->Biomes[Cell.BiomeID];
                if (URTSBiomeAsset* Biome = BiomePtr.LoadSynchronous())
                {
                    Cell.Height = FMath::Clamp(Cell.Height + Biome->HeightBias, 0.0f, 1.0f);
                }
            }
        }
    }
}

TArray<FRTSBiomeAssigner::FBiomeSeed> FRTSBiomeAssigner::GenerateSeeds(
    const FRTSGrid& Grid,
    const URTSGenerationSettings* Settings,
    UFRTSSeedManager* SeedManager) const
{
    TArray<FBiomeSeed> Seeds;
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    // Simple deterministic seeded placement: one seed region per biome, scattered
    int32 NumBiomes = Settings->Biomes.Num();
    float Margin = FMath::Max(W, H) * 0.1f;

    for (int32 b = 0; b < NumBiomes; ++b)
    {
        FBiomeSeed Seed;
        Seed.BiomeIndex = b;
        // Deterministic random position within margins
        float X = Margin + SeedManager->RandFloat() * (W - 2.0f * Margin);
        float Y = Margin + SeedManager->RandFloat() * (H - 2.0f * Margin);
        Seed.Position = FVector2D(X, Y);
        Seeds.Add(Seed);
    }

    return Seeds;
}

int32 FRTSBiomeAssigner::FindNearestSeedIndex(const FRTSGrid& Grid, int32 X, int32 Y, const TArray<FBiomeSeed>& Seeds) const
{
    float BestDistSq = MAX_FLT;
    int32 BestIdx = 0;
    FVector2D Pos(static_cast<float>(X), static_cast<float>(Y));

    for (int32 i = 0; i < Seeds.Num(); ++i)
    {
        float DistSq = FVector2D::DistSquared(Pos, Seeds[i].Position);
        if (DistSq < BestDistSq)
        {
            BestDistSq = DistSq;
            BestIdx = i;
        }
    }

    return BestIdx;
}
