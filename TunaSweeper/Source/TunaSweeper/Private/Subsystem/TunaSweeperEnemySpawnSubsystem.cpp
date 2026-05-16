#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
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
	const TCHAR* DefaultEnemyClassPath = TEXT("/Game/Characters/Enemy/BP_TunaSweeperEnemy.BP_TunaSweeperEnemy_C");

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
	LastSpawnedWorld.Reset();
	Super::Deinitialize();
}

bool UTunaSweeperEnemySpawnSubsystem::EnsureEnemiesSpawnedForWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return true;
	}

	if (LastSpawnedWorld.Get() == World)
	{
		return true;
	}

	if (!LoadEnemySpawnData(false))
	{
		return false;
	}

	LastSpawnedWorld = World;

	int32 SpawnedCount = 0;
	for (const FEnemySpawnDefinition& SpawnDefinition : EnemySpawnDefinitions)
	{
		if (!DoesSpawnMatchWorld(SpawnDefinition, World))
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

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ATunaSweeperEnemyCharacter* SpawnedEnemy = World->SpawnActor<ATunaSweeperEnemyCharacter>(
			LoadedEnemyClass,
			SpawnDefinition.Location,
			SpawnDefinition.Rotation,
			SpawnParameters);
		if (SpawnedEnemy)
		{
			++SpawnedCount;
		}
	}

	UE_LOG(
		LogTunaSweeperEnemySpawn,
		Log,
		TEXT("Spawned %d enemies for level %s."),
		SpawnedCount,
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
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("enemy_class"), EnemyClassPath);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);

		FEnemySpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.EnemyClass = TSoftClassPtr<ATunaSweeperEnemyCharacter>(
			FSoftObjectPath(EnemyClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultEnemyClassPath)
				: EnemyClassPath.TrimStartAndEnd()));
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;

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

void UTunaSweeperEnemySpawnSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	EnsureEnemiesSpawnedForWorld(LoadedWorld);
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedEnemySpawnData()
{
	EnemySpawnDefinitions.Reset();
	bEnemySpawnDataLoaded = false;
}

FString UTunaSweeperEnemySpawnSubsystem::GetEnemySpawnJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::EnemySpawnsJsonRelativePath);
}

bool UTunaSweeperEnemySpawnSubsystem::DoesSpawnMatchWorld(
	const FEnemySpawnDefinition& SpawnDefinition,
	const UWorld* World) const
{
	if (!World || SpawnDefinition.LevelName.IsNone())
	{
		return false;
	}

	const FString SpawnLevelName = TunaSweeperEnemySpawn::NormalizeLevelName(SpawnDefinition.LevelName.ToString());
	const FString WorldMapName = TunaSweeperEnemySpawn::NormalizeLevelName(World->GetMapName());
	const FString WorldPackageName = TunaSweeperEnemySpawn::NormalizeLevelName(World->GetOutermost()->GetName());
	return SpawnLevelName.Equals(WorldMapName, ESearchCase::IgnoreCase) ||
		SpawnLevelName.Equals(WorldPackageName, ESearchCase::IgnoreCase);
}
