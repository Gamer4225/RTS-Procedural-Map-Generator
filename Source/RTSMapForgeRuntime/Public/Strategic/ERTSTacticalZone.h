#pragma once

#include "CoreMinimal.h"
#include "ERTSTacticalZone.generated.h"

UENUM(BlueprintType)
enum class ERTSTacticalZone : uint8
{
    Unclassified     UMETA(DisplayName = "Unclassified"),
    MainBase         UMETA(DisplayName = "Main Base"),
    NatExpansion     UMETA(DisplayName = "Natural Expansion"),
    ContestedExp     UMETA(DisplayName = "Contested Expansion"),
    ChokePoint       UMETA(DisplayName = "Choke Point"),
    RiverCrossing    UMETA(DisplayName = "River Crossing"),    // NEW V1.5: tactical ford/bridge site
    OpenBattlefield  UMETA(DisplayName = "Open Battlefield"),
    HighGround       UMETA(DisplayName = "High Ground"),
    FlankRoute       UMETA(DisplayName = "Flank Route"),
    VisionControl    UMETA(DisplayName = "Vision Control"),
    ResourceCluster  UMETA(DisplayName = "Resource Cluster"),
};
