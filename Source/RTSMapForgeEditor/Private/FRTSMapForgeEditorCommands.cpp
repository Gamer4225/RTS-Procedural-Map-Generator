#include "FRTSMapForgeEditorCommands.h"

#define LOCTEXT_NAMESPACE "RTSMapForgeEditor"

FRTSMapForgeEditorCommands::FRTSMapForgeEditorCommands()
    : TCommands<FRTSMapForgeEditorCommands>(
        TEXT("RTSMapForge"),
        LOCTEXT("RTSMapForgeEditor","RTS MapForge"),
        NAME_None,
        FAppStyle::GetAppStyleSetName())
{}

void FRTSMapForgeEditorCommands::RegisterCommands()
{
    UI_COMMAND(OpenGeneratorWindow, "MapForge", "Open the RTS MapForge Generator window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
