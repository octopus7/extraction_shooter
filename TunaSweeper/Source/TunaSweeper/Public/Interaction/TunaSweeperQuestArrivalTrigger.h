#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperQuestArrivalTrigger.generated.h"

class UBoxComponent;
class ATunaSweeperTopDownCharacter;

UENUM(BlueprintType)
enum class ETunaSweeperArrivalVehicleCondition : uint8
{
	None,
	ATV
};

/** Reports a local player's presence at a destination to accepted quest objectives.
 * Progress belongs to the quest save, never to this actor's lifetime. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuestArrivalTrigger : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperQuestArrivalTrigger();
	virtual void Tick(float DeltaSeconds) override;

	// Also callable after scripted relocation; validates position and all conditions again.
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest Arrival")
	void CheckArrivals();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Quest Arrival")
	TObjectPtr<UBoxComponent> ArrivalArea;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest Arrival")
	FName LocationId;

	// None accepts any arrival; ATV requires the player to be riding an intact ATV.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest Arrival")
	ETunaSweeperArrivalVehicleCondition VehicleCondition = ETunaSweeperArrivalVehicleCondition::None;

	// Optional placed-vehicle identity. Empty accepts any intact ATV with this player in its seat.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest Arrival", meta = (EditCondition = "VehicleCondition == ETunaSweeperArrivalVehicleCondition::ATV"))
	FName RequiredVehicleTag;

	// Optional authoring gates. Every quest must have its reward claimed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest Arrival")
	TArray<int32> RequiredCompletedQuestIds;

	// Optional repair/refuel gates using existing Completed world-progress records.
	// These are not a live fuel gauge; future vehicle systems must maintain their own records.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Quest Arrival")
	TArray<FName> RequiredCompletedProgressIds;

private:
	bool CanReportArrival(const ATunaSweeperTopDownCharacter* Character) const;
};
