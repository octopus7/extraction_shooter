#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputMappingContext.h"
#include "Misc/AutomationTest.h"
#include "Player/TunaSweeperPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperPauseMenuKeyTest,
	"TunaSweeper.UI.PauseMenu.Keys",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperPauseMenuKeyTest::RunTest(const FString& Parameters)
{
	const auto* Mapping = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Player.IMC_Player"));
	if (!TestNotNull(TEXT("Actual player input mapping loads"), Mapping)) return false;
	for (const auto& Entry : Mapping->GetMappings())
	{
		TestNotEqual(TEXT("K is unassigned in the shipped player mapping"), Entry.Key, EKeys::K);
	}
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestTrue(TEXT("ESC opens pause in standalone gameplay"), ATunaSweeperPlayerController::IsPauseMenuKey(EKeys::Escape, World));
	TestFalse(TEXT("K is not added to packaged gameplay"), ATunaSweeperPlayerController::IsPauseMenuKey(EKeys::K, World));
	World->WorldType = EWorldType::PIE;
	TestTrue(TEXT("PIE gets the unused K shortcut"), ATunaSweeperPlayerController::IsPauseMenuKey(EKeys::K, World));
	TestFalse(TEXT("Other gameplay keys remain unaffected"), ATunaSweeperPlayerController::IsPauseMenuKey(EKeys::J, World));
	World->WorldType = EWorldType::Game;
	World->DestroyWorld(false);
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperPauseExitSaveFailureTest,
	"TunaSweeper.UI.PauseMenu.SaveFailurePreservesRaid",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperPauseExitSaveFailureTest::RunTest(const FString& Parameters)
{
	auto* Instance = NewObject<UTunaSweeperGameInstance>();
	Instance->bInventoryStateInitialized = true;
	// The existing retired-slot guard refuses saving before any disk I/O.
	Instance->RetiredDemoSaveSlotIndex = Instance->ActiveSaveSlotIndex;
	FTunaSweeperItemInstance Item;
	Item.Uid = FGuid::NewGuid(); Item.ItemId = 1002; Item.Quantity = 1;
	Instance->ItemInstancesByUid.Add(Item.Uid, Item);
	Instance->EquipmentSlots.SetNum(8);
	Instance->EquipmentSlots[0].ItemUid = Item.Uid;
	Instance->UsableQuickSlots.SetNum(8);
	Instance->UsableQuickSlots[0].ItemUid = Item.Uid;
	Instance->PendingRaidExperiencePoints = 27;
	Instance->bRaidExperienceSessionActive = true;
	TestFalse(TEXT("Failed exit save cannot authorize travel or quitting"), Instance->SaveForGameplayExit(true));
	TestTrue(TEXT("Failed save preserves carried item instances"), Instance->ItemInstancesByUid.Contains(Item.Uid));
	TestEqual(TEXT("Failed save preserves equipment"), Instance->EquipmentSlots[0].ItemUid, Item.Uid);
	TestEqual(TEXT("Failed save preserves quick slots"), Instance->UsableQuickSlots[0].ItemUid, Item.Uid);
	TestEqual(TEXT("Failed save preserves pending raid experience"), Instance->PendingRaidExperiencePoints, int64(27));
	TestTrue(TEXT("Failed save leaves the raid active"), Instance->bRaidExperienceSessionActive);
	TestFalse(TEXT("Bunker exit also requires a successful save without pending item changes"), Instance->SaveForGameplayExit(false));
	TestTrue(TEXT("Failed bunker exit preserves carried items"), Instance->ItemInstancesByUid.Contains(Item.Uid));
	TestEqual(TEXT("Bunker exit preserves equipment"), Instance->EquipmentSlots[0].ItemUid, Item.Uid);
	TestEqual(TEXT("Bunker exit preserves quick slots"), Instance->UsableQuickSlots[0].ItemUid, Item.Uid);
	return true;
}
#endif
