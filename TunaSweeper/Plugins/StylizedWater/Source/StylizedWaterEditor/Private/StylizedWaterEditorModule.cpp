#include "StylizedWaterBodyActor.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/World.h"
#include "Framework/Commands/UIAction.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#define LOCTEXT_NAMESPACE "StylizedWaterEditor"

class FStylizedWaterEditorModule final : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        if(!IsRunningCommandlet()) UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this,&FStylizedWaterEditorModule::RegisterMenus));
    }
    virtual void ShutdownModule() override
    {
        if(UToolMenus::IsToolMenuUIEnabled()) { UToolMenus::UnRegisterStartupCallback(this); UToolMenus::UnregisterOwner(this); }
    }
private:
    void RegisterMenus()
    {
        FToolMenuOwnerScoped Owner(this);
        auto& Main=UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu")->FindOrAddSection(NAME_None);
        if(!Main.FindEntry("TunaSweeper")) Main.AddSubMenu("TunaSweeper",LOCTEXT("Menu","TunaSweeper"),FText::GetEmpty(),FNewToolMenuChoice());
        auto& Section=UToolMenus::Get()->RegisterMenu("LevelEditor.MainMenu.TunaSweeper",NAME_None,EMultiBoxType::Menu,false)->FindOrAddSection("Rendering",LOCTEXT("Rendering","Rendering"));
        const EStylizedWaterPreset Presets[]={EStylizedWaterPreset::CalmLake,EStylizedWaterPreset::GentleBeach,EStylizedWaterPreset::FlowingRiver};
        const TCHAR* Names[]={TEXT("Calm Lake"),TEXT("Gentle Beach"),TEXT("Flowing River")};
        for(int32 I=0;I<3;++I)
            Section.AddMenuEntry(FName(*FString::Printf(TEXT("AddMaskWater%d"),I)),FText::FromString(FString(TEXT("Stylized Water: Add "))+Names[I]),LOCTEXT("Tip","Place water, then edit its boundary texture and appearance in Details."),FSlateIcon(),FUIAction(FExecuteAction::CreateRaw(this,&FStylizedWaterEditorModule::AddWater,Presets[I])));
    }
    void AddWater(EStylizedWaterPreset Preset)
    {
        if(!GEditor) return;
        UWorld* World=GEditor->GetEditorWorldContext().World();
        if(!World) return;
        const FScopedTransaction Transaction(LOCTEXT("Add","Add Stylized Water"));
        FVector P=FVector::ZeroVector;
        if(AActor* Selected=GEditor->GetSelectedActors()->GetTop<AActor>()) P=Selected->GetActorLocation()+FVector(0,0,5);
        FTransform Transform(FRotator::ZeroRotator,P);
        auto* Body=World->SpawnActorDeferred<AStylizedWaterBodyActor>(AStylizedWaterBodyActor::StaticClass(),Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(!Body) return;
        Body->SetFlags(RF_Transactional);
        Body->ApplyPreset(Preset,false);
        Body->SetActorLabel(TEXT("StylizedWater"));
        Body->FinishSpawning(Transform);
        Body->MarkPackageDirty();
        GEditor->SelectNone(false,true);
        GEditor->SelectActor(Body,true,true);
    }
};
IMPLEMENT_MODULE(FStylizedWaterEditorModule,StylizedWaterEditor)
#undef LOCTEXT_NAMESPACE
