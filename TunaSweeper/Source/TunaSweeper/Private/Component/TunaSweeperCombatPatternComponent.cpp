#include "Component/TunaSweeperCombatPatternComponent.h"

#include "AI/TunaSweeperAttackTelegraph.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperEnemyAIController.h"
#include "AI/TunaSweeperMissileTurret.h"
#include "AI/TunaSweeperRollingRobotMinion.h"
#include "AIController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Component/TunaSweeperScratchComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Effect/TunaSweeperCombatPatternEffectActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"

UTunaSweeperCombatPatternComponent::UTunaSweeperCombatPatternComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	ChargeTelegraphClass = ATunaSweeperAttackTelegraph::StaticClass();
	MissileTurretClass = ATunaSweeperMissileTurret::StaticClass();
	RollingMinionClass = ATunaSweeperRollingRobotMinion::StaticClass();
	PatternSequence = { ETunaSweeperCombatPattern::MissileTurret, ETunaSweeperCombatPattern::Charge,
		ETunaSweeperCombatPattern::RollingMinions };
	EnabledPatterns = PatternSequence;
}

void UTunaSweeperCombatPatternComponent::BeginPlay()
{
	Super::BeginPlay();
	if (GetOwner()) AddTickPrerequisiteActor(GetOwner());
}

bool UTunaSweeperCombatPatternComponent::IsOwnerAvailable() const
{
	if (!IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed()) return false;
	const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(GetOwner());
	return !Enemy || !Enemy->IsDead();
}

bool UTunaSweeperCombatPatternComponent::IsValidTarget(AActor* Actor) const
{
	if (!IsValid(Actor) || Actor->IsActorBeingDestroyed() || !GetWorld()) return false;
	if (const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(Actor); Enemy && Enemy->IsDead()) return false;
	if (const ATunaSweeperTopDownCharacter* Player = Cast<ATunaSweeperTopDownCharacter>(Actor); Player && Player->IsDead()) return false;
	const UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
	return Factions && Factions->CanTargetActor(GetOwner(), Actor);
}

bool UTunaSweeperCombatPatternComponent::TryStartAutomaticPattern(AActor* TargetActor)
{
	if (!bAutomaticPatterns || PatternSequence.IsEmpty()) return false;
	for (int32 Offset = 0; Offset < PatternSequence.Num(); ++Offset)
	{
		const int32 Index = (SequenceIndex + Offset) % PatternSequence.Num();
		if (TryStartPattern(PatternSequence[Index], TargetActor))
		{
			SequenceIndex = (Index + 1) % PatternSequence.Num();
			return true;
		}
	}
	return false;
}

bool UTunaSweeperCombatPatternComponent::TryStartPattern(ETunaSweeperCombatPattern Pattern, AActor* TargetActor)
{
	if (!EnabledPatterns.Contains(Pattern) || !GetWorld() || !IsOwnerAvailable() || IsPatternActive() || !IsValidTarget(TargetActor)
		|| GetWorld()->GetTimeSeconds() < NextPatternTime
		|| FVector::DistSquared2D(GetOwner()->GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(FMath::Max(100.0f, ActivationRange)))
	{
		return false;
	}
	if (const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(GetOwner());
		Enemy && Enemy->IsStandardCombatSuppressed()) return false;
	ATunaSweeperEnemyAIController* EnemyAI = nullptr;
	if (const APawn* Pawn = Cast<APawn>(GetOwner())) EnemyAI = Cast<ATunaSweeperEnemyAIController>(Pawn->GetController());
	if (EnemyAI && !EnemyAI->CanStartCombatPattern()) return false;
	RemoveExpiredSummons();
	Target = TargetActor;
	ActivePattern = Pattern;
	PhaseSeconds = 0.0f;
	LockedDirection = (TargetActor->GetActorLocation() - GetOwner()->GetActorLocation()).GetSafeNormal2D();
	if (LockedDirection.IsNearlyZero()) LockedDirection = GetOwner()->GetActorForwardVector().GetSafeNormal2D();

	switch (Pattern)
	{
	case ETunaSweeperCombatPattern::Charge:
		if (!PrepareCharge(TargetActor)) return false;
		Phase = ETunaSweeperCombatPatternPhase::Warning;
		ChargeVictims.Reset();
		++ChargeSerial;
		ChargeTrailCountdown = 0.0f;
		break;
	case ETunaSweeperCombatPattern::MissileTurret:
		if (Turrets.Num() >= FMath::Clamp(MaxActiveTurrets, 1, 8) || !SpawnTurret(TargetActor)) return false;
		Phase = ETunaSweeperCombatPatternPhase::Recovery;
		break;
	case ETunaSweeperCombatPattern::RollingMinions:
		if (bWaitForMinionsDefeated && !Minions.IsEmpty()) return false;
		WaveSize = FMath::Min(FMath::Clamp(MinionsPerWave, 1, 24), FMath::Clamp(MaxActiveMinions, 1, 64) - Minions.Num());
		if (!RollingMinionClass || WaveSize <= 0) return false;
		WaveSpawnIndex = 0;
		NextMinionSeconds = 0.0f;
		// Lock the preparation duration so changing settings cannot shorten an active cue.
		MinionWarningDuration = FMath::Max(0.0f, MinionWarningSeconds);
		Phase = MinionWarningDuration > 0.0f ? ETunaSweeperCombatPatternPhase::Warning : ETunaSweeperCombatPatternPhase::Executing;
		if (Phase == ETunaSweeperCombatPatternPhase::Warning) PulseMinionWarning();
		break;
	default:
		return false;
	}
	// Set phase before StopMovement: path-following completion callbacks must see suppression.
	if (EnemyAI) EnemyAI->NotifyCombatPatternStarted();
	HoldOwnerMovement();
	SetComponentTickEnabled(true);
	return true;
}

bool UTunaSweeperCombatPatternComponent::FindGround(const FVector& NearLocation, FVector& OutGround) const
{
	if (!GetWorld()) return false;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CombatPatternGround), false, GetOwner());
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByObjectType(Hit, NearLocation + FVector(0, 0, 150),
		NearLocation - FVector(0, 0, 350), Objects, Params) || Hit.ImpactNormal.Z < 0.72f)
	{
		return false;
	}
	OutGround = Hit.ImpactPoint;
	return true;
}

bool UTunaSweeperCombatPatternComponent::PrepareCharge(AActor* TargetActor)
{
	ACharacter* Character = Cast<ACharacter>(GetOwner());
	if (!Character || !ChargeTelegraphClass || !Character->GetCharacterMovement()->IsMovingOnGround()) return false;
	FVector Ground;
	if (!FindGround(Character->GetActorLocation(), Ground)) return false;
	ChargeStart = Character->GetActorLocation();
	const float Distance = FMath::Min(FMath::Max(100.0f, ChargeDistance),
		FMath::Max(100.0f, FVector::Dist2D(ChargeStart, TargetActor->GetActorLocation()) + 150.0f));
	ChargeEnd = ChargeStart + LockedDirection * Distance;
	// Stop the warning at solid scenery, while targets in the lane remain dodgeable.
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CombatPatternChargePlan), false, GetOwner());
	FHitResult Hit;
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	if (GetWorld()->SweepSingleByObjectType(Hit, ChargeStart, ChargeEnd, FQuat::Identity, Objects,
		FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight() - 2.0f), Params))
	{
		ChargeEnd = ChargeStart + LockedDirection * FMath::Max(0.0f, Hit.Distance - 2.0f);
	}
	// Reject cliff crossings and steep steps before advertising the lane.
	FVector PreviousGround = Ground;
	const int32 Steps = FMath::CeilToInt(FVector::Dist2D(ChargeStart, ChargeEnd) / 60.0f);
	for (int32 Index = 1; Index <= Steps; ++Index)
	{
		FVector NextGround;
		const FVector Sample = FMath::Lerp(ChargeStart, ChargeEnd, static_cast<float>(Index) / Steps);
		if (!FindGround(FVector(Sample.X, Sample.Y, PreviousGround.Z + Capsule->GetScaledCapsuleHalfHeight()), NextGround)
			|| FMath::Abs(NextGround.Z - PreviousGround.Z) > Character->GetCharacterMovement()->MaxStepHeight)
		{
			ChargeEnd = FVector(PreviousGround.X, PreviousGround.Y, ChargeStart.Z);
			break;
		}
		PreviousGround = NextGround;
	}
	ChargeEnd.Z = PreviousGround.Z + Capsule->GetScaledCapsuleHalfHeight() + 2.0f;
	if (FVector::Dist2D(ChargeStart, ChargeEnd) < 80.0f) return false;
	FActorSpawnParameters Spawn;
	Spawn.Owner = GetOwner();
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Warning = GetWorld()->SpawnActor<ATunaSweeperAttackTelegraph>(ChargeTelegraphClass, Ground, FRotator::ZeroRotator, Spawn);
	if (!Warning) return false;
	Warning->InitLane(Ground - LockedDirection * Capsule->GetScaledCapsuleRadius(), FVector(ChargeEnd.X, ChargeEnd.Y, PreviousGround.Z) + LockedDirection * Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleRadius(), FMath::Max(0.2f, ChargeWarningSeconds));
	return true;
}

void UTunaSweeperCombatPatternComponent::HoldOwnerMovement()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		SavedMovementMode = Movement->MovementMode;
		SavedCustomMovementMode = Movement->CustomMovementMode;
		bSavedOrientToMovement = Movement->bOrientRotationToMovement;
		bSavedUseControllerRotationYaw = Character->bUseControllerRotationYaw;
		bMovementHeld = true;
		if (AAIController* AI = Cast<AAIController>(Character->GetController()))
		{
			AI->StopMovement();
			AI->ClearFocus(EAIFocusPriority::Gameplay);
		}
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
		Movement->bOrientRotationToMovement = false;
		Character->bUseControllerRotationYaw = false;
	}
}

void UTunaSweeperCombatPatternComponent::RestoreOwnerMovement()
{
	if (!bMovementHeld) return;
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
		Movement->bOrientRotationToMovement = bSavedOrientToMovement;
		Character->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
		if (IsOwnerAvailable())
		{
			Movement->StopMovementImmediately();
			Movement->SetMovementMode(static_cast<EMovementMode>(SavedMovementMode), SavedCustomMovementMode);
		}
	}
	bMovementHeld = false;
}

void UTunaSweeperCombatPatternComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!IsOwnerAvailable())
	{
		CancelPatterns();
		return;
	}
	if (!IsPatternActive())
	{
		RemoveExpiredSummons();
		SetComponentTickEnabled(!Turrets.IsEmpty() || !Minions.IsEmpty());
		return;
	}
	// Losing/destroying a target cancels an attack; it never accelerates a warning.
	if (!IsValidTarget(Target.Get()))
	{
		CancelPatterns(false);
		return;
	}
	PhaseSeconds += FMath::Max(0.0f, DeltaTime);
	if (Phase == ETunaSweeperCombatPatternPhase::Recovery)
	{
		if (PhaseSeconds >= FMath::Max(0.1f, RecoverySeconds)) FinishPattern();
		return;
	}
	if (ActivePattern == ETunaSweeperCombatPattern::Charge)
	{
		if (Phase == ETunaSweeperCombatPatternPhase::Warning)
		{
			const float WarningSeconds = IsValid(Warning) ? Warning->GetWarningDuration() : 0.2f;
			if (!IsValid(Warning)) { CancelPatterns(false); return; }
			Warning->SetProgress(FMath::Clamp(PhaseSeconds / WarningSeconds, 0.0f, 1.0f));
			ACharacter* Character = CastChecked<ACharacter>(GetOwner());
			const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(Character);
			const float TurnSpeed = Enemy ? Enemy->GetCombatProfile().TurnSpeedDegreesPerSecond : 360.0f;
			const float Yaw = FMath::FixedTurn(Character->GetActorRotation().Yaw, LockedDirection.Rotation().Yaw, FMath::Max(0.0f, TurnSpeed) * DeltaTime);
			Character->SetActorRotation(FRotator(0, Yaw, 0));
			if (PhaseSeconds >= WarningSeconds)
			{
				const float Tolerance = Enemy ? Enemy->GetCombatProfile().AttackFacingToleranceDegrees : 12.0f;
				if (FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw, LockedDirection.Rotation().Yaw)) > Tolerance)
				{
					EnterRecovery();
					return;
				}
				Phase = ETunaSweeperCombatPatternPhase::Executing;
				PhaseSeconds = 0.0f;
			}
		}
		else TickCharge(DeltaTime);
	}
	else if (ActivePattern == ETunaSweeperCombatPattern::RollingMinions)
	{
		if (Phase == ETunaSweeperCombatPatternPhase::Warning)
		{
			if (PhaseSeconds < MinionWarningDuration)
			{
				MinionWarningPulseSeconds -= FMath::Max(0.0f, DeltaTime);
				if (MinionWarningPulseSeconds <= 0.0f) PulseMinionWarning();
				return;
			}
			ClearMinionWarning();
			Phase = ETunaSweeperCombatPatternPhase::Executing;
			PhaseSeconds = 0.0f;
		}
		NextMinionSeconds -= FMath::Max(0.0f, DeltaTime);
		// A hitch must not emit the whole wave in one frame.
		if (NextMinionSeconds <= 0.0f)
		{
			SpawnNextMinion();
			NextMinionSeconds = FMath::Max(0.05f, MinionSpawnInterval);
			if (WaveSpawnIndex >= WaveSize) EnterRecovery();
		}
	}
}

void UTunaSweeperCombatPatternComponent::TickCharge(float DeltaTime)
{
	ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	ChargeTrailCountdown -= DeltaTime;
	if (ChargeTrailCountdown <= 0.0f)
	{
		ATunaSweeperCombatPatternEffectActor::Spawn(GetWorld(), ETunaSweeperCombatPatternEffect::ChargeTrail,
			Character->GetActorLocation() - FVector(0, 0, Capsule->GetScaledCapsuleHalfHeight()),
			Capsule->GetScaledCapsuleRadius() * 1.3f, -LockedDirection, GetOwner());
		ChargeTrailCountdown = 0.08f;
	}
	float Travel = FMath::Min(FMath::Max(100.0f, ChargeSpeed) * FMath::Max(0.0f, DeltaTime), FVector::Dist2D(Character->GetActorLocation(), ChargeEnd));
	while (Travel > 0.01f)
	{
		const float Step = FMath::Min(Travel, 60.0f);
		const FVector Start = Character->GetActorLocation();
		FVector Destination = Start + LockedDirection * Step;
		FVector Ground;
		if (!FindGround(Destination, Ground) || FMath::Abs(Ground.Z + Capsule->GetScaledCapsuleHalfHeight() + 2.0f - Start.Z) > Character->GetCharacterMovement()->MaxStepHeight)
		{
			EnterRecovery();
			return;
		}
		Destination.Z = Ground.Z + Capsule->GetScaledCapsuleHalfHeight() + 2.0f;
		FHitResult Hit;
		Character->SetActorLocation(Destination, true, &Hit);
		// MoveComponent pulls the actor slightly back from a hit; use the true swept contact
		// for body damage so that a blocking capsule is not mistaken for a near miss.
		ApplyChargeDamage(Start, Hit.bBlockingHit && !Hit.bStartPenetrating ? Hit.Location : Character->GetActorLocation());
		if (Hit.bBlockingHit || FVector::DistSquared2D(Start, Character->GetActorLocation()) < 0.01f)
		{
			EnterRecovery();
			return;
		}
		Travel -= Step;
	}
	if (FVector::Dist2D(Character->GetActorLocation(), ChargeEnd) < 2.0f) EnterRecovery();
}

void UTunaSweeperCombatPatternComponent::ApplyChargeDamage(const FVector& Start, const FVector& End)
{
	const UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>();
	const ACharacter* Character = CastChecked<ACharacter>(GetOwner());
	const UCapsuleComponent* Capsule = Character->GetCapsuleComponent();
	TArray<AActor*> Candidates;
	if (!Factions) return;
	Factions->GetActorsWithAttitude(GetOwner(), ETunaSweeperFactionAttitude::Hostile, Candidates);
	for (AActor* Victim : Candidates)
	{
		if (!IsValidTarget(Victim) || ChargeVictims.Contains(Victim)) continue;
		float Radius = 0.0f, HalfHeight = 0.0f;
		Victim->GetSimpleCollisionCylinder(Radius, HalfHeight);
		const FVector Location = Victim->GetActorLocation();
		const FVector Closest = FMath::ClosestPointOnSegment(FVector(Location.X, Location.Y, Start.Z), Start, FVector(End.X, End.Y, Start.Z));
		if (FMath::Abs(Location.Z - FMath::Lerp(Start.Z, End.Z, 0.5f)) > HalfHeight + Capsule->GetScaledCapsuleHalfHeight()) continue;
		const float Clearance = FVector::Dist2D(Location, Closest) - Radius - Capsule->GetScaledCapsuleRadius();
		if (Clearance > 40.0f) continue;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(CombatPatternChargeDamage), false, GetOwner());
		Params.AddIgnoredActor(Victim);
		FHitResult Obstruction;
		if (GetWorld()->LineTraceSingleByChannel(Obstruction, Closest, Location, ECC_Visibility, Params)) continue;
		if (UTunaSweeperScratchComponent* Scratch = Victim->FindComponentByClass<UTunaSweeperScratchComponent>())
		{
			Scratch->TryRegisterNearMiss(IsValid(Warning) ? Warning.Get() : GetOwner(), ChargeSerial, ETunaSweeperNearMissAttackType::Melee, Clearance, Clearance <= 1.0f);
		}
		if (Clearance <= 1.0f)
		{
			ChargeVictims.Add(Victim);
			UGameplayStatics::ApplyDamage(Victim, FMath::Max(0.0f, ChargeDamage), Character->GetController(), GetOwner(), nullptr);
		}
	}
}

bool UTunaSweeperCombatPatternComponent::SpawnTurret(AActor* TargetActor)
{
	if (!MissileTurretClass) return false;
	const float OwnerRadius = GetOwner()->GetSimpleCollisionRadius();
	for (int32 Attempt = 0; Attempt < 4; ++Attempt)
	{
		const FVector Offset = GetOwner()->GetActorForwardVector().RotateAngleAxis(90.0f + 90.0f * Attempt, FVector::UpVector) * (OwnerRadius + 160.0f);
		FVector Ground;
		if (!FindGround(GetOwner()->GetActorLocation() + Offset, Ground)) continue;
		const ATunaSweeperMissileTurret* Defaults = MissileTurretClass->GetDefaultObject<ATunaSweeperMissileTurret>();
		const UBoxComponent* Box = Defaults->FindComponentByClass<UBoxComponent>();
		const FVector Extent = Box ? Box->GetUnscaledBoxExtent() : FVector(44.0f, 44.0f, 60.0f);
		const FVector Center = Ground + FVector(0, 0, Extent.Z + 3.0f);
		FCollisionQueryParams PlacementQuery(SCENE_QUERY_STAT(CombatPatternTurretPlacement), false, GetOwner());
		FHitResult BlockedPlacement;
		if (GetWorld()->OverlapBlockingTestByChannel(Center, FQuat::Identity, ECC_Pawn,
			FCollisionShape::MakeBox(Extent), PlacementQuery)
			|| GetWorld()->SweepSingleByChannel(BlockedPlacement, GetOwner()->GetActorLocation(), Center,
				FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Extent.X), PlacementQuery)) continue;
		const FTransform Transform(FRotator::ZeroRotator, Ground);
		ATunaSweeperMissileTurret* Turret = GetWorld()->SpawnActorDeferred<ATunaSweeperMissileTurret>(
			MissileTurretClass, Transform, GetOwner(), Cast<APawn>(GetOwner()), ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
		if (!Turret) continue;
		Turret->InitializeTurret(GetOwner(), TargetActor);
		Turret->FinishSpawning(Transform);
		if (!IsValid(Turret) || Turret->IsActorBeingDestroyed()) continue;
		ATunaSweeperCombatPatternEffectActor::Spawn(GetWorld(), ETunaSweeperCombatPatternEffect::Summon,
			Ground, 75.0f, FVector::UpVector, Turret);
		Turrets.Add(Turret);
		return true;
	}
	return false;
}

void UTunaSweeperCombatPatternComponent::PulseMinionWarning()
{
	// One harmless cyan scanner cue at a time, even after a hitch or in a long teaching windup.
	if (IsValid(MinionWarningEffect)) MinionWarningEffect->Destroy();
	FVector Ground = GetOwner()->GetActorLocation();
	if (!FindGround(Ground, Ground))
	{
		float Radius = 0.0f, HalfHeight = 0.0f;
		GetOwner()->GetSimpleCollisionCylinder(Radius, HalfHeight);
		Ground.Z -= HalfHeight;
	}
	MinionWarningEffect = ATunaSweeperCombatPatternEffectActor::Spawn(GetWorld(),
		ETunaSweeperCombatPatternEffect::Summon, Ground,
		FMath::Max(60.0f, GetOwner()->GetSimpleCollisionRadius() * 1.6f), FVector::UpVector, GetOwner());
	MinionWarningPulseSeconds = 0.8f;
}

void UTunaSweeperCombatPatternComponent::ClearMinionWarning()
{
	if (IsValid(MinionWarningEffect)) MinionWarningEffect->Destroy();
	MinionWarningEffect = nullptr;
	MinionWarningDuration = 0.0f;
	MinionWarningPulseSeconds = 0.0f;
}

void UTunaSweeperCombatPatternComponent::SpawnNextMinion()
{
	const float Fraction = WaveSize <= 1 ? 0.5f : static_cast<float>(WaveSpawnIndex) / (WaveSize - 1);
	const float Angle = FMath::Lerp(-0.5f, 0.5f, Fraction) * FMath::Clamp(MinionFanAngle, 0.0f, 160.0f);
	++WaveSpawnIndex;
	const FVector Direction = LockedDirection.RotateAngleAxis(Angle, FVector::UpVector);
	const ATunaSweeperRollingRobotMinion* Defaults = RollingMinionClass->GetDefaultObject<ATunaSweeperRollingRobotMinion>();
	const float Radius = Defaults->GetRollRadius();
	FVector Ground;
	if (!FindGround(GetOwner()->GetActorLocation() + Direction * (GetOwner()->GetSimpleCollisionRadius() + Radius + 59.0f), Ground)) return;
	const FVector SpawnLocation = Ground + FVector(0, 0, Radius + 3.0f);
	FCollisionQueryParams PlacementQuery(SCENE_QUERY_STAT(CombatPatternMinionPlacement), false, GetOwner());
	FHitResult BlockedPlacement;
	if (GetWorld()->SweepSingleByChannel(BlockedPlacement, GetOwner()->GetActorLocation(), SpawnLocation,
		FQuat::Identity, ECC_Pawn, FCollisionShape::MakeSphere(Radius), PlacementQuery)) return;
	const FTransform Transform(Direction.Rotation(), SpawnLocation);
	ATunaSweeperRollingRobotMinion* Minion = GetWorld()->SpawnActorDeferred<ATunaSweeperRollingRobotMinion>(
		RollingMinionClass, Transform, GetOwner(), Cast<APawn>(GetOwner()), ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding);
	if (!Minion) return;
	if (const UTunaSweeperFactionSubsystem* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>())
	{
		Minion->GetFactionComponent()->SetFactionId(Factions->GetFactionIdForActor(GetOwner()));
	}
	Minion->InitializeRoll(Direction, Target.Get());
	Minion->FinishSpawning(Transform);
	if (IsValid(Minion) && !Minion->IsActorBeingDestroyed())
	{
		Minions.Add(Minion);
		ATunaSweeperCombatPatternEffectActor::Spawn(GetWorld(), ETunaSweeperCombatPatternEffect::Summon,
			Ground, Radius, Direction, Minion);
	}
}

void UTunaSweeperCombatPatternComponent::EnterRecovery()
{
	if (IsValid(Warning)) Warning->Destroy();
	Warning = nullptr;
	ClearMinionWarning();
	Phase = ETunaSweeperCombatPatternPhase::Recovery;
	PhaseSeconds = 0.0f;
}

void UTunaSweeperCombatPatternComponent::FinishPattern()
{
	if (IsValid(Warning)) Warning->Destroy();
	Warning = nullptr;
	ClearMinionWarning();
	RestoreOwnerMovement();
	Phase = ETunaSweeperCombatPatternPhase::Idle;
	Target.Reset();
	NextPatternTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, CooldownSeconds);
	RemoveExpiredSummons();
	SetComponentTickEnabled(!Turrets.IsEmpty() || !Minions.IsEmpty());
}

void UTunaSweeperCombatPatternComponent::RemoveExpiredSummons()
{
	Turrets.RemoveAll([](const TWeakObjectPtr<ATunaSweeperMissileTurret>& Actor) { return !Actor.IsValid() || Actor->IsActorBeingDestroyed(); });
	Minions.RemoveAll([](const TWeakObjectPtr<ATunaSweeperRollingRobotMinion>& Actor) { return !Actor.IsValid() || Actor->IsDead() || Actor->IsActorBeingDestroyed(); });
}

void UTunaSweeperCombatPatternComponent::CancelPatterns(bool bDestroySummons)
{
	if (IsValid(Warning)) Warning->Destroy();
	Warning = nullptr;
	ClearMinionWarning();
	RestoreOwnerMovement();
	Phase = ETunaSweeperCombatPatternPhase::Idle;
	Target.Reset();
	ChargeVictims.Reset();
	if (GetWorld()) NextPatternTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, CooldownSeconds);
	if (bDestroySummons)
	{
		for (auto& Turret : Turrets) if (Turret.IsValid()) Turret->Destroy();
		for (auto& Minion : Minions) if (Minion.IsValid()) Minion->Destroy();
		Turrets.Reset();
		Minions.Reset();
	}
	SetComponentTickEnabled(!Turrets.IsEmpty() || !Minions.IsEmpty());
}

void UTunaSweeperCombatPatternComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelPatterns();
	Super::EndPlay(EndPlayReason);
}
