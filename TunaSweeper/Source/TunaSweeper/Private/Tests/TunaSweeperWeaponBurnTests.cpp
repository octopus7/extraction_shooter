#if WITH_DEV_AUTOMATION_TESTS

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Combat/TunaSweeperWeaponBurnResolver.h"
#include "Component/TunaSweeperBurnComponent.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "UObject/Script.h"
#include <limits>

#include "Weapon/TunaSweeperProjectile.h"
#include "Weapon/TunaSweeperWeapon.h"

namespace TunaSweeperWeaponBurnTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

	struct FTestWorld
	{
		UWorld* World = nullptr;

		FTestWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false)
				.CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false)
				.ShouldSimulatePhysics(false).EnableTraceCollision(true).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("WeaponBurnTestWorld")),
				GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}

		~FTestWorld()
		{
			if (!World) return;
			World->DestroyWorld(false);
			if (GEngine) GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}

		APawn* SpawnSource()
		{
			APawn* Pawn = World->SpawnActor<APawn>();
			if (!Pawn) return nullptr;
			UTunaSweeperFactionComponent* Faction = NewObject<UTunaSweeperFactionComponent>(Pawn);
			Pawn->AddInstanceComponent(Faction);
			Faction->SetFactionId(TunaSweeperFactionIds::Player);
			Faction->RegisterComponent();
			return Pawn;
		}

		ATunaSweeperEnemyCharacter* SpawnEnemy(uint8 FactionId = TunaSweeperFactionIds::Enemy)
		{
			const FTransform Transform(FVector(600.0f, 0.0f, 88.0f));
			ATunaSweeperEnemyCharacter* Enemy = World->SpawnActorDeferred<ATunaSweeperEnemyCharacter>(
				ATunaSweeperEnemyCharacter::StaticClass(), Transform, nullptr, nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Enemy) return nullptr;
			Enemy->AutoPossessAI = EAutoPossessAI::Disabled;
			Enemy->GetFactionComponent()->SetFactionId(FactionId);
			Enemy->ConfigureSpawnData(TSoftObjectPtr<UMaterialInterface>(), NAME_None,
				INDEX_NONE, INDEX_NONE, 100.0f, 0);
			Enemy->FinishSpawning(Transform);
			return Enemy;
		}

		TArray<ATunaSweeperProjectile*> ProjectilesFrom(ATunaSweeperWeapon* Weapon) const
		{
			TArray<ATunaSweeperProjectile*> Result;
			for (TActorIterator<ATunaSweeperProjectile> It(World); It; ++It)
			{
				if (IsValid(*It) && !It->IsActorBeingDestroyed() && It->GetOwner() == Weapon) Result.Add(*It);
			}
			return Result;
		}

		ATunaSweeperProjectile* HitEnemy(APawn* Source, ATunaSweeperEnemyCharacter* Enemy,
			const FTunaSweeperBurnSpec& Spec)
		{
			ATunaSweeperProjectile* Projectile = World->SpawnActorDeferred<ATunaSweeperProjectile>(
				ATunaSweeperProjectile::StaticClass(), FTransform::Identity, Source, Source,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (!Projectile) return nullptr;
			Projectile->SetDamageAmount(10.0f);
			Projectile->SetBurnSpec(Spec);
			Projectile->FinishSpawning(FTransform::Identity);
			USphereComponent* Collision = Cast<USphereComponent>(Projectile->GetRootComponent());
			if (!Collision) return nullptr;
			const FHitResult Hit(Enemy, Enemy->GetCapsuleComponent(), Enemy->GetActorLocation(), FVector::UpVector);
			// The transient world has no actor initialization phase; allow its native dynamic delegate to dispatch.
			FEditorScriptExecutionGuard ScriptExecutionGuard;
			// Exercise the same bound hit handler as a physics collision without depending on a world frame.
			Collision->OnComponentHit.Broadcast(Collision, Enemy, Enemy->GetCapsuleComponent(), FVector::ZeroVector, Hit);
			return Projectile;
		}
	};

	bool Fire(ATunaSweeperWeapon* Weapon, APawn* Source, FName WeaponType, const FTunaSweeperBurnSpec& Spec)
	{
		return Weapon->FireWithAimIntent(FVector::ForwardVector, Source, NAME_None, NAME_None, WeaponType,
			1.0f, 0, 0.0f, FVector::ZeroVector, false, nullptr, nullptr, FVector::ZeroVector, false,
			0.0f, true, Spec);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperWeaponBurnResolutionTest,
	"TunaSweeper.Combat.Burn.WeaponAndAmmoResolution", TunaSweeperWeaponBurnTests::TestFlags)

bool FTunaSweeperWeaponBurnResolutionTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	FTunaSweeperItemDefinition Weapon;
	FTunaSweeperItemDefinition Ammo;
	FTunaSweeperResearchBurnBonuses Research;
	Research.AdditionalTickCount = 3;
	Research.DamageMultiplier = 1.5f;

	FTunaSweeperBurnSpec Spec = TunaSweeperBurn::ResolveWeaponBurnSpec(Weapon, Ammo, Research);
	TestFalse(TEXT("Research alone cannot ignite an ordinary weapon and round"), Spec.bEnabled);
	TestEqual(TEXT("Opted-out ammunition deals no burn damage"), Spec.GetDamagePerTick(), 0.0f);

	Ammo.BurnDamagePerTick = 2.0f;
	Spec = TunaSweeperBurn::ResolveWeaponBurnSpec(Weapon, Ammo, FTunaSweeperResearchBurnBonuses());
	TestTrue(TEXT("Incendiary ammunition enables burn with an ordinary weapon"), Spec.bEnabled);
	TestEqual(TEXT("Base ammunition duration is five ticks"), Spec.TickCount, 5);
	TestEqual(TEXT("Base ammunition duration is five seconds"), Spec.GetDurationSeconds(), 5.0f);
	TestEqual(TEXT("Base ammunition damage is unchanged without research"), Spec.GetDamagePerTick(), 2.0f);

	Spec = TunaSweeperBurn::ResolveWeaponBurnSpec(Weapon, Ammo, Research);
	TestEqual(TEXT("Research adds three ticks to the loaded ammunition"), Spec.TickCount, 8);
	TestEqual(TEXT("Research multiplies tick damage"), Spec.GetDamagePerTick(), 3.0f);

	Ammo.BurnDamagePerTick = 0.0f;
	Ammo.BurnTickCount = 90;
	Weapon.BurnDamagePerTick = 4.0f;
	Weapon.BurnTickCount = 7;
	Spec = TunaSweeperBurn::ResolveWeaponBurnSpec(Weapon, Ammo, Research);
	TestTrue(TEXT("An incendiary weapon works with ordinary ammunition"), Spec.bEnabled);
	TestEqual(TEXT("A non-burning ammunition duration cannot extend weapon burn"), Spec.TickCount, 10);
	TestEqual(TEXT("Weapon burn receives the damage multiplier"), Spec.GetDamagePerTick(), 6.0f);

	Ammo.BurnDamagePerTick = 2.0f;
	Ammo.BurnTickCount = 9;
	Spec = TunaSweeperBurn::ResolveWeaponBurnSpec(Weapon, Ammo, Research);
	TestEqual(TEXT("Weapon and ammunition use the stronger damage rather than adding two burns"), Spec.GetDamagePerTick(), 6.0f);
	TestEqual(TEXT("Weapon and ammunition use the longer base duration plus research once"), Spec.TickCount, 12);

	Weapon.BurnDamagePerTick = std::numeric_limits<float>::quiet_NaN();
	Ammo.BurnDamagePerTick = std::numeric_limits<float>::infinity();
	Spec = TunaSweeperBurn::ResolveWeaponBurnSpec(Weapon, Ammo, Research);
	TestFalse(TEXT("Non-finite authored strengths cannot opt in to burn"), Spec.bEnabled);
	Ammo.BurnDamagePerTick = 2.0f;
	Ammo.BurnTickCount = MAX_int32;
	Research.AdditionalTickCount = MAX_int32;
	Spec = TunaSweeperBurn::ResolveWeaponBurnSpec(Weapon, Ammo, Research);
	TestEqual(TEXT("Overflowing base and research duration remains finite and bounded"), Spec.TickCount, FTunaSweeperBurnSpec::MaxTickCount);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperWeaponBurnProjectileSnapshotTest,
	"TunaSweeper.Combat.Burn.ProjectileAndPelletSnapshots", TunaSweeperWeaponBurnTests::TestFlags)

bool FTunaSweeperWeaponBurnProjectileSnapshotTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperWeaponBurnTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Weapon test world exists"), TestWorld.World)) return false;
	APawn* Source = TestWorld.SpawnSource();
	ATunaSweeperWeapon* Weapon = TestWorld.World->SpawnActor<ATunaSweeperWeapon>();
	ATunaSweeperWeapon* Shotgun = TestWorld.World->SpawnActor<ATunaSweeperWeapon>();
	if (!TestNotNull(TEXT("Firing pawn exists"), Source) ||
		!TestNotNull(TEXT("Weapon exists"), Weapon) || !TestNotNull(TEXT("Shotgun exists"), Shotgun)) return false;

	FTunaSweeperBurnSpec Spec;
	Spec.bEnabled = true;
	Spec.BaseDamagePerTick = 2.0f;
	Spec.DamageMultiplier = 1.5f;
	Spec.TickCount = 8;
	if (!TestTrue(TEXT("A burning single shot fires"), Fire(Weapon, Source, NAME_None, Spec))) return false;
	TArray<ATunaSweeperProjectile*> Shots = TestWorld.ProjectilesFrom(Weapon);
	if (!TestEqual(TEXT("Single-shot firing creates one projectile"), Shots.Num(), 1)) return false;
	ATunaSweeperProjectile* FirstShot = Shots[0];
	TestEqual(TEXT("Projectile retains firing pawn"), FirstShot->GetInstigator(), Source);
	TestEqual(TEXT("Projectile receives the resolved tick count"), FirstShot->GetBurnSpec().TickCount, 8);
	TestEqual(TEXT("Projectile receives the resolved damage"), FirstShot->GetBurnSpec().GetDamagePerTick(), 3.0f);

	if (!TestTrue(TEXT("A burning shotgun shot fires"), Fire(Shotgun, Source, FName(TEXT("weapon.type.shotgun")), Spec))) return false;
	const TArray<ATunaSweeperProjectile*> Pellets = TestWorld.ProjectilesFrom(Shotgun);
	TestTrue(TEXT("Shotgun creates multiple pellets"), Pellets.Num() > 1);
	Spec.bEnabled = false;
	Spec.TickCount = 1;
	Spec.BaseDamagePerTick = 90.0f;
	TestTrue(TEXT("Switching to an ordinary round fires"), Fire(Weapon, Source, NAME_None, Spec));
	Shots = TestWorld.ProjectilesFrom(Weapon);
	TestEqual(TEXT("Two single shots remain in flight"), Shots.Num(), 2);
	for (const ATunaSweeperProjectile* Shot : Shots)
	{
		if (Shot == FirstShot) continue;
		TestFalse(TEXT("Subsequent ordinary round does not inherit previous burn"), Shot->GetBurnSpec().bEnabled);
	}
	TestEqual(TEXT("Changing ammunition does not alter an in-flight shot's duration"), FirstShot->GetBurnSpec().TickCount, 8);
	TestEqual(TEXT("Changing ammunition does not alter an in-flight shot's damage"), FirstShot->GetBurnSpec().GetDamagePerTick(), 3.0f);
	for (const ATunaSweeperProjectile* Pellet : Pellets)
	{
		TestEqual(TEXT("Each pellet keeps the original duration snapshot"), Pellet->GetBurnSpec().TickCount, 8);
		TestEqual(TEXT("Each pellet keeps the original damage snapshot"), Pellet->GetBurnSpec().GetDamagePerTick(), 3.0f);
		TestEqual(TEXT("Each pellet keeps its firing pawn"), Pellet->GetInstigator(), Source);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBurnProjectileImpactRoutingTest,
	"TunaSweeper.Combat.Burn.ProjectileImpactRouting", TunaSweeperWeaponBurnTests::TestFlags)

bool FTunaSweeperBurnProjectileImpactRoutingTest::RunTest(const FString& Parameters)
{
	(void)Parameters;
	using namespace TunaSweeperWeaponBurnTests;
	FTestWorld TestWorld;
	if (!TestNotNull(TEXT("Impact test world exists"), TestWorld.World)) return false;
	APawn* Source = TestWorld.SpawnSource();
	ATunaSweeperEnemyCharacter* Hostile = TestWorld.SpawnEnemy();
	ATunaSweeperEnemyCharacter* Friendly = TestWorld.SpawnEnemy(TunaSweeperFactionIds::Player);
	ATunaSweeperEnemyCharacter* OrdinaryHit = TestWorld.SpawnEnemy();
	ATunaSweeperEnemyCharacter* ProtectedEnemy = TestWorld.SpawnEnemy();
	if (!TestNotNull(TEXT("Source exists"), Source) || !TestNotNull(TEXT("Hostile exists"), Hostile) ||
		!TestNotNull(TEXT("Friendly exists"), Friendly) || !TestNotNull(TEXT("Ordinary-hit enemy exists"), OrdinaryHit) ||
		!TestNotNull(TEXT("Non-combat enemy exists"), ProtectedEnemy)) return false;

	FTunaSweeperBurnSpec Spec;
	Spec.bEnabled = true;
	Spec.BaseDamagePerTick = 3.0f;
	ATunaSweeperProjectile* HostileShot = TestWorld.HitEnemy(Source, Hostile, Spec);
	if (!TestNotNull(TEXT("Incendiary projectile exists"), HostileShot)) return false;
	TestTrue(TEXT("Successful hit ignites a hostile enemy"), Hostile->GetBurnComponent()->IsBurning());
	TestTrue(TEXT("Impact destroys the projectile"), HostileShot->IsActorBeingDestroyed());
	static_cast<UActorComponent*>(Hostile->GetBurnComponent())->TickComponent(1.0f, LEVELTICK_All, nullptr);
	TestTrue(TEXT("Burn survives its projectile's destruction"), Hostile->GetBurnComponent()->IsBurning());
	// A lethal damage probe returns the actual remaining health through the public combat API.
	TestEqual(TEXT("One impact and one burn tick deal 10 plus 3 damage"),
		UGameplayStatics::ApplyDamage(Hostile, 100000.0f, nullptr, nullptr, nullptr), 87.0f);
	TestFalse(TEXT("A lethal follow-up clears the active burn"), Hostile->GetBurnComponent()->IsBurning());

	TestNotNull(TEXT("Friendly impact projectile exists"), TestWorld.HitEnemy(Source, Friendly, Spec));
	TestFalse(TEXT("Friendly actor cannot acquire burn on impact"), Friendly->GetBurnComponent()->IsBurning());
	TestEqual(TEXT("Friendly impact also remains blocked by faction damage rules"),
		UGameplayStatics::ApplyDamage(Friendly, 100000.0f, nullptr, nullptr, nullptr), 100.0f);

	TestNotNull(TEXT("Ordinary projectile exists"), TestWorld.HitEnemy(Source, OrdinaryHit, FTunaSweeperBurnSpec()));
	TestFalse(TEXT("Ordinary successful hit cannot acquire burn"), OrdinaryHit->GetBurnComponent()->IsBurning());
	ProtectedEnemy->GetFactionComponent()->SetCanBeCombatTarget(false);
	TestNotNull(TEXT("Non-combat impact projectile exists"), TestWorld.HitEnemy(Source, ProtectedEnemy, Spec));
	TestFalse(TEXT("Non-combat enemy cannot acquire burn"), ProtectedEnemy->GetBurnComponent()->IsBurning());
	return true;
}

#endif
