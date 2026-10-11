#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperQuestRewardLockerActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SOverlay.h"

namespace TunaQuestRewardLockerInteractionTests
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Game, FWorldContext* Context)
		{
			auto Member = &FContextAccess::WorldContext;
			Game->*Member = Context;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaQuestRewardLockerInteractionTest,
	"TunaSweeper.QuestRewardLocker.InteractionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaQuestRewardLockerInteractionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("QuestRewardLockerBunkerMap"),
		CreatePackage(TEXT("/Temp/QuestRewardLockerBunkerMap")));
	if (!TestNotNull(TEXT("Transient bunker world exists"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	TunaQuestRewardLockerInteractionTests::FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	// Exercise real UI and gameplay code without loading or overwriting player saves.
	Game->bInventoryStateInitialized = true;
	Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
	Game->UGameInstance::Init();
	Context.GameViewport = NewObject<UGameViewportClient>(GEngine);
	// The viewport keeps only a weak reference; retain the Slate host through cleanup.
	TSharedRef<SOverlay> ViewportOverlay = SNew(SOverlay);
	Context.GameViewport->SetViewportOverlayWidget(nullptr, ViewportOverlay);
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		Context.GameViewport = nullptr;
		TunaQuestRewardLockerInteractionTests::FContextAccess::Attach(Game.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	World->InitializeActorsForPlay(FURL());
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Controller = World->SpawnActor<ATunaSweeperPlayerController>(Spawn);
	UClass* PlayerClass = LoadClass<ATunaSweeperTopDownCharacter>(nullptr,
		TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
	if (!TestNotNull(TEXT("Actual player Blueprint loads"), PlayerClass)) return false;
	auto* Pawn = World->SpawnActor<ATunaSweeperTopDownCharacter>(PlayerClass, FVector(100, 0, 0), FRotator::ZeroRotator, Spawn);
	auto* QuestRewardLocker = World->SpawnActor<ATunaSweeperQuestRewardLockerActor>(Spawn);
	if (!TestNotNull(TEXT("Controller exists"), Controller) ||
		!TestNotNull(TEXT("Pawn exists"), Pawn) || !TestNotNull(TEXT("QuestRewardLocker exists"), QuestRewardLocker)) return false;
	auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	LocalPlayer->ViewportClient = Context.GameViewport;
	Game->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Pawn);
	Pawn->DispatchBeginPlay();
	TestFalse(TEXT("Opening without a source is rejected"), Controller->OpenQuestRewardLockerPanel(nullptr));
	TestFalse(TEXT("Applying without an open reward locker is rejected"), Controller->TryReceiveQuestReward(0));
	QuestRewardLocker->SetActorLocation(FVector::ZeroVector);
	Pawn->SetActorLocation(FVector(100, 0, 0));
	if (!TestTrue(TEXT("A nearby reward locker opens the actual HUD panel"), Controller->OpenQuestRewardLockerPanel(QuestRewardLocker))) return false;
	auto* Hud = Controller->GetGameHudWidget();
	if (!TestNotNull(TEXT("Opening creates the game HUD"), Hud)) return false;
	TestTrue(TEXT("The HUD is attached to the live viewport overlay"),
		Hud->IsInViewport() && ViewportOverlay->GetChildren()->Num() > 0);
	TestEqual(TEXT("Opening activates reward locker mode"), Hud->GetHudMode(), ETunaSweeperHudMode::QuestRewardLocker);
	TestTrue(TEXT("An open reward locker suppresses gameplay input"), Controller->IsInventoryUiOpen());
	TestTrue(TEXT("Living local pawn beside the reward locker can use it"), Controller->IsQuestRewardLockerInteractionValid());
	TestFalse(TEXT("Invalid reward index is rejected by the real transaction"), Controller->TryReceiveQuestReward(INDEX_NONE));
	TestEqual(TEXT("A failed receive leaves the valid panel open"), Hud->GetHudMode(), ETunaSweeperHudMode::QuestRewardLocker);
	Controller->TogglePauseMenu();
	TestEqual(TEXT("Escape closes reward locker instead of stacking pause"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestFalse(TEXT("Escape releases the interaction source"), Controller->IsQuestRewardLockerInteractionValid());
	TestFalse(TEXT("Escape does not open pause"), Controller->IsPauseMenuOpen());
	TestTrue(TEXT("QuestRewardLocker can reopen after Escape"), Controller->OpenQuestRewardLockerPanel(QuestRewardLocker));
	Hud->SetHudMode(ETunaSweeperHudMode::Research);
	TestFalse(TEXT("Switching modes releases the reward locker source immediately"), Controller->IsQuestRewardLockerInteractionValid());
	TestTrue(TEXT("QuestRewardLocker can reopen after switching modes"), Controller->OpenQuestRewardLockerPanel(QuestRewardLocker));
	Pawn->SetActorLocation(FVector(301, 0, 0));
	TestFalse(TEXT("Leaving the interaction radius invalidates the session"), Controller->IsQuestRewardLockerInteractionValid());
	Controller->UpdateQuestRewardLockerInteraction();
	TestEqual(TEXT("Range validation closes the open HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestFalse(TEXT("Opening outside the radius is rejected"), Controller->OpenQuestRewardLockerPanel(QuestRewardLocker));
	Pawn->SetActorLocation(FVector(100, 0, 0));
	TestTrue(TEXT("A second nearby session opens"), Controller->OpenQuestRewardLockerPanel(QuestRewardLocker));
	Pawn->SetActorLocation(FVector(301, 0, 0));
	TestFalse(TEXT("An out-of-range apply is rejected"), Controller->TryReceiveQuestReward(0));
	TestEqual(TEXT("Apply revalidation closes an invalid session"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	Pawn->SetActorLocation(FVector(100, 0, 0));
	TestTrue(TEXT("QuestRewardLocker opens before being hidden"), Controller->OpenQuestRewardLockerPanel(QuestRewardLocker));
	QuestRewardLocker->SetActorHiddenInGame(true);
	TestFalse(TEXT("A hidden facility cannot remain usable"), Controller->IsQuestRewardLockerInteractionValid());
	Controller->UpdateQuestRewardLockerInteraction();
	TestEqual(TEXT("Hidden facility closes the HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	QuestRewardLocker->SetActorHiddenInGame(false);
	TestTrue(TEXT("QuestRewardLocker opens before source deletion"), Controller->OpenQuestRewardLockerPanel(QuestRewardLocker));
	TestTrue(TEXT("The live source remains usable before destruction"), Controller->IsQuestRewardLockerInteractionValid());
	QuestRewardLocker->Destroy();
	TestFalse(TEXT("Deleting the source invalidates the session"), Controller->IsQuestRewardLockerInteractionValid());
	Controller->UpdateQuestRewardLockerInteraction();
	TestEqual(TEXT("Source deletion closes the open HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestFalse(TEXT("Closing releases the active source"), Controller->ActiveQuestRewardLockerActor.IsValid());
	QuestRewardLocker = World->SpawnActor<ATunaSweeperQuestRewardLockerActor>(Spawn);
	TestTrue(TEXT("Replacement reward locker opens before death"), Controller->OpenQuestRewardLockerPanel(QuestRewardLocker));
	TestTrue(TEXT("Replacement source is usable before death"), Controller->IsQuestRewardLockerInteractionValid());
	auto* Vitals = Pawn->GetVitalsComponent();
	if (!TestNotNull(TEXT("Real health component exists"), Vitals)) return false;
	FTunaSweeperVitalsDelta LethalDamage;
	LethalDamage.Health = -100000.0f;
	Vitals->ApplyVitalsDelta(LethalDamage);
	TestEqual(TEXT("Lethal health delta reaches zero"), Vitals->GetVitalsState().Health, 0.0f);
	TestTrue(TEXT("Zero health causes the real death state"), Pawn->IsDead());
	TestFalse(TEXT("Death invalidates the reward locker"), Controller->IsQuestRewardLockerInteractionValid());
	Controller->UpdateQuestRewardLockerInteraction();
	TestEqual(TEXT("Death closes the open HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestTrue(TEXT("Closing after death preserves the movement lock"), Controller->IsMoveInputIgnored());
	TestTrue(TEXT("Closing after death preserves the look lock"), Controller->IsLookInputIgnored());
	return true;
}

#endif
