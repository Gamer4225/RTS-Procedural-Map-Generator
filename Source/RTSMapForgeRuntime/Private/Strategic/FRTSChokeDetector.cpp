#include "Strategic/FRTSChokeDetector.h"
#include "Math/UnrealMathUtility.h"

void FRTSChokeDetector::DetectChokes(FRTSGrid& Grid, FRTSMapMetadata& Metadata, const URTSGenerationSettings* Settings)
{
    if (!Settings) return;
    const float MinW = Settings->MinChokeWidth, MaxW = Settings->MaxChokeWidth;
    TSet<uint64> Checked;
    const int32 Dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (int32 Y = 1; Y < Grid.Height-1; ++Y)
    for (int32 X = 1; X < Grid.Width-1; ++X)
    {
        FRTSCell& Cell = Grid.GetCell(X,Y);
        if (!Cell.bWalkable || Cell.RegionID == INDEX_NONE) continue;
        for (int32 d = 0; d < 4; ++d)
        {
            int32 NX=X+Dirs[d][0], NY=Y+Dirs[d][1];
            int32 NR = Grid.GetCell(NX,NY).RegionID;
            if (NR==INDEX_NONE || NR==Cell.RegionID) continue;
            uint64 Key = ((uint64)FMath::Min(Cell.RegionID,NR)<<32)|(uint64)FMath::Max(Cell.RegionID,NR);
            if (Checked.Contains(Key)) continue; Checked.Add(Key);
            int32 Band=0;
            for (int32 w=-5;w<=5;++w) { int32 SX=X+Dirs[d][1]*w, SY=Y+Dirs[d][0]*w; if (Grid.IsValidCoord(SX,SY)&&Grid.GetCell(SX,SY).bWalkable) ++Band; else break; }
            if (Band>=MinW && Band<=MaxW)
            {
                FRTSChokeInfo Ch; Ch.WidthCells=Band; Ch.RegionA=Cell.RegionID; Ch.RegionB=NR;
                Ch.Cells.Add(FIntPoint(X,Y)); Ch.Hardness=1.0f-((Band-MinW)/(MaxW-MinW));
                Metadata.Chokes.Add(Ch);
                Cell.TacticalZone=ERTSTacticalZone::ChokePoint; Grid.GetCell(NX,NY).TacticalZone=ERTSTacticalZone::ChokePoint;
            }
        }
    }
}
