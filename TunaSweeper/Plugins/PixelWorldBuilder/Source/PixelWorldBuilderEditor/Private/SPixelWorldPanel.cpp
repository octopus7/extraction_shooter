#include "SPixelWorldPanel.h"
#include "PixelWorldCore.h"
#include "PixelWorldService.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetData.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/Selection.h"
#include "Factories/DataAssetFactory.h"
#include "IDetailsView.h"
#include "PropertyEditorModule.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void SPixelWorldPanel::SetProfile(UPixelWorldProfile* InProfile)
{
    Profile.Reset(InProfile);
    Details->SetObject(InProfile);
    Status=PixelWorld::Text(TEXT("Tool.Initial"));
}

void SPixelWorldPanel::Construct(const FArguments&)
{
    FDetailsViewArgs Args;
    Args.bAllowSearch=true;
    Args.bHideSelectionTip=true;
    Args.NameAreaSettings=FDetailsViewArgs::HideNameArea;
    Details=FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor")).CreateDetailView(Args);
    SetProfile(NewObject<UPixelWorldProfile>(GetTransientPackage(),NAME_None,RF_Transactional));
    ChildSlot
    [
        SNew(SBorder).Padding(10)
        [
            SNew(SVerticalBox)
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
            [ SNew(STextBlock).Text(PixelWorld::Text(TEXT("Tool.Description"))).AutoWrapText(true) ]
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0,0,8,0)
                [ SNew(STextBlock).Text(PixelWorld::Text(TEXT("Tool.Profile"))) ]
                +SHorizontalBox::Slot().FillWidth(1)
                [ SNew(SObjectPropertyEntryBox).AllowedClass(UPixelWorldProfile::StaticClass())
                    .ObjectPath_Lambda([this] { return Profile.IsValid() && !Profile->HasAnyFlags(RF_Transient) && Profile->GetOutermost()!=GetTransientPackage() ? Profile->GetPathName() : FString(); })
                    .OnObjectChanged_Lambda([this](const FAssetData& Asset) { SetProfile(Cast<UPixelWorldProfile>(Asset.GetAsset())); }) ]
                +SHorizontalBox::Slot().AutoWidth().Padding(8,0,0,0)
                [ SNew(SButton).Text(PixelWorld::Text(TEXT("Tool.NewProfile"))).OnClicked(this,&SPixelWorldPanel::SaveProfile) ]
            ]
            +SVerticalBox::Slot().FillHeight(1) [ Details.ToSharedRef() ]
            +SVerticalBox::Slot().AutoHeight().Padding(0,8)
            [
                SNew(SHorizontalBox)
                +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
                [ SNew(SButton).Text(PixelWorld::Text(TEXT("Tool.Analyze"))).OnClicked(this,&SPixelWorldPanel::Analyze) ]
                +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)
                [ SNew(SButton).Text(PixelWorld::Text(TEXT("Tool.Generate"))).OnClicked_Lambda([this] { return Generate(false); }) ]
                +SHorizontalBox::Slot().AutoWidth()
                [ SNew(SButton).Text(PixelWorld::Text(TEXT("Tool.Replace"))).OnClicked_Lambda([this] { return Generate(true); }) ]
            ]
            +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)
            [ SNew(STextBlock).Text_Lambda([this] { return Status; }).AutoWrapText(true) ]
            +SVerticalBox::Slot().AutoHeight()
            [ SNew(STextBlock).Text(PixelWorld::Text(TEXT("Tool.Hint"))).AutoWrapText(true) ]
        ]
    ];
}

FReply SPixelWorldPanel::Analyze()
{
    if(!Profile.IsValid()) { Status=PixelWorld::Text(TEXT("Error.Profile")); return FReply::Handled(); }
    FPixelWorldPlan Plan;
    if(PixelWorld::BuildPlan(*Profile,Plan,Status)) Status=PixelWorld::Describe(*Profile,Plan);
    return FReply::Handled();
}

FReply SPixelWorldPanel::Generate(bool bReplace)
{
    if(!Profile.IsValid()) { Status=PixelWorld::Text(TEXT("Error.Profile")); return FReply::Handled(); }
    AActor* Previous=nullptr;
    if(bReplace)
    {
        USelection* Selected=GEditor->GetSelectedActors();
        Previous=Selected->Num()==1 ? Cast<AActor>(Selected->GetSelectedObject(0)) : nullptr;
        if(!PixelWorld::IsRoot(Previous)) { Status=PixelWorld::Text(TEXT("Error.Selection")); return FReply::Handled(); }
    }
    AActor* Created=nullptr;
    if(PixelWorld::Generate(GEditor->GetEditorWorldContext().World(),*Profile,Previous,Created,Status))
    {
        GEditor->SelectNone(false,true);
        GEditor->SelectActor(Created,true,true);
        GEditor->RedrawLevelEditingViewports();
        Status=PixelWorld::Text(TEXT("Tool.Success"));
    }
    return FReply::Handled();
}

FReply SPixelWorldPanel::SaveProfile()
{
    if(!Profile.IsValid()) SetProfile(NewObject<UPixelWorldProfile>(GetTransientPackage(),NAME_None,RF_Transactional));
    UDataAssetFactory* Factory=NewObject<UDataAssetFactory>();
    Factory->DataAssetClass=UPixelWorldProfile::StaticClass();
    auto& Assets=FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
    auto* Saved=Cast<UPixelWorldProfile>(Assets.CreateAssetWithDialog(TEXT("DA_PixelWorld"),TEXT("/Game"),UPixelWorldProfile::StaticClass(),Factory,NAME_None,false));
    if(Saved)
    {
        UEngine::CopyPropertiesForUnrelatedObjects(Profile.Get(),Saved);
        Saved->MarkPackageDirty();
        SetProfile(Saved);
    }
    return FReply::Handled();
}
