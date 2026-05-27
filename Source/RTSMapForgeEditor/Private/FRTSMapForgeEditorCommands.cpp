#include "FRTSMapForgeEditorCommands.h"

#define LOCTEXT_NAMESPACE "RTSMapForgeCommands"

void FRTSMapForgeEditorCommands::RegisterCommands()
{
    UI_COMMAND(OpenGeneratorWindow,
        "RTS MapForge",
        "Open the RTS MapForge battlefield generator window.",
        EUserInterfaceActionType::Button,
        FInputChord());
}

#undef LOCTEXT_NAMESPACE
