#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Quest/TunaSweeperQuestId.h"
#include "Settings/TunaSweeperLanguage.h"
#include "Subsystem/TunaSweeperAdditionalTranslations.h"

#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Serialization/Csv/CsvParser.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "Subsystem/TunaSweeperToastSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperQuest, Log, All);

namespace TunaSweeperQuestIds
{
	constexpr int32 FirstOuting = 0;
}

namespace TunaSweeperQuestProviders
{
	const FName Mole(TEXT("provider.mole"));
}

namespace TunaSweeperQuestData
{
	void NormalizeNpcDeliveryObjectives(FTunaSweeperQuestDefinition& Definition)
	{
		const FName Recipient = Definition.GetCompletionProviderId();
		if (Recipient.IsNone()) return;
		for (auto& Objective : Definition.Objectives)
		{
			// NPC hand-ins become ready from ownership and consume at explicit reward claim.
			// World-destination deliveries keep their separate submission interaction.
			if (Objective.Type == ETunaSweeperObjectiveType::ItemSubmitted && Objective.TargetProviderId == Recipient)
			{
				Definition.LegacyNpcSubmissionObjectiveIds.Add(Objective.ObjectiveId);
				Objective.Type = ETunaSweeperObjectiveType::ItemPossessed;
			}
		}
	}

	FString GetCsvCell(const TArray<const TCHAR*>& Row, int32 CellIndex)
	{
		return Row.IsValidIndex(CellIndex)
			? FString(Row[CellIndex]).TrimStartAndEnd()
			: FString();
	}

	FName ReadNameField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		FString Value;
		return JsonObject.IsValid() && JsonObject->TryGetStringField(FieldName, Value) && !Value.TrimStartAndEnd().IsEmpty()
			? FName(*Value.TrimStartAndEnd())
			: NAME_None;
	}

	void ReadNameArrayField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, TArray<FName>& OutNames)
	{
		OutNames.Reset();
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!JsonObject.IsValid() || !JsonObject->TryGetArrayField(FieldName, Values) || !Values)
		{
			return;
		}

		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			FString RawName;
			if (Value.IsValid() && Value->TryGetString(RawName))
			{
				const FString TrimmedName = RawName.TrimStartAndEnd();
				if (!TrimmedName.IsEmpty())
				{
					OutNames.AddUnique(FName(*TrimmedName));
				}
			}
		}
	}

	FText ReadTextField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName)
	{
		FString Value;
		return JsonObject.IsValid() && JsonObject->TryGetStringField(FieldName, Value)
			? FText::FromString(Value)
			: FText::GetEmpty();
	}

	bool ReadObjectiveType(const FString& RawType, ETunaSweeperObjectiveType& OutType)
	{
		const FString Type = RawType.TrimStartAndEnd().ToLower();
		if (Type == TEXT("world_progress_completed"))
		{
			OutType = ETunaSweeperObjectiveType::WorldProgressCompleted;
			return true;
		}
		if (Type == TEXT("location_reached") || Type == TEXT("vehicle_location_reached"))
		{
			OutType = Type == TEXT("location_reached")
				? ETunaSweeperObjectiveType::LocationReached : ETunaSweeperObjectiveType::VehicleLocationReached;
			return true;
		}
		if (Type == TEXT("extraction_return"))
		{
			OutType = ETunaSweeperObjectiveType::ExtractionReturn;
			return true;
		}
		if (Type == TEXT("level_travel") || Type == TEXT("leveltravel"))
		{
			OutType = ETunaSweeperObjectiveType::LevelTravel;
			return true;
		}

		if (Type == TEXT("item_acquired") || Type == TEXT("itemacquired"))
		{
			OutType = ETunaSweeperObjectiveType::ItemAcquired;
			return true;
		}
		if (Type == TEXT("item_possessed"))
        {
            OutType = ETunaSweeperObjectiveType::ItemPossessed;
            return true;
        }
        if (Type == TEXT("item_submitted"))
		{
			OutType = ETunaSweeperObjectiveType::ItemSubmitted;
			return true;
		}

		if (Type == TEXT("enemy_killed") || Type == TEXT("enemykilled"))
		{
			OutType = ETunaSweeperObjectiveType::EnemyKilled;
			return true;
		}

		if (Type == TEXT("interaction_completed") || Type == TEXT("interactioncompleted"))
		{
			OutType = ETunaSweeperObjectiveType::InteractionCompleted;
			return true;
		}

		if (Type == TEXT("bunker_rescue_return") ||
			Type == TEXT("bunkerrescuereturn") ||
			Type == TEXT("rescue_return") ||
			Type == TEXT("rescuereturn"))
		{
			OutType = ETunaSweeperObjectiveType::BunkerRescueReturn;
			return true;
		}

		if (Type == TEXT("warp_point_used") ||
			Type == TEXT("warppointused") ||
			Type == TEXT("warp_used") ||
			Type == TEXT("warpused"))
		{
			OutType = ETunaSweeperObjectiveType::WarpPointUsed;
			return true;
		}

		return false;
	}

	int32 GetQuestProviderStateRank(ETunaSweeperQuestState State)
	{
		switch (State)
		{
		case ETunaSweeperQuestState::RewardAvailable:
			return 0;
		case ETunaSweeperQuestState::Accepted:
			return 1;
		case ETunaSweeperQuestState::Available:
			return 2;
		default:
			return 3;
		}
	}

	FName NormalizeTypeName(const FString& RawTypeName)
	{
		const FString Normalized = RawTypeName.TrimStartAndEnd().ToLower();
		return Normalized.IsEmpty() ? NAME_None : FName(*Normalized);
	}

	bool ParseObjective(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperObjectiveDefinition& OutObjective)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		OutObjective.ObjectiveId = ReadNameField(JsonObject, TEXT("objective_id"));
		if (OutObjective.ObjectiveId.IsNone())
		{
			return false;
		}

		FString TypeString;
		if (!JsonObject->TryGetStringField(TEXT("type"), TypeString) ||
			!ReadObjectiveType(TypeString, OutObjective.Type))
		{
			return false;
		}

		OutObjective.TextStringKey = ReadNameField(JsonObject, TEXT("text_string_key"));
		OutObjective.Text = ReadTextField(JsonObject, TEXT("text"));
		double RequiredCount = 1.0;
		const bool bHasRequiredCount = JsonObject->TryGetNumberField(TEXT("required_count"), RequiredCount);
		OutObjective.LocationId = ReadNameField(JsonObject, TEXT("location_id"));
		OutObjective.WorldProgressId = ReadNameField(JsonObject, TEXT("world_progress_id"));
		if (OutObjective.Type == ETunaSweeperObjectiveType::WorldProgressCompleted &&
			(!JsonObject->HasTypedField<EJson::String>(TEXT("world_progress_id")) ||
			OutObjective.WorldProgressId.IsNone() || RequiredCount != 1.0 ||
			(JsonObject->HasField(TEXT("required_count")) &&
				!JsonObject->HasTypedField<EJson::Number>(TEXT("required_count"))))) return false;
		const bool bArrival = OutObjective.Type == ETunaSweeperObjectiveType::LocationReached ||
			OutObjective.Type == ETunaSweeperObjectiveType::VehicleLocationReached;
		if (bArrival && (!JsonObject->HasTypedField<EJson::String>(TEXT("location_id")) ||
			OutObjective.LocationId.IsNone() || RequiredCount != 1.0 ||
			(JsonObject->HasField(TEXT("required_count")) &&
				!JsonObject->HasTypedField<EJson::Number>(TEXT("required_count"))))) return false;
		if ((OutObjective.Type == ETunaSweeperObjectiveType::ItemSubmitted || OutObjective.Type == ETunaSweeperObjectiveType::ItemPossessed) &&
			(!bHasRequiredCount || !FMath::IsFinite(RequiredCount) || RequiredCount < 1 ||
			RequiredCount > MAX_int32 || FMath::FloorToDouble(RequiredCount) != RequiredCount)) return false;
		OutObjective.RequiredCount = FMath::Max(1, FMath::RoundToInt(RequiredCount));

		OutObjective.SourceLevelName = ReadNameField(JsonObject, TEXT("source_level"));
		OutObjective.TargetLevelName = ReadNameField(JsonObject, TEXT("target_level"));
		OutObjective.EnemyId = ReadNameField(JsonObject, TEXT("enemy_id"));
		OutObjective.InteractionEventId = ReadNameField(JsonObject, TEXT("interaction_event_id"));
		OutObjective.WarpPointId = ReadNameField(JsonObject, TEXT("warp_point_id"));
		OutObjective.TargetWarpPointId = ReadNameField(JsonObject, TEXT("target_warp_point_id"));

		double ItemId = INDEX_NONE;
		const bool bHasItemId = JsonObject->TryGetNumberField(TEXT("item_id"), ItemId);
		OutObjective.TargetProviderId = ReadNameField(JsonObject, TEXT("target_provider_id"));
		if ((OutObjective.Type == ETunaSweeperObjectiveType::ItemSubmitted || OutObjective.Type == ETunaSweeperObjectiveType::ItemPossessed) &&
			(!bHasItemId || !FMath::IsFinite(ItemId) || ItemId < 1 || ItemId > MAX_int32 ||
			FMath::FloorToDouble(ItemId) != ItemId || OutObjective.TargetProviderId.IsNone())) return false;
		OutObjective.ItemId = FMath::RoundToInt(ItemId);

		FString InteractionTypeString;
		if (JsonObject->TryGetStringField(TEXT("interaction_type"), InteractionTypeString))
		{
			OutObjective.InteractionTypeName = NormalizeTypeName(InteractionTypeString);
		}

		return true;
	}

	bool ReadVectorField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, FVector& OutVector)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!JsonObject.IsValid() || !JsonObject->TryGetArrayField(FieldName, Values) || !Values || Values->Num() < 3)
		{
			return false;
		}

		double X = 0.0;
		double Y = 0.0;
		double Z = 0.0;
		if (!(*Values)[0].IsValid() ||
			!(*Values)[1].IsValid() ||
			!(*Values)[2].IsValid() ||
			!(*Values)[0]->TryGetNumber(X) ||
			!(*Values)[1]->TryGetNumber(Y) ||
			!(*Values)[2]->TryGetNumber(Z))
		{
			return false;
		}

		OutVector = FVector(X, Y, Z);
		return true;
	}

	bool ParsePresentationStep(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperQuestPresentationStep& OutStep)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		OutStep.SpeakerNameStringKey = ReadNameField(JsonObject, TEXT("speaker_name_string_key"));
		OutStep.DialogueTextStringKey = ReadNameField(JsonObject, TEXT("dialogue_text_string_key"));
		if (OutStep.DialogueTextStringKey.IsNone())
		{
			return false;
		}

		JsonObject->TryGetBoolField(TEXT("use_camera_focus"), OutStep.bUseCameraFocus);
		ReadVectorField(JsonObject, TEXT("camera_focus_location"), OutStep.CameraFocusLocation);

		double CameraBlendSeconds = OutStep.CameraBlendSeconds;
		JsonObject->TryGetNumberField(TEXT("camera_blend_seconds"), CameraBlendSeconds);
		OutStep.CameraBlendSeconds = FMath::Max(0.0f, static_cast<float>(CameraBlendSeconds));
		return true;
	}

	void ParsePresentationSteps(
		const TSharedPtr<FJsonObject>& QuestObject,
		const TCHAR* FieldName,
		TArray<FTunaSweeperQuestPresentationStep>& OutSteps)
	{
		OutSteps.Reset();

		const TArray<TSharedPtr<FJsonValue>>* StepValues = nullptr;
		if (!QuestObject.IsValid() || !QuestObject->TryGetArrayField(FieldName, StepValues))
		{
			return;
		}

		for (const TSharedPtr<FJsonValue>& StepValue : *StepValues)
		{
			FTunaSweeperQuestPresentationStep Step;
			if (ParsePresentationStep(StepValue.IsValid() ? StepValue->AsObject() : nullptr, Step))
			{
				OutSteps.Add(Step);
			}
		}
	}

	bool ParseItemReward(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperItemStack& OutItemReward)
	{
		if (!JsonObject.IsValid())
		{
			return false;
		}

		double ItemId = INDEX_NONE;
		double Quantity = 1.0;
		JsonObject->TryGetNumberField(TEXT("item_id"), ItemId);
		JsonObject->TryGetNumberField(TEXT("quantity"), Quantity);

		OutItemReward.ItemId = FMath::RoundToInt(ItemId);
		OutItemReward.Quantity = FMath::Max(1, FMath::RoundToInt(Quantity));
		return OutItemReward.ItemId != INDEX_NONE;
	}
}

void UTunaSweeperQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadQuestData(false);
	if (auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Game->OnWorldProgressChanged.AddUObject(this, &ThisClass::HandleWorldProgressChanged);
        Game->OnInventoryStateChanged.AddUObject(this, &ThisClass::HandleQuestInventoryChanged);
	}
	ExtractionPostLoadHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::HandleExtractionArrival);
	if (GEngine)
	{
		ExtractionTravelFailureHandle = GEngine->OnTravelFailure().AddWeakLambda(this,
			[this](UWorld* World, ETravelFailure::Type, const FString&)
			{
				if (!World || World->GetGameInstance() == GetGameInstance()) CancelExtractionReturn();
			});
	}
}

void UTunaSweeperQuestSubsystem::Deinitialize()
{
	if (auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Game->OnWorldProgressChanged.RemoveAll(this);
        Game->OnInventoryStateChanged.RemoveAll(this);
	}
	FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(ExtractionPostLoadHandle);
	if (GEngine) GEngine->OnTravelFailure().Remove(ExtractionTravelFailureHandle);
	CancelExtractionReturn();
	Super::Deinitialize();
}

int32 UTunaSweeperQuestSubsystem::GetFirstOutingQuestId()
{
	return TunaSweeperQuestIds::FirstOuting;
}

FName UTunaSweeperQuestSubsystem::GetMoleProviderId()
{
	return TunaSweeperQuestProviders::Mole;
}

bool UTunaSweeperQuestSubsystem::LoadQuestData(bool bForceReload)
{
	if (bQuestDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedQuestData();
	const bool bLoadedQuestTextStrings = LoadQuestTextStringsCsv();
	const bool bLoadedQuestDefinitions = LoadQuestDefinitionsJson();
	if (!bLoadedQuestTextStrings || !bLoadedQuestDefinitions) QuestDefinitions.Reset();
	bQuestDataLoaded = bLoadedQuestTextStrings && bLoadedQuestDefinitions;
	return bQuestDataLoaded;
}

bool UTunaSweeperQuestSubsystem::TryGetQuestDefinition(
	int32 QuestId,
	FTunaSweeperQuestDefinition& OutDefinition) const
{
	if (const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId))
	{
		OutDefinition = *Definition;
		ResolveDefinitionText(OutDefinition);
		return true;
	}

	OutDefinition = FTunaSweeperQuestDefinition();
	return false;
}

const FTunaSweeperQuestDefinition* UTunaSweeperQuestSubsystem::FindQuestDefinition(int32 QuestId) const
{
	return QuestDefinitions.Find(QuestId);
}

bool UTunaSweeperQuestSubsystem::GetAllQuestDefinitions(TArray<FTunaSweeperQuestDefinition>& OutDefinitions) const
{
	OutDefinitions.Reset();
	if (!EnsureQuestDataLoaded())
	{
		return false;
	}

	QuestDefinitions.GenerateValueArray(OutDefinitions);
	for (FTunaSweeperQuestDefinition& Definition : OutDefinitions)
	{
		ResolveDefinitionText(Definition);
	}
	OutDefinitions.Sort([](
		const FTunaSweeperQuestDefinition& Left,
		const FTunaSweeperQuestDefinition& Right)
	{
		if (Left.SortOrder != Right.SortOrder)
		{
			return Left.SortOrder < Right.SortOrder;
		}

		return Left.QuestId < Right.QuestId;
	});
	return true;
}

bool UTunaSweeperQuestSubsystem::TryGetQuestTextByKey(
	FName StringKey,
	ETunaSweeperItemTextLanguage Language,
	FText& OutText) const
{
	const FTunaSweeperQuestTextString* QuestTextString = QuestTextStringsByKey.Find(StringKey);
	if (!QuestTextString)
	{
		OutText = FText::GetEmpty();
		return false;
	}

	OutText = TunaSweeperLanguage::Resolve(*QuestTextString, Language);
#if WITH_EDITOR
	const bool bKoreanOnly = !QuestTextString->Korean.IsEmpty() &&
		QuestTextString->English.IsEmpty() && QuestTextString->Japanese.IsEmpty() &&
		QuestTextString->SimplifiedChinese.IsEmpty() && QuestTextString->TraditionalChinese.IsEmpty() &&
		QuestTextString->Russian.IsEmpty() && QuestTextString->BrazilianPortuguese.IsEmpty();
	if (bKoreanOnly)
	{
		const FTunaSweeperQuestTextString* PreviewFormat = QuestTextStringsByKey.Find(TEXT("ui.quest.editor_korean_only_format"));
		OutText = PreviewFormat
			? FText::Format(TunaSweeperLanguage::Resolve(*PreviewFormat, Language), QuestTextString->Korean)
			: QuestTextString->Korean;
	}
	else if (OutText.IsEmpty())
	{
		OutText = QuestTextString->Korean;
	}
#endif

	return !OutText.IsEmpty();
}

bool UTunaSweeperQuestSubsystem::GetQuestPresentationLines(
	int32 QuestId,
	ETunaSweeperQuestPresentationTrigger Trigger,
	TArray<FTunaSweeperQuestPresentationLineView>& OutLines) const
{
	OutLines.Reset();

	const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId);
	if (!Definition)
	{
		return false;
	}

	const TArray<FTunaSweeperQuestPresentationStep>& Steps =
		Trigger == ETunaSweeperQuestPresentationTrigger::OnRewardClaim
			? Definition->RewardPresentationSteps
			: Definition->AcceptPresentationSteps;
	OutLines.Reserve(Steps.Num());
	for (const FTunaSweeperQuestPresentationStep& Step : Steps)
	{
		FText DialogueText = ResolveQuestText(Step.DialogueTextStringKey);
		if (DialogueText.IsEmpty())
		{
			continue;
		}

		FTunaSweeperQuestPresentationLineView LineView;
		LineView.SpeakerName = ResolveQuestText(Step.SpeakerNameStringKey);
		LineView.DialogueText = DialogueText;
		LineView.bUseCameraFocus = Step.bUseCameraFocus;
		LineView.CameraFocusLocation = Step.CameraFocusLocation;
		LineView.CameraBlendSeconds = Step.CameraBlendSeconds;
		OutLines.Add(LineView);
	}

	return OutLines.Num() > 0;
}

ETunaSweeperQuestState UTunaSweeperQuestSubsystem::GetQuestState(int32 QuestId) const
{
	if (const FTunaSweeperNumericQuestProgressSaveData* Progress = QuestProgressById.Find(QuestId))
	{
		return Progress->State;
	}

	return ETunaSweeperQuestState::Available;
}

bool UTunaSweeperQuestSubsystem::CanAcceptQuest(int32 QuestId) const
{
	const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId);
	return Definition &&
		GetQuestState(QuestId) == ETunaSweeperQuestState::Available &&
		AreDefinitionPrerequisitesMet(*Definition);
}

bool UTunaSweeperQuestSubsystem::AreQuestPrerequisitesMet(int32 QuestId) const
{
	const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId);
	return Definition && AreDefinitionPrerequisitesMet(*Definition);
}

bool UTunaSweeperQuestSubsystem::AcceptQuest(int32 QuestId)
{
	if (const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()); Instance && Instance->IsCombatTestSession()) { return false; }
	EnsureSaveStateLoaded();
	if (!CanAcceptQuest(QuestId))
	{
		return false;
	}

	SetQuestState(QuestId, ETunaSweeperQuestState::Accepted);

	if (const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId))
	{
		if (Definition->bCompleteOnAccept && Definition->Objectives.IsEmpty())
		{
			SetQuestState(QuestId, ETunaSweeperQuestState::RewardAvailable);
			ShowQuestCompletedToast(QuestId);
		}
		if (Definition->bAutoTrackOnAccept)
		{
			TrackedQuestId = QuestId;
		}
	}

	ReconcileWorldProgressObjectives(true);
    ReconcilePossessionObjectives(true);
	BroadcastQuestProgressChanged(true);
	return true;
}

void UTunaSweeperQuestSubsystem::ReconcileStoryQuestAcceptance(const TSet<FName>& CompletedScenarioFlags)
{
	if (CompletedScenarioFlags.IsEmpty() || !EnsureQuestDataLoaded()) return;
	TArray<int32> Candidates;
	for (const auto& Entry : QuestDefinitions)
	{
		if (!Entry.Value.AutoAcceptAfterScenarioFlag.IsNone() &&
			CompletedScenarioFlags.Contains(Entry.Value.AutoAcceptAfterScenarioFlag))
		{
			Candidates.Add(Entry.Key);
		}
	}
	Candidates.Sort();
	for (const int32 QuestId : Candidates)
	{
		// AcceptQuest preserves every non-Available state and saves newly accepted progress.
		if (CanAcceptQuest(QuestId)) AcceptQuest(QuestId);
	}
}

bool UTunaSweeperQuestSubsystem::CanClaimQuestReward(int32 QuestId) const
{
	return FindQuestDefinition(QuestId) && GetQuestState(QuestId) == ETunaSweeperQuestState::RewardAvailable;
}

bool UTunaSweeperQuestSubsystem::ClaimQuestReward(int32 QuestId)
{
    if (bQuestRewardClaimInProgress) return false;
    ReconcilePossessionObjectives(false);
	if (const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()); Instance && Instance->IsCombatTestSession()) { return false; }
	EnsureSaveStateLoaded();
	if (!CanClaimQuestReward(QuestId))
	{
		return false;
	}

	const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId);
	if (!Definition)
	{
		return false;
	}

	TGuardValue<bool> ClaimGuard(bQuestRewardClaimInProgress, true);
    UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
    if (TunaGameInstance && (TunaGameInstance->IsQuestRewardTransferInProgress() || !TunaGameInstance->ValidateQuestItemRewards(Definition->Rewards.Items))) return false;
    if (!TunaGameInstance && (!Definition->Rewards.Items.IsEmpty() || !Definition->Rewards.HousingFacilityUnlocks.IsEmpty() || !Definition->Rewards.WorkbenchRecipeUnlocks.IsEmpty())) return false;
    TArray<FTunaSweeperItemStack> Requirements;
    for (const auto& Objective : Definition->Objectives)
    {
        if (Objective.Type != ETunaSweeperObjectiveType::ItemPossessed) continue;
        FTunaSweeperItemStack Requirement;
        Requirement.ItemId = Objective.ItemId;
        const auto& Progress = GetOrCreateQuestProgress(QuestId);
        const auto* Saved = Progress.ObjectiveProgress.FindByPredicate([&](const auto& Value) { return Value.ObjectiveId == Objective.ObjectiveId; });
        Requirement.Quantity = Objective.RequiredCount - (Saved ? FMath::Clamp(Saved->SubmittedItemCount, 0, Objective.RequiredCount) : 0);
        if (Requirement.Quantity == 0) continue;
        Requirements.Add(Requirement);
    }
    if (!Requirements.IsEmpty() && (!TunaGameInstance || !TunaGameInstance->TryConsumeQuestItems(Requirements, [] {}, false))) return false;
    if (Definition->Rewards.Items.Num() > 0)
	{
		if (!TunaGameInstance || !TunaGameInstance->GrantQuestItemRewards(Definition->Rewards.Items, false))
		{
			return false;
		}
	}

	if (Definition->Rewards.HousingFacilityUnlocks.Num() > 0)
	{
		if (!TunaGameInstance)
		{
			return false;
		}

		for (const FName& FacilityId : Definition->Rewards.HousingFacilityUnlocks)
		{
			TunaGameInstance->UnlockHousingFacility(FacilityId, false);
		}
	}

	if (Definition->Rewards.WorkbenchRecipeUnlocks.Num() > 0)
	{
		if (!TunaGameInstance)
		{
			return false;
		}

		for (const FName& RecipeId : Definition->Rewards.WorkbenchRecipeUnlocks)
		{
			TunaGameInstance->UnlockWorkbenchRecipe(RecipeId, false);
		}
	}

	CoinBalance += FMath::Max(0, Definition->Rewards.Coins);
	SetQuestState(QuestId, ETunaSweeperQuestState::RewardCompleted);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperAchievementSubsystem* AchievementSubsystem =
			GameInstance->GetSubsystem<UTunaSweeperAchievementSubsystem>())
		{
			AchievementSubsystem->ReportQuestRewardClaimed(QuestId);
		}
	}
	if (TrackedQuestId == QuestId)
	{
		TrackedQuestId = 0;
	}

	bQuestRewardClaimInProgress = false;
    if (TunaGameInstance) TunaGameInstance->NotifyQuestItemMutation();
	BroadcastQuestProgressChanged(true);
	return true;
}

bool UTunaSweeperQuestSubsystem::SetTrackedQuest(int32 QuestId)
{
	if (const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()); Instance && Instance->IsCombatTestSession()) { return false; }
	EnsureSaveStateLoaded();
	if (!IsQuestTrackable(QuestId))
	{
		return false;
	}

	if (TrackedQuestId == QuestId)
	{
		return true;
	}

	TrackedQuestId = QuestId;
	BroadcastQuestProgressChanged(true);
	return true;
}

void UTunaSweeperQuestSubsystem::ClearTrackedQuest()
{
	if (const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()); Instance && Instance->IsCombatTestSession()) { return; }
	if ((TrackedQuestId > 0))
	{
		EnsureSaveStateLoaded();
		TrackedQuestId = 0;
		BroadcastQuestProgressChanged(true);
	}
}

bool UTunaSweeperQuestSubsystem::GetQuestObjectiveProgress(
	int32 QuestId,
	TArray<FTunaSweeperObjectiveProgressView>& OutProgress) const
{
	OutProgress.Reset();

	const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId);
	if (!Definition)
	{
		return false;
	}

	OutProgress.Reserve(Definition->Objectives.Num());
	for (const FTunaSweeperObjectiveDefinition& Objective : Definition->Objectives)
	{
		FTunaSweeperObjectiveProgressView ProgressView;
		ProgressView.ObjectiveId = Objective.ObjectiveId;
		ProgressView.Text = ResolveQuestText(Objective.TextStringKey, Objective.Text);
		ProgressView.RequiredCount = FMath::Max(1, Objective.RequiredCount);
		ProgressView.CurrentCount = FMath::Clamp(
			GetObjectiveProgressCount(QuestId, Objective.ObjectiveId),
			0,
			ProgressView.RequiredCount);
		ProgressView.bCompleted = ProgressView.CurrentCount >= ProgressView.RequiredCount;
		OutProgress.Add(ProgressView);
	}

	return true;
}

bool UTunaSweeperQuestSubsystem::TryResolveQuestForProvider(
	FName ProviderId,
	int32 FallbackQuestId,
	int32& OutQuestId) const
{
	OutQuestId = 0;
	if (!EnsureQuestDataLoaded())
	{
		return false;
	}

	EnsureSaveStateLoaded();

	TArray<const FTunaSweeperQuestDefinition*> Candidates;
	bool bHasProviderQuests = false;
	for (const TPair<int32, FTunaSweeperQuestDefinition>& QuestPair : QuestDefinitions)
	{
		const FTunaSweeperQuestDefinition& Definition = QuestPair.Value;
		const bool bIsGiver = IsQuestForProvider(Definition, ProviderId);
		const bool bIsCompletionProvider = !ProviderId.IsNone() && Definition.GetCompletionProviderId() == ProviderId;
		if (!bIsGiver && !bIsCompletionProvider)
		{
			continue;
		}

		bHasProviderQuests = true;
		const ETunaSweeperQuestState State = GetQuestState(Definition.QuestId);
		if (State == ETunaSweeperQuestState::RewardCompleted ||
			(State == ETunaSweeperQuestState::RewardAvailable ? !bIsCompletionProvider : !bIsGiver) ||
			(State == ETunaSweeperQuestState::Available && !AreDefinitionPrerequisitesMet(Definition)))
		{
			continue;
		}

		Candidates.Add(&Definition);
	}

	Candidates.Sort([this](
		const FTunaSweeperQuestDefinition& Left,
		const FTunaSweeperQuestDefinition& Right)
	{
		const ETunaSweeperQuestState LeftState = GetQuestState(Left.QuestId);
		const ETunaSweeperQuestState RightState = GetQuestState(Right.QuestId);
		const int32 LeftRank = TunaSweeperQuestData::GetQuestProviderStateRank(LeftState);
		const int32 RightRank = TunaSweeperQuestData::GetQuestProviderStateRank(RightState);
		if (LeftRank != RightRank)
		{
			return LeftRank < RightRank;
		}

		if (Left.SortOrder != Right.SortOrder)
		{
			return Left.SortOrder < Right.SortOrder;
		}

		return Left.QuestId < Right.QuestId;
	});

	if (Candidates.Num() > 0)
	{
		OutQuestId = Candidates[0]->QuestId;
		return true;
	}

	if (bHasProviderQuests)
	{
		return false;
	}

	const FTunaSweeperQuestDefinition* FallbackDefinition = FindQuestDefinition(FallbackQuestId);
	if (FallbackDefinition &&
		FallbackDefinition->ProviderId.IsNone() && FallbackDefinition->GetCompletionProviderId().IsNone() &&
		GetQuestState(FallbackQuestId) != ETunaSweeperQuestState::RewardCompleted &&
		AreDefinitionPrerequisitesMet(*FallbackDefinition))
	{
		OutQuestId = FallbackQuestId;
		return true;
	}

	return false;
}

bool UTunaSweeperQuestSubsystem::TryGetLatestQuestInProviderChain(
	FName ProviderId,
	int32& OutQuestId) const
{
	OutQuestId = 0;
	if (!EnsureQuestDataLoaded() || ProviderId.IsNone())
	{
		return false;
	}

	EnsureSaveStateLoaded();

	TArray<const FTunaSweeperQuestDefinition*> Candidates;
	for (const TPair<int32, FTunaSweeperQuestDefinition>& QuestPair : QuestDefinitions)
	{
		const FTunaSweeperQuestDefinition& Definition = QuestPair.Value;
		if (!IsQuestForProvider(Definition, ProviderId))
		{
			continue;
		}

		const ETunaSweeperQuestState State = GetQuestState(Definition.QuestId);
		if (State == ETunaSweeperQuestState::Accepted ||
			State == ETunaSweeperQuestState::RewardAvailable ||
			State == ETunaSweeperQuestState::RewardCompleted)
		{
			Candidates.Add(&Definition);
		}
	}

	Candidates.Sort([](
		const FTunaSweeperQuestDefinition& Left,
		const FTunaSweeperQuestDefinition& Right)
	{
		if (Left.SortOrder != Right.SortOrder)
		{
			return Left.SortOrder > Right.SortOrder;
		}

		return Right.QuestId < Left.QuestId;
	});

	if (Candidates.Num() <= 0)
	{
		return false;
	}

	OutQuestId = Candidates[0]->QuestId;
	return true;
}

void UTunaSweeperQuestSubsystem::NotifyLevelTravelRequested(FName SourceLevelName, FName TargetLevelName)
{
	// Normal travel supersedes any prior extraction. The extraction actor arms its record after this call.
	CancelExtractionReturn();
	AdvanceMatchingObjectives(
		[this, SourceLevelName, TargetLevelName](const FTunaSweeperObjectiveDefinition& Objective)
		{
			return DoesObjectiveMatchLevelTravel(Objective, SourceLevelName, TargetLevelName);
		},
		1);
}

void UTunaSweeperQuestSubsystem::NotifyBunkerRescueReturn(FName SourceLevelName, FName TargetLevelName)
{
	CancelExtractionReturn();
	AdvanceMatchingObjectives(
		[this, SourceLevelName, TargetLevelName](const FTunaSweeperObjectiveDefinition& Objective)
		{
			return DoesObjectiveMatchBunkerRescueReturn(Objective, SourceLevelName, TargetLevelName);
		},
		1);
}

void UTunaSweeperQuestSubsystem::CancelExtractionReturn()
{
	PendingExtractionDestination = NAME_None;
	PendingExtractionObjectives.Reset();
}

void UTunaSweeperQuestSubsystem::BeginExtractionReturn(FName SourceLevelName, FName TargetLevelName)
{
	CancelExtractionReturn();
	if (const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()); Instance && Instance->IsCombatTestSession()) return;
	const FName Destination(*UWorld::RemovePIEPrefix(FPackageName::GetShortName(TargetLevelName.ToString())));
	const FName Bunker = TunaSweeperBuildFlavor::GetBunkerLevelName();
	if (!EnsureQuestDataLoaded() || !TunaSweeperBuildFlavor::IsRaidGameplayLevelName(SourceLevelName) ||
		Destination != Bunker) return;
	EnsureSaveStateLoaded();
	// Snapshot eligibility at extraction time; quests accepted after departure get no credit.
	for (const auto& Pair : QuestDefinitions)
	{
		if (GetQuestState(Pair.Key) != ETunaSweeperQuestState::Accepted) continue;
		for (const auto& Objective : Pair.Value.Objectives)
		{
			if (Objective.Type == ETunaSweeperObjectiveType::ExtractionReturn &&
				(Objective.SourceLevelName.IsNone() || IsMapNameMatch(SourceLevelName, *Objective.SourceLevelName.ToString())) &&
				(Objective.TargetLevelName.IsNone() || IsMapNameMatch(Destination, *Objective.TargetLevelName.ToString())))
			{
				PendingExtractionObjectives.FindOrAdd(Pair.Key).Add(Objective.ObjectiveId);
			}
		}
	}
	if (!PendingExtractionObjectives.IsEmpty()) PendingExtractionDestination = Bunker;
}

void UTunaSweeperQuestSubsystem::HandleExtractionArrival(UWorld* World)
{
	if (!World || !World->IsGameWorld() || World->GetGameInstance() != GetGameInstance()) return;
	CompleteExtractionReturn(FName(*UGameplayStatics::GetCurrentLevelName(World, true)));
}

void UTunaSweeperQuestSubsystem::CompleteExtractionReturn(FName ArrivedLevelName)
{
	const FName Arrived(*UWorld::RemovePIEPrefix(FPackageName::GetShortName(ArrivedLevelName.ToString())));
	const bool bArrived = !PendingExtractionDestination.IsNone() && Arrived == PendingExtractionDestination;
	const auto Pending = MoveTemp(PendingExtractionObjectives);
	CancelExtractionReturn(); // Consume before callbacks/save; repeated map notifications cannot double count.
	if (!bArrived) return;
	bool bChanged = false;
	for (const auto& Pair : Pending)
	{
		if (GetQuestState(Pair.Key) != ETunaSweeperQuestState::Accepted) continue;
		bool bQuestChanged = false;
		for (const FName ObjectiveId : Pair.Value) bQuestChanged |= AdvanceObjectiveProgress(Pair.Key, ObjectiveId, 1);
		if (bQuestChanged && AreAllObjectivesComplete(Pair.Key))
		{
			SetQuestState(Pair.Key, ETunaSweeperQuestState::RewardAvailable);
			ShowQuestCompletedToast(Pair.Key);
		}
		bChanged |= bQuestChanged;
	}
	if (bChanged) BroadcastQuestProgressChanged(true);
}

void UTunaSweeperQuestSubsystem::NotifyWarpPointUsed(FName LevelName, FName WarpPointId, FName TargetWarpPointId)
{
	AdvanceMatchingObjectives(
		[this, LevelName, WarpPointId, TargetWarpPointId](const FTunaSweeperObjectiveDefinition& Objective)
		{
			return DoesObjectiveMatchWarpPointUsed(Objective, LevelName, WarpPointId, TargetWarpPointId);
		},
		1);
}

void UTunaSweeperQuestSubsystem::NotifyItemAcquired(int32 ItemId, int32 Quantity, bool bSaveImmediately)
{
	if (ItemId == INDEX_NONE || Quantity <= 0)
	{
		return;
	}

	AdvanceMatchingObjectives(
		[this, ItemId](const FTunaSweeperObjectiveDefinition& Objective)
		{
			return DoesObjectiveMatchItemAcquired(Objective, ItemId);
		},
		Quantity,
		bSaveImmediately);
}

void UTunaSweeperQuestSubsystem::NotifyEnemyKilled(FName EnemyId)
{
	AdvanceMatchingObjectives(
		[this, EnemyId](const FTunaSweeperObjectiveDefinition& Objective)
		{
			return DoesObjectiveMatchEnemyKilled(Objective, EnemyId);
		},
		1);
}

void UTunaSweeperQuestSubsystem::NotifyInteractionCompleted(FName InteractionEventId, FName InteractionTypeName)
{
	AdvanceMatchingObjectives(
		[this, InteractionEventId, InteractionTypeName](const FTunaSweeperObjectiveDefinition& Objective)
		{
			return DoesObjectiveMatchInteractionCompleted(Objective, InteractionEventId, InteractionTypeName);
		},
		1);
}

void UTunaSweeperQuestSubsystem::NotifyLocationReached(FName LocationId, bool bMountedVehicleArrival)
{
	if (LocationId.IsNone()) return;
	const auto ArrivalType = bMountedVehicleArrival
		? ETunaSweeperObjectiveType::VehicleLocationReached : ETunaSweeperObjectiveType::LocationReached;
	AdvanceMatchingObjectives(
		[LocationId, ArrivalType](const FTunaSweeperObjectiveDefinition& Objective)
		{
			return Objective.Type == ArrivalType && Objective.LocationId == LocationId && Objective.RequiredCount == 1;
		}, 1);
}

void UTunaSweeperQuestSubsystem::AddCoins(int32 Amount, bool bSaveImmediately)
{
	if (const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()); Instance && Instance->IsCombatTestSession()) { return; }
	EnsureSaveStateLoaded();

	const int32 PositiveAmount = FMath::Max(0, Amount);
	if (PositiveAmount <= 0)
	{
		return;
	}

	CoinBalance += PositiveAmount;
	BroadcastQuestProgressChanged(bSaveImmediately);
}

bool UTunaSweeperQuestSubsystem::TrySpendCoins(int32 Amount, bool bSaveImmediately)
{
	if (const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()); Instance && Instance->IsCombatTestSession()) { return false; }
	EnsureSaveStateLoaded();

	const int32 PositiveAmount = FMath::Max(0, Amount);
	if (PositiveAmount <= 0)
	{
		return true;
	}

	if (CoinBalance < PositiveAmount)
	{
		return false;
	}

	CoinBalance -= PositiveAmount;
	BroadcastQuestProgressChanged(bSaveImmediately);
	return true;
}

void UTunaSweeperQuestSubsystem::ExportQuestProgressForSave(
	TArray<FTunaSweeperNumericQuestProgressSaveData>& OutQuestProgress,
	int32& OutTrackedQuestId,
	int32& OutQuestCoinBalance) const
{
	OutQuestProgress.Reset();
	QuestProgressById.GenerateValueArray(OutQuestProgress);
	OutQuestProgress.Sort([](
		const FTunaSweeperNumericQuestProgressSaveData& Left,
		const FTunaSweeperNumericQuestProgressSaveData& Right)
	{
		return Left.QuestId < Right.QuestId;
	});

	OutTrackedQuestId = TrackedQuestId;
	OutQuestCoinBalance = FMath::Max(0, CoinBalance);
}

void UTunaSweeperQuestSubsystem::LoadQuestProgressFromSave(
	const TArray<FTunaSweeperNumericQuestProgressSaveData>& SavedQuestProgress,
	int32 SavedTrackedQuestId,
	int32 SavedQuestCoinBalance)
{
	CancelExtractionReturn();
	LoadQuestData(false);
	QuestProgressById.Reset();
	CoinBalance = FMath::Max(0, SavedQuestCoinBalance);
	TrackedQuestId = 0;

	for (const FTunaSweeperNumericQuestProgressSaveData& SavedProgress : SavedQuestProgress)
	{
		const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(SavedProgress.QuestId);
		if (!Definition)
		{
			continue;
		}

		FTunaSweeperNumericQuestProgressSaveData LoadedProgress;
		LoadedProgress.QuestId = SavedProgress.QuestId;
		LoadedProgress.State = SavedProgress.State;
		LoadedProgress.bOwnershipReadinessRecorded = true;

		for (const FTunaSweeperObjectiveDefinition& Objective : Definition->Objectives)
		{
			const FTunaSweeperObjectiveProgressSaveData* SavedObjectiveProgress =
				SavedProgress.ObjectiveProgress.FindByPredicate(
					[&Objective](const FTunaSweeperObjectiveProgressSaveData& Progress)
					{
						return Progress.ObjectiveId == Objective.ObjectiveId;
					});

			FTunaSweeperObjectiveProgressSaveData LoadedObjectiveProgress;
			LoadedObjectiveProgress.ObjectiveId = Objective.ObjectiveId;
			LoadedObjectiveProgress.CurrentCount = SavedObjectiveProgress
				? FMath::Clamp(SavedObjectiveProgress->CurrentCount, 0, FMath::Max(1, Objective.RequiredCount))
				: 0;
			if (SavedObjectiveProgress && Objective.Type == ETunaSweeperObjectiveType::ItemPossessed)
			{
				const bool bLegacySubmission = !SavedProgress.bOwnershipReadinessRecorded && Definition->LegacyNpcSubmissionObjectiveIds.Contains(Objective.ObjectiveId);
				LoadedObjectiveProgress.SubmittedItemCount = FMath::Clamp(bLegacySubmission ? SavedObjectiveProgress->CurrentCount : SavedObjectiveProgress->SubmittedItemCount, 0, Objective.RequiredCount);
			}
			LoadedProgress.ObjectiveProgress.Add(LoadedObjectiveProgress);
		}

		QuestProgressById.Add(LoadedProgress.QuestId, LoadedProgress);
		if (LoadedProgress.State == ETunaSweeperQuestState::RewardCompleted)
		{
			if (UGameInstance* GameInstance = GetGameInstance())
			{
				if (UTunaSweeperAchievementSubsystem* AchievementSubsystem =
					GameInstance->GetSubsystem<UTunaSweeperAchievementSubsystem>())
				{
					AchievementSubsystem->ReportQuestRewardClaimed(LoadedProgress.QuestId);
				}
			}
		}
	}

	if (IsQuestTrackable(SavedTrackedQuestId))
	{
		TrackedQuestId = SavedTrackedQuestId;
	}

	// WorldProgressStates is restored before quest progress. Do not save or toast during loading.
	ReconcileWorldProgressObjectives(false);
	OnQuestProgressChanged.Broadcast();
}

void UTunaSweeperQuestSubsystem::ResetQuestProgressForNewGame()
{
	CancelExtractionReturn();
	QuestProgressById.Reset();
	TrackedQuestId = 0;
	CoinBalance = 0;
	OnQuestProgressChanged.Broadcast();
}

bool UTunaSweeperQuestSubsystem::EnsureQuestDataLoaded() const
{
	return bQuestDataLoaded;
}

bool UTunaSweeperQuestSubsystem::LoadQuestDefinitionsJson()
{
	FString JsonText;
	if (!FFileHelper::LoadFileToString(JsonText, *GetQuestDefinitionsJsonPath()))
	{
		UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Could not load quest definitions: %s"), *GetQuestDefinitionsJsonPath());
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> QuestValues;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
	if (!FJsonSerializer::Deserialize(Reader, QuestValues))
	{
		UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Could not parse quest definitions JSON."));
		return false;
	}

	for (const TSharedPtr<FJsonValue>& QuestValue : QuestValues)
	{
		const TSharedPtr<FJsonObject> QuestObject = QuestValue.IsValid() ? QuestValue->AsObject() : nullptr;
		if (!QuestObject.IsValid())
		{
			continue;
		}

		FTunaSweeperQuestDefinition Definition;
		Definition.QuestId = TunaSweeperQuestId::Read(QuestObject, TEXT("quest_id"));
		if (Definition.QuestId <= 0 || QuestDefinitions.Contains(Definition.QuestId))
		{
			QuestDefinitions.Reset();
			return false;
		}

		Definition.TitleStringKey = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("title_string_key"));
		Definition.Title = TunaSweeperQuestData::ReadTextField(QuestObject, TEXT("title"));
		Definition.DescriptionStringKey = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("description_string_key"));
		Definition.ObjectiveDisplayStringKey = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("objective_display_string_key"));
		Definition.Description = TunaSweeperQuestData::ReadTextField(QuestObject, TEXT("description"));
		Definition.ProviderId = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("provider_id"));
		Definition.CompletionProviderId = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("completion_provider_id"));
		double SortOrder = 0.0;
		QuestObject->TryGetNumberField(TEXT("sort_order"), SortOrder);
		Definition.SortOrder = FMath::RoundToInt(SortOrder);
		const TArray<TSharedPtr<FJsonValue>>* Prerequisites = nullptr;
		if (QuestObject->HasField(TEXT("required_completed_quest_ids")))
		{
			if (!QuestObject->TryGetArrayField(TEXT("required_completed_quest_ids"), Prerequisites)) return false;
			for (const auto& Value : *Prerequisites)
			{
				const int32 RequiredId = TunaSweeperQuestId::FromJson(Value);
				if (RequiredId <= 0 || RequiredId == Definition.QuestId)
				{
					QuestDefinitions.Reset();
					return false;
				}
				Definition.RequiredCompletedQuestIds.AddUnique(RequiredId);
			}
		}
		QuestObject->TryGetBoolField(TEXT("auto_track_on_accept"), Definition.bAutoTrackOnAccept);
		QuestObject->TryGetBoolField(TEXT("complete_on_accept"), Definition.bCompleteOnAccept);
		Definition.AutoAcceptAfterScenarioFlag = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("auto_accept_after_scenario_flag"));

		const TArray<TSharedPtr<FJsonValue>>* ObjectiveValues = nullptr;
		if (QuestObject->TryGetArrayField(TEXT("objectives"), ObjectiveValues))
		{
			for (const TSharedPtr<FJsonValue>& ObjectiveValue : *ObjectiveValues)
			{
				FTunaSweeperObjectiveDefinition Objective;
				if (TunaSweeperQuestData::ParseObjective(ObjectiveValue.IsValid() ? ObjectiveValue->AsObject() : nullptr, Objective))
				{
					Definition.Objectives.Add(Objective);
				}
				else
				{
					UE_LOG(LogTunaSweeperQuest, Error, TEXT("Invalid objective in quest %d"), Definition.QuestId);
					QuestDefinitions.Reset();
					return false;
				}
			}
		}

		TunaSweeperQuestData::NormalizeNpcDeliveryObjectives(Definition);

		const TSharedPtr<FJsonObject>* RewardObject = nullptr;
		if (QuestObject->TryGetObjectField(TEXT("rewards"), RewardObject) && RewardObject && RewardObject->IsValid())
		{
			double Coins = 0.0;
			(*RewardObject)->TryGetNumberField(TEXT("coins"), Coins);
			Definition.Rewards.Coins = FMath::Max(0, FMath::RoundToInt(Coins));

			const TArray<TSharedPtr<FJsonValue>>* ItemRewardValues = nullptr;
			if ((*RewardObject)->TryGetArrayField(TEXT("items"), ItemRewardValues))
			{
				for (const TSharedPtr<FJsonValue>& ItemRewardValue : *ItemRewardValues)
				{
					FTunaSweeperItemStack ItemReward;
					if (TunaSweeperQuestData::ParseItemReward(ItemRewardValue.IsValid() ? ItemRewardValue->AsObject() : nullptr, ItemReward))
					{
						Definition.Rewards.Items.Add(ItemReward);
					}
				}
			}

			TArray<FName> HousingFacilityUnlocks;
			TunaSweeperQuestData::ReadNameArrayField(
				*RewardObject,
				TEXT("housing_facilities"),
				HousingFacilityUnlocks);
			for (const FName& FacilityId : HousingFacilityUnlocks)
			{
				Definition.Rewards.HousingFacilityUnlocks.AddUnique(FacilityId);
			}
			TunaSweeperQuestData::ReadNameArrayField(
				*RewardObject,
				TEXT("housing_facility_unlocks"),
				HousingFacilityUnlocks);
			for (const FName& FacilityId : HousingFacilityUnlocks)
			{
				Definition.Rewards.HousingFacilityUnlocks.AddUnique(FacilityId);
			}

			TArray<FName> WorkbenchRecipeUnlocks;
			TunaSweeperQuestData::ReadNameArrayField(
				*RewardObject,
				TEXT("workbench_recipes"),
				WorkbenchRecipeUnlocks);
			for (const FName& RecipeId : WorkbenchRecipeUnlocks)
			{
				Definition.Rewards.WorkbenchRecipeUnlocks.AddUnique(RecipeId);
			}
			TunaSweeperQuestData::ReadNameArrayField(
				*RewardObject,
				TEXT("workbench_recipe_unlocks"),
				WorkbenchRecipeUnlocks);
			for (const FName& RecipeId : WorkbenchRecipeUnlocks)
			{
				Definition.Rewards.WorkbenchRecipeUnlocks.AddUnique(RecipeId);
			}
		}

		TunaSweeperQuestData::ParsePresentationSteps(
			QuestObject,
			TEXT("accept_presentation"),
			Definition.AcceptPresentationSteps);
		TunaSweeperQuestData::ParsePresentationSteps(
			QuestObject,
			TEXT("reward_presentation"),
			Definition.RewardPresentationSteps);

		if (Definition.bCompleteOnAccept && !Definition.Objectives.IsEmpty())
		{
			UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Quest %d cannot combine complete_on_accept with objectives."), Definition.QuestId);
			QuestDefinitions.Reset();
			return false;
		}

		if (Definition.Objectives.Num() > 0 || Definition.bCompleteOnAccept)
		{
			ResolveDefinitionText(Definition);
			QuestDefinitions.Add(Definition.QuestId, Definition);
		}
	}

	return QuestValues.Num() == 0 || QuestDefinitions.Num() > 0;
}

bool UTunaSweeperQuestSubsystem::LoadQuestTextStringsCsv()
{
	TArray<FString> CsvPaths;
	TunaSweeperBuildFlavor::GetQuestTextStringPaths(CsvPaths);
	bool bAllFilesValid = true;
	bool bHasValidRows = false;

	for (const FString& CsvPath : CsvPaths)
	{
		FString CsvContent;
		if (!FFileHelper::LoadFileToString(CsvContent, *CsvPath))
		{
			UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Could not load quest text strings: %s"), *CsvPath);
			bAllFilesValid = false;
			continue;
		}

		FTunaSweeperAdditionalTranslations AdditionalTranslations;
		if (!AdditionalTranslations.Load(CsvPath))
		{
			bAllFilesValid = false;
			continue;
		}

		FCsvParser CsvParser(CsvContent);
		const FCsvParser::FRows& Rows = CsvParser.GetRows();
		if (Rows.Num() < 1)
		{
			UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Quest text strings CSV has no header: %s"), *CsvPath);
			bAllFilesValid = false;
			continue;
		}

		const TArray<const TCHAR*>& HeaderRow = Rows[0];
		const bool bHeaderIsValid =
			TunaSweeperQuestData::GetCsvCell(HeaderRow, 0).Equals(TEXT("string_key"), ESearchCase::IgnoreCase) &&
			TunaSweeperQuestData::GetCsvCell(HeaderRow, 1).Equals(TEXT("ko"), ESearchCase::IgnoreCase) &&
			TunaSweeperQuestData::GetCsvCell(HeaderRow, 2).Equals(TEXT("en"), ESearchCase::IgnoreCase) &&
			TunaSweeperQuestData::GetCsvCell(HeaderRow, 3).Equals(TEXT("ja"), ESearchCase::IgnoreCase);
		if (!bHeaderIsValid)
		{
			UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Quest text strings CSV header must be string_key,ko,en,ja: %s"), *CsvPath);
			bAllFilesValid = false;
			continue;
		}

		for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
		{
			const TArray<const TCHAR*>& Row = Rows[RowIndex];
			if (Row.Num() < 4)
			{
				UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Skipping quest text row %d in %s: expected 4 columns."), RowIndex, *CsvPath);
				bAllFilesValid = false;
				continue;
			}

			const FString StringKey = TunaSweeperQuestData::GetCsvCell(Row, 0);
			const FString Korean = TunaSweeperQuestData::GetCsvCell(Row, 1);
			const FString English = TunaSweeperQuestData::GetCsvCell(Row, 2);
			const FString Japanese = TunaSweeperQuestData::GetCsvCell(Row, 3);
			if (StringKey.IsEmpty() || Korean.IsEmpty()
#if !WITH_EDITOR
				|| English.IsEmpty() || Japanese.IsEmpty()
#endif
			)
			{
				UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Skipping quest text row %d in %s: required cell is empty."), RowIndex, *CsvPath);
				bAllFilesValid = false;
				continue;
			}

			FTunaSweeperQuestTextString QuestTextString;
			QuestTextString.StringKey = FName(*StringKey);
			QuestTextString.Korean = FText::FromString(Korean);
			QuestTextString.English = FText::FromString(English);
			QuestTextString.Japanese = FText::FromString(Japanese);
			AdditionalTranslations.Apply(QuestTextString.StringKey, QuestTextString);
			if (QuestTextStringsByKey.Contains(QuestTextString.StringKey))
			{
				UE_LOG(LogTunaSweeperQuest, Error, TEXT("Duplicate quest text string key %s across build-flavor data. Keys may not override each other."), *StringKey);
				bAllFilesValid = false;
				continue;
			}

			QuestTextStringsByKey.Add(QuestTextString.StringKey, QuestTextString);
			bHasValidRows = true;
		}
	}

	return bAllFilesValid && bHasValidRows;
}

void UTunaSweeperQuestSubsystem::ResetLoadedQuestData()
{
	CancelExtractionReturn();
	QuestDefinitions.Reset();
	QuestTextStringsByKey.Reset();
	bQuestDataLoaded = false;
}

FString UTunaSweeperQuestSubsystem::GetQuestDefinitionsJsonPath() const
{
	return TunaSweeperBuildFlavor::GetQuestDefinitionsPath();
}

void UTunaSweeperQuestSubsystem::ResolveDefinitionText(FTunaSweeperQuestDefinition& Definition) const
{
	Definition.Title = ResolveQuestText(Definition.TitleStringKey, Definition.Title);
	Definition.Description = ResolveQuestText(Definition.DescriptionStringKey, Definition.Description);
	Definition.ObjectiveDisplayText = ResolveQuestText(Definition.ObjectiveDisplayStringKey);
	for (FTunaSweeperObjectiveDefinition& Objective : Definition.Objectives)
	{
		Objective.Text = ResolveQuestText(Objective.TextStringKey, Objective.Text);
	}
}

FText UTunaSweeperQuestSubsystem::ResolveQuestText(FName StringKey, const FText& FallbackText) const
{
	if (StringKey.IsNone())
	{
		return FallbackText;
	}

	FText Text;
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;
	if (TryGetQuestTextByKey(StringKey, Language, Text))
	{
		return Text;
	}

	return FallbackText.IsEmpty() ? FText::FromString(StringKey.ToString()) : FallbackText;
}

bool UTunaSweeperQuestSubsystem::IsMapNameMatch(FName ActualMapName, const TCHAR* ExpectedMapName) const
{
	if (ActualMapName.IsNone() || !ExpectedMapName)
	{
		return false;
	}
	const FName ActualShortName(*UWorld::RemovePIEPrefix(FPackageName::GetShortName(ActualMapName.ToString())));
	const FName ExpectedShortName(*UWorld::RemovePIEPrefix(FPackageName::GetShortName(ExpectedMapName)));
	if (ExpectedShortName == FName(TEXT("RaidMap")) &&
		TunaSweeperBuildFlavor::IsRaidGameplayLevelName(ActualShortName))
	{
		return true;
	}

	return !ActualShortName.IsNone() && ActualShortName == ExpectedShortName;
}

bool UTunaSweeperQuestSubsystem::DoesObjectiveMatchLevelTravel(
	const FTunaSweeperObjectiveDefinition& Objective,
	FName SourceLevelName,
	FName TargetLevelName) const
{
	return Objective.Type == ETunaSweeperObjectiveType::LevelTravel &&
		(Objective.SourceLevelName.IsNone() || IsMapNameMatch(SourceLevelName, *Objective.SourceLevelName.ToString())) &&
		(Objective.TargetLevelName.IsNone() || IsMapNameMatch(TargetLevelName, *Objective.TargetLevelName.ToString()));
}

bool UTunaSweeperQuestSubsystem::DoesObjectiveMatchBunkerRescueReturn(
	const FTunaSweeperObjectiveDefinition& Objective,
	FName SourceLevelName,
	FName TargetLevelName) const
{
	return Objective.Type == ETunaSweeperObjectiveType::BunkerRescueReturn &&
		(Objective.SourceLevelName.IsNone() || IsMapNameMatch(SourceLevelName, *Objective.SourceLevelName.ToString())) &&
		(Objective.TargetLevelName.IsNone() || IsMapNameMatch(TargetLevelName, *Objective.TargetLevelName.ToString()));
}

bool UTunaSweeperQuestSubsystem::DoesObjectiveMatchWarpPointUsed(
	const FTunaSweeperObjectiveDefinition& Objective,
	FName LevelName,
	FName WarpPointId,
	FName TargetWarpPointId) const
{
	return Objective.Type == ETunaSweeperObjectiveType::WarpPointUsed &&
		(Objective.SourceLevelName.IsNone() || IsMapNameMatch(LevelName, *Objective.SourceLevelName.ToString())) &&
		(Objective.WarpPointId.IsNone() || Objective.WarpPointId == WarpPointId) &&
		(Objective.TargetWarpPointId.IsNone() || Objective.TargetWarpPointId == TargetWarpPointId);
}

bool UTunaSweeperQuestSubsystem::DoesObjectiveMatchItemAcquired(
	const FTunaSweeperObjectiveDefinition& Objective,
	int32 ItemId) const
{
	return Objective.Type == ETunaSweeperObjectiveType::ItemAcquired &&
		(Objective.ItemId == INDEX_NONE || Objective.ItemId == ItemId);
}

bool UTunaSweeperQuestSubsystem::DoesObjectiveMatchEnemyKilled(
	const FTunaSweeperObjectiveDefinition& Objective,
	FName EnemyId) const
{
	return Objective.Type == ETunaSweeperObjectiveType::EnemyKilled &&
		(Objective.EnemyId.IsNone() || Objective.EnemyId == EnemyId);
}

bool UTunaSweeperQuestSubsystem::DoesObjectiveMatchInteractionCompleted(
	const FTunaSweeperObjectiveDefinition& Objective,
	FName InteractionEventId,
	FName InteractionTypeName) const
{
	return Objective.Type == ETunaSweeperObjectiveType::InteractionCompleted &&
		(Objective.InteractionEventId.IsNone() || Objective.InteractionEventId == InteractionEventId) &&
		(Objective.InteractionTypeName.IsNone() || Objective.InteractionTypeName == InteractionTypeName);
}

bool UTunaSweeperQuestSubsystem::AdvanceObjectiveProgress(int32 QuestId, FName ObjectiveId, int32 Amount)
{
	const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId);
	if (!Definition || ObjectiveId.IsNone() || Amount <= 0)
	{
		return false;
	}

	const FTunaSweeperObjectiveDefinition* Objective = Definition->Objectives.FindByPredicate(
		[ObjectiveId](const FTunaSweeperObjectiveDefinition& Candidate)
		{
			return Candidate.ObjectiveId == ObjectiveId;
		});
	if (!Objective)
	{
		return false;
	}

	FTunaSweeperNumericQuestProgressSaveData& Progress = GetOrCreateQuestProgress(QuestId);
	FTunaSweeperObjectiveProgressSaveData* ObjectiveProgress = Progress.ObjectiveProgress.FindByPredicate(
		[ObjectiveId](const FTunaSweeperObjectiveProgressSaveData& Candidate)
		{
			return Candidate.ObjectiveId == ObjectiveId;
		});
	if (!ObjectiveProgress)
	{
		ObjectiveProgress = &Progress.ObjectiveProgress.AddDefaulted_GetRef();
		ObjectiveProgress->ObjectiveId = ObjectiveId;
	}

	const int32 RequiredCount = FMath::Max(1, Objective->RequiredCount);
	const int32 OldCount = FMath::Clamp(ObjectiveProgress->CurrentCount, 0, RequiredCount);
	const int32 NewCount = FMath::Clamp(OldCount + Amount, 0, RequiredCount);
	ObjectiveProgress->CurrentCount = NewCount;
	return NewCount != OldCount;
}

void UTunaSweeperQuestSubsystem::AdvanceMatchingObjectives(
	TFunctionRef<bool(const FTunaSweeperObjectiveDefinition&)> Predicate,
	int32 Amount,
	bool bSaveImmediately)
{
	if (const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance()); Instance && Instance->IsCombatTestSession()) { return; }
	if (!EnsureQuestDataLoaded() || Amount <= 0)
	{
		return;
	}

	EnsureSaveStateLoaded();
	bool bChanged = false;
	for (const TPair<int32, FTunaSweeperQuestDefinition>& QuestPair : QuestDefinitions)
	{
		if (GetQuestState(QuestPair.Key) != ETunaSweeperQuestState::Accepted)
		{
			continue;
		}

		bool bQuestChanged = false;
		for (const FTunaSweeperObjectiveDefinition& Objective : QuestPair.Value.Objectives)
		{
			if (Predicate(Objective))
			{
				bQuestChanged |= AdvanceObjectiveProgress(QuestPair.Key, Objective.ObjectiveId, Amount);
			}
		}

		if (bQuestChanged && AreAllObjectivesComplete(QuestPair.Key))
		{
			SetQuestState(QuestPair.Key, ETunaSweeperQuestState::RewardAvailable);
			ShowQuestCompletedToast(QuestPair.Key);
		}

		bChanged |= bQuestChanged;
	}

	if (bChanged)
	{
		BroadcastQuestProgressChanged(bSaveImmediately);
	}
}

void UTunaSweeperQuestSubsystem::SetQuestState(int32 QuestId, ETunaSweeperQuestState NewState)
{
	if (FindQuestDefinition(QuestId))
	{
		GetOrCreateQuestProgress(QuestId).State = NewState;
	}
}

void UTunaSweeperQuestSubsystem::ShowQuestCompletedToast(int32 QuestId) const
{
	FTunaSweeperQuestDefinition Definition;
	if (!TryGetQuestDefinition(QuestId, Definition))
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperToastSubsystem* ToastSubsystem = GameInstance->GetSubsystem<UTunaSweeperToastSubsystem>())
		{
			ToastSubsystem->ShowQuestCompletedToast(Definition.Title);
		}
	}
}

bool UTunaSweeperQuestSubsystem::AreDefinitionPrerequisitesMet(const FTunaSweeperQuestDefinition& Definition) const
{
	for (const int32 RequiredQuestId : Definition.RequiredCompletedQuestIds)
	{
		if (RequiredQuestId <= 0 || GetQuestState(RequiredQuestId) != ETunaSweeperQuestState::RewardCompleted)
		{
			return false;
		}
	}

	return true;
}

bool UTunaSweeperQuestSubsystem::IsQuestForProvider(
	const FTunaSweeperQuestDefinition& Definition,
	FName ProviderId) const
{
	return !ProviderId.IsNone() && Definition.ProviderId == ProviderId;
}

bool UTunaSweeperQuestSubsystem::AreAllObjectivesComplete(int32 QuestId) const
{
	const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId);
	if (!Definition || Definition->Objectives.Num() <= 0)
	{
		return false;
	}

	for (const FTunaSweeperObjectiveDefinition& Objective : Definition->Objectives)
	{
		if (GetObjectiveProgressCount(QuestId, Objective.ObjectiveId) < FMath::Max(1, Objective.RequiredCount))
		{
			return false;
		}
	}

	return true;
}

int32 UTunaSweeperQuestSubsystem::GetObjectiveProgressCount(int32 QuestId, FName ObjectiveId) const
{
	const FTunaSweeperNumericQuestProgressSaveData* Progress = QuestProgressById.Find(QuestId);
	if (!Progress)
	{
		return 0;
	}

	const FTunaSweeperObjectiveProgressSaveData* ObjectiveProgress = Progress->ObjectiveProgress.FindByPredicate(
		[ObjectiveId](const FTunaSweeperObjectiveProgressSaveData& Candidate)
		{
			return Candidate.ObjectiveId == ObjectiveId;
		});
	return ObjectiveProgress ? FMath::Max(0, ObjectiveProgress->CurrentCount) : 0;
}

FTunaSweeperNumericQuestProgressSaveData& UTunaSweeperQuestSubsystem::GetOrCreateQuestProgress(int32 QuestId)
{
	FTunaSweeperNumericQuestProgressSaveData* Progress = QuestProgressById.Find(QuestId);
	if (!Progress)
	{
		FTunaSweeperNumericQuestProgressSaveData NewProgress;
		NewProgress.QuestId = QuestId;
		NewProgress.bOwnershipReadinessRecorded = true;

		if (const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId))
		{
			NewProgress.ObjectiveProgress.Reserve(Definition->Objectives.Num());
			for (const FTunaSweeperObjectiveDefinition& Objective : Definition->Objectives)
			{
				FTunaSweeperObjectiveProgressSaveData ObjectiveProgress;
				ObjectiveProgress.ObjectiveId = Objective.ObjectiveId;
				NewProgress.ObjectiveProgress.Add(ObjectiveProgress);
			}
		}

		Progress = &QuestProgressById.Add(QuestId, NewProgress);
	}

	return *Progress;
}

void UTunaSweeperQuestSubsystem::BroadcastQuestProgressChanged(bool bSaveImmediately)
{
	OnQuestProgressChanged.Broadcast();
	if (bSaveImmediately)
	{
		RequestSaveGameState();
	}
}

void UTunaSweeperQuestSubsystem::EnsureSaveStateLoaded() const
{
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->GetInventorySlots();
	}
}

void UTunaSweeperQuestSubsystem::RequestSaveGameState() const
{
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->SaveGameState();
	}
}

bool UTunaSweeperQuestSubsystem::IsQuestTrackable(int32 QuestId) const
{
	const ETunaSweeperQuestState State = GetQuestState(QuestId);
	return FindQuestDefinition(QuestId) &&
		(State == ETunaSweeperQuestState::Accepted || State == ETunaSweeperQuestState::RewardAvailable);
}
