#if WITH_DEV_AUTOMATION_TESTS

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/TunaSweeperGraphicsQualityRowWidget.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperLocalizationRefreshTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::EngineFilter;

	struct FWorldContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FWorldContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};

	UWidget* FindNestedWidget(UUserWidget* Root, FName Name)
	{
		if (!Root || !Root->WidgetTree)
		{
			return nullptr;
		}
		if (UWidget* Direct = Root->WidgetTree->FindWidget(Name))
		{
			return Direct;
		}

		UWidget* Result = nullptr;
		Root->WidgetTree->ForEachWidget([&](UWidget* Widget)
		{
			if (!Result)
			{
				Result = FindNestedWidget(Cast<UUserWidget>(Widget), Name);
			}
		});
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperLocalizationRefreshTest,
	"TunaSweeper.UI.Localization.RefreshesTitleAndGraphicsLabels",
	TunaSweeperLocalizationRefreshTests::TestFlags)

bool FTunaSweeperLocalizationRefreshTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperLocalizationRefreshTests;
	(void)Parameters;

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient world exists"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
	FWorldContextAccess::Attach(Instance.Get(), &Context);
	Context.OwningGameInstance = Instance.Get();
	World->SetGameInstance(Instance.Get());
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

	World->InitializeActorsForPlay(FURL());
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Controller exists"), Controller)) return false;
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	LocalPlayer->SetControllerId(0);
	Controller->SetPlayer(LocalPlayer);
	UTunaSweeperTextSubsystem* Strings = Instance->GetSubsystem<UTunaSweeperTextSubsystem>();
	if (!TestNotNull(TEXT("Localization subsystem exists"), Strings) ||
		!TestTrue(TEXT("Localization CSV loads"), Strings->LoadTextData())) return false;

	Instance->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::English, false);
	UClass* IntroClass = LoadClass<UTunaSweeperIntroMenuWidget>(
		nullptr,
		TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
	if (!TestNotNull(TEXT("Authored intro menu class loads"), IntroClass)) return false;
	TStrongObjectPtr<UTunaSweeperIntroMenuWidget> Intro(
		CreateWidget<UTunaSweeperIntroMenuWidget>(Controller, IntroClass));
	if (!TestNotNull(TEXT("Authored intro menu is created"), Intro.Get())) return false;
	Intro->PrepareForPauseSettings();
	TSharedPtr<SWidget> IntroSlate = Intro->TakeWidget();

	for (const TPair<FName, FString>& Expected : {
		TPair<FName, FString>(TEXT("SettingsButtonText"), TEXT("Settings")),
		TPair<FName, FString>(TEXT("QuitButtonText"), TEXT("Quit")) })
	{
		UTextBlock* Label = Cast<UTextBlock>(FindNestedWidget(Intro.Get(), Expected.Key));
		if (!TestNotNull(*Expected.Key.ToString(), Label)) return false;
		TestEqual(TEXT("Title action label follows the selected language"), Label->GetText().ToString(), Expected.Value);
	}

	Instance->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::Korean, false);
	TStrongObjectPtr<UTunaSweeperGraphicsSettingsWidget> Graphics(
		CreateWidget<UTunaSweeperGraphicsSettingsWidget>(Controller, UTunaSweeperGraphicsSettingsWidget::StaticClass()));
	if (!TestNotNull(TEXT("Graphics settings widget is created"), Graphics.Get())) return false;
	TSharedPtr<SWidget> GraphicsSlate = Graphics->TakeWidget();
	Instance->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::English, false);
	Graphics->RefreshFromSettings();

	UTunaSweeperGraphicsQualityRowWidget* TextureRow = Cast<UTunaSweeperGraphicsQualityRowWidget>(
		Graphics->WidgetTree->FindWidget(TEXT("TextureQualityRow")));
	if (!TestNotNull(TEXT("Texture quality row exists"), TextureRow)) return false;
	UTextBlock* TextureLabel = Cast<UTextBlock>(TextureRow->WidgetTree->FindWidget(TEXT("OptionLabelText")));
	if (!TestNotNull(TEXT("Texture quality label exists"), TextureLabel)) return false;
	TestEqual(TEXT("Graphics quality label refreshes after a language change"), TextureLabel->GetText().ToString(), FString(TEXT("Texture")));

	GraphicsSlate.Reset();
	IntroSlate.Reset();
	return true;
}

#endif
