#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Internationalization/StringTableRegistry.h"
#include "PixelWorldCore.h"
#include "SPixelWorldPanel.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "PropertyEditorModule.h"
#include "UObject/UnrealType.h"

class FPixelWorldDetails : public IDetailCustomization
{
public:
    virtual void CustomizeDetails(IDetailLayoutBuilder& Builder) override
    {
        for(const TCHAR* Category : {TEXT("Input"),TEXT("World"),TEXT("Rules")})
            Builder.EditCategory(Category,PixelWorld::Text(*(FString(TEXT("Category."))+Category)));
    }
};

class FPixelWorldBuilderEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        const auto Plugin = IPluginManager::Get().FindPlugin(TEXT("PixelWorldBuilder"));
        FStringTableRegistry::Get().Internal_LocTableFromFile(TEXT("PixelWorldBuilder.Editor"), TEXT("PixelWorldBuilder"),
            TEXT("Localization/PixelWorldBuilder.csv"), Plugin->GetContentDir());
        UStruct* Types[] = { UPixelWorldProfile::StaticClass(), FPixelWorldColorRule::StaticStruct(), FPixelWorldVegetation::StaticStruct() };
        for(UStruct* Type : Types)
        {
            for(TFieldIterator<FProperty> It(Type,EFieldIteratorFlags::ExcludeSuper);It;++It)
            {
                It->SetMetaData(TEXT("DisplayName"),*PixelWorld::Text(*(TEXT("Field.")+It->GetName())).ToString());
                It->SetMetaData(TEXT("ToolTip"),*PixelWorld::Text(*(TEXT("Tip.")+It->GetName())).ToString());
            }
        }
        auto& Properties=FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
        Properties.RegisterCustomClassLayout(UPixelWorldProfile::StaticClass()->GetFName(),
            FOnGetDetailCustomizationInstance::CreateLambda([] { return MakeShared<FPixelWorldDetails>(); }));
        if(!IsRunningCommandlet())
        {
            FGlobalTabmanager::Get()->RegisterNomadTabSpawner(TEXT("PixelWorldBuilder"),FOnSpawnTab::CreateLambda([](const FSpawnTabArgs&)
            { return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SPixelWorldPanel)]; }))
                .SetDisplayName(PixelWorld::Text(TEXT("Tool.Title"))).SetMenuType(ETabSpawnerMenuType::Hidden);
            UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this,&FPixelWorldBuilderEditorModule::RegisterMenus));
        }
    }
    virtual void ShutdownModule() override
    {
        if(!IsRunningCommandlet())
        {
            UToolMenus::UnRegisterStartupCallback(this);
            UToolMenus::UnregisterOwner(this);
            FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TEXT("PixelWorldBuilder"));
        }
        if(auto* Properties=FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
            Properties->UnregisterCustomClassLayout(UPixelWorldProfile::StaticClass()->GetFName());
        FStringTableRegistry::Get().UnregisterStringTable(TEXT("PixelWorldBuilder.Editor"));
    }
private:
    void RegisterMenus()
    {
        FToolMenuOwnerScoped Owner(this);
        auto& Section=UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu.Tools"))->FindOrAddSection(TEXT("PixelWorldBuilder"),PixelWorld::Text(TEXT("Tool.Section")));
        Section.AddMenuEntry(TEXT("OpenPixelWorldBuilder"),PixelWorld::Text(TEXT("Tool.Menu")),PixelWorld::Text(TEXT("Tool.Description")),FSlateIcon(),
            FUIAction(FExecuteAction::CreateLambda([] { FGlobalTabmanager::Get()->TryInvokeTab(FName(TEXT("PixelWorldBuilder"))); })));
    }
};
IMPLEMENT_MODULE(FPixelWorldBuilderEditorModule, PixelWorldBuilderEditor)
