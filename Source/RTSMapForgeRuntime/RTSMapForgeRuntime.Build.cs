using UnrealBuildTool;

public class RTSMapForgeRuntime : ModuleRules
{
    public RTSMapForgeRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "NavigationSystem",
                "AIModule",
                "Projects",
                "Landscape",          // V1: Landscape heightmap bake
                "InstancedPlacements" // V1: HISM/ISM prop spawning
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // Landscape edit layer support (editor-only functionality in runtime for baking)
            }
        );

        // Editor-only landscape dependencies (for Landscape Bake in editor builds)
        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "EditorFramework"
                }
            );
        }

        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
            }
        );
    }
}
