#include "Subsystem/TunaSweeperRaidPlacementSubsystem.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperRaidPlacement, Log, All);

namespace TunaSweeperRaidPlacement
{
	const TCHAR* EnemySpawnProfilesFileName = TEXT("EnemySpawnProfiles.json");
	const TCHAR* EnemySpawnsFileName = TEXT("EnemySpawns.json");
	const TCHAR* LootContainerSpawnsFileName = TEXT("LootContainerSpawns.json");

	FString NormalizeLevelId(FName RawLevelId)
	{
		FString LevelId = FPackageName::GetShortName(RawLevelId.ToString());
		if (LevelId.StartsWith(TEXT("UEDPIE_")))
		{
			const int32 SecondUnderscore = LevelId.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 7);
			if (SecondUnderscore != INDEX_NONE)
			{
				LevelId.RightChopInline(SecondUnderscore + 1);
			}
		}
		return LevelId;
	}

	bool TryGetPlacementId(const TSharedPtr<FJsonObject>& JsonObject, int32& OutPlacementId)
	{
		double NumericPlacementId = 0.0;
		if (!JsonObject.IsValid() || !JsonObject->TryGetNumberField(TEXT("placement_id"), NumericPlacementId))
		{
			return false;
		}
		OutPlacementId = FMath::RoundToInt(NumericPlacementId);
		return true;
	}

	float ReadSpawnChance(const TSharedPtr<FJsonObject>& JsonObject)
	{
		double NumericChance = 1.0;
		JsonObject->TryGetNumberField(TEXT("spawn_chance"), NumericChance);
		return FMath::Clamp(static_cast<float>(NumericChance), 0.0f, 1.0f);
	}

	FName ReadOptionalName(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		FString Value;
		return JsonObject->TryGetStringField(FieldName, Value) && !Value.TrimStartAndEnd().IsEmpty()
			? FName(*Value.TrimStartAndEnd())
			: NAME_None;
	}

	uint32 Mix(uint32 Value)
	{
		Value ^= Value >> 16;
		Value *= 0x7FEB352Du;
		Value ^= Value >> 15;
		Value *= 0x846CA68Bu;
		Value ^= Value >> 16;
		return Value;
	}
}

void UTunaSweeperRaidPlacementSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UTunaSweeperRaidPlacementSubsystem::HandlePostLoadMapWithWorld);
}

void UTunaSweeperRaidPlacementSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}
	ResetLoadedData();
	LastSpawnedWorld.Reset();
	Super::Deinitialize();
}

void UTunaSweeperRaidPlacementSubsystem::SetRaidSeed(int32 InRaidSeed)
{
	if (RaidSeed != InRaidSeed)
	{
		RaidSeed = InRaidSeed;
		LastSpawnedWorld.Reset();
	}
	bRaidSeedExplicitlySet = true;
}

float UTunaSweeperRaidPlacementSubsystem::GetDeterministicPlacementRoll(int32 InRaidSeed, int32 PlacementId)
{
	const uint32 Combined = static_cast<uint32>(InRaidSeed) ^ (static_cast<uint32>(PlacementId) * 0x9E3779B9u);
	return static_cast<float>(TunaSweeperRaidPlacement::Mix(Combined) & 0x00FFFFFFu) / 16777216.0f;
}

bool UTunaSweeperRaidPlacementSubsystem::EnsureRaidPlacementActorsSpawnedForWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return true;
	}
	if (!TunaSweeperBuildFlavor::IsRaidGameplayLevelName(FName(*World->GetMapName())))
	{
		return true;
	}
	if (LastSpawnedWorld.Get() == World)
	{
		return true;
	}
	if (!LoadData(false))
	{
		return false;
	}

	if (!bRaidSeedExplicitlySet)
	{
		UE_LOG(LogTunaSweeperRaidPlacement, Warning, TEXT("RaidSeed was not set before loading %s; using deterministic fallback seed 0."), *World->GetMapName());
	}

	TMap<int32, ATunaSweeperRaidPlacementAnchor*> AnchorsByPlacementId;
	TSet<int32> InvalidPlacementIds;
	for (TActorIterator<ATunaSweeperRaidPlacementAnchor> It(World); It; ++It)
	{
		ATunaSweeperRaidPlacementAnchor* Anchor = *It;
		const int32 PlacementId = Anchor->GetPlacementId();
		if (PlacementId <= 0)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Raid anchor '%s' has invalid PlacementId %d."), *Anchor->GetPathName(), PlacementId);
			continue;
		}
		if (ATunaSweeperRaidPlacementAnchor** Existing = AnchorsByPlacementId.Find(PlacementId))
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Duplicate raid PlacementId %d in level %s: '%s' and '%s'."), PlacementId, *World->GetMapName(), *(*Existing)->GetPathName(), *Anchor->GetPathName());
			InvalidPlacementIds.Add(PlacementId);
			continue;
		}
		AnchorsByPlacementId.Add(PlacementId, Anchor);
	}
	for (int32 InvalidId : InvalidPlacementIds)
	{
		AnchorsByPlacementId.Remove(InvalidId);
	}

	TSet<int32> ConnectedPlacementIds;
	TMap<int32, ETunaSweeperRaidPlacementAnchorKind> DataKindsByPlacementId;
	int32 SpawnedEnemies = 0;
	int32 SpawnedLootContainers = 0;
	UTunaSweeperEnemySpawnSubsystem* LegacyEnemySubsystem = GetGameInstance()->GetSubsystem<UTunaSweeperEnemySpawnSubsystem>();

	for (const FEnemyPlacementDefinition& Placement : EnemyPlacementDefinitions)
	{
		if (!DoesLevelIdMatchWorld(Placement.LevelId, World))
		{
			continue;
		}
		if (const ETunaSweeperRaidPlacementAnchorKind* ExistingKind = DataKindsByPlacementId.Find(Placement.PlacementId))
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Level %s data reuses PlacementId %d for %s and Enemy."), *Placement.LevelId.ToString(), Placement.PlacementId, *UEnum::GetValueAsString(*ExistingKind));
			continue;
		}
		DataKindsByPlacementId.Add(Placement.PlacementId, ETunaSweeperRaidPlacementAnchorKind::Enemy);

		ATunaSweeperRaidPlacementAnchor* const* Anchor = AnchorsByPlacementId.Find(Placement.PlacementId);
		if (!Anchor)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy placement %s/%d has no level anchor."), *Placement.LevelId.ToString(), Placement.PlacementId);
			continue;
		}
		ConnectedPlacementIds.Add(Placement.PlacementId);
		if ((*Anchor)->GetAnchorKind() != ETunaSweeperRaidPlacementAnchorKind::Enemy)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy placement %s/%d points to a %s anchor."), *Placement.LevelId.ToString(), Placement.PlacementId, *UEnum::GetValueAsString((*Anchor)->GetAnchorKind()));
			continue;
		}
		const FEnemySpawnProfile* Profile = EnemyProfilesById.Find(Placement.ProfileId);
		if (!Profile)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy placement %s/%d references missing ProfileId '%s'."), *Placement.LevelId.ToString(), Placement.PlacementId, *Placement.ProfileId.ToString());
			continue;
		}
		if (!DoesSpawnConditionPass(Placement.ConditionId, FString::Printf(TEXT("enemy %s/%d"), *Placement.LevelId.ToString(), Placement.PlacementId)) || !ShouldSpawnAtPlacement(Placement.PlacementId, Placement.SpawnChance))
		{
			continue;
		}
		FTunaSweeperEnemyCombatProfile CombatProfile;
		if (!LegacyEnemySubsystem || !LegacyEnemySubsystem->TryGetEnemyCombatProfile(Profile->CombatProfileId, CombatProfile))
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy placement %s/%d cannot resolve combat ProfileId '%s'."), *Placement.LevelId.ToString(), Placement.PlacementId, *Profile->CombatProfileId.ToString());
			continue;
		}
		TSubclassOf<ATunaSweeperEnemyCharacter> EnemyClass = Profile->EnemyClass.LoadSynchronous();
		if (!EnemyClass)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy ProfileId '%s' cannot load its enemy class."), *Profile->ProfileId.ToString());
			continue;
		}
		const FTransform SpawnTransform = (*Anchor)->GetActorTransform();
		ATunaSweeperEnemyCharacter* SpawnedEnemy = World->SpawnActorDeferred<ATunaSweeperEnemyCharacter>(EnemyClass, SpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!SpawnedEnemy)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Failed to spawn enemy for %s/%d."), *Placement.LevelId.ToString(), Placement.PlacementId);
			continue;
		}
		SpawnedEnemy->ConfigureCombatProfile(CombatProfile, Profile->FactionId, Profile->SquadId, Profile->SquadSlot);
		SpawnedEnemy->ConfigureSpawnData(Profile->BodyMaterial, MakeRuntimeInstanceId(Placement.LevelId, Placement.PlacementId), Profile->DropContainerDefinitionId, Profile->DropContentsId, Profile->MaxHealth, Profile->ExperienceValue, Profile->BleedingChanceBonus, Profile->BleedingDurationBonusSeconds, Profile->WeaponItemId, Profile->AmmoItemId, Profile->ReserveAmmoCount, Profile->LootLoadedAmmoDeductionRatio, Profile->LootLoadedAmmoFlatDeduction);
		SpawnedEnemy->Tags.AddUnique(MakeRuntimeInstanceId(Placement.LevelId, Placement.PlacementId));
		UGameplayStatics::FinishSpawningActor(SpawnedEnemy, SpawnTransform);
		++SpawnedEnemies;
	}

	for (const FLootPlacementDefinition& Placement : LootPlacementDefinitions)
	{
		if (!DoesLevelIdMatchWorld(Placement.LevelId, World))
		{
			continue;
		}
		if (const ETunaSweeperRaidPlacementAnchorKind* ExistingKind = DataKindsByPlacementId.Find(Placement.PlacementId))
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Level %s data reuses PlacementId %d for %s and LootContainer."), *Placement.LevelId.ToString(), Placement.PlacementId, *UEnum::GetValueAsString(*ExistingKind));
			continue;
		}
		DataKindsByPlacementId.Add(Placement.PlacementId, ETunaSweeperRaidPlacementAnchorKind::LootContainer);

		ATunaSweeperRaidPlacementAnchor* const* Anchor = AnchorsByPlacementId.Find(Placement.PlacementId);
		if (!Anchor)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Loot placement %s/%d has no level anchor."), *Placement.LevelId.ToString(), Placement.PlacementId);
			continue;
		}
		ConnectedPlacementIds.Add(Placement.PlacementId);
		if ((*Anchor)->GetAnchorKind() != ETunaSweeperRaidPlacementAnchorKind::LootContainer)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Loot placement %s/%d points to a %s anchor."), *Placement.LevelId.ToString(), Placement.PlacementId, *UEnum::GetValueAsString((*Anchor)->GetAnchorKind()));
			continue;
		}
		if (!DoesSpawnConditionPass(Placement.ConditionId, FString::Printf(TEXT("loot %s/%d"), *Placement.LevelId.ToString(), Placement.PlacementId)) || !ShouldSpawnAtPlacement(Placement.PlacementId, Placement.SpawnChance))
		{
			continue;
		}
		TSubclassOf<ATunaSweeperLootContainerActor> ContainerClass = Placement.LootContainerClass.LoadSynchronous();
		if (!ContainerClass)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Loot placement %s/%d cannot load its container class."), *Placement.LevelId.ToString(), Placement.PlacementId);
			continue;
		}
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ATunaSweeperLootContainerActor* SpawnedContainer = World->SpawnActor<ATunaSweeperLootContainerActor>(ContainerClass, (*Anchor)->GetActorTransform(), SpawnParameters);
		if (!SpawnedContainer)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Failed to spawn loot container for %s/%d."), *Placement.LevelId.ToString(), Placement.PlacementId);
			continue;
		}
		SpawnedContainer->SetContainerDataIds(Placement.ContainerDefinitionId, Placement.ContentsId);
		SpawnedContainer->Tags.AddUnique(MakeRuntimeInstanceId(Placement.LevelId, Placement.PlacementId));
		++SpawnedLootContainers;
	}

	for (const TPair<int32, ATunaSweeperRaidPlacementAnchor*>& Pair : AnchorsByPlacementId)
	{
		if (!ConnectedPlacementIds.Contains(Pair.Key))
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Warning, TEXT("Raid anchor %s/%d (%s) has no connected external placement data."), *World->GetMapName(), Pair.Key, *UEnum::GetValueAsString(Pair.Value->GetAnchorKind()));
		}
	}

	LastSpawnedWorld = World;
	UE_LOG(LogTunaSweeperRaidPlacement, Log, TEXT("Resolved raid anchors for %s: enemies=%d loot_containers=%d seed=%d."), *World->GetMapName(), SpawnedEnemies, SpawnedLootContainers, RaidSeed);
	return true;
}

bool UTunaSweeperRaidPlacementSubsystem::LoadData(bool bForceReload)
{
	if (bDataLoaded && !bForceReload)
	{
		return true;
	}
	ResetLoadedData();
	const bool bProfilesLoaded = LoadEnemyProfiles(GetEnemyProfilesJsonPath());
	const bool bEnemyPlacementsLoaded = LoadEnemyPlacements(GetEnemyPlacementsJsonPath());
	const bool bLootPlacementsLoaded = LoadLootPlacements(GetLootPlacementsJsonPath());
	bDataLoaded = bProfilesLoaded && bEnemyPlacementsLoaded && bLootPlacementsLoaded;
	return bDataLoaded;
}

bool UTunaSweeperRaidPlacementSubsystem::LoadEnemyProfiles(const FString& JsonPath)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Failed to read enemy spawn profiles: %s"), *JsonPath);
		return false;
	}
	TArray<TSharedPtr<FJsonValue>> Rows;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonContent), Rows))
	{
		UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Failed to parse enemy spawn profiles: %s"), *JsonPath);
		return false;
	}
	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!Rows[RowIndex].IsValid() || !Rows[RowIndex]->TryGetObject(JsonObjectPtr) || !JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy profile row %d is not an object."), RowIndex);
			continue;
		}
		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString ProfileIdString;
		FString EnemyClassPath;
		FString BodyMaterialPath;
		FString CombatProfileIdString;
		if (!JsonObject->TryGetStringField(TEXT("profile_id"), ProfileIdString) || !JsonObject->TryGetStringField(TEXT("enemy_class"), EnemyClassPath) || !JsonObject->TryGetStringField(TEXT("combat_profile_id"), CombatProfileIdString))
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy profile row %d is missing profile_id, enemy_class, or combat_profile_id."), RowIndex);
			continue;
		}
		FEnemySpawnProfile Profile;
		Profile.ProfileId = FName(*ProfileIdString.TrimStartAndEnd());
		Profile.CombatProfileId = FName(*CombatProfileIdString.TrimStartAndEnd());
		JsonObject->TryGetStringField(TEXT("body_material"), BodyMaterialPath);
		Profile.EnemyClass = TSoftClassPtr<ATunaSweeperEnemyCharacter>(FSoftObjectPath(EnemyClassPath.TrimStartAndEnd()));
		if (!BodyMaterialPath.TrimStartAndEnd().IsEmpty()) { Profile.BodyMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(BodyMaterialPath.TrimStartAndEnd())); }
		double NumericValue = 0.0;
		if (JsonObject->TryGetNumberField(TEXT("drop_container_definition_id"), NumericValue)) { Profile.DropContainerDefinitionId = FMath::RoundToInt(NumericValue); }
		if (JsonObject->TryGetNumberField(TEXT("drop_contents_id"), NumericValue)) { Profile.DropContentsId = FMath::RoundToInt(NumericValue); }
		if (JsonObject->TryGetNumberField(TEXT("weapon_item_id"), NumericValue)) { Profile.WeaponItemId = FMath::RoundToInt(NumericValue); }
		if (JsonObject->TryGetNumberField(TEXT("ammo_item_id"), NumericValue)) { Profile.AmmoItemId = FMath::RoundToInt(NumericValue); }
		if (JsonObject->TryGetNumberField(TEXT("reserve_ammo_count"), NumericValue)) { Profile.ReserveAmmoCount = FMath::RoundToInt(NumericValue); }
		if (JsonObject->TryGetNumberField(TEXT("loot_loaded_ammo_deduction_ratio"), NumericValue)) { Profile.LootLoadedAmmoDeductionRatio = FMath::Clamp(static_cast<float>(NumericValue), 0.0f, 1.0f); }
		if (JsonObject->TryGetNumberField(TEXT("loot_loaded_ammo_flat_deduction"), NumericValue)) { Profile.LootLoadedAmmoFlatDeduction = FMath::Max(0, FMath::RoundToInt(NumericValue)); }
		if (JsonObject->TryGetNumberField(TEXT("experience_value"), NumericValue)) { Profile.ExperienceValue = FMath::Max(0, FMath::RoundToInt(NumericValue)); }
		if (JsonObject->TryGetNumberField(TEXT("max_health"), NumericValue)) { Profile.MaxHealth = FMath::Max(1.0f, static_cast<float>(NumericValue)); }
		if (JsonObject->TryGetNumberField(TEXT("bleeding_chance_bonus"), NumericValue)) { Profile.BleedingChanceBonus = TunaSweeperDataValues::ClampProbabilityValue(FMath::RoundToInt(NumericValue)); }
		if (JsonObject->TryGetNumberField(TEXT("bleeding_duration_bonus_seconds"), NumericValue)) { Profile.BleedingDurationBonusSeconds = FMath::Max(0.0f, static_cast<float>(NumericValue)); }
		if (JsonObject->TryGetNumberField(TEXT("faction_id"), NumericValue))
		{
			const int32 FactionId = FMath::RoundToInt(NumericValue);
			Profile.FactionId = (FactionId >= 1 && FactionId <= 254) || FactionId == TunaSweeperFactionIds::NoFaction ? static_cast<uint8>(FactionId) : TunaSweeperFactionIds::NoFaction;
		}
		Profile.SquadId = TunaSweeperRaidPlacement::ReadOptionalName(JsonObject, TEXT("squad_id"));
		if (JsonObject->TryGetNumberField(TEXT("squad_slot"), NumericValue)) { Profile.SquadSlot = FMath::Max(0, FMath::RoundToInt(NumericValue)); }
		if (Profile.ProfileId.IsNone() || Profile.EnemyClass.IsNull() || Profile.CombatProfileId.IsNone() || EnemyProfilesById.Contains(Profile.ProfileId))
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy profile row %d has an invalid or duplicate ProfileId."), RowIndex);
			continue;
		}
		EnemyProfilesById.Add(Profile.ProfileId, Profile);
	}
	return true;
}

bool UTunaSweeperRaidPlacementSubsystem::LoadEnemyPlacements(const FString& JsonPath)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath)) { UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Failed to read enemy placements: %s"), *JsonPath); return false; }
	TArray<TSharedPtr<FJsonValue>> Rows;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonContent), Rows)) { UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Failed to parse enemy placements: %s"), *JsonPath); return false; }
	TSet<FString> SeenKeys;
	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!Rows[RowIndex].IsValid() || !Rows[RowIndex]->TryGetObject(JsonObjectPtr) || !JsonObjectPtr || !JsonObjectPtr->IsValid()) { continue; }
		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		int32 PlacementId = INDEX_NONE;
		if (!TunaSweeperRaidPlacement::TryGetPlacementId(JsonObject, PlacementId)) { continue; } // Legacy coordinate row.
		if (JsonObject->HasField(TEXT("location")) || JsonObject->HasField(TEXT("rotation")) || JsonObject->HasField(TEXT("scale")))
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy anchor placement row %d must not contain transform fields."), RowIndex);
			continue;
		}
		FString LevelIdString;
		FString ProfileIdString;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelIdString) || !JsonObject->TryGetStringField(TEXT("profile_id"), ProfileIdString) || PlacementId <= 0)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy placement row %d is missing a valid level_name, placement_id, or profile_id."), RowIndex);
			continue;
		}
		FEnemyPlacementDefinition Placement;
		Placement.LevelId = FName(*LevelIdString.TrimStartAndEnd());
		Placement.PlacementId = PlacementId;
		Placement.ProfileId = FName(*ProfileIdString.TrimStartAndEnd());
		Placement.SpawnChance = TunaSweeperRaidPlacement::ReadSpawnChance(JsonObject);
		Placement.ConditionId = TunaSweeperRaidPlacement::ReadOptionalName(JsonObject, TEXT("condition_id"));
		const FString Key = Placement.LevelId.ToString() + TEXT(":") + FString::FromInt(Placement.PlacementId);
		if (Placement.LevelId.IsNone() || Placement.ProfileId.IsNone() || SeenKeys.Contains(Key)) { UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Enemy placement row %d has an invalid or duplicate level/PlacementId."), RowIndex); continue; }
		SeenKeys.Add(Key);
		EnemyPlacementDefinitions.Add(Placement);
	}
	return true;
}

bool UTunaSweeperRaidPlacementSubsystem::LoadLootPlacements(const FString& JsonPath)
{
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath)) { UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Failed to read loot placements: %s"), *JsonPath); return false; }
	TArray<TSharedPtr<FJsonValue>> Rows;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonContent), Rows)) { UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Failed to parse loot placements: %s"), *JsonPath); return false; }
	TSet<FString> SeenKeys;
	for (int32 RowIndex = 0; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!Rows[RowIndex].IsValid() || !Rows[RowIndex]->TryGetObject(JsonObjectPtr) || !JsonObjectPtr || !JsonObjectPtr->IsValid()) { continue; }
		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		int32 PlacementId = INDEX_NONE;
		if (!TunaSweeperRaidPlacement::TryGetPlacementId(JsonObject, PlacementId)) { continue; } // Legacy coordinate row.
		if (JsonObject->HasField(TEXT("location")) || JsonObject->HasField(TEXT("rotation")) || JsonObject->HasField(TEXT("scale")))
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Loot anchor placement row %d must not contain transform fields."), RowIndex);
			continue;
		}
		FString LevelIdString;
		FString ClassPath;
		double ContainerDefinitionId = INDEX_NONE;
		double ContentsId = INDEX_NONE;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelIdString) || !JsonObject->TryGetNumberField(TEXT("container_definition_id"), ContainerDefinitionId) || !JsonObject->TryGetNumberField(TEXT("contents_id"), ContentsId) || PlacementId <= 0)
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Loot placement row %d is missing a valid level_name, placement_id, definition, or contents id."), RowIndex);
			continue;
		}
		if (!JsonObject->TryGetStringField(TEXT("loot_container_class"), ClassPath) || ClassPath.TrimStartAndEnd().IsEmpty())
		{
			UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Loot placement row %d is missing loot_container_class."), RowIndex);
			continue;
		}
		FLootPlacementDefinition Placement;
		Placement.LevelId = FName(*LevelIdString.TrimStartAndEnd());
		Placement.PlacementId = PlacementId;
		Placement.LootContainerClass = TSoftClassPtr<ATunaSweeperLootContainerActor>(FSoftObjectPath(ClassPath.TrimStartAndEnd()));
		Placement.ContainerDefinitionId = FMath::RoundToInt(ContainerDefinitionId);
		Placement.ContentsId = FMath::RoundToInt(ContentsId);
		Placement.SpawnChance = TunaSweeperRaidPlacement::ReadSpawnChance(JsonObject);
		Placement.ConditionId = TunaSweeperRaidPlacement::ReadOptionalName(JsonObject, TEXT("condition_id"));
		const FString Key = Placement.LevelId.ToString() + TEXT(":") + FString::FromInt(Placement.PlacementId);
		if (Placement.LevelId.IsNone() || Placement.ContainerDefinitionId <= 0 || Placement.ContentsId <= 0 || SeenKeys.Contains(Key)) { UE_LOG(LogTunaSweeperRaidPlacement, Error, TEXT("Loot placement row %d has invalid or duplicate identifiers."), RowIndex); continue; }
		SeenKeys.Add(Key);
		LootPlacementDefinitions.Add(Placement);
	}
	return true;
}

void UTunaSweeperRaidPlacementSubsystem::ResetLoadedData()
{
	EnemyProfilesById.Reset();
	EnemyPlacementDefinitions.Reset();
	LootPlacementDefinitions.Reset();
	bDataLoaded = false;
}

FString UTunaSweeperRaidPlacementSubsystem::GetEnemyProfilesJsonPath() const { return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TunaSweeperRaidPlacement::EnemySpawnProfilesFileName); }
FString UTunaSweeperRaidPlacementSubsystem::GetEnemyPlacementsJsonPath() const { return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TunaSweeperRaidPlacement::EnemySpawnsFileName); }
FString UTunaSweeperRaidPlacementSubsystem::GetLootPlacementsJsonPath() const { return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TunaSweeperRaidPlacement::LootContainerSpawnsFileName); }

bool UTunaSweeperRaidPlacementSubsystem::DoesLevelIdMatchWorld(FName LevelId, const UWorld* World) const
{
	return World &&
		!LevelId.IsNone() &&
		TunaSweeperRaidPlacement::NormalizeLevelId(TunaSweeperBuildFlavor::ResolveGameplayLevelName(LevelId)) ==
			TunaSweeperRaidPlacement::NormalizeLevelId(FName(*World->GetMapName()));
}

bool UTunaSweeperRaidPlacementSubsystem::DoesSpawnConditionPass(FName ConditionId, const FString& DebugId) const
{
	if (ConditionId.IsNone() || ConditionId == FName(TEXT("always"))) { return true; }
	UE_LOG(LogTunaSweeperRaidPlacement, Warning, TEXT("Skipping %s: condition_id '%s' has no registered evaluator."), *DebugId, *ConditionId.ToString());
	return false;
}

bool UTunaSweeperRaidPlacementSubsystem::ShouldSpawnAtPlacement(int32 PlacementId, float SpawnChance) const
{
	return SpawnChance >= 1.0f || (SpawnChance > 0.0f && GetDeterministicPlacementRoll(RaidSeed, PlacementId) < SpawnChance);
}

FName UTunaSweeperRaidPlacementSubsystem::MakeRuntimeInstanceId(FName LevelId, int32 PlacementId) const
{
	return FName(*FString::Printf(TEXT("raid_runtime_%s_%d_%d"), *TunaSweeperRaidPlacement::NormalizeLevelId(LevelId), PlacementId, RaidSeed));
}

void UTunaSweeperRaidPlacementSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	EnsureRaidPlacementActorsSpawnedForWorld(LoadedWorld);
}
