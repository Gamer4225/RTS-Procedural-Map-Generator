#include "Visualization/FRTSDebugRenderer.h"
#include "Math/UnrealMathUtility.h"
#include "Data/FRTSMapMetadata.h"

void FRTSDebugRenderer::GenerateMinimapBitmap(const FRTSGrid& Grid, ERTSDebugOverlayMode Mode, TArray<FColor>& OutBitmap, int32& OutWidth, int32& OutHeight) const
{
    const int32 Stride=GetVisualStride(Grid);
    OutWidth=FMath::Max(1,FMath::DivideAndRoundUp(Grid.Width,Stride));
    OutHeight=FMath::Max(1,FMath::DivideAndRoundUp(Grid.Height,Stride));
    OutBitmap.SetNumUninitialized(OutWidth*OutHeight);
    for (int32 Y=0;Y<OutHeight;++Y)
    {
        const int32 SY=FMath::Min(Y*Stride,Grid.Height-1);
        for (int32 X=0;X<OutWidth;++X)
        {
            const int32 SX=FMath::Min(X*Stride,Grid.Width-1);
            const FRTSCell& Cell=Grid.GetCell(SX,SY);
            FColor Color=FColor::Black;
            switch(Mode)
            {
                case ERTSDebugOverlayMode::Heightmap:     Color=GetHeightmapColor(Cell.Height); break;
                case ERTSDebugOverlayMode::WaterCliff:    Color=GetWaterCliffColor(Cell.bWater,Cell.bCliff,Cell.bWalkable); break;
                case ERTSDebugOverlayMode::Walkable:      Color=GetWalkableColor(Cell.bWalkable); break;
                case ERTSDebugOverlayMode::Buildable:     Color=GetBuildableColor(Cell.bBuildable); break;
                case ERTSDebugOverlayMode::Slope:         Color=GetSlopeColor(Cell.Slope); break;
                case ERTSDebugOverlayMode::Regions:       Color=GetRegionColor(Cell.RegionID); break;
                case ERTSDebugOverlayMode::Biomes:        Color=GetBiomeColor(Cell.BiomeID); break;
                case ERTSDebugOverlayMode::TacticalZones: Color=GetTacticalZoneColor(Cell.TacticalZone); break;
                case ERTSDebugOverlayMode::Influence:     Color=GetInfluenceColor(Cell.ControlValue); break;
                case ERTSDebugOverlayMode::CombatHeat:    Color=GetCombatHeatColor(Cell.ExposureScore); break;
                case ERTSDebugOverlayMode::ChokePoints:   Color=GetChokeColor(Cell.TacticalZone==ERTSTacticalZone::ChokePoint); break;
                default: Color=FColor::Black; break;
            }
            OutBitmap[Y*OutWidth+X]=Color;
        }
    }
}

void FRTSDebugRenderer::RenderOverlay(const FRTSGrid& Grid, ERTSDebugOverlayMode Mode, FPrimitiveDrawInterface* PDI, const FMatrix& LocalToWorld, float ZOffset) const
{
    if (!PDI||Mode==ERTSDebugOverlayMode::None) return;
    const int32 Stride=GetVisualStride(Grid);
    const float CS=Grid.CellSize;
    const FVector ZL(0,0,ZOffset);
    for (int32 Y=0;Y<Grid.Height;Y+=Stride) for (int32 X=0;X<Grid.Width;X+=Stride)
    {
        const FRTSCell& Cell=Grid.GetCell(X,Y);
        FLinearColor LC=FLinearColor::Black;
        switch(Mode)
        {
            case ERTSDebugOverlayMode::Heightmap:     LC=FLinearColor(GetHeightmapColor(Cell.Height)); break;
            case ERTSDebugOverlayMode::TacticalZones: LC=FLinearColor(GetTacticalZoneColor(Cell.TacticalZone)); break;
            case ERTSDebugOverlayMode::Walkable:      LC=FLinearColor(GetWalkableColor(Cell.bWalkable)); break;
            default: break;
        }
        FVector C=Cell.WorldPosition+ZL;
        FVector E(CS*0.45f*Stride,CS*0.45f*Stride,0);
        FColor DC=LC.ToFColor(false);
        PDI->DrawLine(C+FVector(-E.X,-E.Y,0),C+FVector(E.X,-E.Y,0),DC,SDPG_Foreground,1);
        PDI->DrawLine(C+FVector(E.X,-E.Y,0),C+FVector(E.X,E.Y,0),DC,SDPG_Foreground,1);
        PDI->DrawLine(C+FVector(E.X,E.Y,0),C+FVector(-E.X,E.Y,0),DC,SDPG_Foreground,1);
        PDI->DrawLine(C+FVector(-E.X,E.Y,0),C+FVector(-E.X,-E.Y,0),DC,SDPG_Foreground,1);
    }
}

int32 FRTSDebugRenderer::GetVisualStride(const FRTSGrid& Grid) const
{
    const int32 Total=Grid.Width*Grid.Height, Max=16384;
    if (Total<=Max||Total<=0) return 1;
    return FMath::Clamp(FMath::CeilToInt(FMath::Sqrt((float)Total/(float)Max)),1,FMath::Max(Grid.Width,Grid.Height));
}

FColor FRTSDebugRenderer::GetHeightmapColor(float H) const { uint8 V=FMath::Clamp((int32)(H*255),0,255); return FColor(V,V,V,255); }
FColor FRTSDebugRenderer::GetWaterCliffColor(bool bW,bool bC,bool bWalk) const { if(bW) return FColor(0,64,255,255); if(bC) return FColor(128,128,128,255); if(bWalk) return FColor(34,139,34,255); return FColor(160,82,45,255); }
FColor FRTSDebugRenderer::GetWalkableColor(bool bW) const { return bW?FColor(0,255,0,255):FColor(255,0,0,255); }
FColor FRTSDebugRenderer::GetBuildableColor(bool bB) const { return bB?FColor(0,200,0,255):FColor(200,0,0,255); }
FColor FRTSDebugRenderer::GetSlopeColor(float S) const { float D=FMath::RadiansToDegrees(S); uint8 V=FMath::Clamp((int32)((D/90.0f)*255),0,255); return FColor(V,255-V,0,255); }
FColor FRTSDebugRenderer::GetRegionColor(int32 ID) const { if(ID==INDEX_NONE) return FColor::Black; return FColor((ID*7919)%256,(ID*104729)%256,(ID*1299709)%256,255); }
FColor FRTSDebugRenderer::GetBiomeColor(int32 ID) const { switch(ID){case 0:return FColor::Green;case 1:return FColor::Yellow;case 2:return FColor::White;case 3:return FColor::Orange;default:return FColor::Cyan;} }
FColor FRTSDebugRenderer::GetTacticalZoneColor(ERTSTacticalZone Z) const
{
    switch(Z){ case ERTSTacticalZone::MainBase:return FColor(0,255,0,255);case ERTSTacticalZone::NatExpansion:return FColor(0,200,100,255);case ERTSTacticalZone::ContestedExp:return FColor(255,165,0,255);case ERTSTacticalZone::ChokePoint:return FColor(255,0,0,255);case ERTSTacticalZone::RiverCrossing:return FColor(255,255,0,255);case ERTSTacticalZone::OpenBattlefield:return FColor(200,200,200,255);case ERTSTacticalZone::HighGround:return FColor(139,69,19,255);case ERTSTacticalZone::FlankRoute:return FColor(128,0,128,255);case ERTSTacticalZone::VisionControl:return FColor(0,255,255,255);case ERTSTacticalZone::ResourceCluster:return FColor(255,215,0,255);default:return FColor::Black;}
}
FColor FRTSDebugRenderer::GetInfluenceColor(float V) const { if(V<0){uint8 I=FMath::Clamp((int32)(-V*255),0,255);return FColor(I,0,0,255);}else{uint8 I=FMath::Clamp((int32)(V*255),0,255);return FColor(0,0,I,255);} }
FColor FRTSDebugRenderer::GetCombatHeatColor(float E) const { uint8 V=FMath::Clamp((int32)(E*255),0,255); return FColor(V,0,255-V,255); }
FColor FRTSDebugRenderer::GetChokeColor(bool bC) const { return bC?FColor(255,0,0,255):FColor(0,0,0,0); }
FColor FRTSDebugRenderer::GetBasePlacementColor(const FRTSGrid& Grid, int32 X, int32 Y, const TArray<FRTSBaseInfo>& Bases, const TArray<FRTSExpansionInfo>& Expansions) const { return GetTacticalZoneColor(Grid.GetCell(X,Y).TacticalZone); }
