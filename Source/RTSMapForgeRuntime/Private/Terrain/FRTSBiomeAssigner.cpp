#include "Terrain/FRTSBiomeAssigner.h"
#include "Data/URTSBiomeAsset.h"
#include "Math/UnrealMathUtility.h"

void FRTSBiomeAssigner::Assign(FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings || !SeedManager || Settings->Biomes.Num() == 0) return;
    TArray<FBiomeSeed> Seeds = GenerateSeeds(Grid, Settings, SeedManager);
    if (Seeds.Num() == 0) return;
    for (int32 Y = 0; Y < Grid.Height; ++Y)
    for (int32 X = 0; X < Grid.Width; ++X)
    {
        int32 NearestIdx = FindNearestSeedIndex(Grid, X, Y, Seeds);
        FRTSCell& Cell = Grid.GetCell(X, Y);
        Cell.BiomeID = Seeds[NearestIdx].BiomeIndex;
        if (Settings->Biomes.IsValidIndex(Cell.BiomeID))
        {
            if (URTSBiomeAsset* Biome = Settings->Biomes[Cell.BiomeID].LoadSynchronous())
                Cell.Height = FMath::Clamp(Cell.Height + Biome->HeightBias, 0.0f, 1.0f);
        }
    }
}

TArray<FRTSBiomeAssigner::FBiomeSeed> FRTSBiomeAssigner::GenerateSeeds(const FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager) const
{
    TArray<FBiomeSeed> Seeds;
    float Margin = FMath::Max(Grid.Width, Grid.Height) * 0.1f;
    for (int32 b = 0; b < Settings->Biomes.Num(); ++b)
    {
        FBiomeSeed S; S.BiomeIndex = b;
        S.Position = FVector2D(Margin + SeedManager->RandFloat() * (Grid.Width - 2.0f * Margin),
                               Margin + SeedManager->RandFloat() * (Grid.Height - 2.0f * Margin));
        Seeds.Add(S);
    }
    return Seeds;
}

int32 FRTSBiomeAssigner::FindNearestSeedIndex(const FRTSGrid& Grid, int32 X, int32 Y, const TArray<FBiomeSeed>& Seeds) const
{
    float Best = MAX_FLT; int32 BestIdx = 0;
    FVector2D Pos(static_cast<float>(X), static_cast<float>(Y));
    for (int32 i = 0; i < Seeds.Num(); ++i) { float D = FVector2D::DistSquared(Pos, Seeds[i].Position); if (D < Best) { Best = D; BestIdx = i; } }
    return BestIdx;
}
