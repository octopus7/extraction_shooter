#include "TunaSweeperTopDownCharacterShared.h"

float ATunaSweeperTopDownCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f || !VitalsComponent)
	{
		return 0.0f;
	}

	if (IsDamageInvulnerable())
	{
		return 0.0f;
	}

	LastDamageImpulseDirection = ResolveDamageCameraReactionDirection(DamageCauser);

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const int32 DefenseValue = TunaGameInstance ? TunaGameInstance->GetEquippedDefenseValue() : 0;
	const float AppliedDamage = FMath::Max(0.0f, DamageAmount - static_cast<float>(DefenseValue));
	if (AppliedDamage <= 0.0f)
	{
		return 0.0f;
	}

	FTunaSweeperVitalsDelta DamageDelta;
	DamageDelta.Health = -AppliedDamage;
	VitalsComponent->ApplyVitalsDelta(DamageDelta);
	TriggerDamageCameraReaction(AppliedDamage, DamageEvent, DamageCauser);
	return AppliedDamage;
}

void ATunaSweeperTopDownCharacter::HandleVitalsChanged(const FTunaSweeperVitalsState& VitalsState)
{
	if (!bIsDead && VitalsState.Health <= 0.0f)
	{
		HandleDeath();
	}
}

void ATunaSweeperTopDownCharacter::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	FinishRoll();
	bFireHeld = false;
	bIsAiming = false;
	bSprintInputHeld = false;
	bIsSprinting = false;
	bSprintLockedUntilReleased = false;
	CurrentMoveInput = FVector2D::ZeroVector;
	CancelReload();
	CancelItemUse();
	CloseAmmoSelection();
	ClearEquippedWeaponActor();
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
	if (StaminaGaugeWidgetComponent)
	{
		StaminaGaugeWidgetComponent->SetVisibility(false);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		const FName SourceLevelName = GetWorld() ? FName(*GetWorld()->GetMapName()) : NAME_None;
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyBunkerRescueReturn(SourceLevelName, RespawnTargetLevelName);
		}

		TunaGameInstance->ClearInventoryAndSave();
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	ApplyDeathRagdoll();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
		DisableInput(PlayerController);
	}

	GetWorldTimerManager().SetTimer(
		RespawnTransitionTimerHandle,
		this,
		&ATunaSweeperTopDownCharacter::StartRespawnTransition,
		FMath::Max(0.0f, RespawnDelaySeconds),
		false);
}

void ATunaSweeperTopDownCharacter::ApplyDeathRagdoll()
{
	if (!bEnableDeathRagdoll)
	{
		return;
	}

	const FVector RagdollImpulse = ResolveDeathRagdollImpulse();

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Capsule->SetGenerateOverlapEvents(false);
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh();
		CharacterMesh && CharacterMesh->GetSkeletalMeshAsset())
	{
		CharacterMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		if (!DeathRagdollCollisionProfileName.IsNone())
		{
			CharacterMesh->SetCollisionProfileName(DeathRagdollCollisionProfileName);
		}
		CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CharacterMesh->SetEnableGravity(true);
		CharacterMesh->SetAllBodiesSimulatePhysics(true);
		CharacterMesh->SetSimulatePhysics(true);
		CharacterMesh->WakeAllRigidBodies();
		CharacterMesh->bBlendPhysics = true;
		if (!RagdollImpulse.IsNearlyZero())
		{
			CharacterMesh->AddImpulse(RagdollImpulse);
		}
		return;
	}

	if (VisualMesh)
	{
		VisualMesh->SetHiddenInGame(false);
		VisualMesh->SetVisibility(true, true);
		VisualMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		if (!DeathRagdollCollisionProfileName.IsNone())
		{
			VisualMesh->SetCollisionProfileName(DeathRagdollCollisionProfileName);
		}
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		VisualMesh->SetEnableGravity(true);
		VisualMesh->SetSimulatePhysics(true);
		VisualMesh->WakeRigidBody();
		if (!RagdollImpulse.IsNearlyZero())
		{
			VisualMesh->AddImpulse(RagdollImpulse);
		}
	}
}

FVector ATunaSweeperTopDownCharacter::ResolveDeathRagdollImpulse() const
{
	FVector HorizontalDirection = LastDamageImpulseDirection;
	HorizontalDirection.Z = 0.0f;
	if (!HorizontalDirection.Normalize())
	{
		HorizontalDirection = -GetActorForwardVector();
		HorizontalDirection.Z = 0.0f;
		HorizontalDirection.Normalize();
	}

	if (HorizontalDirection.IsNearlyZero())
	{
		HorizontalDirection = -FVector::ForwardVector;
	}

	return HorizontalDirection * FMath::Max(0.0f, DeathRagdollHorizontalImpulse) +
		FVector::UpVector * FMath::Max(0.0f, DeathRagdollUpwardImpulse);
}

void ATunaSweeperTopDownCharacter::StartRespawnTransition()
{
	if (RespawnTargetLevelName.IsNone())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GameInstance);
		if (UTunaSweeperLevelTransitionSubsystem* TransitionSubsystem = GameInstance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>())
		{
			if (TransitionSubsystem->StartTransition(
				this,
				RespawnTargetLevelName,
				RespawnMediaSource,
				RespawnTransitionWidgetClass,
				RespawnFadeToBlackDuration,
				RespawnFadeFromBlackDuration,
				TunaGameInstance
					? TunaGameInstance->ResolveLocalizedText(
						FName(TEXT("ui.notification.rescue_cart")),
						FText::FromString(TEXT("\uAD6C\uAE09 \uCE74\uD2B8 \uD6C4\uC1A1 \uC911")))
					: FText::FromString(TEXT("\uAD6C\uAE09 \uCE74\uD2B8 \uD6C4\uC1A1 \uC911"))))
			{
				return;
			}
		}
	}

	UGameplayStatics::OpenLevel(this, RespawnTargetLevelName);
}

