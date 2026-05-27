#include "RTSMapForgeRuntimeModule.h"

#define LOCTEXT_NAMESPACE "FRTSMapForgeRuntimeModule"

void FRTSMapForgeRuntimeModule::StartupModule()
{
    // Module startup logic if needed (e.g., register gameplay tags, settings)
}

void FRTSMapForgeRuntimeModule::ShutdownModule()
{
    // Cleanup if needed
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRTSMapForgeRuntimeModule, RTSMapForgeRuntime)
