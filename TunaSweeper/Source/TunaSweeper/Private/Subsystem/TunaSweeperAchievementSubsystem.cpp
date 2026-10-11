#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "Quest/TunaSweeperQuestId.h"

#include "Achievement/TunaSweeperAchievementModel.h"
#include "Achievement/TunaSweeperAchievementPublisher.h"
#include "Achievement/TunaSweeperOnlineAchievementPublisher.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Game/TunaSweeperSafeSave.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "Subsystem/TunaSweeperToastSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperAchievement, Log, All);

namespace
{
	constexpr int32 AchievementSaveVersion = 2;
	constexpr int32 MaximumConfiguredSteamAchievements = 256;
	const TCHAR* AchievementConfigSection = TEXT("TunaSweeperAchievements");

	FName ReadNameField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName)
	{
		FString Value;
		return Object.IsValid() && Object->TryGetStringField(FieldName, Value) && !Value.TrimStartAndEnd().IsEmpty()
			? FName(*Value.TrimStartAndEnd())
			: NAME_None;
	}

	bool ParseConditionType(const FString& RawType, ETunaSweeperAchievementConditionType& OutType)
	{
		const FString Type = RawType.TrimStartAndEnd().ToLower();
		if (Type == TEXT("reserved")) { OutType = ETunaSweeperAchievementConditionType::Reserved; return true; }
		if (Type == TEXT("gameplay_event")) { OutType = ETunaSweeperAchievementConditionType::GameplayEvent; return true; }
		if (Type == TEXT("all_facilities_restored")) { OutType = ETunaSweeperAchievementConditionType::AllFacilitiesRestored; return true; }
		if (Type == TEXT("extraction_health")) { OutType = ETunaSweeperAchievementConditionType::ExtractionHealth; return true; }
		if (Type == TEXT("timed_extraction")) { OutType = ETunaSweeperAchievementConditionType::TimedExtraction; return true; }
		if (Type == TEXT("current_item_quantity")) { OutType = ETunaSweeperAchievementConditionType::CurrentItemQuantity; return true; }
		if (Type == TEXT("specific_enemy_first_kill"))
		{
			OutType = ETunaSweeperAchievementConditionType::SpecificEnemyFirstKill;
			return true;
		}
		if (Type == TEXT("total_enemy_kills"))
		{
			OutType = ETunaSweeperAchievementConditionType::TotalEnemyKills;
			return true;
		}
		if (Type == TEXT("location_reached"))
		{
			OutType = ETunaSweeperAchievementConditionType::LocationReached;
			return true;
		}
		if (Type == TEXT("quest_reward_claimed"))
		{
			OutType = ETunaSweeperAchievementConditionType::QuestRewardClaimed;
			return true;
		}
		return false;
	}

	void SortNames(TArray<FName>& Names)
	{
		Names.Sort([](const FName& Left, const FName& Right)
		{
			return Left.LexicalLess(Right);
		});
	}

	void ArchiveCorruptSaveFile(const FString& FilePath)
	{
		if (!FPaths::FileExists(FilePath))
		{
			return;
		}

		const FString ArchivePath = FilePath + TEXT(".corrupt-") +
			FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
		IFileManager::Get().Move(*ArchivePath, *FilePath, true, true, false, true);
	}
}

void UTunaSweeperAchievementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadAchievementDefinitions();
	LoadProgressState();
	if (auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Instance->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshGameplaySnapshot);
		Instance->OnWorldProgressChanged.AddUObject(this, &ThisClass::RefreshGameplaySnapshot);
	}

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UTunaSweeperAchievementSubsystem::HandlePostLoadMapWithWorld);
	if (GEngine)
		TravelFailureHandle = GEngine->OnTravelFailure().AddWeakLambda(this,
			[this](UWorld* World, ETravelFailure::Type, const FString&) { DiscardPendingExtraction(World); });
	TryStartPlatformSync();
}

void UTunaSweeperAchievementSubsystem::Deinitialize()
{
	if (bProgressDirty && bProgressStateLoaded) SaveProgressState();
	if (GEngine) GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	if (auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Instance->OnInventoryStateChanged.RemoveAll(this);
		Instance->OnWorldProgressChanged.RemoveAll(this);
	}
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	Publisher.Reset();
	Super::Deinitialize();
}

bool UTunaSweeperAchievementSubsystem::LoadAchievementDefinitions(bool bForceReload)
{
	if (bDefinitionsLoaded && !bForceReload)
	{
		return true;
	}

	FString JsonText;
	const FString DefinitionsPath = GetDefinitionsPath();
	if (!FFileHelper::LoadFileToString(JsonText, *DefinitionsPath))
	{
		UE_LOG(LogTunaSweeperAchievement, Error, TEXT("Failed to read achievement definitions: %s"), *DefinitionsPath);
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonText), RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTunaSweeperAchievement, Error, TEXT("Invalid achievement definitions JSON: %s"), *DefinitionsPath);
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* AchievementValues = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("achievements"), AchievementValues) || !AchievementValues)
	{
		UE_LOG(LogTunaSweeperAchievement, Error, TEXT("Achievement definitions require an achievements array: %s"), *DefinitionsPath);
		return false;
	}

	TArray<FTunaSweeperAchievementDefinition> LoadedDefinitions;
	for (const TSharedPtr<FJsonValue>& AchievementValue : *AchievementValues)
	{
		const TSharedPtr<FJsonObject> AchievementObject = AchievementValue.IsValid()
			? AchievementValue->AsObject()
			: nullptr;
		if (!AchievementObject.IsValid())
		{
			UE_LOG(LogTunaSweeperAchievement, Error, TEXT("Achievement definition entry must be an object"));
			return false;
		}

		FTunaSweeperAchievementDefinition Definition;
		Definition.AchievementId = ReadNameField(AchievementObject, TEXT("achievement_id"));
		Definition.NameStringKey = ReadNameField(AchievementObject, TEXT("name_string_key"));
		AchievementObject->TryGetBoolField(TEXT("enabled"), Definition.bEnabled);
		AchievementObject->TryGetNumberField(TEXT("target_item_id"), Definition.TargetItemId);
		for (const TCHAR* Field : {TEXT("required_facility_ids"), TEXT("map_paths")})
		{
			const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
			if (!AchievementObject->TryGetArrayField(Field, Values)) continue;
			TArray<FName>& Destination = FCString::Strcmp(Field, TEXT("map_paths")) == 0 ? Definition.MapPaths : Definition.RequiredFacilityIds;
			for (const auto& Value : *Values)
			{
				FString Id;
				if (!Value->TryGetString(Id) || Id.TrimStartAndEnd().IsEmpty()) return false;
				Destination.Add(FName(*Id));
			}
		}

		FString ConditionType;
		if (!AchievementObject->TryGetStringField(TEXT("condition_type"), ConditionType) ||
			!ParseConditionType(ConditionType, Definition.ConditionType))
		{
			UE_LOG(
				LogTunaSweeperAchievement,
				Error,
				TEXT("Unknown achievement condition_type for %s: %s"),
				*Definition.AchievementId.ToString(),
				*ConditionType);
			return false;
		}

		if (Definition.ConditionType == ETunaSweeperAchievementConditionType::GameplayEvent)
		{
			FString EventName;
			AchievementObject->TryGetStringField(TEXT("event_type"), EventName);
			const int64 Value = StaticEnum<ETunaSweeperAchievementEvent>()->GetValueByNameString(EventName);
			if (Value < 0 || Value > static_cast<int64>(ETunaSweeperAchievementEvent::EndingReached)) return false;
			Definition.EventType = static_cast<ETunaSweeperAchievementEvent>(Value);
		}

		if (Definition.ConditionType == ETunaSweeperAchievementConditionType::QuestRewardClaimed)
			Definition.TargetQuestId = TunaSweeperQuestId::Read(AchievementObject, TEXT("target_id"));
		else
			Definition.TargetId = ReadNameField(AchievementObject, TEXT("target_id"));

		double RequiredCount = 1.0;
		AchievementObject->TryGetNumberField(TEXT("required_count"), RequiredCount);
		if (!FMath::IsFinite(RequiredCount) || RequiredCount < 1 || RequiredCount > MAX_int32 || FMath::FloorToDouble(RequiredCount) != RequiredCount) return false;
		Definition.RequiredCount = FMath::RoundToInt64(RequiredCount);

		const TSharedPtr<FJsonObject>* PlatformIdsObject = nullptr;
		if (AchievementObject->TryGetObjectField(TEXT("platform_ids"), PlatformIdsObject) &&
			PlatformIdsObject && PlatformIdsObject->IsValid())
		{
			for (const TPair<FString, TSharedPtr<FJsonValue>>& PlatformPair : (*PlatformIdsObject)->Values)
			{
				FString PlatformId;
				if (PlatformPair.Value.IsValid() && PlatformPair.Value->TryGetString(PlatformId))
				{
					Definition.PlatformIds.Add(
						FName(*PlatformPair.Key.TrimStartAndEnd()),
						PlatformId.TrimStartAndEnd());
				}
			}
		}

		LoadedDefinitions.Add(MoveTemp(Definition));
	}

	FString ValidationError;
	if (!TunaSweeperAchievementModel::ValidateDefinitions(LoadedDefinitions, ValidationError))
	{
		UE_LOG(LogTunaSweeperAchievement, Error, TEXT("Invalid achievement definitions: %s"), *ValidationError);
		return false;
	}

	Definitions = MoveTemp(LoadedDefinitions);
	bDefinitionsLoaded = true;
	UE_LOG(LogTunaSweeperAchievement, Log, TEXT("Loaded %d achievement definitions"), Definitions.Num());

	if (bProgressStateLoaded)
	{
		ProcessProgressChanged(true);

		bPlatformQueryComplete = false;
		AttemptedPlatformWrites.Reset();
		TryStartPlatformSync();
	}

	return true;
}

void UTunaSweeperAchievementSubsystem::ReportEnemyKilled(FName EnemyId)
{
	// Practice events must never reach the separate achievement save or platform publisher.
	if (!IsGameplayReportingAllowed()) return;
	RaidState.RecordKill();
	ProcessProgressChanged(TunaSweeperAchievementModel::RecordEnemyKilled(ProgressState, EnemyId));
}

void UTunaSweeperAchievementSubsystem::ReportLocationReached(FName LocationId)
{
	// Practice events must never reach the separate achievement save or platform publisher.
	if (!IsGameplayReportingAllowed()) return;
	ProcessProgressChanged(TunaSweeperAchievementModel::RecordLocationReached(ProgressState, LocationId));
}

void UTunaSweeperAchievementSubsystem::ReportQuestRewardClaimed(int32 QuestId)
{
	// Practice events must never reach the separate achievement save or platform publisher.
	if (!IsGameplayReportingAllowed()) return;
	ProcessProgressChanged(TunaSweeperAchievementModel::RecordQuestRewardClaimed(ProgressState, QuestId));
}

void UTunaSweeperAchievementSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!LoadedWorld || LoadedWorld->GetGameInstance() != GetGameInstance())
	{
		return;
	}
	HandleGameplayWorldReady(LoadedWorld);
	FlushPendingProgress();

	if (bPlatformQueryComplete)
	{
		AttemptedPlatformWrites.Reset();
		PublishNextPendingAchievement();
	}
	else
	{
		TryStartPlatformSync();
	}
}

bool UTunaSweeperAchievementSubsystem::LoadProgressState()
{
	if (bProgressStateLoaded)
	{
		return true;
	}

	const FName ExpectedFlavor = TunaSweeperBuildFlavor::GetName();
	const FName ExpectedNamespace = GetDistributionNamespace();
	const FString SavePath = GetProgressSavePath();
	const FString PreviousPath = TunaSweeperSafeSave::GetPreviousFilePath(SavePath);
	const bool bHadSaveArtifacts = FPaths::FileExists(SavePath) || FPaths::FileExists(PreviousPath);
	const TunaSweeperSafeSave::FSaveValidator Validator =
		[ExpectedFlavor, ExpectedNamespace](const USaveGame& Candidate)
		{
			return TunaSweeperAchievementModel::IsSupportedSave(Candidate, ExpectedFlavor, ExpectedNamespace);
		};

	FString RecoveryPath;
	UTunaSweeperAchievementSaveGame* SaveGame = Cast<UTunaSweeperAchievementSaveGame>(
		TunaSweeperSafeSave::LoadSaveFileWithRecovery(
			SavePath,
			{ PreviousPath },
			Validator,
			&RecoveryPath));

	if (!SaveGame && bHadSaveArtifacts)
	{
		UE_LOG(LogTunaSweeperAchievement, Error, TEXT("Achievement progress save is corrupt; archiving it: %s"), *SavePath);
		ArchiveCorruptSaveFile(SavePath);
		ArchiveCorruptSaveFile(PreviousPath);
	}

	if (SaveGame)
	{
		TunaSweeperAchievementModel::RestoreProgress(*SaveGame, ProgressState);
	}

	bProgressStateLoaded = true;
	TArray<FName> NewlyUnlockedIds;
	TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, ProgressState, NewlyUnlockedIds);
	if (!NewlyUnlockedIds.IsEmpty())
	{
		PendingNotifications.Append(NewlyUnlockedIds);
		bProgressDirty = true;
		FlushPendingProgress();
	}

	if (!RecoveryPath.IsEmpty())
	{
		UE_LOG(LogTunaSweeperAchievement, Warning, TEXT("Recovered achievement progress from: %s"), *RecoveryPath);
	}
	return true;
}

bool UTunaSweeperAchievementSubsystem::SaveProgressState() const
{
	UTunaSweeperAchievementSaveGame* SaveGame = NewObject<UTunaSweeperAchievementSaveGame>();
	SaveGame->SaveVersion = AchievementSaveVersion;
	SaveGame->BuildFlavor = TunaSweeperBuildFlavor::GetName();
	SaveGame->DistributionNamespace = GetDistributionNamespace();
	TunaSweeperAchievementModel::CaptureProgress(ProgressState, *SaveGame);
	SortNames(SaveGame->KilledEnemyIds);
	SortNames(SaveGame->ReachedLocationIds);
	SaveGame->NumericClaimedQuestIds.Sort();
	SortNames(SaveGame->UnlockedAchievementIds);
	SaveGame->ConfirmedPlatformUnlockKeys.Sort();

	const FName ExpectedFlavor = SaveGame->BuildFlavor;
	const FName ExpectedNamespace = SaveGame->DistributionNamespace;
	const TunaSweeperSafeSave::FSaveValidator Validator =
		[ExpectedFlavor, ExpectedNamespace](const USaveGame& Candidate)
		{
			return TunaSweeperAchievementModel::IsSupportedSave(Candidate, ExpectedFlavor, ExpectedNamespace);
		};

	const bool bSaved = TunaSweeperSafeSave::SaveGameFileFailClosed(
		SaveGame,
		GetProgressSavePath(),
		Validator);
	if (!bSaved)
	{
		UE_LOG(LogTunaSweeperAchievement, Error, TEXT("Failed to save achievement progress"));
	}
	return bSaved;
}

void UTunaSweeperAchievementSubsystem::ProcessProgressChanged(bool bStateChanged)
{
	if (!bStateChanged && !bProgressDirty)
	{
		return;
	}

	TArray<FName> NewlyUnlockedIds;
	TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, ProgressState, NewlyUnlockedIds, GameplaySnapshot);
	PendingNotifications.Append(NewlyUnlockedIds);
	bProgressDirty = bProgressDirty || bStateChanged || !NewlyUnlockedIds.IsEmpty();
	FlushPendingProgress();
	if (bProgressDirty) return;

	if (bPlatformQueryComplete)
	{
		PublishNextPendingAchievement();
	}
	else
	{
		TryStartPlatformSync();
	}
}

void UTunaSweeperAchievementSubsystem::FlushPendingProgress()
{
	if (!bProgressDirty || !bProgressStateLoaded || !SaveProgressState()) return;
	bProgressDirty = false;
	TArray<FName> Notifications = MoveTemp(PendingNotifications);
	PendingNotifications.Reset();
	BroadcastUnlocked(Notifications);
}

void UTunaSweeperAchievementSubsystem::BroadcastUnlocked(const TArray<FName>& AchievementIds)
{
	for (const FName AchievementId : AchievementIds)
	{
		UE_LOG(LogTunaSweeperAchievement, Log, TEXT("Achievement unlocked locally: %s"), *AchievementId.ToString());
		OnAchievementUnlocked.Broadcast(AchievementId);
		const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
		const auto* Text = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTunaSweeperTextSubsystem>() : nullptr;
		auto* Toast = GetGameInstance() ? GetGameInstance()->GetSubsystem<UTunaSweeperToastSubsystem>() : nullptr;
		FText Format;
		const FText Name = GetAchievementName(AchievementId);
		if (Instance && Text && Toast && !Name.IsEmpty() &&
			Text->TryGetTextByKey(TEXT("ui.achievement.unlocked"), Instance->GetCurrentTextLanguage(), Format))
			Toast->ShowToast(FText::Format(Format, Name));
	}
}

void UTunaSweeperAchievementSubsystem::TryStartPlatformSync()
{
	if (bPlatformQueryInFlight || bPlatformWriteInFlight || !bDefinitionsLoaded || Definitions.IsEmpty())
	{
		return;
	}

	if (!Publisher.IsValid())
	{
		Publisher = MakeTunaSweeperOnlineAchievementPublisher();
	}
	if (!Publisher.IsValid() || !Publisher->IsAvailable())
	{
		return;
	}

	CurrentPlatformName = Publisher->GetPlatformName();
	if (!HasPlatformMappings(CurrentPlatformName) || !ValidatePlatformConfiguration(CurrentPlatformName))
	{
		return;
	}

	bPlatformQueryInFlight = true;
	TWeakObjectPtr<UTunaSweeperAchievementSubsystem> WeakThis(this);
	if (!Publisher->QueryUnlockedAchievements(
		[WeakThis](bool bSuccess, TSet<FString> UnlockedPlatformIds)
		{
			if (UTunaSweeperAchievementSubsystem* Subsystem = WeakThis.Get())
			{
				Subsystem->HandlePlatformQueryComplete(bSuccess, MoveTemp(UnlockedPlatformIds));
			}
		}))
	{
		bPlatformQueryInFlight = false;
	}
}

void UTunaSweeperAchievementSubsystem::HandlePlatformQueryComplete(
	bool bSuccess,
	TSet<FString> UnlockedPlatformIds)
{
	bPlatformQueryInFlight = false;
	if (!bSuccess)
	{
		UE_LOG(LogTunaSweeperAchievement, Warning, TEXT("Platform achievement query failed; unlocks remain pending"));
		return;
	}

	bPlatformQueryComplete = true;
	AttemptedPlatformWrites.Reset();
	TArray<FName> RemotelyUnlockedInternalIds;
	if (TunaSweeperAchievementModel::MergePlatformState(
		Definitions,
		CurrentPlatformName,
		UnlockedPlatformIds,
		ProgressState,
		RemotelyUnlockedInternalIds))
	{
		bProgressDirty = true;
	}
	for (FName Id : RemotelyUnlockedInternalIds) PendingNotifications.AddUnique(Id);
	FlushPendingProgress();
	PublishNextPendingAchievement();
}

void UTunaSweeperAchievementSubsystem::PublishNextPendingAchievement()
{
	if (bProgressDirty || !bPlatformQueryComplete || bPlatformWriteInFlight || !Publisher.IsValid())
	{
		return;
	}

	FName InternalId;
	FString PlatformId;
	if (TunaSweeperAchievementModel::FindNextPendingUnlock(
		Definitions,
		ProgressState,
		CurrentPlatformName,
		AttemptedPlatformWrites,
		InternalId,
		PlatformId))
	{
		AttemptedPlatformWrites.Add(InternalId);
		bPlatformWriteInFlight = true;
		const FString PlatformIdCopy = PlatformId;
		TWeakObjectPtr<UTunaSweeperAchievementSubsystem> WeakThis(this);
		if (!Publisher->UnlockAchievement(
			PlatformIdCopy,
			[WeakThis, InternalId, PlatformIdCopy](bool bSuccess)
			{
				UTunaSweeperAchievementSubsystem* Subsystem = WeakThis.Get();
				if (!Subsystem)
				{
					return;
				}

				Subsystem->bPlatformWriteInFlight = false;
				if (bSuccess)
				{
					Subsystem->ProgressState.ConfirmedPlatformUnlockKeys.Add(
						TunaSweeperAchievementModel::MakePlatformUnlockKey(
							Subsystem->CurrentPlatformName,
							PlatformIdCopy));
					Subsystem->bProgressDirty = true;
					Subsystem->FlushPendingProgress();
					UE_LOG(
						LogTunaSweeperAchievement,
						Log,
						TEXT("Published achievement %s as %s"),
						*InternalId.ToString(),
						*PlatformIdCopy);
				}
				else
				{
					UE_LOG(
						LogTunaSweeperAchievement,
						Warning,
						TEXT("Failed to publish achievement %s; it remains pending"),
						*InternalId.ToString());
				}
				Subsystem->PublishNextPendingAchievement();
			}))
		{
			bPlatformWriteInFlight = false;
		}
	}
}

bool UTunaSweeperAchievementSubsystem::HasPlatformMappings(FName PlatformName) const
{
	for (const FTunaSweeperAchievementDefinition& Definition : Definitions)
	{
		if (!TunaSweeperAchievementModel::CanUnlock(Definition)) continue;
		if (Definition.PlatformIds.Contains(PlatformName))
		{
			return true;
		}
	}
	return false;
}

bool UTunaSweeperAchievementSubsystem::ValidatePlatformConfiguration(FName PlatformName) const
{
	if (PlatformName != FName(TEXT("Steam")))
	{
		return true;
	}

	TArray<FString> ConfiguredIds;
	FString ValidationError;
	if (!ReadSteamConfiguredAchievementIds(ConfiguredIds, ValidationError) ||
		!TunaSweeperAchievementModel::ValidateConfiguredPlatformIds(
			Definitions,
			PlatformName,
			ConfiguredIds,
			ValidationError))
	{
		UE_LOG(LogTunaSweeperAchievement, Error, TEXT("Steam achievement configuration mismatch: %s"), *ValidationError);
		return false;
	}
	return true;
}

bool UTunaSweeperAchievementSubsystem::ReadSteamConfiguredAchievementIds(
	TArray<FString>& OutIds,
	FString& OutError) const
{
	OutIds.Reset();
	OutError.Reset();
	bool bEncounteredGap = false;
	for (int32 Index = 0; Index < MaximumConfiguredSteamAchievements; ++Index)
	{
		FString Id;
		GConfig->GetString(
			TEXT("OnlineSubsystemSteam"),
			*FString::Printf(TEXT("Achievement_%d_Id"), Index),
			Id,
			GEngineIni);
		Id.TrimStartAndEndInline();
		if (Id.IsEmpty())
		{
			bEncounteredGap = true;
			continue;
		}
		if (bEncounteredGap)
		{
			OutError = FString::Printf(TEXT("Achievement_%d_Id appears after a missing index"), Index);
			return false;
		}
		OutIds.Add(Id);
	}
	return true;
}

const FTunaSweeperAchievementDefinition* UTunaSweeperAchievementSubsystem::FindDefinition(FName AchievementId) const
{
	return Definitions.FindByPredicate([AchievementId](const FTunaSweeperAchievementDefinition& Definition)
	{
		return Definition.AchievementId == AchievementId;
	});
}

FString UTunaSweeperAchievementSubsystem::GetDefinitionsPath() const
{
	return FPaths::ConvertRelativePathToFull(FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("Data"),
		TEXT("AchievementDefinitions.json")));
}

FName UTunaSweeperAchievementSubsystem::GetDistributionNamespace() const
{
	FString Namespace;
	GConfig->GetString(AchievementConfigSection, TEXT("DistributionNamespace"), Namespace, GEngineIni);
	Namespace.TrimStartAndEndInline();
	return Namespace.IsEmpty() ? FName(TEXT("Local")) : FName(*Namespace);
}

FString UTunaSweeperAchievementSubsystem::GetProgressSavePath() const
{
	const FString FileName = FString::Printf(
		TEXT("Achievements_%s.sav"),
		*FPaths::MakeValidFileName(GetDistributionNamespace().ToString()));
	return FPaths::Combine(TunaSweeperBuildFlavor::GetSaveGameDirectory(), FileName);
}
