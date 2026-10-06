#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Game/TunaSweeperSafeSave.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperSaveToolTest, "TunaSweeper.SaveTool.Transactions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperSaveToolTest::RunTest(const FString& Parameters)
{
	const FString Path = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("Automation/SaveTool") / FGuid::NewGuid().ToString() / TEXT("TunaSweeperSave_Slot01.sav"));
	auto Save = NewObject<UTunaSweeperSaveGame>();
	Save->BuildFlavor = TEXT("Demo");
	Save->InventorySlots.SetNum(40);
	Save->EquipmentSlots.SetNum(8);
	Save->QuestCoinBalance = 12345;
	Save->TotalExperiencePoints = 6789;
	Save->CompletedScenarioFlags.Add(TEXT("test.untouched"));
	auto Validator = [](const USaveGame& Object) { return Object.IsA<UTunaSweeperSaveGame>(); };
	if (!TestTrue(TEXT("Synthetic fixture saved"), TunaSweeperSafeSave::SaveGameFileFailClosed(Save, Path, Validator))) return false;
	TArray<uint8> Original;
	FFileHelper::LoadFileToArray(Original, *Path);
	auto Request = [&](const TCHAR* Operation, const FString& Extra = FString())
	{
		TSharedPtr<FJsonObject> Json;
		FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(TEXT("{") + Extra + TEXT("}")), Json);
		Json->SetNumberField(TEXT("version"), 1);
		Json->SetStringField(TEXT("operation"), Operation);
		Json->SetStringField(TEXT("savePath"), Path);
		Json->SetStringField(TEXT("flavor"), TEXT("Demo"));
		Json->SetNumberField(TEXT("slot"), 1);
		return Json;
	};
	auto Run = [&](const TCHAR* Op, const FString& Extra = FString()) { return TunaSweeperSaveTool::Execute(Request(Op, Extra)); };
	auto Inspect = Run(TEXT("inspect"));
	if (!TestTrue(TEXT("Inspect succeeds"), Inspect->GetBoolField(TEXT("ok")))) return false;
	TestTrue(TEXT("Snapshot exposed"), Inspect->HasField(TEXT("save")));
	auto Preview = Run(TEXT("add"), TEXT("\"items\":[{\"itemId\":2002,\"quantity\":250}]"));
	TestTrue(TEXT("Preview succeeds"), Preview->GetBoolField(TEXT("ok")));
	TArray<uint8> After;
	FFileHelper::LoadFileToArray(After, *Path);
	TestTrue(TEXT("Preview preserves exact bytes"), Original == After);
	auto Added = Run(TEXT("add"), TEXT("\"commit\":true,\"items\":[{\"itemId\":2002,\"quantity\":250},{\"itemId\":1002,\"quantity\":1,\"loadedAmmoItemId\":2002,\"loadedAmmoCount\":30}]"));
	if (!TestTrue(TEXT("Commit succeeds"), Added->GetBoolField(TEXT("ok")))) return false;
	TestTrue(TEXT("Unique backup exists"), FPaths::FileExists(Added->GetStringField(TEXT("backupPath"))));
	auto Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
	if (!TestNotNull(TEXT("Native reload succeeds"), Reload)) return false;
	TestEqual(TEXT("Ammo split plus rifle"), Reload->ItemInstances.Num(), 4);
	TestEqual(TEXT("First ammo stack"), Reload->ItemInstances[0].Quantity, 120);
	TestEqual(TEXT("Final ammo stack"), Reload->ItemInstances[2].Quantity, 10);
	TestEqual(TEXT("Loaded rounds"), Reload->ItemInstances[3].LoadedAmmoCount, 30);
	TestEqual(TEXT("Currency unchanged"), Reload->QuestCoinBalance, 12345);
	TestEqual(TEXT("Experience unchanged"), Reload->TotalExperiencePoints, int64(6789));
	TestTrue(TEXT("Save timestamp follows existing local-time contract"), FMath::Abs((FDateTime::Now() - FDateTime(Reload->LastSavedAtTicks)).GetTotalSeconds()) < 10);
	TestTrue(TEXT("Acquisition history updated"), Reload->EverAcquiredItemIds.Contains(1002));
	FFileHelper::LoadFileToArray(Original, *Path);
	for (const FString& Invalid : {
		FString(TEXT("\"commit\":true,\"items\":[{\"itemId\":2002,\"quantity\":1},{\"itemId\":999999,\"quantity\":1}]")),
		FString(TEXT("\"commit\":true,\"items\":[{\"itemId\":1002,\"quantity\":1000}]")),
		FString(TEXT("\"commit\":true,\"items\":[{\"itemId\":1002,\"quantity\":1,\"loadedAmmoItemId\":2001,\"loadedAmmoCount\":1}]")),
		FString(TEXT("\"commit\":true,\"items\":[{\"itemId\":2002,\"quantity\":-1}]")),
		FString(TEXT("\"commit\":\"true\",\"items\":[{\"itemId\":2002,\"quantity\":1}]")),
		FString(TEXT("\"commit\":true,\"expectedHash\":\"stale\",\"items\":[{\"itemId\":2002,\"quantity\":1}]")) })
	{
		TestFalse(TEXT("Invalid mutation rejected"), Run(TEXT("add"), Invalid)->GetBoolField(TEXT("ok")));
		FFileHelper::LoadFileToArray(After, *Path);
		TestTrue(TEXT("Failed transaction preserves bytes"), Original == After);
	}
	const FString Preset = TEXT("\"preset\":{\"version\":1,\"equipment\":[{\"slot\":0,\"item\":{\"itemId\":1002,\"quantity\":1,\"loadedAmmoItemId\":2002,\"loadedAmmoCount\":30}}]}");
	TestTrue(TEXT("Preset applied"), Run(TEXT("preset-apply"), TEXT("\"commit\":true,") + Preset)->GetBoolField(TEXT("ok")));
	Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
	TestTrue(TEXT("Weapon equipped"), Reload && Reload->EquipmentSlots[0].ItemUid.IsValid());
	auto Export = Run(TEXT("preset-export"));
	TestTrue(TEXT("Preset exported"), Export->GetBoolField(TEXT("ok")));
	TestTrue(TEXT("Preset object returned"), Export->HasField(TEXT("preset")));
	TestTrue(TEXT("Second preset applied"), Run(TEXT("preset-apply"), TEXT("\"commit\":true,") + Preset)->GetBoolField(TEXT("ok")));
	Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
	TestEqual(TEXT("Previous equipped rifle preserved"), Reload->ItemInstances.Num(), 6);
	TestTrue(TEXT("Original scenario preserved"), Reload->CompletedScenarioFlags.Contains(TEXT("test.untouched")));
	// A quick slot owns an item, rather than aliasing an inventory slot.
	FTunaSweeperItemInstance Med;
	Med.Uid = FGuid::NewGuid(); Med.ItemId = 3001;
	Reload->ItemInstances.Add(Med);
	Reload->UsableQuickSlots.SetNum(6);
	Reload->UsableQuickSlots[0].ItemUid = Med.Uid;
	TestTrue(TEXT("Quick-slot fixture saved"), TunaSweeperSafeSave::SaveGameFileFailClosed(Reload, Path, Validator));
	TestTrue(TEXT("Quick-slot ownership accepted"), Run(TEXT("validate"))->GetBoolField(TEXT("ok")));
	const FString AttachedPreset = TEXT("\"commit\":true,\"preset\":{\"version\":1,\"equipment\":[{\"slot\":0,\"item\":{\"itemId\":1002,\"quantity\":1,\"loadedAmmoItemId\":2002,\"loadedAmmoCount\":45,\"attachments\":[{\"slotTag\":\"attachment.slot.magazine\",\"itemId\":2004}]}},{\"slot\":7,\"item\":{\"itemId\":5002}}]}");
	TestTrue(TEXT("Magazine attachment and backpack applied"), Run(TEXT("preset-apply"), AttachedPreset)->GetBoolField(TEXT("ok")));
	Reload = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
	TestEqual(TEXT("Backpack expands capacity"), Reload->InventorySlots.Num(), 50);
	TestEqual(TEXT("Quick slot preserved"), Reload->UsableQuickSlots[0].ItemUid, Med.Uid);
	auto ExportedPreset = Run(TEXT("preset-export"));
	const auto& Rows = ExportedPreset->GetObjectField(TEXT("preset"))->GetArrayField(TEXT("equipment"));
	TestEqual(TEXT("Export keeps attachments"), Rows[0]->AsObject()->GetObjectField(TEXT("item"))->GetArrayField(TEXT("attachments")).Num(), 1);
	FFileHelper::LoadFileToArray(Original, *Path);
	TestFalse(TEXT("Wrong equipment category rejected"), Run(TEXT("preset-apply"), TEXT("\"commit\":true,\"preset\":{\"version\":1,\"equipment\":[{\"slot\":3,\"item\":{\"itemId\":1002}}]}"))->GetBoolField(TEXT("ok")));
	TestFalse(TEXT("Duplicate preset slot rejected"), Run(TEXT("preset-apply"), TEXT("\"commit\":true,\"preset\":{\"version\":1,\"equipment\":[{\"slot\":0,\"item\":{\"itemId\":1002}},{\"slot\":0,\"item\":{\"itemId\":1002}}]}"))->GetBoolField(TEXT("ok")));
	FFileHelper::LoadFileToArray(After, *Path);
	TestTrue(TEXT("Invalid presets preserve bytes"), Original == After);
	// Fill all inventory slots, then attempt to remove the capacity-providing backpack.
	for (auto& InventorySlot : Reload->InventorySlots)
	{
		if (!InventorySlot.IsEmpty()) continue;
		FTunaSweeperItemInstance Rifle;
		Rifle.Uid = FGuid::NewGuid(); Rifle.ItemId = 1002;
		Reload->ItemInstances.Add(Rifle); InventorySlot.ItemUid = Rifle.Uid;
	}
	TestTrue(TEXT("Full backpack fixture saved"), TunaSweeperSafeSave::SaveGameFileFailClosed(Reload, Path, Validator));
	FFileHelper::LoadFileToArray(Original, *Path);
	TestFalse(TEXT("Smaller capacity preset fails atomically"), Run(TEXT("preset-apply"), TEXT("\"commit\":true,\"preset\":{\"version\":1,\"equipment\":[]}"))->GetBoolField(TEXT("ok")));
	FFileHelper::LoadFileToArray(After, *Path);
	TestTrue(TEXT("Capacity failure preserves all original bytes"), Original == After);
	auto WrongFlavor = Request(TEXT("inspect"));
	WrongFlavor->SetStringField(TEXT("flavor"), TEXT("Main"));
	TestFalse(TEXT("Flavor mismatch rejected"), TunaSweeperSaveTool::Execute(WrongFlavor)->GetBoolField(TEXT("ok")));
	FFileHelper::LoadFileToArray(Original, *Path);
	Original.Last() ^= 1;
	FFileHelper::SaveArrayToFile(Original, *Path);
	TestFalse(TEXT("CRC corruption rejected without recovery"), Run(TEXT("inspect"))->GetBoolField(TEXT("ok")));
	FFileHelper::LoadFileToArray(After, *Path);
	TestTrue(TEXT("Corrupt file untouched"), Original == After);
	return true;
}
#endif
