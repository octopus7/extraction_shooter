#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BackgroundBlur.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/TunaSweeperPauseMenuWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperPauseWidgetTests
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperPauseWidgetTest,
	"TunaSweeper.UI.PauseMenu.WidgetConfirmation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperPauseWidgetTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperPauseWidgetTests;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient world exists"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
	FWorldContextAccess::Attach(Instance.Get(), &Context);
	Context.OwningGameInstance = Instance.Get();
	World->SetGameInstance(Instance.Get());
	// Skip TunaSweeper Init/Shutdown: neither the player save nor language config is written.
	// Base Init supplies the normal localization subsystem used by the actual widget.
	Instance->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Instance->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FWorldContextAccess::Attach(Instance.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	// Register spawned controllers in the world so FLocalPlayerContext can resolve them.
	World->InitializeActorsForPlay(FURL());
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Generic controller cannot execute project exit actions"), Controller)) return false;
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	LocalPlayer->SetControllerId(0);
	Controller->SetPlayer(LocalPlayer);
	UTunaSweeperTextSubsystem* Strings = Instance->GetSubsystem<UTunaSweeperTextSubsystem>();
	if (!TestNotNull(TEXT("Real localization subsystem exists"), Strings)) return false;
	if (!TestTrue(TEXT("Real localization CSV loads"), Strings->LoadTextData())) return false;

	for (const bool bRaid : {false, true})
	{
		TStrongObjectPtr<UTunaSweeperPauseMenuWidget> Widget(
			CreateWidget<UTunaSweeperPauseMenuWidget>(Controller, UTunaSweeperPauseMenuWidget::StaticClass()));
		if (!TestNotNull(TEXT("Actual pause widget is created"), Widget.Get())) return false;
		Widget->InitializePauseMenu(bRaid);
		TSharedPtr<SWidget> SlateWidget = Widget->TakeWidget();
		Widget->ForceLayoutPrepass();
		auto Button = [&](const TCHAR* Name) { return Cast<UButton>(Widget->WidgetTree->FindWidget(FName(Name))); };
		auto Text = [&](const TCHAR* Name) { return Cast<UTextBlock>(Widget->WidgetTree->FindWidget(FName(Name))); };
		UBorder* Menu = Cast<UBorder>(Widget->WidgetTree->FindWidget(TEXT("PauseMenuPanel")));
		UBorder* Confirmation = Cast<UBorder>(Widget->WidgetTree->FindWidget(TEXT("PauseConfirmationPanel")));
		UButton* Cancel = Button(TEXT("CancelExitButton"));
		UTextBlock* Warning = Text(TEXT("LossWarning"));
		if (!TestNotNull(TEXT("Menu exists"), Menu) || !TestNotNull(TEXT("Confirmation exists"), Confirmation)
			|| !TestNotNull(TEXT("Cancel exists"), Cancel) || !TestNotNull(TEXT("Warning exists"), Warning)) return false;
		TestEqual(TEXT("Confirmation starts hidden"), Confirmation->GetVisibility(), ESlateVisibility::Collapsed);
		UBackgroundBlur* Blur = Cast<UBackgroundBlur>(Widget->WidgetTree->FindWidget(TEXT("PauseBackgroundBlur")));
		if (!TestNotNull(TEXT("Pause uses a real UMG background blur"), Blur)) return false;
		TestTrue(TEXT("Background blur is active"), Blur->GetBlurStrength() > 0.0f);
		TestEqual(TEXT("Main panel has no visible backing"), Menu->GetBrushColor().A, 0.0f);
		TestEqual(TEXT("Confirmation has no visible backing"), Confirmation->GetBrushColor().A, 0.0f);
		Widget->WidgetTree->ForEachWidget([&](UWidget* Child)
		{
			if (UTextBlock* Label = Cast<UTextBlock>(Child))
			{
				TestTrue(TEXT("Every pause label has a bottom-right directional shadow"),
					Label->GetShadowOffset().X > 0.0f && Label->GetShadowOffset().Y > 0.0f);
			}
		});
		const TCHAR* ButtonNames[] = {TEXT("ResumeButton"), TEXT("SettingsButton"), TEXT("ReturnToTitleButton"), TEXT("QuitButton")};
		const TCHAR* StringKeys[] = {TEXT("ui.pause.resume"), TEXT("ui.title.settings"), TEXT("ui.pause.return_to_title"), TEXT("ui.title.quit")};
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(ButtonNames); ++Index)
		{
			UButton* Control = Button(ButtonNames[Index]);
			if (!TestNotNull(ButtonNames[Index], Control)) return false;
			TestEqual(TEXT("Buttons have rounded backgrounds"), Control->GetStyle().Normal.DrawAs, ESlateBrushDrawType::RoundedBox);
			TestEqual(TEXT("Buttons have no outline"), Control->GetStyle().Normal.OutlineSettings.Width, 0.0f);
			UTextBlock* Label = Cast<UTextBlock>(Control->GetContent());
			if (!TestNotNull(TEXT("Button has a text label"), Label)) return false;
			TestFalse(TEXT("Localized label is nonempty"), Label->GetText().IsEmpty());
			TestEqual(TEXT("Label comes from the real UI string key"), Label->GetText().ToString(),
				Instance->ResolveLocalizedText(FName(StringKeys[Index]), FText::GetEmpty()).ToString());
		}
		for (const TCHAR* ExitButton : {TEXT("ReturnToTitleButton"), TEXT("QuitButton")})
		{
			Button(ExitButton)->OnClicked.Broadcast();
			TestEqual(TEXT("Selecting exit opens confirmation"), Confirmation->GetVisibility(), ESlateVisibility::Visible);
			TestEqual(TEXT("Base menu is unavailable behind confirmation"), Menu->GetVisibility(), ESlateVisibility::Collapsed);
			TestEqual(TEXT("Warning matches raid or bunker loss policy"), Warning->GetText().ToString(),
				Instance->ResolveLocalizedText(bRaid ? TEXT("ui.pause.raid_warning") : TEXT("ui.pause.bunker_warning"), FText::GetEmpty()).ToString());
			Widget->ShowExitFailure();
			TestEqual(TEXT("Save failure stays in confirmation"), Confirmation->GetVisibility(), ESlateVisibility::Visible);
			TestEqual(TEXT("Save failure displays localized recovery guidance"), Warning->GetText().ToString(),
				Instance->ResolveLocalizedText(TEXT("ui.pause.save_failed"), FText::GetEmpty()).ToString());
			Cancel->OnClicked.Broadcast();
			TestEqual(TEXT("Cancel restores menu"), Menu->GetVisibility(), ESlateVisibility::Visible);
			TestEqual(TEXT("Cancel hides confirmation"), Confirmation->GetVisibility(), ESlateVisibility::Collapsed);
			TestFalse(TEXT("Cancel does not request world travel"), World->bIsTearingDown);
		}
		SlateWidget.Reset();
	}
	return true;
}
#endif
