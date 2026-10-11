#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperQuestLightController.generated.h"

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperQuestLightGroup
{
	GENERATED_BODY()
	// Completion means RewardCompleted (reward claimed), not RewardAvailable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Lights", meta = (ClampMin = "1"))
	int32 CompletedQuestId = 0;
	// All light components owned by each actor. Use movable lights in the controller's level.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Quest Lights")
	TArray<TObjectPtr<AActor>> LightActors;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Lights")
	bool bVisibleBeforeCompletion = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest Lights")
	bool bVisibleAfterCompletion = true;
};

/** Event-driven, level-authored quest -> light lists. Later groups win duplicate references. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuestLightController : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperQuestLightController();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Quest Lights", meta = (TitleProperty = "CompletedQuestId"))
	TArray<FTunaSweeperQuestLightGroup> LightGroups;
	// Also call after changing mappings or adding light components dynamically.
	UFUNCTION(BlueprintCallable, Category = "Quest Lights")
	void RefreshQuestLights();
};
