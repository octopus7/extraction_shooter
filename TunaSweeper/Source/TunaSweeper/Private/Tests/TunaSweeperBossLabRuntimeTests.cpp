#if WITH_DEV_AUTOMATION_TESTS

#include "AI/TunaSweeperAttackTelegraph.h"
#include "BossLab/TunaSweeperModularBoss.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Misc/AutomationTest.h"
#include "TunaSweeperCollisionChannels.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaBossLabRuntimeTests
{
	constexpr auto Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
	struct FWorld
	{
		UWorld* World = nullptr;
		FWorld()
		{
			const UWorld::InitializationValues Values = UWorld::InitializationValues()
				.AllowAudioPlayback(false).RequiresHitProxies(false).CreatePhysicsScene(true)
				.CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false)
				.EnableTraceCollision(true).SetTransactional(false);
			World = UWorld::CreateWorld(EWorldType::Game, false,
				MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("BossLabRuntimeTest")),
				GetTransientPackage(), true, ERHIFeatureLevel::Num, &Values);
			if (World && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
		}
		~FWorld()
		{
			if (!World) return;
			World->DestroyWorld(false);
			if (GEngine) GEngine->DestroyWorldContext(World);
			World->RemoveFromRoot();
		}
		ATunaSweeperModularBoss* Boss(const FTunaSweeperBossDefinition& Definition, bool bPreview = false)
		{
			if (!World) return nullptr;
			auto* Result = World->SpawnActor<ATunaSweeperModularBoss>();
			FName Error;
			if (!Result || !Result->InitializeBoss(Definition, bPreview, Error)) return nullptr;
			Result->SetActorLocation(FVector(14400.f, 10000.f, Result->GetGroundOffset()));
			return Result;
		}
		APawn* Target()
		{
			auto* Result = World->SpawnActor<APawn>();
			auto* Box = NewObject<UBoxComponent>(Result);
			Result->SetRootComponent(Box); Result->AddInstanceComponent(Box);
			Box->SetBoxExtent(FVector(35.f, 35.f, 88.f)); Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Box->RegisterComponent(); Result->SetActorLocation(FVector(15500.f, 10000.f, 88.f));
			return Result;
		}
		template<typename T> int32 Count() const
		{
			int32 Result = 0;
			for (TActorIterator<T> It(World); It; ++It) if (!It->IsActorBeingDestroyed()) ++Result;
			return Result;
		}
	};
	float Hit(ATunaSweeperModularBoss* Boss, int32 Id, float Damage)
	{
		FPointDamageEvent Event;
		Event.Damage = Damage;
		Event.HitInfo.Component = Boss->GetPartComponent(Id);
		return Boss->TakeDamage(Damage, Event, nullptr, nullptr);
	}
	FTunaSweeperBossDefinition BranchDesign()
	{
		FTunaSweeperBossDefinition Definition;
		Definition.Parts = {{1, TEXT("core"), INDEX_NONE, 0, 0},
			{2, TEXT("frame"), 1, 0, 0}, {3, TEXT("cannon"), 2, 0, 0}};
		return Definition;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaBossLabPartDamageTest,
	"TunaSweeper.BossLab.Runtime.PartDamageAndReset", TunaBossLabRuntimeTests::Flags)
bool FTunaBossLabPartDamageTest::RunTest(const FString& Parameters)
{
	using namespace TunaBossLabRuntimeTests;
	FWorld Scope;
	const auto Definition = BranchDesign();
	auto* Boss = Scope.Boss(Definition);
	if (!TestNotNull(TEXT("Validated assembly spawns"), Boss)) return false;
	const float CoreBefore = Boss->GetCoreHealth();
	TestEqual(TEXT("Point hit applies to selected part"), Hit(Boss, 2, 35.f), 35.f);
	TestEqual(TEXT("Shooting frame preserves core health"), Boss->GetCoreHealth(), CoreBefore);
	TestTrue(TEXT("Frame durability decreases"), Boss->GetPartHealth(2) < TunaSweeperBossDefinition::FindModule(TEXT("frame"))->Health);
	Hit(Boss, 2, 10000.f);
	TestFalse(TEXT("Destroyed frame stops functioning"), Boss->IsPartOperational(2));
	TestFalse(TEXT("Child weapon stops when parent is destroyed"), Boss->IsPartOperational(3));
	TestEqual(TEXT("Disabled child cannot intercept bullets"), Boss->GetPartComponent(3)->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestTrue(TEXT("Destroying frame does not end core encounter"), !Boss->IsDefeated());
	Hit(Boss, 1, 10000.f);
	TestTrue(TEXT("Core destruction wins"), Boss->IsDefeated());
	FName Error;
	TestTrue(TEXT("Same immutable design can start again"), Boss->InitializeBoss(Definition, false, Error));
	TestEqual(TEXT("Restart restores full core"), Boss->GetCoreHealth(), CoreBefore);
	TestTrue(TEXT("Restart restores child weapon"), Boss->IsPartOperational(3));
	TestEqual(TEXT("Restart has exactly the defined active parts"), Boss->GetOperationalPartCount(), Definition.Parts.Num());
	TestTrue(TEXT("Preview can reuse definition"), Boss->InitializeBoss(Definition, true, Error));
	TestEqual(TEXT("Preview is immune to combat damage"), Hit(Boss, 1, 50.f), 0.f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaBossLabCancelTest,
	"TunaSweeper.BossLab.Runtime.AttackCancellation", TunaBossLabRuntimeTests::Flags)
bool FTunaBossLabCancelTest::RunTest(const FString& Parameters)
{
	using namespace TunaBossLabRuntimeTests;
	FWorld Scope;
	auto* Boss = Scope.Boss(BranchDesign());
	if (!TestNotNull(TEXT("Combat assembly spawns"), Boss)) return false;
	auto* Target = Scope.Target();
	Boss->BeginCombat(Target);
	for (int32 Step = 0; Step < 30 && !Boss->IsWarningActive(); ++Step) Boss->Tick(0.1f);
	TestTrue(TEXT("Attack is advertised before firing"), Boss->IsWarningActive());
	TestEqual(TEXT("Cannon warning has not yet spawned its projectile"), Scope.Count<ATunaSweeperProjectile>(), 0);
	Hit(Boss, 2, 10000.f);
	TestFalse(TEXT("Destroying weapon parent cancels pending attack immediately"), Boss->IsWarningActive());
	TestEqual(TEXT("Destroyed branch leaves no warning"), Scope.Count<ATunaSweeperAttackTelegraph>(), 0);
	for (int32 Step = 0; Step < 20; ++Step) Boss->Tick(0.1f);
	TestEqual(TEXT("Cancelled weapon never fires later"), Scope.Count<ATunaSweeperProjectile>(), 0);
	FName Error; Boss->InitializeBoss(BranchDesign(), false, Error); Boss->BeginCombat(Target);
	for (int32 Step = 0; Step < 30; ++Step) Boss->Tick(0.1f);
	TestTrue(TEXT("Restored cannon actually fires"), Scope.Count<ATunaSweeperProjectile>() > 0);
	Boss->StopCombat();
	TestEqual(TEXT("Stop removes every owned projectile"), Scope.Count<ATunaSweeperProjectile>(), 0);
	TestEqual(TEXT("Stop removes every warning"), Scope.Count<ATunaSweeperAttackTelegraph>(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaBossLabSupportSettlementTest,
	"TunaSweeper.BossLab.Runtime.SupportSettlement", TunaBossLabRuntimeTests::Flags)
bool FTunaBossLabSupportSettlementTest::RunTest(const FString& Parameters)
{
	using namespace TunaBossLabRuntimeTests;
	FWorld Scope;
	FTunaSweeperBossDefinition Definition;
	Definition.Parts = {{1, TEXT("core"), INDEX_NONE, 0, 0},
		{2, TEXT("frame"), 1, 5, 0}, {3, TEXT("frame"), 2, 5, 0}, {4, TEXT("drive"), 3, 5, 0}};
	auto* Boss = Scope.Boss(Definition);
	if (!TestNotNull(TEXT("Tall legal assembly spawns"), Boss)) return false;
	const float OriginalHeight = Boss->GetActorLocation().Z;
	Hit(Boss, 4, 10000.f);
	TestTrue(TEXT("Breaking lowest support settles the assembly"), Boss->GetActorLocation().Z < OriginalHeight);
	TestTrue(TEXT("Next support remains available"), Boss->IsPartOperational(3));
	Hit(Boss, 3, 10000.f); Hit(Boss, 2, 10000.f);
	const auto* Core = Boss->GetPartComponent(1);
	TestTrue(TEXT("With supports gone core reaches the firing plane"), Core->Bounds.GetBox().Min.Z < 20.f);
	FHitResult HitResult;
	const FVector Center = Boss->GetActorLocation();
	TestTrue(TEXT("A horizontal projectile at 90cm can reach core"), Scope.World->LineTraceSingleByChannel(HitResult,
		FVector(Center.X - 1000.f, Center.Y, 90.f), FVector(Center.X + 1000.f, Center.Y, 90.f), TunaSweeperCollisionChannels::Projectile));
	TestEqual(TEXT("Horizontal hit resolves to surviving core"), HitResult.GetComponent(), static_cast<UPrimitiveComponent*>(Boss->GetPartComponent(1)));
	Hit(Boss, 1, 10000.f);
	TestTrue(TEXT("A formerly raised core remains defeatable"), Boss->IsDefeated());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaBossLabAsymmetricMovementTest,
	"TunaSweeper.BossLab.Runtime.AsymmetricMovementClearance", TunaBossLabRuntimeTests::Flags)
bool FTunaBossLabAsymmetricMovementTest::RunTest(const FString& Parameters)
{
	using namespace TunaBossLabRuntimeTests;
	FWorld Scope;
	auto Definition = BranchDesign();
	Definition.Parts.Add({4, TEXT("drive"), 1, 5, 0});
	Definition.Tactic = ETunaSweeperBossTactic::Advance;
	auto* Boss = Scope.Boss(Definition);
	if (!TestNotNull(TEXT("Asymmetric assembly spawns"), Boss)) return false;
	TestTrue(TEXT("Clearance includes far corners measured from core root"), Boss->GetMovementRadius() > 610.f);
	auto* Target = Scope.Target();
	Target->SetActorLocation(Boss->GetActorLocation() + FVector(650.f, 0.f, 0.f));
	const FVector Start = Boss->GetActorLocation();
	Boss->BeginCombat(Target); Boss->Tick(0.1f);
	TestTrue(TEXT("Long front branch cannot advance into nearby player"), Boss->GetActorLocation().Equals(Start, 0.1f));
	Boss->SetActorLocation(FVector(16480.f, 10000.f, Start.Z));
	Boss->KeepInsideArena();
	TestTrue(TEXT("Spawn clamp keeps far front part inside arena"), Boss->GetPartComponent(3)->Bounds.GetBox().Max.X <= 16500.1f);
	const float FullRadius = Boss->GetMovementRadius();
	Hit(Boss, 2, 10000.f);
	TestTrue(TEXT("Destroyed side branch no longer enlarges movement clearance"), Boss->GetMovementRadius() < FullRadius);
	return true;
}

#endif
