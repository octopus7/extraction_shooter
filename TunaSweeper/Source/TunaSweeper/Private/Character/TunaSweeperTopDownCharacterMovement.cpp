#include "TunaSweeperTopDownCharacterShared.h"

#include "Character/TunaSweeperFootstepPresentationDataAsset.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundWaveProcedural.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"

namespace TunaSweeperPlayerFootsteps
{
	constexpr int32 ProceduralSampleRate = 24000;
	constexpr float ProceduralSoundDurationSeconds = 0.13f;
	constexpr float MinimumIntervalSeconds = 0.05f;
}

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

void ATunaSweeperTopDownCharacter::UpdatePlayerFootsteps(float DeltaSeconds)
{
	const UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	const bool bCanMakeFootsteps =
		MovementComponent &&
		MovementComponent->IsMovingOnGround() &&
		!bIsRolling &&
		HorizontalVelocity.Size() >= FMath::Max(0.0f, FootstepMinimumSpeed);
	if (!bCanMakeFootsteps)
	{
		FootstepElapsedSeconds = 0.0f;
		NextFootstepIntervalSeconds = 0.0f;
		bFootstepMovementStateInitialized = false;
		return;
	}

	const bool bSprintFootstep = bIsSprinting;
	if (!bFootstepMovementStateInitialized)
	{
		bFootstepMovementStateInitialized = true;
		bFootstepWasSprinting = bSprintFootstep;
		NextFootstepIntervalSeconds = RollNextFootstepInterval(bSprintFootstep);
	}
	else if (bFootstepWasSprinting != bSprintFootstep)
	{
		const float PreviousStrideProgress = NextFootstepIntervalSeconds > KINDA_SMALL_NUMBER
			? FMath::Clamp(FootstepElapsedSeconds / NextFootstepIntervalSeconds, 0.0f, 1.0f)
			: 0.0f;
		bFootstepWasSprinting = bSprintFootstep;
		NextFootstepIntervalSeconds = RollNextFootstepInterval(bSprintFootstep);
		FootstepElapsedSeconds = PreviousStrideProgress * NextFootstepIntervalSeconds;
	}

	FootstepElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	if (FootstepElapsedSeconds < NextFootstepIntervalSeconds)
	{
		return;
	}

	FootstepElapsedSeconds = FMath::Max(0.0f, FootstepElapsedSeconds - NextFootstepIntervalSeconds);
	EmitPlayerFootstep(bSprintFootstep);
	NextFootstepIntervalSeconds = RollNextFootstepInterval(bSprintFootstep);
}

float ATunaSweeperTopDownCharacter::RollNextFootstepInterval(bool bSprintFootstep) const
{
	const FVector2D IntervalRange = bSprintFootstep
		? SprintFootstepIntervalSeconds
		: WalkFootstepIntervalSeconds;
	const float MinInterval = FMath::Max(
		TunaSweeperPlayerFootsteps::MinimumIntervalSeconds,
		FMath::Min(IntervalRange.X, IntervalRange.Y));
	const float MaxInterval = FMath::Max(MinInterval, FMath::Max(IntervalRange.X, IntervalRange.Y));
	return FMath::FRandRange(MinInterval, MaxInterval);
}

void ATunaSweeperTopDownCharacter::EmitPlayerFootstep(bool bSprintFootstep)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector SourceLocation = GetActorTransform().TransformPosition(FootstepSourceOffset);
	const float NoiseLoudness = bSprintFootstep
		? SprintFootstepNoiseLoudness
		: WalkFootstepNoiseLoudness;
	const float NoiseMaxRange = bSprintFootstep
		? SprintFootstepNoiseMaxRange
		: WalkFootstepNoiseMaxRange;
	if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
	{
		NoiseSubsystem->ReportNoiseAtLocation(
			SourceLocation,
			NoiseLoudness,
			NoiseMaxRange,
			PlayerFootstepNoiseTag,
			this,
			this);
	}

	PlayPlayerFootstepSound(SourceLocation, bSprintFootstep);
}

void ATunaSweeperTopDownCharacter::PlayPlayerFootstepSound(
	const FVector& SoundLocation,
	bool bSprintFootstep)
{
	USoundBase* SoundToPlay = nullptr;
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const UTunaSweeperFootstepPresentationDataAsset* PresentationData = TunaGameInstance
		? TunaGameInstance->FootstepPresentationDataAsset.LoadSynchronous()
		: nullptr;
	if (PresentationData)
	{
		SoundToPlay = PresentationData->BasicFootstepSound.LoadSynchronous();
	}

	USoundWaveProcedural* ProceduralSound = nullptr;
	if (!SoundToPlay)
	{
		ProceduralSound = CreateProceduralFootstepSound(bSprintFootstep);
		SoundToPlay = ProceduralSound;
		if (ProceduralSound)
		{
			ActiveProceduralFootstepSounds.Add(ProceduralSound);
		}
	}
	if (!SoundToPlay)
	{
		return;
	}

	const float MinPitch = FMath::Max(0.01f, FMath::Min(FootstepSoundPitchRange.X, FootstepSoundPitchRange.Y));
	const float MaxPitch = FMath::Max(MinPitch, FMath::Max(FootstepSoundPitchRange.X, FootstepSoundPitchRange.Y));
	UGameplayStatics::PlaySoundAtLocation(
		this,
		SoundToPlay,
		SoundLocation,
		FMath::Max(0.0f, FootstepSoundVolumeMultiplier) * (bSprintFootstep ? 1.12f : 1.0f),
		FMath::FRandRange(MinPitch, MaxPitch));

	if (!ProceduralSound)
	{
		return;
	}

	const TWeakObjectPtr<ATunaSweeperTopDownCharacter> WeakThis(this);
	const TWeakObjectPtr<USoundWaveProcedural> WeakSound(ProceduralSound);
	FTimerHandle CleanupTimerHandle;
	GetWorldTimerManager().SetTimer(
		CleanupTimerHandle,
		FTimerDelegate::CreateLambda([WeakThis, WeakSound]()
		{
			ATunaSweeperTopDownCharacter* Character = WeakThis.Get();
			USoundWaveProcedural* Sound = WeakSound.Get();
			if (!Character || !Sound)
			{
				return;
			}
			Character->ActiveProceduralFootstepSounds.RemoveAll(
				[Sound](const TObjectPtr<USoundWaveProcedural>& Candidate)
				{
					return Candidate == Sound;
				});
		}),
		TunaSweeperPlayerFootsteps::ProceduralSoundDurationSeconds + 0.25f,
		false);
}

USoundWaveProcedural* ATunaSweeperTopDownCharacter::CreateProceduralFootstepSound(bool bSprintFootstep)
{
	USoundWaveProcedural* SoundWave = NewObject<USoundWaveProcedural>(this);
	if (!SoundWave)
	{
		return nullptr;
	}

	const int32 SampleCount = FMath::CeilToInt(
		TunaSweeperPlayerFootsteps::ProceduralSampleRate *
		TunaSweeperPlayerFootsteps::ProceduralSoundDurationSeconds);
	TArray<int16> Samples;
	Samples.SetNumUninitialized(SampleCount);
	FRandomStream NoiseStream(FMath::Rand());
	const float BodyFrequency = bSprintFootstep ? 112.0f : 88.0f;
	for (int32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex)
	{
		const float TimeSeconds = static_cast<float>(SampleIndex) /
			static_cast<float>(TunaSweeperPlayerFootsteps::ProceduralSampleRate);
		const float Attack = FMath::Min(1.0f, TimeSeconds * 180.0f);
		const float BodyEnvelope = Attack * FMath::Exp(-TimeSeconds * 30.0f);
		const float TextureEnvelope = Attack * FMath::Exp(-TimeSeconds * 48.0f);
		const float Body = FMath::Sin(2.0f * UE_PI * BodyFrequency * TimeSeconds) * BodyEnvelope;
		const float SoleSnap = FMath::Sin(2.0f * UE_PI * BodyFrequency * 4.3f * TimeSeconds) * TextureEnvelope;
		const float Texture = NoiseStream.FRandRange(-1.0f, 1.0f) * TextureEnvelope;
		const float Sample = FMath::Clamp(
			Body * 0.58f + SoleSnap * 0.15f + Texture * 0.18f,
			-0.9f,
			0.9f);
		Samples[SampleIndex] = static_cast<int16>(Sample * 32767.0f);
	}

	SoundWave->SetSampleRate(TunaSweeperPlayerFootsteps::ProceduralSampleRate);
	SoundWave->NumChannels = 1;
	SoundWave->Duration = TunaSweeperPlayerFootsteps::ProceduralSoundDurationSeconds;
	SoundWave->SoundGroup = SOUNDGROUP_Effects;
	SoundWave->bLooping = false;
	SoundWave->QueueAudio(
		reinterpret_cast<const uint8*>(Samples.GetData()),
		Samples.Num() * sizeof(int16));
	return SoundWave;
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
