#include "TunaSweeperGameInstanceShared.h"

#include "Game/TunaSweeperSafeSave.h"
#include "Settings/TunaSweeperBuildFlavor.h"

namespace TunaSweeperSave
{
	namespace
	{
		FString GetFlavorSaveFilePath(const FString& SaveSlotName)
		{
			return FPaths::Combine(
				TunaSweeperBuildFlavor::GetSaveGameDirectory(),
				SaveSlotName + TEXT(".sav"));
		}

		bool TryParseGameplaySaveSlotIndex(const FString& SaveSlotName, int32& OutSaveSlotIndex)
		{
			const FString Prefix(SaveSlotNamePrefix);
			if (!SaveSlotName.StartsWith(Prefix))
			{
				return false;
			}

			const FString SlotIndexText = SaveSlotName.RightChop(Prefix.Len());
			if (SlotIndexText.IsEmpty() || !SlotIndexText.IsNumeric())
			{
				return false;
			}

			OutSaveSlotIndex = FCString::Atoi(*SlotIndexText);
			return OutSaveSlotIndex >= MinSaveSlotIndex && OutSaveSlotIndex <= MaxSaveSlotIndex;
		}

		TunaSweeperSafeSave::FSaveValidator MakeFlavorSaveValidator(const FString& SaveSlotName)
		{
			int32 ExpectedSaveSlotIndex = INDEX_NONE;
			const bool bGameplaySave = TryParseGameplaySaveSlotIndex(SaveSlotName, ExpectedSaveSlotIndex);
			return [SaveSlotName, bGameplaySave, ExpectedSaveSlotIndex](const USaveGame& SaveGame)
			{
				if (!bGameplaySave)
				{
					return SaveSlotName == SaveSettingsSlotName &&
						SaveGame.IsA<UTunaSweeperSaveSettings>();
				}

				const UTunaSweeperSaveGame* TunaSaveGame = Cast<UTunaSweeperSaveGame>(&SaveGame);
				return TunaSaveGame &&
					TunaSaveGame->SaveSlotIndex == ExpectedSaveSlotIndex &&
					!IsOutdatedSaveVersion(TunaSaveGame->SaveVersion) &&
					!TunaSaveGame->BuildFlavor.IsNone() &&
					TunaSaveGame->BuildFlavor == TunaSweeperBuildFlavor::GetName();
			};
		}

		void GetBackupSaveFilePaths(const FString& SaveSlotName, TArray<FString>& OutBackupFilePaths)
		{
			OutBackupFilePaths.Reset();
			int32 SaveSlotIndex = INDEX_NONE;
			if (!TryParseGameplaySaveSlotIndex(SaveSlotName, SaveSlotIndex))
			{
				return;
			}

			const FString BackupDirectory = FPaths::Combine(
				TunaSweeperBuildFlavor::GetSaveGameDirectory(),
				TEXT("Backups"));
			TArray<FString> BackupFileNames;
			IFileManager::Get().FindFiles(
				BackupFileNames,
				*FPaths::Combine(
					BackupDirectory,
					FString::Printf(TEXT("SaveSlot%02d_*.sav"), SaveSlotIndex)),
				true,
				false);

			for (const FString& BackupFileName : BackupFileNames)
			{
				OutBackupFilePaths.Add(FPaths::Combine(BackupDirectory, BackupFileName));
			}
			OutBackupFilePaths.Sort([](const FString& Left, const FString& Right)
			{
				const FDateTime LeftTime = IFileManager::Get().GetTimeStamp(*Left);
				const FDateTime RightTime = IFileManager::Get().GetTimeStamp(*Right);
				return LeftTime == RightTime ? Right < Left : LeftTime > RightTime;
			});
		}

		bool DoesFlavorSaveExist(const FString& SaveSlotName)
		{
			const FString PrimaryFilePath = GetFlavorSaveFilePath(SaveSlotName);
			if (FPaths::FileExists(PrimaryFilePath) ||
				FPaths::FileExists(TunaSweeperSafeSave::GetPreviousFilePath(PrimaryFilePath)))
			{
				return true;
			}

			TArray<FString> BackupFilePaths;
			GetBackupSaveFilePaths(SaveSlotName, BackupFilePaths);
			return !BackupFilePaths.IsEmpty();
		}

		USaveGame* LoadFlavorSave(const FString& SaveSlotName)
		{
			const FString PrimaryFilePath = GetFlavorSaveFilePath(SaveSlotName);
			TArray<FString> RecoveryFilePaths = {
				TunaSweeperSafeSave::GetPreviousFilePath(PrimaryFilePath)
			};
			TArray<FString> BackupFilePaths;
			GetBackupSaveFilePaths(SaveSlotName, BackupFilePaths);
			RecoveryFilePaths.Append(BackupFilePaths);

			FString RecoveryFilePath;
			USaveGame* SaveGame = TunaSweeperSafeSave::LoadSaveFileWithRecovery(
				PrimaryFilePath,
				RecoveryFilePaths,
				MakeFlavorSaveValidator(SaveSlotName),
				&RecoveryFilePath);
			if (SaveGame && !RecoveryFilePath.IsEmpty())
			{
				UE_LOG(
					LogTunaSweeperGameInstance,
					Warning,
					TEXT("Recovered save '%s' from last verified generation: %s"),
					*SaveSlotName,
					*RecoveryFilePath);
			}
			return SaveGame;
		}

		bool SaveFlavorSave(USaveGame* SaveGame, const FString& SaveSlotName)
		{
			if (!SaveGame ||
				!IFileManager::Get().MakeDirectory(*TunaSweeperBuildFlavor::GetSaveGameDirectory(), true))
			{
				return false;
			}

			return TunaSweeperSafeSave::SaveGameFileFailClosed(
				SaveGame,
				GetFlavorSaveFilePath(SaveSlotName),
				MakeFlavorSaveValidator(SaveSlotName));
		}

		bool DeleteFlavorSave(const FString& SaveSlotName)
		{
			bool bDeletedAll = TunaSweeperSafeSave::DeleteSaveArtifacts(
				GetFlavorSaveFilePath(SaveSlotName));
			TArray<FString> BackupFilePaths;
			GetBackupSaveFilePaths(SaveSlotName, BackupFilePaths);
			for (const FString& BackupFilePath : BackupFilePaths)
			{
				bDeletedAll = TunaSweeperSafeSave::DeleteSaveArtifacts(BackupFilePath) && bDeletedAll;
			}
			return bDeletedAll;
		}

		bool TryReadSaveVersion(const FString& SaveFilePath, int32& OutSaveVersion)
		{
			const UTunaSweeperSaveGame* SaveGame = Cast<UTunaSweeperSaveGame>(
				TunaSweeperSafeSave::LoadVerifiedSaveFile(
					SaveFilePath,
					[](const USaveGame& CandidateSaveGame)
					{
						return CandidateSaveGame.IsA<UTunaSweeperSaveGame>();
					}));
			if (!SaveGame)
			{
				return false;
			}

			OutSaveVersion = SaveGame->SaveVersion;
			return true;
		}

		FString GetAutoDeletedSaveLogPath(const FString& SaveGameDirectory)
		{
			return FPaths::Combine(SaveGameDirectory, AutoDeletedSaveLogFileName);
		}

		bool AppendAutoDeletedSaveLog(const FString& SaveGameDirectory, const FString& LogLine)
		{
			if (!IFileManager::Get().MakeDirectory(*SaveGameDirectory, true))
			{
				return false;
			}

			return FFileHelper::SaveStringToFile(
				LogLine,
				*GetAutoDeletedSaveLogPath(SaveGameDirectory),
				FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
				&IFileManager::Get(),
				FILEWRITE_Append);
		}

		FString MakeLoggedSavePath(const FString& SaveFilePath, const FString& SaveGameDirectory)
		{
			FString RelativePath = SaveFilePath;
			FString BaseDirectory = FPaths::ConvertRelativePathToFull(SaveGameDirectory);
			FPaths::NormalizeDirectoryName(BaseDirectory);
			BaseDirectory += TEXT("/");
			if (!FPaths::MakePathRelativeTo(RelativePath, *BaseDirectory))
			{
				RelativePath = FPaths::GetCleanFilename(SaveFilePath);
			}
			FPaths::NormalizeFilename(RelativePath);
			return RelativePath;
		}
	}

	EOutdatedSaveCleanupResult DeleteOutdatedSaveFileIfNeeded(
		const FString& SaveFilePath,
		const FString& SaveGameDirectory)
	{
		int32 SaveVersion = CurrentSaveVersion;
		if (!TryReadSaveVersion(SaveFilePath, SaveVersion) || !IsOutdatedSaveVersion(SaveVersion))
		{
			return EOutdatedSaveCleanupResult::NotOutdated;
		}

		if (!AppendAutoDeletedSaveLog(SaveGameDirectory, FString()))
		{
			UE_LOG(
				LogTunaSweeperGameInstance,
				Error,
				TEXT("Could not prepare outdated save deletion log: %s"),
				*GetAutoDeletedSaveLogPath(SaveGameDirectory));
			return EOutdatedSaveCleanupResult::DeleteFailed;
		}

		if (!IFileManager::Get().Delete(*SaveFilePath, false, true))
		{
			UE_LOG(
				LogTunaSweeperGameInstance,
				Error,
				TEXT("Could not delete outdated save version %d: %s"),
				SaveVersion,
				*SaveFilePath);
			return EOutdatedSaveCleanupResult::DeleteFailed;
		}

		const FString LoggedPath = MakeLoggedSavePath(SaveFilePath, SaveGameDirectory);
		const FString LogLine = FString::Printf(
			TEXT("[%s] Deleted outdated save: version=%d, file=%s%s"),
			*FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")),
			SaveVersion,
			*LoggedPath,
			LINE_TERMINATOR);
		if (!AppendAutoDeletedSaveLog(SaveGameDirectory, LogLine))
		{
			UE_LOG(
				LogTunaSweeperGameInstance,
				Error,
				TEXT("Deleted outdated save but could not append audit log: %s"),
				*SaveFilePath);
		}

		UE_LOG(
			LogTunaSweeperGameInstance,
			Display,
			TEXT("Deleted outdated save version %d: %s"),
			SaveVersion,
			*SaveFilePath);
		return EOutdatedSaveCleanupResult::Deleted;
	}

	void PurgeOutdatedSaveFiles(const FString& SaveGameDirectory)
	{
		if (!FPaths::DirectoryExists(SaveGameDirectory))
		{
			return;
		}

		TArray<FString> SaveFiles;
		IFileManager::Get().FindFilesRecursive(
			SaveFiles,
			*SaveGameDirectory,
			TEXT("*.sav"),
			true,
			false);
		SaveFiles.Sort();

		for (const FString& SaveFilePath : SaveFiles)
		{
			DeleteOutdatedSaveFileIfNeeded(SaveFilePath, SaveGameDirectory);
		}
	}

	void PurgeLegacyFlatSaveFiles()
	{
		const FString LegacyDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"));
		TArray<FString> LegacySaveFiles;
		IFileManager::Get().FindFiles(
			LegacySaveFiles,
			*FPaths::Combine(LegacyDirectory, TEXT("*.sav")),
			true,
			false);
		for (FString& LegacySaveFile : LegacySaveFiles)
		{
			LegacySaveFile = FPaths::Combine(LegacyDirectory, LegacySaveFile);
		}
		TArray<FString> LegacyBackupFiles;
		IFileManager::Get().FindFilesRecursive(
			LegacyBackupFiles,
			*FPaths::Combine(LegacyDirectory, TEXT("Backups")),
			TEXT("*.sav"),
			true,
			false);
		LegacySaveFiles.Append(LegacyBackupFiles);
		LegacySaveFiles.Sort();

		const FString AuditDirectory = TunaSweeperBuildFlavor::GetSaveGameDirectory();
		for (const FString& FilePath : LegacySaveFiles)
		{
			if (!IFileManager::Get().Delete(*FilePath, false, true))
			{
				UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Could not delete legacy flat save: %s"), *FilePath);
				continue;
			}

			AppendAutoDeletedSaveLog(
				AuditDirectory,
				FString::Printf(
					TEXT("[%s] Deleted legacy save: file=%s%s"),
					*FDateTime::Now().ToString(TEXT("%Y-%m-%d %H:%M:%S")),
					*MakeLoggedSavePath(FilePath, LegacyDirectory),
					LINE_TERMINATOR));
		}
	}
}

namespace TunaSweeperSaveFlavor
{
	bool IsCompatible(const UTunaSweeperSaveGame& SaveGame)
	{
		return !SaveGame.BuildFlavor.IsNone() &&
			SaveGame.BuildFlavor == TunaSweeperBuildFlavor::GetName();
	}
}

void UTunaSweeperGameInstance::ClearRuntimeState()
{
	ResetRuntimeStateForSaveSlotSelection();
	EnsureInventoryStateInitialized();
}

FTunaSweeperSaveSlotSummary UTunaSweeperGameInstance::GetSaveSlotSummary(int32 SaveSlotIndex) const
{
	FTunaSweeperSaveSlotSummary Summary;
	Summary.SaveSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);

	const FString ExistingSlotName = GetExistingSaveGameSlotName(Summary.SaveSlotIndex);
	if (ExistingSlotName.IsEmpty())
	{
		return Summary;
	}
	Summary.bHasData = true;
	Summary.bDifficultySelected = TunaSweeperBuildFlavor::IsDemo();

	UTunaSweeperSaveGame* SaveGame = Cast<UTunaSweeperSaveGame>(
		TunaSweeperSave::LoadFlavorSave(ExistingSlotName));
	if (!SaveGame)
	{
		return Summary;
	}
	if (!TunaSweeperSaveFlavor::IsCompatible(*SaveGame))
	{
		UE_LOG(
			LogTunaSweeperGameInstance,
			Warning,
			TEXT("Save slot %d does not match active build flavor '%s'."),
			Summary.SaveSlotIndex,
			*TunaSweeperBuildFlavor::GetName().ToString());
		return Summary;
	}

	Summary.TotalPlaySeconds = FMath::Max(0.0f, SaveGame->TotalPlaySeconds);
	Summary.DifficultyStage = TunaSweeperSave::SanitizeDifficultyStage(SaveGame->DifficultyStage);
	Summary.bDifficultySelected =
		TunaSweeperBuildFlavor::IsDemo() ||
		SaveGame->bDifficultySelected ||
		SaveGame->CompletedScenarioFlags.Contains(TunaSweeperScenario::OpeningScenarioFlag);
	Summary.LastSavedAtTicks = SaveGame->LastSavedAtTicks;
	return Summary;
}

bool UTunaSweeperGameInstance::ActivateSaveSlot(int32 SaveSlotIndex, bool bStartNewGame)
{
	if (!SetActiveSaveSlotIndex(SaveSlotIndex))
	{
		return false;
	}

	bInventoryStateInitializing = true;
	if (bStartNewGame)
	{
		LoadedSlotTotalPlaySeconds = 0.0f;
		ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
		GenerateDefaultInventoryState();
		if (TunaSweeperBuildFlavor::IsDemo())
		{
			ActiveSaveSlotDifficultyStage = 2;
			bActiveSaveSlotDifficultySelected = true;
			if (!InitializeDemoStartingLoadout())
			{
				UE_LOG(
					LogTunaSweeperGameInstance,
					Error,
					TEXT("Could not initialize the Demo starting loadout."));
				bInventoryStateInitializing = false;
				return false;
			}
		}
		CompleteInventoryStateInitialization();
		const bool bSaved = SaveGameStateInternal(EUsableQuickSlotSaveMode::Clear);
		if (bSaved)
		{
			bPendingBunkerItemStateSave = false;
		}
		return bSaved;
	}

	if (!LoadGameState())
	{
		bInventoryStateInitializing = false;
		return false;
	}

	CompleteInventoryStateInitialization();
	return true;
}

bool UTunaSweeperGameInstance::SetActiveSaveSlotDifficultyStage(int32 DifficultyStage, bool bSaveImmediately)
{
	EnsureInventoryStateInitialized();

	ActiveSaveSlotDifficultyStage = TunaSweeperSave::SanitizeDifficultyStage(DifficultyStage);
	bActiveSaveSlotDifficultySelected = true;

	return !bSaveImmediately || SaveGameStateInternal();
}

bool UTunaSweeperGameInstance::SetActiveSaveSlotIndex(int32 SaveSlotIndex)
{
	ActiveSaveSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);
	ResetRuntimeStateForSaveSlotSelection();
	return SaveActiveSaveSlotSelection();
}

bool UTunaSweeperGameInstance::DeleteSaveSlot(int32 SaveSlotIndex)
{
	const int32 SanitizedSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);
	const FString SlotName = GetSaveGameSlotName(SanitizedSlotIndex);
	const bool bDeleted = TunaSweeperSave::DoesFlavorSaveExist(SlotName) &&
		TunaSweeperSave::DeleteFlavorSave(SlotName);

	if (ActiveSaveSlotIndex == SanitizedSlotIndex)
	{
		ResetRuntimeStateForSaveSlotSelection();
		ActiveSaveSlotIndex = SanitizedSlotIndex;
	}

	return bDeleted;
}

bool UTunaSweeperGameInstance::DeleteSaveSlotAndStartNewGame(int32 SaveSlotIndex)
{
	const int32 SanitizedSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);
	const FString SlotName = GetSaveGameSlotName(SanitizedSlotIndex);
	if (TunaSweeperSave::DoesFlavorSaveExist(SlotName) &&
		!TunaSweeperSave::DeleteFlavorSave(SlotName))
	{
		return false;
	}

	ResetRuntimeStateForSaveSlotSelection();
	return ActivateSaveSlot(SanitizedSlotIndex, true);
}

void UTunaSweeperGameInstance::GeneratePlayerInventoryItems()
{
	EnsureInventoryStateInitialized();
	RefreshLegacyPlayerInventoryItems();
}

void UTunaSweeperGameInstance::EnsureInventoryStateInitialized()
{
	if (bInventoryStateInitialized || bInventoryStateInitializing)
	{
		return;
	}

	bInventoryStateInitializing = true;
	if (!LoadGameState())
	{
		LoadedSlotTotalPlaySeconds = 0.0f;
		ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
		GenerateDefaultInventoryState();
	}

	CompleteInventoryStateInitialization();
}

void UTunaSweeperGameInstance::CompleteInventoryStateInitialization()
{
	bInventoryStateInitialized = true;
	bInventoryStateInitializing = false;
	RefreshLegacyPlayerInventoryItems();
	if (UTunaSweeperResearchSubsystem* ResearchSubsystem = GetSubsystem<UTunaSweeperResearchSubsystem>())
	{
		ResearchSubsystem->FlushDeferredResearchNotifications();
	}
}

bool UTunaSweeperGameInstance::LoadGameState()
{
	const FString ExistingSlotName = GetExistingSaveGameSlotName(ActiveSaveSlotIndex);
	if (ExistingSlotName.IsEmpty())
	{
		return false;
	}

	UTunaSweeperSaveGame* SaveGame = Cast<UTunaSweeperSaveGame>(
		TunaSweeperSave::LoadFlavorSave(ExistingSlotName));
	if (!SaveGame)
	{
		return false;
	}
	if (!TunaSweeperSaveFlavor::IsCompatible(*SaveGame))
	{
		UE_LOG(
			LogTunaSweeperGameInstance,
			Error,
			TEXT("Refusing to load save '%s' for active build flavor '%s'."),
			*ExistingSlotName,
			*TunaSweeperBuildFlavor::GetName().ToString());
		return false;
	}

	LoadedSlotTotalPlaySeconds = FMath::Max(0.0f, SaveGame->TotalPlaySeconds);
	ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
	ActiveSaveSlotDifficultyStage = TunaSweeperSave::SanitizeDifficultyStage(SaveGame->DifficultyStage);
	// Demo has no difficulty selection. A compatible Demo slot can only exist after
	// the player confirms the one-time save-data notice, so treat it as initialized.
	bActiveSaveSlotDifficultySelected =
		TunaSweeperBuildFlavor::IsDemo() || SaveGame->bDifficultySelected;
	TotalExperiencePoints = FMath::Max<int64>(0, SaveGame->TotalExperiencePoints);
	RaidStartExperiencePoints = TotalExperiencePoints;
	PendingRaidExperiencePoints = 0;
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	CompletedScenarioFlags.Reset();
	for (const FName& ScenarioFlag : SaveGame->CompletedScenarioFlags)
	{
		if (!ScenarioFlag.IsNone())
		{
			CompletedScenarioFlags.Add(ScenarioFlag);
		}
	}
	if (CompletedScenarioFlags.Contains(TunaSweeperScenario::OpeningScenarioFlag))
	{
		bActiveSaveSlotDifficultySelected = true;
	}
	AcquiredMemoIds.Reset();
	for (int32 MemoId : SaveGame->AcquiredMemoIds)
	{
		if (MemoId > 0)
		{
			AcquiredMemoIds.Add(MemoId);
		}
	}
	EverAcquiredItemIds.Reset();
	for (int32 ItemId : SaveGame->EverAcquiredItemIds)
	{
		if (ItemId != INDEX_NONE)
		{
			EverAcquiredItemIds.Add(ItemId);
		}
	}
	MapMarkers.Reset();
	NextMapMarkerId = 1;
	TSet<int32> LoadedMapMarkerIds;
	for (const FTunaSweeperMapMarkerSaveData& SavedMapMarker : SaveGame->MapMarkers)
	{
		if (SavedMapMarker.MarkerId <= 0 || LoadedMapMarkerIds.Contains(SavedMapMarker.MarkerId))
		{
			continue;
		}

		FTunaSweeperMapMarkerSaveData LoadedMapMarker = TunaSweeperMapMarkers::SanitizeMarker(SavedMapMarker);
		MapMarkers.Add(LoadedMapMarker);
		LoadedMapMarkerIds.Add(LoadedMapMarker.MarkerId);
		NextMapMarkerId = FMath::Max(NextMapMarkerId, LoadedMapMarker.MarkerId + 1);
	}
	MapMarkers.Sort([](
		const FTunaSweeperMapMarkerSaveData& Left,
		const FTunaSweeperMapMarkerSaveData& Right)
	{
		return Left.MarkerId < Right.MarkerId;
	});
	WorldProgressStatesById.Reset();
	for (const FTunaSweeperWorldProgressSaveData& SavedWorldProgressState : SaveGame->WorldProgressStates)
	{
		if (SavedWorldProgressState.ObjectId.IsNone())
		{
			continue;
		}

		FTunaSweeperWorldProgressSaveData LoadedWorldProgressState = SavedWorldProgressState;
		LoadedWorldProgressState.ProgressQuantity = FMath::Max(0, LoadedWorldProgressState.ProgressQuantity);
		WorldProgressStatesById.Add(LoadedWorldProgressState.ObjectId, LoadedWorldProgressState);
	}
	PiggyBankStatesById.Reset();
	for (const FTunaSweeperPiggyBankSaveData& SavedPiggyBankState : SaveGame->PiggyBankStates)
	{
		if (SavedPiggyBankState.PiggyBankId.IsNone())
		{
			continue;
		}

		FTunaSweeperPiggyBankSaveData LoadedPiggyBankState = SavedPiggyBankState;
		LoadedPiggyBankState.StoredAncientCoinValue = FMath::Max(0, LoadedPiggyBankState.StoredAncientCoinValue);
		PiggyBankStatesById.Add(LoadedPiggyBankState.PiggyBankId, LoadedPiggyBankState);
	}
	HousingFacilities.Reset();
	TSet<FGuid> LoadedHousingFacilityIds;
	for (const FTunaSweeperHousingPlacedFacilitySaveData& SavedHousingFacility : SaveGame->HousingFacilities)
	{
		if (!SavedHousingFacility.IsValid() || LoadedHousingFacilityIds.Contains(SavedHousingFacility.InstanceId))
		{
			continue;
		}

		FTunaSweeperHousingPlacedFacilitySaveData LoadedHousingFacility = SavedHousingFacility;
		LoadedHousingFacility.RotationQuarterTurns = FMath::Clamp(LoadedHousingFacility.RotationQuarterTurns, 0, 3);
		HousingFacilities.Add(LoadedHousingFacility);
		LoadedHousingFacilityIds.Add(LoadedHousingFacility.InstanceId);
	}
	UnlockedHousingFacilityIds.Reset();
	for (const FName& FacilityId : SaveGame->UnlockedHousingFacilityIds)
	{
		if (!FacilityId.IsNone())
		{
			UnlockedHousingFacilityIds.Add(FacilityId);
		}
	}
	UnlockedWorkbenchRecipeIds.Reset();
	for (const FName& RecipeId : SaveGame->UnlockedWorkbenchRecipeIds)
	{
		if (!RecipeId.IsNone())
		{
			UnlockedWorkbenchRecipeIds.Add(RecipeId);
		}
	}
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->LoadQuestProgressFromSave(
			SaveGame->QuestProgressStates,
			SaveGame->TrackedQuestId,
			SaveGame->QuestCoinBalance);

		TArray<FTunaSweeperQuestDefinition> QuestDefinitions;
		if (QuestSubsystem->GetAllQuestDefinitions(QuestDefinitions))
		{
			for (const FTunaSweeperQuestDefinition& QuestDefinition : QuestDefinitions)
			{
				if (QuestSubsystem->GetQuestState(QuestDefinition.QuestId) != ETunaSweeperQuestState::RewardCompleted)
				{
					continue;
				}

				for (const FName& FacilityId : QuestDefinition.Rewards.HousingFacilityUnlocks)
				{
					if (!FacilityId.IsNone())
					{
						UnlockedHousingFacilityIds.Add(FacilityId);
					}
				}

				for (const FName& RecipeId : QuestDefinition.Rewards.WorkbenchRecipeUnlocks)
				{
					if (!RecipeId.IsNone())
					{
						UnlockedWorkbenchRecipeIds.Add(RecipeId);
					}
				}
			}
		}
	}
	if (UTunaSweeperResearchSubsystem* ResearchSubsystem = GetSubsystem<UTunaSweeperResearchSubsystem>())
	{
		ResearchSubsystem->LoadResearchProgressFromSave(
			SaveGame->AppliedResearchNodeIds,
			SaveGame->ActiveResearchStates,
			SaveGame->ResearchLastObservedUtcTicks,
			ETunaSweeperResearchNotificationMode::Deferred);
	}
	PendingScenarioCompletionFlag = NAME_None;
	bPendingScenarioBunkerEntryPresentation = false;

	ItemInstancesByUid.Reset();
	for (const FTunaSweeperItemInstance& ItemInstance : SaveGame->ItemInstances)
	{
		FTunaSweeperItemInstance LoadedItemInstance = ItemInstance;
		TunaSweeperInventory::NormalizeLoadedAmmoPersistenceFields(LoadedItemInstance);
		if (LoadedItemInstance.IsValid())
		{
			ItemInstancesByUid.Add(LoadedItemInstance.Uid, LoadedItemInstance);
		}
	}

	PlayerInventorySlots = SaveGame->InventorySlots;
	EquipmentSlots = SaveGame->EquipmentSlots;
	AuxiliaryBagSlots = SaveGame->AuxiliaryBagSlots;
	UsableQuickSlots = SaveGame->UsableQuickSlots;
	StorageSlotCapacity = NormalizeStorageSlotCapacity(SaveGame->StorageSlotCapacity);
	StorageSlots = SaveGame->StorageSlots;
	ShopStockStatesByKey.Reset();
	for (const FTunaSweeperShopStockSaveData& SavedShopStockState : SaveGame->ShopStockStates)
	{
		if (!TunaSweeperShop::IsValidShopSlotKey(
			SavedShopStockState.ShopId,
			SavedShopStockState.SlotIndex,
			SavedShopStockState.ItemId))
		{
			continue;
		}

		FTunaSweeperShopStockSaveData LoadedShopStockState = SavedShopStockState;
		LoadedShopStockState.StockQuantity = FMath::Max(0, LoadedShopStockState.StockQuantity);
		ShopStockStatesByKey.Add(
			TunaSweeperShop::MakeStockKey(
				LoadedShopStockState.ShopId,
				LoadedShopStockState.SlotIndex,
				LoadedShopStockState.ItemId),
			LoadedShopStockState);
	}
	RemoveInvalidSlotReferences(PlayerInventorySlots);
	RemoveInvalidSlotReferences(EquipmentSlots);
	RemoveInvalidSlotReferences(AuxiliaryBagSlots);
	RemoveInvalidSlotReferences(UsableQuickSlots);
	RemoveInvalidSlotReferences(StorageSlots);

	EnsureSlotArraySize(EquipmentSlots, FMath::Max(TunaSweeperInventory::RequiredEquipmentSlots, GameplaySettings.EquipmentSlotCount));
	EnsureSlotArraySize(AuxiliaryBagSlots, FMath::Max(0, GameplaySettings.AuxiliaryBagSlotCount));
	EnsureSlotArraySize(UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);
	for (int32 SlotIndex = StorageSlots.Num() - 1; SlotIndex >= StorageSlotCapacity; --SlotIndex)
	{
		if (StorageSlots[SlotIndex].ItemUid.IsValid())
		{
			StorageSlotCapacity = NormalizeStorageSlotCapacity(SlotIndex + 1);
			break;
		}
	}
	EnsureSlotArraySize(StorageSlots, StorageSlotCapacity);
	for (FTunaSweeperInventorySlot& UsableQuickSlot : UsableQuickSlots)
	{
		if (UsableQuickSlot.ItemUid.IsValid() && !IsItemCompatibleWithUsableQuickSlot(UsableQuickSlot.ItemUid))
		{
			UsableQuickSlot.Clear();
		}
	}
	MigrateLegacyEquipmentSlots();
	BackfillEverAcquiredItemIdsFromCurrentItems();

	int32 InventoryCapacity = CalculateInventoryCapacityForEquipmentSlots(EquipmentSlots);
	for (int32 SlotIndex = PlayerInventorySlots.Num() - 1; SlotIndex >= InventoryCapacity; --SlotIndex)
	{
		if (PlayerInventorySlots[SlotIndex].ItemUid.IsValid())
		{
			InventoryCapacity = FMath::Min(
				FMath::Max(TunaSweeperInventory::RequiredMaxInventorySlots, GameplaySettings.MaxInventorySlots),
				SlotIndex + 1);
			break;
		}
	}
	EnsureSlotArraySize(PlayerInventorySlots, InventoryCapacity);

	ActiveLootContainerSlots.Reset();
	ActiveLootContainerOwner.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	HoveredItemSlotReference = FTunaSweeperItemSlotReference();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();
	return true;
}

bool UTunaSweeperGameInstance::SaveGameStateInternal(
	UTunaSweeperGameInstance::EUsableQuickSlotSaveMode UsableQuickSlotSaveMode) const
{
	const FString ExistingSlotName = GetExistingSaveGameSlotName(ActiveSaveSlotIndex);
	UTunaSweeperSaveGame* ExistingSaveGame = nullptr;
	if (!ExistingSlotName.IsEmpty())
	{
		UTunaSweeperSaveGame* LoadedExistingSaveGame = Cast<UTunaSweeperSaveGame>(
			TunaSweeperSave::LoadFlavorSave(ExistingSlotName));
		if (!LoadedExistingSaveGame || !TunaSweeperSaveFlavor::IsCompatible(*LoadedExistingSaveGame))
		{
			UE_LOG(
				LogTunaSweeperGameInstance,
				Error,
				TEXT("Refusing to overwrite an incompatible build-flavor save: %s"),
				*ExistingSlotName);
			return false;
		}

		if (UsableQuickSlotSaveMode == EUsableQuickSlotSaveMode::PreserveExisting)
		{
			ExistingSaveGame = LoadedExistingSaveGame;
		}
	}

	UTunaSweeperSaveGame* SaveGame = Cast<UTunaSweeperSaveGame>(
		UGameplayStatics::CreateSaveGameObject(UTunaSweeperSaveGame::StaticClass()));
	if (!SaveGame)
	{
		return false;
	}

	TSet<FGuid> PlayerOwnedItemUids;
	CollectPlayerOwnedItemUids(
		PlayerOwnedItemUids,
		UsableQuickSlotSaveMode == EUsableQuickSlotSaveMode::PersistRuntime);
	for (const FGuid& ItemUid : PlayerOwnedItemUids)
	{
		if (const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(ItemUid))
		{
			SaveGame->ItemInstances.Add(TunaSweeperInventory::MakeItemInstanceForSave(*ItemInstance));
		}
	}

	SaveGame->SaveVersion = TunaSweeperSave::CurrentSaveVersion;
	SaveGame->SaveSlotIndex = ActiveSaveSlotIndex;
	SaveGame->BuildFlavor = TunaSweeperBuildFlavor::GetName();
	SaveGame->TotalPlaySeconds = GetCurrentActiveSlotTotalPlaySeconds();
	SaveGame->DifficultyStage = TunaSweeperSave::SanitizeDifficultyStage(ActiveSaveSlotDifficultyStage);
	SaveGame->bDifficultySelected = bActiveSaveSlotDifficultySelected;
	SaveGame->LastSavedAtTicks = FDateTime::Now().GetTicks();
	SaveGame->TotalExperiencePoints = FMath::Max<int64>(0, TotalExperiencePoints);
	SaveGame->CompletedScenarioFlags = CompletedScenarioFlags.Array();
	SaveGame->AcquiredMemoIds = AcquiredMemoIds.Array();
	SaveGame->AcquiredMemoIds.Sort();
	SaveGame->EverAcquiredItemIds = EverAcquiredItemIds.Array();
	SaveGame->EverAcquiredItemIds.Sort();
	SaveGame->MapMarkers = MapMarkers;
	SaveGame->MapMarkers.Sort([](
		const FTunaSweeperMapMarkerSaveData& Left,
		const FTunaSweeperMapMarkerSaveData& Right)
	{
		return Left.MarkerId < Right.MarkerId;
	});
	WorldProgressStatesById.GenerateValueArray(SaveGame->WorldProgressStates);
	SaveGame->WorldProgressStates.Sort([](
		const FTunaSweeperWorldProgressSaveData& Left,
		const FTunaSweeperWorldProgressSaveData& Right)
	{
		return Left.ObjectId.LexicalLess(Right.ObjectId);
	});
	PiggyBankStatesById.GenerateValueArray(SaveGame->PiggyBankStates);
	SaveGame->PiggyBankStates.Sort([](
		const FTunaSweeperPiggyBankSaveData& Left,
		const FTunaSweeperPiggyBankSaveData& Right)
	{
		return Left.PiggyBankId.LexicalLess(Right.PiggyBankId);
	});
	SaveGame->HousingFacilities = HousingFacilities;
	SaveGame->HousingFacilities.Sort([](
		const FTunaSweeperHousingPlacedFacilitySaveData& Left,
		const FTunaSweeperHousingPlacedFacilitySaveData& Right)
	{
		return Left.FacilityId.LexicalLess(Right.FacilityId) ||
			(Left.FacilityId == Right.FacilityId && Left.InstanceId.ToString() < Right.InstanceId.ToString());
	});
	SaveGame->UnlockedHousingFacilityIds = UnlockedHousingFacilityIds.Array();
	SaveGame->UnlockedHousingFacilityIds.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
	SaveGame->UnlockedWorkbenchRecipeIds = UnlockedWorkbenchRecipeIds.Array();
	SaveGame->UnlockedWorkbenchRecipeIds.Sort([](const FName& Left, const FName& Right)
	{
		return Left.LexicalLess(Right);
	});
	if (const UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->ExportQuestProgressForSave(
			SaveGame->QuestProgressStates,
			SaveGame->TrackedQuestId,
			SaveGame->QuestCoinBalance);
	}
	if (const UTunaSweeperResearchSubsystem* ResearchSubsystem = GetSubsystem<UTunaSweeperResearchSubsystem>())
	{
		ResearchSubsystem->ExportResearchProgressForSave(
			SaveGame->AppliedResearchNodeIds,
			SaveGame->ActiveResearchStates,
			SaveGame->ResearchLastObservedUtcTicks);
	}
	SaveGame->InventorySlots = PlayerInventorySlots;
	SaveGame->EquipmentSlots = EquipmentSlots;
	SaveGame->AuxiliaryBagSlots = AuxiliaryBagSlots;
	SaveGame->StorageSlotCapacity = NormalizeStorageSlotCapacity(StorageSlotCapacity);
	SaveGame->StorageSlots = StorageSlots;
	EnsureSlotArraySize(SaveGame->StorageSlots, SaveGame->StorageSlotCapacity);
	ShopStockStatesByKey.GenerateValueArray(SaveGame->ShopStockStates);
	SaveGame->ShopStockStates.Sort([](
		const FTunaSweeperShopStockSaveData& Left,
		const FTunaSweeperShopStockSaveData& Right)
	{
		if (Left.ShopId != Right.ShopId)
		{
			return Left.ShopId < Right.ShopId;
		}
		if (Left.SlotIndex != Right.SlotIndex)
		{
			return Left.SlotIndex < Right.SlotIndex;
		}
		return Left.ItemId < Right.ItemId;
	});

	switch (UsableQuickSlotSaveMode)
	{
	case EUsableQuickSlotSaveMode::PersistRuntime:
		SaveGame->UsableQuickSlots = UsableQuickSlots;
		break;
	case EUsableQuickSlotSaveMode::PreserveExisting:
		if (ExistingSaveGame)
		{
			TMap<FGuid, FTunaSweeperItemInstance> ExistingItemInstancesByUid;
			for (const FTunaSweeperItemInstance& ExistingItemInstance : ExistingSaveGame->ItemInstances)
			{
				FTunaSweeperItemInstance NormalizedItemInstance = ExistingItemInstance;
				TunaSweeperInventory::NormalizeLoadedAmmoPersistenceFields(NormalizedItemInstance);
				if (NormalizedItemInstance.IsValid())
				{
					ExistingItemInstancesByUid.Add(NormalizedItemInstance.Uid, NormalizedItemInstance);
				}
			}

			TSet<FGuid> SavedItemUids;
			for (const FTunaSweeperItemInstance& SavedItemInstance : SaveGame->ItemInstances)
			{
				if (SavedItemInstance.Uid.IsValid())
				{
					SavedItemUids.Add(SavedItemInstance.Uid);
				}
			}

			TFunction<void(const FGuid&)> AppendExistingItemUid;
			AppendExistingItemUid = [
				&AppendExistingItemUid,
				&ExistingItemInstancesByUid,
				&SavedItemUids,
				SaveGame](const FGuid& ItemUid)
			{
				if (!ItemUid.IsValid() || SavedItemUids.Contains(ItemUid))
				{
					return;
				}

				const FTunaSweeperItemInstance* ExistingItemInstance = ExistingItemInstancesByUid.Find(ItemUid);
				if (!ExistingItemInstance)
				{
					return;
				}

				SavedItemUids.Add(ItemUid);
				SaveGame->ItemInstances.Add(TunaSweeperInventory::MakeItemInstanceForSave(*ExistingItemInstance));
				for (const TPair<FName, FGuid>& AttachmentSlot : ExistingItemInstance->AttachmentSlots)
				{
					AppendExistingItemUid(AttachmentSlot.Value);
				}
			};

			SaveGame->UsableQuickSlots = ExistingSaveGame->UsableQuickSlots;
			EnsureSlotArraySize(SaveGame->UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);
			for (FTunaSweeperInventorySlot& UsableQuickSlot : SaveGame->UsableQuickSlots)
			{
				if (!UsableQuickSlot.ItemUid.IsValid())
				{
					continue;
				}

				if (!ExistingItemInstancesByUid.Contains(UsableQuickSlot.ItemUid))
				{
					UsableQuickSlot.Clear();
					continue;
				}

				AppendExistingItemUid(UsableQuickSlot.ItemUid);
			}
		}
		break;
	case EUsableQuickSlotSaveMode::Clear:
	default:
		SaveGame->UsableQuickSlots.Reset();
		break;
	}
	EnsureSlotArraySize(SaveGame->UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);

	if (!ExistingSlotName.IsEmpty() && !BackupExistingSaveGame(ExistingSlotName))
	{
		return false;
	}

	return TunaSweeperSave::SaveFlavorSave(
		SaveGame,
		GetSaveGameSlotName(ActiveSaveSlotIndex));
}

void UTunaSweeperGameInstance::ResetRuntimeStateForSaveSlotSelection()
{
	TGuardValue<bool> InitializationGuard(bInventoryStateInitializing, true);
	DespawnPetCompanion();

	GameplayInfo.Reset();
	NumberSettings.Reset();
	BoolSettings.Reset();
	PlayerHudState = FTunaSweeperPlayerHudState();
	PlayerInventoryItems.Reset();
	bHasGeneratedPlayerInventoryItems = false;
	ItemInstancesByUid.Reset();
	PlayerInventorySlots.Reset();
	EquipmentSlots.Reset();
	AuxiliaryBagSlots.Reset();
	UsableQuickSlots.Reset();
	StorageSlots.Reset();
	StorageSlotCapacity = GetDefaultStorageSlotCapacity();
	ShopStockStatesByKey.Reset();
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerOwner.Reset();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	HoveredItemSlotReference = FTunaSweeperItemSlotReference();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	bInventoryStateInitialized = false;
	bPendingBunkerItemStateSave = false;
	LoadedSlotTotalPlaySeconds = 0.0f;
	ActiveSlotStartTimeSeconds = FPlatformTime::Seconds();
	ActiveSaveSlotDifficultyStage = TunaSweeperSave::DefaultDifficultyStage;
	bActiveSaveSlotDifficultySelected = false;
	TotalExperiencePoints = 0;
	RaidStartExperiencePoints = 0;
	PendingRaidExperiencePoints = 0;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	bHasPendingBunkerEntryVitals = false;
	PendingBunkerEntryHealthRatio = 1.0f;
	PendingBunkerEntryFoodRatio = 1.0f;
	PendingBunkerEntryHydrationRatio = 1.0f;
	CompletedScenarioFlags.Reset();
	AcquiredMemoIds.Reset();
	EverAcquiredItemIds.Reset();
	MapMarkers.Reset();
	NextMapMarkerId = 1;
	WorldProgressStatesById.Reset();
	PiggyBankStatesById.Reset();
	HousingFacilities.Reset();
	UnlockedHousingFacilityIds.Reset();
	UnlockedWorkbenchRecipeIds.Reset();
	PendingScenarioCompletionFlag = NAME_None;
	bPendingScenarioBunkerEntryPresentation = false;
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->ResetQuestProgressForNewGame();
	}
	if (UTunaSweeperResearchSubsystem* ResearchSubsystem = GetSubsystem<UTunaSweeperResearchSubsystem>())
	{
		ResearchSubsystem->ResetResearchProgressForNewGame(
			ETunaSweeperResearchNotificationMode::Deferred);
	}
}

void UTunaSweeperGameInstance::GenerateDefaultInventoryState()
{
	ItemInstancesByUid.Reset();
	ActiveSaveSlotDifficultyStage = TunaSweeperSave::DefaultDifficultyStage;
	bActiveSaveSlotDifficultySelected = false;
	TotalExperiencePoints = 0;
	RaidStartExperiencePoints = 0;
	PendingRaidExperiencePoints = 0;
	PendingRaidExperienceAnimationState = FTunaSweeperExperienceAnimationState();
	bRaidExperienceSessionActive = false;
	bHasPendingRaidExperienceAnimationState = false;
	bHasPendingBunkerEntryVitals = false;
	bPendingBunkerItemStateSave = false;
	PendingBunkerEntryHealthRatio = 1.0f;
	PendingBunkerEntryFoodRatio = 1.0f;
	PendingBunkerEntryHydrationRatio = 1.0f;
	CompletedScenarioFlags.Reset();
	AcquiredMemoIds.Reset();
	EverAcquiredItemIds.Reset();
	MapMarkers.Reset();
	NextMapMarkerId = 1;
	WorldProgressStatesById.Reset();
	PiggyBankStatesById.Reset();
	HousingFacilities.Reset();
	UnlockedHousingFacilityIds.Reset();
	UnlockedWorkbenchRecipeIds.Reset();
	PendingScenarioCompletionFlag = NAME_None;
	bPendingScenarioBunkerEntryPresentation = false;
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetSubsystem<UTunaSweeperQuestSubsystem>())
	{
		QuestSubsystem->ResetQuestProgressForNewGame();
	}
	if (UTunaSweeperResearchSubsystem* ResearchSubsystem = GetSubsystem<UTunaSweeperResearchSubsystem>())
	{
		ResearchSubsystem->ResetResearchProgressForNewGame(
			ETunaSweeperResearchNotificationMode::Deferred);
	}
	ResetPlayerSlotArrays();
	StorageSlotCapacity = GetDefaultStorageSlotCapacity();
	StorageSlots.Reset();
	EnsureSlotArraySize(StorageSlots, StorageSlotCapacity);
	ShopStockStatesByKey.Reset();
	ActiveShopId = INDEX_NONE;
	bHasActiveShop = false;
	ActiveWorkbenchId = INDEX_NONE;
	ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;
	bHasActiveWorkbench = false;
	ActiveLootContainerSlots.Reset();
	ActiveLootContainerOwner.Reset();
	ActiveLootContainerDisplayName = FText::GetEmpty();
	ActiveLootContainerCapacity = 0;
	bHasActiveLootContainer = false;
	SelectedItemSlotReference = FTunaSweeperItemSlotReference();
	HoveredItemSlotReference = FTunaSweeperItemSlotReference();
	SelectedWeaponAttachmentSlotTags.Reset();
	SelectedWeaponAttachmentSlots.Reset();
}

bool UTunaSweeperGameInstance::InitializeDemoStartingLoadout()
{
	constexpr int32 RifleItemId = 1002;
	constexpr int32 RifleAmmoItemId = 2002;
	constexpr int32 StartingAmmoCount = 60;
	constexpr int32 PrimaryWeaponEquipmentSlotIndex = 0;
	constexpr int32 ReserveAmmoInventorySlotIndex = 0;

	if (!EquipmentSlots.IsValidIndex(PrimaryWeaponEquipmentSlotIndex) ||
		!PlayerInventorySlots.IsValidIndex(ReserveAmmoInventorySlotIndex))
	{
		return false;
	}

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetSubsystem<UTunaSweeperItemDataSubsystem>();
	FTunaSweeperItemDefinition RifleDefinition;
	FTunaSweeperItemDefinition RifleAmmoDefinition;
	if (!ItemDataSubsystem ||
		!ItemDataSubsystem->TryGetItemDefinition(RifleItemId, RifleDefinition) ||
		!ItemDataSubsystem->TryGetItemDefinition(RifleAmmoItemId, RifleAmmoDefinition) ||
		!DoesItemDefinitionMatchEquipmentSlot(PrimaryWeaponEquipmentSlotIndex, RifleDefinition) ||
		!IsAmmoDefinitionCompatibleWithWeapon(RifleDefinition, RifleAmmoDefinition))
	{
		return false;
	}

	const FGuid RifleUid = CreateItemInstance(RifleItemId, 1);
	FTunaSweeperItemInstance* RifleInstance = ItemInstancesByUid.Find(RifleUid);
	if (!RifleInstance)
	{
		return false;
	}

	const int32 LoadedAmmoCount = FMath::Min(
		StartingAmmoCount,
		CalculateWeaponMagazineCapacity(*RifleInstance, RifleDefinition));
	RifleInstance->LoadedAmmoItemId = RifleAmmoItemId;
	RifleInstance->SelectedAmmoItemId = RifleAmmoItemId;
	RifleInstance->LoadedAmmoCount = LoadedAmmoCount;
	EquipmentSlots[PrimaryWeaponEquipmentSlotIndex].ItemUid = RifleUid;

	const int32 ReserveAmmoCount = StartingAmmoCount - LoadedAmmoCount;
	if (ReserveAmmoCount > 0)
	{
		const FGuid AmmoUid = CreateItemInstance(RifleAmmoItemId, ReserveAmmoCount);
		if (!AmmoUid.IsValid())
		{
			EquipmentSlots[PrimaryWeaponEquipmentSlotIndex].Clear();
			ItemInstancesByUid.Remove(RifleUid);
			return false;
		}
		PlayerInventorySlots[ReserveAmmoInventorySlotIndex].ItemUid = AmmoUid;
	}

	MarkItemEverAcquired(RifleItemId);
	MarkItemEverAcquired(RifleAmmoItemId);
	SetRuntimeSelectedWeaponSlotNumber(1);
	return true;
}

void UTunaSweeperGameInstance::ResetPlayerSlotArrays()
{
	EquipmentSlots.Reset();
	AuxiliaryBagSlots.Reset();
	UsableQuickSlots.Reset();
	PlayerInventorySlots.Reset();
	EnsureSlotArraySize(EquipmentSlots, FMath::Max(TunaSweeperInventory::RequiredEquipmentSlots, GameplaySettings.EquipmentSlotCount));
	EnsureSlotArraySize(AuxiliaryBagSlots, FMath::Max(0, GameplaySettings.AuxiliaryBagSlotCount));
	EnsureSlotArraySize(UsableQuickSlots, TunaSweeperInventory::UsableQuickSlotCount);
	EnsureSlotArraySize(PlayerInventorySlots, FMath::Max(TunaSweeperInventory::RequiredBareInventorySlots, GameplaySettings.BareInventorySlots));
}

void UTunaSweeperGameInstance::RefreshLegacyPlayerInventoryItems()
{
	PlayerInventoryItems.Reset();
	for (const FTunaSweeperInventorySlot& InventorySlot : PlayerInventorySlots)
	{
		FTunaSweeperItemStack ItemStack;
		if (const FTunaSweeperItemInstance* ItemInstance = ItemInstancesByUid.Find(InventorySlot.ItemUid))
		{
			ItemStack.ItemId = ItemInstance->ItemId;
			ItemStack.Quantity = ItemInstance->Quantity;
		}
		else
		{
			ItemStack.ItemId = INDEX_NONE;
		}

		PlayerInventoryItems.Add(ItemStack);
	}

	bHasGeneratedPlayerInventoryItems = true;
}

bool UTunaSweeperGameInstance::BackupExistingSaveGame(const FString& ExistingSlotName) const
{
	if (ExistingSlotName.IsEmpty())
	{
		return true;
	}

	const FString BackupDirectory = GetSaveGameBackupDirectory();
	if (!IFileManager::Get().MakeDirectory(*BackupDirectory, true))
	{
		return false;
	}

	const int32 BackupSlotIndex = SanitizeSaveSlotIndex(ActiveSaveSlotIndex);
	const FString BackupFilePath = CreateSaveGameBackupFilePath(BackupSlotIndex, FDateTime::Now());
	UTunaSweeperSaveGame* ExistingSaveGame = Cast<UTunaSweeperSaveGame>(
		TunaSweeperSave::LoadFlavorSave(ExistingSlotName));
	if (!ExistingSaveGame)
	{
		return false;
	}
	ExistingSaveGame->SaveSlotIndex = BackupSlotIndex;
	if (!TunaSweeperSafeSave::SaveGameFileFailClosed(
			ExistingSaveGame,
			BackupFilePath,
			TunaSweeperSave::MakeFlavorSaveValidator(ExistingSlotName)))
	{
		return false;
	}

	TrimSaveGameBackups();
	return true;
}

void UTunaSweeperGameInstance::TrimSaveGameBackups() const
{
	const FString BackupDirectory = GetSaveGameBackupDirectory();
	TArray<FString> BackupFiles;
	IFileManager::Get().FindFilesRecursive(
		BackupFiles,
		*BackupDirectory,
		TEXT("*.sav"),
		true,
		false);

	if (BackupFiles.Num() <= TunaSweeperSave::MaxSaveGameBackupCount)
	{
		return;
	}

	BackupFiles.Sort([](const FString& Left, const FString& Right)
	{
		const FDateTime LeftTime = IFileManager::Get().GetTimeStamp(*Left);
		const FDateTime RightTime = IFileManager::Get().GetTimeStamp(*Right);
		return LeftTime == RightTime ? Left < Right : LeftTime < RightTime;
	});

	const int32 DeleteCount = BackupFiles.Num() - TunaSweeperSave::MaxSaveGameBackupCount;
	for (int32 BackupIndex = 0; BackupIndex < DeleteCount; ++BackupIndex)
	{
		IFileManager::Get().Delete(*BackupFiles[BackupIndex], false, true);
	}
}

bool UTunaSweeperGameInstance::LoadActiveSaveSlotSelection(int32& OutSaveSlotIndex) const
{
	if (!TunaSweeperSave::DoesFlavorSaveExist(GetSaveSettingsSlotName()))
	{
		OutSaveSlotIndex = 1;
		return false;
	}

	const UTunaSweeperSaveSettings* SaveSettings = Cast<UTunaSweeperSaveSettings>(
		TunaSweeperSave::LoadFlavorSave(GetSaveSettingsSlotName()));
	if (!SaveSettings)
	{
		OutSaveSlotIndex = 1;
		return false;
	}

	OutSaveSlotIndex = SanitizeSaveSlotIndex(SaveSettings->LastSelectedSaveSlotIndex);
	return true;
}

bool UTunaSweeperGameInstance::SaveActiveSaveSlotSelection() const
{
	UTunaSweeperSaveSettings* SaveSettings = Cast<UTunaSweeperSaveSettings>(
		UGameplayStatics::CreateSaveGameObject(UTunaSweeperSaveSettings::StaticClass()));
	if (!SaveSettings)
	{
		return false;
	}

	SaveSettings->LastSelectedSaveSlotIndex = SanitizeSaveSlotIndex(ActiveSaveSlotIndex);
	return TunaSweeperSave::SaveFlavorSave(SaveSettings, GetSaveSettingsSlotName());
}

int32 UTunaSweeperGameInstance::FindFirstExistingSaveSlotIndex() const
{
	for (int32 SaveSlotIndex = TunaSweeperSave::MinSaveSlotIndex;
		SaveSlotIndex <= TunaSweeperBuildFlavor::GetMaximumSaveSlotIndex();
		++SaveSlotIndex)
	{
		if (!GetExistingSaveGameSlotName(SaveSlotIndex).IsEmpty())
		{
			return SaveSlotIndex;
		}
	}

	return TunaSweeperSave::MinSaveSlotIndex;
}

int32 UTunaSweeperGameInstance::SanitizeSaveSlotIndex(int32 SaveSlotIndex) const
{
	return FMath::Clamp(
		SaveSlotIndex,
		TunaSweeperSave::MinSaveSlotIndex,
		TunaSweeperBuildFlavor::GetMaximumSaveSlotIndex());
}

FString UTunaSweeperGameInstance::GetSaveGameSlotName(int32 SaveSlotIndex) const
{
	return FString::Printf(
		TEXT("%s%02d"),
		TunaSweeperSave::SaveSlotNamePrefix,
		SanitizeSaveSlotIndex(SaveSlotIndex));
}

FString UTunaSweeperGameInstance::GetSaveSettingsSlotName() const
{
	return FString(TunaSweeperSave::SaveSettingsSlotName);
}

FString UTunaSweeperGameInstance::GetExistingSaveGameSlotName(int32 SaveSlotIndex) const
{
	const int32 SanitizedSlotIndex = SanitizeSaveSlotIndex(SaveSlotIndex);
	const FString SlotName = GetSaveGameSlotName(SanitizedSlotIndex);
	if (TunaSweeperSave::DoesFlavorSaveExist(SlotName))
	{
		const FString SaveFilePath = GetSaveGameFilePath(SlotName);
		if (TunaSweeperSave::DeleteOutdatedSaveFileIfNeeded(
				SaveFilePath,
				TunaSweeperBuildFlavor::GetSaveGameDirectory()) ==
			TunaSweeperSave::EOutdatedSaveCleanupResult::Deleted)
		{
			TunaSweeperSave::DeleteFlavorSave(SlotName);
			return FString();
		}
		return SlotName;
	}

	return FString();
}

FString UTunaSweeperGameInstance::GetSaveGameFilePath(const FString& SaveSlotName) const
{
	return FPaths::Combine(
		TunaSweeperBuildFlavor::GetSaveGameDirectory(),
		SaveSlotName + TEXT(".sav"));
}

FString UTunaSweeperGameInstance::GetSaveGameBackupDirectory() const
{
	return FPaths::Combine(TunaSweeperBuildFlavor::GetSaveGameDirectory(), TEXT("Backups"));
}

FString UTunaSweeperGameInstance::CreateSaveGameBackupFilePath(int32 SaveSlotIndex, FDateTime BackupTime) const
{
	const FString BackupFileName = FString::Printf(
		TEXT("SaveSlot%02d_%s_%lld.sav"),
		SanitizeSaveSlotIndex(SaveSlotIndex),
		*BackupTime.ToString(TEXT("%Y%m%d_%H%M%S")),
		BackupTime.GetTicks());

	return FPaths::Combine(GetSaveGameBackupDirectory(), BackupFileName);
}

float UTunaSweeperGameInstance::GetCurrentActiveSlotTotalPlaySeconds() const
{
	const double SessionSeconds = ActiveSlotStartTimeSeconds > 0.0
		? FPlatformTime::Seconds() - ActiveSlotStartTimeSeconds
		: 0.0;
	return LoadedSlotTotalPlaySeconds + static_cast<float>(FMath::Max(0.0, SessionSeconds));
}

bool UTunaSweeperGameInstance::IsCurrentWorldBunkerMap() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().EndsWith(TEXT("BunkerMap"));
}

bool UTunaSweeperGameInstance::IsBunkerToRaidTravel(FName SourceLevelName, FName TargetLevelName) const
{
	return IsMapNameMatch(SourceLevelName, TEXT("BunkerMap")) &&
		TunaSweeperBuildFlavor::IsRaidGameplayLevelName(TargetLevelName);
}

bool UTunaSweeperGameInstance::IsRaidToBunkerTravel(FName SourceLevelName, FName TargetLevelName) const
{
	return TunaSweeperBuildFlavor::IsRaidGameplayLevelName(SourceLevelName) &&
		IsMapNameMatch(TargetLevelName, TEXT("BunkerMap"));
}

bool UTunaSweeperGameInstance::IsMapNameMatch(FName MapName, const TCHAR* ExpectedMapName) const
{
	return MapName.ToString().EndsWith(ExpectedMapName);
}
