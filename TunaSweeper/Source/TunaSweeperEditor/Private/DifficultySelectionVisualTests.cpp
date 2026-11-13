#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/DifficultySelectionWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "Settings/TunaSweeperLanguage.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Editor.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "AssetCompilingManager.h"
#include "RenderingThread.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "ShaderCompiler.h"
#include "Tests/AutomationCommon.h"

class FDifficultyCaptureStep : public IAutomationLatentCommand
{
    TFunction<void()> Step;
public:
    explicit FDifficultyCaptureStep(TFunction<void()> InStep) : Step(MoveTemp(InStep)) {}
    virtual bool Update() override { Step(); return true; }
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDifficultySelectionVisualTest,"TunaSweeper.UI.Difficulty.VisualAndNavigation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDifficultySelectionVisualTest::RunTest(const FString& Parameters)
{
    if(!FApp::CanEverRender()) { AddError(TEXT("This test requires rendering")); return false; }
    auto* Class=LoadClass<UTunaSweeperIntroMenuWidget>(nullptr,TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
    auto* Menu=CreateWidget<UTunaSweeperIntroMenuWidget>(GEditor->GetEditorWorldContext().World(),Class);
    if(!TestNotNull(TEXT("Actual intro menu"),Menu)) return false;
    Menu->AddToRoot(); auto MenuSlate=Menu->TakeWidget(); Menu->BindScreenWidgets();
    Menu->bDifficultyAdjustmentMode=true; Menu->ShowDifficultySelection();
    auto* View=Cast<UDifficultySelectionWidget>(Menu->DifficultySelectPanel);
    if(!TestNotNull(TEXT("Authored child bound to controller"),View)) { Menu->RemoveFromRoot(); return false; }
    TestEqual(TEXT("Live child retains six-language label references"),View->LabelTextures.Num(),36);
    TestNotNull(TEXT("Live child retains label material"),View->LabelMaterial.Get());
    TestTrue(TEXT("All difficulty click handlers bound"),Menu->DifficultyFarmingButton->OnClicked.IsBound() && Menu->DifficultyNormalButton->OnClicked.IsBound() && Menu->DifficultyHardButton->OnClicked.IsBound());
    // Editor-world fixture has no local player for SetUserFocus in click handlers.
    // Verify their bindings above, then exercise the shared selection path directly.
    Menu->SelectDifficultyStage(1); TestEqual(TEXT("Story selects stage 1"),Menu->SelectedDifficultyStage,1);
    Menu->SelectDifficultyStage(3); TestEqual(TEXT("Hard selects stage 3"),Menu->SelectedDifficultyStage,3);
    Menu->SelectDifficultyStage(2); TestEqual(TEXT("Normal selects stage 2"),Menu->SelectedDifficultyStage,2);
    View->Present(2,true,false); TestFalse(TEXT("Busy start disabled"),Menu->DifficultyStartButton->GetIsEnabled()); TestFalse(TEXT("Busy selection disabled"),Menu->DifficultyHardButton->GetIsEnabled());
    View->Present(2,false,true); TestEqual(TEXT("Adjustment uses apply text"),View->GetWidgetFromName(TEXT("AdjustmentStart"))->GetVisibility(),ESlateVisibility::HitTestInvisible);
    auto* PreviewGI=NewObject<UGameInstance>(); auto* Strings=NewObject<UTunaSweeperTextSubsystem>(PreviewGI);
    PreviewGI->AddToRoot(); Strings->AddToRoot();
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("Screenshots/Difficulty"); IFileManager::Get().MakeDirectory(*Dir,true);
    FAssetCompilingManager::Get().FinishAllCompilation(); FlushRenderingCommands();
    auto Renderer=MakeShared<FWidgetRenderer>(false); const auto Slate=View->TakeWidget();
    auto SetLanguage=[this,View,Strings](ETunaSweeperItemTextLanguage Language,int32 Stage)
    {
        const FString Culture=TunaSweeperLanguage::ToLanguageCode(Language);
        const TCHAR* Images[]={TEXT("TitleLabel"),TEXT("StoryLabel"),TEXT("NormalLabel"),TEXT("HardLabel"),TEXT("BackLabel"),TEXT("StartLabel")};
        const TCHAR* Keys[]={TEXT("ui.difficulty.select_title"),TEXT("difficulty.stage.1.title"),TEXT("difficulty.stage.2.title"),TEXT("difficulty.stage.3.title"),TEXT("ui.common.back"),TEXT("ui.difficulty.start")};
        View->Present(Stage,false,false);
        for(int32 I=0;I<6;++I) View->SetLabel(Images[I],Keys[I],Culture,I==5,I==0?1320.f:(I==5?470.f:(I==4?290.f:380.f)),I==0?125.f:(I>=4?62.f:80.f));
        for(int32 I=0;I<3;++I)
        {
            auto* Text=CastChecked<UTextBlock>(View->GetWidgetFromName(*FString::Printf(TEXT("Description%d"),I)));
            Text->SetText(Strings->ResolveText(FName(*FString::Printf(TEXT("difficulty.stage.%d.description"),I+1)),Language,FText::GetEmpty()));
            TestFalse(TEXT("Localized description resolves"),Text->GetText().IsEmpty()); TunaSweeperUIFont::ApplyFont(Text,Culture==TEXT("ru")?22.f:24.f);
        }
    };
    auto Capture=[this,View,Renderer,Slate,Dir](const FString& Name)
    {
        GShaderCompilingManager->FinishAllCompilation(); View->ForceLayoutPrepass();
        auto* Target=Renderer->DrawWidget(Slate,FVector2D(1920,1080)); FlushRenderingCommands();
        Renderer->DrawWidget(Target,Slate,FVector2D(1920,1080),0); FlushRenderingCommands();
        for(int32 I=0;I<3;++I) TestTrue(Name+TEXT(" description fits reserved height"),View->GetWidgetFromName(*FString::Printf(TEXT("Description%d"),I))->GetDesiredSize().Y<=140.f);
        FImage Pixels;
        if(TestTrue(TEXT("Rendered pixels"),FImageUtils::GetRenderTargetImage(Target,Pixels)))
        {
            Pixels.GammaSpace=EGammaSpace::Linear; FImage DisplayPixels;
            Pixels.CopyTo(DisplayPixels,ERawImageFormat::BGRA8,EGammaSpace::sRGB);
            TestTrue(TEXT("Saved capture"),FImageUtils::SaveImageByExtension(*(Dir/(Name+TEXT(".png"))),DisplayPixels));
        }
        TArray<FColor> Colors; Target->GameThread_GetRenderTargetResource()->ReadPixels(Colors); return Colors;
    };
    for(const auto Language:TunaSweeperLanguage::SupportedLanguages)
    {
        ADD_LATENT_AUTOMATION_COMMAND(FDifficultyCaptureStep([=]() {
            SetLanguage(Language,2); View->ForceLayoutPrepass();
            Renderer->DrawWidget(Slate,FVector2D(1920,1080)); FlushRenderingCommands();
        }));
        // Let Slate upload new font atlas pages and material resources on real engine frames.
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.35f));
        ADD_LATENT_AUTOMATION_COMMAND(FDifficultyCaptureStep([=]() { Capture(TunaSweeperLanguage::ToLanguageCode(Language)); }));
    }
    for(int32 Stage=1;Stage<=3;++Stage)
    {
        ADD_LATENT_AUTOMATION_COMMAND(FDifficultyCaptureStep([=]() { SetLanguage(ETunaSweeperItemTextLanguage::Korean,Stage); Renderer->DrawWidget(Slate,FVector2D(1920,1080)); }));
        ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.2f));
        ADD_LATENT_AUTOMATION_COMMAND(FDifficultyCaptureStep([=]() { Capture(FString::Printf(TEXT("selected_%d"),Stage)); }));
    }
    ADD_LATENT_AUTOMATION_COMMAND(FDifficultyCaptureStep([=,this]() {
        View->Present(1,false,false); auto* Image=CastChecked<UImage>(View->GetWidgetFromName(TEXT("Card0"))); float Selected=0;
        Image->GetDynamicMaterial()->GetScalarParameterValue(TEXT("Selected"),Selected); TestEqual(TEXT("Selected material follows story"),Selected,1.f);
        View->Present(3,false,false); Image->GetDynamicMaterial()->GetScalarParameterValue(TEXT("Selected"),Selected); TestEqual(TEXT("Unselected material disables story effects"),Selected,0.f);
        auto* Fish=View->GetWidgetFromName(TEXT("DriftFish0")); const FVector2D Before=Fish->GetRenderTransform().Translation;
        View->NativeTick(FGeometry(),5.f); TestTrue(TEXT("Background fish drifts"),Before!=Fish->GetRenderTransform().Translation);
        const auto FrameA=Capture(TEXT("wave_before")); View->NativeTick(FGeometry(),2.f); const auto FrameB=Capture(TEXT("wave_after"));
        int32 Changed=0;
        if(FrameA.Num()==1920*1080 && FrameB.Num()==FrameA.Num())
            for(int32 Y=650;Y<800;++Y) for(int32 X=1280;X<1740;++X) if(FrameA[Y*1920+X]!=FrameB[Y*1920+X]) ++Changed;
        TestTrue(TEXT("Selected-card wave/shine changes rendered pixels"),Changed>100);
        Menu->bDifficultyAdjustmentMode=false; Menu->DifficultyBackButton->OnClicked.Broadcast(); TestEqual(TEXT("Back hides difficulty"),View->GetVisibility(),ESlateVisibility::Collapsed);
        Menu->NativeDestruct(); Menu->RemoveFromRoot(); Strings->RemoveFromRoot(); PreviewGI->RemoveFromRoot();
    }));
    return true;
}
#endif
