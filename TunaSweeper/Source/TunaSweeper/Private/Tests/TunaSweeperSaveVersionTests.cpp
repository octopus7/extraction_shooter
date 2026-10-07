#if WITH_DEV_AUTOMATION_TESTS

#include "Game/TunaSweeperGameInstanceShared.h"
#include "Game/TunaSweeperSafeSave.h"
#include "Misc/AutomationTest.h"

namespace TunaSweeperSaveVersionTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext |
		EAutomationTestFlags::ClientContext |
		EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperRetiredAmmoMigrationTest,
	"TunaSweeper.Save.RetiredUtilityAmmoMigration",
	TunaSweeperSaveVersionTests::TestFlags)

bool FTunaSweeperRetiredAmmoMigrationTest::RunTest(const FString& Parameters)
{
	FTunaSweeperItemInstance Stack;
	Stack.Uid = FGuid::NewGuid();
	Stack.ItemId = 2023;
	Stack.Quantity = 73;
	const FTunaSweeperItemInstance SavedStack = TunaSweeperInventory::MakeItemInstanceForSave(Stack);
	TestEqual(TEXT("Retired ammunition is saved as standard rifle ammunition"), SavedStack.ItemId, 2002);
	TestEqual(TEXT("Migration preserves stack quantity"), SavedStack.Quantity, 73);
	TestEqual(TEXT("Migration preserves slot identity"), SavedStack.Uid, Stack.Uid);
	TestEqual(TEXT("Serializing a migrated copy does not mutate the source"), Stack.ItemId, 2023);
	TunaSweeperInventory::NormalizeLoadedAmmoPersistenceFields(Stack);
	TestEqual(TEXT("Inventory and storage load converts retired ammunition"), Stack.ItemId, 2002);
	FTunaSweeperItemInstance Weapon;
	Weapon.Uid = FGuid::NewGuid();
	Weapon.ItemId = 1002;
	Weapon.LoadedAmmoItemId = 2023;
	Weapon.SelectedAmmoItemId = 2023;
	Weapon.LoadedAmmoCount = 17;
	const FGuid Attachment = FGuid::NewGuid();
	Weapon.AttachmentSlots.Add(TEXT("attachment.slot.tactical"), Attachment);
	TunaSweeperInventory::NormalizeLoadedAmmoPersistenceFields(Weapon);
	TestEqual(TEXT("Loaded rounds become ordinary rifle rounds"), Weapon.LoadedAmmoItemId, 2002);
	TestEqual(TEXT("Selected ammo follows the converted loaded rounds"), Weapon.SelectedAmmoItemId, 2002);
	TestEqual(TEXT("Loaded round count is retained"), Weapon.LoadedAmmoCount, 17);
	TestEqual(TEXT("Weapon itself is retained"), Weapon.ItemId, 1002);
	TestEqual(TEXT("Attachments are retained"), Weapon.AttachmentSlots.FindChecked(TEXT("attachment.slot.tactical")), Attachment);
	Weapon.LoadedAmmoItemId = INDEX_NONE;
	Weapon.SelectedAmmoItemId = 2023;
	TunaSweeperInventory::NormalizeLoadedAmmoPersistenceFields(Weapon);
	TestEqual(TEXT("Old selected-only ammo state migrates before normalization"), Weapon.LoadedAmmoItemId, 2002);
	TunaSweeperInventory::NormalizeLoadedAmmoPersistenceFields(Weapon);
	TestEqual(TEXT("Migration is idempotent"), Weapon.LoadedAmmoItemId, 2002);
	FTunaSweeperShopDefinition Shop;
	FTunaSweeperShopItemDefinition RemainingOffer;
	RemainingOffer.ItemId = 7002;
	Shop.Items.Add(RemainingOffer);
	FTunaSweeperShopStockSaveData Stock;
	Stock.ItemId = 7002;
	Stock.SlotIndex = 1;
	Stock.StockQuantity = 3;
	TestEqual(TEXT("Removing an offer preserves subsequent item stock under its new slot"),
		TunaSweeperShop::ResolveSavedStockSlot(Stock, Shop), 0);
	TestEqual(TEXT("Stock quantity is untouched"), Stock.StockQuantity, 3);
	Stock.ItemId = 2023;
	TestEqual(TEXT("Retired shop stock has no replacement offer"), TunaSweeperShop::ResolveSavedStockSlot(Stock, Shop), INDEX_NONE);
	Stock.ItemId = 7002;
	Stock.SlotIndex = 5;
	Shop.Items.Add(RemainingOffer);
	TestEqual(TEXT("Ambiguous duplicate offers do not transfer stock"), TunaSweeperShop::ResolveSavedStockSlot(Stock, Shop), INDEX_NONE);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperSaveVersionPolicyTest,
	"TunaSweeper.Save.VersionPolicy",
	TunaSweeperSaveVersionTests::TestFlags)

bool FTunaSweeperSaveVersionPolicyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	TestEqual(TEXT("Current save version"), TunaSweeperSave::CurrentSaveVersion, 22);
	TestEqual(TEXT("Minimum supported save version"), TunaSweeperSave::MinimumSupportedSaveVersion, 20);
	TestTrue(TEXT("Version 19 is outdated"), TunaSweeperSave::IsOutdatedSaveVersion(19));
	TestFalse(TEXT("Version 20 is supported"), TunaSweeperSave::IsOutdatedSaveVersion(20));
	TestFalse(TEXT("Version 21 remains supported"), TunaSweeperSave::IsOutdatedSaveVersion(21));
	TestFalse(TEXT("Current version is supported"), TunaSweeperSave::IsOutdatedSaveVersion(22));
	TestFalse(TEXT("Future versions are not auto-deleted"), TunaSweeperSave::IsOutdatedSaveVersion(23));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperOutdatedSaveCleanupTest,
	"TunaSweeper.Save.OutdatedCleanup",
	TunaSweeperSaveVersionTests::TestFlags)

bool FTunaSweeperOutdatedSaveCleanupTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FString TestDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation"),
		FString::Printf(TEXT("SaveVersionCleanup_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	IFileManager::Get().MakeDirectory(*TestDirectory, true);

	auto WriteSaveFile = [this, &TestDirectory](const TCHAR* FileName, int32 SaveVersion)
	{
		UTunaSweeperSaveGame* SaveGame = NewObject<UTunaSweeperSaveGame>();
		SaveGame->SaveVersion = SaveVersion;
		TArray<uint8> SaveData;
		const bool bSerialized = UGameplayStatics::SaveGameToMemory(SaveGame, SaveData);
		TestTrue(FString::Printf(TEXT("Serialize %s"), FileName), bSerialized);
		const FString FilePath = FPaths::Combine(TestDirectory, FileName);
		TestTrue(
			FString::Printf(TEXT("Write %s"), FileName),
			bSerialized && FFileHelper::SaveArrayToFile(SaveData, *FilePath));
		return FilePath;
	};

	const FString OldSavePath = WriteSaveFile(TEXT("OldSave.sav"), 19);
	const FString CurrentSavePath = WriteSaveFile(TEXT("CurrentSave.sav"), 20);
	const FString FutureSavePath = WriteSaveFile(TEXT("FutureSave.sav"), 21);

	TestTrue(
		TEXT("Version 19 cleanup result"),
		TunaSweeperSave::DeleteOutdatedSaveFileIfNeeded(OldSavePath, TestDirectory) ==
			TunaSweeperSave::EOutdatedSaveCleanupResult::Deleted);
	TestFalse(TEXT("Version 19 file was deleted"), FPaths::FileExists(OldSavePath));

	TestTrue(
		TEXT("Version 20 cleanup result"),
		TunaSweeperSave::DeleteOutdatedSaveFileIfNeeded(CurrentSavePath, TestDirectory) ==
			TunaSweeperSave::EOutdatedSaveCleanupResult::NotOutdated);
	TestTrue(TEXT("Version 20 file remains"), FPaths::FileExists(CurrentSavePath));

	TestTrue(
		TEXT("Future version cleanup result"),
		TunaSweeperSave::DeleteOutdatedSaveFileIfNeeded(FutureSavePath, TestDirectory) ==
			TunaSweeperSave::EOutdatedSaveCleanupResult::NotOutdated);
	TestTrue(TEXT("Future version file remains"), FPaths::FileExists(FutureSavePath));

	FString LogText;
	const FString DeletionLogPath = FPaths::Combine(TestDirectory, TunaSweeperSave::AutoDeletedSaveLogFileName);
	TestTrue(TEXT("Deletion log exists"), FFileHelper::LoadFileToString(LogText, *DeletionLogPath));
	TestTrue(TEXT("Deletion log records version"), LogText.Contains(TEXT("version=19")));
	TestTrue(TEXT("Deletion log records file"), LogText.Contains(TEXT("OldSave.sav")));

	IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperFailClosedSaveTest,
	"TunaSweeper.Save.FailClosedCandidatePromotion",
	TunaSweeperSaveVersionTests::TestFlags)

bool FTunaSweeperFailClosedSaveTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FString TestDirectory = FPaths::Combine(
		FPaths::ProjectSavedDir(),
		TEXT("Automation"),
		FString::Printf(TEXT("FailClosedSave_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
	IFileManager::Get().MakeDirectory(*TestDirectory, true);
	const FString PrimaryFilePath = FPaths::Combine(TestDirectory, TEXT("Slot01.sav"));
	const FString PreviousFilePath = TunaSweeperSafeSave::GetPreviousFilePath(PrimaryFilePath);
	const FString CandidateFilePath = TunaSweeperSafeSave::GetCandidateFilePath(PrimaryFilePath);
	const TunaSweeperSafeSave::FSaveValidator Validator = [](const USaveGame& SaveGame)
	{
		return SaveGame.IsA<UTunaSweeperSaveGame>();
	};

	auto MakeSave = [](int64 SavedAtTicks)
	{
		UTunaSweeperSaveGame* SaveGame = NewObject<UTunaSweeperSaveGame>();
		SaveGame->SaveVersion = TunaSweeperSave::CurrentSaveVersion;
		SaveGame->LastSavedAtTicks = SavedAtTicks;
		return SaveGame;
	};

	TestTrue(
		TEXT("Initial candidate is verified and promoted"),
		TunaSweeperSafeSave::SaveGameFileFailClosed(MakeSave(100), PrimaryFilePath, Validator));
	TestFalse(TEXT("Candidate is removed after promotion"), FPaths::FileExists(CandidateFilePath));

	TestTrue(
		TEXT("Second generation is promoted"),
		TunaSweeperSafeSave::SaveGameFileFailClosed(MakeSave(200), PrimaryFilePath, Validator));
	const UTunaSweeperSaveGame* ActiveSave = Cast<UTunaSweeperSaveGame>(
		TunaSweeperSafeSave::LoadVerifiedSaveFile(PrimaryFilePath, Validator));
	const UTunaSweeperSaveGame* PreviousSave = Cast<UTunaSweeperSaveGame>(
		TunaSweeperSafeSave::LoadVerifiedSaveFile(PreviousFilePath, Validator));
	TestNotNull(TEXT("Active generation remains readable"), ActiveSave);
	TestNotNull(TEXT("Previous verified generation exists"), PreviousSave);
	if (ActiveSave)
	{
		TestEqual(TEXT("Active generation value"), ActiveSave->LastSavedAtTicks, int64(200));
	}
	if (PreviousSave)
	{
		TestEqual(TEXT("Previous generation value"), PreviousSave->LastSavedAtTicks, int64(100));
	}

	TArray<uint8> CorruptedBytes;
	TestTrue(TEXT("Read active bytes before corruption"), FFileHelper::LoadFileToArray(CorruptedBytes, *PrimaryFilePath));
	if (!CorruptedBytes.IsEmpty())
	{
		CorruptedBytes.Last() ^= 0xFF;
	}
	TestTrue(TEXT("Write simulated interrupted/corrupt active file"), FFileHelper::SaveArrayToFile(CorruptedBytes, *PrimaryFilePath));
	TestNull(
		TEXT("CRC rejects corrupt active file"),
		TunaSweeperSafeSave::LoadVerifiedSaveFile(PrimaryFilePath, Validator));

	TArray<uint8> CorruptedSnapshot;
	FFileHelper::LoadFileToArray(CorruptedSnapshot, *PrimaryFilePath);
	TestFalse(
		TEXT("Saving refuses to overwrite a corrupt active generation"),
		TunaSweeperSafeSave::SaveGameFileFailClosed(MakeSave(300), PrimaryFilePath, Validator));
	TArray<uint8> AfterRefusedSave;
	FFileHelper::LoadFileToArray(AfterRefusedSave, *PrimaryFilePath);
	TestTrue(TEXT("Refused save leaves corrupt active bytes untouched"), CorruptedSnapshot == AfterRefusedSave);
	TestFalse(TEXT("Refused save removes its candidate"), FPaths::FileExists(CandidateFilePath));

	FString UsedRecoveryFilePath;
	const UTunaSweeperSaveGame* RecoveredSave = Cast<UTunaSweeperSaveGame>(
		TunaSweeperSafeSave::LoadSaveFileWithRecovery(
			PrimaryFilePath,
			{ PreviousFilePath },
			Validator,
			&UsedRecoveryFilePath));
	TestNotNull(TEXT("Previous generation recovers corrupt active file"), RecoveredSave);
	TestEqual(TEXT("Recovery source is previous generation"), UsedRecoveryFilePath, PreviousFilePath);
	if (RecoveredSave)
	{
		TestEqual(TEXT("Recovered generation value"), RecoveredSave->LastSavedAtTicks, int64(100));
	}
	TestNotNull(
		TEXT("Recovered active file validates after repair"),
		TunaSweeperSafeSave::LoadVerifiedSaveFile(PrimaryFilePath, Validator));

	UTunaSweeperSaveGame* UncommittedCandidateSave = MakeSave(999);
	TArray<uint8> UncommittedCandidateBytes;
	UGameplayStatics::SaveGameToMemory(UncommittedCandidateSave, UncommittedCandidateBytes);
	FFileHelper::SaveArrayToFile(UncommittedCandidateBytes, *CandidateFilePath);
	const UTunaSweeperSaveGame* SaveAfterCandidateCleanup = Cast<UTunaSweeperSaveGame>(
		TunaSweeperSafeSave::LoadSaveFileWithRecovery(
			PrimaryFilePath,
			{ PreviousFilePath },
			Validator));
	TestNotNull(TEXT("Committed active file loads with stale candidate present"), SaveAfterCandidateCleanup);
	if (SaveAfterCandidateCleanup)
	{
		TestEqual(
			TEXT("Stale candidate is never promoted"),
			SaveAfterCandidateCleanup->LastSavedAtTicks,
			int64(100));
	}
	TestFalse(TEXT("Stale candidate is deleted on load"), FPaths::FileExists(CandidateFilePath));

	IFileManager::Get().DeleteDirectory(*TestDirectory, false, true);
	return true;
}

#endif
