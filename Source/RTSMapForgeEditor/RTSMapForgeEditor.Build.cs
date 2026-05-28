// RTSMapForgeEditor.Build.cs
// FIX Bug 3: Removed "WorkspaceMenuStructure" — it was never actually used in any
//            source file and the double-include path in RTSMapForgeEditorModule.cpp
//            would fail on most UE installs. Both includes have been removed from
//            RTSMapForgeEditorModule.cpp as well.

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
                // NOTE: "WorkspaceMenuStructure" removed — unused in all source files.
                //       The tab spawner and toolbar button do not require it.
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                // Add private-only static dependencies here as needed
            }
        );
    }
}
