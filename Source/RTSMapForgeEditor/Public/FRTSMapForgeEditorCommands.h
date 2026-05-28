#pragma once
#include "Framework/Commands/Commands.h"
class RTSMAPFORGEEDITOR_API FRTSMapForgeEditorCommands : public TCommands<FRTSMapForgeEditorCommands>
{
public:
    FRTSMapForgeEditorCommands();
    virtual void RegisterCommands() override;
    TSharedPtr<FUICommandInfo> OpenGeneratorWindow;
};
