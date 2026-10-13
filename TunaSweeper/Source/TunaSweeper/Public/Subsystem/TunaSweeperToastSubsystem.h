#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TunaSweeperToastSubsystem.generated.h"

class UTunaSweeperToastWidget;

struct FTunaSweeperToastRequest
{
	FText Message;
	float DurationSeconds = 2.0f;
};

struct FTunaSweeperItemRewardToastRequest
{
	int32 ItemId = 0;
	int32 Quantity = 0;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperToastSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	bool ShowItemRewardToast(int32 ItemId, int32 Quantity);
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|UI|Toast")
	bool ShowToast(const FText& MessageText, float DurationSeconds = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|UI|Toast")
	bool ShowLocalizedToast(FName StringKey, const FText& FallbackText, float DurationSeconds = 2.0f);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|UI|Toast")
	bool ShowSaveSlotDeletedToast();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|UI|Toast")
	bool ShowQuestCompletedToast(const FText& QuestTitle);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|UI|Toast")
	bool ShowQuestAcceptedToast(const FText& QuestTitle);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|UI|Toast")
	void ClearToasts();

private:
	friend class FTunaSweeperFeedingRewardsTest;
	bool StartNextRewardToast();
	void TickRewardToasts(float DeltaTime);
	void ClearRewardToasts();
	void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
	UPROPERTY(Transient) TObjectPtr<UTunaSweeperToastWidget> RewardToastWidget;
	TArray<FTunaSweeperItemRewardToastRequest> RewardQueue;
	FTunaSweeperItemRewardToastRequest ActiveReward;
	float RewardElapsed = 0.0f;
	bool bRewardActive = false;
	FDelegateHandle WorldCleanupHandle;
	bool EnsureToastWidget();
	bool StartNextToast();
	void FinishActiveToast();
	void UpdateActiveToastOpacity();
	FText ResolveToastText(FName StringKey, const FText& FallbackText) const;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperToastWidget> ActiveToastWidget;

	TArray<FTunaSweeperToastRequest> ToastQueue;

	float ActiveToastElapsedSeconds = 0.0f;
	float ActiveToastDurationSeconds = 0.0f;
	bool bToastActive = false;

	static constexpr int32 ToastViewportZOrder = 950;
	static constexpr float MinimumToastDurationSeconds = 0.25f;
	static constexpr float ToastFadeInSeconds = 0.12f;
	static constexpr float ToastFadeOutSeconds = 0.25f;
};
