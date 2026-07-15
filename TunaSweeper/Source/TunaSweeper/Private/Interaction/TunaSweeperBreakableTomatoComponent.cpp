#include "Interaction/TunaSweeperBreakableTomatoComponent.h"

UTunaSweeperBreakableTomatoComponent::UTunaSweeperBreakableTomatoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTunaSweeperBreakableTomatoComponent::BeginPlay()
{
	Super::BeginPlay();
	ResetTomatoHealth();
}

bool UTunaSweeperBreakableTomatoComponent::ApplyTomatoDamage(float DamageAmount)
{
	if (bBroken || DamageAmount <= 0.0f)
	{
		return false;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	bBroken = CurrentHealth <= 0.0f;
	return bBroken;
}

void UTunaSweeperBreakableTomatoComponent::ResetTomatoHealth()
{
	MaxHealth = FMath::Max(0.01f, MaxHealth);
	CurrentHealth = MaxHealth;
	bBroken = false;
}

void UTunaSweeperBreakableTomatoComponent::SetMaxHealth(float InMaxHealth)
{
	MaxHealth = FMath::Max(0.01f, InMaxHealth);
	ResetTomatoHealth();
}
