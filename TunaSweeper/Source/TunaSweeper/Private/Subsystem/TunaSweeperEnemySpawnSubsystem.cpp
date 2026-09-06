#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"

#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/TunaSweeperBuildFlavor.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperEnemySpawn, Log, All);

namespace TunaSweeperEnemySpawn
{
	const TCHAR* EnemyCombatProfilesJsonRelativePath = TEXT("Data/EnemyCombatProfiles.json");
	const FName DefaultEnemyCombatProfileId(TEXT("enemy.rifle_anchor"));
	const TCHAR* DefaultLootContainerClassPath = TEXT("/Game/Interaction/BP_LootContainer.BP_LootContainer_C");

	float ReadNonNegativeFloatField(
		const TSharedPtr<FJsonObject>& JsonObject,
		const TCHAR* FieldName,
		float DefaultValue)
	{
		double NumericValue = DefaultValue;
		JsonObject->TryGetNumberField(FieldName, NumericValue);
		return FMath::Max(0.0f, static_cast<float>(NumericValue));
	}

	int32 ReadNonNegativeIntField(
		const TSharedPtr<FJsonObject>& JsonObject,
		const TCHAR* FieldName,
		int32 DefaultValue)
	{
		double NumericValue = DefaultValue;
		JsonObject->TryGetNumberField(FieldName, NumericValue);
		return FMath::Max(0, FMath::RoundToInt(NumericValue));
	}

	bool TryResolveEnemyAttackMode(const FString& AttackModeString, ETunaSweeperEnemyAttackMode& OutAttackMode)
	{
		const FString NormalizedAttackMode = AttackModeString.TrimStartAndEnd().ToLower();
		if (NormalizedAttackMode == TEXT("ranged"))
		{
			OutAttackMode = ETunaSweeperEnemyAttackMode::Ranged;
			return true;
		}
		if (NormalizedAttackMode == TEXT("melee"))
		{
			OutAttackMode = ETunaSweeperEnemyAttackMode::Melee;
			return true;
		}
		return false;
	}

	bool TryResolveEnemyCombatRole(const FString& CombatRoleString, ETunaSweeperEnemyCombatRole& OutCombatRole)
	{
		const FString NormalizedRole = CombatRoleString.TrimStartAndEnd().ToLower();
		if (NormalizedRole == TEXT("anchor"))
		{
			OutCombatRole = ETunaSweeperEnemyCombatRole::Anchor;
			return true;
		}
		if (NormalizedRole == TEXT("flanker"))
		{
			OutCombatRole = ETunaSweeperEnemyCombatRole::Flanker;
			return true;
		}
		if (NormalizedRole == TEXT("melee"))
		{
			OutCombatRole = ETunaSweeperEnemyCombatRole::Melee;
			return true;
		}
		return false;
	}

	FString NormalizeLevelName(const FString& RawLevelName)
	{
		FString LevelName = FPackageName::GetShortName(RawLevelName);
		if (LevelName.StartsWith(TEXT("UEDPIE_")))
		{
			const int32 SearchStart = FString(TEXT("UEDPIE_")).Len();
			const int32 SecondUnderscoreIndex = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
			if (SecondUnderscoreIndex != INDEX_NONE)
			{
				LevelName = LevelName.Mid(SecondUnderscoreIndex + 1);
			}
		}
		return LevelName;
	}

	bool TryReadVectorField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, FVector& OutVector)
	{
		const TArray<TSharedPtr<FJsonValue>>* VectorArray = nullptr;
		if (!JsonObject.IsValid() || !JsonObject->TryGetArrayField(FieldName, VectorArray) || !VectorArray || VectorArray->Num() < 3)
		{
			return false;
		}
		OutVector = FVector(
			static_cast<float>((*VectorArray)[0]->AsNumber()),
			static_cast<float>((*VectorArray)[1]->AsNumber()),
			static_cast<float>((*VectorArray)[2]->AsNumber()));
		return true;
	}

	bool TryReadRotatorField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, FRotator& OutRotator)
	{
		FVector RotationVector = FVector::ZeroVector;
		if (!TryReadVectorField(JsonObject, FieldName, RotationVector))
		{
			return false;
		}
		OutRotator = FRotator(RotationVector.X, RotationVector.Y, RotationVector.Z);
		return true;
	}

	bool ShouldIncludeEditorOnlySpawn(const UWorld* World)
	{
#if WITH_EDITOR
		return World && World->IsGameWorld();
#else
		(void)World;
		return false;
#endif
	}
}

void UTunaSweeperEnemySpawnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UTunaSweeperEnemySpawnSubsystem::HandlePostLoadMapWithWorld);
}

void UTunaSweeperEnemySpawnSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	ResetLoadedEnemyCombatProfileData();
	ResetLoadedLootContainerSpawnData();
	LastLootSpawnedWorld.Reset();
	Super::Deinitialize();
}

bool UTunaSweeperEnemySpawnSubsystem::EnsureLootContainersSpawnedForWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld() || LastLootSpawnedWorld.Get() == World)
	{
		return true;
	}
	if (!LoadLootContainerSpawnData(false))
	{
		return false;
	}

	LastLootSpawnedWorld = World;
	int32 SpawnedLootContainerCount = 0;
	for (const FLootContainerSpawnDefinition& SpawnDefinition : LootContainerSpawnDefinitions)
	{
		if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World) ||
			(SpawnDefinition.bEditorOnly && !TunaSweeperEnemySpawn::ShouldIncludeEditorOnlySpawn(World)))
		{
			continue;
		}

		TSubclassOf<ATunaSweeperLootContainerActor> LoadedContainerClass = SpawnDefinition.LootContainerClass.LoadSynchronous();
		if (!LoadedContainerClass)
		{
			LoadedContainerClass = ATunaSweeperLootContainerActor::StaticClass();
		}
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATunaSweeperLootContainerActor* SpawnedContainer = World->SpawnActor<ATunaSweeperLootContainerActor>(
			LoadedContainerClass,
			SpawnDefinition.Location,
			SpawnDefinition.Rotation,
			SpawnParameters);
		if (SpawnedContainer)
		{
			SpawnedContainer->SetContainerDataIds(SpawnDefinition.ContainerDefinitionId, SpawnDefinition.ContentsId);
			++SpawnedLootContainerCount;
		}
	}

	UE_LOG(LogTunaSweeperEnemySpawn, Log, TEXT("Spawned %d coordinate-authored loot containers for level %s."), SpawnedLootContainerCount, *World->GetMapName());
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadEnemyCombatProfileData(bool bForceReload)
{
	if (bEnemyCombatProfileDataLoaded && !bForceReload)
	{
		return true;
	}
	ResetLoadedEnemyCombatProfileData();

	FString JsonContent;
	const FString EnemyCombatProfileJsonPath = GetEnemyCombatProfileJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *EnemyCombatProfileJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read enemy combat profile JSON: %s"), *EnemyCombatProfileJsonPath);
		return false;
	}
	TArray<TSharedPtr<FJsonValue>> JsonRows;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonContent), JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse enemy combat profile JSON: %s"), *EnemyCombatProfileJsonPath);
		return false;
	}

	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) || !JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy combat profile row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString ProfileIdString;
		FString AttackModeString;
		FString CombatRoleString;
		if (!JsonObject->TryGetStringField(TEXT("profile_id"), ProfileIdString) ||
			!JsonObject->TryGetStringField(TEXT("attack_mode"), AttackModeString) ||
			!JsonObject->TryGetStringField(TEXT("role"), CombatRoleString))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy combat profile row %d: required field is missing."), RowIndex);
			continue;
		}

		FTunaSweeperEnemyCombatProfile Profile;
		Profile.ProfileId = FName(*ProfileIdString.TrimStartAndEnd());
		if (Profile.ProfileId.IsNone() ||
			!TunaSweeperEnemySpawn::TryResolveEnemyAttackMode(AttackModeString, Profile.AttackMode) ||
			!TunaSweeperEnemySpawn::TryResolveEnemyCombatRole(CombatRoleString, Profile.Role) ||
			EnemyCombatProfilesById.Contains(Profile.ProfileId) ||
			((Profile.AttackMode == ETunaSweeperEnemyAttackMode::Melee) != (Profile.Role == ETunaSweeperEnemyCombatRole::Melee)))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy combat profile row %d: identifiers, role, or attack mode are invalid."), RowIndex);
			continue;
		}

		Profile.MovementSpeed = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("movement_speed"), Profile.MovementSpeed);
		Profile.TrackingRange = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("tracking_range"), Profile.TrackingRange);
		Profile.PreferredRangeMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("preferred_range_min"), Profile.PreferredRangeMin);
		Profile.PreferredRangeMax = FMath::Max(Profile.PreferredRangeMin, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("preferred_range_max"), Profile.PreferredRangeMax));
		Profile.DangerRange = FMath::Min(Profile.PreferredRangeMin, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("danger_range"), Profile.DangerRange));
		Profile.AlertSeconds = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("alert_seconds"), Profile.AlertSeconds);
		Profile.AimSecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("aim_seconds_min"), Profile.AimSecondsMin);
		Profile.AimSecondsMax = FMath::Max(Profile.AimSecondsMin, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("aim_seconds_max"), Profile.AimSecondsMax));
		Profile.TurnSpeedDegreesPerSecond = FMath::Max(1.0f, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("turn_speed_degrees_per_second"), Profile.TurnSpeedDegreesPerSecond));
		Profile.AttackFacingToleranceDegrees = FMath::Clamp(TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("attack_facing_tolerance_degrees"), Profile.AttackFacingToleranceDegrees), 0.0f, 90.0f);
		Profile.WeaponSpreadMultiplier = FMath::Max(0.01f, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("weapon_spread_multiplier"), Profile.WeaponSpreadMultiplier));
		Profile.FiringShotCount = TunaSweeperEnemySpawn::ReadNonNegativeIntField(JsonObject, TEXT("firing_shot_count"), Profile.FiringShotCount);
		Profile.OpeningFiringShotCount = TunaSweeperEnemySpawn::ReadNonNegativeIntField(JsonObject, TEXT("opening_firing_shot_count"), Profile.OpeningFiringShotCount);
		Profile.ShotIntervalSecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("shot_interval_seconds_min"), Profile.ShotIntervalSecondsMin);
		Profile.ShotIntervalSecondsMax = FMath::Max(Profile.ShotIntervalSecondsMin, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("shot_interval_seconds_max"), Profile.ShotIntervalSecondsMax));
		Profile.RecoverSecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("recover_seconds_min"), Profile.RecoverSecondsMin);
		Profile.RecoverSecondsMax = FMath::Max(Profile.RecoverSecondsMin, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("recover_seconds_max"), Profile.RecoverSecondsMax));
		Profile.ObserveSecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("observe_seconds_min"), Profile.ObserveSecondsMin);
		Profile.ObserveSecondsMax = FMath::Max(Profile.ObserveSecondsMin, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("observe_seconds_max"), Profile.ObserveSecondsMax));
		Profile.ReloadReadySecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("reload_ready_seconds_min"), Profile.ReloadReadySecondsMin);
		Profile.ReloadReadySecondsMax = FMath::Max(Profile.ReloadReadySecondsMin, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("reload_ready_seconds_max"), Profile.ReloadReadySecondsMax));
		Profile.PositionFiringBudgetMin = TunaSweeperEnemySpawn::ReadNonNegativeIntField(JsonObject, TEXT("position_firing_budget_min"), Profile.PositionFiringBudgetMin);
		Profile.PositionFiringBudgetMax = FMath::Max(Profile.PositionFiringBudgetMin, TunaSweeperEnemySpawn::ReadNonNegativeIntField(JsonObject, TEXT("position_firing_budget_max"), Profile.PositionFiringBudgetMax));
		Profile.RepositionDistanceMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("reposition_distance_min"), Profile.RepositionDistanceMin);
		Profile.RepositionDistanceMax = FMath::Max(Profile.RepositionDistanceMin, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("reposition_distance_max"), Profile.RepositionDistanceMax));
		Profile.CrossRepositionChance = FMath::Clamp(TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("cross_reposition_chance"), Profile.CrossRepositionChance), 0.0f, 1.0f);
		Profile.CrossRepositionCooldownSeconds = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("cross_reposition_cooldown_seconds"), Profile.CrossRepositionCooldownSeconds);
		Profile.CrossRepositionOrbitRadius = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("cross_reposition_orbit_radius"), Profile.CrossRepositionOrbitRadius);
		Profile.MeleeAttackDamage = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("melee_attack_damage"), Profile.MeleeAttackDamage);
		Profile.MeleeApproachStartRange = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("melee_approach_start_range"), Profile.MeleeApproachStartRange);
		Profile.MeleeApproachStopRange = FMath::Min(Profile.MeleeApproachStartRange, TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("melee_approach_stop_range"), Profile.MeleeApproachStopRange));
		Profile.AttackCooldownSeconds = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("attack_cooldown_seconds"), Profile.AttackCooldownSeconds);

		if (Profile.AttackMode == ETunaSweeperEnemyAttackMode::Ranged)
		{
			Profile.FiringShotCount = FMath::Max(1, Profile.FiringShotCount);
			Profile.OpeningFiringShotCount = FMath::Clamp(Profile.OpeningFiringShotCount, 1, Profile.FiringShotCount);
			Profile.PositionFiringBudgetMin = FMath::Max(1, Profile.PositionFiringBudgetMin);
			Profile.PositionFiringBudgetMax = FMath::Max(Profile.PositionFiringBudgetMin, Profile.PositionFiringBudgetMax);
		}
		else
		{
			Profile.FiringShotCount = 0;
			Profile.OpeningFiringShotCount = 0;
			Profile.PositionFiringBudgetMin = 0;
			Profile.PositionFiringBudgetMax = 0;
			Profile.CrossRepositionChance = 0.0f;
		}
		EnemyCombatProfilesById.Add(Profile.ProfileId, Profile);
	}

	if (!EnemyCombatProfilesById.Contains(TunaSweeperEnemySpawn::DefaultEnemyCombatProfileId))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Enemy combat profile JSON is missing required default profile '%s': %s"), *TunaSweeperEnemySpawn::DefaultEnemyCombatProfileId.ToString(), *EnemyCombatProfileJsonPath);
		ResetLoadedEnemyCombatProfileData();
		return false;
	}
	bEnemyCombatProfileDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::TryGetEnemyCombatProfile(FName ProfileId, FTunaSweeperEnemyCombatProfile& OutProfile)
{
	OutProfile = FTunaSweeperEnemyCombatProfile();
	if (ProfileId.IsNone() || !LoadEnemyCombatProfileData(false))
	{
		return false;
	}
	const FTunaSweeperEnemyCombatProfile* Profile = EnemyCombatProfilesById.Find(ProfileId);
	if (!Profile)
	{
		return false;
	}
	OutProfile = *Profile;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadLootContainerSpawnData(bool bForceReload)
{
	if (bLootContainerSpawnDataLoaded && !bForceReload)
	{
		return true;
	}
	ResetLoadedLootContainerSpawnData();

	FString JsonContent;
	const FString JsonPath = GetLootContainerSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read loot container spawn JSON: %s"), *JsonPath);
		return false;
	}
	TArray<TSharedPtr<FJsonValue>> Rows;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonContent), Rows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse loot container spawn JSON: %s"), *JsonPath);
		return false;
	}

	bool bHasValidRows = false;
	bool bHasAnchorRows = false;
	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!Rows[RowIndex].IsValid() || !Rows[RowIndex]->TryGetObject(JsonObjectPtr) || !JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping loot container spawn row %d: row is not an object."), RowIndex);
			continue;
		}
		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		if (JsonObject->HasField(TEXT("placement_id")))
		{
			if (JsonObject->HasField(TEXT("location")) || JsonObject->HasField(TEXT("rotation")) || JsonObject->HasField(TEXT("scale")))
			{
				UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Loot spawn row %d mixes anchor placement_id with transform fields."), RowIndex);
				return false;
			}
			bHasAnchorRows = true;
			continue;
		}

		FString LevelName;
		FString ClassPath;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		bool bEditorOnly = false;
		double ContainerDefinitionId = INDEX_NONE;
		double ContentsId = INDEX_NONE;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location) ||
			!JsonObject->TryGetNumberField(TEXT("container_definition_id"), ContainerDefinitionId) ||
			!JsonObject->TryGetNumberField(TEXT("contents_id"), ContentsId))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping loot container spawn row %d: required field is missing."), RowIndex);
			continue;
		}
		JsonObject->TryGetStringField(TEXT("loot_container_class"), ClassPath);
		JsonObject->TryGetBoolField(TEXT("editor_only"), bEditorOnly);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);

		FLootContainerSpawnDefinition Definition;
		Definition.LevelName = FName(*LevelName.TrimStartAndEnd());
		Definition.LootContainerClass = TSoftClassPtr<ATunaSweeperLootContainerActor>(FSoftObjectPath(
			ClassPath.TrimStartAndEnd().IsEmpty() ? FString(TunaSweeperEnemySpawn::DefaultLootContainerClassPath) : ClassPath.TrimStartAndEnd()));
		Definition.Location = Location;
		Definition.Rotation = Rotation;
		Definition.ContainerDefinitionId = FMath::RoundToInt(ContainerDefinitionId);
		Definition.ContentsId = FMath::RoundToInt(ContentsId);
		Definition.bEditorOnly = bEditorOnly;
		if (Definition.LevelName.IsNone() || Definition.ContainerDefinitionId <= 0 || Definition.ContentsId <= 0)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping loot container spawn row %d: identifiers are invalid."), RowIndex);
			continue;
		}
		LootContainerSpawnDefinitions.Add(Definition);
		bHasValidRows = true;
	}

	if (!bHasValidRows && !bHasAnchorRows && Rows.Num() > 0)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Loot container spawn JSON has no valid rows: %s"), *JsonPath);
		return false;
	}
	bLootContainerSpawnDataLoaded = true;
	return true;
}

void UTunaSweeperEnemySpawnSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	EnsureLootContainersSpawnedForWorld(LoadedWorld);
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedEnemyCombatProfileData()
{
	EnemyCombatProfilesById.Reset();
	bEnemyCombatProfileDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedLootContainerSpawnData()
{
	LootContainerSpawnDefinitions.Reset();
	bLootContainerSpawnDataLoaded = false;
}

FString UTunaSweeperEnemySpawnSubsystem::GetEnemyCombatProfileJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::EnemyCombatProfilesJsonRelativePath);
}

FString UTunaSweeperEnemySpawnSubsystem::GetLootContainerSpawnJsonPath() const
{
	return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("LootContainerSpawns.json"));
}

bool UTunaSweeperEnemySpawnSubsystem::DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const
{
	if (!World || LevelName.IsNone())
	{
		return false;
	}
	const FString SpawnLevelName = TunaSweeperEnemySpawn::NormalizeLevelName(TunaSweeperBuildFlavor::ResolveGameplayLevelName(LevelName).ToString());
	const FString WorldMapName = TunaSweeperEnemySpawn::NormalizeLevelName(World->GetMapName());
	const FString WorldPackageName = TunaSweeperEnemySpawn::NormalizeLevelName(World->GetOutermost()->GetName());
	return SpawnLevelName.Equals(WorldMapName, ESearchCase::IgnoreCase) || SpawnLevelName.Equals(WorldPackageName, ESearchCase::IgnoreCase);
}
