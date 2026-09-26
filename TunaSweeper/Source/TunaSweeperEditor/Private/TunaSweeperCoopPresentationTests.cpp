#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AssetCompilingManager.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"

#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/TunaSweeperOnlineCoopWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperCoopPresentation
{
	struct FWorldContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FWorldContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperCoopPresentationTest, "TunaSweeper.UI.OnlineCoop.Presentation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperCoopPresentationTest::RunTest(const FString&)
{
	UClass* Class = LoadClass<UTunaSweeperOnlineCoopWidget>(nullptr, TEXT("/Game/UI/WBP_OnlineCoop.WBP_OnlineCoop_C"));
	if (!TestNotNull(TEXT("Co-op generated widget loads with custom parent"), Class)) return false;
	TestTrue(TEXT("Real widget generated class"), Cast<UWidgetBlueprintGeneratedClass>(Class) != nullptr);
	UWorld* Staging = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/CoopStaging.CoopStaging"));
	if (!TestNotNull(TEXT("Cookable staging map exists"), Staging)) return false;
	if (!TestNotNull(TEXT("Staging game mode assigned"), Staging->GetWorldSettings()->DefaultGameMode.Get())) return false;
	TestEqual(TEXT("Independent staging game mode"), Staging->GetWorldSettings()->DefaultGameMode->GetName(), FString(TEXT("TunaSweeperCoopStagingGameMode")));
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
	TunaSweeperCoopPresentation::FWorldContextAccess::Attach(Instance.Get(), &Context);
	Context.OwningGameInstance = Instance.Get();
	World->SetGameInstance(Instance.Get());
	Instance->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Instance->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		TunaSweeperCoopPresentation::FWorldContextAccess::Attach(Instance.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	World->InitializeActorsForPlay(FURL());
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	Player->SetControllerId(0);
	Controller->SetPlayer(Player);
	if (!TestTrue(TEXT("Localized strings load"), Instance->GetSubsystem<UTunaSweeperTextSubsystem>()->LoadTextData())) return false;
	TStrongObjectPtr<UTunaSweeperOnlineCoopWidget> Widget(CreateWidget<UTunaSweeperOnlineCoopWidget>(Controller, Class));
	if (!TestNotNull(TEXT("Co-op widget instance"), Widget.Get())) return false;
	TSharedRef<SWidget> SlateWidget = Widget->TakeWidget();
	for (const TCHAR* Name : { TEXT("ConnectButton"), TEXT("HostButton"), TEXT("JoinButton"), TEXT("LeaveButton"), TEXT("CloseButton") })
	{
		UButton* Button = Cast<UButton>(Widget->GetWidgetFromName(FName(Name)));
		if (!TestNotNull(Name, Button)) return false;
		TestTrue(*FString::Printf(TEXT("%s has a bound handler"), Name), Button->OnClicked.IsBound());
		TestTrue(*FString::Printf(TEXT("%s has a visible label child"), Name), Cast<UTextBlock>(Button->GetContent()) != nullptr);
	}
	TestTrue(TEXT("State callback is reflected for dynamic delegates"), Widget->FindFunction(TEXT("HandleStateChanged")) != nullptr);
	UButton* Host = CastChecked<UButton>(Widget->GetWidgetFromName(TEXT("HostButton")));
	UButton* Join = CastChecked<UButton>(Widget->GetWidgetFromName(TEXT("JoinButton")));
	UButton* Leave = CastChecked<UButton>(Widget->GetWidgetFromName(TEXT("LeaveButton")));
	TestFalse(TEXT("Offline host requires authentication"), Host->GetIsEnabled());
	TestFalse(TEXT("Offline join requires authentication"), Join->GetIsEnabled());
	TestFalse(TEXT("Offline leave action disabled"), Leave->GetIsEnabled());
	FAssetCompilingManager::Get().FinishAllCompilation();
	UClass* MenuClass = LoadClass<UTunaSweeperIntroMenuWidget>(nullptr, TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
	if (!TestNotNull(TEXT("Title menu class loads"), MenuClass)) return false;
	TStrongObjectPtr<UTunaSweeperIntroMenuWidget> Menu(CreateWidget<UTunaSweeperIntroMenuWidget>(Controller, MenuClass));
	if (!TestNotNull(TEXT("Localized composed title menu"), Menu.Get())) return false;
	TSharedRef<SWidget> MenuSlate = Menu->TakeWidget();
	UTextBlock* EntryLabel = Cast<UTextBlock>(Menu->FindIntroWidget(TEXT("OnlineCoopButtonText")));
	if (!TestNotNull(TEXT("Runtime title co-op label exists"), EntryLabel)) return false;
	UVerticalBox* MainStack = Cast<UVerticalBox>(Menu->FindIntroWidget(TEXT("MainMenuPanel")));
	USizeBox* EntryBox = Cast<USizeBox>(Menu->FindIntroWidget(TEXT("OnlineCoopButtonBox")));
	UWidget* SettingsRow = Menu->FindIntroWidget(TEXT("SettingsButtonBox"));
	if (!TestNotNull(TEXT("Main menu stack exists"), MainStack) ||
		!TestNotNull(TEXT("Co-op row exists"), EntryBox) ||
		!TestNotNull(TEXT("Settings row exists"), SettingsRow)) return false;
	TestEqual(TEXT("Co-op appears immediately before settings"),
		MainStack->GetChildIndex(EntryBox) + 1, MainStack->GetChildIndex(SettingsRow));
	if (UWidget* LaboratoryRow = Menu->FindIntroWidget(TEXT("LaboratoryButtonBox")))
		TestEqual(TEXT("Co-op follows the laboratory row"),
			MainStack->GetChildIndex(LaboratoryRow) + 1, MainStack->GetChildIndex(EntryBox));
	const int32 InitialRowCount = MainStack->GetChildrenCount();
	Menu->EnsureOnlineCoopEntry();
	Menu->EnsureOnlineCoopEntry();
	TestEqual(TEXT("Repeated co-op construction preserves menu rows"), MainStack->GetChildrenCount(), InitialRowCount);
	int32 CoopRows = 0;
	for (UWidget* Row : MainStack->GetAllChildren())
		if (Row->GetFName() == TEXT("OnlineCoopButtonBox")) ++CoopRows;
	TestEqual(TEXT("Exactly one co-op row is present"), CoopRows, 1);

	Menu->bStartTravelPending = true;
	Menu->SetStartTravelControlsEnabled(false);
	TestFalse(TEXT("Travel locks the co-op entry"), Menu->OnlineCoopButton->GetIsEnabled());
	Menu->HandleOnlineCoopClicked();
	TestNull(TEXT("Pending travel cannot open co-op"), Menu->OnlineCoopPanel.Get());
	Menu->bStartTravelPending = false;
	Menu->SetStartTravelControlsEnabled(true);
	TestTrue(TEXT("Travel unlock restores the co-op entry"), Menu->OnlineCoopButton->GetIsEnabled());
	Menu->bPauseSettingsMode = true;
	Menu->HandleOnlineCoopClicked();
	TestNull(TEXT("Pause settings cannot open co-op"), Menu->OnlineCoopPanel.Get());
	Menu->bPauseSettingsMode = false;
	Menu->bDifficultyAdjustmentMode = true;
	Menu->HandleOnlineCoopClicked();
	TestNull(TEXT("Difficulty adjustment cannot open co-op"), Menu->OnlineCoopPanel.Get());
	Menu->bDifficultyAdjustmentMode = false;
	if (!TestNotNull(TEXT("Steam wishlist row is available for the tallest menu"), Menu->SteamDemoWishlistButtonContainer.Get()) ||
		!TestNotNull(TEXT("Steam wishlist button exists"), Menu->SteamDemoWishlistButton.Get())) return false;
	Menu->ShowMainMenu();
	Menu->TickMenuTransitions(1.f);
	FWidgetRenderer Renderer(false);
	const ETunaSweeperItemTextLanguage Languages[] = { ETunaSweeperItemTextLanguage::Korean, ETunaSweeperItemTextLanguage::English, ETunaSweeperItemTextLanguage::Japanese };
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Instance->SetCurrentTextLanguage(Languages[Index], false);
		TestEqual(TEXT("Actual title co-op entry resolves current language"), EntryLabel->GetText().ToString(), Instance->ResolveLocalizedText(TEXT("ui.coop.title"), FText::GetEmpty()).ToString());
		TestFalse(TEXT("Actual title co-op label is nonempty"), EntryLabel->GetText().IsEmpty());
		if (UTextBlock* LaboratoryLabel = Cast<UTextBlock>(Menu->FindIntroWidget(TEXT("LaboratoryButtonText"))))
		{
			TestFalse(TEXT("Actual laboratory label is nonempty"), LaboratoryLabel->GetText().IsEmpty());
			TestEqual(TEXT("Language event updates the neighboring laboratory label"), LaboratoryLabel->GetText().ToString(),
				Instance->ResolveLocalizedText(TEXT("ui.lab.title"), FText::GetEmpty()).ToString());
		}
		for (const TCHAR* Name : { TEXT("TitleText"), TEXT("DescriptionText"), TEXT("ConnectButtonText"), TEXT("HostButtonText"), TEXT("JoinButtonText"), TEXT("LeaveButtonText"), TEXT("CloseButtonText"), TEXT("InviteCodeLabel"), TEXT("InviteCodeText"), TEXT("StatusText") })
		{
			UTextBlock* Text = Cast<UTextBlock>(Widget->GetWidgetFromName(FName(Name)));
			if (!TestNotNull(Name, Text)) return false;
			TestFalse(*FString::Printf(TEXT("Language %d: %s resolves"), Index, Name), Text->GetText().IsEmpty());
		}
		for (int32 Error = 1; Error < StaticEnum<ETunaSweeperOnlineCoopError>()->NumEnums() - 1; ++Error)
		{
			const FString Key = TEXT("ui.coop.error.") + StaticEnum<ETunaSweeperOnlineCoopError>()->GetNameStringByIndex(Error);
			TestFalse(*Key, Instance->ResolveLocalizedText(FName(*Key), FText::GetEmpty()).IsEmpty());
		}
		Widget->ForceLayoutPrepass();
		TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(SlateWidget, FVector2D(1280, 720)));
		FlushRenderingCommands();
		UTextBlock* Description = CastChecked<UTextBlock>(Widget->GetWidgetFromName(TEXT("DescriptionText")));
		UTextBlock* Status = CastChecked<UTextBlock>(Widget->GetWidgetFromName(TEXT("StatusText")));
		const FGeometry DescriptionGeometry = Description->GetCachedGeometry();
		// Measure after paint without rearranging the parent: the first frame must already fit the wrapped text.
		Description->ForceLayoutPrepass();
		const FVector2D DescriptionDesiredSize = Description->GetDesiredSize();
		const float DescriptionBottom = DescriptionGeometry.LocalToAbsolute(FVector2D(0, DescriptionDesiredSize.Y)).Y;
		TestTrue(*FString::Printf(TEXT("Language %d: description fits its first-paint allocation"), Index),
			DescriptionDesiredSize.Y <= DescriptionGeometry.GetLocalSize().Y + 1.f);
		TestTrue(*FString::Printf(TEXT("Language %d: description does not overlap status on first paint"), Index),
			DescriptionBottom <= Status->GetCachedGeometry().GetAbsolutePosition().Y + 1.f);
		FImage Pixels;
		if (!TestTrue(TEXT("Co-op render readable"), FImageUtils::GetRenderTargetImage(Target.Get(), Pixels))) return false;
		Pixels.GammaSpace = EGammaSpace::Linear;
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Screenshots");
		IFileManager::Get().MakeDirectory(*Directory, true);
		TestTrue(TEXT("Co-op render saved"), FImageUtils::SaveImageByExtension(*(Directory / FString::Printf(TEXT("OnlineCoop_%d.png"), Index)), Pixels));
		const FGeometry& HostGeometry = Host->GetCachedGeometry();
		const FGeometry& JoinGeometry = Join->GetCachedGeometry();
		TestTrue(TEXT("Host button has usable width"), HostGeometry.GetLocalSize().X > 400.f);
		TestTrue(TEXT("Host button has usable height"), HostGeometry.GetLocalSize().Y >= 40.f);
		TestTrue(TEXT("Panel fits 720p"), CastChecked<UButton>(Widget->GetWidgetFromName(TEXT("CloseButton")))->GetCachedGeometry().GetAbsolutePosition().Y + CastChecked<UButton>(Widget->GetWidgetFromName(TEXT("CloseButton")))->GetCachedGeometry().GetAbsoluteSize().Y <= 720.f);
		TestTrue(TEXT("Host and join do not overlap"), HostGeometry.GetAbsolutePosition().Y + HostGeometry.GetAbsoluteSize().Y <= JoinGeometry.GetAbsolutePosition().Y);
		TestEqual(TEXT("Language event updates visible title"), CastChecked<UTextBlock>(Widget->GetWidgetFromName(TEXT("TitleText")))->GetText().ToString(), Instance->ResolveLocalizedText(TEXT("ui.coop.title"), FText::GetEmpty()).ToString());

		// Include the optional Steam action so the capture covers the tallest title menu.
		Menu->SteamDemoWishlistButtonContainer->SetVisibility(ESlateVisibility::Visible);
		Menu->SteamDemoWishlistButton->SetVisibility(ESlateVisibility::Visible);
		Menu->ForceLayoutPrepass();
		TStrongObjectPtr<UTextureRenderTarget2D> MenuTarget(Renderer.DrawWidget(MenuSlate, FVector2D(1920, 1080)));
		FlushRenderingCommands();
		FImage MenuPixels;
		if (!TestTrue(TEXT("Composed title render readable"), FImageUtils::GetRenderTargetImage(MenuTarget.Get(), MenuPixels))) return false;
		MenuPixels.GammaSpace = EGammaSpace::Linear;
		TestTrue(TEXT("Composed title render saved"), FImageUtils::SaveImageByExtension(
			*(Directory / FString::Printf(TEXT("OnlineCoop_Title_%d.png"), Index)), MenuPixels));
		for (const TCHAR* RowName : { TEXT("StartButtonBox"), TEXT("SlotSelectButtonBox"), TEXT("LaboratoryButtonBox"),
			TEXT("OnlineCoopButtonBox"), TEXT("SettingsButtonBox"), TEXT("QuitButtonBox"), TEXT("SteamDemoWishlistButtonBox") })
		{
			UWidget* Row = Menu->FindIntroWidget(RowName);
			if (!Row && FName(RowName) == TEXT("LaboratoryButtonBox")) continue;
			if (!TestNotNull(RowName, Row)) continue;
			if (Row->GetVisibility() == ESlateVisibility::Collapsed || Row->GetVisibility() == ESlateVisibility::Hidden) continue;
			const FGeometry& Geometry = Row->GetCachedGeometry();
			const FVector2D Position = Geometry.GetAbsolutePosition();
			const FVector2D Size = Geometry.GetAbsoluteSize();
			TestTrue(*FString::Printf(TEXT("Language %d: %s is laid out"), Index, RowName), Size.X > 100.f && Size.Y > 40.f);
			TestTrue(*FString::Printf(TEXT("Language %d: %s fits the title viewport"), Index, RowName),
				Position.X >= -1.f && Position.Y >= -1.f && Position.X + Size.X <= 1921.f && Position.Y + Size.Y <= 1081.f);
		}
		const FGeometry& EntryGeometry = EntryBox->GetCachedGeometry();
		const FGeometry& LabelGeometry = EntryLabel->GetCachedGeometry();
		TestTrue(TEXT("Localized co-op label fits its menu row"),
			LabelGeometry.GetAbsolutePosition().X >= EntryGeometry.GetAbsolutePosition().X &&
			LabelGeometry.GetAbsolutePosition().X + LabelGeometry.GetAbsoluteSize().X <=
				EntryGeometry.GetAbsolutePosition().X + EntryGeometry.GetAbsoluteSize().X + 1.f);
	}
	Widget->RemoveFromParent();
	Widget->ReleaseSlateResources(true);
	return true;
}
#endif
