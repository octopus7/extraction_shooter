#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperWardrobeActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SOverlay.h"

namespace TunaWardrobeInteractionTests
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaWardrobeInteractionTest,
	"TunaSweeper.Wardrobe.InteractionValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaWardrobeInteractionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, TEXT("WardrobeBunkerMap"),
		CreatePackage(TEXT("/Temp/WardrobeBunkerMap")));
	if (!TestNotNull(TEXT("Transient bunker world exists"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	TunaWardrobeInteractionTests::FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	// Exercise real UI and gameplay code without loading or overwriting player saves.
	Game->bInventoryStateInitialized = true;
	Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
	Game->bOutfitUnlocksLoaded = true;
	Game->UnlockedOutfitIds.Add(TEXT("Maid"));
	Game->bUnlockAllOutfitsOverride = true;
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
		TunaWardrobeInteractionTests::FContextAccess::Attach(Game.Get(), nullptr);
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
	auto* Wardrobe = World->SpawnActor<ATunaSweeperWardrobeActor>(Spawn);
	if (!TestNotNull(TEXT("Controller exists"), Controller) ||
		!TestNotNull(TEXT("Pawn exists"), Pawn) || !TestNotNull(TEXT("Wardrobe exists"), Wardrobe)) return false;
	auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	LocalPlayer->ViewportClient = Context.GameViewport;
	Game->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Pawn);
	Pawn->DispatchBeginPlay();
	TestFalse(TEXT("Opening without a source is rejected"), Controller->OpenWardrobePanel(nullptr));
	TestFalse(TEXT("Applying without an open wardrobe is rejected"), Controller->TryEquipWardrobeOutfit(TEXT("maid")));
	Wardrobe->SetActorLocation(FVector::ZeroVector);
	Pawn->SetActorLocation(FVector(100, 0, 0));
	if (!TestTrue(TEXT("A nearby wardrobe opens the actual HUD panel"), Controller->OpenWardrobePanel(Wardrobe))) return false;
	auto* Hud = Controller->GetGameHudWidget();
	if (!TestNotNull(TEXT("Opening creates the game HUD"), Hud)) return false;
	TestTrue(TEXT("The HUD is attached to the live viewport overlay"),
		Hud->IsInViewport() && ViewportOverlay->GetChildren()->Num() > 0);
	TestEqual(TEXT("Opening activates wardrobe mode"), Hud->GetHudMode(), ETunaSweeperHudMode::Wardrobe);
	TestTrue(TEXT("An open wardrobe suppresses gameplay input"), Controller->IsInventoryUiOpen());
	TestTrue(TEXT("Living local pawn beside the wardrobe can use it"), Controller->IsWardrobeInteractionValid());
	TestTrue(TEXT("Valid apply reaches the real outfit transaction"), Controller->TryEquipWardrobeOutfit(TEXT("Maid")));
	TestFalse(TEXT("Unknown outfit is rejected by the real transaction"), Controller->TryEquipWardrobeOutfit(TEXT("Unknown")));
	TestEqual(TEXT("An outfit failure leaves the valid panel open"), Hud->GetHudMode(), ETunaSweeperHudMode::Wardrobe);
	Controller->TogglePauseMenu();
	TestEqual(TEXT("Escape closes wardrobe instead of stacking pause"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestFalse(TEXT("Escape releases the interaction source"), Controller->IsWardrobeInteractionValid());
	TestFalse(TEXT("Escape does not open pause"), Controller->IsPauseMenuOpen());
	TestTrue(TEXT("Wardrobe can reopen after Escape"), Controller->OpenWardrobePanel(Wardrobe));
	Hud->SetHudMode(ETunaSweeperHudMode::Research);
	TestFalse(TEXT("Switching modes releases the wardrobe source immediately"), Controller->IsWardrobeInteractionValid());
	TestTrue(TEXT("Wardrobe can reopen after switching modes"), Controller->OpenWardrobePanel(Wardrobe));
	Pawn->SetActorLocation(FVector(201, 0, 0));
	TestFalse(TEXT("Leaving the interaction radius invalidates the session"), Controller->IsWardrobeInteractionValid());
	Controller->UpdateWardrobeInteraction();
	TestEqual(TEXT("Range validation closes the open HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestFalse(TEXT("Opening outside the radius is rejected"), Controller->OpenWardrobePanel(Wardrobe));
	Pawn->SetActorLocation(FVector(100, 0, 0));
	TestTrue(TEXT("A second nearby session opens"), Controller->OpenWardrobePanel(Wardrobe));
	Pawn->SetActorLocation(FVector(201, 0, 0));
	TestFalse(TEXT("An out-of-range apply is rejected"), Controller->TryEquipWardrobeOutfit(TEXT("maid")));
	TestEqual(TEXT("Apply revalidation closes an invalid session"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	Pawn->SetActorLocation(FVector(100, 0, 0));
	TestTrue(TEXT("Wardrobe opens before being hidden"), Controller->OpenWardrobePanel(Wardrobe));
	Wardrobe->SetActorHiddenInGame(true);
	TestFalse(TEXT("A hidden facility cannot remain usable"), Controller->IsWardrobeInteractionValid());
	Controller->UpdateWardrobeInteraction();
	TestEqual(TEXT("Hidden facility closes the HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	Wardrobe->SetActorHiddenInGame(false);
	TestTrue(TEXT("Wardrobe opens before source deletion"), Controller->OpenWardrobePanel(Wardrobe));
	TestTrue(TEXT("The live source remains usable before destruction"), Controller->IsWardrobeInteractionValid());
	Wardrobe->Destroy();
	TestFalse(TEXT("Deleting the source invalidates the session"), Controller->IsWardrobeInteractionValid());
	Controller->UpdateWardrobeInteraction();
	TestEqual(TEXT("Source deletion closes the open HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestFalse(TEXT("Closing releases the active source"), Controller->ActiveWardrobeActor.IsValid());
	Wardrobe = World->SpawnActor<ATunaSweeperWardrobeActor>(Spawn);
	TestTrue(TEXT("Replacement wardrobe opens before death"), Controller->OpenWardrobePanel(Wardrobe));
	TestTrue(TEXT("Replacement source is usable before death"), Controller->IsWardrobeInteractionValid());
	auto* Vitals = Pawn->GetVitalsComponent();
	if (!TestNotNull(TEXT("Real health component exists"), Vitals)) return false;
	FTunaSweeperVitalsDelta LethalDamage;
	LethalDamage.Health = -100000.0f;
	Vitals->ApplyVitalsDelta(LethalDamage);
	TestEqual(TEXT("Lethal health delta reaches zero"), Vitals->GetVitalsState().Health, 0.0f);
	TestTrue(TEXT("Zero health causes the real death state"), Pawn->IsDead());
	TestFalse(TEXT("Death invalidates the wardrobe"), Controller->IsWardrobeInteractionValid());
	Controller->UpdateWardrobeInteraction();
	TestEqual(TEXT("Death closes the open HUD"), Hud->GetHudMode(), ETunaSweeperHudMode::None);
	TestTrue(TEXT("Closing after death preserves the movement lock"), Controller->IsMoveInputIgnored());
	TestTrue(TEXT("Closing after death preserves the look lock"), Controller->IsLookInputIgnored());
	return true;
}

#endif
