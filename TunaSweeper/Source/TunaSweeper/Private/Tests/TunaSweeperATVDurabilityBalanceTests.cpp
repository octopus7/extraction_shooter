#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/WorldSettings.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Weapon/TunaSweeperProjectile.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperATVDurabilityBalanceTest,
	"TunaSweeper.Vehicle.DurabilityBalance", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperATVDurabilityBalanceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	UClass* ATVClass = LoadClass<ATunaSweeperATVActor>(nullptr, TEXT("/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA.BP_ATV_TypeA_C"));
	if (!TestNotNull(TEXT("Production ATV Blueprint"), ATVClass)) return false;
	auto* ATV = World->SpawnActor<ATunaSweeperATVActor>(ATVClass, FVector::ZeroVector, FRotator::ZeroRotator, Spawn);
	auto* Enemy = World->SpawnActor<ATunaSweeperEnemyCharacter>(ATunaSweeperEnemyCharacter::StaticClass(), FVector(1000,0,0), FRotator::ZeroRotator, Spawn);
	auto* Projectile = World->SpawnActor<ATunaSweeperProjectile>(ATunaSweeperProjectile::StaticClass(), FVector(2000,0,0), FRotator::ZeroRotator, Spawn);
	if (!TestNotNull(TEXT("ATV"), ATV) || !TestNotNull(TEXT("Enemy"), Enemy) || !TestNotNull(TEXT("Bullet"), Projectile)) return false;
	TestEqual(TEXT("Production vehicle has expected durability budget"), ATV->MaxDurability, 300.0f);
	World->InitializeActorsForPlay(FURL());
	World->GetWorldSettings()->NotifyBeginPlay();
	World->GetWorldSettings()->NotifyMatchStarted();
	World->BeginPlay();
	TestEqual(TEXT("Runtime durability initializes from the authored maximum"), ATV->CurrentDurability, 300.0f);
	Projectile->SetOwner(Enemy);
	FPointDamageEvent BulletHit;
	for (int32 Shot = 0; Shot < 9; ++Shot)
		TestEqual(TEXT("Enemy rounds retain their normal damage"), ATV->TakeDamage(10, BulletHit, nullptr, Projectile), 10.0f);
	TestEqual(TEXT("Three bursts leave time to approach and run over enemies"), ATV->CurrentDurability, 210.0f);
	TestFalse(TEXT("Extra durability does not make the vehicle invulnerable"), ATV->CurrentDurability == ATV->MaxDurability);
	TestEqual(TEXT("Small enemy bullets still chip durability"), ATV->TakeDamage(1, BulletHit, nullptr, Projectile), 1.0f);
	TestEqual(TEXT("Enemy melee remains unchanged"), ATV->TakeDamage(20, FDamageEvent(), nullptr, Enemy), 20.0f);
	TestEqual(TEXT("Radial damage remains unchanged even from enemy projectile"), ATV->TakeDamage(20, FRadialDamageEvent(), nullptr, Projectile), 20.0f);
	Projectile->SetOwner(nullptr);
	Projectile->SetInstigator(Enemy);
	TestEqual(TEXT("Enemy bullets without direct enemy owner retain damage"), ATV->TakeDamage(20, BulletHit, nullptr, Projectile), 20.0f);
	Projectile->SetInstigator(nullptr);
	TestEqual(TEXT("Unattributed and player bullets are not reduced"), ATV->TakeDamage(20, BulletHit, nullptr, Projectile), 20.0f);
	TestEqual(TEXT("Other point damage remains unchanged"), ATV->TakeDamage(20, BulletHit, nullptr, Enemy), 20.0f);
	ATV->SetCanBeDamaged(false);
	Projectile->SetOwner(Enemy);
	TestEqual(TEXT("Existing invulnerability is respected"), ATV->TakeDamage(20, BulletHit, nullptr, Projectile), 0.0f);
	return true;
}
#endif
