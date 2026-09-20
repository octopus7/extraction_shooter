// One-off editor generator; remove immediately after committing validated assets.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Components/WidgetSwitcherSlot.h"
#include "Factories/TextureFactory.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Engine/Texture2D.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Misc/FileHelper.h"
#include "Fonts/CompositeFont.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UI/TunaSweeperTutorialPopupWidget.h"

namespace TutorialDraft
{
bool Save(UObject* Asset)
{
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    Args.SaveFlags = SAVE_NoError;
    const FString File = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
    return UPackage::SavePackage(Asset->GetOutermost(), Asset, *File, Args);
}
UCanvasPanelSlot* Place(UCanvasPanel* Parent, UWidget* Child, float X, float Y, float W, float H, int32 Z=0)
{
    auto* Slot=Parent->AddChildToCanvas(Child);
    Slot->SetPosition(FVector2D(X,Y)); Slot->SetSize(FVector2D(W,H)); Slot->SetZOrder(Z);
    return Slot;
}
void Fill(UCanvasPanelSlot* Slot, FMargin Margin=FMargin(0))
{
    Slot->SetAnchors(FAnchors(0,0,1,1)); Slot->SetOffsets(Margin);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialGenerate, "TunaSweeper.UI.Tutorial.GenerateDraft",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialGenerate::RunTest(const FString& Parameters)
{
    using namespace TutorialDraft;
    const FString Path=TEXT("/Game/UI/Tutorial/WBP_TutorialPopup");
    if(FParse::Param(FCommandLine::Get(),TEXT("TutorialFixContinueWrap")))
    {
        auto* Existing=LoadObject<UWidgetBlueprint>(nullptr,*Path);
        auto* Label=Existing?Cast<UTextBlock>(Existing->WidgetTree->FindWidget(TEXT("ContinueLabel"))):nullptr;
        if(!Label) return false;
        Label->SetAutoWrapText(false);
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Existing);
        FKismetEditorUtilities::CompileBlueprint(Existing);
        return Existing->Status!=BS_Error && Save(Existing);
    }
    if (FindObject<UObject>(nullptr,*Path) || FPackageName::DoesPackageExist(Path))
    {
        AddError(TEXT("Refusing to overwrite an existing authored tutorial WBP."));
        return false;
    }
    UTexture2D* Textures[4]={};
    const TCHAR* Names[]={TEXT("Frame"),TEXT("Basics"),TEXT("Combat"),TEXT("Items")};
    for(int32 I=0;I<4;++I)
    {
        const FString Name=FString(TEXT("T_Tutorial_"))+Names[I];
        const FString TexturePath=TEXT("/Game/UI/Tutorial/")+Name;
        Textures[I]=LoadObject<UTexture2D>(nullptr,*(TexturePath+TEXT(".")+Name));
        if(!Textures[I])
        {
            UTextureFactory* Factory=NewObject<UTextureFactory>();
            Factory->SuppressImportOverwriteDialog();
            bool Canceled=false;
            Textures[I]=Cast<UTexture2D>(Factory->FactoryCreateFile(UTexture2D::StaticClass(),
                CreatePackage(*TexturePath),FName(*Name),RF_Public|RF_Standalone,
                FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../GeneratedImages/UI/Tutorial/")/ (Name+TEXT(".png"))),
                nullptr,GWarn,Canceled));
            if(!Textures[I]) { AddError(TEXT("Texture import failed: ")+Name); return false; }
            FAssetRegistryModule::AssetCreated(Textures[I]);
        }
        Textures[I]->CompressionSettings=TC_EditorIcon;
        Textures[I]->MipGenSettings=TMGS_NoMipmaps;
        Textures[I]->LODGroup=TEXTUREGROUP_UI;
        Textures[I]->SRGB=true;
        Textures[I]->PostEditChange();
        if(!Save(Textures[I])) return false;
    }
    auto* Font=NewObject<UFont>(CreatePackage(TEXT("/Game/UI/Tutorial/F_TutorialText")),TEXT("F_TutorialText"),RF_Public|RF_Standalone);
    Font->FontCacheType=EFontCacheType::Runtime;
    auto MakeFace=[&](const TCHAR* Name,const FString& Filename)->UFontFace*
    {
        TArray<uint8> Bytes;
        if(!FFileHelper::LoadFileToArray(Bytes,*Filename)) return nullptr;
        auto* Face=NewObject<UFontFace>(CreatePackage(*(FString(TEXT("/Game/UI/Tutorial/"))+Name)),Name,RF_Public|RF_Standalone);
        Face->InitializeFromBulkData(Filename,EFontHinting::Default,Bytes.GetData(),Bytes.Num());
        Face->LoadingPolicy=EFontLoadingPolicy::Inline;
        FAssetRegistryModule::AssetCreated(Face);
        return Save(Face)?Face:nullptr;
    };
    UFontFace* Regular=MakeFace(TEXT("FF_TutorialRegular"),FPaths::ProjectContentDir()/TEXT("Slate/Fonts/NanumSquareRoundR.ttf"));
    UFontFace* Bold=MakeFace(TEXT("FF_TutorialBold"),FPaths::ProjectContentDir()/TEXT("Slate/Fonts/NanumSquareRoundB.ttf"));
    UFontFace* Fallback=MakeFace(TEXT("FF_TutorialCJK"),FPaths::EngineContentDir()/TEXT("Slate/Fonts/DroidSansFallback.ttf"));
    if(!Regular||!Bold||!Fallback) { AddError(TEXT("Failed to save embedded tutorial fonts")); return false; }
    Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Emplace_GetRef(FName(TEXT("Regular"))).Font=FFontData(Regular);
    Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Emplace_GetRef(FName(TEXT("Bold"))).Font=FFontData(Bold);
    Font->GetMutableInternalCompositeFont().FallbackTypeface.Typeface.Fonts.Emplace_GetRef(FName(TEXT("Regular"))).Font=FFontData(Fallback);
    Font->GetMutableInternalCompositeFont().FallbackTypeface.Typeface.Fonts.Emplace_GetRef(FName(TEXT("Bold"))).Font=FFontData(Fallback);
    FAssetRegistryModule::AssetCreated(Font);
    if(!Save(Font)) return false;

    auto* BP=Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
        UTunaSweeperTutorialPopupWidget::StaticClass(),CreatePackage(*Path),TEXT("WBP_TutorialPopup"),
        BPTYPE_Normal,UWidgetBlueprint::StaticClass(),UWidgetBlueprintGeneratedClass::StaticClass()));
    if(!BP) return false;
    BP->bIsNewlyCreated=false;
    BP->bCanCallInitializedWithoutPlayerContext=true;
    UWidgetTree* Tree=BP->WidgetTree;
    TMap<FName,FName> Keys;
    auto Text=[&](const FString& Name,const FString& Key,int32 Size,bool Bold=false)
    {
        auto* Label=Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),FName(*Name));
        Label->SetFont(FSlateFontInfo(Font,Size,Bold?TEXT("Bold"):TEXT("Regular")));
        Label->SetColorAndOpacity(FLinearColor(0.12f,0.065f,0.025f,1));
        Label->SetJustification(ETextJustify::Center);
        Label->SetAutoWrapText(true);
        Label->SetVisibility(ESlateVisibility::HitTestInvisible);
        Keys.Add(FName(*Name),FName(*Key));
        return Label;
    };
    auto* Root=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("RootCanvas"));
    Tree->RootWidget=Root;
    auto* Dimmer=Tree->ConstructWidget<UBorder>(UBorder::StaticClass(),TEXT("Dimmer"));
    Dimmer->SetBrushColor(FLinearColor(0.012f,0.016f,0.019f,0.76f));
    Dimmer->SetVisibility(ESlateVisibility::HitTestInvisible);
    Fill(Place(Root,Dimmer,0,0,0,0));
    auto* Scale=Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(),TEXT("PopupScale"));
    Scale->SetStretch(EStretch::ScaleToFit);
    Fill(Place(Root,Scale,0,0,0,0,1),FMargin(60,32,60,32));
    auto* Size=Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),TEXT("PopupSize"));
    Size->SetWidthOverride(1440); Size->SetHeightOverride(960);
    auto* ScaleSlot=Cast<UScaleBoxSlot>(Scale->AddChild(Size));
    ScaleSlot->SetHorizontalAlignment(HAlign_Center); ScaleSlot->SetVerticalAlignment(VAlign_Center);
    auto* Canvas=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("PopupCanvas"));
    Size->AddChild(Canvas);
    auto* Frame=Tree->ConstructWidget<UImage>(UImage::StaticClass(),TEXT("CommonFrame"));
    Frame->SetBrushFromTexture(Textures[0]); Frame->SetVisibility(ESlateVisibility::HitTestInvisible);
    Fill(Place(Canvas,Frame,0,0,1440,960));
    auto* Switcher=Tree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(),TEXT("PageSwitcher"));
    Fill(Place(Canvas,Switcher,0,0,1440,960,1));
    const TCHAR* PageKeys[]={TEXT("basics"),TEXT("combat"),TEXT("items")};
    const TCHAR* Actions[][3]={{TEXT("move"),TEXT("interact"),TEXT("fire")},{TEXT("reload"),TEXT("dodge"),TEXT("sprint")},{TEXT("loot"),TEXT("inventory"),TEXT("heal")}};
    for(int32 Page=0;Page<3;++Page)
    {
        const FString Prefix=FString(Names[Page+1]);
        auto* Panel=Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),FName(*(Prefix+TEXT("Page"))));
        auto* SwitchSlot=Cast<UWidgetSwitcherSlot>(Switcher->AddChild(Panel));
        SwitchSlot->SetHorizontalAlignment(HAlign_Fill); SwitchSlot->SetVerticalAlignment(VAlign_Fill);
        Place(Panel,Text(Prefix+TEXT("PageNumber"),FString::Printf(TEXT("ui.tutorial.page.%d"),Page+1),19),560,69,320,28);
        Place(Panel,Text(Prefix+TEXT("Title"),FString(TEXT("ui.tutorial."))+PageKeys[Page]+TEXT(".title"),38,true),200,105,1040,58);
        auto* Art=Tree->ConstructWidget<UImage>(UImage::StaticClass(),FName(*(Prefix+TEXT("Illustration"))));
        Art->SetBrushFromTexture(Textures[Page+1]);
        Art->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Panel,Art,80,-12,1280,853);
        for(int32 Col=0;Col<3;++Col)
        {
            const FString Stem=Prefix+FString::Printf(TEXT("Column%d"),Col+1);
            const FString KeyStem=FString(TEXT("ui.tutorial."))+Actions[Page][Col];
            const float X=112+Col*408;
            Place(Panel,Text(Stem+TEXT("Heading"),KeyStem+TEXT(".title"),27,true),X,640,400,42);
            Place(Panel,Text(Stem+TEXT("Description"),KeyStem+TEXT(".body"),23),X+18,692,364,104);
        }
    }
    Switcher->SetActiveWidgetIndex(0);
    auto* Button=Tree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("ContinueButton"));
    FButtonStyle Style=Button->GetStyle();
    Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor(0.16f,0.085f,0.035f,1),12));
    Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.23f,0.12f,0.045f,1),12));
    Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.09f,0.045f,0.02f,1),12));
    Button->SetStyle(Style);
    auto* Continue=Text(TEXT("ContinueLabel"),TEXT("ui.common.continue"),24,true);
    Continue->SetAutoWrapText(false);
    Continue->SetColorAndOpacity(FLinearColor(1,0.94f,0.78f,1));
    auto* ButtonSlot=Cast<UButtonSlot>(Button->AddChild(Continue));
    ButtonSlot->SetHorizontalAlignment(HAlign_Center); ButtonSlot->SetVerticalAlignment(VAlign_Center);
    Place(Canvas,Button,554,824,332,58,2);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    if(BP->Status==BS_Error) { AddError(TEXT("Tutorial blueprint compilation failed")); return false; }
    auto* Defaults=CastChecked<UTunaSweeperTutorialPopupWidget>(BP->GeneratedClass->GetDefaultObject());
    Defaults->LocalizedTextKeys=Keys;
    Defaults->PreviewLanguage=ETunaSweeperItemTextLanguage::Korean;
    FAssetRegistryModule::AssetCreated(BP);
    BP->MarkPackageDirty();
    return Save(BP);
}
#endif
