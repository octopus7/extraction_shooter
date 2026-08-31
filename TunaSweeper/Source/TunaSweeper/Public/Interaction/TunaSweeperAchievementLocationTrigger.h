#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperAchievementLocationTrigger.generated.h"

class UBoxComponent;

UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperAchievementLocationTrigger : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperAchievementLocationTrigger();

protected:
	UFUNCTION()
	void HandleTriggerBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Achievement")
	TObjectPtr<UBoxComponent> TriggerVolume;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Achievement")
	FName LocationId = NAME_None;

private:
	bool bTriggeredThisInstance = false;
};
