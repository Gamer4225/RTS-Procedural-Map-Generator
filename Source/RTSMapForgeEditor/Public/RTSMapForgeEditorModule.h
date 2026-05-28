#pragma once
#include "Modules/ModuleManager.h"
#include "Framework/Commands/UICommandList.h"
class FExtender;
class FToolBarBuilder;
class RTSMAPFORGEEDITOR_API FRTSMapForgeEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
private:
    TSharedPtr<FUICommandList> PluginCommands;
    TSharedPtr<FExtender>      ToolbarExtender;
    void RegisterEditorCommands();
    void RegisterTabSpawner();
    void UnregisterTabSpawner();
    void RegisterToolbarButton();
    void AddToolbarExtension(FToolBarBuilder& Builder);
    void OpenGeneratorWindow();
};
