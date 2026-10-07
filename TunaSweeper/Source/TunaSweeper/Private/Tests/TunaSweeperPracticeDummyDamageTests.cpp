#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperShootingPracticeDummyActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Weapon/TunaSweeperProjectile.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperPracticeDummyDamageTest,
	"TunaSweeper.Combat.PracticeDummy.HitDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperPracticeDummyDamageTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world exists"), World)) return false;
	ON_SCOPE_EXIT { World->DestroyWorld(false); World->RemoveFromRoot(); };
	ATunaSweeperShootingPracticeDummyActor* Dummy = World->SpawnActor<ATunaSweeperShootingPracticeDummyActor>();
	ATunaSweeperShootingPracticeDummyActor* OtherDummy = World->SpawnActor<ATunaSweeperShootingPracticeDummyActor>();
	ATunaSweeperProjectile* Projectile = World->SpawnActor<ATunaSweeperProjectile>();
	if (!TestNotNull(TEXT("Dummy exists"), Dummy) ||
		!TestNotNull(TEXT("Other dummy exists"), OtherDummy) ||
		!TestNotNull(TEXT("Projectile exists"), Projectile)) return false;

	TArray<UStaticMeshComponent*> Meshes;
	Dummy->GetComponents(Meshes);
	TestTrue(TEXT("Dummy has hittable meshes"), !Meshes.IsEmpty());
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		FHitResult Hit;
		Hit.Component = Mesh;
		FPointDamageEvent DamageEvent(10.0f, Hit, FVector::ForwardVector, nullptr);
		Projectile->SetAimIntent(nullptr, nullptr, FVector::ZeroVector, false);
		TestEqual(FString::Printf(TEXT("Unintentional hit on %s preserves incoming damage"), *Mesh->GetName()),
			Dummy->TakeDamage(10.0f, DamageEvent, nullptr, Projectile), 10.0f);
		TestFalse(TEXT("Unintentional hit does not receive headshot feedback"), Dummy->IsHeadshotHit(Mesh, Projectile));
	}

	for (const FName HeadName : {FName(TEXT("HeadMesh")), FName(TEXT("HeadshotPlateMesh"))})
	{
		UStaticMeshComponent* Head = Cast<UStaticMeshComponent>(Dummy->GetDefaultSubobjectByName(HeadName));
		if (!TestNotNull(TEXT("Head hit zone exists"), Head)) continue;
		FHitResult Hit;
		Hit.Component = Head;
		FPointDamageEvent DamageEvent(10.0f, Hit, FVector::ForwardVector, nullptr);
		Projectile->SetAimIntent(Dummy, Head, FVector::ZeroVector, true);
		TestEqual(TEXT("Aimed headshot adds 100 percent damage"),
			Dummy->TakeDamage(10.0f, DamageEvent, nullptr, Projectile), 20.0f);
		TestTrue(TEXT("Twofold damage retains headshot feedback"), Dummy->IsHeadshotHit(Head, Projectile));
		UStaticMeshComponent* Body = Cast<UStaticMeshComponent>(Dummy->GetDefaultSubobjectByName(TEXT("BodyMesh")));
		if (TestNotNull(TEXT("Body hit zone exists"), Body))
		{
			FHitResult BodyHit;
			BodyHit.Component = Body;
			FPointDamageEvent BodyDamageEvent(10.0f, BodyHit, FVector::ForwardVector, nullptr);
			TestEqual(TEXT("Aiming at the head but hitting the body does not multiply damage"),
				Dummy->TakeDamage(10.0f, BodyDamageEvent, nullptr, Projectile), 10.0f);
			TestFalse(TEXT("Body hit does not receive headshot feedback"), Dummy->IsHeadshotHit(Body, Projectile));
		}
		FDamageEvent GenericDamage;
		TestEqual(TEXT("Damage without a hit zone does not become a headshot"),
			Dummy->TakeDamage(10.0f, GenericDamage, nullptr, Projectile), 10.0f);
		Projectile->SetAimIntent(OtherDummy, Head, FVector::ZeroVector, true);
		TestEqual(TEXT("Aiming at another target does not grant headshot damage"),
			Dummy->TakeDamage(10.0f, DamageEvent, nullptr, Projectile), 10.0f);
	}
	return true;
}

#endif
