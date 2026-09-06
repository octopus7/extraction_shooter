#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "ShaderCore.h"
class FStylizedWaterModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        const auto Plugin=IPluginManager::Get().FindPlugin(TEXT("StylizedWater"));
        if(Plugin) AddShaderSourceDirectoryMapping(TEXT("/Plugin/StylizedWater"),FPaths::Combine(Plugin->GetBaseDir(),TEXT("Shaders")));
    }
};
IMPLEMENT_MODULE(FStylizedWaterModule,StylizedWater)
