#if WITH_DEV_AUTOMATION_TESTS

#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperExplosiveBarrelActor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBreakableConcreteWallTest,
	"TunaSweeper.Interaction.BreakableConcreteWall.BarrelExplosion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperBreakableConcreteWallTest::RunTest(const FString& Parameters)
{
	auto* WallClass = LoadClass<AActor>(nullptr,
		TEXT("/Game/Interaction/BP_BreakableConcreteWall.BP_BreakableConcreteWall_C"));
	auto* BarrelClass = LoadClass<ATunaSweeperExplosiveBarrelActor>(nullptr,
		TEXT("/Game/Interaction/BP_ExplosiveBarrel.BP_ExplosiveBarrel_C"));
	if (!TestNotNull(TEXT("Saved wall Blueprint"), WallClass) ||
		!TestNotNull(TEXT("Existing barrel Blueprint"), BarrelClass))
	{
		return false;
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ON_SCOPE_EXIT { World->DestroyWorld(false); World->RemoveFromRoot(); };
	auto* Barrel = World->SpawnActor<ATunaSweeperExplosiveBarrelActor>(BarrelClass, FVector::ZeroVector, FRotator::ZeroRotator);
	auto* NearWall = World->SpawnActor<AActor>(WallClass, FVector(0, 150, 0), FRotator::ZeroRotator);
	auto* FarWall = World->SpawnActor<AActor>(WallClass, FVector(0, 3000, 0), FRotator::ZeroRotator);
	if (!Barrel || !NearWall || !FarWall)
	{
		AddError(TEXT("Could not spawn the wall/barrel fixtures"));
		return false;
	}
	Barrel->DispatchBeginPlay();

	const auto* Blocker = NearWall->FindComponentByClass<UBoxComponent>();
	if (!TestNotNull(TEXT("Wall has independent collision"), Blocker)) return false;
	TestEqual(TEXT("Intact wall blocks pawns"), Blocker->GetCollisionResponseToChannel(ECC_Pawn), ECR_Block);
	TestEqual(TEXT("Ordinary damage leaves wall intact"), UGameplayStatics::ApplyDamage(NearWall, 1000.f, nullptr, nullptr, nullptr), 0.0f);
	TestEqual(TEXT("An intact barrel cannot break the wall"), UGameplayStatics::ApplyDamage(NearWall, 1000.f, nullptr, Barrel, nullptr), 0.0f);
	TestTrue(TEXT("Wall remains intact before detonation"), IsValid(NearWall) && !NearWall->IsActorBeingDestroyed());

	UGameplayStatics::ApplyDamage(Barrel, Barrel->GetMaxHealth(), nullptr, nullptr, nullptr);
	TestTrue(TEXT("The actual barrel detonated"), Barrel->IsBarrelDestroyed());
	TestTrue(TEXT("The barrel blast destroyed the nearby Blueprint wall"), !IsValid(NearWall) || NearWall->IsActorBeingDestroyed());
	TestTrue(TEXT("The wall outside the blast remains intact"), IsValid(FarWall) && !FarWall->IsActorBeingDestroyed());
	return true;
}

#endif
