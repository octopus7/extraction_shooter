#include "Subsystem/TunaSweeperQuestSubsystem.h"

namespace TunaSweeperQuestIds
{
	const FName FirstOuting(TEXT("quest_first_outing"));
}

void UTunaSweeperQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RegisterFirstPassQuests();
}

FName UTunaSweeperQuestSubsystem::GetFirstOutingQuestId()
{
	return TunaSweeperQuestIds::FirstOuting;
}

bool UTunaSweeperQuestSubsystem::TryGetQuestDefinition(
	FName QuestId,
	FTunaSweeperQuestDefinition& OutDefinition) const
{
	if (const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId))
	{
		OutDefinition = *Definition;
		return true;
	}

	return false;
}

const FTunaSweeperQuestDefinition* UTunaSweeperQuestSubsystem::FindQuestDefinition(FName QuestId) const
{
	return QuestDefinitions.Find(QuestId);
}

ETunaSweeperQuestState UTunaSweeperQuestSubsystem::GetQuestState(FName QuestId) const
{
	if (const ETunaSweeperQuestState* State = QuestStates.Find(QuestId))
	{
		return *State;
	}

	return ETunaSweeperQuestState::Available;
}

bool UTunaSweeperQuestSubsystem::CanAcceptQuest(FName QuestId) const
{
	return FindQuestDefinition(QuestId) && GetQuestState(QuestId) == ETunaSweeperQuestState::Available;
}

bool UTunaSweeperQuestSubsystem::AcceptQuest(FName QuestId)
{
	if (!CanAcceptQuest(QuestId))
	{
		return false;
	}

	SetQuestState(QuestId, ETunaSweeperQuestState::Accepted);
	return true;
}

bool UTunaSweeperQuestSubsystem::CanClaimQuestReward(FName QuestId) const
{
	return FindQuestDefinition(QuestId) && GetQuestState(QuestId) == ETunaSweeperQuestState::RewardAvailable;
}

bool UTunaSweeperQuestSubsystem::ClaimQuestReward(FName QuestId)
{
	if (!CanClaimQuestReward(QuestId))
	{
		return false;
	}

	if (const FTunaSweeperQuestDefinition* Definition = FindQuestDefinition(QuestId))
	{
		CoinBalance += FMath::Max(0, Definition->CoinReward);
	}

	SetQuestState(QuestId, ETunaSweeperQuestState::RewardCompleted);
	return true;
}

void UTunaSweeperQuestSubsystem::NotifyLevelTravelRequested(FName SourceLevelName, FName TargetLevelName)
{
	if (!IsMapNameMatch(SourceLevelName, TEXT("BunkerMap")) || !IsMapNameMatch(TargetLevelName, TEXT("RaidMap")))
	{
		return;
	}

	const FName QuestId = GetFirstOutingQuestId();
	if (GetQuestState(QuestId) == ETunaSweeperQuestState::Accepted)
	{
		SetQuestState(QuestId, ETunaSweeperQuestState::RewardAvailable);
	}
}

void UTunaSweeperQuestSubsystem::RegisterFirstPassQuests()
{
	FTunaSweeperQuestDefinition FirstOuting;
	FirstOuting.QuestId = GetFirstOutingQuestId();
	FirstOuting.Title = FText::FromString(TEXT("\uCCAB \uC678\uCD9C"));
	FirstOuting.Description = FText::FromString(TEXT("\uC774\uC81C \uB4E4\uC5B4\uC654\uC73C\uB2C8 \uB098\uAC00\uC11C \uD55C\uBC88 \uC0B0\uCC45\uD558\uACE0 \uB4E4\uC5B4\uC640"));
	FirstOuting.ObjectiveText = FText::FromString(TEXT("\uBC99\uCEE4 \uBC16\uC73C\uB85C \uC774\uB3D9"));
	FirstOuting.CoinReward = 100;

	QuestDefinitions.Add(FirstOuting.QuestId, FirstOuting);

	if (!QuestStates.Contains(FirstOuting.QuestId))
	{
		QuestStates.Add(FirstOuting.QuestId, ETunaSweeperQuestState::Available);
	}
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

void UTunaSweeperQuestSubsystem::SetQuestState(FName QuestId, ETunaSweeperQuestState NewState)
{
	if (FindQuestDefinition(QuestId))
	{
		QuestStates.FindOrAdd(QuestId) = NewState;
	}
}
