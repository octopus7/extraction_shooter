#if WITH_DEV_AUTOMATION_TESTS
#include "Components/ScrollBox.h"
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
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
	UTexture2D* DemoVersionRibbon = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/Game/UI/Title/T_DemoVersionRibbon.T_DemoVersionRibbon"));
	TestNotNull(TEXT("Demo version ribbon texture exists"), DemoVersionRibbon);
	if (Menu->DemoBuildImage)
	{
		TestEqual(
			TEXT("Demo marker uses the version ribbon texture"),
			Menu->DemoBuildImage->GetBrush().GetResourceObject(),
			static_cast<UObject*>(DemoVersionRibbon));
		if (const UCanvasPanelSlot* DemoSlot = Cast<UCanvasPanelSlot>(Menu->DemoBuildImage->Slot))
		{
			TestEqual(TEXT("Demo ribbon position"), DemoSlot->GetPosition(), FVector2D(113.0f, 245.0f));
			TestEqual(TEXT("Demo ribbon size"), DemoSlot->GetSize(), FVector2D(318.0f, 54.0f));
		}
		else
		{
			AddError(TEXT("Demo version ribbon must use a canvas slot"));
		}
	}
	for (const TPair<const TCHAR*, float>& Spacing : {
		TPair<const TCHAR*, float>(TEXT("StartButtonBox"), 4.0f),
		TPair<const TCHAR*, float>(TEXT("SettingsButtonBox"), -4.0f)})
	{
		const USizeBox* ButtonBox = Cast<USizeBox>(Menu->FindIntroWidget(Spacing.Key));
		const UVerticalBoxSlot* ButtonSlot = ButtonBox ? Cast<UVerticalBoxSlot>(ButtonBox->Slot) : nullptr;
		TestTrue(
			FString::Printf(TEXT("%s has a vertical-box slot"), Spacing.Key),
			ButtonSlot != nullptr);
		if (ButtonSlot)
		{
			TestEqual(
				FString::Printf(TEXT("%s bottom spacing"), Spacing.Key),
				ButtonSlot->GetPadding().Bottom,
				Spacing.Value);
		}
	}
	if (!DemoVersionRibbon)
	{
		Menu->NativeDestruct();
		Menu->RemoveFromRoot();
		return false;
	}
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
	FWidgetRenderer Renderer(false);
	// Render in linear space and let PNG export apply display gamma exactly once.
	// The widget renderer's byte target otherwise advertises sRGB for linear pixels.
	auto SaveCapture = [](UTextureRenderTarget2D* RenderTarget, const TCHAR* Name)
	{
		FImage Pixels;
		if (RenderTarget && FImageUtils::GetRenderTargetImage(RenderTarget, Pixels))
		{
			Pixels.GammaSpace = EGammaSpace::Linear;
			FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name), Pixels);
		}
	};
	UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(1920, 1080));
	FlushRenderingCommands();
	Renderer.DrawWidget(Target, Slate, FVector2D(1920, 1080), 0.0f);
	if (Target)
	{
		SaveCapture(Target, TEXT("TitleSettings.png"));
	}
	if (UScrollBox* Scroll = Cast<UScrollBox>(Menu->TitleGraphicsSettingsWidget->WidgetTree->FindWidget(TEXT("GraphicsSettingsScroll"))))
	{
		Scroll->SetScrollOffset(10000.0f);
		Renderer.DrawWidget(Target, Slate, FVector2D(1920, 1080), 0.0f);
		FlushRenderingCommands();
		SaveCapture(Target, TEXT("TitleSettingsEffects.png"));
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
		SaveCapture(Target, TEXT("TitleMain.png"));
	}
	Menu->ShowSettingsPanel();
	Menu->TickMenuTransitions(1.0f);
	TestTrue(TEXT("Re-entry restores content input"), Menu->FindIntroWidget(TEXT("SettingsPageStack"))->GetIsEnabled());
	Menu->NativeDestruct();
	Menu->RemoveFromRoot();
	return true;
}
#endif
