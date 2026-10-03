#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/StringTableRegistry.h"
class FRaidLevelRuntimeModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("RaidLevelKit"));
        check(Plugin.IsValid());
        FStringTableRegistry::Get().Internal_LocTableFromFile(TEXT("RaidLevelKit.Editor"), TEXT("RaidLevelKit"),
            TEXT("Localization/RaidLevelEditor.csv"), Plugin->GetContentDir());
    }
    virtual void ShutdownModule() override
    {
        FStringTableRegistry::Get().UnregisterStringTable(TEXT("RaidLevelKit.Editor"));
    }
};
IMPLEMENT_MODULE(FRaidLevelRuntimeModule, RaidLevelRuntime)
