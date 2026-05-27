#include "Core/URTSMapForgeSubsystem.h"
#include "Core/FRTSGenerationPipeline.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"

void URTSMapForgeSubsystem::GenerateMap(URTSGenerationSettings* Settings)
{
    if (!Settings)
    {
        LastValidationResult = FRTSValidationResult();
        LastValidationResult.Issues.Add(FRTSValidationIssue{ TEXT("Init"), TEXT("Settings null"), ERTSValidationSeverity::Critical });
        return;
    }

    FRTSGenerationPipeline Pipeline;
    Pipeline.Generate(Settings, CurrentGrid, LastMetadata, LastValidationResult, /*MaxRetries=*/10);
}

void URTSMapForgeSubsystem::GenerateMapAsync(URTSGenerationSettings* Settings, const FOnMapGenComplete& OnComplete)
{
    // V1: Synchronous wrapper. V2 will use UE::Tasks::Launch.
    GenerateMap(Settings);
    OnComplete.ExecuteIfBound(LastValidationResult);
}

FRTSCell URTSMapForgeSubsystem::GetCellAtWorldLocation(FVector WorldLocation) const
{
    FIntPoint Coord = CurrentGrid.WorldToGrid(WorldLocation);
    if (CurrentGrid.IsValidCoord(Coord.X, Coord.Y))
    {
        return CurrentGrid.GetCell(Coord.X, Coord.Y);
    }
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
    Json += FString::Printf(TEXT("  \"Height\": %d,\n"), LastMetadata.GridHeight);
    Json += FString::Printf(TEXT("  \"Bases\": %d,\n"), LastMetadata.Bases.Num());
    Json += FString::Printf(TEXT("  \"Expansions\": %d,\n"), LastMetadata.Expansions.Num());
    Json += FString::Printf(TEXT("  \"Chokes\": %d,\n"), LastMetadata.Chokes.Num());
    Json += FString::Printf(TEXT("  \"ValidationScore\": %.2f,\n"), LastValidationResult.OverallScore);
    Json += FString::Printf(TEXT("  \"Passed\": %s\n"), LastValidationResult.bPassed ? TEXT("true") : TEXT("false"));
    Json += TEXT("}\n");

    return FFileHelper::SaveStringToFile(Json, *FilePath);
}

void URTSMapForgeSubsystem::BakeToLevel()
{
    // V1: Placeholder for level actor spawning / Landscape heightmap write.
    // Will integrate with Unreal Landscape / ProceduralMesh in V1.5.
}
