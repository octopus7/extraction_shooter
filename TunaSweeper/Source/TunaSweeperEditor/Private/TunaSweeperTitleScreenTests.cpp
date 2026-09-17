#if WITH_DEV_AUTOMATION_TESTS
#include "Components/ScrollBox.h"
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/PanelWidget.h"
#include "Editor.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"
#include "UI/TunaSweeperOptionRowWidget.h"
#include "UI/TunaSweeperCheckIndicatorWidget.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "AssetCompilingManager.h"
#include "RenderingThread.h"
#include "Engine/GameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "Framework/Application/SlateApplication.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleScreenAssetTest,
	"TunaSweeper.UI.Title.ScreenAssetsAndTransitions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTitleScreenAssetTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadClass<UTunaSweeperIntroMenuWidget>(nullptr, TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
	if (!TestNotNull(TEXT("Intro class loads"), Class)) return false;
	UTunaSweeperIntroMenuWidget* Menu = CreateWidget<UTunaSweeperIntroMenuWidget>(GEditor->GetEditorWorldContext().World(), Class);
	if (!TestNotNull(TEXT("Intro instance"), Menu)) return false;
	Menu->AddToRoot();
	TSharedRef<SWidget> Slate = Menu->TakeWidget();
	Menu->BindScreenWidgets();
	for (const TCHAR* Name : { TEXT("MainMenuPanelView"), TEXT("SaveSlotPanelView"), TEXT("SettingsPanelView"), TEXT("DemoNoticePanelView"), TEXT("TitleGraphicsSettingsWidget") })
		TestNotNull(FString::Printf(TEXT("Child WBP %s"), Name), Cast<UUserWidget>(Menu->FindIntroWidget(Name)));
	const bool bCreditsButtonAbsent = TestNull(
		TEXT("Credits button is absent"),
		Menu->FindIntroWidget(TEXT("CreditsButton")));
	const bool bCreditsScreenAbsent = TestNull(
		TEXT("Credits screen is absent"),
		Menu->FindIntroWidget(TEXT("CreditsPanelView")));
	if (!bCreditsButtonAbsent || !bCreditsScreenAbsent)
	{
		Menu->NativeDestruct();
		Menu->RemoveFromRoot();
		return false;
	}
	TestNotNull(TEXT("Bound development button"), Menu->SettingsDevelopmentTabButton.Get());
	TestNotNull(TEXT("Bound graphics widget"), Menu->TitleGraphicsSettingsWidget.Get());
	if (!Menu->SettingsDevelopmentTabButton || !Menu->TitleGraphicsSettingsWidget) { Menu->RemoveFromRoot(); return false; }
	TestTrue(TEXT("Development click is bound"), Menu->SettingsDevelopmentTabButton->OnClicked.IsBound());
	TestNotNull(TEXT("Settings sidebar fade exists"), Menu->FindIntroWidget(TEXT("SettingsSidebarShade")));
	TestNotNull(TEXT("Graphics section heading exists"), Menu->TitleGraphicsSettingsWidget->WidgetTree->FindWidget(TEXT("GraphicsSectionTitleText")));
	for (const TCHAR* Name : { TEXT("ApplyGraphicsSettingsButton"), TEXT("CancelGraphicsSettingsButton") }) {
		UButton* Action=Cast<UButton>(Menu->TitleGraphicsSettingsWidget->WidgetTree->FindWidget(Name));
		TestTrue(FString::Printf(TEXT("Rounded and bound action %s"),Name),Action && Action->GetStyle().Normal.DrawAs==ESlateBrushDrawType::RoundedBox && Action->OnClicked.IsBound());
	}
	UWidget* BackTitle = Menu->FindIntroWidget(TEXT("SettingsTitleText"));
	TestTrue(TEXT("Settings title is inside back button hit area"), BackTitle && BackTitle->GetParent() && BackTitle->GetParent()->GetParent() == Menu->BackFromSettingsButton);
	TestNotNull(TEXT("Curved back arrow image exists"), Menu->FindIntroWidget(TEXT("SettingsBackArrowImage")));
	TestTrue(TEXT("Back header uses the shared borderless rounded control"), Menu->BackFromSettingsButton
		&& Menu->BackFromSettingsButton->GetStyle().Normal.DrawAs == ESlateBrushDrawType::RoundedBox
		&& Menu->BackFromSettingsButton->GetStyle().Normal.OutlineSettings.Width == 0.0f);
	if (Menu->BackFromSettingsButton) {
		TestEqual(TEXT("Back header hovered alpha"), Menu->BackFromSettingsButton->GetStyle().HoveredForeground.GetSpecifiedColor().A, 1.0f);
	}
	TestEqual(TEXT("Settings tab uses a fading image background"), Menu->SettingsDevelopmentTabButton->GetStyle().Normal.DrawAs, ESlateBrushDrawType::Image);
	for (const TCHAR* Name : { TEXT("VSyncToggleButton"), TEXT("MotionBlurToggleButton"), TEXT("DynamicResolutionToggleButton"), TEXT("HardwareRayTracingToggleButton") })
	{
		UButton* Toggle = Cast<UButton>(Menu->TitleGraphicsSettingsWidget->WidgetTree->FindWidget(Name));
		TestTrue(FString::Printf(TEXT("Checkbox remains bound %s"), Name), Toggle && Toggle->OnClicked.IsBound());
		TestNotNull(TEXT("Checkbox uses a painted indicator"), Cast<UTunaSweeperCheckIndicatorWidget>(
			Menu->TitleGraphicsSettingsWidget->WidgetTree->FindWidget(FName(*(FString(Name) + TEXT("_CheckIndicator"))))));
	}
	UTunaSweeperOptionRowWidget* Preset = Cast<UTunaSweeperOptionRowWidget>(Menu->TitleGraphicsSettingsWidget->WidgetTree->FindWidget(TEXT("PresetOptionRow")));
	TestTrue(TEXT("Compact preset selector is bound"), Preset && Preset->OnStepRequested.IsBound());
	Menu->ShowSettingsPanel();
	Menu->TickMenuTransitions(0.15f);
	UWidget* First = Menu->FindIntroWidget(TEXT("GraphicsTabButtonBox"));
	UWidget* Last = Menu->FindIntroWidget(TEXT("DevelopmentTabButtonBox"));
	TestTrue(TEXT("Entry tabs fade with stagger"), First && Last && First->GetRenderOpacity() > Last->GetRenderOpacity());
	Menu->TickMenuTransitions(1.0f);
	TestEqual(TEXT("Entry completes"), Menu->SettingsTransitionTime, -1.0f);
	TestEqual(TEXT("Title hidden after settings entry"), Menu->MainMenuPanel->GetVisibility(), ESlateVisibility::Collapsed);
	Menu->SettingsDevelopmentTabButton->OnClicked.Broadcast();
	Menu->TickMenuTransitions(0.19f);
	TestTrue(TEXT("Development selected after fade-out"), Menu->bShowingDevelopmentSettingsTab);
	Menu->TickMenuTransitions(0.20f);
	TestEqual(TEXT("Development content visible"), Menu->DevelopmentSettingsPanel->GetVisibility(), ESlateVisibility::Visible);
	Menu->RequestSettingsTab(1);
	Menu->TickMenuTransitions(0.4f);
	TestTrue(TEXT("Interface tab works"), Menu->bShowingInterfaceSettingsTab);
	Menu->RequestSettingsTab(0);
	Menu->TickMenuTransitions(0.4f);
	TestFalse(TEXT("Graphics tab restored"), Menu->bShowingInterfaceSettingsTab || Menu->bShowingDevelopmentSettingsTab);
	TestEqual(TEXT("Tab switching preserves the gradient background"), Menu->SettingsGraphicsTabButton->GetStyle().Normal.DrawAs, ESlateBrushDrawType::Image);
	TestEqual(TEXT("Selected tab keeps full background strength"), Menu->SettingsGraphicsTabButton->GetStyle().Hovered.TintColor.GetSpecifiedColor().A, 1.0f);
	TestEqual(TEXT("Inactive tab has no normal background"), Menu->SettingsInterfaceTabButton->GetStyle().Normal.TintColor.GetSpecifiedColor().A, 0.0f);
	TestEqual(TEXT("Inactive tab hover uses half background strength"), Menu->SettingsInterfaceTabButton->GetStyle().Hovered.TintColor.GetSpecifiedColor().A, 0.5f);

	// Render the actual composed UMG tree, including nested WBP controls, for visual inspection.
	// The editor-world fixture has no game instance; supply real string-table labels for the captures.
	UGameInstance* PreviewInstance = NewObject<UGameInstance>();
	UTunaSweeperTextSubsystem* PreviewStrings = NewObject<UTunaSweeperTextSubsystem>(PreviewInstance);
	Menu->SetNamedText(TEXT("SettingsTitleText"), PreviewStrings->ResolveText(
		TEXT("ui.common.back"), ETunaSweeperItemTextLanguage::Korean, FText::GetEmpty()));
	Menu->SetNamedText(TEXT("SteamDemoWishlistButtonText"), PreviewStrings->ResolveText(
		TEXT("ui.title.wishlist"), ETunaSweeperItemTextLanguage::Korean, FText::GetEmpty()));
	FAssetCompilingManager::Get().FinishAllCompilation();
	FlushRenderingCommands();
	FWidgetRenderer Renderer(true);
	UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(1920, 1080));
	FlushRenderingCommands();
	Renderer.DrawWidget(Target, Slate, FVector2D(1920, 1080), 0.0f);
	if (Target)
	{
		FImage Pixels;
		if (FImageUtils::GetRenderTargetImage(Target, Pixels))
			FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() / TEXT("Screenshots/TitleSettings.png")), Pixels);
	}
	if (UScrollBox* Scroll = Cast<UScrollBox>(Menu->TitleGraphicsSettingsWidget->WidgetTree->FindWidget(TEXT("GraphicsSettingsScroll"))))
	{
		Scroll->SetScrollOffset(10000.0f);
		Renderer.DrawWidget(Target, Slate, FVector2D(1920, 1080), 0.0f);
		FlushRenderingCommands();
		FImage Pixels;
		if (FImageUtils::GetRenderTargetImage(Target, Pixels))
			FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() / TEXT("Screenshots/TitleSettingsEffects.png")), Pixels);
	}
	Menu->BackFromSettingsButton->OnClicked.Broadcast();
	TestTrue(TEXT("Back starts reverse fade"), Menu->bSettingsExiting);
	Menu->TickMenuTransitions(1.0f);
	TestEqual(TEXT("Settings closes after exit fade"), Menu->SettingsPanel->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Main menu visible after exit"), Menu->MainMenuPanel->GetVisibility(), ESlateVisibility::Visible);
	Menu->InvalidateLayoutAndVolatility();
	FSlateApplication::Get().InvalidateAllWidgets(true);
	Slate->Invalidate(EInvalidateWidgetReason::Layout | EInvalidateWidgetReason::Paint);
	Menu->ForceLayoutPrepass();
	Renderer.DrawWidget(Target, Slate, FVector2D(1920, 1080), 0.0f);
	FlushRenderingCommands();
	if (Target)
	{
		FImage Pixels;
		if (FImageUtils::GetRenderTargetImage(Target, Pixels))
			FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() / TEXT("Screenshots/TitleMain.png")), Pixels);
	}
	Menu->ShowSettingsPanel();
	Menu->TickMenuTransitions(1.0f);
	TestTrue(TEXT("Re-entry restores content input"), Menu->FindIntroWidget(TEXT("SettingsPageStack"))->GetIsEnabled());
	Menu->NativeDestruct();
	Menu->RemoveFromRoot();
	return true;
}
#endif
