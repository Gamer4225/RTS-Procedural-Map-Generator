#pragma once
#include "Modules/ModuleManager.h"
class FRTSMapForgeRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
