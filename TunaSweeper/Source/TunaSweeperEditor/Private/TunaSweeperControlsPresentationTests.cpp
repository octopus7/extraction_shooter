#if WITH_DEV_AUTOMATION_TESTS

#include "AssetCompilingManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "ImageUtils.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "RenderingThread.h"
#include "Slate/WidgetRenderer.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/TunaSweeperCheckIndicatorWidget.h"
#include "UI/TunaSweeperGraphicsSettingsWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperOptionRowWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperControlsPresentationTests
{
	struct FWorldContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FWorldContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};

	UWidget* Find(UUserWidget* Root, FName Name)
	{
		if (!Root || !Root->WidgetTree) return nullptr;
		UWidget* Result = nullptr;
		Root->WidgetTree->ForEachWidget([&](UWidget* Widget)
		{
			if (Result) return;
			if (Widget->GetFName() == Name) Result = Widget;
			else if (UUserWidget* Child = Cast<UUserWidget>(Widget)) Result = Find(Child, Name);
		});
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperControlsPresentationTest,
	"TunaSweeper.UI.Controls.LocalizedPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperControlsPresentationTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperControlsPresentationTests;
	const FString OriginalCulture = FInternationalization::Get().GetCurrentCulture()->GetName();
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient UI world exists"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
	FWorldContextAccess::Attach(Instance.Get(), &Context);
	Context.OwningGameInstance = Instance.Get();
	World->SetGameInstance(Instance.Get());
	// Base initialization supplies real localization without loading or writing player saves.
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
		FInternationalization::Get().SetCurrentCulture(OriginalCulture);
	};
	World->InitializeActorsForPlay(FURL());
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("UI controller exists"), Controller)) return false;
	ULocalPlayer* Player = NewObject<ULocalPlayer>(GEngine);
	Player->SetControllerId(0);
	Controller->SetPlayer(Player);
	UTunaSweeperTextSubsystem* Strings = Instance->GetSubsystem<UTunaSweeperTextSubsystem>();
	if (!TestNotNull(TEXT("Real text subsystem exists"), Strings) || !Strings->LoadTextData()) return false;
	UClass* MenuClass = LoadClass<UTunaSweeperIntroMenuWidget>(nullptr, TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
	if (!TestNotNull(TEXT("Authored menu class loads"), MenuClass)) return false;

	const ETunaSweeperItemTextLanguage Languages[] = {
		ETunaSweeperItemTextLanguage::Korean, ETunaSweeperItemTextLanguage::English, ETunaSweeperItemTextLanguage::Japanese };
	const TCHAR* LanguageNames[] = { TEXT("ko"), TEXT("en"), TEXT("ja") };
	for (int32 LanguageIndex = 0; LanguageIndex < UE_ARRAY_COUNT(Languages); ++LanguageIndex)
	{
		Instance->SetCurrentTextLanguage(Languages[LanguageIndex], false);
		TStrongObjectPtr<UTunaSweeperIntroMenuWidget> Menu(CreateWidget<UTunaSweeperIntroMenuWidget>(Controller, MenuClass));
		if (!TestNotNull(TEXT("Actual composed menu exists"), Menu.Get())) return false;
		Menu->PrepareForPauseSettings();
		TSharedRef<SWidget> Slate = Menu->TakeWidget();
		Menu->ForceLayoutPrepass();
		UTunaSweeperGraphicsSettingsWidget* Graphics = Cast<UTunaSweeperGraphicsSettingsWidget>(Find(Menu.Get(), TEXT("TitleGraphicsSettingsWidget")));
		if (!TestNotNull(TEXT("Authored graphics child resolves"), Graphics)) return false;
		const TCHAR* ToggleNames[] = { TEXT("VSyncToggleButton"), TEXT("MotionBlurToggleButton"), TEXT("DynamicResolutionToggleButton"), TEXT("HardwareRayTracingToggleButton") };
		const TCHAR* LabelKeys[] = { TEXT("ui.settings.vsync"), TEXT("ui.settings.motion_blur"), TEXT("ui.settings.dynamic_resolution"), TEXT("ui.settings.hardware_ray_tracing") };
		for (int32 ToggleIndex = 0; ToggleIndex < UE_ARRAY_COUNT(ToggleNames); ++ToggleIndex)
		{
			UButton* Button = Cast<UButton>(Find(Graphics, ToggleNames[ToggleIndex]));
			UTextBlock* Label = Cast<UTextBlock>(Find(Graphics, FName(*(FString(ToggleNames[ToggleIndex]) + TEXT("Text")))));
			UTunaSweeperCheckIndicatorWidget* Check = Cast<UTunaSweeperCheckIndicatorWidget>(Find(Graphics,
				FName(*(FString(ToggleNames[ToggleIndex]) + TEXT("_CheckIndicator")))));
			if (!TestNotNull(TEXT("Checkbox has real input"), Button) || !TestNotNull(TEXT("Checkbox has label"), Label)
				|| !TestNotNull(TEXT("Checkbox has a drawn indicator"), Check)) return false;
			TestEqual(TEXT("Checkbox label contains only localized text, no textual check markers"), Label->GetText().ToString(),
				Instance->ResolveLocalizedText(LabelKeys[ToggleIndex], FText::GetEmpty()).ToString());
			TestFalse(TEXT("Checkbox localization is nonempty"), Label->GetText().IsEmpty());
			if (Button->GetIsEnabled())
			{
				const bool bInitialChecked = Check->IsChecked();
				Button->OnClicked.Broadcast();
				TestEqual(TEXT("Click updates actual check state"), Check->IsChecked(), !bInitialChecked);
				TestTrue(TEXT("Click stages a setting change"), Graphics->HasPendingChanges());
				Graphics->DiscardPendingChanges();
				TestEqual(TEXT("Cancel restores actual check state"), Check->IsChecked(), bInitialChecked);
				TestFalse(TEXT("Cancel clears staged change"), Graphics->HasPendingChanges());
			}
		}
		FAssetCompilingManager::Get().FinishAllCompilation();
		FlushRenderingCommands();
		FWidgetRenderer Renderer(true);
		TStrongObjectPtr<UTextureRenderTarget2D> Target(Renderer.DrawWidget(Slate, FVector2D(1920, 1080)));
		if (!TestNotNull(TEXT("UMG render target exists"), Target.Get())) return false;
		auto Capture = [&](const TCHAR* Position)
		{
			Renderer.DrawWidget(Target.Get(), Slate, FVector2D(1920, 1080), 0.0f);
			FlushRenderingCommands();
			FImage Pixels;
			if (!TestTrue(TEXT("Rendered UMG pixels are readable"), FImageUtils::GetRenderTargetImage(Target.Get(), Pixels))) return;
			const FString Path = FPaths::ProjectSavedDir() / FString::Printf(TEXT("Screenshots/UnifiedSettings_%s_%s.png"), LanguageNames[LanguageIndex], Position);
			TestTrue(TEXT("Rendered UMG image is saved"), FImageUtils::SaveImageByExtension(*Path, Pixels));
		};
		Capture(TEXT("top"));
		UUserWidget* LongQualityRow = Cast<UUserWidget>(Find(Graphics, TEXT("GlobalIlluminationQualityRow")));
		UWidget* QualityLabel = Find(LongQualityRow, TEXT("OptionLabelText"));
		UWidget* QualityPrevious = Find(LongQualityRow, TEXT("PreviousButton"));
		if (!TestNotNull(TEXT("Long quality label exists"), QualityLabel)
			|| !TestNotNull(TEXT("Quality arrow exists"), QualityPrevious)) return false;
		TestTrue(TEXT("Localized quality label stays clear of the previous arrow"),
			QualityLabel->GetCachedGeometry().LocalToAbsolute(QualityLabel->GetCachedGeometry().GetLocalSize()).X
			<= QualityPrevious->GetCachedGeometry().GetAbsolutePosition().X + 0.5f);
		if (UScrollBox* Scroll = Cast<UScrollBox>(Find(Graphics, TEXT("GraphicsSettingsScroll"))))
		{
			Scroll->SetScrollOffset(10000.0f);
			Capture(TEXT("bottom"));
		}
		for (const TCHAR* ToggleName : ToggleNames)
		{
			UWidget* Button = Find(Graphics, ToggleName);
			UWidget* Label = Find(Graphics, FName(*(FString(ToggleName) + TEXT("Text"))));
			TestTrue(TEXT("Localized checkbox label fits within its clickable button"),
				Label->GetCachedGeometry().LocalToAbsolute(Label->GetCachedGeometry().GetLocalSize()).X
				<= Button->GetCachedGeometry().LocalToAbsolute(Button->GetCachedGeometry().GetLocalSize()).X + 0.5f);
		}
		UButton* InterfaceTab = Cast<UButton>(Find(Menu.Get(), TEXT("SettingsInterfaceTabButton")));
		if (!TestNotNull(TEXT("Interface tab exists"), InterfaceTab)) return false;
		InterfaceTab->OnClicked.Broadcast();
		Slate->Tick(Menu->GetCachedGeometry(), FPlatformTime::Seconds(), 1.0f);
		Menu->ForceLayoutPrepass();
		Capture(TEXT("language"));
		UTunaSweeperOptionRowWidget* LanguageRow = Cast<UTunaSweeperOptionRowWidget>(Find(Menu.Get(), TEXT("InterfaceLanguageOptionRow")));
		if (!TestNotNull(TEXT("Language uses compact selector"), LanguageRow)) return false;
		UTextBlock* LanguageValue = Cast<UTextBlock>(Find(LanguageRow, TEXT("OptionValueText")));
		UButton* NextLanguage = Cast<UButton>(Find(LanguageRow, TEXT("NextButton")));
		UButton* PreviousLanguage = Cast<UButton>(Find(LanguageRow, TEXT("PreviousButton")));
		if (!TestNotNull(TEXT("Language value exists"), LanguageValue) || !NextLanguage || !PreviousLanguage) return false;
		const FString BeforeLanguage = LanguageValue->GetText().ToString();
		(NextLanguage->GetIsEnabled() ? NextLanguage : PreviousLanguage)->OnClicked.Broadcast();
		TestNotEqual(TEXT("Language arrows change the displayed pending value"), LanguageValue->GetText().ToString(), BeforeLanguage);
		TestEqual(TEXT("Language arrows do not apply or persist immediately"), Instance->GetCurrentTextLanguage(), Languages[LanguageIndex]);
		Menu->ClosePauseSettings();
	}
	return true;
}
#endif
