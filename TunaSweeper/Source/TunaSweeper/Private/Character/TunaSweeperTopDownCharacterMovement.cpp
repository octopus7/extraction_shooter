#include "TunaSweeperTopDownCharacterShared.h"

void ATunaSweeperTopDownCharacter::UpdateRoll(float DeltaSeconds)
{
	if (!bIsRolling)
	{
		return;
	}

	const float EffectiveRollDuration = FMath::Max(0.01f, RollDurationSeconds);
	RollElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);

	AddMovementInput(RollDirection, 1.0f);
	ApplyTemporaryRollVisualRotation(FMath::Clamp(RollElapsedSeconds / EffectiveRollDuration, 0.0f, 1.0f));

	if (RollElapsedSeconds >= EffectiveRollDuration)
	{
		FinishRoll();
	}
}

void ATunaSweeperTopDownCharacter::UpdateSprintAndStamina(float DeltaSeconds)
{
	const float ClampedDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	const float EffectiveMaxStamina = FMath::Max(0.0f, MaxStamina);
	if (EffectiveMaxStamina <= 0.0f)
	{
		CurrentStamina = 0.0f;
		bIsSprinting = false;
		return;
	}

	CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, EffectiveMaxStamina);
	if (bIsRolling)
	{
		bIsSprinting = false;
		return;
	}

	const bool bCanSprint =
		bSprintInputHeld &&
		!bSprintLockedUntilReleased &&
		HasActiveMoveInput() &&
		!IsCarryWeightMovementBlocked() &&
		!IsGameplayActionInputLocked();
	bIsSprinting = bCanSprint && CurrentStamina > 0.0f;

	if (bIsSprinting)
	{
		CurrentStamina = FMath::Max(0.0f, CurrentStamina - FMath::Max(0.0f, SprintStaminaDrainPerSecond) * ClampedDeltaSeconds);
		if (CurrentStamina <= KINDA_SMALL_NUMBER)
		{
			CurrentStamina = 0.0f;
			bIsSprinting = false;
			bSprintLockedUntilReleased = true;
		}
	}
	else
	{
		CurrentStamina = FMath::Min(EffectiveMaxStamina, CurrentStamina + FMath::Max(0.0f, StaminaRegenPerSecond) * ClampedDeltaSeconds);
	}
}

void ATunaSweeperTopDownCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const float CarryWeightSpeedMultiplier = (bIsRolling || !TunaGameInstance)
		? 1.0f
		: TunaGameInstance->GetCarryWeightMovementSpeedMultiplier();
	const float ActionSpeedMultiplier = bIsRolling
		? FMath::Max(0.0f, RollDistance) / FMath::Max(0.01f, RollDurationSeconds) / FMath::Max(1.0f, BaseWalkSpeed)
		: (bIsSprinting ? FMath::Max(1.0f, SprintSpeedMultiplier) : 1.0f);

	MovementComponent->MaxWalkSpeed =
		BaseWalkSpeed *
		FMath::Clamp(CarryWeightSpeedMultiplier, 0.0f, 1.0f) *
		ActionSpeedMultiplier;
}

void ATunaSweeperTopDownCharacter::UpdateStaminaGauge(float DeltaSeconds)
{
	if (!StaminaGaugeWidgetComponent)
	{
		return;
	}

	if (bHousingModeVisualHidden)
	{
		StaminaGaugeWidgetComponent->SetHiddenInGame(true);
		StaminaGaugeWidgetComponent->SetVisibility(false);
		return;
	}

	StaminaGaugeWidgetComponent->SetHiddenInGame(false);
	const float TargetOpacity = GetStaminaPercent() < 0.999f ? 1.0f : 0.0f;
	if (DeltaSeconds <= 0.0f)
	{
		StaminaGaugeOpacity = TargetOpacity;
	}
	else
	{
		StaminaGaugeOpacity = FMath::FInterpTo(
			StaminaGaugeOpacity,
			TargetOpacity,
			DeltaSeconds,
			FMath::Max(0.0f, StaminaGaugeFadeInterpSpeed));
	}

	if (FMath::IsNearlyEqual(StaminaGaugeOpacity, TargetOpacity, 0.01f))
	{
		StaminaGaugeOpacity = TargetOpacity;
	}

	const bool bVisible = StaminaGaugeOpacity > 0.01f;
	StaminaGaugeWidgetComponent->SetVisibility(bVisible);
	if (!bVisible)
	{
		return;
	}

	StaminaGaugeWidgetComponent->InitWidget();
	if (UTunaSweeperStaminaGaugeWidget* StaminaGaugeWidget = Cast<UTunaSweeperStaminaGaugeWidget>(StaminaGaugeWidgetComponent->GetUserWidgetObject()))
	{
		StaminaGaugeWidget->SetStaminaGauge(GetStaminaPercent(), StaminaGaugeOpacity);
	}
}

bool ATunaSweeperTopDownCharacter::HasActiveMoveInput() const
{
	return CurrentMoveInput.SizeSquared() > KINDA_SMALL_NUMBER;
}

void ATunaSweeperTopDownCharacter::FinishRoll()
{
	if (!bIsRolling && !bHasSavedProjectileCollisionResponse && !bWeaponAttachedForRoll)
	{
		return;
	}

	bIsRolling = false;
	RollElapsedSeconds = 0.0f;
	SetRollProjectileCollisionPassthrough(false);
	RestoreTemporaryRollVisualRotation();
	RestoreWeaponAfterRoll();
	UpdateMovementSpeed();
}

void ATunaSweeperTopDownCharacter::SetRollProjectileCollisionPassthrough(bool bEnabled)
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}

	if (bEnabled)
	{
		if (!bHasSavedProjectileCollisionResponse)
		{
			SavedProjectileCollisionResponse = Capsule->GetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile);
			bHasSavedProjectileCollisionResponse = true;
		}

		Capsule->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Ignore);
		return;
	}

	if (bHasSavedProjectileCollisionResponse)
	{
		Capsule->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, SavedProjectileCollisionResponse);
		bHasSavedProjectileCollisionResponse = false;
	}
}

void ATunaSweeperTopDownCharacter::AttachWeaponForRoll()
{
	if (!EquippedWeapon || bWeaponAttachedForRoll)
	{
		return;
	}

	USceneComponent* WeaponRoot = EquippedWeapon->GetRootComponent();
	if (!WeaponRoot)
	{
		return;
	}

	FName RollSocketName = NAME_None;
	USceneComponent* RollAttachParent = ResolveRollWeaponAttachParent(RollSocketName);
	if (!RollAttachParent)
	{
		return;
	}

	SavedWeaponAttachParent = WeaponRoot->GetAttachParent();
	SavedWeaponAttachSocketName = WeaponRoot->GetAttachSocketName();
	SavedWeaponRelativeTransform = WeaponRoot->GetRelativeTransform();
	bWeaponAttachedForRoll = true;

	EquippedWeapon->AttachToComponent(
		RollAttachParent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		RollSocketName);
}

void ATunaSweeperTopDownCharacter::RestoreWeaponAfterRoll()
{
	if (!bWeaponAttachedForRoll)
	{
		return;
	}

	USceneComponent* WeaponRoot = EquippedWeapon ? EquippedWeapon->GetRootComponent() : nullptr;
	USceneComponent* RestoreParent = SavedWeaponAttachParent.Get();
	if (WeaponRoot)
	{
		if (!RestoreParent)
		{
			RestoreParent = WeaponAttachPoint;
		}

		if (RestoreParent)
		{
			EquippedWeapon->AttachToComponent(
				RestoreParent,
				FAttachmentTransformRules::KeepRelativeTransform,
				SavedWeaponAttachSocketName);
			WeaponRoot->SetRelativeTransform(SavedWeaponRelativeTransform);
		}
		else
		{
			EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	bWeaponAttachedForRoll = false;
	SavedWeaponAttachParent.Reset();
	SavedWeaponAttachSocketName = NAME_None;
	SavedWeaponRelativeTransform = FTransform::Identity;
}

USceneComponent* ATunaSweeperTopDownCharacter::ResolveRollWeaponAttachParent(FName& OutSocketName) const
{
	OutSocketName = NAME_None;

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (!RollWeaponHandSocketName.IsNone() && CharacterMesh->DoesSocketExist(RollWeaponHandSocketName))
		{
			OutSocketName = RollWeaponHandSocketName;
			return CharacterMesh;
		}
	}

	if (RollWeaponHandAttachPoint)
	{
		return RollWeaponHandAttachPoint;
	}

	return GetMesh();
}

void ATunaSweeperTopDownCharacter::ApplyTemporaryRollVisualRotation(float NormalizedRollTime)
{
	if (!bUseTemporaryRollVisualRotation)
	{
		return;
	}

	bRollVisualRotationApplied = true;
	const float RollAngleRadians = FMath::DegreesToRadians(
		TemporaryRollVisualRightAxisDegrees * FMath::Clamp(NormalizedRollTime, 0.0f, 1.0f));
	const FQuat RightAxisRoll(FVector::RightVector, RollAngleRadians);
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetRelativeRotation((RightAxisRoll * DefaultSkeletalMeshRelativeRotation.Quaternion()).Rotator());
	}

	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation((RightAxisRoll * DefaultVisualMeshRelativeRotation.Quaternion()).Rotator());
	}
}

void ATunaSweeperTopDownCharacter::RestoreTemporaryRollVisualRotation()
{
	if (!bRollVisualRotationApplied)
	{
		return;
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetRelativeRotation(DefaultSkeletalMeshRelativeRotation);
	}

	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation(DefaultVisualMeshRelativeRotation);
	}

	bRollVisualRotationApplied = false;
}

FVector ATunaSweeperTopDownCharacter::ResolveRollDirection() const
{
	FVector ResolvedDirection(CurrentMoveInput.Y, CurrentMoveInput.X, 0.0f);
	if (!ResolvedDirection.Normalize())
	{
		ResolvedDirection = AimDirection;
		ResolvedDirection.Z = 0.0f;
		ResolvedDirection.Normalize();
	}

	if (ResolvedDirection.IsNearlyZero())
	{
		ResolvedDirection = GetActorForwardVector();
		ResolvedDirection.Z = 0.0f;
		ResolvedDirection.Normalize();
	}

	return ResolvedDirection.IsNearlyZero() ? FVector::ForwardVector : ResolvedDirection;
}
