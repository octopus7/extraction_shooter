#pragma once

#include "CoreMinimal.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UI/TunaSweeperDialogueWidget.h"
#include "TunaSweeperScenarioSubsystem.generated.h"

struct FTunaSweeperScenarioQuestStateCondition
{
	FName QuestId = NAME_None;
	ETunaSweeperQuestState RequiredState = ETunaSweeperQuestState::Available;
};

struct FTunaSweeperScenarioLineDefinition
{
	FName SpeakerNameStringKey = NAME_None;
	FName DialogueTextStringKey = NAME_None;
	bool bUseCameraFocus = false;
	FVector CameraFocusLocation = FVector::ZeroVector;
	float CameraBlendSeconds = 0.75f;
};

struct FTunaSweeperScenarioDefinition
{
	FName ScenarioId = NAME_None;
	TArray<FName> TriggerNames;
	FName LevelName = NAME_None;
	TArray<FName> RequiredCompletedFlags;
	TArray<FName> BlockedCompletedFlags;
	TArray<FTunaSweeperScenarioQuestStateCondition> RequiredQuestStates;
	FName CompletionFlag = NAME_None;
	TArray<FTunaSweeperScenarioLineDefinition> Lines;
	float StartDelaySeconds = 0.0f;
	int32 Priority = 0;
	bool bOneShot = true;
};

struct FTunaSweeperScenarioPresentation
{
	FName ScenarioId = NAME_None;
	FName CompletionFlag = NAME_None;
	TArray<FTunaSweeperDialogueLine> DialogueLines;
	float StartDelaySeconds = 0.0f;
};

struct FTunaSweeperScenarioLocalizedText
{
	FText Korean;
	FText English;
	FText Japanese;
};

/** Loads the active build flavor's scenario pack and resolves eligible presentations from runtime triggers. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperScenarioSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	bool LoadScenarioData(bool bForceReload = false);
	bool TryResolveScenario(
		FName TriggerName,
		FName LevelName,
		bool bIgnoreOneShotCompletion,
		FTunaSweeperScenarioPresentation& OutPresentation) const;

private:
	bool EnsureScenarioDataLoaded() const;
	bool LoadScenarioDefinitionsJson();
	bool LoadScenarioTextStringsCsv();
	void ResetLoadedScenarioData();
	bool AreConditionsMet(
		const FTunaSweeperScenarioDefinition& Definition,
		FName TriggerName,
		FName LevelName,
		bool bIgnoreOneShotCompletion) const;
	FText ResolveScenarioText(FName StringKey) const;

	TArray<FTunaSweeperScenarioDefinition> ScenarioDefinitions;
	TMap<FName, FTunaSweeperScenarioLocalizedText> ScenarioTextStringsByKey;
	bool bScenarioDataLoaded = false;
};
