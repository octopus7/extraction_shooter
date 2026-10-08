#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"

class FCanopyRimModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        AddShaderSourceDirectoryMapping(TEXT("/CanopyRim"), FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders/CanopyRim")));
    }
};
IMPLEMENT_MODULE(FCanopyRimModule, CanopyRim)
