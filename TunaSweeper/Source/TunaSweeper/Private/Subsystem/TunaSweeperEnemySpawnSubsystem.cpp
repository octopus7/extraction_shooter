#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperEnemySpawn, Log, All);

namespace TunaSweeperEnemySpawn
{
	const TCHAR* EnemySpawnsJsonRelativePath = TEXT("Data/EnemySpawns.json");
	const TCHAR* LootContainerSpawnsJsonRelativePath = TEXT("Data/LootContainerSpawns.json");
	const TCHAR* DefaultEnemyClassPath = TEXT("/Game/Characters/Enemy/BP_TunaSweeperEnemy.BP_TunaSweeperEnemy_C");
	const TCHAR* DefaultLootContainerClassPath = TEXT("/Game/Interaction/BP_LootContainer.BP_LootContainer_C");

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

	ETunaSweeperEnemyAttackMode ParseEnemyAttackMode(const FString& RawAttackMode)
	{
		const FString AttackMode = RawAttackMode.TrimStartAndEnd();
		if (AttackMode.Equals(TEXT("melee"), ESearchCase::IgnoreCase) ||
			AttackMode.Equals(TEXT("close"), ESearchCase::IgnoreCase))
		{
			return ETunaSweeperEnemyAttackMode::Melee;
		}

		return ETunaSweeperEnemyAttackMode::Projectile;
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

	ResetLoadedEnemySpawnData();
	ResetLoadedLootContainerSpawnData();
	LastSpawnedWorld.Reset();
	Super::Deinitialize();
}

bool UTunaSweeperEnemySpawnSubsystem::EnsureEnemiesSpawnedForWorld(UWorld* World)
{
	return EnsureRaidRuntimeActorsSpawnedForWorld(World);
}

bool UTunaSweeperEnemySpawnSubsystem::EnsureRaidRuntimeActorsSpawnedForWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return true;
	}

	if (LastSpawnedWorld.Get() == World)
	{
		return true;
	}

	const bool bLoadedEnemies = LoadEnemySpawnData(false);
	const bool bLoadedLootContainers = LoadLootContainerSpawnData(false);
	if (!bLoadedEnemies && !bLoadedLootContainers)
	{
		return false;
	}

	LastSpawnedWorld = World;

	int32 SpawnedCount = 0;
	if (bLoadedEnemies)
	{
		for (const FEnemySpawnDefinition& SpawnDefinition : EnemySpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperEnemyCharacter> LoadedEnemyClass = SpawnDefinition.EnemyClass.LoadSynchronous();
			if (!LoadedEnemyClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("Enemy class failed to load for level %s. Falling back to native enemy character."),
					*SpawnDefinition.LevelName.ToString());
				LoadedEnemyClass = ATunaSweeperEnemyCharacter::StaticClass();
			}

			const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location);
			ATunaSweeperEnemyCharacter* SpawnedEnemy = World->SpawnActorDeferred<ATunaSweeperEnemyCharacter>(
				LoadedEnemyClass,
				SpawnTransform,
				nullptr,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedEnemy)
			{
				SpawnedEnemy->ConfigureSpawnData(
					SpawnDefinition.BodyMaterial,
					SpawnDefinition.DropContainerDefinitionId,
					SpawnDefinition.DropContentsId,
					SpawnDefinition.MaxHealth);
				SpawnedEnemy->ConfigureAttackData(
					SpawnDefinition.AttackMode,
					SpawnDefinition.AttackDamage,
					SpawnDefinition.AttackRange,
					SpawnDefinition.ApproachStartRange,
					SpawnDefinition.ApproachStopRange,
					SpawnDefinition.TrackingRange,
					SpawnDefinition.AttackCooldownSeconds);
				UGameplayStatics::FinishSpawningActor(SpawnedEnemy, SpawnTransform);
				++SpawnedCount;
			}
		}
	}

	int32 SpawnedLootContainerCount = 0;
	if (bLoadedLootContainers)
	{
		for (const FLootContainerSpawnDefinition& SpawnDefinition : LootContainerSpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperLootContainerActor> LoadedContainerClass = SpawnDefinition.LootContainerClass.LoadSynchronous();
			if (!LoadedContainerClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("Loot container class failed to load for level %s. Falling back to native loot container actor."),
					*SpawnDefinition.LevelName.ToString());
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
				SpawnedContainer->SetContainerDataIds(
					SpawnDefinition.ContainerDefinitionId,
					SpawnDefinition.ContentsId);
				++SpawnedLootContainerCount;
			}
		}
	}

	UE_LOG(
		LogTunaSweeperEnemySpawn,
		Log,
		TEXT("Spawned %d enemies and %d loot containers for level %s."),
		SpawnedCount,
		SpawnedLootContainerCount,
		*TunaSweeperEnemySpawn::NormalizeLevelName(World->GetMapName()));
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadEnemySpawnData(bool bForceReload)
{
	if (bEnemySpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedEnemySpawnData();

	FString JsonContent;
	const FString EnemySpawnJsonPath = GetEnemySpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *EnemySpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read enemy spawn JSON: %s"), *EnemySpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse enemy spawn JSON: %s"), *EnemySpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString EnemyClassPath;
		FString BodyMaterialPath;
		FString AttackModeString;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		double NumericDropContainerDefinitionId = INDEX_NONE;
		double NumericDropContentsId = INDEX_NONE;
		double NumericMaxHealth = 30.0;
		double NumericAttackDamage = -1.0;
		double NumericAttackRange = -1.0;
		double NumericApproachStartRange = -1.0;
		double NumericApproachStopRange = -1.0;
		double NumericTrackingRange = -1.0;
		double NumericAttackCooldownSeconds = -1.0;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("enemy_class"), EnemyClassPath);
		JsonObject->TryGetStringField(TEXT("body_material"), BodyMaterialPath);
		JsonObject->TryGetStringField(TEXT("attack_mode"), AttackModeString);
		JsonObject->TryGetNumberField(TEXT("drop_container_definition_id"), NumericDropContainerDefinitionId);
		JsonObject->TryGetNumberField(TEXT("drop_contents_id"), NumericDropContentsId);
		JsonObject->TryGetNumberField(TEXT("max_health"), NumericMaxHealth);
		JsonObject->TryGetNumberField(TEXT("attack_damage"), NumericAttackDamage);
		JsonObject->TryGetNumberField(TEXT("attack_range"), NumericAttackRange);
		JsonObject->TryGetNumberField(TEXT("approach_start_range"), NumericApproachStartRange);
		JsonObject->TryGetNumberField(TEXT("approach_stop_range"), NumericApproachStopRange);
		JsonObject->TryGetNumberField(TEXT("tracking_range"), NumericTrackingRange);
		JsonObject->TryGetNumberField(TEXT("attack_cooldown_seconds"), NumericAttackCooldownSeconds);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);

		FEnemySpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.EnemyClass = TSoftClassPtr<ATunaSweeperEnemyCharacter>(
			FSoftObjectPath(EnemyClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultEnemyClassPath)
				: EnemyClassPath.TrimStartAndEnd()));
		const FString TrimmedBodyMaterialPath = BodyMaterialPath.TrimStartAndEnd();
		if (!TrimmedBodyMaterialPath.IsEmpty())
		{
			SpawnDefinition.BodyMaterial = TSoftObjectPtr<UMaterialInterface>(
				FSoftObjectPath(TrimmedBodyMaterialPath));
		}
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.DropContainerDefinitionId = static_cast<int32>(NumericDropContainerDefinitionId);
		SpawnDefinition.DropContentsId = static_cast<int32>(NumericDropContentsId);
		SpawnDefinition.MaxHealth = FMath::Max(1.0f, static_cast<float>(NumericMaxHealth));
		SpawnDefinition.AttackMode = TunaSweeperEnemySpawn::ParseEnemyAttackMode(AttackModeString);
		SpawnDefinition.AttackDamage = static_cast<float>(NumericAttackDamage);
		SpawnDefinition.AttackRange = static_cast<float>(NumericAttackRange);
		SpawnDefinition.ApproachStartRange = static_cast<float>(NumericApproachStartRange);
		SpawnDefinition.ApproachStopRange = static_cast<float>(NumericApproachStopRange);
		SpawnDefinition.TrackingRange = static_cast<float>(NumericTrackingRange);
		SpawnDefinition.AttackCooldownSeconds = static_cast<float>(NumericAttackCooldownSeconds);

		if (SpawnDefinition.LevelName.IsNone())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy spawn row %d: level_name is empty."), RowIndex);
			continue;
		}

		EnemySpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Enemy spawn JSON has no valid rows: %s"), *EnemySpawnJsonPath);
		return false;
	}

	bEnemySpawnDataLoaded = true;
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
	const FString LootContainerSpawnJsonPath = GetLootContainerSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *LootContainerSpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read loot container spawn JSON: %s"), *LootContainerSpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse loot container spawn JSON: %s"), *LootContainerSpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping loot container spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString LootContainerClassPath;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		double NumericContainerDefinitionId = INDEX_NONE;
		double NumericContentsId = INDEX_NONE;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location) ||
			!JsonObject->TryGetNumberField(TEXT("container_definition_id"), NumericContainerDefinitionId) ||
			!JsonObject->TryGetNumberField(TEXT("contents_id"), NumericContentsId))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping loot container spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("loot_container_class"), LootContainerClassPath);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);

		FLootContainerSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.LootContainerClass = TSoftClassPtr<ATunaSweeperLootContainerActor>(
			FSoftObjectPath(LootContainerClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultLootContainerClassPath)
				: LootContainerClassPath.TrimStartAndEnd()));
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.ContainerDefinitionId = static_cast<int32>(NumericContainerDefinitionId);
		SpawnDefinition.ContentsId = static_cast<int32>(NumericContentsId);

		if (SpawnDefinition.LevelName.IsNone() ||
			SpawnDefinition.ContainerDefinitionId == INDEX_NONE ||
			SpawnDefinition.ContentsId == INDEX_NONE)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping loot container spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		LootContainerSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Loot container spawn JSON has no valid rows: %s"), *LootContainerSpawnJsonPath);
		return false;
	}

	bLootContainerSpawnDataLoaded = true;
	return true;
}

void UTunaSweeperEnemySpawnSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	EnsureRaidRuntimeActorsSpawnedForWorld(LoadedWorld);
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedEnemySpawnData()
{
	EnemySpawnDefinitions.Reset();
	bEnemySpawnDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedLootContainerSpawnData()
{
	LootContainerSpawnDefinitions.Reset();
	bLootContainerSpawnDataLoaded = false;
}

FString UTunaSweeperEnemySpawnSubsystem::GetEnemySpawnJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::EnemySpawnsJsonRelativePath);
}

FString UTunaSweeperEnemySpawnSubsystem::GetLootContainerSpawnJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::LootContainerSpawnsJsonRelativePath);
}

bool UTunaSweeperEnemySpawnSubsystem::DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const
{
	if (!World || LevelName.IsNone())
	{
		return false;
	}

	const FString SpawnLevelName = TunaSweeperEnemySpawn::NormalizeLevelName(LevelName.ToString());
	const FString WorldMapName = TunaSweeperEnemySpawn::NormalizeLevelName(World->GetMapName());
	const FString WorldPackageName = TunaSweeperEnemySpawn::NormalizeLevelName(World->GetOutermost()->GetName());
	return SpawnLevelName.Equals(WorldMapName, ESearchCase::IgnoreCase) ||
		SpawnLevelName.Equals(WorldPackageName, ESearchCase::IgnoreCase);
}
