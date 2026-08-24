#include "Subsystem/TunaSweeperMemoSubsystem.h"

#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperMemoActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperMemo, Log, All);

namespace TunaSweeperMemo
{
	const TCHAR* MemoDefinitionsJsonRelativePath = TEXT("Data/MemoDefinitions.json");
	const TCHAR* MemoSpawnsJsonRelativePath = TEXT("Data/MemoSpawns.json");
	const TCHAR* DefaultInteractionMarkerWidgetClassPath = TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C");
	const TCHAR* DefaultMemoVisualMaterialPath = TEXT("/Game/Interaction/M_MemoStorageDevice.M_MemoStorageDevice");

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

void UTunaSweeperMemoSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UTunaSweeperMemoSubsystem::HandlePostLoadMapWithWorld);
}

void UTunaSweeperMemoSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	ResetLoadedMemoDefinitions();
	ResetLoadedMemoSpawnData();
	LastSpawnedWorld.Reset();
	Super::Deinitialize();
}

bool UTunaSweeperMemoSubsystem::LoadMemoDefinitions(bool bForceReload)
{
	if (bMemoDefinitionDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedMemoDefinitions();

	FString JsonContent;
	const FString MemoDefinitionsJsonPath = GetMemoDefinitionsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *MemoDefinitionsJsonPath))
	{
		UE_LOG(LogTunaSweeperMemo, Error, TEXT("Failed to read memo definitions JSON: %s"), *MemoDefinitionsJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperMemo, Error, TEXT("Failed to parse memo definitions JSON: %s"), *MemoDefinitionsJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo definition row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		double NumericMemoId = INDEX_NONE;
		FString Title;
		FString Body;
		if (!JsonObject->TryGetNumberField(TEXT("memo_id"), NumericMemoId) ||
			!JsonObject->TryGetStringField(TEXT("title"), Title) ||
			!JsonObject->TryGetStringField(TEXT("body"), Body))
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo definition row %d: required field is missing."), RowIndex);
			continue;
		}

		FTunaSweeperMemoDefinition Definition;
		Definition.MemoId = static_cast<int32>(NumericMemoId);
		Definition.Title = FText::FromString(Title.TrimStartAndEnd());
		Definition.Body = FText::FromString(Body.TrimStartAndEnd());
		if (Definition.MemoId <= 0 || Definition.Title.IsEmpty())
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo definition row %d: invalid memo id or title."), RowIndex);
			continue;
		}

		MemoDefinitionsById.Add(Definition.MemoId, Definition);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperMemo, Error, TEXT("Memo definitions JSON has no valid rows: %s"), *MemoDefinitionsJsonPath);
		return false;
	}

	bMemoDefinitionDataLoaded = true;
	return true;
}

bool UTunaSweeperMemoSubsystem::LoadMemoSpawnData(bool bForceReload)
{
	if (bMemoSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedMemoSpawnData();

	FString JsonContent;
	const FString MemoSpawnsJsonPath = GetMemoSpawnsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *MemoSpawnsJsonPath))
	{
		UE_LOG(LogTunaSweeperMemo, Error, TEXT("Failed to read memo spawns JSON: %s"), *MemoSpawnsJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperMemo, Error, TEXT("Failed to parse memo spawns JSON: %s"), *MemoSpawnsJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString SpawnId;
		FString ActorClassPath;
		FString InteractionDisplayName;
		FString MarkerWidgetClassPath;
		FString VisualMeshPath;
		FString VisualMaterialPath;
		double NumericMemoId = INDEX_NONE;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector ActorScale = FVector::OneVector;
		FVector VisualScale(0.85f, 0.55f, 0.08f);
		FVector VisualRelativeLocation = FVector::ZeroVector;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!JsonObject->TryGetNumberField(TEXT("memo_id"), NumericMemoId) ||
			!TunaSweeperMemo::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("spawn_id"), SpawnId);
		JsonObject->TryGetStringField(TEXT("actor_class"), ActorClassPath);
		JsonObject->TryGetStringField(TEXT("interaction_display_name"), InteractionDisplayName);
		JsonObject->TryGetStringField(TEXT("marker_widget_class"), MarkerWidgetClassPath);
		JsonObject->TryGetStringField(TEXT("visual_mesh"), VisualMeshPath);
		JsonObject->TryGetStringField(TEXT("visual_material"), VisualMaterialPath);
		TunaSweeperMemo::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);
		TunaSweeperMemo::TryReadVectorField(JsonObject, TEXT("scale"), ActorScale);
		TunaSweeperMemo::TryReadVectorField(JsonObject, TEXT("visual_scale"), VisualScale);
		TunaSweeperMemo::TryReadVectorField(JsonObject, TEXT("visual_relative_location"), VisualRelativeLocation);

		FMemoSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.MemoId = static_cast<int32>(NumericMemoId);
		SpawnDefinition.SpawnId = SpawnId.TrimStartAndEnd().IsEmpty()
			? FName(*FString::Printf(TEXT("memo_%03d"), SpawnDefinition.MemoId))
			: FName(*SpawnId.TrimStartAndEnd());
		if (SpawnDefinition.LevelName.IsNone() || SpawnDefinition.MemoId <= 0)
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo spawn row %d: invalid level or memo id."), RowIndex);
			continue;
		}

		const FString TrimmedActorClassPath = ActorClassPath.TrimStartAndEnd();
		if (!TrimmedActorClassPath.IsEmpty())
		{
			SpawnDefinition.MemoActorClass = TSoftClassPtr<ATunaSweeperMemoActor>(
				FSoftObjectPath(TrimmedActorClassPath));
		}
		SpawnDefinition.MarkerWidgetClass = TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(MarkerWidgetClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperMemo::DefaultInteractionMarkerWidgetClassPath)
				: MarkerWidgetClassPath.TrimStartAndEnd()));
		if (!VisualMeshPath.TrimStartAndEnd().IsEmpty())
		{
			SpawnDefinition.VisualMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(VisualMeshPath.TrimStartAndEnd()));
		}
		SpawnDefinition.VisualMaterial = TSoftObjectPtr<UMaterialInterface>(
			FSoftObjectPath(VisualMaterialPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperMemo::DefaultMemoVisualMaterialPath)
				: VisualMaterialPath.TrimStartAndEnd()));
		SpawnDefinition.InteractionDisplayName = InteractionDisplayName.TrimStartAndEnd().IsEmpty()
			? FText::FromString(TEXT("\uBA54\uBAA8"))
			: FText::FromString(InteractionDisplayName.TrimStartAndEnd());
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.ActorScale = FVector(
			FMath::Max(0.01f, ActorScale.X),
			FMath::Max(0.01f, ActorScale.Y),
			FMath::Max(0.01f, ActorScale.Z));
		SpawnDefinition.VisualScale = FVector(
			FMath::Max(0.01f, VisualScale.X),
			FMath::Max(0.01f, VisualScale.Y),
			FMath::Max(0.01f, VisualScale.Z));
		SpawnDefinition.VisualRelativeLocation = VisualRelativeLocation;

		MemoSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows && JsonRows.Num() > 0)
	{
		UE_LOG(LogTunaSweeperMemo, Error, TEXT("Memo spawns JSON has no valid rows: %s"), *MemoSpawnsJsonPath);
		return false;
	}

	bMemoSpawnDataLoaded = true;
	return true;
}

bool UTunaSweeperMemoSubsystem::EnsureMemosSpawnedForWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return true;
	}

	if (LastSpawnedWorld.Get() == World)
	{
		return true;
	}

	if (!LoadMemoSpawnData(false))
	{
		return false;
	}
	LoadMemoDefinitions(false);

	LastSpawnedWorld = World;
	int32 SpawnedMemoCount = 0;
	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	for (const FMemoSpawnDefinition& SpawnDefinition : MemoSpawnDefinitions)
	{
		if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
		{
			continue;
		}
		if (TunaGameInstance && TunaGameInstance->IsMemoAcquired(SpawnDefinition.MemoId))
		{
			continue;
		}

		FTunaSweeperMemoDefinition MemoDefinition;
		if (!TryGetMemoDefinition(SpawnDefinition.MemoId, MemoDefinition))
		{
			UE_LOG(LogTunaSweeperMemo, Warning, TEXT("Skipping memo spawn %s: memo definition %d is missing."), *SpawnDefinition.SpawnId.ToString(), SpawnDefinition.MemoId);
			continue;
		}

		TSubclassOf<ATunaSweeperMemoActor> LoadedMemoActorClass = SpawnDefinition.MemoActorClass.LoadSynchronous();
		if (!LoadedMemoActorClass)
		{
			LoadedMemoActorClass = ATunaSweeperMemoActor::StaticClass();
		}

		const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location, SpawnDefinition.ActorScale);
		ATunaSweeperMemoActor* SpawnedMemo = World->SpawnActorDeferred<ATunaSweeperMemoActor>(
			LoadedMemoActorClass,
			SpawnTransform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!SpawnedMemo)
		{
			continue;
		}

		SpawnedMemo->ConfigureMemoDefaults(
			SpawnDefinition.MemoId,
			SpawnDefinition.InteractionDisplayName,
			SpawnDefinition.MarkerWidgetClass,
			SpawnDefinition.VisualMesh,
			SpawnDefinition.VisualMaterial,
			SpawnDefinition.VisualScale,
			SpawnDefinition.VisualRelativeLocation);
		if (!SpawnDefinition.SpawnId.IsNone())
		{
			SpawnedMemo->Tags.AddUnique(SpawnDefinition.SpawnId);
#if WITH_EDITOR
			SpawnedMemo->SetActorLabel(SpawnDefinition.SpawnId.ToString());
#endif
		}
		UGameplayStatics::FinishSpawningActor(SpawnedMemo, SpawnTransform);
		++SpawnedMemoCount;
	}

	UE_LOG(LogTunaSweeperMemo, Log, TEXT("Spawned %d memo actors for level %s."), SpawnedMemoCount, *World->GetMapName());
	return true;
}

bool UTunaSweeperMemoSubsystem::TryGetMemoDefinition(int32 MemoId, FTunaSweeperMemoDefinition& OutDefinition)
{
	if (!LoadMemoDefinitions(false))
	{
		OutDefinition = FTunaSweeperMemoDefinition();
		return false;
	}

	if (const FTunaSweeperMemoDefinition* FoundDefinition = MemoDefinitionsById.Find(MemoId))
	{
		OutDefinition = *FoundDefinition;
		return true;
	}

	OutDefinition = FTunaSweeperMemoDefinition();
	return false;
}

void UTunaSweeperMemoSubsystem::GetMemoListEntries(TArray<FTunaSweeperMemoListEntry>& OutEntries)
{
	OutEntries.Reset();
	if (!LoadMemoDefinitions(false))
	{
		return;
	}

	TArray<int32> MemoIds;
	MemoDefinitionsById.GenerateKeyArray(MemoIds);
	MemoIds.Sort();

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	for (int32 MemoId : MemoIds)
	{
		const FTunaSweeperMemoDefinition* Definition = MemoDefinitionsById.Find(MemoId);
		if (!Definition)
		{
			continue;
		}

		FTunaSweeperMemoListEntry Entry;
		Entry.MemoId = MemoId;
		Entry.Title = Definition->Title;
		Entry.Body = Definition->Body;
		Entry.bAcquired = TunaGameInstance && TunaGameInstance->IsMemoAcquired(MemoId);
		OutEntries.Add(Entry);
	}
}

int32 UTunaSweeperMemoSubsystem::GetFirstAcquiredMemoId()
{
	TArray<FTunaSweeperMemoListEntry> Entries;
	GetMemoListEntries(Entries);
	for (const FTunaSweeperMemoListEntry& Entry : Entries)
	{
		if (Entry.bAcquired)
		{
			return Entry.MemoId;
		}
	}

	return INDEX_NONE;
}

void UTunaSweeperMemoSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	EnsureMemosSpawnedForWorld(LoadedWorld);
}

void UTunaSweeperMemoSubsystem::ResetLoadedMemoDefinitions()
{
	MemoDefinitionsById.Reset();
	bMemoDefinitionDataLoaded = false;
}

void UTunaSweeperMemoSubsystem::ResetLoadedMemoSpawnData()
{
	MemoSpawnDefinitions.Reset();
	bMemoSpawnDataLoaded = false;
}

FString UTunaSweeperMemoSubsystem::GetMemoDefinitionsJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperMemo::MemoDefinitionsJsonRelativePath);
}

FString UTunaSweeperMemoSubsystem::GetMemoSpawnsJsonPath() const
{
	return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("MemoSpawns.json"));
}

bool UTunaSweeperMemoSubsystem::DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const
{
	if (!World || LevelName.IsNone())
	{
		return false;
	}

	const FString SpawnLevelName = TunaSweeperMemo::NormalizeLevelName(
		TunaSweeperBuildFlavor::ResolveGameplayLevelName(LevelName).ToString());
	const FString WorldMapName = TunaSweeperMemo::NormalizeLevelName(World->GetMapName());
	const FString WorldPackageName = TunaSweeperMemo::NormalizeLevelName(World->GetOutermost()->GetName());
	return SpawnLevelName.Equals(WorldMapName, ESearchCase::IgnoreCase) ||
		SpawnLevelName.Equals(WorldPackageName, ESearchCase::IgnoreCase);
}
