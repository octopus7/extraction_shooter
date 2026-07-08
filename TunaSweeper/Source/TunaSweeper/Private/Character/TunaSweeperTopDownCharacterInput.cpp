#include "TunaSweeperTopDownCharacterShared.h"

void ATunaSweeperTopDownCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (UInputAction* LoadedMoveAction = MoveAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedMoveAction, ETriggerEvent::Triggered, this, &ATunaSweeperTopDownCharacter::HandleMove);
		EnhancedInputComponent->BindAction(LoadedMoveAction, ETriggerEvent::Completed, this, &ATunaSweeperTopDownCharacter::HandleMoveStopped);
		EnhancedInputComponent->BindAction(LoadedMoveAction, ETriggerEvent::Canceled, this, &ATunaSweeperTopDownCharacter::HandleMoveStopped);
	}

	if (UInputAction* LoadedFireAction = FireAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedFireAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::BeginFire);
		EnhancedInputComponent->BindAction(LoadedFireAction, ETriggerEvent::Completed, this, &ATunaSweeperTopDownCharacter::EndFire);
		EnhancedInputComponent->BindAction(LoadedFireAction, ETriggerEvent::Canceled, this, &ATunaSweeperTopDownCharacter::EndFire);
	}

	if (UInputAction* LoadedAimAction = AimAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedAimAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::BeginAim);
		EnhancedInputComponent->BindAction(LoadedAimAction, ETriggerEvent::Completed, this, &ATunaSweeperTopDownCharacter::EndAim);
		EnhancedInputComponent->BindAction(LoadedAimAction, ETriggerEvent::Canceled, this, &ATunaSweeperTopDownCharacter::EndAim);
	}

	if (UInputAction* LoadedInteractAction = InteractAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedInteractAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleInteract);
	}

	if (UInputAction* LoadedInteractionFocusAction = InteractionFocusAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedInteractionFocusAction, ETriggerEvent::Triggered, this, &ATunaSweeperTopDownCharacter::HandleInteractionFocus);
	}

	if (UInputAction* LoadedInventoryAction = InventoryAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedInventoryAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleInventory);
	}

	if (UInputAction* LoadedMapAction = MapAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedMapAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleMap);
	}

	if (UInputAction* LoadedReloadAction = ReloadAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedReloadAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleReload);
	}

	if (UInputAction* LoadedAmmoSelectAction = AmmoSelectAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedAmmoSelectAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleAmmoSelect);
	}

	if (UInputAction* LoadedAmmoFocusAction = AmmoFocusAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedAmmoFocusAction, ETriggerEvent::Triggered, this, &ATunaSweeperTopDownCharacter::HandleAmmoFocus);
	}

	if (UInputAction* LoadedCameraModeAction = CameraModeAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedCameraModeAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleCameraMode);
	}

	if (UInputAction* LoadedSprintAction = SprintAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedSprintAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::BeginSprint);
		EnhancedInputComponent->BindAction(LoadedSprintAction, ETriggerEvent::Completed, this, &ATunaSweeperTopDownCharacter::EndSprint);
		EnhancedInputComponent->BindAction(LoadedSprintAction, ETriggerEvent::Canceled, this, &ATunaSweeperTopDownCharacter::EndSprint);
	}

	if (UInputAction* LoadedRollAction = RollAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedRollAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::BeginRoll);
	}
}

void ATunaSweeperTopDownCharacter::AddDefaultInputMapping() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	UInputMappingContext* LoadedMappingContext = DefaultMappingContext.LoadSynchronous();
	if (InputSubsystem && LoadedMappingContext)
	{
		InputSubsystem->RemoveMappingContext(LoadedMappingContext);
		InputSubsystem->AddMappingContext(LoadedMappingContext, 0);
	}
}

void ATunaSweeperTopDownCharacter::HandleMove(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		CurrentMoveInput = FVector2D::ZeroVector;
		return;
	}

	const FVector2D MoveVector = Value.Get<FVector2D>();
	CurrentMoveInput = MoveVector.GetClampedToMaxSize(1.0f);
	if (bIsRolling || IsCarryWeightMovementBlocked())
	{
		return;
	}

	if (!FMath::IsNearlyZero(MoveVector.Y))
	{
		AddMovementInput(FVector::ForwardVector, MoveVector.Y);
	}

	if (!FMath::IsNearlyZero(MoveVector.X))
	{
		AddMovementInput(FVector::RightVector, MoveVector.X);
	}
}

void ATunaSweeperTopDownCharacter::HandleMoveStopped(const FInputActionValue& Value)
{
	(void)Value;
	CurrentMoveInput = FVector2D::ZeroVector;
}

void ATunaSweeperTopDownCharacter::BeginFire(const FInputActionValue& Value)
{
	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController()))
	{
		if (TunaPlayerController->IsHousingPlacementActive())
		{
			TunaPlayerController->TryCommitHousingPlacement();
			return;
		}
	}

	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	bFireHeld = true;
	if (bIsUsingItem)
	{
		CancelItemUse();
	}

	FireWeapon();

	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ATunaSweeperTopDownCharacter::FireWeapon, FireInterval, true, FireInterval);
	}
}

void ATunaSweeperTopDownCharacter::EndFire(const FInputActionValue& Value)
{
	bFireHeld = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void ATunaSweeperTopDownCharacter::BeginAim(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	bIsAiming = true;
}

void ATunaSweeperTopDownCharacter::EndAim(const FInputActionValue& Value)
{
	bIsAiming = false;
}

void ATunaSweeperTopDownCharacter::HandleInteract(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController()))
	{
		if (TunaPlayerController->IsHousingModeOpen())
		{
			return;
		}

		if (TunaPlayerController->TryHandleHoveredItemInteract())
		{
			return;
		}

		if (TunaPlayerController->IsInventoryUiOpen())
		{
			return;
		}
	}

	if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
	{
		InteractionSubsystem->TryInteract(this);
	}
}

void ATunaSweeperTopDownCharacter::HandleInteractionFocus(const FInputActionValue& Value)
{
	if (bIsDead || bAmmoSelectionOpen)
	{
		return;
	}

	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController()))
	{
		if (TunaPlayerController->IsHousingModeOpen() || TunaPlayerController->IsInventoryUiOpen())
		{
			return;
		}
	}

	const float AxisValue = Value.Get<float>();
	if (FMath::Abs(AxisValue) <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
	{
		InteractionSubsystem->MoveFocusedInteractionSelection(AxisValue > 0.0f ? -1 : 1, this);
	}
}

void ATunaSweeperTopDownCharacter::HandleInventory(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController()))
	{
		if (TunaPlayerController->IsDialogueSequenceActive())
		{
			return;
		}

		CancelItemUse();
		TunaPlayerController->ToggleInventoryOnlyPanel();
	}
}

void ATunaSweeperTopDownCharacter::HandleMap(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController()))
	{
		if (TunaPlayerController->IsDialogueSequenceActive())
		{
			return;
		}

		if (TunaPlayerController->IsHousingModeOpen())
		{
			return;
		}

		CancelItemUse();
		TunaPlayerController->ToggleMapPanel();
	}
}

void ATunaSweeperTopDownCharacter::HandleReload(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	StartReload();
}

void ATunaSweeperTopDownCharacter::HandleAmmoSelect(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	if (bIsUsingItem)
	{
		CancelItemUse();
	}

	if (bAmmoSelectionOpen)
	{
		ConfirmAmmoSelection();
	}
	else
	{
		OpenAmmoSelection();
	}
}

void ATunaSweeperTopDownCharacter::HandleAmmoFocus(const FInputActionValue& Value)
{
	if (!bAmmoSelectionOpen || AmmoSelectionItemIds.Num() <= 0)
	{
		return;
	}

	const float AxisValue = Value.Get<float>();
	if (FMath::Abs(AxisValue) <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	MoveAmmoSelectionFocus(AxisValue > 0.0f ? -1 : 1);
}

void ATunaSweeperTopDownCharacter::HandleCameraMode(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	CyclePlayerCameraMode();
}

void ATunaSweeperTopDownCharacter::BeginSprint(const FInputActionValue& Value)
{
	(void)Value;
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	bSprintInputHeld = true;
}

void ATunaSweeperTopDownCharacter::EndSprint(const FInputActionValue& Value)
{
	(void)Value;
	bSprintInputHeld = false;
	bIsSprinting = false;
	bSprintLockedUntilReleased = false;
}

void ATunaSweeperTopDownCharacter::BeginRoll(const FInputActionValue& Value)
{
	(void)Value;
	if (bIsDead || bIsRolling || IsGameplayActionInputLocked())
	{
		return;
	}

	RollDirection = ResolveRollDirection();
	if (RollDirection.IsNearlyZero())
	{
		return;
	}

	const float EffectiveRollStaminaCost = FMath::Max(0.0f, RollStaminaCost);
	CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, FMath::Max(0.0f, MaxStamina));
	if (CurrentStamina < EffectiveRollStaminaCost)
	{
		return;
	}

	bIsRolling = true;
	bIsSprinting = false;
	bSprintInputHeld = false;
	bSprintLockedUntilReleased = false;
	bFireHeld = false;
	bIsAiming = false;
	RollElapsedSeconds = 0.0f;
	CurrentStamina = FMath::Max(0.0f, CurrentStamina - EffectiveRollStaminaCost);
	DefaultSkeletalMeshRelativeRotation = GetMesh() ? GetMesh()->GetRelativeRotation() : DefaultSkeletalMeshRelativeRotation;
	DefaultVisualMeshRelativeRotation = VisualMesh ? VisualMesh->GetRelativeRotation() : DefaultVisualMeshRelativeRotation;

	CancelReload();
	CancelItemUse();
	CloseAmmoSelection();
	CancelMeleeSwing();
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
	ConsumeMovementInputVector();

	SetActorRotation(FRotator(0.0f, RollDirection.Rotation().Yaw, 0.0f));
	SetRollProjectileCollisionPassthrough(true);
	AttachWeaponForRoll();
	UpdateMovementSpeed();
	ApplyTemporaryRollVisualRotation(0.0f);
}

