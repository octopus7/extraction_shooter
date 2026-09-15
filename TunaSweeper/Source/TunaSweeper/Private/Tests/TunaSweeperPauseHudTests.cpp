#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "UI/TunaSweeperHudDebuffBarWidget.h"
#include "UI/TunaSweeperHudQuickSlotBarWidget.h"
#include "UI/TunaSweeperPauseMenuWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperPauseHudTests
{
	struct FControllerAccess : ATunaSweeperPlayerController
	{
		static void SetPauseWidget(ATunaSweeperPlayerController* Controller, UTunaSweeperPauseMenuWidget* Widget)
		{
			auto Member = &FControllerAccess::PauseMenuWidget;
			Controller->*Member = Widget;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperPauseHudTransitionTest,
	"TunaSweeper.UI.PauseMenu.HudTransition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperPauseHudTransitionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Transient world exists"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	ON_SCOPE_EXIT
	{
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	World->InitializeActorsForPlay(FURL());
	ATunaSweeperPlayerController* Controller = World->SpawnActor<ATunaSweeperPlayerController>();
	if (!TestNotNull(TEXT("Project controller exists"), Controller)) return false;
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	LocalPlayer->SetControllerId(0);
	Controller->SetPlayer(LocalPlayer);
	TStrongObjectPtr<UTunaSweeperGameHudWidget> Hud(NewObject<UTunaSweeperGameHudWidget>());
	Hud->SetOwningPlayer(Controller);
	Hud->BottomStatusWidget = NewObject<UTunaSweeperHudBottomStatusWidget>(Hud.Get());
	Hud->QuickSlotBarWidget = NewObject<UTunaSweeperHudQuickSlotBarWidget>(Hud.Get());
	Hud->DebuffBarWidget = NewObject<UTunaSweeperHudDebuffBarWidget>(Hud.Get());
	Hud->BottomStatusWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	Hud->QuickSlotBarWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	Hud->DebuffBarWidget->SetVisibility(ESlateVisibility::Collapsed);
	Hud->bDebuffBarHasActiveDebuffs = false;
	const FVector2D Baseline(3.0, 5.0);
	Hud->BottomStatusWidget->SetRenderTranslation(Baseline);
	TestFalse(TEXT("Gameplay HUD is initially available"), Hud->IsGameplayBottomHudSuppressed());
	TStrongObjectPtr<UTunaSweeperPauseMenuWidget> Pause(NewObject<UTunaSweeperPauseMenuWidget>());
	TunaSweeperPauseHudTests::FControllerAccess::SetPauseWidget(Controller, Pause.Get());
	TestTrue(TEXT("Opening pause suppresses gameplay HUD"), Hud->IsGameplayBottomHudSuppressed());
	TestTrue(TEXT("Opening pause suppresses crosshair"), Hud->IsWeaponCrosshairSuppressed());
	Hud->RefreshDialogueHudVisibility();
	Hud->TickHudTransitions(0.09f);
	TestEqual(TEXT("HUD stays visible while sliding out"), Hud->BottomStatusWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("Bottom HUD moves downward"), Hud->BottomStatusWidget->GetRenderTransform().Translation.Y > Baseline.Y);
	Hud->TickHudTransitions(0.2f);
	TestEqual(TEXT("Bottom HUD hides after transition"), Hud->BottomStatusWidget->GetVisibility(), ESlateVisibility::Collapsed);
	TestEqual(TEXT("Quick slots hide after transition"), Hud->QuickSlotBarWidget->GetVisibility(), ESlateVisibility::Collapsed);
	TunaSweeperPauseHudTests::FControllerAccess::SetPauseWidget(Controller, nullptr);
	Hud->RefreshDialogueHudVisibility();
	Hud->TickHudTransitions(0.2f);
	TestEqual(TEXT("Resume restores bottom HUD"), Hud->BottomStatusWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Resume restores original position"), Hud->BottomStatusWidget->GetRenderTransform().Translation, Baseline);
	TestEqual(TEXT("Resume restores quick slots"), Hud->QuickSlotBarWidget->GetVisibility(), ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Empty debuff bar stays hidden"), Hud->DebuffBarWidget->GetVisibility(), ESlateVisibility::Collapsed);
	return true;
}
#endif
