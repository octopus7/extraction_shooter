#include "Subsystem/TunaSweeperQuestSubsystem.h"

#include "Dom/JsonObject.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "QuestDatasetSwitcher.h"
#include "Serialization/Csv/CsvParser.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Subsystem/TunaSweeperToastSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperQuest, Log, All);

namespace TunaSweeperQuestIds
{
	const FName FirstOuting(TEXT("quest_first_outing"));
}

namespace TunaSweeperQuestProviders
{
	const FName CanBot(TEXT("provider.canbot"));
}

namespace TunaSweeperQuestData
{
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
		JsonObject->TryGetNumberField(TEXT("required_count"), RequiredCount);
		OutObjective.RequiredCount = FMath::Max(1, FMath::RoundToInt(RequiredCount));

		OutObjective.SourceLevelName = ReadNameField(JsonObject, TEXT("source_level"));
		OutObjective.TargetLevelName = ReadNameField(JsonObject, TEXT("target_level"));
		OutObjective.EnemyId = ReadNameField(JsonObject, TEXT("enemy_id"));
		OutObjective.InteractionEventId = ReadNameField(JsonObject, TEXT("interaction_event_id"));
		OutObjective.WarpPointId = ReadNameField(JsonObject, TEXT("warp_point_id"));
		OutObjective.TargetWarpPointId = ReadNameField(JsonObject, TEXT("target_warp_point_id"));

		double ItemId = INDEX_NONE;
		JsonObject->TryGetNumberField(TEXT("item_id"), ItemId);
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
}

FName UTunaSweeperQuestSubsystem::GetFirstOutingQuestId()
{
	return TunaSweeperQuestIds::FirstOuting;
}

FName UTunaSweeperQuestSubsystem::GetCanBotProviderId()
{
	return TunaSweeperQuestProviders::CanBot;
}

bool UTunaSweeperQuestSubsystem::LoadQuestData(bool bForceReload)
{
	if (bQuestDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedQuestData();
	FQuestDatasetSwitcherModule::Get().ReloadActiveDataset();
	const bool bLoadedQuestTextStrings = LoadQuestTextStringsCsv();
	if (!bLoadedQuestTextStrings || !LoadQuestDefinitionsJson())
	{
		RegisterFallbackQuest();
	}

	bQuestDataLoaded = QuestDefinitions.Num() > 0;
	return bQuestDataLoaded;
}

bool UTunaSweeperQuestSubsystem::TryGetQuestDefinition(
	FName QuestId,
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

const FTunaSweeperQuestDefinition* UTunaSweeperQuestSubsystem::FindQuestDefinition(FName QuestId) const
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

		return Left.QuestId.LexicalLess(Right.QuestId);
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

	switch (Language)
	{
	case ETunaSweeperItemTextLanguage::Korean:
		OutText = QuestTextString->Korean;
		break;
	case ETunaSweeperItemTextLanguage::English:
		OutText = QuestTextString->English;
		break;
	case ETunaSweeperItemTextLanguage::Japanese:
		OutText = QuestTextString->Japanese;
		break;
	default:
		OutText = FText::GetEmpty();
		break;
	}

	return !OutText.IsEmpty();
}

bool UTunaSweeperQuestSubsystem::GetQuestPresentationLines(
	FName QuestId,
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

ETunaSweeperQuestState UTunaSweeperQuestSubsystem::GetQuestState(FName QuestId) const
{
	if (const FTunaSweeperQuestProgressSaveData* Progress = QuestProgressById.Find(QuestId))
	{
		return Progress->State;
	}

	return ETunaSweeperQuestState::Available;
}

bool UTunaSweeperQuestSubsystem::CanAcceptQuest(FName QuestId) const
{
	const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId);
	return Definition &&
		GetQuestState(QuestId) == ETunaSweeperQuestState::Available &&
		AreDefinitionPrerequisitesMet(*Definition);
}

bool UTunaSweeperQuestSubsystem::AreQuestPrerequisitesMet(FName QuestId) const
{
	const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId);
	return Definition && AreDefinitionPrerequisitesMet(*Definition);
}

bool UTunaSweeperQuestSubsystem::AcceptQuest(FName QuestId)
{
	EnsureSaveStateLoaded();
	if (!CanAcceptQuest(QuestId))
	{
		return false;
	}

	SetQuestState(QuestId, ETunaSweeperQuestState::Accepted);

	if (const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId))
	{
		if (Definition->bAutoTrackOnAccept)
		{
			TrackedQuestId = QuestId;
		}
	}

	BroadcastQuestProgressChanged(true);
	return true;
}

bool UTunaSweeperQuestSubsystem::CanClaimQuestReward(FName QuestId) const
{
	return FindQuestDefinition(QuestId) && GetQuestState(QuestId) == ETunaSweeperQuestState::RewardAvailable;
}

bool UTunaSweeperQuestSubsystem::ClaimQuestReward(FName QuestId)
{
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

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (Definition->Rewards.Items.Num() > 0)
	{
		if (!TunaGameInstance || !TunaGameInstance->GrantQuestItemRewards(Definition->Rewards.Items))
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
	if (TrackedQuestId == QuestId)
	{
		TrackedQuestId = NAME_None;
	}

	BroadcastQuestProgressChanged(true);
	return true;
}

bool UTunaSweeperQuestSubsystem::SetTrackedQuest(FName QuestId)
{
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
	if (!TrackedQuestId.IsNone())
	{
		EnsureSaveStateLoaded();
		TrackedQuestId = NAME_None;
		BroadcastQuestProgressChanged(true);
	}
}

bool UTunaSweeperQuestSubsystem::GetQuestObjectiveProgress(
	FName QuestId,
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
	FName FallbackQuestId,
	FName& OutQuestId) const
{
	OutQuestId = NAME_None;
	if (!EnsureQuestDataLoaded())
	{
		return false;
	}

	EnsureSaveStateLoaded();

	TArray<const FTunaSweeperQuestDefinition*> Candidates;
	bool bHasProviderQuests = false;
	for (const TPair<FName, FTunaSweeperQuestDefinition>& QuestPair : QuestDefinitions)
	{
		const FTunaSweeperQuestDefinition& Definition = QuestPair.Value;
		if (!IsQuestForProvider(Definition, ProviderId))
		{
			continue;
		}

		bHasProviderQuests = true;
		const ETunaSweeperQuestState State = GetQuestState(Definition.QuestId);
		if (State == ETunaSweeperQuestState::RewardCompleted ||
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

		return Left.QuestId.LexicalLess(Right.QuestId);
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
	FName& OutQuestId) const
{
	OutQuestId = NAME_None;
	if (!EnsureQuestDataLoaded() || ProviderId.IsNone())
	{
		return false;
	}

	EnsureSaveStateLoaded();

	TArray<const FTunaSweeperQuestDefinition*> Candidates;
	for (const TPair<FName, FTunaSweeperQuestDefinition>& QuestPair : QuestDefinitions)
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

		return Right.QuestId.LexicalLess(Left.QuestId);
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
	AdvanceMatchingObjectives(
		[this, SourceLevelName, TargetLevelName](const FTunaSweeperObjectiveDefinition& Objective)
		{
			return DoesObjectiveMatchLevelTravel(Objective, SourceLevelName, TargetLevelName);
		},
		1);
}

void UTunaSweeperQuestSubsystem::NotifyBunkerRescueReturn(FName SourceLevelName, FName TargetLevelName)
{
	AdvanceMatchingObjectives(
		[this, SourceLevelName, TargetLevelName](const FTunaSweeperObjectiveDefinition& Objective)
		{
			return DoesObjectiveMatchBunkerRescueReturn(Objective, SourceLevelName, TargetLevelName);
		},
		1);
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

void UTunaSweeperQuestSubsystem::AddCoins(int32 Amount, bool bSaveImmediately)
{
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
	TArray<FTunaSweeperQuestProgressSaveData>& OutQuestProgress,
	FName& OutTrackedQuestId,
	int32& OutQuestCoinBalance) const
{
	OutQuestProgress.Reset();
	QuestProgressById.GenerateValueArray(OutQuestProgress);
	OutQuestProgress.Sort([](
		const FTunaSweeperQuestProgressSaveData& Left,
		const FTunaSweeperQuestProgressSaveData& Right)
	{
		return Left.QuestId.LexicalLess(Right.QuestId);
	});

	OutTrackedQuestId = TrackedQuestId;
	OutQuestCoinBalance = FMath::Max(0, CoinBalance);
}

void UTunaSweeperQuestSubsystem::LoadQuestProgressFromSave(
	const TArray<FTunaSweeperQuestProgressSaveData>& SavedQuestProgress,
	FName SavedTrackedQuestId,
	int32 SavedQuestCoinBalance)
{
	LoadQuestData(false);
	QuestProgressById.Reset();
	CoinBalance = FMath::Max(0, SavedQuestCoinBalance);
	TrackedQuestId = NAME_None;

	for (const FTunaSweeperQuestProgressSaveData& SavedProgress : SavedQuestProgress)
	{
		const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(SavedProgress.QuestId);
		if (!Definition)
		{
			continue;
		}

		FTunaSweeperQuestProgressSaveData LoadedProgress;
		LoadedProgress.QuestId = SavedProgress.QuestId;
		LoadedProgress.State = SavedProgress.State;

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
			LoadedProgress.ObjectiveProgress.Add(LoadedObjectiveProgress);
		}

		QuestProgressById.Add(LoadedProgress.QuestId, LoadedProgress);
	}

	if (IsQuestTrackable(SavedTrackedQuestId))
	{
		TrackedQuestId = SavedTrackedQuestId;
	}

	OnQuestProgressChanged.Broadcast();
}

void UTunaSweeperQuestSubsystem::ResetQuestProgressForNewGame()
{
	QuestProgressById.Reset();
	TrackedQuestId = NAME_None;
	CoinBalance = 0;
	OnQuestProgressChanged.Broadcast();
}

bool UTunaSweeperQuestSubsystem::EnsureQuestDataLoaded() const
{
	return bQuestDataLoaded && QuestDefinitions.Num() > 0;
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
		Definition.QuestId = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("quest_id"));
		if (Definition.QuestId.IsNone())
		{
			continue;
		}

		Definition.TitleStringKey = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("title_string_key"));
		Definition.Title = TunaSweeperQuestData::ReadTextField(QuestObject, TEXT("title"));
		Definition.DescriptionStringKey = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("description_string_key"));
		Definition.Description = TunaSweeperQuestData::ReadTextField(QuestObject, TEXT("description"));
		Definition.ProviderId = TunaSweeperQuestData::ReadNameField(QuestObject, TEXT("provider_id"));
		double SortOrder = 0.0;
		QuestObject->TryGetNumberField(TEXT("sort_order"), SortOrder);
		Definition.SortOrder = FMath::RoundToInt(SortOrder);
		TunaSweeperQuestData::ReadNameArrayField(
			QuestObject,
			TEXT("required_completed_quest_ids"),
			Definition.RequiredCompletedQuestIds);
		QuestObject->TryGetBoolField(TEXT("auto_track_on_accept"), Definition.bAutoTrackOnAccept);

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
			}
		}

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

		if (Definition.Objectives.Num() > 0)
		{
			ResolveDefinitionText(Definition);
			QuestDefinitions.Add(Definition.QuestId, Definition);
		}
	}

	return QuestDefinitions.Num() > 0;
}

bool UTunaSweeperQuestSubsystem::LoadQuestTextStringsCsv()
{
	FString CsvContent;
	const FString QuestTextStringsCsvPath = GetQuestTextStringsCsvPath();
	if (!FFileHelper::LoadFileToString(CsvContent, *QuestTextStringsCsvPath))
	{
		UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Could not load quest text strings: %s"), *QuestTextStringsCsvPath);
		return false;
	}

	FCsvParser CsvParser(CsvContent);
	const FCsvParser::FRows& Rows = CsvParser.GetRows();
	if (Rows.Num() < 2)
	{
		UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Quest text strings CSV has no data rows: %s"), *QuestTextStringsCsvPath);
		return false;
	}

	const TArray<const TCHAR*>& HeaderRow = Rows[0];
	const bool bHeaderIsValid =
		TunaSweeperQuestData::GetCsvCell(HeaderRow, 0).Equals(TEXT("string_key"), ESearchCase::IgnoreCase) &&
		TunaSweeperQuestData::GetCsvCell(HeaderRow, 1).Equals(TEXT("ko"), ESearchCase::IgnoreCase) &&
		TunaSweeperQuestData::GetCsvCell(HeaderRow, 2).Equals(TEXT("en"), ESearchCase::IgnoreCase) &&
		TunaSweeperQuestData::GetCsvCell(HeaderRow, 3).Equals(TEXT("ja"), ESearchCase::IgnoreCase);
	if (!bHeaderIsValid)
	{
		UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Quest text strings CSV header must be string_key,ko,en,ja: %s"), *QuestTextStringsCsvPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<const TCHAR*>& Row = Rows[RowIndex];
		if (Row.Num() < 4)
		{
			UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Skipping quest text row %d: expected 4 columns."), RowIndex);
			continue;
		}

		const FString StringKey = TunaSweeperQuestData::GetCsvCell(Row, 0);
		const FString Korean = TunaSweeperQuestData::GetCsvCell(Row, 1);
		const FString English = TunaSweeperQuestData::GetCsvCell(Row, 2);
		const FString Japanese = TunaSweeperQuestData::GetCsvCell(Row, 3);
		if (StringKey.IsEmpty() || Korean.IsEmpty() || English.IsEmpty() || Japanese.IsEmpty())
		{
			UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Skipping quest text row %d: required cell is empty."), RowIndex);
			continue;
		}

		FTunaSweeperQuestTextString QuestTextString;
		QuestTextString.StringKey = FName(*StringKey);
		QuestTextString.Korean = FText::FromString(Korean);
		QuestTextString.English = FText::FromString(English);
		QuestTextString.Japanese = FText::FromString(Japanese);

		if (QuestTextStringsByKey.Contains(QuestTextString.StringKey))
		{
			UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Duplicate quest text string key %s found. The later row will replace the earlier row."), *StringKey);
		}

		QuestTextStringsByKey.Add(QuestTextString.StringKey, QuestTextString);
		bHasValidRows = true;
	}

	if (!bHasValidRows)
	{
		UE_LOG(LogTunaSweeperQuest, Warning, TEXT("Quest text strings CSV has no valid rows: %s"), *QuestTextStringsCsvPath);
	}

	return bHasValidRows;
}

void UTunaSweeperQuestSubsystem::RegisterFallbackQuest()
{
	FTunaSweeperObjectiveDefinition FirstObjective;
	FirstObjective.ObjectiveId = FName(TEXT("leave_bunker"));
	FirstObjective.Type = ETunaSweeperObjectiveType::LevelTravel;
	FirstObjective.TextStringKey = FName(TEXT("quest.first_outing.objective.leave_bunker"));
	FirstObjective.Text = FText::FromString(TEXT("\uBC99\uCEE4 \uBC16\uC73C\uB85C \uC774\uB3D9"));
	FirstObjective.RequiredCount = 1;
	FirstObjective.SourceLevelName = FName(TEXT("BunkerMap"));
	FirstObjective.TargetLevelName = FName(TEXT("RaidMap"));

	FTunaSweeperQuestDefinition FirstOuting;
	FirstOuting.QuestId = GetFirstOutingQuestId();
	FirstOuting.ProviderId = GetCanBotProviderId();
	FirstOuting.SortOrder = 10;
	FirstOuting.TitleStringKey = FName(TEXT("quest.first_outing.title"));
	FirstOuting.Title = FText::FromString(TEXT("\uCCAB \uC678\uCD9C"));
	FirstOuting.DescriptionStringKey = FName(TEXT("quest.first_outing.description"));
	FirstOuting.Description = FText::FromString(TEXT("\uC774\uC81C \uB4E4\uC5B4\uC654\uC73C\uB2C8 \uB098\uAC00\uC11C \uD55C\uBC88 \uC0B0\uCC45\uD558\uACE0 \uB4E4\uC5B4\uC640"));
	FirstOuting.Objectives.Add(FirstObjective);
	FirstOuting.Rewards.Coins = 100;

	QuestDefinitions.Add(FirstOuting.QuestId, FirstOuting);
}

void UTunaSweeperQuestSubsystem::ResetLoadedQuestData()
{
	QuestDefinitions.Reset();
	QuestTextStringsByKey.Reset();
	bQuestDataLoaded = false;
}

FString UTunaSweeperQuestSubsystem::GetQuestDefinitionsJsonPath() const
{
	return FQuestDatasetSwitcherModule::Get().GetActiveDataset().QuestDefinitionsPath;
}

FString UTunaSweeperQuestSubsystem::GetQuestTextStringsCsvPath() const
{
	return FQuestDatasetSwitcherModule::Get().GetActiveDataset().QuestTextStringsPath;
}

void UTunaSweeperQuestSubsystem::ResolveDefinitionText(FTunaSweeperQuestDefinition& Definition) const
{
	Definition.Title = ResolveQuestText(Definition.TitleStringKey, Definition.Title);
	Definition.Description = ResolveQuestText(Definition.DescriptionStringKey, Definition.Description);
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

	const FString ActualMapString = ActualMapName.ToString();
	return ActualMapString == ExpectedMapName || ActualMapString.EndsWith(FString::Printf(TEXT("_%s"), ExpectedMapName));
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

bool UTunaSweeperQuestSubsystem::AdvanceObjectiveProgress(FName QuestId, FName ObjectiveId, int32 Amount)
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

	FTunaSweeperQuestProgressSaveData& Progress = GetOrCreateQuestProgress(QuestId);
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
	if (!EnsureQuestDataLoaded() || Amount <= 0)
	{
		return;
	}

	EnsureSaveStateLoaded();
	bool bChanged = false;
	for (const TPair<FName, FTunaSweeperQuestDefinition>& QuestPair : QuestDefinitions)
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

void UTunaSweeperQuestSubsystem::SetQuestState(FName QuestId, ETunaSweeperQuestState NewState)
{
	if (FindQuestDefinition(QuestId))
	{
		GetOrCreateQuestProgress(QuestId).State = NewState;
	}
}

void UTunaSweeperQuestSubsystem::ShowQuestCompletedToast(FName QuestId) const
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
	for (const FName RequiredQuestId : Definition.RequiredCompletedQuestIds)
	{
		if (!RequiredQuestId.IsNone() && GetQuestState(RequiredQuestId) != ETunaSweeperQuestState::RewardCompleted)
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

bool UTunaSweeperQuestSubsystem::AreAllObjectivesComplete(FName QuestId) const
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

int32 UTunaSweeperQuestSubsystem::GetObjectiveProgressCount(FName QuestId, FName ObjectiveId) const
{
	const FTunaSweeperQuestProgressSaveData* Progress = QuestProgressById.Find(QuestId);
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

FTunaSweeperQuestProgressSaveData& UTunaSweeperQuestSubsystem::GetOrCreateQuestProgress(FName QuestId)
{
	FTunaSweeperQuestProgressSaveData* Progress = QuestProgressById.Find(QuestId);
	if (!Progress)
	{
		FTunaSweeperQuestProgressSaveData NewProgress;
		NewProgress.QuestId = QuestId;

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

bool UTunaSweeperQuestSubsystem::IsQuestTrackable(FName QuestId) const
{
	const ETunaSweeperQuestState State = GetQuestState(QuestId);
	return FindQuestDefinition(QuestId) &&
		(State == ETunaSweeperQuestState::Accepted || State == ETunaSweeperQuestState::RewardAvailable);
}
