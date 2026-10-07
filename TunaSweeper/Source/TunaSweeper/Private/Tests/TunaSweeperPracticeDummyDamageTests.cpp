#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperShootingPracticeDummyActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaSweeperPracticeDummyTests
{
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{
			auto Member = &FContextAccess::WorldContext;
			Instance->*Member = Context;
		}
	};
}

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperPracticeDummyArmorTest,
	"TunaSweeper.Combat.PracticeDummy.ArmorEquipment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperPracticeDummyArmorTest::RunTest(const FString& Parameters)
{
	for (const FName Name : {FName(TEXT("BodyArmorTier")), FName(TEXT("HeadArmorTier"))})
	{
		const FIntProperty* Property = FindFProperty<FIntProperty>(ATunaSweeperShootingPracticeDummyActor::StaticClass(), Name);
		if (!TestNotNull(TEXT("Dummy exposes independently editable armor tier"), Property)) return false;
		TestTrue(TEXT("Armor is editable in BP defaults and placed instances"), Property->HasAllPropertyFlags(CPF_Edit | CPF_BlueprintVisible));
		TestFalse(TEXT("Armor is not restricted to defaults"), Property->HasAnyPropertyFlags(CPF_DisableEditOnInstance));
	}

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Armor dummy world"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UGameInstance> Game(NewObject<UGameInstance>(GEngine));
	TunaSweeperPracticeDummyTests::FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	Game->Init();
	ON_SCOPE_EXIT
	{
		Game->Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		TunaSweeperPracticeDummyTests::FContextAccess::Attach(Game.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	UTunaSweeperItemDataSubsystem* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!TestTrue(TEXT("Actual equipment catalog loads"), Items && Items->LoadItemData())) return false;
	TestEqual(TEXT("Non-armor slot has no armor preset"), Items->FindArmorItemId(TEXT("equipment.slot.weapon"), 1), INDEX_NONE);
	const int32 BodyIds[] = {INDEX_NONE, 5010, 5001, 5022, 5024};
	const int32 HeadIds[] = {INDEX_NONE, 5021, 5006, 5023, 5025};
	const float ExpectedDefense[5][4] = {{0, 0, 0, 0}, {1.5f, .75f, 0, 0},
		{4.5f, 3, 1.5f, 0}, {9, 6.75f, 4.5f, 2.25f}, {12, 12, 9, 6}};
	ATunaSweeperProjectile* Projectile = World->SpawnActor<ATunaSweeperProjectile>();
	if (!TestNotNull(TEXT("Actual projectile causer"), Projectile)) return false;
	FPointDamageEvent BulletHit;
	for (int32 Tier = 0; Tier <= 4; ++Tier)
	{
		ATunaSweeperShootingPracticeDummyActor* Dummy = World->SpawnActor<ATunaSweeperShootingPracticeDummyActor>();
		if (!TestNotNull(TEXT("Tier target"), Dummy)) return false;
		Dummy->ConfigurePracticeDummyArmor(Tier, Tier);
		TestEqual(TEXT("Body resolves to actual catalog item"), Dummy->GetBodyArmorItemId(), BodyIds[Tier]);
		TestEqual(TEXT("Head resolves to actual catalog item"), Dummy->GetHeadArmorItemId(), HeadIds[Tier]);
		TestEqual(TEXT("Base defense matches equipped items"), Dummy->GetEffectiveDefense(), Tier * 3.0f);
		for (int32 Penetration = 1; Penetration <= 4; ++Penetration)
		{
			Projectile->SetPenetrationTier(Penetration);
			const float Expected = 20 - ExpectedDefense[Tier][Penetration - 1];
			const float HealthBefore = Dummy->GetHealthFraction() * 100;
			TestEqual(TEXT("Returned damage uses actual armor penetration"), Dummy->TakeDamage(20, BulletHit, nullptr, Projectile), Expected);
			TestTrue(TEXT("Visible health loses the same damage"), FMath::IsNearlyEqual(HealthBefore - Dummy->GetHealthFraction() * 100, Expected, .001f));
		}
		Dummy->Destroy();
	}
	ATunaSweeperShootingPracticeDummyActor* Mixed = World->SpawnActor<ATunaSweeperShootingPracticeDummyActor>();
	if (!TestNotNull(TEXT("Mixed armor target"), Mixed)) return false;
	Mixed->ConfigurePracticeDummyArmor(2, 3);
	Projectile->SetPenetrationTier(2);
	TestEqual(TEXT("Mixed armor resolves each tier before adding"), Mixed->GetEffectiveDefense(2), 4.25f);
	UStaticMeshComponent* Head = Cast<UStaticMeshComponent>(Mixed->GetDefaultSubobjectByName(TEXT("HeadMesh")));
	if (!TestNotNull(TEXT("Headshot zone"), Head)) return false;
	BulletHit.HitInfo.Component = Head;
	Projectile->SetAimIntent(Mixed, Head, FVector::ZeroVector, true);
	TestEqual(TEXT("Armor is subtracted after aimed headshot multiplier"), Mixed->TakeDamage(20, BulletHit, nullptr, Projectile), 35.75f);
	TestEqual(TEXT("Generic damage cannot borrow penetration or headshot"), Mixed->TakeDamage(20, FDamageEvent(), nullptr, Projectile), 13.0f);
	Mixed->ConfigurePracticeDummyArmor(-1, 9);
	TestEqual(TEXT("Negative tier removes body"), Mixed->BodyArmorTier, 0);
	TestEqual(TEXT("Excessive head tier clamps to four"), Mixed->HeadArmorTier, 4);
	TestEqual(TEXT("Body is removed"), Mixed->GetBodyArmorItemId(), INDEX_NONE);
	TestEqual(TEXT("Head-only defense uses real helmet"), Mixed->GetEffectiveDefense(), 4.0f);
	Mixed->BodyArmorTier = 20;
	Mixed->HeadArmorTier = -4;
	TestEqual(TEXT("Direct BP writes are clamped during resolution"), Mixed->GetEffectiveDefense(), 8.0f);
	const float HealthBeforeBlock = Mixed->GetHealthFraction();
	TestEqual(TEXT("Fully absorbed hit returns zero"), Mixed->TakeDamage(1, FDamageEvent(), nullptr, Projectile), 0.0f);
	TestEqual(TEXT("Fully absorbed hit does not change health"), Mixed->GetHealthFraction(), HealthBeforeBlock);
	Mixed->ConfigurePracticeDummyArmor(0, 0);
	TestEqual(TEXT("Unequipping both slots returns naked damage"), Mixed->TakeDamage(10, FDamageEvent(), nullptr, Projectile), 10.0f);
	World->SetGameInstance(nullptr);
	Mixed->ConfigurePracticeDummyArmor(4, 4);
	TestEqual(TEXT("No catalog safely yields no equipped item"), Mixed->GetHeadArmorItemId(), INDEX_NONE);
	TestEqual(TEXT("No catalog safely yields zero defense"), Mixed->GetEffectiveDefense(), 0.0f);
	World->SetGameInstance(Game.Get());
	return true;
}

#endif
