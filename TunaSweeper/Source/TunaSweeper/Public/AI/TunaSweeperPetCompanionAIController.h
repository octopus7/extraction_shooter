#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "GenericTeamAgentInterface.h"
#include "TunaSweeperPetCompanionAIController.generated.h"

class ATunaSweeperPetCompanionCharacter;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPetCompanionAIController : public AAIController
{
	GENERATED_BODY()

public:
	ATunaSweeperPetCompanionAIController();

	void OnFollowTargetChanged();
	virtual void SetGenericTeamId(const FGenericTeamId& InTeamId) override;
	virtual FGenericTeamId GetGenericTeamId() const override;
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;

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
