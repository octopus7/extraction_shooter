#if WITH_DEV_AUTOMATION_TESTS
#include "Debug/TunaSweeperMovementCheatSubsystem.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperEnemyAIController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMovementCheatTest,
	"TunaSweeper.Debug.MovementCheats.GateCombatAndMapTeleport",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperMovementCheatTest::RunTest(const FString& Parameters)
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("World"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	Game->Init();
	const uint32 PreviousToken = TunaSweeperMovementCheatControl.EnableToken;
	TunaSweeperMovementCheatControl.EnableToken = 0;
	ON_SCOPE_EXIT
	{
		TunaSweeperMovementCheatControl.EnableToken = PreviousToken;
		Game->Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	auto* Cheats = Game->GetSubsystem<UTunaSweeperMovementCheatSubsystem>();
	if (!TestNotNull(TEXT("Cheat service"), Cheats)) return false;
	TestFalse(TEXT("Default disabled"), Cheats->IsEnabled());
	Cheats->CycleSpeed();
	TestEqual(TEXT("Disabled F6 remains normal"), Cheats->GetSpeedMultiplier(), 1.0f);
	TunaSweeperMovementCheatControl.EnableToken = 123;
	TestFalse(TEXT("Invalid token disabled"), Cheats->IsEnabled());
	TunaSweeperMovementCheatControl.EnableToken = 0x54534D43;
	TestTrue(TEXT("Explicit process token enables"), Cheats->IsEnabled());
	Cheats->CycleSpeed();
	TestEqual(TEXT("First speed"), Cheats->GetSpeedMultiplier(), 3.0f);
	TestTrue(TEXT("Combat suppression active"), Cheats->IsCombatSuppressed());
	auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>();
	auto* Enemy = World->SpawnActor<ATunaSweeperEnemyCharacter>();
	auto* Factions = World->GetSubsystem<UTunaSweeperFactionSubsystem>();
	if (!TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("Enemy"), Enemy) || !TestNotNull(TEXT("Factions"), Factions)) return false;
	TestFalse(TEXT("Enemy cannot acquire player"), Factions->CanTargetActor(Enemy, Player));
	TestFalse(TEXT("Player outgoing effects blocked"), Factions->CanApplyCombatEffect(Player, Enemy));
	TestFalse(TEXT("Player incoming effects blocked"), Factions->CanApplyCombatEffect(Enemy, Player));
	const float EnemyHealth = Enemy->GetHealth();
	TestEqual(TEXT("Actual enemy damage entry blocked"), Enemy->TakeDamage(10.0f, FDamageEvent(), nullptr, Player), 0.0f);
	TestEqual(TEXT("Enemy health preserved"), Enemy->GetHealth(), EnemyHealth);
	Cheats->CycleSpeed();
	TestEqual(TEXT("Second speed"), Cheats->GetSpeedMultiplier(), 8.0f);
	Cheats->CycleSpeed();
	TestEqual(TEXT("Cycle returns to normal"), Cheats->GetSpeedMultiplier(), 1.0f);
	TestTrue(TEXT("Normal outgoing effects restored"), Factions->CanApplyCombatEffect(Player, Enemy));
	auto* EnemyAI = World->SpawnActor<ATunaSweeperEnemyAIController>();
	if (!TestNotNull(TEXT("Enemy controller"), EnemyAI)) return false;
	EnemyAI->Possess(Enemy);
	EnemyAI->NotifyDamageTaken(Player);
	TestTrue(TEXT("Normal AI can acquire player"), EnemyAI->IsEngagedWith(Player));
	Cheats->CycleSpeed();
	static_cast<AActor*>(EnemyAI)->Tick(0.01f);
	TestFalse(TEXT("Existing player aggro cleared"), EnemyAI->IsEngagedWith(Player));
	TunaSweeperMovementCheatControl.EnableToken = 0;
	TestEqual(TEXT("Revocation applies before next tick"), Cheats->GetSpeedMultiplier(), 1.0f);
	Cheats->Tick(0);
	TunaSweeperMovementCheatControl.EnableToken = 0x54534D43;
	TestEqual(TEXT("Re-enable resets normal speed"), Cheats->GetSpeedMultiplier(), 1.0f);
	const FName Map = Cheats->GetCurrentMapId(Cheats);
	const FVector MapPosition(100, 200, 300);
	APlayerController* Controller = World->SpawnActor<APlayerController>();
	if (!TestNotNull(TEXT("Teleport controller"), Controller)) return false;
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	Game->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
	Controller->SetPlayer(LocalPlayer);
	Controller->Possess(Player);
	Player->SetActorLocation(FVector(-1000, 0, 150));
	auto CreateBox = [World](FVector Location, FVector Extent, bool bVisibleToTrace)
	{
		AActor* Actor = World->SpawnActor<AActor>();
		UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
		Actor->SetRootComponent(Box);
		Box->SetBoxExtent(Extent);
		Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Box->SetCollisionObjectType(ECC_WorldStatic);
		Box->SetCollisionResponseToAllChannels(ECR_Block);
		if (!bVisibleToTrace) Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
		Box->RegisterComponent();
		Actor->SetActorLocation(Location);
		return Actor;
	};
	AActor* Floor = CreateBox(FVector(100, 200, -10), FVector(500, 500, 10), true);
	TestTrue(TEXT("Teleport player resolves through normal entry point"), UGameplayStatics::GetPlayerCharacter(Cheats, 0) == Player);
	FCollisionQueryParams ProbeQuery(SCENE_QUERY_STAT(MovementCheatTestGround), false, Player);
	FHitResult Ground;
	TestTrue(TEXT("Fixture ground trace"), World->LineTraceSingleByChannel(Ground, FVector(100, 200, 100300), FVector(100, 200, -99700), ECC_Visibility, ProbeQuery));
	TestTrue(TEXT("Fixture ground walkable"), Player->GetCharacterMovement()->IsWalkable(Ground));
	const FVector Landing = Ground.ImpactPoint + FVector(0, 0, Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3);
	TestFalse(TEXT("Fixture landing clear"), World->OverlapBlockingTestByChannel(Landing, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeCapsule(Player->GetCapsuleComponent()->GetScaledCapsuleRadius(), Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), ProbeQuery));
	TestFalse(TEXT("Different map teleport rejected"), Cheats->TryTeleportToMapPosition(TEXT("DifferentMap"), MapPosition));
	TestFalse(TEXT("Empty map teleport rejected"), Cheats->TryTeleportToMapPosition(NAME_None, MapPosition));
	TestFalse(TEXT("Out of range teleport rejected"), Cheats->TryTeleportToMapPosition(Map, FVector(1.e20, 0, 0)));
	TestTrue(TEXT("Teleport finds walkable ground"), Cheats->TryTeleportToMapPosition(Map, MapPosition));
	TestEqual(TEXT("Teleport map X"), Player->GetActorLocation().X, 100.0);
	TestEqual(TEXT("Teleport map Y"), Player->GetActorLocation().Y, 200.0);
	TestTrue(TEXT("Teleport places capsule above ground"), Player->GetActorLocation().Z > 0.0);
	Player->SetActorLocation(FVector(-1000, 0, 150));
	const FVector BeforeBlockedTeleport = Player->GetActorLocation();
	AActor* Obstruction = CreateBox(FVector(100, 200, 100), FVector(60, 60, 100), false);
	TestFalse(TEXT("Blocked landing rejected"), Cheats->TryTeleportToMapPosition(Map, MapPosition));
	TestEqual(TEXT("Blocked landing preserves player position"), Player->GetActorLocation(), BeforeBlockedTeleport);
	Obstruction->Destroy();
	Floor->Destroy();
	TunaSweeperMovementCheatControl.EnableToken = 0;
	TestFalse(TEXT("Disabled teleport rejected"), Cheats->TryTeleportToMapPosition(Map, MapPosition));
	return true;
}
#endif
