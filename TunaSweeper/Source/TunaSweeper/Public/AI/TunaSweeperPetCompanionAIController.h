#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TunaSweeperPetCompanionAIController.generated.h"

class ATunaSweeperPetCompanionCharacter;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPetCompanionAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATunaSweeperPetCompanionAIController();

	void OnFollowTargetChanged();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float UpdateInterval = 0.2f;

private:
	void UpdateFollowBehavior();
	void ResetPetWalkSpeed();

	UPROPERTY(Transient)
	TObjectPtr<ATunaSweeperPetCompanionCharacter> PetCharacter;

	float TimeSinceLastUpdate = 0.0f;
	bool bIsFollowing = false;
};
