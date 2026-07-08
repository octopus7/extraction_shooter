#include "TunaSweeperTopDownCharacterShared.h"

void ATunaSweeperTopDownCharacter::StartMeleeAttack()
{
	if (bIsDead || bIsRolling || !bMeleeWeaponSelected || !CanUseSelectedMeleeWeapon())
	{
		return;
	}

	UWorld* World = GetWorld();
	const float CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0f;
	if (CurrentTimeSeconds - LastMeleeAttackWorldSeconds < FMath::Max(0.01f, MeleeAttackCooldownSeconds))
	{
		return;
	}

	CancelReload();
	CancelItemUse();
	CloseAmmoSelection();
	EnsureEquippedWeaponActor();
	if (!EquippedWeapon)
	{
		return;
	}

	FVector AttackDirection = AimDirection.GetSafeNormal2D();
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = FVector::ForwardVector;
	}

	SetActorRotation(FRotator(0.0f, AttackDirection.Rotation().Yaw, 0.0f));
	SpawnMeleeSwingEffect(AttackDirection);

	LastMeleeAttackWorldSeconds = CurrentTimeSeconds;
	MeleeSwingElapsedSeconds = 0.0f;
	bMeleeSwingActive = true;
	bMeleeJudgementApplied = false;
	ResetEquippedWeaponRelativeTransform();
}

void ATunaSweeperTopDownCharacter::UpdateMeleeSwing(float DeltaSeconds)
{
	if (!bMeleeSwingActive)
	{
		return;
	}

	const float EffectiveDuration = FMath::Max(0.01f, MeleeSwingDurationSeconds);
	MeleeSwingElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	const float JudgementTime = FMath::Clamp(MeleeJudgementTimeSeconds, 0.0f, EffectiveDuration);
	if (!bMeleeJudgementApplied && MeleeSwingElapsedSeconds >= JudgementTime)
	{
		ApplyMeleeAttackJudgement();
	}

	const float Alpha = FMath::Clamp(MeleeSwingElapsedSeconds / EffectiveDuration, 0.0f, 1.0f);
	const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
	if (EquippedWeapon)
	{
		if (USceneComponent* WeaponRoot = EquippedWeapon->GetRootComponent())
		{
			const float SideOffset = FMath::Lerp(-24.0f, 24.0f, SmoothAlpha);
			const float LiftOffset = FMath::Sin(Alpha * PI) * 10.0f;
			const float SwingYaw = FMath::Lerp(74.0f, -66.0f, SmoothAlpha);
			const float SwingPitch = FMath::Sin(Alpha * PI) * -18.0f;
			const float SwingRoll = FMath::Lerp(-18.0f, 16.0f, SmoothAlpha);
			WeaponRoot->SetRelativeLocation(FVector(0.0f, SideOffset, LiftOffset));
			WeaponRoot->SetRelativeRotation(FRotator(SwingPitch, SwingYaw, SwingRoll));
		}
	}

	if (MeleeSwingElapsedSeconds >= EffectiveDuration)
	{
		if (!bMeleeJudgementApplied)
		{
			ApplyMeleeAttackJudgement();
		}
		bMeleeSwingActive = false;
		MeleeSwingElapsedSeconds = 0.0f;
		ResetEquippedWeaponRelativeTransform();
	}
}

void ATunaSweeperTopDownCharacter::CancelMeleeSwing()
{
	bMeleeSwingActive = false;
	bMeleeJudgementApplied = false;
	MeleeSwingElapsedSeconds = 0.0f;
	ResetEquippedWeaponRelativeTransform();
}

void ATunaSweeperTopDownCharacter::ResetEquippedWeaponRelativeTransform()
{
	if (EquippedWeapon)
	{
		if (USceneComponent* WeaponRoot = EquippedWeapon->GetRootComponent())
		{
			WeaponRoot->SetRelativeLocation(FVector::ZeroVector);
			WeaponRoot->SetRelativeRotation(FRotator::ZeroRotator);
			WeaponRoot->SetRelativeScale3D(FVector::OneVector);
		}
	}
}

void ATunaSweeperTopDownCharacter::ApplyEquippedMeleeWeaponVisual()
{
	if (!EquippedWeapon || !bMeleeWeaponSelected)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	FTunaSweeperItemInstance MeleeInstance;
	FTunaSweeperItemDefinition MeleeDefinition;
	if (!TunaGameInstance->TryGetEquipmentMeleeSlotItem(MeleeInstance, MeleeDefinition) ||
		MeleeDefinition.Id != TunaSweeperEquippedWeaponVisual::BaseballBatItemId)
	{
		return;
	}

	UStaticMesh* BaseballBatMesh = Cast<UStaticMesh>(TunaSweeperEquippedWeaponVisual::BaseballBatMeshPath.TryLoad());
	UMaterialInterface* BaseballBatMaterial =
		Cast<UMaterialInterface>(TunaSweeperEquippedWeaponVisual::BaseballBatMaterialPath.TryLoad());
	EquippedWeapon->SetWeaponMeshOverride(
		BaseballBatMesh,
		BaseballBatMaterial,
		FVector(26.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		FVector(0.54f, 1.0f, 1.0f));
}

void ATunaSweeperTopDownCharacter::ApplyMeleeAttackJudgement()
{
	bMeleeJudgementApplied = true;

	UWorld* World = GetWorld();
	if (!World || MeleeAttackDamage <= 0.0f || MeleeAttackRange <= 0.0f)
	{
		return;
	}

	FVector AttackDirection = AimDirection.GetSafeNormal2D();
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = FVector::ForwardVector;
	}

	const FVector Origin = GetActorLocation();
	const float RangeSquared = FMath::Square(MeleeAttackRange);
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(MeleeAttackHalfAngleDegrees, 0.0f, 180.0f)));

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperPlayerMeleeCone), false, this);
	if (EquippedWeapon)
	{
		QueryParams.AddIgnoredActor(EquippedWeapon);
	}

	TArray<FOverlapResult> Overlaps;
	if (!World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(MeleeAttackRange),
		QueryParams))
	{
		return;
	}

	TSet<AActor*> HitActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!IsValid(TargetActor) || TargetActor == this || HitActors.Contains(TargetActor))
		{
			continue;
		}

		FVector ToTarget = TargetActor->GetActorLocation() - Origin;
		ToTarget.Z = 0.0f;
		const float DistanceSquared = ToTarget.SizeSquared();
		if (DistanceSquared > RangeSquared)
		{
			continue;
		}

		FVector DirectionToTarget = ToTarget.GetSafeNormal();
		if (DirectionToTarget.IsNearlyZero())
		{
			DirectionToTarget = AttackDirection;
		}

		if (FVector::DotProduct(AttackDirection, DirectionToTarget) < CosHalfAngle)
		{
			continue;
		}

		HitActors.Add(TargetActor);
		const float AppliedDamage = UGameplayStatics::ApplyDamage(
			TargetActor,
			MeleeAttackDamage,
			GetController(),
			this,
			UDamageType::StaticClass());
		if (AppliedDamage > 0.0f)
		{
			const FVector HitLocation = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, MeleeImpactHeight);
			SpawnMeleeImpactBurst(HitLocation, -AttackDirection);
		}
	}
}

void ATunaSweeperTopDownCharacter::SpawnMeleeSwingEffect(const FVector& AttackDirection)
{
	UWorld* World = GetWorld();
	const FVector SafeAttackDirection = AttackDirection.GetSafeNormal2D();
	if (!World || SafeAttackDirection.IsNearlyZero())
	{
		return;
	}

	TSubclassOf<ATunaSweeperMeleeSwingTrailActor> LoadedTrailClass = MeleeSwingTrailActorClass.LoadSynchronous();
	if (!LoadedTrailClass)
	{
		LoadedTrailClass = ATunaSweeperMeleeSwingTrailActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<ATunaSweeperMeleeSwingTrailActor>(
		LoadedTrailClass,
		GetActorLocation(),
		FRotator(0.0f, SafeAttackDirection.Rotation().Yaw, 0.0f),
		SpawnParameters);
}

void ATunaSweeperTopDownCharacter::SpawnMeleeImpactBurst(
	const FVector& HitLocation,
	const FVector& BurstDirection)
{
	UWorld* World = GetWorld();
	const FVector SafeBurstDirection = BurstDirection.GetSafeNormal2D();
	if (!World || SafeBurstDirection.IsNearlyZero())
	{
		return;
	}

	TSubclassOf<ATunaSweeperMeleeImpactBurstActor> LoadedBurstClass = MeleeImpactBurstActorClass.LoadSynchronous();
	if (!LoadedBurstClass)
	{
		LoadedBurstClass = ATunaSweeperMeleeImpactBurstActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<ATunaSweeperMeleeImpactBurstActor>(
		LoadedBurstClass,
		HitLocation,
		FRotator(0.0f, SafeBurstDirection.Rotation().Yaw, 0.0f),
		SpawnParameters);
}

