#include "Strategic/FRTSChokeDetector.h"
#include "Math/UnrealMathUtility.h"

void FRTSChokeDetector::DetectChokes(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    if (!Settings)
    {
        return;
    }

    // V1: Simple choke detection by scanning boundaries between large regions.
    // For each cell on the edge between two different regions, measure band width.
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;
    const float MinWidth = Settings->MinChokeWidth;
    const float MaxWidth = Settings->MaxChokeWidth;

    TSet<uint64> CheckedPairs;

    for (int32 Y = 1; Y < H - 1; ++Y)
    {
        for (int32 X = 1; X < W - 1; ++X)
        {
            FRTSCell& Cell = Grid.GetCell(X, Y);
            if (!Cell.bWalkable)
            {
                continue;
            }

            int32 R = Cell.RegionID;
            if (R == INDEX_NONE)
            {
                continue;
            }

            // Check 4-neighbor for different region
            const int32 Dirs[4][2] = { {1,0}, {-1,0}, {0,1}, {0,-1} };
            for (int32 d = 0; d < 4; ++d)
            {
                int32 NX = X + Dirs[d][0];
                int32 NY = Y + Dirs[d][1];
                int32 NR = Grid.GetCell(NX, NY).RegionID;
                if (NR != INDEX_NONE && NR != R)
                {
                    uint64 PairKey = (static_cast<uint64>(FMath::Min(R, NR)) << 32) | static_cast<uint32>(FMath::Max(R, NR));
                    if (CheckedPairs.Contains(PairKey))
                    {
                        continue;
                    }
                    CheckedPairs.Add(PairKey);

                    // Measure approximate crossing width at this boundary using BFS across narrowest axis
                    // V1 simplified: count adjacent walkable cells forming a band
                    int32 BandWidth = 0;
                    for (int32 w = -5; w <= 5; ++w)
                    {
                        int32 SX = X + Dirs[d][1] * w; // perpendicular
                        int32 SY = Y + Dirs[d][0] * w;
                        if (Grid.IsValidCoord(SX, SY) && Grid.GetCell(SX, SY).bWalkable)
                        {
                            ++BandWidth;
                        }
                        else
                        {
                            break;
                        }
                    }

                    if (BandWidth >= MinWidth && BandWidth <= MaxWidth)
                    {
                        FRTSChokeInfo Choke;
                        Choke.WidthCells = BandWidth;
                        Choke.RegionA = R;
                        Choke.RegionB = NR;
                        Choke.Cells.Add(FIntPoint(X, Y));
                        Choke.Hardness = 1.0f - ((BandWidth - MinWidth) / (MaxWidth - MinWidth));
                        Metadata.Chokes.Add(Choke);

                        // Mark cells as choke tactical zone
                        Grid.GetCell(X, Y).TacticalZone = ERTSTacticalZone::ChokePoint;
                        Grid.GetCell(NX, NY).TacticalZone = ERTSTacticalZone::ChokePoint;
                    }
                }
            }
        }
    }
}
