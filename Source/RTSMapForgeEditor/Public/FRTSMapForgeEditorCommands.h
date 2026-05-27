#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"

/**
 * UI command definitions for the RTS MapForge editor toolbar.
 */
class FRTSMapForgeEditorCommands : public TCommands<FRTSMapForgeEditorCommands>
{
public:
    FRTSMapForgeEditorCommands()
        : TCommands<FRTSMapForgeEditorCommands>(
            TEXT("RTSMapForge"),
            NSLOCTEXT("Contexts", "RTSMapForge", "RTS MapForge Plugin"),
            NAME_None,
            FAppStyle::GetAppStyleSetName()
        )
    {}

    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> OpenGeneratorWindow;
};
