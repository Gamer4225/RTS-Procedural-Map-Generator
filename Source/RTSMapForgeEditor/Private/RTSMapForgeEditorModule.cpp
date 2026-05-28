#include "RTSMapForgeEditorModule.h"
#include "FRTSMapForgeEditorCommands.h"
#include "FRTSMapForgeEdMode.h"
#include "SRTSMapGeneratorWindow.h"
#include "LevelEditor.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
// FIX Bug 3: Removed both WorkspaceMenuStructure includes. They were unused in
//            this file and the second path ("WorkspaceMenuStructure/Public/...")
//            fails to resolve on most UE installs. The tab spawner and toolbar
//            button work correctly without them.

#define LOCTEXT_NAMESPACE "RTSMapForgeEditor"

static const FName RTSMapForgeTabId(TEXT("RTSMapForgeGeneratorWindow"));

void FRTSMapForgeEditorModule::StartupModule()
{
    // 1. Register UI commands
    RegisterEditorCommands();

    // 2. Register EdMode for viewport overlay
    FEditorModeRegistry::Get().RegisterMode<FRTSMapForgeEdMode>(
        FRTSMapForgeEdMode::EM_RTSMapForge,
        LOCTEXT("RTSMapForgeEdMode", "RTS MapForge Overlay"),
        FSlateIcon(),
        true // bVisible in mode bar
    );

    // 3. Register tab spawner
    RegisterTabSpawner();

    // 4. Register toolbar button in Level Editor
    RegisterToolbarButton();
}

void FRTSMapForgeEditorModule::ShutdownModule()
{
    // CRITICAL: Remove ONLY our extender, NOT all extenders.
    // RemoveAllExtenders() destroys other plugins' toolbar buttons.
    // We stored our extender handle during registration.
    if (FModuleManager::Get().IsModuleLoaded("LevelEditor"))
    {
        FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

        if (ToolbarExtender.IsValid())
        {
            LevelEditorModule.GetToolBarExtensibilityManager()->RemoveExtender(ToolbarExtender);
        }
        else
        {
            UE_LOG(LogTemp, Warning,
                TEXT("RTSMapForgeEditor: ToolbarExtender handle was invalid during shutdown. ")
                TEXT("Another plugin's toolbar button may have been affected if RemoveAllExtenders was used."));
        }
    }

    UnregisterTabSpawner();

    FEditorModeRegistry::Get().UnregisterMode(FRTSMapForgeEdMode::EM_RTSMapForge);

    if (FRTSMapForgeEditorCommands::IsRegistered())
    {
        FRTSMapForgeEditorCommands::Unregister();
    }
}

void FRTSMapForgeEditorModule::RegisterEditorCommands()
{
    FRTSMapForgeEditorCommands::Register();

    PluginCommands = MakeShared<FUICommandList>();
    PluginCommands->MapAction(
        FRTSMapForgeEditorCommands::Get().OpenGeneratorWindow,
        FExecuteAction::CreateRaw(this, &FRTSMapForgeEditorModule::OpenGeneratorWindow),
        FCanExecuteAction()
    );
}

void FRTSMapForgeEditorModule::RegisterTabSpawner()
{
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        RTSMapForgeTabId,
        FOnSpawnTab::CreateLambda([](const FSpawnTabArgs& Args) -> TSharedRef<SDockTab>
        {
            return SNew(SDockTab)
                .TabRole(ETabRole::NomadTab)
                .Label(LOCTEXT("TabTitle", "RTS MapForge"))
                [
                    SNew(SRTSMapGeneratorWindow)
                ];
        })
    )
    .SetDisplayName(LOCTEXT("TabTitle", "RTS MapForge"))
    .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FRTSMapForgeEditorModule::UnregisterTabSpawner()
{
    if (FGlobalTabmanager::Get()->HasNomadTabSpawner(RTSMapForgeTabId))
    {
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(RTSMapForgeTabId);
    }
}

void FRTSMapForgeEditorModule::RegisterToolbarButton()
{
    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

    // CRITICAL: Store the extender handle so we can remove ONLY ours during shutdown.
    ToolbarExtender = MakeShared<FExtender>();
    ToolbarExtender->AddToolBarExtension(
        "Play",
        EExtensionHook::After,
        PluginCommands.ToSharedRef(),
        FToolBarExtensionDelegate::CreateRaw(this, &FRTSMapForgeEditorModule::AddToolbarExtension)
    );

    LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
}

void FRTSMapForgeEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
    Builder.AddToolBarButton(
        FRTSMapForgeEditorCommands::Get().OpenGeneratorWindow,
        NAME_None,
        LOCTEXT("ToolbarLabel", "MapForge"),
        LOCTEXT("ToolbarTooltip", "Open RTS MapForge generator window"),
        FSlateIcon(FAppStyle::GetAppStyleSetName(), "LevelEditor.BuildAndSubmit")
    );
}

void FRTSMapForgeEditorModule::OpenGeneratorWindow()
{
    FGlobalTabmanager::Get()->TryInvokeTab(FTabIdentifier(RTSMapForgeTabId));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRTSMapForgeEditorModule, RTSMapForgeEditor)
