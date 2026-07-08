#include "TunaSweeperTopDownCharacterShared.h"

void ATunaSweeperTopDownCharacter::SetAimWorldPoint(const FVector& WorldPoint)
{
	AimWorldPoint = WorldPoint;
	bHasAimWorldPoint = true;
	AimIntentActor.Reset();
	AimIntentComponent.Reset();
	AimIntentWorldPoint = WorldPoint;
	bHasAimIntent = false;

	const FVector ToAimPoint = FVector(WorldPoint.X - GetActorLocation().X, WorldPoint.Y - GetActorLocation().Y, 0.0f);
	const FVector NewAimDirection = ToAimPoint.GetSafeNormal();
	if (!NewAimDirection.IsNearlyZero())
	{
		AimDirection = NewAimDirection;
	}
}

void ATunaSweeperTopDownCharacter::SetAimWorldHit(const FVector& WorldPoint, const FHitResult& AimHit)
{
	SetAimWorldPoint(WorldPoint);

	AActor* HitActor = AimHit.GetActor();
	UPrimitiveComponent* HitComponent = AimHit.GetComponent();
	if (!HitActor || !HitComponent)
	{
		return;
	}

	AimIntentActor = HitActor;
	AimIntentComponent = HitComponent;
	AimIntentWorldPoint = AimHit.ImpactPoint;
	bHasAimIntent = true;
}

FVector2D ATunaSweeperTopDownCharacter::GetWeaponRecoilCrosshairScreenOffset() const
{
	return WeaponRecoilOffsetDegrees * FMath::Max(0.0f, WeaponRecoilScreenPixelsPerDegree);
}

float ATunaSweeperTopDownCharacter::GetWeaponAimPlaneZ() const
{
	return EquippedWeapon ? EquippedWeapon->GetMuzzleWorldLocation().Z : GetActorLocation().Z;
}

void ATunaSweeperTopDownCharacter::CyclePlayerCameraMode()
{
	switch (CurrentCameraMode)
	{
	case ETunaSweeperPlayerCameraMode::Default:
		CurrentCameraMode = ETunaSweeperPlayerCameraMode::TopDown;
		break;
	case ETunaSweeperPlayerCameraMode::TopDown:
		CurrentCameraMode = ETunaSweeperPlayerCameraMode::LowFront;
		break;
	case ETunaSweeperPlayerCameraMode::LowFront:
	default:
		CurrentCameraMode = ETunaSweeperPlayerCameraMode::Default;
		break;
	}
}

float ATunaSweeperTopDownCharacter::ResolveCameraCursorLeadRatio() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return 0.0f;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return 0.0f;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return 0.0f;
	}

	const FVector2D ClampedMousePosition(
		FMath::Clamp(MouseX, 0.0f, static_cast<float>(ViewportSizeX)),
		FMath::Clamp(MouseY, 0.0f, static_cast<float>(ViewportSizeY)));
	FVector2D CharacterScreenPosition(
		static_cast<float>(ViewportSizeX) * 0.5f,
		static_cast<float>(ViewportSizeY) * 0.5f);
	PlayerController->ProjectWorldLocationToScreen(GetActorLocation(), CharacterScreenPosition, true);

	const FVector2D CursorDelta = ClampedMousePosition - CharacterScreenPosition;
	const float CursorDistance = CursorDelta.Size();
	if (CursorDistance <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const FVector2D CursorDirection = CursorDelta / CursorDistance;
	float DistanceToViewportEdge = TNumericLimits<float>::Max();
	auto ConsiderEdgeDistance = [&DistanceToViewportEdge](float CandidateDistance)
	{
		if (CandidateDistance > KINDA_SMALL_NUMBER)
		{
			DistanceToViewportEdge = FMath::Min(DistanceToViewportEdge, CandidateDistance);
		}
	};

	if (CursorDirection.X > KINDA_SMALL_NUMBER)
	{
		ConsiderEdgeDistance((static_cast<float>(ViewportSizeX) - CharacterScreenPosition.X) / CursorDirection.X);
	}
	else if (CursorDirection.X < -KINDA_SMALL_NUMBER)
	{
		ConsiderEdgeDistance((0.0f - CharacterScreenPosition.X) / CursorDirection.X);
	}

	if (CursorDirection.Y > KINDA_SMALL_NUMBER)
	{
		ConsiderEdgeDistance((static_cast<float>(ViewportSizeY) - CharacterScreenPosition.Y) / CursorDirection.Y);
	}
	else if (CursorDirection.Y < -KINDA_SMALL_NUMBER)
	{
		ConsiderEdgeDistance((0.0f - CharacterScreenPosition.Y) / CursorDirection.Y);
	}

	if (DistanceToViewportEdge == TNumericLimits<float>::Max() || DistanceToViewportEdge <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(CursorDistance / DistanceToViewportEdge, 0.0f, 1.0f);
}

void ATunaSweeperTopDownCharacter::UpdateAimingVisuals(float DeltaSeconds)
{
	if (IsGameplayActionInputLocked())
	{
		return;
	}

	float HitReactionRollDegrees = 0.0f;
	float HitReactionFOVDegrees = 0.0f;
	const FVector HitReactionOffset = UpdateDamageCameraReaction(DeltaSeconds, HitReactionRollDegrees, HitReactionFOVDegrees);
	const FTunaSweeperPlayerCameraModeSettings CameraModeSettings = ResolveCurrentCameraModeSettings();

	if (!bIsRolling && !AimDirection.IsNearlyZero())
	{
		const FRotator CurrentRotation = GetActorRotation();
		const FRotator TargetRotation(0.0f, AimDirection.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 18.0f));
	}

	if (TopDownCamera)
	{
		const float TargetFOV = CameraModeSettings.DefaultFOV;
		CurrentCameraBaseFOV = FMath::FInterpTo(CurrentCameraBaseFOV, TargetFOV, DeltaSeconds, CameraInterpSpeed);
		TopDownCamera->SetFieldOfView(CurrentCameraBaseFOV + HitReactionFOVDegrees);

		const FRotator TargetCameraRotation = DefaultCameraRelativeRotation + FRotator(0.0f, 0.0f, HitReactionRollDegrees);
		TopDownCamera->SetRelativeRotation(TargetCameraRotation);
	}

	if (CameraBoom)
	{
		CurrentCameraArmLength = FMath::FInterpTo(
			CurrentCameraArmLength,
			CameraModeSettings.TargetArmLength,
			DeltaSeconds,
			CameraInterpSpeed);
		CameraBoom->TargetArmLength = CurrentCameraArmLength;

		CurrentCameraBoomRotation = FMath::RInterpTo(
			CurrentCameraBoomRotation,
			CameraModeSettings.BoomRotation,
			DeltaSeconds,
			CameraInterpSpeed);
		CameraBoom->SetRelativeRotation(CurrentCameraBoomRotation);

		CurrentCameraModeOffset = FMath::VInterpTo(
			CurrentCameraModeOffset,
			CameraModeSettings.TargetOffset,
			DeltaSeconds,
			CameraInterpSpeed);

		FVector AimTargetOffset = FVector::ZeroVector;
		if (bHasAimWorldPoint)
		{
			const FVector AimLeadDirection = AimDirection.GetSafeNormal2D();
			if (!AimLeadDirection.IsNearlyZero())
			{
				AimTargetOffset =
					AimLeadDirection *
					FMath::Max(0.0f, AimCameraLeadDistance) *
					ResolveCameraCursorLeadRatio();
			}
		}
		CurrentCameraAimOffset = FMath::VInterpTo(CurrentCameraAimOffset, AimTargetOffset, DeltaSeconds, CameraInterpSpeed);
		CameraBoom->TargetOffset = CurrentCameraModeOffset + CurrentCameraAimOffset + HitReactionOffset;
	}

	UpdateEquippedWeaponLaserSightBeam();
}

FTunaSweeperPlayerCameraModeSettings ATunaSweeperTopDownCharacter::ResolveCurrentCameraModeSettings() const
{
	switch (CurrentCameraMode)
	{
	case ETunaSweeperPlayerCameraMode::TopDown:
		return TopDownCameraModeSettings;
	case ETunaSweeperPlayerCameraMode::LowFront:
		return LowFrontCameraModeSettings;
	case ETunaSweeperPlayerCameraMode::Default:
	default:
		break;
	}

	FTunaSweeperPlayerCameraModeSettings DefaultSettings;
	DefaultSettings.TargetArmLength = DefaultCameraArmLength;
	DefaultSettings.BoomRotation = DefaultCameraBoomRotation;
	DefaultSettings.TargetOffset = DefaultCameraTargetOffset;
	DefaultSettings.DefaultFOV = DefaultCameraFOV;
	DefaultSettings.AimFOV = AimCameraFOV;
	return DefaultSettings;
}

void ATunaSweeperTopDownCharacter::TriggerDamageCameraReaction(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AActor* DamageCauser)
{
	if (DamageAmount <= 0.0f || (!CameraBoom && !TopDownCamera))
	{
		return;
	}

	ActiveCameraHitReaction = ResolveDamageCameraReactionSettings(DamageEvent, DamageCauser);
	if (const ATunaSweeperProjectile* ProjectileCauser = Cast<ATunaSweeperProjectile>(DamageCauser))
	{
		const float ProjectileReactionScale = ProjectileCauser->GetCameraHitReactionScale();
		ActiveCameraHitReaction.LocationAmplitude *= ProjectileReactionScale;
		ActiveCameraHitReaction.RollAmplitudeDegrees *= ProjectileReactionScale;
		ActiveCameraHitReaction.FOVAmplitudeDegrees *= ProjectileReactionScale;
	}
	if (ActiveCameraHitReaction.Duration <= 0.0f ||
		(ActiveCameraHitReaction.LocationAmplitude <= 0.0f && ActiveCameraHitReaction.RollAmplitudeDegrees <= 0.0f))
	{
		return;
	}

	const float DamageReference = FMath::Max(0.01f, ActiveCameraHitReaction.DamageScaleReference);
	const float MinDamageScale = FMath::Min(ActiveCameraHitReaction.MinDamageScale, ActiveCameraHitReaction.MaxDamageScale);
	const float MaxDamageScale = FMath::Max(ActiveCameraHitReaction.MinDamageScale, ActiveCameraHitReaction.MaxDamageScale);
	CameraHitReactionScale = FMath::Clamp(
		DamageAmount / DamageReference,
		MinDamageScale,
		MaxDamageScale);
	CameraHitReactionDirection = ResolveDamageCameraReactionDirection(DamageCauser);
	CameraHitReactionElapsed = 0.0f;
	CameraHitReactionPhase = 0.0f;
	bCameraHitReactionActive = true;
}

void ATunaSweeperTopDownCharacter::TriggerDebuffCameraReaction(
	FName DebuffId,
	const FTunaSweeperDebuffCameraReactionSettings& ReactionSettings)
{
	(void)DebuffId;

	if (!CameraBoom && !TopDownCamera)
	{
		return;
	}

	FTunaSweeperDebuffCameraReactionSettings NormalizedSettings = ReactionSettings;
	NormalizedSettings.Normalize();
	if (NormalizedSettings.DurationSeconds <= 0.0f ||
		(NormalizedSettings.LocationAmplitude <= 0.0f &&
		 NormalizedSettings.RollAmplitudeDegrees <= 0.0f &&
		 NormalizedSettings.FOVAmplitudeDegrees <= 0.0f))
	{
		return;
	}

	ActiveCameraHitReaction.Duration = NormalizedSettings.DurationSeconds;
	ActiveCameraHitReaction.LocationAmplitude = NormalizedSettings.LocationAmplitude;
	ActiveCameraHitReaction.RollAmplitudeDegrees = NormalizedSettings.RollAmplitudeDegrees;
	ActiveCameraHitReaction.FOVAmplitudeDegrees = NormalizedSettings.FOVAmplitudeDegrees;
	ActiveCameraHitReaction.Frequency = NormalizedSettings.Frequency;
	ActiveCameraHitReaction.DamageScaleReference = 1.0f;
	ActiveCameraHitReaction.MinDamageScale = 1.0f;
	ActiveCameraHitReaction.MaxDamageScale = 1.0f;
	CameraHitReactionScale = 1.0f;

	const float ReactionAngleRadians = FMath::FRandRange(0.0f, 2.0f * UE_PI);
	CameraHitReactionDirection = FVector(FMath::Cos(ReactionAngleRadians), FMath::Sin(ReactionAngleRadians), 0.0f);
	CameraHitReactionElapsed = 0.0f;
	CameraHitReactionPhase = FMath::FRandRange(0.0f, 2.0f * UE_PI);
	bCameraHitReactionActive = true;
}

ETunaSweeperHitReactionType ATunaSweeperTopDownCharacter::ResolveDamageCameraReactionType(
	FDamageEvent const& DamageEvent,
	AActor* DamageCauser) const
{
	(void)DamageEvent;

	if (Cast<ATunaSweeperProjectile>(DamageCauser))
	{
		return ETunaSweeperHitReactionType::Projectile;
	}

	return ETunaSweeperHitReactionType::Default;
}

FTunaSweeperCameraHitReactionSettings ATunaSweeperTopDownCharacter::ResolveDamageCameraReactionSettings(
	FDamageEvent const& DamageEvent,
	AActor* DamageCauser) const
{
	const ETunaSweeperHitReactionType ReactionType = ResolveDamageCameraReactionType(DamageEvent, DamageCauser);
	if (const FTunaSweeperCameraHitReactionSettings* OverrideSettings = CameraHitReactionOverrides.Find(ReactionType))
	{
		return *OverrideSettings;
	}

	return DefaultCameraHitReaction;
}

FVector ATunaSweeperTopDownCharacter::ResolveDamageCameraReactionDirection(AActor* DamageCauser) const
{
	FVector ReactionDirection = DamageCauser
		? GetActorLocation() - DamageCauser->GetActorLocation()
		: -GetActorForwardVector();
	ReactionDirection.Z = 0.0f;

	if (!ReactionDirection.Normalize())
	{
		ReactionDirection = -GetActorForwardVector();
		ReactionDirection.Z = 0.0f;
		ReactionDirection.Normalize();
	}

	return ReactionDirection.IsNearlyZero() ? FVector::ForwardVector : ReactionDirection;
}

FVector ATunaSweeperTopDownCharacter::UpdateDamageCameraReaction(
	float DeltaSeconds,
	float& OutRollDegrees,
	float& OutFOVDegrees)
{
	OutRollDegrees = 0.0f;
	OutFOVDegrees = 0.0f;
	if (!bCameraHitReactionActive)
	{
		return FVector::ZeroVector;
	}

	const float Duration = FMath::Max(0.01f, ActiveCameraHitReaction.Duration);
	CameraHitReactionElapsed += FMath::Max(0.0f, DeltaSeconds);

	const float NormalizedTime = FMath::Clamp(CameraHitReactionElapsed / Duration, 0.0f, 1.0f);
	if (NormalizedTime >= 1.0f)
	{
		bCameraHitReactionActive = false;
		return FVector::ZeroVector;
	}

	const float Decay = FMath::Square(1.0f - NormalizedTime);
	const float BaseRadians = (CameraHitReactionElapsed * ActiveCameraHitReaction.Frequency * 2.0f * PI) + CameraHitReactionPhase;
	const float ForwardOscillation = FMath::Sin(BaseRadians);
	const float SideOscillation = FMath::Cos(BaseRadians * 1.37f);
	const FVector PlanarDirection = CameraHitReactionDirection.GetSafeNormal2D();
	FVector SideDirection = FVector::CrossProduct(FVector::UpVector, PlanarDirection).GetSafeNormal();
	if (SideDirection.IsNearlyZero())
	{
		SideDirection = FVector::RightVector;
	}

	const float ScaledDecay = CameraHitReactionScale * Decay;
	OutRollDegrees = ActiveCameraHitReaction.RollAmplitudeDegrees * ScaledDecay * ForwardOscillation;
	OutFOVDegrees = ActiveCameraHitReaction.FOVAmplitudeDegrees * ScaledDecay;
	return (PlanarDirection * ForwardOscillation + SideDirection * 0.45f * SideOscillation) *
		ActiveCameraHitReaction.LocationAmplitude *
		ScaledDecay;
}

