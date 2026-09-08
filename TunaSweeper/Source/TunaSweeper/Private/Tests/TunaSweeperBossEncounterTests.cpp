#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/TunaSweeperBossEncounter.h"
#include "AI/TunaSweeperAttackTelegraph.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperMissileTurret.h"
#include "AI/TunaSweeperPatternEnemyCharacter.h"
#include "AI/TunaSweeperRollingRobotMinion.h"
#include "AI/TunaSweeperTeachingMinibossCharacters.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Effect/TunaSweeperCombatPatternEffectActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"

namespace TunaSweeperBossEncounterTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	const FVector SafeLanding(-1600.0f, 0.0f, 120.0f);
	const FVector CombatEntry(-800.0f, 0.0f, 120.0f);

	/** Registered collision and a real player controller; explicit encounter ticks avoid unrelated AI. */
	struct FTestWorld
	{
		UWorld* World = nullptr;
		APlayerController* Controller = nullptr;
		ATunaSweeperEnemyCharacter* Player = nullptr;
		ATunaSweeperBossEncounter* Arena = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false)
				.ShouldSimulatePhysics(false).EnableTraceCollision(true).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("BossEncounterTestWorld")),
				GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
			if (!World) { return; }
			if (GEngine) { GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World); }
			// PostInitializeComponents registers controllers and enables spawn collision checks.
			World->InitializeActorsForPlay(FURL());
			AddBlocker(FVector(0, 0, -20), FVector(5000, 5000, 20));
			Controller = World->SpawnActor<APlayerController>();
			// UE 5.7 identifies player-controlled pawns through their non-bot PlayerState.
			// This small world has no game mode to create one automatically.
			APlayerState* PlayerState = World->SpawnActor<APlayerState>();
			if (Controller && PlayerState)
			{
				PlayerState->SetIsABot(false);
				Controller->SetPlayerState(PlayerState);
			}
			Player = SpawnPawn(SafeLanding, 1);
			if (Controller && Player) { Controller->Possess(Player); }
		}

		~FTestWorld()
		{
			if (World)
			{
				World->DestroyWorld(false);
				if (GEngine) { GEngine->DestroyWorldContext(World); }
				World->RemoveFromRoot();
			}
		}

		ATunaSweeperEnemyCharacter* SpawnPawn(FVector Location, uint8 Faction = 10)
		{
			ATunaSweeperEnemyCharacter* Pawn = World->SpawnActorDeferred<ATunaSweeperEnemyCharacter>(
				ATunaSweeperEnemyCharacter::StaticClass(), FTransform(Location), nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Pawn) { return nullptr; }
			Pawn->AutoPossessAI = EAutoPossessAI::Disabled;
			Pawn->ConfigureCombatProfile(Pawn->GetCombatProfile(), Faction, NAME_None, INDEX_NONE);
			Pawn->ConfigureSpawnData(TSoftObjectPtr<UMaterialInterface>(), NAME_None, INDEX_NONE,
				INDEX_NONE, 100.0f, 0);
			Pawn->FinishSpawning(FTransform(Location));
			World->GetSubsystem<UTunaSweeperFactionSubsystem>()->RegisterFactionActor(Pawn);
			return Pawn;
		}

		ATunaSweeperBossEncounter* AddArena(TSubclassOf<ATunaSweeperEnemyCharacter> BossClass,
			FTransform Transform = FTransform::Identity)
		{
			Arena = World->SpawnActorDeferred<ATunaSweeperBossEncounter>(
				ATunaSweeperBossEncounter::StaticClass(), Transform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Arena) { return nullptr; }
			Arena->BossClass = BossClass;
			Arena->EncounterId = TEXT("AutomationArena");
			Arena->CombatBounds->SetBoxExtent(FVector(1000, 600, 400));
			Arena->BossSpawnOffset = FVector(600, 0, 120);
			Arena->FinishSpawning(Transform);
			Arena->DispatchBeginPlay();
			return Arena;
		}

		AActor* AddBlocker(FVector Location, FVector Extent)
		{
			AActor* Blocker = World->SpawnActor<AActor>();
			UBoxComponent* Box = NewObject<UBoxComponent>(Blocker);
			Blocker->SetRootComponent(Box);
			Blocker->AddInstanceComponent(Box);
			Box->SetBoxExtent(Extent);
			Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			Box->SetCollisionObjectType(ECC_WorldStatic);
			Box->SetCollisionResponseToAllChannels(ECR_Block);
			Box->SetGenerateOverlapEvents(false);
			Box->RegisterComponent();
			Blocker->SetActorLocation(Location);
			return Blocker;
		}

		void MovePlayer(FVector Location)
		{
			Player->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
			Player->GetCapsuleComponent()->UpdateOverlaps();
			Advance();
		}

		void Advance(int32 Frames = 1)
		{
			for (int32 Index = 0; Index < Frames; ++Index)
			{
				World->Tick(LEVELTICK_TimeOnly, 0.02f);
				if (IsValid(Arena)) { Arena->Tick(0.02f); }
			}
		}

		template <typename T>
		T* SpawnOwned(AActor* Owner, FVector Location = FVector(300, 300, 120))
		{
			// Match combat summons: publish ownership before FinishSpawning auto-possesses pawns.
			const FTransform Transform(Location);
			T* Actor = World->SpawnActorDeferred<T>(T::StaticClass(), Transform, Owner,
				Cast<APawn>(Owner), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (Actor) { Actor->FinishSpawning(Transform); }
			return Actor;
		}

		template <typename T>
		TArray<T*> Actors() const
		{
			TArray<T*> Result;
			for (TActorIterator<T> It(World); It; ++It)
			{
				if (IsValid(*It) && !It->IsActorBeingDestroyed()) { Result.Add(*It); }
			}
			return Result;
		}
	};

	/** Attach a disposable test world without running the project's save-loading Init. */
	struct FGameInstanceWorldContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			// A member pointer grants access without casting an existing UObject to a fake subclass.
			auto ContextMember = &FGameInstanceWorldContextAccess::WorldContext;
			Instance->*ContextMember = Context;
		}
	};
	bool WasRemoved(const AActor* Actor)
	{
		return !IsValid(Actor) || Actor->IsActorBeingDestroyed();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBossEncounterEntryTest,
	"TunaSweeper.Combat.BossEncounter.SafeLandingAndPlayerBoundary",
	TunaSweeperBossEncounterTests::TestFlags)

bool FTunaSweeperBossEncounterEntryTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBossEncounterTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Controlled player exists"), TestWorld.Player) ||
		!TestNotNull(TEXT("Encounter exists"), TestWorld.AddArena(ATunaSweeperChargeTeachingMiniboss::StaticClass()))) { return false; }
	TestTrue(TEXT("Fixture has a recognized player-controlled pawn"), TestWorld.Player->IsPlayerControlled());
	TestTrue(TEXT("Fixture controller is registered with its world"), TestWorld.World->GetFirstPlayerController() == TestWorld.Controller);
	TestTrue(TEXT("Fixture encounter has entered BeginPlay"), TestWorld.Arena->HasActorBegunPlay());
	ATunaSweeperEnemyCharacter* Bystander = TestWorld.SpawnPawn(FVector(0, 300, 120));
	TestWorld.Advance(25);
	TestNull(TEXT("An AI pawn inside and a player in the landing area cannot start combat"), TestWorld.Arena->GetBoss());
	TestEqual(TEXT("The safe landing remains ready"), TestWorld.Arena->GetState(), ETunaSweeperBossEncounterState::Ready);
	TestFalse(TEXT("The safe landing is outside combat occupancy"), TestWorld.Arena->IsPlayerInside());
	TestWorld.MovePlayer(FVector(-1010, 0, 120));
	TestNull(TEXT("The player's capsule may touch the boundary without starting combat early"), TestWorld.Arena->GetBoss());
	TestWorld.MovePlayer(CombatEntry);
	ATunaSweeperEnemyCharacter* Boss = TestWorld.Arena->GetBoss();
	if (!TestNotNull(TEXT("Crossing with the player center starts combat"), Boss)) { return false; }
	TestTrue(TEXT("The configured charge teacher spawned"), Boss->GetClass() == ATunaSweeperChargeTeachingMiniboss::StaticClass());
	TestEqual(TEXT("Player entry activates the encounter"), TestWorld.Arena->GetState(), ETunaSweeperBossEncounterState::Active);
	TestWorld.Advance(50);
	TestTrue(TEXT("Remaining inside cannot spawn a duplicate boss"), TestWorld.Arena->GetBoss() == Boss);
	TestEqual(TEXT("Only the player, bystander, and one boss exist"), TestWorld.Actors<ATunaSweeperEnemyCharacter>().Num(), 3);
	TestTrue(TEXT("Unrelated enemy remains alive"), IsValid(Bystander));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBossEncounterTransformTest,
	"TunaSweeper.Combat.BossEncounter.RotatedVolumeAndConfiguredClasses",
	TunaSweeperBossEncounterTests::TestFlags)

bool FTunaSweeperBossEncounterTransformTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBossEncounterTests;
	const TSubclassOf<ATunaSweeperEnemyCharacter> Classes[] = {
		ATunaSweeperChargeTeachingMiniboss::StaticClass(),
		ATunaSweeperRobotTeachingMiniboss::StaticClass(),
		ATunaSweeperPatternEnemyCharacter::StaticClass() };
	for (const TSubclassOf<ATunaSweeperEnemyCharacter> BossClass : Classes)
	{
		FTestWorld TestWorld;
		const FTransform Placement(FRotator(0, 45, 0), FVector(2500, 0, 0));
		if (!TestWorld.Player || !TestWorld.AddArena(BossClass, Placement)) { AddError(TEXT("Transformed arena could not be created")); return false; }
		TestWorld.MovePlayer(Placement.TransformPosition(FVector(0, 650, 120)));
		TestNull(TEXT("A point inside the world AABB but outside the rotated box stays safe"), TestWorld.Arena->GetBoss());
		TestWorld.MovePlayer(Placement.TransformPosition(CombatEntry));
		ATunaSweeperEnemyCharacter* Boss = TestWorld.Arena->GetBoss();
		if (!TestNotNull(TEXT("Rotated arena starts on actual volume entry"), Boss)) { return false; }
		TestTrue(TEXT("Every encounter spawns its exact configured character class"), Boss->GetClass() == BossClass.Get());
		TestTrue(TEXT("Boss placement transforms with the arena in XY"), FVector::Dist2D(Boss->GetActorLocation(),
			Placement.TransformPosition(TestWorld.Arena->BossSpawnOffset)) < 0.1f);
		const float FeetHeight = Boss->GetActorLocation().Z - Boss->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		TestTrue(TEXT("Possession settles each boss upright just above the floor"), FeetHeight >= 0.0f && FeetHeight <= 3.0f);
		TestTrue(TEXT("Boss faces the configured local entrance direction"), Boss->GetActorForwardVector().Equals(
			Placement.TransformVectorNoScale(FVector(-1, 0, 0)), 0.01f));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBossEncounterCleanupTest,
	"TunaSweeper.Combat.BossEncounter.ExitCancelsOwnedHazardsAndAllowsRetry",
	TunaSweeperBossEncounterTests::TestFlags)

bool FTunaSweeperBossEncounterCleanupTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBossEncounterTests;
	FTestWorld TestWorld;
	if (!TestWorld.Player || !TestWorld.AddArena(ATunaSweeperPatternEnemyCharacter::StaticClass())) { return false; }
	TestWorld.MovePlayer(CombatEntry);
	ATunaSweeperEnemyCharacter* Boss = TestWorld.Arena->GetBoss();
	if (!TestNotNull(TEXT("Boss exists before hazard setup"), Boss)) { return false; }
	ATunaSweeperMissileTurret* Turret = TestWorld.SpawnOwned<ATunaSweeperMissileTurret>(Boss);
	ATunaSweeperRollingRobotMinion* Minion = TestWorld.SpawnOwned<ATunaSweeperRollingRobotMinion>(Boss, FVector(100, 200, 120));
	ATunaSweeperRollingRobotMinion* UnrelatedMinion = TestWorld.SpawnOwned<ATunaSweeperRollingRobotMinion>(nullptr, FVector(2000, 0, 120));
	if (!Turret || !Minion || !UnrelatedMinion) { AddError(TEXT("Hazard fixture could not be created")); return false; }
	Turret->InitializeTurret(Boss, TestWorld.Player);
	Turret->Tick(1.0f);
	const TArray<ATunaSweeperAttackTelegraph*> Warnings = TestWorld.Actors<ATunaSweeperAttackTelegraph>();
	if (!TestEqual(TEXT("The live turret created a warning"), Warnings.Num(), 1)) { return false; }
	ATunaSweeperAttackTelegraph* Warning = Warnings[0];
	UStaticMeshComponent* FallingMissile = nullptr;
	TInlineComponentArray<UStaticMeshComponent*> Meshes(Warning);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (Mesh->GetName() == TEXT("FallingMissile")) { FallingMissile = Mesh; }
	}
	TestNotNull(TEXT("The warning owns the falling missile visual"), FallingMissile);
	ATunaSweeperCombatPatternEffectActor* DetachedTrail = ATunaSweeperCombatPatternEffectActor::Spawn(
		TestWorld.World, ETunaSweeperCombatPatternEffect::MissileTrail, FVector(300, 300, 200), 24, FVector::UpVector, Warning);
	if (!TestNotNull(TEXT("An owned missile trail exists"), DetachedTrail)) { return false; }
	DetachedTrail->SetOwner(nullptr);
	// Ownership may be cleared when the visual detaches; the encounter must remember the spawn.
	TestWorld.MovePlayer(SafeLanding);
	TestEqual(TEXT("Exiting returns the encounter to ready"), TestWorld.Arena->GetState(), ETunaSweeperBossEncounterState::Ready);
	TestNull(TEXT("Exiting removes the active boss"), TestWorld.Arena->GetBoss());
	TestTrue(TEXT("The old boss was removed"), WasRemoved(Boss));
	TestTrue(TEXT("The turret cannot keep firing into the landing area"), WasRemoved(Turret));
	TestTrue(TEXT("The rolling minion cannot follow the player out"), WasRemoved(Minion));
	TestTrue(TEXT("The warning and its missile were removed"), WasRemoved(Warning));
	TestTrue(TEXT("Already detached missile trails were removed"), WasRemoved(DetachedTrail));
	TestTrue(TEXT("Unrelated actors are preserved"), IsValid(UnrelatedMinion) && !UnrelatedMinion->IsActorBeingDestroyed());
	TestWorld.Advance(10);
	TestNull(TEXT("Waiting in safety does not restart combat"), TestWorld.Arena->GetBoss());
	TestWorld.MovePlayer(CombatEntry);
	if (!TestNotNull(TEXT("Reentering starts a fresh attempt"), TestWorld.Arena->GetBoss())) { return false; }
	TestTrue(TEXT("Retry uses a fresh boss"), TestWorld.Arena->GetBoss() != Boss);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBossEncounterClearTest,
	"TunaSweeper.Combat.BossEncounter.DefeatResetAndPlayerLoss",
	TunaSweeperBossEncounterTests::TestFlags)

bool FTunaSweeperBossEncounterClearTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBossEncounterTests;
	FTestWorld TestWorld;
	if (!TestWorld.Player || !TestWorld.AddArena(ATunaSweeperRobotTeachingMiniboss::StaticClass())) { return false; }
	TestWorld.MovePlayer(CombatEntry);
	ATunaSweeperEnemyCharacter* Boss = TestWorld.Arena->GetBoss();
	if (!TestNotNull(TEXT("Boss exists before defeat"), Boss)) { return false; }
	ATunaSweeperRollingRobotMinion* Summon = TestWorld.SpawnOwned<ATunaSweeperRollingRobotMinion>(Boss);
	UGameplayStatics::ApplyDamage(Boss, 100000.0f, TestWorld.Controller, TestWorld.Player, nullptr);
	TestWorld.Advance();
	TestEqual(TEXT("Defeating the boss completes the encounter"), TestWorld.Arena->GetState(), ETunaSweeperBossEncounterState::Cleared);
	TestTrue(TEXT("Defeat clears surviving summons"), WasRemoved(Summon));
	TestWorld.Advance(10);
	TestNull(TEXT("A cleared encounter cannot immediately restart beneath the player"), TestWorld.Arena->GetBoss());
	TestWorld.MovePlayer(SafeLanding);
	TestWorld.MovePlayer(CombatEntry);
	if (!TestNotNull(TEXT("Returning after a clear creates another attempt"), TestWorld.Arena->GetBoss())) { return false; }
	TestWorld.Arena->ResetEncounter();
	TestWorld.Advance(10);
	TestNull(TEXT("Manual reset holds until the player leaves"), TestWorld.Arena->GetBoss());
	TestWorld.MovePlayer(SafeLanding);
	TestWorld.MovePlayer(CombatEntry);
	Boss = TestWorld.Arena->GetBoss();
	if (!TestNotNull(TEXT("Leaving and reentering releases the reset hold"), Boss)) { return false; }
	TestWorld.Controller->UnPossess();
	TestWorld.Advance();
	TestTrue(TEXT("Losing the player controller cancels combat"), WasRemoved(Boss));
	TestEqual(TEXT("Player loss leaves the arena ready"), TestWorld.Arena->GetState(), ETunaSweeperBossEncounterState::Ready);
	TestWorld.Controller->Possess(TestWorld.Player);
	TestWorld.Advance();
	Boss = TestWorld.Arena->GetBoss();
	if (!TestNotNull(TEXT("A new controlled participant can start an available arena"), Boss)) { return false; }
	UGameplayStatics::ApplyDamage(TestWorld.Player, 100000.0f, nullptr, Boss, nullptr);
	TestWorld.Advance();
	TestTrue(TEXT("Player death removes the remaining boss"), WasRemoved(Boss));
	TestEqual(TEXT("Player death returns the encounter to ready"), TestWorld.Arena->GetState(), ETunaSweeperBossEncounterState::Ready);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBossEncounterInvalidSpawnTest,
	"TunaSweeper.Combat.BossEncounter.InvalidSpawnRequiresNewEntry",
	TunaSweeperBossEncounterTests::TestFlags)

bool FTunaSweeperBossEncounterInvalidSpawnTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBossEncounterTests;
	FTestWorld TestWorld;
	if (!TestWorld.Player || !TestWorld.AddArena(nullptr)) { return false; }
	AddExpectedError(TEXT("cannot start: missing/abstract boss class or unavailable player"), EAutomationExpectedErrorFlags::Contains, 1);
	TestWorld.MovePlayer(CombatEntry);
	TestWorld.Arena->BossClass = ATunaSweeperChargeTeachingMiniboss::StaticClass();
	TestWorld.Advance(25);
	TestNull(TEXT("An invalid class does not retry every frame or surprise the waiting player"), TestWorld.Arena->GetBoss());
	TestWorld.MovePlayer(SafeLanding);
	TestWorld.Arena->BossSpawnOffset = FVector(2000, 0, 120);
	AddExpectedError(TEXT("cannot start: boss spawn is outside CombatBounds"), EAutomationExpectedErrorFlags::Contains, 1);
	TestWorld.MovePlayer(CombatEntry);
	TestNull(TEXT("A misconfigured spawn cannot put the boss in the safe landing area"), TestWorld.Arena->GetBoss());
	TestWorld.MovePlayer(SafeLanding);
	TestWorld.Arena->BossSpawnOffset = FVector(600, 0, 120);
	AActor* Blocker = TestWorld.AddBlocker(FVector(600, 0, 160), FVector(300, 300, 160));
	AddExpectedError(TEXT("cannot start: boss spawn is blocked"), EAutomationExpectedErrorFlags::Contains, 1);
	TestWorld.MovePlayer(CombatEntry);
	TestNull(TEXT("Blocked placement cannot embed a boss in geometry"), TestWorld.Arena->GetBoss());
	Blocker->Destroy();
	TestWorld.Advance(25);
	TestNull(TEXT("Removing a blocker does not cause an unannounced delayed spawn"), TestWorld.Arena->GetBoss());
	TestWorld.MovePlayer(SafeLanding);
	TestWorld.MovePlayer(CombatEntry);
	TestNotNull(TEXT("A new entry retries the now valid placement"), TestWorld.Arena->GetBoss());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBossTestInventorySessionTest,
	"TunaSweeper.Combat.BossEncounter.TransientLoadoutRestoresPlayerState",
	TunaSweeperBossEncounterTests::TestFlags)

bool FTunaSweeperBossTestInventorySessionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperBossEncounterTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Loadout fixture world exists"), TestWorld.World)) { return false; }
	TStrongObjectPtr<UTunaSweeperGameInstance> Instance(NewObject<UTunaSweeperGameInstance>(GEngine));
	FWorldContext* Context = GEngine->GetWorldContextFromWorld(TestWorld.World);
	FGameInstanceWorldContextAccess::Attach(Instance.Get(), Context);
	Context->OwningGameInstance = Instance.Get();
	TestWorld.World->SetGameInstance(Instance.Get());
	// This fixture must not run the project's Init or load an actual player save.
	// Seed only initialization and slots; all mutations under test use the runtime public API.
	const FBoolProperty* Initialized = FindFProperty<FBoolProperty>(Instance->GetClass(), TEXT("bInventoryStateInitialized"));
	if (!TestNotNull(TEXT("Transient inventory fixture can be initialized"), Initialized)) { return false; }
	Initialized->SetPropertyValue_InContainer(Instance.Get(), true);
	Instance->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Instance->EndCombatTestSession();
		Instance->UGameInstance::Shutdown();
		TestWorld.World->SetGameInstance(nullptr);
		Context->OwningGameInstance = nullptr;
		FGameInstanceWorldContextAccess::Attach(Instance.Get(), nullptr);
	};
	const_cast<TArray<FTunaSweeperInventorySlot>&>(Instance->GetInventorySlots()).SetNum(20);
	const_cast<TArray<FTunaSweeperInventorySlot>&>(Instance->GetEquipmentSlots()).SetNum(8);
	const_cast<TArray<FTunaSweeperInventorySlot>&>(Instance->GetStorageSlots()).SetNum(20);

	FTunaSweeperItemInstance OriginalRifle;
	OriginalRifle.ItemId = 1002;
	OriginalRifle.LoadedAmmoItemId = 2002;
	OriginalRifle.LoadedAmmoCount = 7;
	const FGuid RifleUid = Instance->CreateItemInstanceFromTemplate(OriginalRifle);
	FTunaSweeperItemInstance OriginalAmmo;
	OriginalAmmo.ItemId = 2002;
	OriginalAmmo.Quantity = 17;
	const FGuid AmmoUid = Instance->CreateItemInstanceFromTemplate(OriginalAmmo);
	OriginalAmmo.Quantity = 9;
	const FGuid StoredAmmoUid = Instance->CreateItemInstanceFromTemplate(OriginalAmmo);
	FTunaSweeperInventorySlot& RifleSlot = const_cast<TArray<FTunaSweeperInventorySlot>&>(Instance->GetEquipmentSlots())[0];
	RifleSlot.ItemUid = RifleUid;
	RifleSlot.bSortLocked = true;
	const_cast<TArray<FTunaSweeperInventorySlot>&>(Instance->GetInventorySlots())[0].ItemUid = AmmoUid;
	const_cast<TArray<FTunaSweeperInventorySlot>&>(Instance->GetStorageSlots())[2].ItemUid = StoredAmmoUid;
	Instance->SetRuntimeSelectedWeaponSlotNumber(2);
	Instance->BeginRaidExperienceSession();
	Instance->AddRaidExperience(17);
	if (!TestEqual(TEXT("Original rifle holds seven loaded rounds"), Instance->GetWeaponLoadedAmmoCount(1), 7)) { return false; }

	Instance->BeginCombatTestSession();
	TestTrue(TEXT("The lab enables the temporary-session save guard"), Instance->IsCombatTestSession());
	const int32 FullMagazine = Instance->GetWeaponMagazineCapacity(1);
	TestTrue(TEXT("The lab provides a functioning loaded rifle"), FullMagazine > 0 && Instance->GetWeaponLoadedAmmoCount(1) == FullMagazine);
	TestTrue(TEXT("The lab provides enough reserve rounds for repeated attempts"), Instance->GetWeaponInventoryAmmoCount(1) >= 300);
	TestTrue(TEXT("Lab ammunition can actually be consumed"), Instance->TryConsumeLoadedAmmoForWeaponSlot(1));
	Instance->BeginCombatTestSession();
	TestEqual(TEXT("Repeated Begin does not replace the original backup or silently refill ammunition"), Instance->GetWeaponLoadedAmmoCount(1), FullMagazine - 1);
	Instance->ResetCombatTestLoadout();
	TestEqual(TEXT("Restarting an attempt refills the test magazine"), Instance->GetWeaponLoadedAmmoCount(1), FullMagazine);
	TestTrue(TEXT("A restarted attempt can fire normally"), Instance->TryConsumeLoadedAmmoForWeaponSlot(1));
	Instance->ClearInventoryAndSave();
	TestEqual(TEXT("The normal death-clear path safely replenishes test equipment"), Instance->GetWeaponLoadedAmmoCount(1), FullMagazine);
	Instance->AddRaidExperience(200);
	FTunaSweeperExperienceAnimationState DuringLab;
	Instance->CommitRaidExperienceGain(DuringLab);
	TestTrue(TEXT("The fixture exercised experience changes during combat testing"), Instance->GetTotalExperiencePoints() > 0);
	Instance->EndCombatTestSession();

	TestFalse(TEXT("Ending the lab releases the temporary-session guard"), Instance->IsCombatTestSession());
	TestEqual(TEXT("The original rifle UID is restored"), Instance->GetEquipmentSlots()[0].ItemUid, RifleUid);
	TestTrue(TEXT("Equipment sort-lock state is restored"), Instance->GetEquipmentSlots()[0].bSortLocked);
	TestEqual(TEXT("The original ammunition UID is restored"), Instance->GetInventorySlots()[0].ItemUid, AmmoUid);
	TestEqual(TEXT("Storage contents are restored in their original slot"), Instance->GetStorageSlots()[2].ItemUid, StoredAmmoUid);
	TestEqual(TEXT("Original magazine ammunition survives firing and restarts"), Instance->GetWeaponLoadedAmmoCount(1), 7);
	TestEqual(TEXT("Original reserve ammunition survives the session"), Instance->GetWeaponInventoryAmmoCount(1), 17);
	TestEqual(TEXT("Original total experience is restored"), Instance->GetTotalExperiencePoints(), static_cast<int64>(0));
	bool bMeleeSelected = true;
	int32 SelectedSlot = INDEX_NONE;
	TestTrue(TEXT("Original selection remains available"), Instance->TryGetRuntimeSelectedWeaponSelection(bMeleeSelected, SelectedSlot));
	TestFalse(TEXT("The original selection is a gun slot"), bMeleeSelected);
	TestEqual(TEXT("The original selected slot is restored"), SelectedSlot, 2);
	FTunaSweeperExperienceAnimationState AfterLab;
	TestTrue(TEXT("Original pending raid experience remains available"), Instance->CommitRaidExperienceGain(AfterLab));
	TestEqual(TEXT("Only the original seventeen pending experience points are committed"), Instance->GetTotalExperiencePoints(), static_cast<int64>(17));
	Instance->EndCombatTestSession();
	TestEqual(TEXT("Repeated End cannot overwrite restored state"), Instance->GetWeaponLoadedAmmoCount(1), 7);
	return true;
}

#endif
