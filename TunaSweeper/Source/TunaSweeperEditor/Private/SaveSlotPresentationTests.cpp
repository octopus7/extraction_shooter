#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "Settings/TunaSweeperLanguage.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/PanelWidget.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Editor.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "AssetCompilingManager.h"
#include "RenderingThread.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Tests/AutomationCommon.h"

class FSaveSlotCaptureStep : public IAutomationLatentCommand
{
    TFunction<void()> Step;
public:
    explicit FSaveSlotCaptureStep(TFunction<void()> In):Step(MoveTemp(In)){}
    bool Update() override {Step();return true;}
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSaveSlotPresentationTest,"TunaSweeper.UI.SaveSlots.PresentationAndNavigation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSaveSlotPresentationTest::RunTest(const FString&)
{
    auto* Class=LoadClass<UTunaSweeperIntroMenuWidget>(nullptr,TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
    auto* Menu=CreateWidget<UTunaSweeperIntroMenuWidget>(GEditor->GetEditorWorldContext().World(),Class);
    if(!TestNotNull(TEXT("Intro"),Menu)) return false;
    Menu->AddToRoot(); auto Slate=Menu->TakeWidget(); Menu->BindScreenWidgets();
    for(const TCHAR* Name:{TEXT("SaveSlot1Glow"),TEXT("SaveSlot2Glow"),TEXT("SaveSlot3Glow"),TEXT("DeleteSaveSlotHoldProgressFill"),TEXT("SaveSlotPanelTitleText")})
        if(!TestNotNull(Name,Menu->FindIntroWidget(Name))) {Menu->RemoveFromRoot();return false;}
    Menu->ShowSaveSlotSelection(); Menu->TickMenuTransitions(1);
    TestEqual(TEXT("No selection hides deletion"),Menu->DeleteSaveSlotButtonBox->GetVisibility(),ESlateVisibility::Hidden);
    TestFalse(TEXT("No selection disables deletion"),Menu->DeleteSaveSlotButton->GetIsEnabled());
    TestTrue(TEXT("Primary action bound"),Menu->PrimarySaveSlotButton->OnClicked.IsBound());
    TestTrue(TEXT("Deletion hold bound"),Menu->DeleteSaveSlotButton->OnPressed.IsBound() && Menu->DeleteSaveSlotButton->OnReleased.IsBound());
    for(int32 I=1;I<=3;++I)
    {
        auto* B=CastChecked<UButton>(Menu->FindIntroWidget(*FString::Printf(TEXT("SaveSlot%dButton"),I)));
        B->OnClicked.Broadcast(); TestEqual(TEXT("Card selection"),Menu->SelectedSaveSlotIndex,I);
        auto* G=CastChecked<UImage>(Menu->FindIntroWidget(*FString::Printf(TEXT("SaveSlot%dGlow"),I)));
        float Selected=0;G->GetDynamicMaterial()->GetScalarParameterValue(TEXT("Selected"),Selected);TestEqual(TEXT("Selection controls inner glow"),Selected,1.f);
        auto* S=CastChecked<UCanvasPanelSlot>(B->GetParent()->Slot);TestTrue(TEXT("Cards occupy right half"),S->GetPosition().X>=960);
    }
    Menu->SetDeleteHoldProgress(.5f);TestEqual(TEXT("Half hold progress"),Menu->DeleteSaveSlotHoldProgressFill->GetRenderTransform().Scale.X,.5);
    TestEqual(TEXT("Hold fills from left edge"), Menu->DeleteSaveSlotHoldProgressFill->GetRenderTransformPivot().X,0.0);
    Menu->ResetDeleteHoldProgress();TestEqual(TEXT("Release clears hold progress"),Menu->DeleteSaveSlotHoldProgressFill->GetVisibility(),ESlateVisibility::Collapsed);
    Menu->DeleteSaveSlotButtonBox->SetVisibility(ESlateVisibility::Visible);
    Menu->DeleteSaveSlotButton->SetIsEnabled(true);
    Menu->ShowSaveSlotSelection();Menu->TickMenuTransitions(1);
    TestEqual(TEXT("Reopening clears stale delete action"),Menu->DeleteSaveSlotButtonBox->GetVisibility(),ESlateVisibility::Hidden);
    TestFalse(TEXT("Reopening disables stale delete action"),Menu->DeleteSaveSlotButton->GetIsEnabled());
    auto* GI=NewObject<UGameInstance>();auto* Strings=NewObject<UTunaSweeperTextSubsystem>(GI);GI->AddToRoot();Strings->AddToRoot();
    auto Renderer=MakeShared<FWidgetRenderer>(false);
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Screenshots/SaveSlots");IFileManager::Get().MakeDirectory(*Dir,true);
    FAssetCompilingManager::Get().FinishAllCompilation();
    for(auto Language:TunaSweeperLanguage::SupportedLanguages)
    {
        ADD_LATENT_AUTOMATION_COMMAND(FSaveSlotCaptureStep([=,this]() {
            auto Resolve=[&](const TCHAR* Key){auto T=Strings->ResolveText(Key,Language,FText::GetEmpty());TestFalse(Key,T.IsEmpty());return T;};
            Menu->SelectedSaveSlotIndex=1; Menu->SaveSlotActionRow->SetVisibility(ESlateVisibility::Visible);Menu->DeleteSaveSlotButtonBox->SetVisibility(ESlateVisibility::Visible);
            Menu->SetNamedText(TEXT("SaveSlotPanelTitleText"),Resolve(TEXT("ui.title.slot_select")));
            Menu->SetNamedText(TEXT("PrimarySaveSlotButtonText"),Resolve(TEXT("ui.title.primary_save_slot")));
            Menu->SetNamedText(TEXT("BackToMainMenuButtonText"),Resolve(TEXT("ui.common.back")));
            Menu->SetNamedText(TEXT("DeleteSaveSlotButtonText"),Resolve(TEXT("ui.title.delete_hold")));
            Menu->DeleteSaveSlotButton->SetIsEnabled(true);
            Menu->DeleteSaveSlotButtonText->SetColorAndOpacity(FLinearColor(.78,.43,.36,1));
            for(int32 I=1;I<=3;++I)
            {
                FTunaSweeperSaveSlotSummary Summary; Summary.SaveSlotIndex=I;Summary.bHasData=I<3;Summary.bDifficultySelected=true;Summary.DifficultyStage=I;Summary.TotalPlaySeconds=7560;
                Menu->PresentSaveSlot(I,Summary,I==1);
                Menu->SetNamedText(*FString::Printf(TEXT("SaveSlot%dText"),I),FText::Format(Resolve(TEXT("ui.title.slot_label")),FText::AsNumber(I)));
                Menu->SetNamedText(*FString::Printf(TEXT("SaveSlot%dDetail"),I),I<3?FText::Format(Resolve(TEXT("ui.title.play_time_pattern")),FText::FromString(TEXT("02:06"))):Resolve(TEXT("ui.title.empty_slot")));
                Menu->SetNamedText(*FString::Printf(TEXT("SaveSlot%dDifficulty"),I),I<3?FText::Format(Resolve(TEXT("ui.title.difficulty_pattern")),Resolve(*FString::Printf(TEXT("difficulty.stage.%d.title"),I))):Resolve(TEXT("ui.title.start_new_game")));
            }
            Menu->ForceLayoutPrepass();Renderer->DrawWidget(Slate,{1920,1080});FlushRenderingCommands();
        }));
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.4f));
        ADD_LATENT_AUTOMATION_COMMAND(FSaveSlotCaptureStep([=,this]() {
            auto* Target=Renderer->DrawWidget(Slate,{1920,1080});FlushRenderingCommands();FImage Pixels,Display;
            if(TestTrue(TEXT("Capture"),FImageUtils::GetRenderTargetImage(Target,Pixels))) {Pixels.GammaSpace=EGammaSpace::Linear;Pixels.CopyTo(Display,ERawImageFormat::BGRA8,EGammaSpace::sRGB);FImageUtils::SaveImageByExtension(*(Dir/(FString(TunaSweeperLanguage::ToLanguageCode(Language))+TEXT(".png"))),Display);}
        }));
    }
    ADD_LATENT_AUTOMATION_COMMAND(FSaveSlotCaptureStep([=,this]() {
        Menu->BackToMainMenuButton->OnClicked.Broadcast();TestEqual(TEXT("Back hides slots"),Menu->SaveSlotPanel->GetVisibility(),ESlateVisibility::Collapsed);
        Menu->NativeDestruct();Menu->RemoveFromRoot();Strings->RemoveFromRoot();GI->RemoveFromRoot();
    }));
    return true;
}
#endif
