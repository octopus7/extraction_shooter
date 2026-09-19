#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/WorldSettings.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "Vehicle/TunaSweeperATVActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperATVExplosionTest,
	"TunaSweeper.Vehicle.DelayedExplosion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperATVExplosionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
	UClass* ATVClass = LoadClass<ATunaSweeperATVActor>(nullptr, TEXT("/Game/Blueprints/Vehicles/ATV/BP_ATV_TypeA.BP_ATV_TypeA_C"));
	if (!TestNotNull(TEXT("Saved ATV BP"), ATVClass)) return false;
	World->InitializeActorsForPlay(FURL());
	World->GetWorldSettings()->NotifyBeginPlay();
	World->BeginPlay();
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto MakeATV = [&]() { return World->SpawnActor<ATunaSweeperATVActor>(ATVClass, FVector(0,0,30), FRotator::ZeroRotator, Spawn); };
	auto Advance = [&](float Seconds) { ++GFrameCounter; World->GetTimerManager().Tick(Seconds); };
	auto* ATV = MakeATV();
	TestEqual(TEXT("Default destruction delay is three seconds"), ATV->DestructionExplosionDelay, 3.0f);
	if (!TestNotNull(TEXT("Explosion component exists in saved BP"), ATV->DestructionExplosion.Get())) return false;
	TestNotNull(TEXT("Requested explosion system is loaded"), ATV->DestructionExplosion->GetAsset());
	TestEqual(TEXT("Uses requested NS_Explosion_Tuna"), GetPathNameSafe(ATV->DestructionExplosion->GetAsset()), FString(TEXT("/Game/Effects/ExplosionTuna/NS_Explosion_Tuna.NS_Explosion_Tuna")));
	TestFalse(TEXT("Explosion does not auto-activate"), ATV->DestructionExplosion->bAutoActivate);
	ATV->DestructionExplosionDelay = 0.5f;
	ATV->TakeDamage(1, FDamageEvent(), nullptr, nullptr);
	Advance(1);
	TestFalse(TEXT("Nonlethal damage never starts explosion"), ATV->bDestructionExplosionTriggered);
	ATV->TakeDamage(1000, FDamageEvent(), nullptr, nullptr);
	TestTrue(TEXT("Vehicle is destroyed before burst"), ATV->IsVehicleDestroyed());
	TestFalse(TEXT("No immediate burst with positive delay"), ATV->bDestructionExplosionTriggered);
	Advance(0.3f);
	ATV->TakeDamage(1000, FDamageEvent(), nullptr, nullptr);
	TestFalse(TEXT("Burst waits for complete delay"), ATV->bDestructionExplosionTriggered);
	ATV->SetActorLocation(FVector(400,300,30), false, nullptr, ETeleportType::TeleportPhysics);
	const FVector ExpectedOrigin = ATV->DestructionExplosion->GetComponentLocation();
	Advance(0.21f);
	TestTrue(TEXT("Repeated hit does not reset delay"), ATV->bDestructionExplosionTriggered);
	TestNull(TEXT("Burst detaches from moving wreck"), ATV->DestructionExplosion->GetAttachParent());
	TestTrue(TEXT("Burst uses current wreck location"), ATV->DestructionExplosion->GetComponentLocation().Equals(ExpectedOrigin, 0.01));
	if (FApp::CanEverRender()) TestEqual(TEXT("Niagara burst activated"), ATV->DestructionExplosion->GetRequestedExecutionState(), ENiagaraExecutionState::Active);
	ATV->DestructionExplosion->DeactivateImmediate();
	ATV->TakeDamage(1000, FDamageEvent(), nullptr, nullptr);
	Advance(2);
	TestFalse(TEXT("Further damage never restarts burst"), ATV->DestructionExplosion->IsActive());
	auto* Immediate = MakeATV();
	Immediate->DestructionExplosionDelay = 0;
	Immediate->TakeDamage(1000, FDamageEvent(), nullptr, nullptr);
	TestTrue(TEXT("Zero delay bursts immediately"), Immediate->bDestructionExplosionTriggered);
	auto* Removed = MakeATV();
	Removed->DestructionExplosionDelay = 0.5f;
	Removed->TakeDamage(1000, FDamageEvent(), nullptr, nullptr);
	Removed->Destroy();
	Advance(1);
	TestFalse(TEXT("Removing wreck cancels pending burst"), Removed->bDestructionExplosionTriggered);
	return true;
}
#endif
