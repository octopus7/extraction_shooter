#include "Subsystem/TunaSweeperScenarioSubsystem.h"

#include "Dom/JsonObject.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Misc/FileHelper.h"
#include "Serialization/Csv/CsvParser.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperScenarioData, Log, All);

namespace TunaSweeperScenarioData
{
	FString GetCsvCell(const TArray<const TCHAR*>& Row, int32 CellIndex)
	{
		return Row.IsValidIndex(CellIndex) ? FString(Row[CellIndex]).TrimStartAndEnd() : FString();
	}

	FName ReadNameField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName)
	{
		FString Value;
		return Object.IsValid() && Object->TryGetStringField(FieldName, Value) && !Value.TrimStartAndEnd().IsEmpty()
			? FName(*Value.TrimStartAndEnd())
			: NAME_None;
	}

	void ReadNameArrayField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, TArray<FName>& OutNames)
	{
		OutNames.Reset();
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, Values) || !Values)
		{
			return;
		}
		for (const TSharedPtr<FJsonValue>& Value : *Values)
		{
			FString RawName;
			if (Value.IsValid() && Value->TryGetString(RawName))
			{
				const FString Name = RawName.TrimStartAndEnd();
				if (!Name.IsEmpty())
				{
					OutNames.AddUnique(FName(*Name));
				}
			}
		}
	}

	bool ReadVectorField(const TSharedPtr<FJsonObject>& Object, const TCHAR* FieldName, FVector& OutVector)
	{
		const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
		if (!Object.IsValid() || !Object->TryGetArrayField(FieldName, Values) || !Values || Values->Num() < 3)
		{
			return false;
		}
		double X = 0.0;
		double Y = 0.0;
		double Z = 0.0;
		if (!(*Values)[0].IsValid() || !(*Values)[0]->TryGetNumber(X) ||
			!(*Values)[1].IsValid() || !(*Values)[1]->TryGetNumber(Y) ||
			!(*Values)[2].IsValid() || !(*Values)[2]->TryGetNumber(Z))
		{
			return false;
		}
		OutVector = FVector(X, Y, Z);
		return true;
	}

	bool ParseQuestState(const FString& RawState, ETunaSweeperQuestState& OutState)
	{
		const FString State = RawState.TrimStartAndEnd().ToLower();
		if (State == TEXT("available"))
		{
			OutState = ETunaSweeperQuestState::Available;
			return true;
		}
		if (State == TEXT("accepted"))
		{
			OutState = ETunaSweeperQuestState::Accepted;
			return true;
		}
		if (State == TEXT("reward_available") || State == TEXT("rewardavailable"))
		{
			OutState = ETunaSweeperQuestState::RewardAvailable;
			return true;
		}
		if (State == TEXT("reward_completed") || State == TEXT("rewardcompleted") || State == TEXT("completed"))
		{
			OutState = ETunaSweeperQuestState::RewardCompleted;
			return true;
		}
		return false;
	}

	FString NormalizeLevelName(FName LevelName)
	{
		FString Result = LevelName.ToString();
		int32 SlashIndex = INDEX_NONE;
		if (Result.FindLastChar(TEXT('/'), SlashIndex))
		{
			Result.RightChopInline(SlashIndex + 1);
		}
		while (Result.StartsWith(TEXT("UEDPIE_")))
		{
			int32 PrefixEnd = INDEX_NONE;
			if (!Result.FindChar(TEXT('_'), PrefixEnd))
			{
				break;
			}
			Result.RightChopInline(PrefixEnd + 1);
			if (!Result.IsEmpty() && FChar::IsDigit(Result[0]))
			{
				int32 NextUnderscore = INDEX_NONE;
				if (Result.FindChar(TEXT('_'), NextUnderscore))
				{
					Result.RightChopInline(NextUnderscore + 1);
				}
			}
		}
		return Result;
	}
}

void UTunaSweeperScenarioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	LoadScenarioData(false);
}

bool UTunaSweeperScenarioSubsystem::LoadScenarioData(bool bForceReload)
{
	if (bScenarioDataLoaded && !bForceReload)
	{
		return true;
	}
	ResetLoadedScenarioData();
	bScenarioDataLoaded = LoadScenarioDefinitionsJson() && LoadScenarioTextStringsCsv();
	if (!bScenarioDataLoaded)
	{
		ResetLoadedScenarioData();
	}
	return bScenarioDataLoaded;
}

bool UTunaSweeperScenarioSubsystem::TryResolveScenario(
	FName TriggerName,
	FName LevelName,
	bool bIgnoreOneShotCompletion,
	FTunaSweeperScenarioPresentation& OutPresentation) const
{
	OutPresentation = FTunaSweeperScenarioPresentation();
	if (TriggerName.IsNone() || !EnsureScenarioDataLoaded())
	{
		return false;
	}

	for (const FTunaSweeperScenarioDefinition& Definition : ScenarioDefinitions)
	{
		if (!AreConditionsMet(Definition, TriggerName, LevelName, bIgnoreOneShotCompletion))
		{
			continue;
		}

		TArray<FTunaSweeperDialogueLine> Lines;
		Lines.Reserve(Definition.Lines.Num());
		for (const FTunaSweeperScenarioLineDefinition& LineDefinition : Definition.Lines)
		{
			FTunaSweeperDialogueLine Line;
			Line.SpeakerName = ResolveScenarioText(LineDefinition.SpeakerNameStringKey);
			Line.DialogueText = ResolveScenarioText(LineDefinition.DialogueTextStringKey);
			if (Line.SpeakerName.IsEmpty() || Line.DialogueText.IsEmpty())
			{
				UE_LOG(LogTunaSweeperScenarioData, Error, TEXT("Scenario %s has an unresolved text key."), *Definition.ScenarioId.ToString());
				Lines.Reset();
				break;
			}
			Line.bUseCameraFocus = LineDefinition.bUseCameraFocus;
			Line.CameraFocusLocation = LineDefinition.CameraFocusLocation;
			Line.CameraBlendSeconds = LineDefinition.CameraBlendSeconds;
			Lines.Add(MoveTemp(Line));
		}
		if (Lines.IsEmpty())
		{
			continue;
		}

		OutPresentation.ScenarioId = Definition.ScenarioId;
		OutPresentation.CompletionFlag = Definition.CompletionFlag;
		OutPresentation.DialogueLines = MoveTemp(Lines);
		OutPresentation.StartDelaySeconds = Definition.StartDelaySeconds;
		return true;
	}
	return false;
}

bool UTunaSweeperScenarioSubsystem::EnsureScenarioDataLoaded() const
{
	return bScenarioDataLoaded || const_cast<UTunaSweeperScenarioSubsystem*>(this)->LoadScenarioData(false);
}

bool UTunaSweeperScenarioSubsystem::LoadScenarioDefinitionsJson()
{
	const FString JsonPath = TunaSweeperBuildFlavor::GetScenarioDefinitionsPath();
	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		UE_LOG(LogTunaSweeperScenarioData, Error, TEXT("Failed to read scenario definitions: %s"), *JsonPath);
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(JsonContent), RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTunaSweeperScenarioData, Error, TEXT("Failed to parse scenario definitions: %s"), *JsonPath);
		return false;
	}

	double SchemaVersion = 0.0;
	FString BuildFlavor;
	const FString ExpectedFlavor = TunaSweeperBuildFlavor::IsDemo() ? TEXT("demo") : TEXT("main");
	if (!RootObject->TryGetNumberField(TEXT("schema_version"), SchemaVersion) || FMath::RoundToInt(SchemaVersion) != 1 ||
		!RootObject->TryGetStringField(TEXT("build_flavor"), BuildFlavor) ||
		!BuildFlavor.Equals(ExpectedFlavor, ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTunaSweeperScenarioData, Error, TEXT("Scenario pack flavor/schema mismatch: %s"), *JsonPath);
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* ScenarioValues = nullptr;
	if (!RootObject->TryGetArrayField(TEXT("scenarios"), ScenarioValues) || !ScenarioValues)
	{
		UE_LOG(LogTunaSweeperScenarioData, Error, TEXT("Scenario pack has no scenarios array: %s"), *JsonPath);
		return false;
	}

	TSet<FName> ScenarioIds;
	for (const TSharedPtr<FJsonValue>& ScenarioValue : *ScenarioValues)
	{
		const TSharedPtr<FJsonObject> ScenarioObject = ScenarioValue.IsValid() ? ScenarioValue->AsObject() : nullptr;
		if (!ScenarioObject.IsValid())
		{
			continue;
		}

		FTunaSweeperScenarioDefinition Definition;
		Definition.ScenarioId = TunaSweeperScenarioData::ReadNameField(ScenarioObject, TEXT("scenario_id"));
		TunaSweeperScenarioData::ReadNameArrayField(ScenarioObject, TEXT("triggers"), Definition.TriggerNames);
		Definition.LevelName = TunaSweeperScenarioData::ReadNameField(ScenarioObject, TEXT("level_name"));
		Definition.CompletionFlag = TunaSweeperScenarioData::ReadNameField(ScenarioObject, TEXT("completion_flag"));
		TunaSweeperScenarioData::ReadNameArrayField(
			ScenarioObject,
			TEXT("required_completed_flags"),
			Definition.RequiredCompletedFlags);
		TunaSweeperScenarioData::ReadNameArrayField(
			ScenarioObject,
			TEXT("blocked_completed_flags"),
			Definition.BlockedCompletedFlags);

		double Priority = 0.0;
		ScenarioObject->TryGetNumberField(TEXT("priority"), Priority);
		Definition.Priority = FMath::RoundToInt(Priority);
		double Delay = 0.0;
		ScenarioObject->TryGetNumberField(TEXT("start_delay_seconds"), Delay);
		Definition.StartDelaySeconds = FMath::Max(0.0f, static_cast<float>(Delay));
		ScenarioObject->TryGetBoolField(TEXT("one_shot"), Definition.bOneShot);

		const TArray<TSharedPtr<FJsonValue>>* QuestConditions = nullptr;
		if (ScenarioObject->TryGetArrayField(TEXT("required_quest_states"), QuestConditions) && QuestConditions)
		{
			for (const TSharedPtr<FJsonValue>& ConditionValue : *QuestConditions)
			{
				const TSharedPtr<FJsonObject> ConditionObject = ConditionValue.IsValid() ? ConditionValue->AsObject() : nullptr;
				FTunaSweeperScenarioQuestStateCondition Condition;
				Condition.QuestId = TunaSweeperScenarioData::ReadNameField(ConditionObject, TEXT("quest_id"));
				FString StateName;
				if (ConditionObject.IsValid() && ConditionObject->TryGetStringField(TEXT("state"), StateName) &&
					!Condition.QuestId.IsNone() && TunaSweeperScenarioData::ParseQuestState(StateName, Condition.RequiredState))
				{
					Definition.RequiredQuestStates.Add(Condition);
				}
			}
		}

		const TArray<TSharedPtr<FJsonValue>>* LineValues = nullptr;
		if (ScenarioObject->TryGetArrayField(TEXT("lines"), LineValues) && LineValues)
		{
			for (const TSharedPtr<FJsonValue>& LineValue : *LineValues)
			{
				const TSharedPtr<FJsonObject> LineObject = LineValue.IsValid() ? LineValue->AsObject() : nullptr;
				FTunaSweeperScenarioLineDefinition Line;
				Line.SpeakerNameStringKey = TunaSweeperScenarioData::ReadNameField(LineObject, TEXT("speaker_name_string_key"));
				Line.DialogueTextStringKey = TunaSweeperScenarioData::ReadNameField(LineObject, TEXT("dialogue_text_string_key"));
				if (LineObject.IsValid())
				{
					LineObject->TryGetBoolField(TEXT("use_camera_focus"), Line.bUseCameraFocus);
					TunaSweeperScenarioData::ReadVectorField(LineObject, TEXT("camera_focus_location"), Line.CameraFocusLocation);
					double BlendSeconds = Line.CameraBlendSeconds;
					LineObject->TryGetNumberField(TEXT("camera_blend_seconds"), BlendSeconds);
					Line.CameraBlendSeconds = FMath::Max(0.0f, static_cast<float>(BlendSeconds));
				}
				if (!Line.SpeakerNameStringKey.IsNone() && !Line.DialogueTextStringKey.IsNone())
				{
					Definition.Lines.Add(Line);
				}
			}
		}

		const bool bValid = !Definition.ScenarioId.IsNone() && !ScenarioIds.Contains(Definition.ScenarioId) &&
			!Definition.TriggerNames.IsEmpty() && !Definition.LevelName.IsNone() && !Definition.Lines.IsEmpty() &&
			(!Definition.bOneShot || !Definition.CompletionFlag.IsNone());
		if (!bValid)
		{
			UE_LOG(LogTunaSweeperScenarioData, Warning, TEXT("Skipping invalid scenario row in %s"), *JsonPath);
			continue;
		}

		ScenarioIds.Add(Definition.ScenarioId);
		ScenarioDefinitions.Add(MoveTemp(Definition));
	}

	ScenarioDefinitions.StableSort([](const FTunaSweeperScenarioDefinition& A, const FTunaSweeperScenarioDefinition& B)
	{
		return A.Priority > B.Priority;
	});
	return !ScenarioDefinitions.IsEmpty();
}

bool UTunaSweeperScenarioSubsystem::LoadScenarioTextStringsCsv()
{
	const FString CsvPath = TunaSweeperBuildFlavor::GetScenarioTextStringsPath();
	FString CsvContent;
	if (!FFileHelper::LoadFileToString(CsvContent, *CsvPath))
	{
		UE_LOG(LogTunaSweeperScenarioData, Error, TEXT("Failed to read scenario strings: %s"), *CsvPath);
		return false;
	}

	FCsvParser Parser(CsvContent);
	const FCsvParser::FRows& Rows = Parser.GetRows();
	if (Rows.Num() < 2)
	{
		return false;
	}
	const TArray<const TCHAR*>& Header = Rows[0];
	if (!TunaSweeperScenarioData::GetCsvCell(Header, 0).Equals(TEXT("string_key"), ESearchCase::IgnoreCase) ||
		!TunaSweeperScenarioData::GetCsvCell(Header, 1).Equals(TEXT("ko"), ESearchCase::IgnoreCase) ||
		!TunaSweeperScenarioData::GetCsvCell(Header, 2).Equals(TEXT("en"), ESearchCase::IgnoreCase) ||
		!TunaSweeperScenarioData::GetCsvCell(Header, 3).Equals(TEXT("ja"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTunaSweeperScenarioData, Error, TEXT("Scenario CSV header must be string_key,ko,en,ja: %s"), *CsvPath);
		return false;
	}

	for (int32 RowIndex = 1; RowIndex < Rows.Num(); ++RowIndex)
	{
		const TArray<const TCHAR*>& Row = Rows[RowIndex];
		const FString Key = TunaSweeperScenarioData::GetCsvCell(Row, 0);
		const FString Korean = TunaSweeperScenarioData::GetCsvCell(Row, 1);
		const FString English = TunaSweeperScenarioData::GetCsvCell(Row, 2);
		const FString Japanese = TunaSweeperScenarioData::GetCsvCell(Row, 3);
		if (Key.IsEmpty() || Korean.IsEmpty() || English.IsEmpty() || Japanese.IsEmpty())
		{
			UE_LOG(LogTunaSweeperScenarioData, Warning, TEXT("Skipping invalid scenario string row %d."), RowIndex);
			continue;
		}

		const FName StringKey(*Key);
		if (ScenarioTextStringsByKey.Contains(StringKey))
		{
			UE_LOG(LogTunaSweeperScenarioData, Error, TEXT("Duplicate scenario string key: %s"), *Key);
			return false;
		}
		FTunaSweeperScenarioLocalizedText Text;
		Text.Korean = FText::FromString(Korean);
		Text.English = FText::FromString(English);
		Text.Japanese = FText::FromString(Japanese);
		ScenarioTextStringsByKey.Add(StringKey, MoveTemp(Text));
	}
	return !ScenarioTextStringsByKey.IsEmpty();
}

void UTunaSweeperScenarioSubsystem::ResetLoadedScenarioData()
{
	ScenarioDefinitions.Reset();
	ScenarioTextStringsByKey.Reset();
	bScenarioDataLoaded = false;
}

bool UTunaSweeperScenarioSubsystem::AreConditionsMet(
	const FTunaSweeperScenarioDefinition& Definition,
	FName TriggerName,
	FName LevelName,
	bool bIgnoreOneShotCompletion) const
{
	if (!Definition.TriggerNames.Contains(TriggerName) ||
		TunaSweeperScenarioData::NormalizeLevelName(Definition.LevelName) != TunaSweeperScenarioData::NormalizeLevelName(LevelName))
	{
		return false;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance)
	{
		return false;
	}
	if (Definition.bOneShot && !bIgnoreOneShotCompletion &&
		TunaGameInstance->IsScenarioProgressFlagSet(Definition.CompletionFlag))
	{
		return false;
	}
	for (const FName RequiredFlag : Definition.RequiredCompletedFlags)
	{
		if (!TunaGameInstance->IsScenarioProgressFlagSet(RequiredFlag))
		{
			return false;
		}
	}
	for (const FName BlockedFlag : Definition.BlockedCompletedFlags)
	{
		if (TunaGameInstance->IsScenarioProgressFlagSet(BlockedFlag))
		{
			return false;
		}
	}

	if (!Definition.RequiredQuestStates.IsEmpty())
	{
		const UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>();
		if (!QuestSubsystem)
		{
			return false;
		}
		for (const FTunaSweeperScenarioQuestStateCondition& Condition : Definition.RequiredQuestStates)
		{
			if (QuestSubsystem->GetQuestState(Condition.QuestId) != Condition.RequiredState)
			{
				return false;
			}
		}
	}
	return true;
}

FText UTunaSweeperScenarioSubsystem::ResolveScenarioText(FName StringKey) const
{
	const FTunaSweeperScenarioLocalizedText* Text = ScenarioTextStringsByKey.Find(StringKey);
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!Text || !TunaGameInstance)
	{
		return FText::GetEmpty();
	}

	switch (TunaGameInstance->GetCurrentTextLanguage())
	{
	case ETunaSweeperItemTextLanguage::Korean:
		return Text->Korean;
	case ETunaSweeperItemTextLanguage::Japanese:
		return Text->Japanese;
	case ETunaSweeperItemTextLanguage::English:
	default:
		return Text->English;
	}
}
