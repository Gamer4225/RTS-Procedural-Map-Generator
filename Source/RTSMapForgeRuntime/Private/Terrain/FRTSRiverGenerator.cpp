#include "Terrain/FRTSRiverGenerator.h"
#include "Math/UnrealMathUtility.h"

void FRTSRiverGenerator::Generate(FRTSGrid& Grid, const URTSGenerationSettings* Settings, UFRTSSeedManager* SeedManager)
{
    if (!Settings || !SeedManager || Grid.Cells.Num() == 0) return;
    const float MtLevel = Settings->MountainLevel, WaterLevel = Settings->WaterLevel;
    const int32 W = Grid.Width, H = Grid.Height;

    // Find peaks
    TArray<FIntPoint> Peaks;
    for (int32 Y=1;Y<H-1;++Y) for (int32 X=1;X<W-1;++X)
    {
        float HC = Grid.GetCell(X,Y).Height;
        if (HC < MtLevel) continue;
        bool bPeak=true;
        for (int32 dy=-1;dy<=1&&bPeak;++dy) for (int32 dx=-1;dx<=1;++dx) { if (dx==0&&dy==0) continue; if (Grid.GetCell(X+dx,Y+dy).Height>HC) { bPeak=false; break; } }
        if (bPeak) Peaks.Add(FIntPoint(X,Y));
    }
    if (Peaks.Num()==0) return;

    // Select sources with spacing
    int32 NumRivers = FMath::Clamp(Settings->NumPlayers*2,2,6);
    float MinSrcDistSq = FMath::Square(FMath::Min(W,H)*0.15f);
    TArray<FIntPoint> Sources;
    for (const FIntPoint& P : Peaks)
    {
        if (Sources.Num()>=NumRivers) break;
        bool bClose=false; for (const FIntPoint& S:Sources) if (FIntPoint::DistSquared(P,S)<MinSrcDistSq){bClose=true;break;}
        if (!bClose) Sources.Add(P);
    }

    // Trace rivers (simplified gradient descent)
    int32 NB[8];
    for (const FIntPoint& Src : Sources)
    {
        int32 CX=Src.X, CY=Src.Y;
        const int32 MaxSteps=W+H;
        for (int32 Step=0;Step<MaxSteps;++Step)
        {
            // Widen: mark 3x3 as water
            for (int32 dy=-1;dy<=1;++dy) for (int32 dx=-1;dx<=1;++dx)
            { int32 NX=CX+dx,NY=CY+dy; if (Grid.IsValidCoord(NX,NY)) { FRTSCell& C=Grid.GetCell(NX,NY); C.bWater=true; C.Height=FMath::Min(C.Height,WaterLevel-0.02f); } }

            int32 Num=Grid.GetNeighborsFixed(Grid.ToIndex(CX,CY),true,NB);
            int32 BestIdx=INDEX_NONE; float BestH=MAX_FLT;
            for (int32 n=0;n<Num;++n) { const FRTSCell& NC=Grid.GetCell(NB[n]); float Score=NC.Height-(NC.bWater?0.1f:0); if (Score<BestH){BestH=Score;BestIdx=NB[n];} }
            if (BestIdx==INDEX_NONE) break;
            FIntPoint BC=Grid.ToCoord(BestIdx); CX=BC.X; CY=BC.Y;
            if (CX<=1||CX>=W-2||CY<=1||CY>=H-2) break;
        }
    }

    // Reclassify
    for (FRTSCell& C : Grid.Cells) if (C.bWater) { C.bWalkable=false; C.bBuildable=false; C.MovementCostMultiplier=0; }
}
