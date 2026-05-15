#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperQuestSubsystem.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperQuestState : uint8
{
	Available UMETA(DisplayName = "Available"),
	Accepted UMETA(DisplayName = "Accepted"),
	RewardAvailable UMETA(DisplayName = "Reward Available"),
	RewardCompleted UMETA(DisplayName = "Reward Completed")
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText ObjectiveText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (ClampMin = "0", UIMin = "0"))
	int32 CoinReward = 0;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperQuestSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	static FName GetFirstOutingQuestId();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool TryGetQuestDefinition(FName QuestId, FTunaSweeperQuestDefinition& OutDefinition) const;

	const FTunaSweeperQuestDefinition* FindQuestDefinition(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	ETunaSweeperQuestState GetQuestState(FName QuestId) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool CanAcceptQuest(FName QuestId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool AcceptQuest(FName QuestId);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	bool CanClaimQuestReward(FName QuestId) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool ClaimQuestReward(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void NotifyLevelTravelRequested(FName SourceLevelName, FName TargetLevelName);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	int32 GetCoinBalance() const { return CoinBalance; }

private:
	void RegisterFirstPassQuests();
	bool IsMapNameMatch(FName ActualMapName, const TCHAR* ExpectedMapName) const;
	void SetQuestState(FName QuestId, ETunaSweeperQuestState NewState);

	TMap<FName, FTunaSweeperQuestDefinition> QuestDefinitions;
	TMap<FName, ETunaSweeperQuestState> QuestStates;
	int32 CoinBalance = 0;
};
