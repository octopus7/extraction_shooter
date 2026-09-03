#include "Subsystem/TunaSweeperBunkerRuntimeSpawnSubsystem.h"

#include "Character/TunaSweeperMoleCompanionActor.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/TunaSweeperBuildFlavor.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperBunkerRuntimeSpawn, Log, All);

namespace TunaSweeperBunkerRuntimeSpawn
{
	const TCHAR* BunkerCharacterSpawnsJsonRelativePath = TEXT("Data/BunkerCharacterSpawns.json");

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

void UTunaSweeperBunkerRuntimeSpawnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UTunaSweeperBunkerRuntimeSpawnSubsystem::HandlePostLoadMapWithWorld);
}

void UTunaSweeperBunkerRuntimeSpawnSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	ResetLoadedBunkerCharacterSpawnData();
	LastSpawnedBunkerWorld.Reset();
	Super::Deinitialize();
}

bool UTunaSweeperBunkerRuntimeSpawnSubsystem::EnsureBunkerRuntimeActorsSpawnedForWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return true;
	}

	if (LastSpawnedBunkerWorld.Get() == World)
	{
		return true;
	}

	if (!LoadBunkerCharacterSpawnData(false))
	{
		return false;
	}

	LastSpawnedBunkerWorld = World;

	int32 SpawnedCount = 0;
	for (const FBunkerCharacterSpawnDefinition& SpawnDefinition : BunkerCharacterSpawnDefinitions)
	{
		if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
		{
			continue;
		}

		TSubclassOf<ATunaSweeperMoleCompanionActor> LoadedActorClass = SpawnDefinition.ActorClass.IsNull()
			? ATunaSweeperMoleCompanionActor::StaticClass()
			: SpawnDefinition.ActorClass.LoadSynchronous();
		if (!LoadedActorClass)
		{
			UE_LOG(
				LogTunaSweeperBunkerRuntimeSpawn,
				Warning,
				TEXT("Bunker character class failed to load for spawn %s. Falling back to native mole companion actor."),
				*SpawnDefinition.SpawnId.ToString());
			LoadedActorClass = ATunaSweeperMoleCompanionActor::StaticClass();
		}

		const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location, SpawnDefinition.Scale);
		ATunaSweeperMoleCompanionActor* SpawnedCompanion =
			World->SpawnActorDeferred<ATunaSweeperMoleCompanionActor>(
				LoadedActorClass,
				SpawnTransform,
				nullptr,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!SpawnedCompanion)
		{
			continue;
		}

		SpawnedCompanion->ConfigureCompanionDefaults(
			SpawnDefinition.SpawnId,
			SpawnDefinition.DummyMesh,
			SpawnDefinition.VisualMaterial);

		if (!SpawnDefinition.SpawnId.IsNone())
		{
			SpawnedCompanion->Tags.AddUnique(SpawnDefinition.SpawnId);
#if WITH_EDITOR
			SpawnedCompanion->SetActorLabel(SpawnDefinition.SpawnId.ToString());
#endif
		}

		UGameplayStatics::FinishSpawningActor(SpawnedCompanion, SpawnTransform);
		++SpawnedCount;
	}

	if (SpawnedCount > 0)
	{
		UE_LOG(LogTunaSweeperBunkerRuntimeSpawn, Log, TEXT("Spawned %d bunker runtime character actor(s)."), SpawnedCount);
	}

	return true;
}

bool UTunaSweeperBunkerRuntimeSpawnSubsystem::LoadBunkerCharacterSpawnData(bool bForceReload)
{
	if (bBunkerCharacterSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedBunkerCharacterSpawnData();

	FString JsonContent;
	const FString JsonPath = GetBunkerCharacterSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		UE_LOG(LogTunaSweeperBunkerRuntimeSpawn, Error, TEXT("Failed to read bunker character spawn JSON: %s"), *JsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperBunkerRuntimeSpawn, Error, TEXT("Failed to parse bunker character spawn JSON: %s"), *JsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperBunkerRuntimeSpawn, Warning, TEXT("Skipping bunker character spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString SpawnId;
		FVector Location = FVector::ZeroVector;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!JsonObject->TryGetStringField(TEXT("spawn_id"), SpawnId) ||
			!TunaSweeperBunkerRuntimeSpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperBunkerRuntimeSpawn, Warning, TEXT("Skipping bunker character spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		FBunkerCharacterSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.SpawnId = FName(*SpawnId.TrimStartAndEnd());
		if (SpawnDefinition.LevelName.IsNone() || SpawnDefinition.SpawnId.IsNone())
		{
			UE_LOG(LogTunaSweeperBunkerRuntimeSpawn, Warning, TEXT("Skipping bunker character spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		FString ActorClassPath;
		FString DummyMeshPath;
		FString VisualMaterialPath;
		FVector Scale = FVector::OneVector;
		FRotator Rotation = FRotator::ZeroRotator;

		JsonObject->TryGetStringField(TEXT("actor_class"), ActorClassPath);
		JsonObject->TryGetStringField(TEXT("dummy_mesh"), DummyMeshPath);
		if (DummyMeshPath.IsEmpty())
		{
			JsonObject->TryGetStringField(TEXT("body_mesh"), DummyMeshPath);
		}
		JsonObject->TryGetStringField(TEXT("visual_material"), VisualMaterialPath);
		TunaSweeperBunkerRuntimeSpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);
		TunaSweeperBunkerRuntimeSpawn::TryReadVectorField(JsonObject, TEXT("scale"), Scale);

		const FString TrimmedActorClassPath = ActorClassPath.TrimStartAndEnd();
		if (!TrimmedActorClassPath.IsEmpty())
		{
			SpawnDefinition.ActorClass = TSoftClassPtr<ATunaSweeperMoleCompanionActor>(
				FSoftObjectPath(TrimmedActorClassPath));
		}
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.Scale = FVector(
			FMath::Max(0.01f, Scale.X),
			FMath::Max(0.01f, Scale.Y),
			FMath::Max(0.01f, Scale.Z));

		auto ResolveStaticMesh = [](const FString& RawPath)
		{
			const FString TrimmedPath = RawPath.TrimStartAndEnd();
			return TrimmedPath.IsEmpty()
				? TSoftObjectPtr<UStaticMesh>()
				: TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TrimmedPath));
		};
		SpawnDefinition.DummyMesh = ResolveStaticMesh(DummyMeshPath);

		const FString TrimmedVisualMaterialPath = VisualMaterialPath.TrimStartAndEnd();
		if (!TrimmedVisualMaterialPath.IsEmpty())
		{
			SpawnDefinition.VisualMaterial = TSoftObjectPtr<UMaterialInterface>(
				FSoftObjectPath(TrimmedVisualMaterialPath));
		}

		BunkerCharacterSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows && JsonRows.Num() > 0)
	{
		UE_LOG(LogTunaSweeperBunkerRuntimeSpawn, Error, TEXT("Bunker character spawn JSON has no valid rows: %s"), *JsonPath);
		return false;
	}

	bBunkerCharacterSpawnDataLoaded = true;
	return true;
}

void UTunaSweeperBunkerRuntimeSpawnSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	EnsureBunkerRuntimeActorsSpawnedForWorld(LoadedWorld);
}

void UTunaSweeperBunkerRuntimeSpawnSubsystem::ResetLoadedBunkerCharacterSpawnData()
{
	BunkerCharacterSpawnDefinitions.Reset();
	bBunkerCharacterSpawnDataLoaded = false;
}

FString UTunaSweeperBunkerRuntimeSpawnSubsystem::GetBunkerCharacterSpawnJsonPath() const
{
	return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("BunkerCharacterSpawns.json"));
}

bool UTunaSweeperBunkerRuntimeSpawnSubsystem::DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const
{
	if (!World || LevelName.IsNone())
	{
		return false;
	}

	const FString SpawnLevelName = TunaSweeperBunkerRuntimeSpawn::NormalizeLevelName(
		TunaSweeperBuildFlavor::ResolveGameplayLevelName(LevelName).ToString());
	const FString WorldMapName = TunaSweeperBunkerRuntimeSpawn::NormalizeLevelName(World->GetMapName());
	const FString WorldPackageName = TunaSweeperBunkerRuntimeSpawn::NormalizeLevelName(World->GetOutermost()->GetName());
	return SpawnLevelName.Equals(WorldMapName, ESearchCase::IgnoreCase) ||
		SpawnLevelName.Equals(WorldPackageName, ESearchCase::IgnoreCase);
}
