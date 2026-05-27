#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Framework/Commands/UICommandList.h"

class FToolBarBuilder;
class FMenuBuilder;
class FExtender;

/**
 * Editor-only module for RTS MapForge.
 * Registers UI, toolbar, tab spawner, and the viewport overlay EdMode.
 * 
 * CRITICAL: Toolbar extender is stored as a member (ToolbarExtender)
 * so ShutdownModule can remove ONLY our extender, not all extenders.
 * RemoveAllExtenders() would destroy other plugins' toolbar buttons.
 */
class FRTSMapForgeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    void OpenGeneratorWindow();

private:
    void RegisterEditorCommands();
    void RegisterToolbarButton();
    void RegisterTabSpawner();
    void UnregisterTabSpawner();

    void AddToolbarExtension(FToolBarBuilder& Builder);

    TSharedPtr<FUICommandList> PluginCommands;
    
    // Stored extender handle for safe removal during shutdown.
    // This prevents RemoveAllExtenders() from destroying other plugins.
    TSharedPtr<FExtender> ToolbarExtender;
};
