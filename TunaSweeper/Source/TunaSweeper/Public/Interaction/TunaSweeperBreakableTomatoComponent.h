#pragma once

#include "Components/ActorComponent.h"
#include "TunaSweeperBreakableTomatoComponent.generated.h"

/** Owns a tomato's damage state so presentation and movement remain independent of destruction. */
UCLASS(ClassGroup = (TunaSweeper), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperBreakableTomatoComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperBreakableTomatoComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Breakable Tomato")
	bool ApplyTomatoDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Breakable Tomato")
	void ResetTomatoHealth();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Breakable Tomato")
	void SetMaxHealth(float InMaxHealth);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Breakable Tomato")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Breakable Tomato")
	bool IsBroken() const { return bBroken; }

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tomato|Break", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float MaxHealth = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tomato|Break")
	float CurrentHealth = 1.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Tomato|Break")
	bool bBroken = false;
};
