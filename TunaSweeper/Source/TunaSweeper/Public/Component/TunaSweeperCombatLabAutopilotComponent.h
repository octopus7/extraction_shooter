#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TunaSweeperCombatLabAutopilotComponent.generated.h"
class ATunaSweeperEnemyCharacter;
class ATunaSweeperTopDownCharacter;

/** Temporary lab-only pilot: feeds real player actions, never applies synthetic damage. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperCombatLabAutopilotComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UTunaSweeperCombatLabAutopilotComponent();
	void SetAutopilotEnabled(bool bEnabled);
	bool IsAutopilotEnabled() const { return bEnabled; }
	FName GetStateKey() const { return StateKey; }
	int32 GetRollCount() const { return RollCount; }
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
	bool CanSee(const FVector& From, ATunaSweeperEnemyCharacter* Enemy) const;
	FVector Navigate(const FVector& Goal);
	void StopActions();
	TWeakObjectPtr<ATunaSweeperEnemyCharacter> Target;
	TArray<FVector> Path;
	FVector LastGoal = FVector::ZeroVector;
	float NextDecision = 0, NextPath = 0, NextRoll = 0, NextBurst = 0;
	float StrafeSign = 1;
	int32 PathIndex = 0, RollCount = 0;
	bool bEnabled = false, bFiring = false;
	FName StateKey = TEXT("ui.combat_lab.manual");
};
