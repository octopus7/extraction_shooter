#include "TunaSweeperTopDownCharacterShared.h"

float ATunaSweeperTopDownCharacter::GetStaminaPercent() const
{
	return MaxStamina > 0.0f
		? FMath::Clamp(CurrentStamina / MaxStamina, 0.0f, 1.0f)
		: 0.0f;
}

void ATunaSweeperTopDownCharacter::SetHousingModeVisualHidden(bool bShouldHide)
{
	if (bHousingModeVisualHidden == bShouldHide)
	{
		RefreshCharacterVisualVisibility();
		return;
	}

	bHousingModeVisualHidden = bShouldHide;
	RefreshCharacterVisualVisibility();
}

void ATunaSweeperTopDownCharacter::RefreshCarryWeightConditionDebuffs()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	TunaGameInstance->RefreshCarryWeightState();
	const bool bMovementBlocked = TunaGameInstance->IsCarryWeightMovementBlocked();
	const bool bOverweight = TunaGameInstance->IsCarryWeightOverLimit();
	if (DebuffComponent)
	{
		DebuffComponent->SetConditionalDebuffActive(
			TunaSweeperDebuff::MovementBlockedDebuffId(),
			bMovementBlocked,
			this);
		DebuffComponent->SetConditionalDebuffActive(
			TunaSweeperDebuff::OverweightDebuffId(),
			bOverweight && !bMovementBlocked,
			this);
	}

	if (!bOverweight)
	{
		bSprintLockedUntilReleased = false;
	}

	UpdateMovementSpeed();
}

void ATunaSweeperTopDownCharacter::RefreshCharacterVisualVisibility()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (CharacterMesh)
	{
		CharacterMesh->SetHiddenInGame(bHousingModeVisualHidden);
		CharacterMesh->SetVisibility(!bHousingModeVisualHidden, true);
	}

	const bool bHasCharacterMesh = CharacterMesh && CharacterMesh->GetSkeletalMeshAsset();
	if (VisualMesh)
	{
		VisualMesh->SetHiddenInGame(bHousingModeVisualHidden || bHasCharacterMesh);
		VisualMesh->SetVisibility(!bHousingModeVisualHidden && !bHasCharacterMesh, true);
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(bHousingModeVisualHidden);
		ApplyEquippedWeaponAttachmentVisuals();
	}

	if (StaminaGaugeWidgetComponent)
	{
		StaminaGaugeWidgetComponent->SetHiddenInGame(bHousingModeVisualHidden);
		if (bHousingModeVisualHidden)
		{
			StaminaGaugeWidgetComponent->SetVisibility(false);
		}
	}
}

void ATunaSweeperTopDownCharacter::CacheBaseSurvivalStats()
{
	if (bBaseSurvivalStatsCached)
	{
		return;
	}

	if (VitalsComponent)
	{
		const FTunaSweeperVitalsState& VitalsState = VitalsComponent->GetVitalsState();
		BaseMaxHealth = FMath::Max(1.0f, VitalsState.MaxHealth);
		BaseMaxFood = FMath::Max(1.0f, VitalsState.MaxFood);
		BaseMaxHydration = FMath::Max(1.0f, VitalsState.MaxHydration);
	}

	BaseMaxStamina = FMath::Max(1.0f, MaxStamina);
	bBaseSurvivalStatsCached = true;
}

void ATunaSweeperTopDownCharacter::ApplyExperienceLevelStatBonuses()
{
	CacheBaseSurvivalStats();

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const FTunaSweeperExperienceLevelStatBonuses Bonuses = TunaGameInstance
		? TunaGameInstance->GetCurrentExperienceLevelStatBonuses()
		: FTunaSweeperExperienceLevelStatBonuses();

	if (VitalsComponent)
	{
		VitalsComponent->SetMaxVitals(
			BaseMaxHealth + Bonuses.MaxHealthBonus,
			BaseMaxFood + Bonuses.MaxFoodBonus,
			BaseMaxHydration + Bonuses.MaxHydrationBonus,
			true);
	}

	const float OldMaxStamina = FMath::Max(1.0f, MaxStamina);
	const float StaminaRatio = FMath::Clamp(CurrentStamina / OldMaxStamina, 0.0f, 1.0f);
	MaxStamina = FMath::Max(1.0f, BaseMaxStamina + Bonuses.MaxStaminaBonus);
	CurrentStamina = FMath::Clamp(MaxStamina * StaminaRatio, 0.0f, MaxStamina);
	RefreshCarryWeightConditionDebuffs();
}

void ATunaSweeperTopDownCharacter::ApplyBunkerPeaceZoneVitalsRules()
{
	const UWorld* World = GetWorld();
	if (!World || !World->GetMapName().EndsWith(TEXT("BunkerMap")) || !VitalsComponent)
	{
		return;
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->ConsumePendingBunkerEntryVitals(VitalsComponent);
	}

	FTunaSweeperVitalsDepletionMultipliers PeaceZoneMultipliers;
	PeaceZoneMultipliers.Health = 1.0f;
	PeaceZoneMultipliers.Food = 0.0f;
	PeaceZoneMultipliers.Hydration = 0.0f;
	VitalsComponent->SetDepletionRateMultipliers(PeaceZoneMultipliers);
}

