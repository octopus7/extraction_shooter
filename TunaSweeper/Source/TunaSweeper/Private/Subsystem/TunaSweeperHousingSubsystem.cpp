#include "Subsystem/TunaSweeperHousingSubsystem.h"

#include "Dom/JsonObject.h"
#include "DrawDebugHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Actor.h"
#include "Housing/TunaSweeperHousingAreaActor.h"
#include "Housing/TunaSweeperHousingFacilityActor.h"
#include "Interaction/TunaSweeperHousingManagementActor.h"
#include "Interaction/TunaSweeperPiggyBankActor.h"
#include "Interaction/TunaSweeperWorkbenchActor.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "SceneManagement.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperUiText.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperHousing, Log, All);

namespace TunaSweeperHousing
{
	const TCHAR* FacilityDefinitionsJsonRelativePath = TEXT("Data/HousingFacilityDefinitions.json");
	const FName BunkerMapName(TEXT("BunkerMap"));
	constexpr float DefaultFacilityHeight = 70.0f;
	constexpr float PreviewFacilityHeight = 42.0f;
	constexpr float DebugLifeTime = 0.035f;
	constexpr float CellDebugZOffset = 3.0f;
	constexpr float InvalidCellDebugZOffset = 9.0f;
	constexpr float CellRoundRadiusRatio = 0.18f;
	constexpr float CellRoundRadiusMax = 18.0f;
	constexpr float CellOutlineMargin = 8.0f;
	constexpr float CellOutlineThickness = 2.0f;
	constexpr float InvalidCellOutlineThickness = 4.0f;
	constexpr float PlacedActorZOffset = 2.0f;
	const FColor HousingCellColor(70, 210, 255, 255);
	const FColor InvalidCellColor(255, 38, 38, 255);
	const FVector DefaultManagementActorLocation(-420.0f, -560.0f, 60.0f);
	const FRotator DefaultManagementActorRotation(0.0f, 18.0f, 0.0f);

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

	bool TryReadIntPointField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, FIntPoint& OutPoint)
	{
		const TArray<TSharedPtr<FJsonValue>>* ValueArray = nullptr;
		if (!JsonObject.IsValid() || !JsonObject->TryGetArrayField(FieldName, ValueArray) || !ValueArray || ValueArray->Num() < 2)
		{
			return false;
		}

		OutPoint = FIntPoint(
			FMath::Max(1, static_cast<int32>((*ValueArray)[0]->AsNumber())),
			FMath::Max(1, static_cast<int32>((*ValueArray)[1]->AsNumber())));
		return true;
	}

	void ReadIntArrayField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, TArray<int32>& OutValues)
	{
		const TArray<TSharedPtr<FJsonValue>>* ValueArray = nullptr;
		if (!JsonObject.IsValid() || !JsonObject->TryGetArrayField(FieldName, ValueArray) || !ValueArray)
		{
			return;
		}

		for (const TSharedPtr<FJsonValue>& Value : *ValueArray)
		{
			if (!Value.IsValid())
			{
				continue;
			}

			const int32 ItemId = FMath::RoundToInt(Value->AsNumber());
			if (ItemId > 0)
			{
				OutValues.AddUnique(ItemId);
			}
		}
	}

	FString SanitizeStringField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		FString Value;
		if (JsonObject.IsValid())
		{
			JsonObject->TryGetStringField(FieldName, Value);
		}
		return Value.TrimStartAndEnd();
	}

	int32 NormalizeQuarterTurns(int32 QuarterTurns)
	{
		int32 NormalizedTurns = QuarterTurns % 4;
		if (NormalizedTurns < 0)
		{
			NormalizedTurns += 4;
		}
		return NormalizedTurns;
	}

	FText ResolveUiText(const UGameInstance* GameInstance, const TCHAR* StringKey, const TCHAR* Fallback)
	{
		return TunaSweeperUiText::ResolveUiText(
			Cast<UTunaSweeperGameInstance>(GameInstance),
			StringKey,
			Fallback);
	}
}

void UTunaSweeperHousingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UTunaSweeperHousingSubsystem::HandlePostLoadMapWithWorld);
}

void UTunaSweeperHousingSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	ResetRuntimeWorldState();
	ResetLoadedFacilityDefinitions();
	SavedFacilities.Reset();
	bSavedFacilitiesLoaded = false;
	Super::Deinitialize();
}

bool UTunaSweeperHousingSubsystem::EnsureHousingForWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return true;
	}

	if (!IsBunkerWorld(World))
	{
		if (ActiveHousingArea.IsValid())
		{
			ResetRuntimeWorldState();
		}
		return true;
	}

	if (!LoadHousingFacilityDefinitions(false))
	{
		return false;
	}

	LoadSavedFacilitiesFromGameInstance();
	ActiveHousingArea = ResolveActiveAreaForWorld(World);
	if (!ActiveHousingArea.IsValid())
	{
		ActiveHousingArea = SpawnDefaultHousingArea(World);
	}

	EnsureHousingManagementActorForWorld(World);
	RefreshSpawnedFacilities();
	BroadcastHousingChanged();
	return ActiveHousingArea.IsValid();
}

void UTunaSweeperHousingSubsystem::RegisterHousingArea(ATunaSweeperHousingAreaActor* HousingArea)
{
	if (!HousingArea)
	{
		return;
	}

	RegisteredHousingAreas.AddUnique(HousingArea);
	if (IsBunkerWorld(HousingArea->GetWorld()))
	{
		ActiveHousingArea = HousingArea;
		LoadHousingFacilityDefinitions(false);
		LoadSavedFacilitiesFromGameInstance();
		RefreshSpawnedFacilities();
		BroadcastHousingChanged();
	}
}

void UTunaSweeperHousingSubsystem::UnregisterHousingArea(ATunaSweeperHousingAreaActor* HousingArea)
{
	if (!HousingArea)
	{
		return;
	}

	RegisteredHousingAreas.RemoveAll([HousingArea](const TWeakObjectPtr<ATunaSweeperHousingAreaActor>& RegisteredArea)
	{
		return !RegisteredArea.IsValid() || RegisteredArea.Get() == HousingArea;
	});

	if (ActiveHousingArea.Get() == HousingArea)
	{
		ActiveHousingArea.Reset();
		DestroySpawnedFacilities();
		DestroyPreviewActor();
		ActivePlacement = FActiveHousingPlacement();
		ActivePlacementStatus = ETunaSweeperHousingPlacementStatus::None;
		BroadcastHousingChanged();
	}
}

bool UTunaSweeperHousingSubsystem::LoadHousingFacilityDefinitions(bool bForceReload)
{
	if (bFacilityDefinitionsLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedFacilityDefinitions();

	FString JsonContent;
	const FString JsonPath = GetHousingFacilityDefinitionsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		UE_LOG(LogTunaSweeperHousing, Error, TEXT("Failed to read housing facility definition JSON: %s"), *JsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperHousing, Error, TEXT("Failed to parse housing facility definition JSON: %s"), *JsonPath);
		return false;
	}

	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperHousing, Warning, TEXT("Skipping housing facility row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		const FString FacilityId = TunaSweeperHousing::SanitizeStringField(JsonObject, TEXT("facility_id"));
		if (FacilityId.IsEmpty())
		{
			UE_LOG(LogTunaSweeperHousing, Warning, TEXT("Skipping housing facility row %d: facility_id is missing."), RowIndex);
			continue;
		}

		FIntPoint SizeCells(1, 1);
		TunaSweeperHousing::TryReadIntPointField(JsonObject, TEXT("size_cells"), SizeCells);

		FTunaSweeperHousingFacilityDefinition Definition;
		Definition.FacilityId = FName(*FacilityId);
		Definition.DisplayNameStringKey = FName(*TunaSweeperHousing::SanitizeStringField(JsonObject, TEXT("display_name_key")));
		Definition.DescriptionStringKey = FName(*TunaSweeperHousing::SanitizeStringField(JsonObject, TEXT("description_key")));
		Definition.FallbackDisplayName = FText::FromString(TunaSweeperHousing::SanitizeStringField(JsonObject, TEXT("display_name")));
		Definition.FallbackDescription = FText::FromString(TunaSweeperHousing::SanitizeStringField(JsonObject, TEXT("description")));
		Definition.SizeX = SizeCells.X;
		Definition.SizeY = SizeCells.Y;
		TunaSweeperHousing::ReadIntArrayField(
			JsonObject,
			TEXT("unlock_when_ever_acquired_items"),
			Definition.UnlockWhenEverAcquiredItemIds);
		Definition.ActorClassPath = TunaSweeperHousing::SanitizeStringField(JsonObject, TEXT("actor_class"));
		Definition.StaticMeshPath = TunaSweeperHousing::SanitizeStringField(JsonObject, TEXT("static_mesh"));
		Definition.MaterialPath = TunaSweeperHousing::SanitizeStringField(JsonObject, TEXT("material"));

		double SortOrder = 0.0;
		if (JsonObject->TryGetNumberField(TEXT("sort_order"), SortOrder))
		{
			Definition.SortOrder = static_cast<int32>(SortOrder);
		}

		bool bUnlocked = true;
		if (JsonObject->TryGetBoolField(TEXT("function_unlocked"), bUnlocked))
		{
			Definition.bFunctionUnlockedByDefault = bUnlocked;
		}

		const TArray<TSharedPtr<FJsonValue>>* MaterialRows = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("required_materials"), MaterialRows) && MaterialRows)
		{
			for (const TSharedPtr<FJsonValue>& MaterialValue : *MaterialRows)
			{
				const TSharedPtr<FJsonObject>* MaterialObjectPtr = nullptr;
				if (!MaterialValue.IsValid() || !MaterialValue->TryGetObject(MaterialObjectPtr) ||
					!MaterialObjectPtr || !MaterialObjectPtr->IsValid())
				{
					continue;
				}

				double ItemId = INDEX_NONE;
				double Quantity = 1.0;
				if (!(*MaterialObjectPtr)->TryGetNumberField(TEXT("item_id"), ItemId))
				{
					continue;
				}

				(*MaterialObjectPtr)->TryGetNumberField(TEXT("quantity"), Quantity);
				FTunaSweeperHousingMaterialCost MaterialCost;
				MaterialCost.ItemId = static_cast<int32>(ItemId);
				MaterialCost.Quantity = FMath::Max(1, static_cast<int32>(Quantity));
				if (MaterialCost.ItemId != INDEX_NONE)
				{
					Definition.RequiredMaterials.Add(MaterialCost);
				}
			}
		}

		FacilityDefinitions.Add(Definition);
		FacilityDefinitionsById.Add(Definition.FacilityId, Definition);
	}

	FacilityDefinitions.Sort([](
		const FTunaSweeperHousingFacilityDefinition& Left,
		const FTunaSweeperHousingFacilityDefinition& Right)
	{
		if (Left.SortOrder != Right.SortOrder)
		{
			return Left.SortOrder < Right.SortOrder;
		}
		return Left.FacilityId.LexicalLess(Right.FacilityId);
	});

	bFacilityDefinitionsLoaded = FacilityDefinitions.Num() > 0;
	return bFacilityDefinitionsLoaded;
}

void UTunaSweeperHousingSubsystem::GetFacilityViews(TArray<FTunaSweeperHousingFacilityView>& OutViews)
{
	OutViews.Reset();
	LoadHousingFacilityDefinitions(false);
	LoadSavedFacilitiesFromGameInstance();

	TMap<FName, int32> OwnedCountsByFacilityId;
	for (const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility : SavedFacilities)
	{
		if (!SavedFacility.FacilityId.IsNone())
		{
			++OwnedCountsByFacilityId.FindOrAdd(SavedFacility.FacilityId);
		}
	}

	for (const FTunaSweeperHousingFacilityDefinition& Definition : FacilityDefinitions)
	{
		const bool bOwned = OwnedCountsByFacilityId.Contains(Definition.FacilityId);
		if (!IsFacilityFunctionUnlocked(Definition) && !bOwned)
		{
			continue;
		}

		for (const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility : SavedFacilities)
		{
			if (SavedFacility.FacilityId != Definition.FacilityId)
			{
				continue;
			}

			FTunaSweeperHousingFacilityView View;
			View.InstanceId = SavedFacility.InstanceId;
			View.FacilityId = Definition.FacilityId;
			View.DisplayName = ResolveFacilityDisplayName(Definition);
			View.Description = ResolveFacilityDescription(Definition);
			View.MaterialsText = FText::GetEmpty();
			View.BuildState = SavedFacility.bStored
				? ETunaSweeperHousingFacilityBuildState::Stored
				: ETunaSweeperHousingFacilityBuildState::Placed;
			View.StateText = BuildStateText(View.BuildState);
			View.SizeX = Definition.SizeX;
			View.SizeY = Definition.SizeY;
			View.bCanStartPlacement = SavedFacility.bStored;
			View.bCanStore = !SavedFacility.bStored;
			OutViews.Add(View);
		}

		if (OwnedCountsByFacilityId.Contains(Definition.FacilityId))
		{
			continue;
		}

		const bool bHasMaterials = HasEnoughMaterials(Definition);
		FTunaSweeperHousingFacilityView View;
		View.FacilityId = Definition.FacilityId;
		View.DisplayName = ResolveFacilityDisplayName(Definition);
		View.Description = ResolveFacilityDescription(Definition);
		View.MaterialsText = BuildMaterialsText(Definition);
		View.BuildState = bHasMaterials
			? ETunaSweeperHousingFacilityBuildState::Buildable
			: ETunaSweeperHousingFacilityBuildState::InsufficientMaterials;
		View.StateText = BuildStateText(View.BuildState);
		View.SizeX = Definition.SizeX;
		View.SizeY = Definition.SizeY;
		View.bCanStartPlacement = bHasMaterials;
		View.bCanStore = false;
		OutViews.Add(View);
	}
}

bool UTunaSweeperHousingSubsystem::TryGetFacilityDefinition(
	FName FacilityId,
	FTunaSweeperHousingFacilityDefinition& OutDefinition) const
{
	return TryGetDefinition(FacilityId, OutDefinition);
}

bool UTunaSweeperHousingSubsystem::OpenHousingMode(APlayerController* PlayerController)
{
	if (!PlayerController || !EnsureHousingForWorld(PlayerController->GetWorld()))
	{
		return false;
	}

	bHousingModeOpen = true;
	BroadcastHousingChanged();
	return true;
}

void UTunaSweeperHousingSubsystem::CloseHousingMode()
{
	const bool bWasOpen = bHousingModeOpen;
	if (HasActivePlacement())
	{
		CancelPlacement();
	}

	bHousingModeOpen = false;
	if (bWasOpen)
	{
		BroadcastHousingChanged();
	}
}

bool UTunaSweeperHousingSubsystem::UpdateHousingMode(APlayerController* PlayerController)
{
	if (!bHousingModeOpen)
	{
		return false;
	}

	if (PlayerController)
	{
		EnsureHousingForWorld(PlayerController->GetWorld());
	}

	if (HasActivePlacement())
	{
		return UpdatePlacementPreview(PlayerController);
	}

	DrawHousingGridDebug(nullptr);
	return true;
}

bool UTunaSweeperHousingSubsystem::StartPlacement(FName FacilityId, FGuid ExistingInstanceId)
{
	LoadHousingFacilityDefinitions(false);
	LoadSavedFacilitiesFromGameInstance();

	FTunaSweeperHousingFacilityDefinition Definition;
	if (!TryGetDefinition(FacilityId, Definition))
	{
		return false;
	}

	ActivePlacement = FActiveHousingPlacement();
	ActivePlacement.FacilityId = FacilityId;
	ActivePlacement.ExistingInstanceId = ExistingInstanceId;
	if (ExistingInstanceId.IsValid())
	{
		const FTunaSweeperHousingPlacedFacilitySaveData* ExistingFacility = SavedFacilities.FindByPredicate(
			[ExistingInstanceId](const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility)
			{
				return SavedFacility.InstanceId == ExistingInstanceId;
			});
		if (!ExistingFacility || ExistingFacility->FacilityId != FacilityId || !ExistingFacility->bStored)
		{
			ActivePlacement = FActiveHousingPlacement();
			return false;
		}
		ActivePlacement.RotationQuarterTurns = TunaSweeperHousing::NormalizeQuarterTurns(ExistingFacility->RotationQuarterTurns);
	}
	else if (!IsFacilityFunctionUnlocked(Definition) || !HasEnoughMaterials(Definition))
	{
		ActivePlacement = FActiveHousingPlacement();
		return false;
	}

	ActivePlacementStatus = ETunaSweeperHousingPlacementStatus::None;
	bHousingModeOpen = true;
	DestroyPreviewActor();
	BroadcastHousingChanged();
	return true;
}

void UTunaSweeperHousingSubsystem::CancelPlacement()
{
	if (!HasActivePlacement())
	{
		return;
	}

	ActivePlacement = FActiveHousingPlacement();
	ActivePlacementStatus = ETunaSweeperHousingPlacementStatus::None;
	DestroyPreviewActor();
	BroadcastHousingChanged();
}

bool UTunaSweeperHousingSubsystem::RotateActivePlacement(int32 QuarterTurnDelta)
{
	if (!HasActivePlacement() || QuarterTurnDelta == 0)
	{
		return false;
	}

	ActivePlacement.RotationQuarterTurns = TunaSweeperHousing::NormalizeQuarterTurns(
		ActivePlacement.RotationQuarterTurns + QuarterTurnDelta);
	ActivePlacement.bHasPreviewCell = false;
	BroadcastHousingChanged();
	return true;
}

bool UTunaSweeperHousingSubsystem::UpdatePlacementPreview(APlayerController* PlayerController)
{
	if (!HasActivePlacement())
	{
		return false;
	}

	if (!PlayerController)
	{
		DrawHousingGridDebug(nullptr);
		return false;
	}

	EnsureHousingForWorld(PlayerController->GetWorld());

	FTunaSweeperHousingPlacedFacilitySaveData PreviewPlacement;
	ETunaSweeperHousingPlacementStatus PlacementStatus = ETunaSweeperHousingPlacementStatus::None;
	const bool bResolvedPlacement = TryResolvePlacementAtMouse(PlayerController, PreviewPlacement, PlacementStatus);
	ActivePlacementStatus = PlacementStatus;
	if (!bResolvedPlacement)
	{
		DrawHousingGridDebug(nullptr);
		return false;
	}

	ActivePlacement.AnchorCell = PreviewPlacement.AnchorCell;
	ActivePlacement.RotationQuarterTurns = PreviewPlacement.RotationQuarterTurns;
	ActivePlacement.bHasPreviewCell = true;

	FTunaSweeperHousingFacilityDefinition Definition;
	if (!TryGetDefinition(PreviewPlacement.FacilityId, Definition) || !ActiveHousingArea.IsValid())
	{
		DrawHousingGridDebug(&PreviewPlacement);
		return false;
	}

	const TSubclassOf<AActor> DesiredPreviewActorClass = ResolveFacilityActorClass(Definition);
	if (IsValid(PreviewActor) && !PreviewActor->IsA(DesiredPreviewActorClass))
	{
		DestroyPreviewActor();
	}

	if (!PreviewActor)
	{
		UWorld* World = PlayerController->GetWorld();
		PreviewActor = World
			? World->SpawnActor<AActor>(
				DesiredPreviewActorClass,
				BuildPlacedActorWorldTransform(Definition, PreviewPlacement))
			: nullptr;
	}

	const bool bPlacementValid = PlacementStatus == ETunaSweeperHousingPlacementStatus::Valid;
	if (PreviewActor)
	{
		ConfigureFacilityActor(
			PreviewActor,
			Definition,
			PreviewPlacement,
			true,
			bPlacementValid);
	}

	DrawPlacementDebug(PreviewPlacement, bPlacementValid);
	return bPlacementValid;
}

bool UTunaSweeperHousingSubsystem::TryCommitPlacement(APlayerController* PlayerController)
{
	if (!HasActivePlacement())
	{
		return false;
	}

	FTunaSweeperHousingPlacedFacilitySaveData Placement;
	ETunaSweeperHousingPlacementStatus PlacementStatus = ETunaSweeperHousingPlacementStatus::None;
	if (!TryResolvePlacementAtMouse(PlayerController, Placement, PlacementStatus) ||
		PlacementStatus != ETunaSweeperHousingPlacementStatus::Valid)
	{
		ActivePlacementStatus = PlacementStatus;
		if (Placement.FacilityId != NAME_None)
		{
			DrawPlacementDebug(Placement, false);
		}
		BroadcastHousingChanged();
		return false;
	}

	FTunaSweeperHousingFacilityDefinition Definition;
	if (!TryGetDefinition(Placement.FacilityId, Definition))
	{
		return false;
	}

	if (ActivePlacement.ExistingInstanceId.IsValid())
	{
		FTunaSweeperHousingPlacedFacilitySaveData* ExistingFacility = SavedFacilities.FindByPredicate(
			[this](const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility)
			{
				return SavedFacility.InstanceId == ActivePlacement.ExistingInstanceId;
			});
		if (!ExistingFacility)
		{
			return false;
		}

		ExistingFacility->AnchorCell = Placement.AnchorCell;
		ExistingFacility->RotationQuarterTurns = Placement.RotationQuarterTurns;
		ExistingFacility->bStored = false;
	}
	else
	{
		if (!ConsumeRequiredMaterials(Definition))
		{
			return false;
		}

		Placement.InstanceId = FGuid::NewGuid();
		Placement.bStored = false;
		SavedFacilities.Add(Placement);
	}

	ActivePlacement = FActiveHousingPlacement();
	ActivePlacementStatus = ETunaSweeperHousingPlacementStatus::None;
	DestroyPreviewActor();
	PersistSavedFacilitiesToGameInstance(true);
	RefreshSpawnedFacilities();
	BroadcastHousingChanged();
	return true;
}

bool UTunaSweeperHousingSubsystem::StoreFacility(FGuid InstanceId, bool bSaveImmediately)
{
	if (!InstanceId.IsValid())
	{
		UE_LOG(LogTunaSweeperHousing, Warning, TEXT("StoreFacility failed: invalid instance id."));
		return false;
	}

	LoadSavedFacilitiesFromGameInstance();
	FTunaSweeperHousingPlacedFacilitySaveData* ExistingFacility = SavedFacilities.FindByPredicate(
		[InstanceId](const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility)
		{
			return SavedFacility.InstanceId == InstanceId;
		});
	if (!ExistingFacility || ExistingFacility->bStored)
	{
		UE_LOG(
			LogTunaSweeperHousing,
			Warning,
			TEXT("StoreFacility failed: instance not found or already stored. InstanceId=%s"),
			*InstanceId.ToString());
		return false;
	}

	ExistingFacility->bStored = true;
	if (ActivePlacement.ExistingInstanceId == InstanceId)
	{
		ActivePlacement = FActiveHousingPlacement();
		ActivePlacementStatus = ETunaSweeperHousingPlacementStatus::None;
		DestroyPreviewActor();
	}

	PersistSavedFacilitiesToGameInstance(bSaveImmediately);
	RefreshSpawnedFacilities();
	BroadcastHousingChanged();
	UE_LOG(LogTunaSweeperHousing, Log, TEXT("Stored housing facility. InstanceId=%s"), *InstanceId.ToString());
	return true;
}

bool UTunaSweeperHousingSubsystem::TryGetPlacedFacilityAtMouse(
	APlayerController* PlayerController,
	FTunaSweeperHousingPlacedFacilitySaveData& OutFacility)
{
	OutFacility = FTunaSweeperHousingPlacedFacilitySaveData();
	if (!PlayerController || !bHousingModeOpen)
	{
		return false;
	}

	EnsureHousingForWorld(PlayerController->GetWorld());
	LoadSavedFacilitiesFromGameInstance();
	if (!ActiveHousingArea.IsValid())
	{
		return false;
	}

	auto TryFindByInstanceId = [this, &OutFacility](const FGuid& InstanceId)
	{
		if (!InstanceId.IsValid())
		{
			return false;
		}

		const FTunaSweeperHousingPlacedFacilitySaveData* FoundFacility = SavedFacilities.FindByPredicate(
			[InstanceId](const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility)
			{
				return SavedFacility.InstanceId == InstanceId && !SavedFacility.bStored && SavedFacility.IsValid();
			});
		if (!FoundFacility)
		{
			return false;
		}

		OutFacility = *FoundFacility;
		return true;
	};

	auto TryFindByActor = [this, &OutFacility, &TryFindByInstanceId](const AActor* Actor)
	{
		if (!Actor)
		{
			return false;
		}

		if (const ATunaSweeperHousingFacilityActor* FacilityActor = Cast<ATunaSweeperHousingFacilityActor>(Actor))
		{
			if (TryFindByInstanceId(FacilityActor->GetInstanceId()))
			{
				return true;
			}
		}

		for (const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility : SavedFacilities)
		{
			if (SavedFacility.bStored || !SavedFacility.IsValid())
			{
				continue;
			}

			const FName ActorId = BuildPlacedFacilityActorId(SavedFacility);
			if (!ActorId.IsNone() && Actor->Tags.Contains(ActorId))
			{
				OutFacility = SavedFacility;
				return true;
			}
		}

		return false;
	};

	FHitResult CursorHit;
	if (PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, CursorHit))
	{
		if (TryFindByActor(CursorHit.GetActor()))
		{
			return true;
		}
	}

	FVector MouseWorldPoint;
	if (!TryGetMouseWorldPoint(PlayerController, MouseWorldPoint))
	{
		return false;
	}

	const float CellSize = FMath::Max(1.0f, ActiveHousingArea->GetCellSize());
	const float HalfWidth = static_cast<float>(FMath::Max(1, ActiveHousingArea->GetGridSizeX())) * CellSize * 0.5f;
	const float HalfHeight = static_cast<float>(FMath::Max(1, ActiveHousingArea->GetGridSizeY())) * CellSize * 0.5f;
	const FVector LocalLocation = ActiveHousingArea->GetActorTransform().InverseTransformPosition(MouseWorldPoint);
	const FVector2D FractionalCell(
		(LocalLocation.X + HalfWidth) / CellSize,
		(LocalLocation.Y + HalfHeight) / CellSize);

	for (const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility : SavedFacilities)
	{
		if (SavedFacility.bStored || !SavedFacility.IsValid())
		{
			continue;
		}

		FTunaSweeperHousingFacilityDefinition Definition;
		if (!TryGetDefinition(SavedFacility.FacilityId, Definition))
		{
			continue;
		}

		const FIntPoint FootprintSize = GetRotatedFootprintSize(Definition, SavedFacility.RotationQuarterTurns);
		if (FractionalCell.X >= static_cast<float>(SavedFacility.AnchorCell.X) &&
			FractionalCell.X < static_cast<float>(SavedFacility.AnchorCell.X + FootprintSize.X) &&
			FractionalCell.Y >= static_cast<float>(SavedFacility.AnchorCell.Y) &&
			FractionalCell.Y < static_cast<float>(SavedFacility.AnchorCell.Y + FootprintSize.Y))
		{
			OutFacility = SavedFacility;
			return true;
		}
	}

	return false;
}

void UTunaSweeperHousingSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	EnsureHousingForWorld(LoadedWorld);
}

void UTunaSweeperHousingSubsystem::ResetLoadedFacilityDefinitions()
{
	FacilityDefinitionsById.Reset();
	FacilityDefinitions.Reset();
	bFacilityDefinitionsLoaded = false;
}

void UTunaSweeperHousingSubsystem::ResetRuntimeWorldState()
{
	if (RuntimeHousingManagementActor.IsValid())
	{
		RuntimeHousingManagementActor->Destroy();
		RuntimeHousingManagementActor.Reset();
	}

	ActiveHousingArea.Reset();
	RegisteredHousingAreas.Reset();
	DestroySpawnedFacilities();
	DestroyPreviewActor();
	ActivePlacement = FActiveHousingPlacement();
	ActivePlacementStatus = ETunaSweeperHousingPlacementStatus::None;
	bHousingModeOpen = false;
}

void UTunaSweeperHousingSubsystem::LoadSavedFacilitiesFromGameInstance()
{
	SavedFacilities.Reset();
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->GetHousingFacilities(SavedFacilities);
	}

	TSet<FGuid> SeenInstanceIds;
	for (int32 Index = SavedFacilities.Num() - 1; Index >= 0; --Index)
	{
		FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility = SavedFacilities[Index];
		SavedFacility.RotationQuarterTurns = TunaSweeperHousing::NormalizeQuarterTurns(SavedFacility.RotationQuarterTurns);
		if (!SavedFacility.IsValid() || SeenInstanceIds.Contains(SavedFacility.InstanceId))
		{
			SavedFacilities.RemoveAtSwap(Index);
			continue;
		}

		SeenInstanceIds.Add(SavedFacility.InstanceId);
	}

	bSavedFacilitiesLoaded = true;
}

void UTunaSweeperHousingSubsystem::PersistSavedFacilitiesToGameInstance(bool bSaveImmediately)
{
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->SetHousingFacilities(SavedFacilities, bSaveImmediately);
	}
}

void UTunaSweeperHousingSubsystem::RefreshSpawnedFacilities()
{
	if (!ActiveHousingArea.IsValid())
	{
		DestroySpawnedFacilities();
		return;
	}

	TSet<FGuid> DesiredInstanceIds;
	for (const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility : SavedFacilities)
	{
		FTunaSweeperHousingFacilityDefinition Definition;
		if (SavedFacility.bStored || !SavedFacility.IsValid() || !TryGetDefinition(SavedFacility.FacilityId, Definition))
		{
			continue;
		}

		DesiredInstanceIds.Add(SavedFacility.InstanceId);
		const TSubclassOf<AActor> DesiredActorClass = ResolveFacilityActorClass(Definition);
		AActor* SpawnedActor = SpawnedFacilityActors.FindRef(SavedFacility.InstanceId);
		if (IsValid(SpawnedActor) && !SpawnedActor->IsA(DesiredActorClass))
		{
			SpawnedActor->Destroy();
			SpawnedActor = nullptr;
			SpawnedFacilityActors.Remove(SavedFacility.InstanceId);
		}

		if (!IsValid(SpawnedActor) || SpawnedActor->GetWorld() != ActiveHousingArea->GetWorld())
		{
			UWorld* World = ActiveHousingArea->GetWorld();
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnedActor = World
				? World->SpawnActor<AActor>(
					DesiredActorClass,
					BuildPlacedActorWorldTransform(Definition, SavedFacility),
					SpawnParameters)
				: nullptr;
			if (SpawnedActor)
			{
				SpawnedFacilityActors.Add(SavedFacility.InstanceId, SpawnedActor);
			}
		}

		if (SpawnedActor)
		{
			ConfigurePlacedFacilityActor(SpawnedActor, Definition, SavedFacility);
		}
	}

	TArray<FGuid> InstanceIdsToRemove;
	for (const TPair<FGuid, TObjectPtr<AActor>>& SpawnedPair : SpawnedFacilityActors)
	{
		if (!DesiredInstanceIds.Contains(SpawnedPair.Key))
		{
			InstanceIdsToRemove.Add(SpawnedPair.Key);
			if (IsValid(SpawnedPair.Value))
			{
				SpawnedPair.Value->Destroy();
			}
		}
	}

	for (const FGuid& InstanceId : InstanceIdsToRemove)
	{
		SpawnedFacilityActors.Remove(InstanceId);
	}
}

void UTunaSweeperHousingSubsystem::DestroySpawnedFacilities()
{
	for (const TPair<FGuid, TObjectPtr<AActor>>& SpawnedPair : SpawnedFacilityActors)
	{
		if (IsValid(SpawnedPair.Value))
		{
			SpawnedPair.Value->Destroy();
		}
	}
	SpawnedFacilityActors.Reset();
}

void UTunaSweeperHousingSubsystem::DestroyPreviewActor()
{
	if (IsValid(PreviewActor))
	{
		PreviewActor->Destroy();
	}
	PreviewActor = nullptr;
}

void UTunaSweeperHousingSubsystem::BroadcastHousingChanged()
{
	OnHousingStateChanged.Broadcast();
}

bool UTunaSweeperHousingSubsystem::IsBunkerWorld(const UWorld* World) const
{
	if (!World)
	{
		return false;
	}

	const FString MapName = TunaSweeperHousing::NormalizeLevelName(World->GetMapName());
	const FString PackageName = TunaSweeperHousing::NormalizeLevelName(World->GetOutermost()->GetName());
	return MapName.Equals(TunaSweeperHousing::BunkerMapName.ToString(), ESearchCase::IgnoreCase) ||
		PackageName.Equals(TunaSweeperHousing::BunkerMapName.ToString(), ESearchCase::IgnoreCase);
}

bool UTunaSweeperHousingSubsystem::DoesAreaBelongToWorld(
	const ATunaSweeperHousingAreaActor* HousingArea,
	const UWorld* World) const
{
	return HousingArea && World && HousingArea->GetWorld() == World;
}

ATunaSweeperHousingAreaActor* UTunaSweeperHousingSubsystem::ResolveActiveAreaForWorld(UWorld* World)
{
	RegisteredHousingAreas.RemoveAll([](const TWeakObjectPtr<ATunaSweeperHousingAreaActor>& RegisteredArea)
	{
		return !RegisteredArea.IsValid();
	});

	for (TActorIterator<ATunaSweeperHousingAreaActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		RegisterHousingArea(*ActorIt);
	}

	for (const TWeakObjectPtr<ATunaSweeperHousingAreaActor>& RegisteredArea : RegisteredHousingAreas)
	{
		if (DoesAreaBelongToWorld(RegisteredArea.Get(), World))
		{
			return RegisteredArea.Get();
		}
	}

	return nullptr;
}

ATunaSweeperHousingAreaActor* UTunaSweeperHousingSubsystem::SpawnDefaultHousingArea(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATunaSweeperHousingAreaActor* SpawnedArea = World->SpawnActor<ATunaSweeperHousingAreaActor>(
		ATunaSweeperHousingAreaActor::StaticClass(),
		FVector(0.0f, 0.0f, 4.0f),
		FRotator(0.0f, 18.0f, 0.0f),
		SpawnParameters);
	if (SpawnedArea)
	{
		RegisterHousingArea(SpawnedArea);
#if WITH_EDITOR
		SpawnedArea->SetActorLabel(TEXT("TS_BunkerHousingArea_Runtime"));
#endif
	}
	return SpawnedArea;
}

ATunaSweeperHousingManagementActor* UTunaSweeperHousingSubsystem::EnsureHousingManagementActorForWorld(UWorld* World)
{
	if (!World)
	{
		return nullptr;
	}

	if (RuntimeHousingManagementActor.IsValid() && RuntimeHousingManagementActor->GetWorld() == World)
	{
		return RuntimeHousingManagementActor.Get();
	}

	for (TActorIterator<ATunaSweeperHousingManagementActor> ActorIt(World); ActorIt; ++ActorIt)
	{
		if (IsValid(*ActorIt))
		{
			return *ActorIt;
		}
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ATunaSweeperHousingManagementActor* SpawnedActor = World->SpawnActor<ATunaSweeperHousingManagementActor>(
		ATunaSweeperHousingManagementActor::StaticClass(),
		TunaSweeperHousing::DefaultManagementActorLocation,
		TunaSweeperHousing::DefaultManagementActorRotation,
		SpawnParameters);
	if (SpawnedActor)
	{
		RuntimeHousingManagementActor = SpawnedActor;
#if WITH_EDITOR
		SpawnedActor->SetActorLabel(TEXT("TS_BunkerFacilityManagement_Runtime"));
#endif
	}

	return SpawnedActor;
}

bool UTunaSweeperHousingSubsystem::TryGetMouseWorldPoint(
	APlayerController* PlayerController,
	FVector& OutWorldPoint) const
{
	if (!PlayerController)
	{
		return false;
	}

	FVector WorldLocation;
	FVector WorldDirection;
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY) ||
		!PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection))
	{
		return false;
	}

	UWorld* World = PlayerController->GetWorld();
	if (World)
	{
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperHousingMouseTrace), true);
		if (const APawn* ControlledPawn = PlayerController->GetPawn())
		{
			QueryParams.AddIgnoredActor(ControlledPawn);
		}
		if (PreviewActor)
		{
			QueryParams.AddIgnoredActor(PreviewActor);
		}

		FHitResult Hit;
		const FVector TraceEnd = WorldLocation + WorldDirection * 100000.0f;
		if (World->LineTraceSingleByChannel(Hit, WorldLocation, TraceEnd, ECC_Visibility, QueryParams) && Hit.bBlockingHit)
		{
			OutWorldPoint = Hit.ImpactPoint;
			return true;
		}
	}

	if (!ActiveHousingArea.IsValid() || FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float PlaneZ = ActiveHousingArea->GetActorLocation().Z;
	const float DistanceToPlane = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
	if (DistanceToPlane < 0.0f)
	{
		return false;
	}

	OutWorldPoint = WorldLocation + WorldDirection * DistanceToPlane;
	return true;
}

bool UTunaSweeperHousingSubsystem::TryResolvePlacementAtMouse(
	APlayerController* PlayerController,
	FTunaSweeperHousingPlacedFacilitySaveData& OutPlacement,
	ETunaSweeperHousingPlacementStatus& OutStatus) const
{
	OutPlacement = FTunaSweeperHousingPlacedFacilitySaveData();
	OutStatus = ETunaSweeperHousingPlacementStatus::None;

	if (!HasActivePlacement() || !ActiveHousingArea.IsValid())
	{
		return false;
	}

	FTunaSweeperHousingFacilityDefinition Definition;
	if (!TryGetDefinition(ActivePlacement.FacilityId, Definition))
	{
		OutStatus = ETunaSweeperHousingPlacementStatus::UnknownFacility;
		return false;
	}

	FVector MouseWorldPoint;
	if (!TryGetMouseWorldPoint(PlayerController, MouseWorldPoint))
	{
		return false;
	}

	OutPlacement.FacilityId = ActivePlacement.FacilityId;
	OutPlacement.InstanceId = ActivePlacement.ExistingInstanceId;
	OutPlacement.RotationQuarterTurns = TunaSweeperHousing::NormalizeQuarterTurns(ActivePlacement.RotationQuarterTurns);
	OutPlacement.bStored = false;

	const FIntPoint FootprintSize = GetRotatedFootprintSize(Definition, OutPlacement.RotationQuarterTurns);
	const bool bInsideArea = ActiveHousingArea->TryGetAnchorCellForWorldLocation(
		MouseWorldPoint,
		FootprintSize,
		OutPlacement.AnchorCell);
	if (!bInsideArea)
	{
		OutStatus = ETunaSweeperHousingPlacementStatus::OutsideArea;
		return true;
	}

	return ValidatePlacement(OutPlacement, ActivePlacement.ExistingInstanceId, OutStatus);
}

bool UTunaSweeperHousingSubsystem::ValidatePlacement(
	const FTunaSweeperHousingPlacedFacilitySaveData& Placement,
	const FGuid& IgnoredInstanceId,
	ETunaSweeperHousingPlacementStatus& OutStatus) const
{
	OutStatus = ETunaSweeperHousingPlacementStatus::None;

	if (!ActiveHousingArea.IsValid())
	{
		OutStatus = ETunaSweeperHousingPlacementStatus::OutsideArea;
		return true;
	}

	FTunaSweeperHousingFacilityDefinition Definition;
	if (!TryGetDefinition(Placement.FacilityId, Definition))
	{
		OutStatus = ETunaSweeperHousingPlacementStatus::UnknownFacility;
		return true;
	}

	if (!ActiveHousingArea->IsCellRectInside(
		Placement.AnchorCell,
		GetRotatedFootprintSize(Definition, Placement.RotationQuarterTurns)))
	{
		OutStatus = ETunaSweeperHousingPlacementStatus::OutsideArea;
		return true;
	}

	for (const FTunaSweeperHousingPlacedFacilitySaveData& SavedFacility : SavedFacilities)
	{
		if (SavedFacility.bStored ||
			!SavedFacility.IsValid() ||
			(IgnoredInstanceId.IsValid() && SavedFacility.InstanceId == IgnoredInstanceId))
		{
			continue;
		}

		FTunaSweeperHousingFacilityDefinition SavedDefinition;
		if (!TryGetDefinition(SavedFacility.FacilityId, SavedDefinition))
		{
			continue;
		}

		if (DoFootprintsOverlap(Placement, Definition, SavedFacility, SavedDefinition))
		{
			OutStatus = ETunaSweeperHousingPlacementStatus::Occupied;
			return true;
		}
	}

	OutStatus = ETunaSweeperHousingPlacementStatus::Valid;
	return true;
}

bool UTunaSweeperHousingSubsystem::DoFootprintsOverlap(
	const FTunaSweeperHousingPlacedFacilitySaveData& LeftPlacement,
	const FTunaSweeperHousingFacilityDefinition& LeftDefinition,
	const FTunaSweeperHousingPlacedFacilitySaveData& RightPlacement,
	const FTunaSweeperHousingFacilityDefinition& RightDefinition) const
{
	const FIntPoint LeftSize = GetRotatedFootprintSize(LeftDefinition, LeftPlacement.RotationQuarterTurns);
	const FIntPoint RightSize = GetRotatedFootprintSize(RightDefinition, RightPlacement.RotationQuarterTurns);
	const int32 LeftMinX = LeftPlacement.AnchorCell.X;
	const int32 LeftMinY = LeftPlacement.AnchorCell.Y;
	const int32 LeftMaxX = LeftPlacement.AnchorCell.X + LeftSize.X;
	const int32 LeftMaxY = LeftPlacement.AnchorCell.Y + LeftSize.Y;
	const int32 RightMinX = RightPlacement.AnchorCell.X;
	const int32 RightMinY = RightPlacement.AnchorCell.Y;
	const int32 RightMaxX = RightPlacement.AnchorCell.X + RightSize.X;
	const int32 RightMaxY = RightPlacement.AnchorCell.Y + RightSize.Y;

	return LeftMinX < RightMaxX &&
		LeftMaxX > RightMinX &&
		LeftMinY < RightMaxY &&
		LeftMaxY > RightMinY;
}

FIntPoint UTunaSweeperHousingSubsystem::GetRotatedFootprintSize(
	const FTunaSweeperHousingFacilityDefinition& Definition,
	int32 RotationQuarterTurns) const
{
	const FIntPoint BaseSize(FMath::Max(1, Definition.SizeX), FMath::Max(1, Definition.SizeY));
	return (TunaSweeperHousing::NormalizeQuarterTurns(RotationQuarterTurns) % 2) == 0
		? BaseSize
		: FIntPoint(BaseSize.Y, BaseSize.X);
}

FTransform UTunaSweeperHousingSubsystem::BuildFacilityWorldTransform(
	const FTunaSweeperHousingFacilityDefinition& Definition,
	const FTunaSweeperHousingPlacedFacilitySaveData& Placement) const
{
	if (!ActiveHousingArea.IsValid())
	{
		return FTransform::Identity;
	}

	const FIntPoint FootprintSize = GetRotatedFootprintSize(Definition, Placement.RotationQuarterTurns);
	const float CellSize = FMath::Max(1.0f, ActiveHousingArea->GetCellSize());
	const float Height = Placement.InstanceId.IsValid()
		? TunaSweeperHousing::DefaultFacilityHeight
		: TunaSweeperHousing::PreviewFacilityHeight;
	const FVector CenterLocation = ActiveHousingArea->GetWorldLocationForFootprintCenter(
		Placement.AnchorCell,
		FootprintSize);
	const FRotator Rotation = ActiveHousingArea->GetAreaYawRotation() +
		FRotator(0.0f, static_cast<float>(TunaSweeperHousing::NormalizeQuarterTurns(Placement.RotationQuarterTurns)) * 90.0f, 0.0f);
	const FVector Scale(
		static_cast<float>(FootprintSize.X) * CellSize / 100.0f,
		static_cast<float>(FootprintSize.Y) * CellSize / 100.0f,
		Height / 100.0f);

	return FTransform(Rotation, CenterLocation + FVector(0.0f, 0.0f, Height * 0.5f + 2.0f), Scale);
}

FTransform UTunaSweeperHousingSubsystem::BuildPlacedActorWorldTransform(
	const FTunaSweeperHousingFacilityDefinition& Definition,
	const FTunaSweeperHousingPlacedFacilitySaveData& Placement) const
{
	if (!ActiveHousingArea.IsValid())
	{
		return FTransform::Identity;
	}

	const FIntPoint FootprintSize = GetRotatedFootprintSize(Definition, Placement.RotationQuarterTurns);
	const FVector CenterLocation = ActiveHousingArea->GetWorldLocationForFootprintCenter(
		Placement.AnchorCell,
		FootprintSize);
	const FRotator Rotation = ActiveHousingArea->GetAreaYawRotation() +
		FRotator(0.0f, static_cast<float>(TunaSweeperHousing::NormalizeQuarterTurns(Placement.RotationQuarterTurns)) * 90.0f, 0.0f);
	return FTransform(
		Rotation,
		CenterLocation + FVector(0.0f, 0.0f, TunaSweeperHousing::PlacedActorZOffset),
		FVector::OneVector);
}

TSubclassOf<AActor> UTunaSweeperHousingSubsystem::ResolveFacilityActorClass(
	const FTunaSweeperHousingFacilityDefinition& Definition) const
{
	const FString TrimmedActorClassPath = Definition.ActorClassPath.TrimStartAndEnd();
	if (!TrimmedActorClassPath.IsEmpty())
	{
		const TSoftClassPtr<AActor> ActorClass{ FSoftObjectPath(TrimmedActorClassPath) };
		if (TSubclassOf<AActor> LoadedActorClass = ActorClass.LoadSynchronous())
		{
			return LoadedActorClass;
		}

		UE_LOG(
			LogTunaSweeperHousing,
			Warning,
			TEXT("Housing facility actor class failed to load for %s: %s"),
			*Definition.FacilityId.ToString(),
			*TrimmedActorClassPath);
	}

	return ATunaSweeperHousingFacilityActor::StaticClass();
}

FName UTunaSweeperHousingSubsystem::BuildPlacedFacilityActorId(
	const FTunaSweeperHousingPlacedFacilitySaveData& Placement) const
{
	return FName(*FString::Printf(
		TEXT("housing.%s.%s"),
		*Placement.FacilityId.ToString(),
		*Placement.InstanceId.ToString(EGuidFormats::Digits)));
}

void UTunaSweeperHousingSubsystem::ConfigurePlacedFacilityActor(
	AActor* Actor,
	const FTunaSweeperHousingFacilityDefinition& Definition,
	const FTunaSweeperHousingPlacedFacilitySaveData& Placement) const
{
	ConfigureFacilityActor(Actor, Definition, Placement, false, true);
}

void UTunaSweeperHousingSubsystem::ConfigureFacilityActor(
	AActor* Actor,
	const FTunaSweeperHousingFacilityDefinition& Definition,
	const FTunaSweeperHousingPlacedFacilitySaveData& Placement,
	bool bPreview,
	bool bPlacementValid) const
{
	if (!Actor)
	{
		return;
	}

	if (ATunaSweeperHousingFacilityActor* FacilityActor = Cast<ATunaSweeperHousingFacilityActor>(Actor))
	{
		FacilityActor->ConfigureFacilityVisual(
			Definition,
			Placement,
			BuildFacilityWorldTransform(Definition, Placement),
			bPreview,
			bPlacementValid);
	}
	else
	{
		Actor->SetActorTransform(BuildPlacedActorWorldTransform(Definition, Placement));
	}

	Actor->SetActorEnableCollision(!bPreview);
	if (bPreview)
	{
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		Actor->GetComponents(PrimitiveComponents);
		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (!PrimitiveComponent)
			{
				continue;
			}

			PrimitiveComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			PrimitiveComponent->SetGenerateOverlapEvents(false);
			PrimitiveComponent->SetVisibility(true, true);
			PrimitiveComponent->SetHiddenInGame(false, true);
			PrimitiveComponent->SetRenderCustomDepth(true);
			PrimitiveComponent->SetCustomDepthStencilValue(bPlacementValid ? 1 : 2);
		}
	}

	if (!bPreview)
	{
		const FName ActorId = BuildPlacedFacilityActorId(Placement);
		Actor->Tags.AddUnique(ActorId);
		if (ATunaSweeperPiggyBankActor* PiggyBankActor = Cast<ATunaSweeperPiggyBankActor>(Actor))
		{
			PiggyBankActor->SetPiggyBankId(ActorId);
		}
		if (ATunaSweeperWorkbenchActor* WorkbenchActor = Cast<ATunaSweeperWorkbenchActor>(Actor))
		{
			WorkbenchActor->ConfigureWorkbenchDefaults(1);
		}

#if WITH_EDITOR
		if (!ActorId.IsNone())
		{
			Actor->SetActorLabel(ActorId.ToString());
		}
#endif
	}
}

bool UTunaSweeperHousingSubsystem::TryGetDefinition(
	FName FacilityId,
	FTunaSweeperHousingFacilityDefinition& OutDefinition) const
{
	if (const FTunaSweeperHousingFacilityDefinition* Definition = FacilityDefinitionsById.Find(FacilityId))
	{
		OutDefinition = *Definition;
		return true;
	}

	return false;
}

bool UTunaSweeperHousingSubsystem::IsFacilityFunctionUnlocked(
	const FTunaSweeperHousingFacilityDefinition& Definition) const
{
	if (Definition.bFunctionUnlockedByDefault)
	{
		return true;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance)
	{
		return false;
	}

	if (TunaGameInstance->IsHousingFacilityUnlocked(Definition.FacilityId))
	{
		return true;
	}

	for (int32 ItemId : Definition.UnlockWhenEverAcquiredItemIds)
	{
		if (ItemId != INDEX_NONE &&
			(TunaGameInstance->HasEverAcquiredItem(ItemId) || TunaGameInstance->CountInventoryItemById(ItemId) > 0))
		{
			return true;
		}
	}

	return false;
}

bool UTunaSweeperHousingSubsystem::HasEnoughMaterials(const FTunaSweeperHousingFacilityDefinition& Definition) const
{
	const UTunaSweeperGameInstance* ConstGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	UTunaSweeperGameInstance* TunaGameInstance = const_cast<UTunaSweeperGameInstance*>(ConstGameInstance);
	if (!TunaGameInstance)
	{
		return Definition.RequiredMaterials.IsEmpty();
	}

	for (const FTunaSweeperHousingMaterialCost& MaterialCost : Definition.RequiredMaterials)
	{
		if (MaterialCost.ItemId == INDEX_NONE || MaterialCost.Quantity <= 0)
		{
			continue;
		}

		if (TunaGameInstance->CountInventoryItemById(MaterialCost.ItemId) < MaterialCost.Quantity)
		{
			return false;
		}
	}

	return true;
}

bool UTunaSweeperHousingSubsystem::ConsumeRequiredMaterials(const FTunaSweeperHousingFacilityDefinition& Definition)
{
	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance || !HasEnoughMaterials(Definition))
	{
		return false;
	}

	for (const FTunaSweeperHousingMaterialCost& MaterialCost : Definition.RequiredMaterials)
	{
		if (MaterialCost.ItemId == INDEX_NONE || MaterialCost.Quantity <= 0)
		{
			continue;
		}

		if (TunaGameInstance->ConsumeInventoryItemById(MaterialCost.ItemId, MaterialCost.Quantity) < MaterialCost.Quantity)
		{
			return false;
		}
	}

	return true;
}

FText UTunaSweeperHousingSubsystem::ResolveFacilityDisplayName(
	const FTunaSweeperHousingFacilityDefinition& Definition) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return TunaGameInstance && !Definition.DisplayNameStringKey.IsNone()
		? TunaGameInstance->ResolveLocalizedText(Definition.DisplayNameStringKey, Definition.FallbackDisplayName)
		: Definition.FallbackDisplayName;
}

FText UTunaSweeperHousingSubsystem::ResolveFacilityDescription(
	const FTunaSweeperHousingFacilityDefinition& Definition) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return TunaGameInstance && !Definition.DescriptionStringKey.IsNone()
		? TunaGameInstance->ResolveLocalizedText(Definition.DescriptionStringKey, Definition.FallbackDescription)
		: Definition.FallbackDescription;
}

FText UTunaSweeperHousingSubsystem::BuildMaterialsText(
	const FTunaSweeperHousingFacilityDefinition& Definition) const
{
	if (Definition.RequiredMaterials.IsEmpty())
	{
		return TunaSweeperHousing::ResolveUiText(
			GetGameInstance(),
			TEXT("ui.housing.no_materials"),
			TEXT("No materials required"));
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	TArray<FString> MaterialParts;
	for (const FTunaSweeperHousingMaterialCost& MaterialCost : Definition.RequiredMaterials)
	{
		if (MaterialCost.ItemId == INDEX_NONE || MaterialCost.Quantity <= 0)
		{
			continue;
		}

		FText ItemName = FText::FromString(FString::Printf(TEXT("Item %d"), MaterialCost.ItemId));
		if (ItemDataSubsystem)
		{
			ItemDataSubsystem->TryGetItemNameText(MaterialCost.ItemId, Language, ItemName);
		}

		const int32 OwnedQuantity = TunaGameInstance
			? TunaGameInstance->CountInventoryItemById(MaterialCost.ItemId)
			: 0;
		MaterialParts.Add(FString::Printf(
			TEXT("%s %d/%d"),
			*ItemName.ToString(),
			FMath::Max(0, OwnedQuantity),
			MaterialCost.Quantity));
	}

	if (MaterialParts.IsEmpty())
	{
		return TunaSweeperHousing::ResolveUiText(
			GetGameInstance(),
			TEXT("ui.housing.no_materials"),
			TEXT("No materials required"));
	}

	return FText::FromString(FString::Join(MaterialParts, TEXT("  ")));
}

FText UTunaSweeperHousingSubsystem::BuildStateText(ETunaSweeperHousingFacilityBuildState BuildState) const
{
	switch (BuildState)
	{
	case ETunaSweeperHousingFacilityBuildState::Buildable:
		return TunaSweeperHousing::ResolveUiText(
			GetGameInstance(),
			TEXT("ui.housing.ready_to_build"),
			TEXT("Ready to build"));
	case ETunaSweeperHousingFacilityBuildState::Stored:
		return TunaSweeperHousing::ResolveUiText(
			GetGameInstance(),
			TEXT("ui.housing.stored"),
			TEXT("Stored"));
	case ETunaSweeperHousingFacilityBuildState::Placed:
		return TunaSweeperHousing::ResolveUiText(
			GetGameInstance(),
			TEXT("ui.housing.placed_hold_to_store"),
			TEXT("Placed - hold to store"));
	case ETunaSweeperHousingFacilityBuildState::InsufficientMaterials:
	default:
		return TunaSweeperHousing::ResolveUiText(
			GetGameInstance(),
			TEXT("ui.housing.need_materials"),
			TEXT("Need materials"));
	}
}

FString UTunaSweeperHousingSubsystem::GetHousingFacilityDefinitionsJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperHousing::FacilityDefinitionsJsonRelativePath);
}

void UTunaSweeperHousingSubsystem::DrawPlacementDebug(
	const FTunaSweeperHousingPlacedFacilitySaveData& Placement,
	bool bPlacementValid) const
{
	DrawHousingGridDebug(bPlacementValid ? nullptr : &Placement);
}

void UTunaSweeperHousingSubsystem::DrawHousingGridDebug(
	const FTunaSweeperHousingPlacedFacilitySaveData* InvalidPlacement) const
{
	if (!ActiveHousingArea.IsValid() || !ActiveHousingArea->GetWorld())
	{
		return;
	}

	for (int32 CellY = 0; CellY < ActiveHousingArea->GetGridSizeY(); ++CellY)
	{
		for (int32 CellX = 0; CellX < ActiveHousingArea->GetGridSizeX(); ++CellX)
		{
			DrawRoundedCellDebug(
				FIntPoint(CellX, CellY),
				TunaSweeperHousing::HousingCellColor,
				TunaSweeperHousing::CellOutlineThickness,
				SDPG_World,
				TunaSweeperHousing::CellDebugZOffset);
		}
	}

	if (!InvalidPlacement || InvalidPlacement->FacilityId.IsNone())
	{
		return;
	}

	FTunaSweeperHousingFacilityDefinition Definition;
	if (!TryGetDefinition(InvalidPlacement->FacilityId, Definition))
	{
		return;
	}

	const FIntPoint FootprintSize = GetRotatedFootprintSize(Definition, InvalidPlacement->RotationQuarterTurns);
	for (int32 OffsetY = 0; OffsetY < FootprintSize.Y; ++OffsetY)
	{
		for (int32 OffsetX = 0; OffsetX < FootprintSize.X; ++OffsetX)
		{
			DrawRoundedCellDebug(
				FIntPoint(InvalidPlacement->AnchorCell.X + OffsetX, InvalidPlacement->AnchorCell.Y + OffsetY),
				TunaSweeperHousing::InvalidCellColor,
				TunaSweeperHousing::InvalidCellOutlineThickness,
				SDPG_Foreground,
				TunaSweeperHousing::InvalidCellDebugZOffset);
		}
	}
}

void UTunaSweeperHousingSubsystem::DrawRoundedCellDebug(
	const FIntPoint& Cell,
	const FColor& Color,
	float Thickness,
	uint8 DepthPriority,
	float ZOffset) const
{
	if (!ActiveHousingArea.IsValid() || !ActiveHousingArea->GetWorld())
	{
		return;
	}

	const float CellSize = FMath::Max(1.0f, ActiveHousingArea->GetCellSize());
	const float Margin = FMath::Clamp(TunaSweeperHousing::CellOutlineMargin, 0.0f, CellSize * 0.25f);
	const float Radius = FMath::Clamp(
		CellSize * TunaSweeperHousing::CellRoundRadiusRatio,
		2.0f,
		FMath::Min(TunaSweeperHousing::CellRoundRadiusMax, CellSize * 0.5f - Margin));

	const FVector Origin = ActiveHousingArea->GetWorldLocationForCellCorner(Cell) + FVector(0.0f, 0.0f, ZOffset);
	const FVector XCorner = ActiveHousingArea->GetWorldLocationForCellCorner(FIntPoint(Cell.X + 1, Cell.Y)) + FVector(0.0f, 0.0f, ZOffset);
	const FVector YCorner = ActiveHousingArea->GetWorldLocationForCellCorner(FIntPoint(Cell.X, Cell.Y + 1)) + FVector(0.0f, 0.0f, ZOffset);
	const FVector XAxis = (XCorner - Origin).GetSafeNormal();
	const FVector YAxis = (YCorner - Origin).GetSafeNormal();
	if (XAxis.IsNearlyZero() || YAxis.IsNearlyZero())
	{
		return;
	}

	auto ToWorld = [&Origin, &XAxis, &YAxis](float X, float Y)
	{
		return Origin + XAxis * X + YAxis * Y;
	};

	UWorld* World = ActiveHousingArea->GetWorld();
	auto DrawSegment = [World, Color, Thickness, DepthPriority](const FVector& Start, const FVector& End)
	{
		DrawDebugLine(
			World,
			Start,
			End,
			Color,
			false,
			TunaSweeperHousing::DebugLifeTime,
			DepthPriority,
			Thickness);
	};

	const float Min = Margin;
	const float Max = CellSize - Margin;
	DrawSegment(ToWorld(Min + Radius, Min), ToWorld(Max - Radius, Min));
	DrawSegment(ToWorld(Max, Min + Radius), ToWorld(Max, Max - Radius));
	DrawSegment(ToWorld(Max - Radius, Max), ToWorld(Min + Radius, Max));
	DrawSegment(ToWorld(Min, Max - Radius), ToWorld(Min, Min + Radius));

	auto DrawArc = [&ToWorld, &DrawSegment](
		const FVector2D& Center,
		float InRadius,
		float StartDegrees,
		float EndDegrees)
	{
		constexpr int32 SegmentCount = 5;
		FVector PreviousPoint = ToWorld(
			Center.X + FMath::Cos(FMath::DegreesToRadians(StartDegrees)) * InRadius,
			Center.Y + FMath::Sin(FMath::DegreesToRadians(StartDegrees)) * InRadius);
		for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const float Alpha = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float AngleDegrees = FMath::Lerp(StartDegrees, EndDegrees, Alpha);
			const FVector CurrentPoint = ToWorld(
				Center.X + FMath::Cos(FMath::DegreesToRadians(AngleDegrees)) * InRadius,
				Center.Y + FMath::Sin(FMath::DegreesToRadians(AngleDegrees)) * InRadius);
			DrawSegment(PreviousPoint, CurrentPoint);
			PreviousPoint = CurrentPoint;
		}
	};

	DrawArc(FVector2D(Min + Radius, Min + Radius), Radius, 180.0f, 270.0f);
	DrawArc(FVector2D(Max - Radius, Min + Radius), Radius, 270.0f, 360.0f);
	DrawArc(FVector2D(Max - Radius, Max - Radius), Radius, 0.0f, 90.0f);
	DrawArc(FVector2D(Min + Radius, Max - Radius), Radius, 90.0f, 180.0f);
}
