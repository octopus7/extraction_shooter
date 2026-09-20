#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetTree.h"
#include "Components/WidgetSwitcher.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Slate/WidgetRenderer.h"
#include "ImageUtils.h"
#include "AssetCompilingManager.h"
#include "RenderingThread.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/StrongObjectPtr.h"
#include "UI/TunaSweeperTutorialPopupWidget.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialAuthoredAssetTest,"TunaSweeper.UI.Tutorial.AuthoredAsset",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FTutorialAuthoredAssetTest::RunTest(const FString& Parameters)
{
    auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Tutorial/WBP_TutorialPopup.WBP_TutorialPopup"));
    if(!TestNotNull(TEXT("Editable Widget Blueprint exists"),BP)) return false;
    TestTrue(TEXT("Blueprint compiled"),BP->Status!=BS_Error && BP->GeneratedClass);
    if(!BP->GeneratedClass || !TestNotNull(TEXT("Serialized designer tree"),BP->WidgetTree.Get())) return false;
    auto* AuthoredSwitcher=Cast<UWidgetSwitcher>(BP->WidgetTree->FindWidget(TEXT("PageSwitcher")));
    if(!TestNotNull(TEXT("Designer page switcher"),AuthoredSwitcher)) return false;
    TestEqual(TEXT("Three authored tutorial pages"),AuthoredSwitcher->GetChildrenCount(),3);
    TArray<UWidget*> AuthoredNodes; BP->WidgetTree->GetAllWidgets(AuthoredNodes);
    for(const TCHAR* Name:{TEXT("Frame"),TEXT("Basics"),TEXT("Combat"),TEXT("Items")})
    {
        auto* Texture=LoadObject<UTexture2D>(nullptr,*(FString(TEXT("/Game/UI/Tutorial/T_Tutorial_"))+Name));
        if(TestNotNull(TEXT("Tutorial texture loads"),Texture))
        {
            TestEqual(TEXT("UI texture compression"),Texture->CompressionSettings,TC_EditorIcon);
            TestEqual(TEXT("No UI mipmaps"),Texture->MipGenSettings,TMGS_NoMipmaps);
        }
    }
    TStrongObjectPtr<UTunaSweeperTutorialPopupWidget> Widget(CreateWidget<UTunaSweeperTutorialPopupWidget>(
        GEditor->GetEditorWorldContext().World(),BP->GeneratedClass.Get()));
    if(!TestNotNull(TEXT("Saved WBP instantiates"),Widget.Get())) return false;
    TSharedRef<SWidget> Slate=Widget->TakeWidget();
    TArray<UWidget*> InstanceNodes; Widget->WidgetTree->GetAllWidgets(InstanceNodes);
    TestEqual(TEXT("No runtime tree construction"),InstanceNodes.Num(),AuthoredNodes.Num());
    TestEqual(TEXT("All 25 display labels have localization keys"),Widget->LocalizedTextKeys.Num(),25);
    auto* Switcher=Cast<UWidgetSwitcher>(Widget->WidgetTree->FindWidget(TEXT("PageSwitcher")));
    auto* Continue=Cast<UButton>(Widget->WidgetTree->FindWidget(TEXT("ContinueButton")));
    if(!TestNotNull(TEXT("Continue presentation button"),Continue)||!Switcher) return false;
    TestFalse(TEXT("Continue is intentionally unconnected"),Continue->OnClicked.IsBound());
    TestFalse(TEXT("Widget was not added to the viewport"),Widget->IsInViewport());
    TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
    TStrongObjectPtr<UTunaSweeperTextSubsystem> Strings(NewObject<UTunaSweeperTextSubsystem>(Owner.Get()));
    FAssetCompilingManager::Get().FinishAllCompilation();
    FlushRenderingCommands();
    FWidgetRenderer Renderer(false);
    const FString Output=FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()/TEXT("../GeneratedImages/UI/Tutorial/WidgetPreviews"));
    IFileManager::Get().MakeDirectory(*Output,true);
    const ETunaSweeperItemTextLanguage Languages[]={ETunaSweeperItemTextLanguage::Korean,ETunaSweeperItemTextLanguage::English,ETunaSweeperItemTextLanguage::Japanese};
    const TCHAR* Tags[]={TEXT("ko"),TEXT("en"),TEXT("ja")};
    const TCHAR* Pages[]={TEXT("Basics"),TEXT("Combat"),TEXT("Items")};
    for(int32 L=0;L<3;++L)
    {
        Widget->PreviewLanguage=Languages[L];
        Widget->RefreshLocalizedText();
        for(const auto& Pair:Widget->LocalizedTextKeys)
        {
            auto* Label=Cast<UTextBlock>(Widget->WidgetTree->FindWidget(Pair.Key));
            FText Expected;
            TestTrue(TEXT("CSV translation exists"),Strings->TryGetTextByKey(Pair.Value,Languages[L],Expected));
            if(TestNotNull(TEXT("Localized designer label exists"),Label))
            {
                TestFalse(TEXT("Localized label is populated"),Label->GetText().IsEmpty());
                TestEqual(TEXT("Displayed text is resolved by key"),Label->GetText().ToString(),Expected.ToString());
            }
        }
        for(int32 P=0;P<3;++P)
        {
            Switcher->SetActiveWidgetIndex(P);
            Widget->ForceLayoutPrepass();
            Slate->Invalidate(EInvalidateWidgetReason::Layout|EInvalidateWidgetReason::Paint);
            TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate,FVector2D(1920,1080)));
            FlushRenderingCommands();
            if(!TestNotNull(TEXT("UMG preview render target"),Target.Get())) return false;
            Renderer.DrawWidget(Target.Get(),Slate,FVector2D(1920,1080),0);
            FlushRenderingCommands();
            FImage Pixels;
            if(TestTrue(TEXT("Read actual UMG pixels"),FImageUtils::GetRenderTargetImage(Target.Get(),Pixels)))
            {
                Pixels.GammaSpace=EGammaSpace::Linear;
                TestTrue(TEXT("Save actual UMG preview"),FImageUtils::SaveImageByExtension(
                    *(Output/FString::Printf(TEXT("Tutorial_%s_%s.png"),Pages[P],Tags[L])),Pixels));
            }
        }
    }
    return true;
}
#endif
