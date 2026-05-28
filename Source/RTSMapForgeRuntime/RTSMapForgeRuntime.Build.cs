// RTSMapForgeRuntime.Build.cs
// FIX Bug 2: Removed invalid "InstancedPlacements" module (does not exist in UE5.3/5.4).
//            HISM lives in the Engine module which is already a dependency.
// FIX (Minor): Added "LandscapeEditor" under bBuildEditor so FLandscapeEditDataInterface
//              resolves correctly in editor builds.

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
                "Landscape"          // Landscape heightmap bake (header access)
                // NOTE: "InstancedPlacements" removed — it does not exist in UE5.3/5.4.
                // HierarchicalInstancedStaticMeshComponent is part of "Engine".
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // Add private-only static dependencies here as needed
            }
        );

        // Editor-only landscape write dependencies (for Landscape Bake in editor builds)
        if (Target.bBuildEditor)
        {
            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd",
                    "EditorFramework",
                    "LandscapeEditor"   // FIX: Required for FLandscapeEditDataInterface
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
