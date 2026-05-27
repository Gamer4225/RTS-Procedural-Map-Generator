using UnrealBuildTool;

public class RTSMapForgeEditor : ModuleRules
{
    public RTSMapForgeEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "RTSMapForgeRuntime",
                "Core",
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "Slate",
                "SlateCore",
                "EditorStyle",
                "LevelEditor",
                "PropertyEditor",
                "EditorFramework",
                "Projects",
                "InputCore",
                "ToolMenus"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // ... add private dependencies that you statically link with here ...
            }
        );
    }
}
