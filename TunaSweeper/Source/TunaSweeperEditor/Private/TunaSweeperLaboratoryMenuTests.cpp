#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Editor.h"
#include "AssetCompilingManager.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/TunaSweeperIntroMenuWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperLaboratoryMenuTest,
	"TunaSweeper.UI.Title.LaboratoryNavigation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperLaboratoryMenuTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadClass<UTunaSweeperIntroMenuWidget>(nullptr, TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
	if (!TestNotNull(TEXT("Title asset loads"), Class)) return false;
	UTunaSweeperIntroMenuWidget* Menu = CreateWidget<UTunaSweeperIntroMenuWidget>(GEditor->GetEditorWorldContext().World(), Class);
	if (!TestNotNull(TEXT("Title widget exists"), Menu)) return false;
	Menu->AddToRoot();
	TSharedRef<SWidget> Slate = Menu->TakeWidget();
	Menu->BindScreenWidgets();
	UVerticalBox* Stack = Cast<UVerticalBox>(Menu->FindIntroWidget(TEXT("MainMenuPanel")));
	TestNotNull(TEXT("Main menu button stack exists"), Stack);
	TestNotNull(TEXT("Laboratory menu button exists"), Menu->LaboratoryButton.Get());
	UGameInstance* PreviewInstance = NewObject<UGameInstance>();
	UTunaSweeperTextSubsystem* Strings = NewObject<UTunaSweeperTextSubsystem>(PreviewInstance);
	FAssetCompilingManager::Get().FinishAllCompilation();
	FlushRenderingCommands();
	FWidgetRenderer Renderer(false);
	auto Capture = [&](const TCHAR* Name)
	{
		Menu->ForceLayoutPrepass();
		UTextureRenderTarget2D* Target = Renderer.DrawWidget(Slate, FVector2D(1920, 1080));
		FlushRenderingCommands();
		FImage Pixels;
		if (Target && FImageUtils::GetRenderTargetImage(Target, Pixels))
		{
			Pixels.GammaSpace = EGammaSpace::Linear;
			FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() / TEXT("Screenshots") / Name), Pixels);
		}
	};
	auto Localize = [&](ETunaSweeperItemTextLanguage Language)
	{
		const TPair<const TCHAR*, const TCHAR*> Keys[] = {
			{TEXT("StartButtonText"), TEXT("ui.title.new_game")},
			{TEXT("SlotSelectButtonText"), TEXT("ui.title.slot_select")},
			{TEXT("SettingsButtonText"), TEXT("ui.title.settings")},
			{TEXT("QuitButtonText"), TEXT("ui.title.quit")},
			{TEXT("SteamDemoWishlistButtonText"), TEXT("ui.title.wishlist")},
			{TEXT("LaboratoryButtonText"), TEXT("ui.lab.title")},
			{TEXT("LaboratoryTitleText"), TEXT("ui.lab.title")},
			{TEXT("LaboratoryDescriptionText"), TEXT("ui.lab.description")},
			{TEXT("BossDevelopmentButtonText"), TEXT("ui.lab.development")},
			{TEXT("BossSinglePlayerButtonText"), TEXT("ui.lab.single")},
			{TEXT("BossMultiplayerButtonText"), TEXT("ui.lab.multiplayer")},
			{TEXT("LaboratoryMultiplayerStatusText"), TEXT("ui.lab.in_development")},
			{TEXT("LaboratoryBackButtonText"), TEXT("ui.common.back")}};
		for (const auto& Key : Keys)
		{
			const FText Text = Strings->ResolveText(Key.Value, Language, FText::GetEmpty());
			TestFalse(TEXT("Localized text is populated"), Text.IsEmpty());
			Menu->SetNamedText(Key.Key, Text);
		}
	};
	if (Stack && Menu->LaboratoryButton)
	{
		const int32 Count = Stack->GetChildrenCount();
		Menu->EnsureLaboratoryMenu();
		Menu->EnsureLaboratoryMenu();
		TestEqual(TEXT("Repeated construction does not duplicate menu entries"), Stack->GetChildrenCount(), Count);
		TestTrue(TEXT("Lab appears before settings"),
			Stack->GetChildIndex(Menu->FindIntroWidget(TEXT("LaboratoryButtonBox"))) <
			Stack->GetChildIndex(Menu->FindIntroWidget(TEXT("SettingsButtonBox"))));
		Localize(ETunaSweeperItemTextLanguage::Korean);
		Capture(TEXT("BossLaboratoryMainMenu.png"));
		Menu->LaboratoryButton->OnClicked.Broadcast();
		TestTrue(TEXT("Laboratory submenu opens"), Menu->IsLaboratoryVisible());
		TestEqual(TEXT("Main menu hides"), Menu->MainMenuPanel->GetVisibility(), ESlateVisibility::Collapsed);
		TestTrue(TEXT("Development action is bound"), Menu->BossDevelopmentButton && Menu->BossDevelopmentButton->OnClicked.IsBound());
		TestTrue(TEXT("Solo action is bound"), Menu->BossSinglePlayerButton && Menu->BossSinglePlayerButton->OnClicked.IsBound());
		TestTrue(TEXT("Multiplayer placeholder remains clickable"), Menu->BossMultiplayerButton && Menu->BossMultiplayerButton->GetIsEnabled());
		Menu->TickMenuTransitions(1.0f);
		Localize(ETunaSweeperItemTextLanguage::Korean);
		Capture(TEXT("BossLaboratoryMenuKorean.png"));
		Localize(ETunaSweeperItemTextLanguage::English);
		Capture(TEXT("BossLaboratoryMenuEnglish.png"));
		Localize(ETunaSweeperItemTextLanguage::Japanese);
		Capture(TEXT("BossLaboratoryMenuJapanese.png"));
		Menu->HandleBossMultiplayerClicked();
		TestTrue(TEXT("Multiplayer keeps laboratory menu open"), Menu->IsLaboratoryVisible());
		TestFalse(TEXT("Multiplayer does not start travel"), Menu->bStartTravelPending);
		Menu->HandleLaboratoryBackClicked();
		TestFalse(TEXT("Back closes laboratory"), Menu->IsLaboratoryVisible());
		TestEqual(TEXT("Back restores title menu"), Menu->MainMenuPanel->GetVisibility(), ESlateVisibility::Visible);
	}
	Menu->NativeDestruct();
	Menu->RemoveFromRoot();
	return true;
}
#endif
