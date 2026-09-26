#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/World.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Weapon/TunaSweeperWeapon.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMissingProgressMaterialTest,
	"TunaSweeper.Data.WeaponConfiguration.MissingProgressMaterial",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperMissingProgressMaterialTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	ATunaSweeperWorldProgressActor* Actor = World->SpawnActor<ATunaSweeperWorldProgressActor>();
	TestEqual(TEXT("Unconfigured progress has no implicit material"), Actor->GetRequiredItemId(), INDEX_NONE);
	AddExpectedError(TEXT("World progress requires an explicit positive item ID"), EAutomationExpectedErrorFlags::Contains, 1);
	Actor->ConfigureWorldProgressDefaults(TEXT("test.missing"), NAME_None, FText::GetEmpty(),
		FText::GetEmpty(), INDEX_NONE, 1, 1, FText::GetEmpty(), FVector(10), nullptr);
	TestEqual(TEXT("Missing material is not replaced with wood"), Actor->GetRequiredItemId(), INDEX_NONE);
	TestFalse(TEXT("Pre-filled progress cannot bypass missing material"), Actor->IsRepairReady());
	TestFalse(TEXT("Missing material cannot complete repair"), Actor->Repair(false));
	TestFalse(TEXT("Missing material cannot complete combined repair"), Actor->RepairUsingAvailableRequiredItems(false));
	World->DestroyWorld(false);
	World->RemoveFromRoot();
	const UClass* BridgeClass = LoadClass<ATunaSweeperWorldProgressActor>(nullptr,
		TEXT("/Game/Interaction/BP_WorldProgress_BrokenBridge.BP_WorldProgress_BrokenBridge_C"));
	if (TestNotNull(TEXT("Existing bridge class loads"), BridgeClass))
	{
		TestEqual(TEXT("Existing bridge explicitly requires wood"), BridgeClass->GetDefaultObject<ATunaSweeperWorldProgressActor>()->GetRequiredItemId(), 6002);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperWeaponConfigurationFilesTest,
	"TunaSweeper.Data.WeaponConfiguration.AuthoredFiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperWeaponConfigurationFilesTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* File : { TEXT("WeaponVisualDefinitions.json"), TEXT("EnemyDefaultLoadout.json"), TEXT("CombatLabEnemyLoadout.json") })
	{
		FString Content;
		TestTrue(File, FFileHelper::LoadFileToString(Content, *FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"), File)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperWeaponConfigurationDataTest,
	"TunaSweeper.Data.WeaponConfiguration.ResolutionAndValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperWeaponConfigurationDataTest::RunTest(const FString& Parameters)
{
	auto* Game = NewObject<UGameInstance>();
	auto* Items = NewObject<UTunaSweeperItemDataSubsystem>(Game);
	if (!TestTrue(TEXT("Authored item and weapon data loads"), Items->LoadItemData())) return false;
	FTunaSweeperEnemyWeaponLoadout Loadout;
	TestTrue(TEXT("Missing spawn equipment resolves defaults"), Items->TryResolveEnemyLoadout(INDEX_NONE, INDEX_NONE, INDEX_NONE, Loadout));
	TestEqual(TEXT("Default rifle"), Loadout.WeaponItemId, 1002);
	TestEqual(TEXT("Default rifle ammo"), Loadout.AmmoItemId, 2002);
	TestEqual(TEXT("Two reserve magazines"), Loadout.ReserveAmmoCount, 60);
	TestTrue(TEXT("Incompatible rifle ammo on SMG is replaced by configured ammo"), Items->TryResolveEnemyLoadout(1006, 2002, 17, Loadout));
	TestEqual(TEXT("SMG ammo"), Loadout.AmmoItemId, 2001);
	TestEqual(TEXT("Explicit reserve is preserved"), Loadout.ReserveAmmoCount, 17);
	TestTrue(TEXT("Lab authored loadout"), Items->TryGetCombatLabEnemyLoadout(Loadout));
	TestEqual(TEXT("Lab reserve"), Loadout.ReserveAmmoCount, 120);

	for (int32 Id : {1004, 1005, 6003})
	{
		FTunaSweeperWeaponVisualDefinition Visual;
		if (TestTrue(TEXT("Existing melee visual is defined"), Items->TryGetWeaponVisualDefinition(Id, Visual)))
		{
			TestNotNull(TEXT("Mesh path resolves to a mesh"), Cast<UStaticMesh>(Visual.Mesh.TryLoad()));
			TestNotNull(TEXT("Material path resolves to a material"), Cast<UMaterialInterface>(Visual.Material.TryLoad()));
		}
	}
	// A new item ID must work without adding a switch case in the consumer.
	auto NewMelee = Items->ItemDefinitionsById.FindChecked(1004);
	NewMelee.Id = 91004;
	Items->ItemDefinitionsById.Add(NewMelee.Id, NewMelee);
	const FString CustomVisual = TEXT(R"([{"item_id":91004,"mesh":"/Game/Weapons/SM_BaseballBat.SM_BaseballBat","material":"/Game/Weapons/M_BaseballBat_Wood.M_BaseballBat_Wood","location":[9,8,7],"rotation":[1,2,3],"scale":[2,3,4],"align_longest_axis_to_x":false,"center_on_bounds":false,"fit_length_cm":0}])");
	TestTrue(TEXT("New ID mesh data parses"), Items->ParseWeaponVisualDefinitions(CustomVisual));
	FTunaSweeperWeaponVisualDefinition Visual;
	TestTrue(TEXT("New ID visual resolves"), Items->TryGetWeaponVisualDefinition(91004, Visual));
	TestEqual(TEXT("Authored visual location"), Visual.Location, FVector(9,8,7));
	TestEqual(TEXT("Authored visual scale"), Visual.Scale, FVector(2,3,4));
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	auto* Weapon = World->SpawnActor<ATunaSweeperWeapon>();
	TestTrue(TEXT("Runtime weapon applies new ID visual"), Weapon->ApplyVisualDefinition(Visual));
	UStaticMeshComponent* MeshComponent = Weapon->FindComponentByClass<UStaticMeshComponent>();
	if (TestNotNull(TEXT("Weapon mesh component"), MeshComponent))
	{
		TestEqual(TEXT("Runtime mesh location follows data"), MeshComponent->GetRelativeLocation(), FVector(9,8,7));
		TestEqual(TEXT("Runtime mesh scale follows data"), MeshComponent->GetRelativeScale3D(), FVector(2,3,4));
		TestEqual(TEXT("Runtime mesh rotation follows data"), MeshComponent->GetRelativeRotation(), FRotator(1,2,3));
		TestEqual(TEXT("Runtime mesh path follows data"), FSoftObjectPath(MeshComponent->GetStaticMesh()), Visual.Mesh);
	}
	World->DestroyWorld(false);
	World->RemoveFromRoot();
	for (const FString& Invalid : {
		FString(TEXT("{}")), CustomVisual.Replace(TEXT("91004"), TEXT("999999")),
		CustomVisual.Replace(TEXT("91004"), TEXT("91004.5")), CustomVisual.Replace(TEXT("[2,3,4]"), TEXT("[0,3,4]")),
		CustomVisual.LeftChop(1) + TEXT(",") + CustomVisual.Mid(1) })
	{
		TestFalse(TEXT("Invalid visual data rejects entire replacement"), Items->ParseWeaponVisualDefinitions(Invalid));
		TestTrue(TEXT("Failed parse keeps previous visual"), Items->TryGetWeaponVisualDefinition(91004, Visual));
	}
	const FString CustomDefault = TEXT(R"({"weapon_item_id":1006,"reserve_magazine_count":3,"ammo_by_weapon_type":[{"weapon_type_tag":"weapon.type.smg","ammo_item_id":2001}]})");
	TestTrue(TEXT("Custom default parses"), Items->ParseEnemyDefaultLoadout(CustomDefault));
	TestTrue(TEXT("Custom default resolves"), Items->TryResolveEnemyLoadout(INDEX_NONE, INDEX_NONE, INDEX_NONE, Loadout));
	TestEqual(TEXT("Configured SMG is default"), Loadout.WeaponItemId, 1006);
	TestEqual(TEXT("Three 32-round magazines"), Loadout.ReserveAmmoCount, 96);
	TestFalse(TEXT("Unconfigured weapon type gets no implicit rifle ammo"), Items->TryResolveEnemyLoadout(1002, INDEX_NONE, INDEX_NONE, Loadout));
	for (const FString& Invalid : {FString(TEXT("{}")), CustomDefault.Replace(TEXT("2001"), TEXT("2002")),
		CustomDefault.Replace(TEXT(":3,"), TEXT(":-1,")), CustomDefault.Replace(TEXT(":3,"), TEXT(":2147483647,"))})
	{
		TestFalse(TEXT("Invalid default loadout rejects replacement"), Items->ParseEnemyDefaultLoadout(Invalid));
		TestTrue(TEXT("Previous default survives"), Items->TryResolveEnemyLoadout(INDEX_NONE, INDEX_NONE, INDEX_NONE, Loadout));
		TestEqual(TEXT("Previous default still SMG"), Loadout.WeaponItemId, 1006);
	}
	const FString CustomLab = TEXT(R"({"weapon_item_id":1006,"ammo_item_id":2001,"reserve_ammo_count":7})");
	TestTrue(TEXT("Independent lab loadout parses"), Items->ParseCombatLabEnemyLoadout(CustomLab));
	TestTrue(TEXT("Lab resolves separately"), Items->TryGetCombatLabEnemyLoadout(Loadout));
	TestEqual(TEXT("Lab uses configured SMG"), Loadout.WeaponItemId, 1006);
	TestEqual(TEXT("Lab uses configured reserve"), Loadout.ReserveAmmoCount, 7);
	for (const FString& Invalid : {FString(TEXT("{}")), CustomLab.Replace(TEXT("2001"), TEXT("2002")),
		CustomLab.Replace(TEXT(":7}"), TEXT(":7.5}")), CustomLab.Replace(TEXT(":7}"), TEXT(":-1}"))})
	{
		TestFalse(TEXT("Invalid lab loadout is rejected"), Items->ParseCombatLabEnemyLoadout(Invalid));
		TestTrue(TEXT("Previous lab loadout survives"), Items->TryGetCombatLabEnemyLoadout(Loadout));
		TestEqual(TEXT("Previous lab reserve survives"), Loadout.ReserveAmmoCount, 7);
	}
	return true;
}

#endif
