#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperQuestRewardLockerActor.generated.h"

/** Receive-only access point for quest rewards that did not fit in the inventory. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuestRewardLockerActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()
public:
	ATunaSweeperQuestRewardLockerActor();
	virtual void OnConstruction(const FTransform& Transform) override;

	/** Allows the outlet to follow a curved wall independently of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item Collection Terminal")
	FTransform OutletTransform = FTransform(FVector(0, 145, 0));

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> OutletRoot;

};
