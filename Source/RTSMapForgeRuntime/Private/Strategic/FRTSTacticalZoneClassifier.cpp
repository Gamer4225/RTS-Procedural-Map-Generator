#include "Strategic/FRTSTacticalZoneClassifier.h"
#include "Math/UnrealMathUtility.h"

void FRTSTacticalZoneClassifier::Classify(FRTSGrid& Grid, const FRTSMapMetadata& Metadata)
{
    const int32 W = Grid.Width;
    const int32 H = Grid.Height;

    // ========================================================================
    // TACTICAL ZONE PRIORITY RULES (CRITICAL — prevents overlapping ambiguity)
    // ========================================================================
    // Priority (highest to lowest):
    //   1. MainBase        ← Set by base placer, NEVER override
    //   2. ChokePoint      ← Set by choke detector, can be overridden by Base
    //   3. NatExpansion    ← Set by expansion placer, can be overridden by Choke/Base
    //   4. ContestedExp    ← Set by expansion placer (risk > 0.6)
    //   5. ResourceCluster ← Set by resource placer, overrides HighGround/Open
    //   6. HighGround      ← Elevation-based, lowest priority strategic
    //   7. OpenBattlefield ← Default walkable
    //   8. Unclassified    ← Default unwalkable
    // ========================================================================

    // Step 1: Mark high ground candidates (elevation + neighbor comparison)
    TArray<bool> bHighGroundCandidate;
    bHighGroundCandidate.SetNumZeroed(Grid.Cells.Num());
    
    TArray<float> AvgNeighborHeight;
    AvgNeighborHeight.SetNumZeroed(Grid.Cells.Num());

    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        FIntPoint Coord = Grid.ToCoord(i);
        int32 X = Coord.X;
        int32 Y = Coord.Y;
        float Sum = 0.0f;
        int32 Count = 0;
        for (int32 dy = -1; dy <= 1; ++dy)
        {
            for (int32 dx = -1; dx <= 1; ++dx)
            {
                if (dx == 0 && dy == 0)
                {
                    continue;
                }
                if (Grid.IsValidCoord(X + dx, Y + dy))
                {
                    Sum += Grid.GetCell(X + dx, Y + dy).Height;
                    ++Count;
                }
            }
        }
        if (Count > 0)
        {
            AvgNeighborHeight[i] = Sum / static_cast<float>(Count);
        }
    }

    // Step 2: Identify high ground cells
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        const FRTSCell& Cell = Grid.Cells[i];
        if (!Cell.bWalkable || Cell.bWater)
        {
            continue;
        }

        // High ground: significantly elevated AND in a large elevated region
        if (Cell.Height > 0.65f && Cell.Height > AvgNeighborHeight[i] + 0.04f)
        {
            bHighGroundCandidate[i] = true;
        }
    }

    // Step 3: Filter small high-ground pockets (must be contiguous region > N cells)
    FilterSmallRegions(Grid, bHighGroundCandidate, /*MinSize=*/12);

    // Step 4: Apply priority-ordered classification to UNCLASSIFIED cells only
    // Already-assigned zones (Base, Expansion, Choke, Resource) are NEVER overwritten
    for (int32 i = 0; i < Grid.Cells.Num(); ++i)
    {
        FRTSCell& Cell = Grid.Cells[i];
        if (!Cell.bWalkable)
        {
            Cell.TacticalZone = ERTSTacticalZone::Unclassified;
            continue;
        }

        // PRIORITY 1-5 already set by earlier pipeline stages:
        // - MainBase (Stage 8)
        // - NatExpansion / ContestedExp (Stage 9)
        // - ChokePoint (Stage 10)
        // - ResourceCluster (Stage 10b)
        if (Cell.TacticalZone != ERTSTacticalZone::Unclassified)
        {
            continue; // Respect existing assignment
        }

        // PRIORITY 6: HighGround (only if not already strategic)
        if (bHighGroundCandidate[i])
        {
            Cell.TacticalZone = ERTSTacticalZone::HighGround;
            continue;
        }

        // PRIORITY 7: OpenBattlefield (default walkable)
        Cell.TacticalZone = ERTSTacticalZone::OpenBattlefield;
    }
}

void FRTSTacticalZoneClassifier::FilterSmallRegions(FRTSGrid& Grid, TArray<bool>& CandidateMask, int32 MinSize) const
{
    const int32 Total = Grid.Cells.Num();
    TArray<bool> Visited;
    Visited.SetNumZeroed(Total);

    int32 NeighborBuffer[8];

    for (int32 i = 0; i < Total; ++i)
    {
        if (!CandidateMask[i] || Visited[i])
        {
            continue;
        }

        // BFS this contiguous high-ground region
        TArray<int32> Stack;
        TArray<int32> RegionCells;
        Stack.Push(i);
        Visited[i] = true;

        while (Stack.Num() > 0)
        {
            int32 Current = Stack.Pop();
            RegionCells.Add(Current);

            int32 Count = Grid.GetNeighborsFixed(Current, false, NeighborBuffer);
            for (int32 n = 0; n < Count; ++n)
            {
                int32 NIdx = NeighborBuffer[n];
                if (!Visited[NIdx] && CandidateMask[NIdx])
                {
                    Visited[NIdx] = true;
                    Stack.Push(NIdx);
                }
            }
        }

        // If too small, remove from candidates
        if (RegionCells.Num() < MinSize)
        {
            for (int32 Idx : RegionCells)
            {
                CandidateMask[Idx] = false;
            }
        }
    }
}
