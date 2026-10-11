#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "TunaSweeperQuestFacilityActor.generated.h"

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperFacilityQuestCondition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest", meta = (ClampMin = "1"))
	int32 QuestId = 0;
	// Exact states. Include Accepted and both reward states for an unlock that starts on acceptance.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<ETunaSweeperQuestState> AllowedStates = {ETunaSweeperQuestState::RewardCompleted};
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunaSweeperFacilityUsed, APawn*, InstigatorPawn);

/** Reusable quest-gated facility. A BP implements its actual service through ExecuteFacilityInteraction. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuestFacilityActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()
public:
	ATunaSweeperQuestFacilityActor();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

	// All entries must match. Empty or invalid conditions keep an unconfigured facility locked.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Facility", meta = (TitleProperty = "QuestId"))
	TArray<FTunaSweeperFacilityQuestCondition> UnlockConditions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Facility")
	FName InteractionStringKey = TEXT("ui.interaction.use_facility");
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Facility")
	FName InteractionObjectiveEventId;
	UPROPERTY(BlueprintAssignable, Category = "Quest Facility")
	FTunaSweeperFacilityUsed OnFacilityUsed;

	UFUNCTION(BlueprintPure, Category = "Quest Facility")
	bool IsFacilityUnlocked() const;
	UFUNCTION(BlueprintCallable, Category = "Quest Facility")
	void RefreshQuestState();
	// Called by the interaction subsystem; validates again at execution time.
	bool TryUseFacility(APawn* InstigatorPawn);

protected:
	// Return false if the service cannot open. Only success emits OnFacilityUsed and the quest event.
	UFUNCTION(BlueprintNativeEvent, Category = "Quest Facility")
	bool ExecuteFacilityInteraction(APawn* InstigatorPawn);
	virtual bool ExecuteFacilityInteraction_Implementation(APawn* InstigatorPawn);
private:
	bool bInteractionInProgress = false;
};
