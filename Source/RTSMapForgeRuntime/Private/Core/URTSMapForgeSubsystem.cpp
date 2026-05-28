#include "Core/URTSMapForgeSubsystem.h"
#include "Core/FRTSGenerationPipeline.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

void URTSMapForgeSubsystem::GenerateMap(URTSGenerationSettings* Settings)
{
    if (!Settings) { return; }
    FRTSGenerationPipeline Pipeline;
    Pipeline.Generate(Settings, CurrentGrid, LastMetadata, LastValidationResult, 10);
}

void URTSMapForgeSubsystem::GenerateMapAsync(URTSGenerationSettings* Settings, const FOnMapGenComplete& OnComplete)
{
    GenerateMap(Settings);
    OnComplete.ExecuteIfBound(LastValidationResult);
}

FRTSCell URTSMapForgeSubsystem::GetCellAtWorldLocation(FVector WorldLocation) const
{
    FIntPoint Coord = CurrentGrid.WorldToGrid(WorldLocation);
    if (CurrentGrid.IsValidCoord(Coord.X, Coord.Y)) return CurrentGrid.GetCell(Coord.X, Coord.Y);
    return FRTSCell();
}

ERTSTacticalZone URTSMapForgeSubsystem::GetZoneAtLocation(FVector Location) const
{
    return GetCellAtWorldLocation(Location).TacticalZone;
}

bool URTSMapForgeSubsystem::ExportMetadataToJSON(FString FilePath) const
{
    FString Json;
    Json += TEXT("{\n");
    Json += FString::Printf(TEXT("  \"Seed\": %lld,\n"), LastMetadata.Seed);
    Json += FString::Printf(TEXT("  \"Width\": %d,\n"), LastMetadata.GridWidth);
    Json += FString::Printf(TEXT("  \"Height\": %d\n"), LastMetadata.GridHeight);
    Json += TEXT("}\n");
    return FFileHelper::SaveStringToFile(Json, *FilePath);
}

void URTSMapForgeSubsystem::BakeToLevel() { /* V1 placeholder */ }
