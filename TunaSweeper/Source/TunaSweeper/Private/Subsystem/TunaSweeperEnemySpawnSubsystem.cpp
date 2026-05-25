#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Interaction/TunaSweeperTransparentObstacleActor.h"
#include "Interaction/TunaSweeperWarpPointActor.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperEnemySpawn, Log, All);

namespace TunaSweeperEnemySpawn
{
	const TCHAR* EnemySpawnsJsonRelativePath = TEXT("Data/EnemySpawns.json");
	const TCHAR* LootContainerSpawnsJsonRelativePath = TEXT("Data/LootContainerSpawns.json");
	const TCHAR* TransparentObstacleSpawnsJsonRelativePath = TEXT("Data/TransparentObstacleSpawns.json");
	const TCHAR* WorldProgressObjectSpawnsJsonRelativePath = TEXT("Data/WorldProgressObjectSpawns.json");
	const TCHAR* WarpPointSpawnsJsonRelativePath = TEXT("Data/WarpPointSpawns.json");
	const TCHAR* DefaultEnemyClassPath = TEXT("/Game/Characters/Enemy/BP_TunaSweeperEnemy.BP_TunaSweeperEnemy_C");
	const TCHAR* DefaultLootContainerClassPath = TEXT("/Game/Interaction/BP_LootContainer.BP_LootContainer_C");
	const TCHAR* DefaultTransparentObstacleClassPath = TEXT("/Game/Interaction/BP_TransparentObstacle.BP_TransparentObstacle_C");
	const TCHAR* DefaultWorldProgressActorClassPath = TEXT("/Game/Interaction/BP_WorldProgress_BrokenBridge.BP_WorldProgress_BrokenBridge_C");
	const TCHAR* DefaultWorldProgressCompletedActorClassPath = TEXT("/Game/Interaction/BP_WorldProgress_RepairedBridge.BP_WorldProgress_RepairedBridge_C");
	const TCHAR* DefaultWarpPointClassPath = TEXT("/Game/Interaction/BP_WarpPoint.BP_WarpPoint_C");
	const TCHAR* DefaultWarpPointMaterialPath = TEXT("/Game/Interaction/M_WarpPointEnergy.M_WarpPointEnergy");
	const TCHAR* DefaultWarpPointSphereMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");

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

	ResetLoadedEnemySpawnData();
	ResetLoadedLootContainerSpawnData();
	ResetLoadedTransparentObstacleSpawnData();
	ResetLoadedWorldProgressObjectSpawnData();
	ResetLoadedWarpPointSpawnData();
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
	const bool bLoadedTransparentObstacles = LoadTransparentObstacleSpawnData(false);
	const bool bLoadedWorldProgressObjects = LoadWorldProgressObjectSpawnData(false);
	const bool bLoadedWarpPoints = LoadWarpPointSpawnData(false);
	if (!bLoadedEnemies && !bLoadedLootContainers && !bLoadedTransparentObstacles && !bLoadedWorldProgressObjects && !bLoadedWarpPoints)
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
					SpawnDefinition.EnemyId,
					SpawnDefinition.DropContainerDefinitionId,
					SpawnDefinition.DropContentsId,
					SpawnDefinition.MaxHealth);
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
			if (SpawnDefinition.bEditorOnly && !TunaSweeperEnemySpawn::ShouldIncludeEditorOnlySpawn(World))
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

	int32 SpawnedTransparentObstacleCount = 0;
	if (bLoadedTransparentObstacles)
	{
		for (const FTransparentObstacleSpawnDefinition& SpawnDefinition : TransparentObstacleSpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperTransparentObstacleActor> LoadedObstacleClass = SpawnDefinition.ObstacleClass.LoadSynchronous();
			if (!LoadedObstacleClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("Transparent obstacle class failed to load for level %s. Falling back to native transparent obstacle actor."),
					*SpawnDefinition.LevelName.ToString());
				LoadedObstacleClass = ATunaSweeperTransparentObstacleActor::StaticClass();
			}

			const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location);
			ATunaSweeperTransparentObstacleActor* SpawnedObstacle =
				World->SpawnActorDeferred<ATunaSweeperTransparentObstacleActor>(
					LoadedObstacleClass,
					SpawnTransform,
					nullptr,
					nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedObstacle)
			{
				SpawnedObstacle->ConfigureObstacleDefaults(SpawnDefinition.ObstacleId, SpawnDefinition.BoxExtent);
				if (!SpawnDefinition.ObstacleId.IsNone())
				{
					SpawnedObstacle->Tags.AddUnique(SpawnDefinition.ObstacleId);
				}
				UGameplayStatics::FinishSpawningActor(SpawnedObstacle, SpawnTransform);
				++SpawnedTransparentObstacleCount;
			}
		}
	}

	int32 SpawnedWorldProgressObjectCount = 0;
	if (bLoadedWorldProgressObjects)
	{
		for (const FWorldProgressObjectSpawnDefinition& SpawnDefinition : WorldProgressObjectSpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperWorldProgressActor> LoadedProgressActorClass =
				SpawnDefinition.ProgressActorClass.LoadSynchronous();
			if (!LoadedProgressActorClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("World progress actor class failed to load for level %s. Falling back to native world progress actor."),
					*SpawnDefinition.LevelName.ToString());
				LoadedProgressActorClass = ATunaSweeperWorldProgressActor::StaticClass();
			}

			const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location);
			ATunaSweeperWorldProgressActor* SpawnedProgressActor =
				World->SpawnActorDeferred<ATunaSweeperWorldProgressActor>(
					LoadedProgressActorClass,
					SpawnTransform,
					nullptr,
					nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedProgressActor)
			{
				SpawnedProgressActor->ConfigureWorldProgressDefaults(
					SpawnDefinition.ObjectId,
					SpawnDefinition.InfoId,
					SpawnDefinition.DisplayName,
					SpawnDefinition.InteractionDisplayName,
					SpawnDefinition.RequiredItemId,
					SpawnDefinition.RequiredQuantity,
					SpawnDefinition.InitialProgressQuantity,
					SpawnDefinition.RequiredItemDisplayName,
					SpawnDefinition.BoxExtent,
					SpawnDefinition.CompletedActorClass);
				if (!SpawnDefinition.ObjectId.IsNone())
				{
					SpawnedProgressActor->Tags.AddUnique(SpawnDefinition.ObjectId);
				}
				UGameplayStatics::FinishSpawningActor(SpawnedProgressActor, SpawnTransform);
				++SpawnedWorldProgressObjectCount;
			}
		}
	}

	int32 SpawnedWarpPointCount = 0;
	if (bLoadedWarpPoints)
	{
		for (const FWarpPointSpawnDefinition& SpawnDefinition : WarpPointSpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperWarpPointActor> LoadedWarpPointClass =
				SpawnDefinition.WarpPointClass.LoadSynchronous();
			if (!LoadedWarpPointClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("Warp point class failed to load for level %s. Falling back to native warp point actor."),
					*SpawnDefinition.LevelName.ToString());
				LoadedWarpPointClass = ATunaSweeperWarpPointActor::StaticClass();
			}

			const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location);
			ATunaSweeperWarpPointActor* SpawnedWarpPoint =
				World->SpawnActorDeferred<ATunaSweeperWarpPointActor>(
					LoadedWarpPointClass,
					SpawnTransform,
					nullptr,
					nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedWarpPoint)
			{
				SpawnedWarpPoint->ConfigureWarpPointDefaults(
					SpawnDefinition.WarpPointId,
					SpawnDefinition.TargetWarpPointId,
					FText::FromString(TEXT("\uC0C1\uD638\uC791\uC6A9")),
					TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
						FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
					TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TunaSweeperEnemySpawn::DefaultWarpPointMaterialPath)),
					TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TunaSweeperEnemySpawn::DefaultWarpPointSphereMeshPath)),
					SpawnDefinition.VisualScale,
					SpawnDefinition.VisualRelativeLocation,
					SpawnDefinition.ExitOffset,
					SpawnDefinition.bUseTargetRotation);
				if (!SpawnDefinition.WarpPointId.IsNone())
				{
					SpawnedWarpPoint->Tags.AddUnique(SpawnDefinition.WarpPointId);
				}
				UGameplayStatics::FinishSpawningActor(SpawnedWarpPoint, SpawnTransform);
				++SpawnedWarpPointCount;
			}
		}
	}

	UE_LOG(
		LogTunaSweeperEnemySpawn,
		Log,
		TEXT("Spawned %d enemies, %d loot containers, %d transparent obstacles, %d world progress objects, and %d warp points for level %s."),
		SpawnedCount,
		SpawnedLootContainerCount,
		SpawnedTransparentObstacleCount,
		SpawnedWorldProgressObjectCount,
		SpawnedWarpPointCount,
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
		FString EnemyId;
		FString EnemyClassPath;
		FString BodyMaterialPath;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		double NumericDropContainerDefinitionId = INDEX_NONE;
		double NumericDropContentsId = INDEX_NONE;
		double NumericMaxHealth = 30.0;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("enemy_id"), EnemyId);
		JsonObject->TryGetStringField(TEXT("enemy_class"), EnemyClassPath);
		JsonObject->TryGetStringField(TEXT("body_material"), BodyMaterialPath);
		JsonObject->TryGetNumberField(TEXT("drop_container_definition_id"), NumericDropContainerDefinitionId);
		JsonObject->TryGetNumberField(TEXT("drop_contents_id"), NumericDropContentsId);
		JsonObject->TryGetNumberField(TEXT("max_health"), NumericMaxHealth);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);

		FEnemySpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.EnemyId = EnemyId.TrimStartAndEnd().IsEmpty()
			? NAME_None
			: FName(*EnemyId.TrimStartAndEnd());
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
		bool bEditorOnly = false;
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
		JsonObject->TryGetBoolField(TEXT("editor_only"), bEditorOnly);
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
		SpawnDefinition.bEditorOnly = bEditorOnly;

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

bool UTunaSweeperEnemySpawnSubsystem::LoadTransparentObstacleSpawnData(bool bForceReload)
{
	if (bTransparentObstacleSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedTransparentObstacleSpawnData();

	FString JsonContent;
	const FString ObstacleSpawnJsonPath = GetTransparentObstacleSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *ObstacleSpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read transparent obstacle spawn JSON: %s"), *ObstacleSpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse transparent obstacle spawn JSON: %s"), *ObstacleSpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping transparent obstacle spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString ObstacleId;
		FString ObstacleClassPath;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector BoxExtent(260.0f, 45.0f, 140.0f);
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!JsonObject->TryGetStringField(TEXT("obstacle_id"), ObstacleId) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping transparent obstacle spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("obstacle_class"), ObstacleClassPath);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("box_extent"), BoxExtent);

		FTransparentObstacleSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.ObstacleId = FName(*ObstacleId.TrimStartAndEnd());
		SpawnDefinition.ObstacleClass = TSoftClassPtr<ATunaSweeperTransparentObstacleActor>(
			FSoftObjectPath(ObstacleClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultTransparentObstacleClassPath)
				: ObstacleClassPath.TrimStartAndEnd()));
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.BoxExtent = FVector(
			FMath::Max(1.0f, BoxExtent.X),
			FMath::Max(1.0f, BoxExtent.Y),
			FMath::Max(1.0f, BoxExtent.Z));

		if (SpawnDefinition.LevelName.IsNone() || SpawnDefinition.ObstacleId.IsNone())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping transparent obstacle spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		TransparentObstacleSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Transparent obstacle spawn JSON has no valid rows: %s"), *ObstacleSpawnJsonPath);
		return false;
	}

	bTransparentObstacleSpawnDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadWorldProgressObjectSpawnData(bool bForceReload)
{
	if (bWorldProgressObjectSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedWorldProgressObjectSpawnData();

	FString JsonContent;
	const FString ProgressObjectSpawnJsonPath = GetWorldProgressObjectSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *ProgressObjectSpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read world progress object spawn JSON: %s"), *ProgressObjectSpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse world progress object spawn JSON: %s"), *ProgressObjectSpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping world progress object spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString ObjectId;
		FString InfoId;
		FString ProgressActorClassPath;
		FString CompletedActorClassPath;
		FString DisplayName;
		FString InteractionDisplayName;
		FString RequiredItemDisplayName;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector BoxExtent(260.0f, 55.0f, 140.0f);
		double NumericRequiredItemId = 6002.0;
		double NumericRequiredQuantity = 2.0;
		double NumericInitialProgressQuantity = 0.0;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!JsonObject->TryGetStringField(TEXT("object_id"), ObjectId) ||
			!JsonObject->TryGetStringField(TEXT("info_id"), InfoId) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping world progress object spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("progress_actor_class"), ProgressActorClassPath);
		JsonObject->TryGetStringField(TEXT("completed_actor_class"), CompletedActorClassPath);
		JsonObject->TryGetStringField(TEXT("display_name"), DisplayName);
		JsonObject->TryGetStringField(TEXT("interaction_display_name"), InteractionDisplayName);
		JsonObject->TryGetStringField(TEXT("required_item_display_name"), RequiredItemDisplayName);
		JsonObject->TryGetNumberField(TEXT("required_item_id"), NumericRequiredItemId);
		JsonObject->TryGetNumberField(TEXT("required_quantity"), NumericRequiredQuantity);
		JsonObject->TryGetNumberField(TEXT("initial_progress_quantity"), NumericInitialProgressQuantity);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("box_extent"), BoxExtent);

		FWorldProgressObjectSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.ObjectId = FName(*ObjectId.TrimStartAndEnd());
		SpawnDefinition.InfoId = FName(*InfoId.TrimStartAndEnd());
		SpawnDefinition.ProgressActorClass = TSoftClassPtr<ATunaSweeperWorldProgressActor>(
			FSoftObjectPath(ProgressActorClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultWorldProgressActorClassPath)
				: ProgressActorClassPath.TrimStartAndEnd()));
		SpawnDefinition.CompletedActorClass = TSoftClassPtr<AActor>(
			FSoftObjectPath(CompletedActorClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultWorldProgressCompletedActorClassPath)
				: CompletedActorClassPath.TrimStartAndEnd()));
		SpawnDefinition.DisplayName = DisplayName.TrimStartAndEnd().IsEmpty()
			? FText::FromString(TEXT("\uBD80\uC11C\uC9C4 \uB2E4\uB9AC"))
			: FText::FromString(DisplayName.TrimStartAndEnd());
		SpawnDefinition.InteractionDisplayName = InteractionDisplayName.TrimStartAndEnd().IsEmpty()
			? FText::FromString(TEXT("\uC218\uB9AC\uD558\uAE30"))
			: FText::FromString(InteractionDisplayName.TrimStartAndEnd());
		SpawnDefinition.RequiredItemDisplayName = RequiredItemDisplayName.TrimStartAndEnd().IsEmpty()
			? FText::FromString(TEXT("\uBAA9\uC7AC"))
			: FText::FromString(RequiredItemDisplayName.TrimStartAndEnd());
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.BoxExtent = FVector(
			FMath::Max(1.0f, BoxExtent.X),
			FMath::Max(1.0f, BoxExtent.Y),
			FMath::Max(1.0f, BoxExtent.Z));
		SpawnDefinition.RequiredItemId = static_cast<int32>(NumericRequiredItemId);
		SpawnDefinition.RequiredQuantity = FMath::Max(1, static_cast<int32>(NumericRequiredQuantity));
		SpawnDefinition.InitialProgressQuantity = FMath::Clamp(
			static_cast<int32>(NumericInitialProgressQuantity),
			0,
			SpawnDefinition.RequiredQuantity);

		if (SpawnDefinition.LevelName.IsNone() ||
			SpawnDefinition.ObjectId.IsNone() ||
			SpawnDefinition.InfoId.IsNone() ||
			SpawnDefinition.RequiredItemId == INDEX_NONE)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping world progress object spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		WorldProgressObjectSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("World progress object spawn JSON has no valid rows: %s"), *ProgressObjectSpawnJsonPath);
		return false;
	}

	bWorldProgressObjectSpawnDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadWarpPointSpawnData(bool bForceReload)
{
	if (bWarpPointSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedWarpPointSpawnData();

	FString JsonContent;
	const FString WarpPointSpawnJsonPath = GetWarpPointSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *WarpPointSpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read warp point spawn JSON: %s"), *WarpPointSpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse warp point spawn JSON: %s"), *WarpPointSpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping warp point spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString WarpPointId;
		FString TargetWarpPointId;
		FString WarpPointClassPath;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector VisualScale(1.2f, 1.2f, 1.2f);
		FVector VisualRelativeLocation = FVector::ZeroVector;
		FVector ExitOffset(160.0f, 0.0f, 0.0f);
		bool bUseTargetRotation = true;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!JsonObject->TryGetStringField(TEXT("warp_point_id"), WarpPointId) ||
			!JsonObject->TryGetStringField(TEXT("target_warp_point_id"), TargetWarpPointId) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping warp point spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("warp_point_class"), WarpPointClassPath);
		JsonObject->TryGetBoolField(TEXT("use_target_rotation"), bUseTargetRotation);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("visual_scale"), VisualScale);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("visual_relative_location"), VisualRelativeLocation);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("exit_offset"), ExitOffset);

		FWarpPointSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.WarpPointId = FName(*WarpPointId.TrimStartAndEnd());
		SpawnDefinition.TargetWarpPointId = FName(*TargetWarpPointId.TrimStartAndEnd());
		SpawnDefinition.WarpPointClass = TSoftClassPtr<ATunaSweeperWarpPointActor>(
			FSoftObjectPath(WarpPointClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultWarpPointClassPath)
				: WarpPointClassPath.TrimStartAndEnd()));
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.VisualScale = FVector(
			FMath::Max(0.01f, VisualScale.X),
			FMath::Max(0.01f, VisualScale.Y),
			FMath::Max(0.01f, VisualScale.Z));
		SpawnDefinition.VisualRelativeLocation = VisualRelativeLocation;
		SpawnDefinition.ExitOffset = ExitOffset;
		SpawnDefinition.bUseTargetRotation = bUseTargetRotation;

		if (SpawnDefinition.LevelName.IsNone() ||
			SpawnDefinition.WarpPointId.IsNone() ||
			SpawnDefinition.TargetWarpPointId.IsNone())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping warp point spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		WarpPointSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Warp point spawn JSON has no valid rows: %s"), *WarpPointSpawnJsonPath);
		return false;
	}

	bWarpPointSpawnDataLoaded = true;
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

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedTransparentObstacleSpawnData()
{
	TransparentObstacleSpawnDefinitions.Reset();
	bTransparentObstacleSpawnDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedWorldProgressObjectSpawnData()
{
	WorldProgressObjectSpawnDefinitions.Reset();
	bWorldProgressObjectSpawnDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedWarpPointSpawnData()
{
	WarpPointSpawnDefinitions.Reset();
	bWarpPointSpawnDataLoaded = false;
}

FString UTunaSweeperEnemySpawnSubsystem::GetEnemySpawnJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::EnemySpawnsJsonRelativePath);
}

FString UTunaSweeperEnemySpawnSubsystem::GetLootContainerSpawnJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::LootContainerSpawnsJsonRelativePath);
}

FString UTunaSweeperEnemySpawnSubsystem::GetTransparentObstacleSpawnJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::TransparentObstacleSpawnsJsonRelativePath);
}

FString UTunaSweeperEnemySpawnSubsystem::GetWorldProgressObjectSpawnJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::WorldProgressObjectSpawnsJsonRelativePath);
}

FString UTunaSweeperEnemySpawnSubsystem::GetWarpPointSpawnJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::WarpPointSpawnsJsonRelativePath);
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
