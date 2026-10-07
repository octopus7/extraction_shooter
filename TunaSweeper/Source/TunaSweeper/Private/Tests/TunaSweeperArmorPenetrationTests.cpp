#if WITH_DEV_AUTOMATION_TESTS

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Combat/TunaSweeperArmor.h"
#include "Engine/DamageEvents.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaSweeperArmorTests
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperArmorPenetrationTest,
	"TunaSweeper.Combat.Armor.FourTiersAndActualDamage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperArmorPenetrationTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperArmorTests;
	const float ExpectedDefense[4][4] = {
		{1.5f, 0.75f, 0.0f, 0.0f}, {4.5f, 3.0f, 1.5f, 0.0f},
		{9.0f, 6.75f, 4.5f, 2.25f}, {12.0f, 12.0f, 9.0f, 6.0f}};
	const int32 BodyIds[] = {5010, 5001, 5022, 5024};
	const int32 HeadIds[] = {5021, 5006, 5023, 5025};
	TestEqual(TEXT("Legacy armor keeps flat defense"), TunaSweeperArmor::EffectiveDefense(8, 0, 4), 8.0f);
	TestEqual(TEXT("Non-ballistic damage has no penetration"), TunaSweeperArmor::EffectiveDefense(8, 4, 0), 8.0f);
	TestEqual(TEXT("Negative defense does not increase damage"), TunaSweeperArmor::EffectiveDefense(-8, 4, 1), 0.0f);
	TestEqual(TEXT("Armor never heals the target"), TunaSweeperArmor::ApplyDefense(1.0f, 12.0f), 0.0f);

	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Armor test world"), World)) return false;
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	Game->bInventoryStateInitialized = true;
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	UTunaSweeperItemDataSubsystem* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	if (!TestTrue(TEXT("Armor catalog loads"), Items && Items->LoadItemData())) return false;
	ATunaSweeperProjectile* Projectile = World->SpawnActor<ATunaSweeperProjectile>();
	if (!TestNotNull(TEXT("Actual damage causer"), Projectile)) return false;
	FPointDamageEvent BulletHit;
	for (int32 Tier = 1; Tier <= 4; ++Tier)
	{
		Game->ItemInstancesByUid.Reset();
		Game->ResetPlayerSlotArrays();
		const FString Loadout = FString::Printf(TEXT(R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002},{"slot_index":3,"item_id":%d},{"slot_index":4,"item_id":%d}],"inventory":[]})"),
			HeadIds[Tier-1], BodyIds[Tier-1]);
		if (!TestTrue(TEXT("Armor equip loadout"), Game->ApplyStartingLoadoutJson(Loadout))) return false;
		TestEqual(TEXT("Full defense sum"), Game->GetEquippedDefenseValue(), Tier * 3);
		for (const int32 Id : {BodyIds[Tier-1], HeadIds[Tier-1]})
		{
			FTunaSweeperItemDefinition Item;
			TestTrue(TEXT("Armor item exists"), Items->TryGetItemDefinition(Id, Item));
			TestEqual(TEXT("Authored tier"), Item.ArmorTier, Tier);
		}
		ATunaSweeperTopDownCharacter* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>();
		ATunaSweeperEnemyCharacter* Enemy = World->SpawnActor<ATunaSweeperEnemyCharacter>();
		if (!TestNotNull(TEXT("Player"), Player) || !TestNotNull(TEXT("Enemy"), Enemy)) return false;
		Enemy->AutoPossessAI = EAutoPossessAI::Disabled;
		Enemy->ConfigureSpawnData({}, NAME_None, INDEX_NONE, INDEX_NONE, 200.0f, 0);
		Enemy->BodyArmorItemId = BodyIds[Tier-1];
		Enemy->HeadArmorItemId = HeadIds[Tier-1];
		for (int32 Penetration = 1; Penetration <= 4; ++Penetration)
		{
			Projectile->SetPenetrationTier(Penetration);
			const float Defense = ExpectedDefense[Tier-1][Penetration-1];
			TestEqual(TEXT("Per-item defense matches four-by-four balance table"), Game->GetEquippedEffectiveDefense(Penetration), Defense);
			const float EnemyHealthBefore = Enemy->GetHealth();
			TestEqual(TEXT("Enemy takes damage after penetration"), Enemy->TakeDamage(20.0f, BulletHit, nullptr, Projectile), 20.0f - Defense);
			TestEqual(TEXT("Enemy health actually changes"), EnemyHealthBefore - Enemy->GetHealth(), 20.0f - Defense);
			const float PlayerHealthBefore = Player->GetVitalsComponent()->GetVitalsState().Health;
			TestEqual(TEXT("Player uses the same armor calculation"), Player->TakeDamage(20.0f, BulletHit, nullptr, Projectile), 20.0f - Defense);
			TestEqual(TEXT("Player health actually changes"), PlayerHealthBefore - Player->GetVitalsComponent()->GetVitalsState().Health, 20.0f - Defense);
		}
		TestEqual(TEXT("A non-point event cannot borrow projectile penetration"),
			TunaSweeperArmor::ResolvePenetrationTier(FDamageEvent(), Projectile), 0);
		Player->Destroy();
		Enemy->Destroy();
	}
	Game->ItemInstancesByUid.Reset();
	Game->ResetPlayerSlotArrays();
	TestTrue(TEXT("Mixed armor tiers equip"), Game->ApplyStartingLoadoutJson(TEXT(
		R"({"selected_weapon_slot":1,"equipment":[{"slot_index":0,"item_id":1002},{"slot_index":3,"item_id":5023},{"slot_index":4,"item_id":5001}],"inventory":[]})")));
	TestEqual(TEXT("Mixed tiers are resolved separately before adding"), Game->GetEquippedEffectiveDefense(2), 4.25f);
	const int32 AmmoIds[][4] = {{2011,2001,2012,2013},{2021,2002,2022,2024},{2031,2003,2032,2033}};
	for (const auto& Caliber : AmmoIds)
	{
		for (int32 Tier = 1; Tier <= 4; ++Tier)
		{
			FTunaSweeperItemDefinition Ammo;
			TestTrue(TEXT("Each caliber provides all four ammunition tiers"), Items->TryGetItemDefinition(Caliber[Tier-1], Ammo));
			TestEqual(TEXT("Ammunition penetration tier"), Ammo.PenetrationTier, Tier);
		}
	}
	return true;
}

#endif
