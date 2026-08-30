// Fill out your copyright notice in the Description page of Project Settings.

#include "QuadrupedComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "QuadrupedCharacter.h"

DEFINE_LOG_CATEGORY_STATIC(LogQuadrupedDiagnostics, Log, All);

namespace
{
enum class EQuadrupedTargetDiagnosticState : uint8
{
	Valid,
	NoGroundHit,
	GroundTooSteep,
	OutOfReach
};

const TCHAR* LexToString(EQuadrupedTargetDiagnosticState State)
{
	switch (State)
	{
	case EQuadrupedTargetDiagnosticState::Valid:
		return TEXT("Valid");
	case EQuadrupedTargetDiagnosticState::NoGroundHit:
		return TEXT("NoGroundHit");
	case EQuadrupedTargetDiagnosticState::GroundTooSteep:
		return TEXT("GroundTooSteep");
	case EQuadrupedTargetDiagnosticState::OutOfReach:
		return TEXT("OutOfReach");
	default:
		return TEXT("Unknown");
	}
}
}

UQuadrupedComponent::UQuadrupedComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UQuadrupedComponent::BeginPlay()
{
	Super::BeginPlay();
	LogDiagnosticConfiguration();

	if (Legs.Num() == 0)
	{
		if (const AQuadrupedCharacter* QuadrupedCharacter = Cast<AQuadrupedCharacter>(GetOwner()))
		{
			InitializeDefaultLegs(QuadrupedCharacter->BodyLength, QuadrupedCharacter->BodyWidth);
		}
		else
		{
			InitializeDefaultLegs();
		}
	}

	ResetLegPositionsToTargets();
}

void UQuadrupedComponent::OnRegister()
{
	Super::OnRegister();

	if (Legs.Num() == 0)
	{
		if (const AQuadrupedCharacter* QuadrupedCharacter = Cast<AQuadrupedCharacter>(GetOwner()))
		{
			InitializeDefaultLegs(QuadrupedCharacter->BodyLength, QuadrupedCharacter->BodyWidth);
		}
		else
		{
			InitializeDefaultLegs();
		}
	}

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			AddTickPrerequisiteComponent(Movement);
		}

	}

	TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshes(GetOwner());
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		SkeletalMesh->AddTickPrerequisiteComponent(this);
	}

#if WITH_EDITOR
	if (GetWorld() && !GetWorld()->IsGameWorld())
	{
		RefreshLegsInEditor();
	}
#endif
}

void UQuadrupedComponent::OnUnregister()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (UCharacterMovementComponent* Movement = Character->GetCharacterMovement())
		{
			RemoveTickPrerequisiteComponent(Movement);
		}

	}

	TInlineComponentArray<USkeletalMeshComponent*> SkeletalMeshes(GetOwner());
	for (USkeletalMeshComponent* SkeletalMesh : SkeletalMeshes)
	{
		SkeletalMesh->RemoveTickPrerequisiteComponent(this);
	}

	Super::OnUnregister();
}

#if WITH_EDITOR
void UQuadrupedComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RefreshLegsInEditor();
}

void UQuadrupedComponent::RefreshLegsInEditor()
{
	ResetLegPositionsToTargets();
}
#endif

void UQuadrupedComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateMotionPrediction(DeltaTime);
	UpdatePlantedSupportPositions();
	UpdateLegTargets();
	ProcessGaitCycle(DeltaTime);
	InterpolateLegPositions(DeltaTime);

	if (bDrawDebug)
	{
		for (int32 i = 0; i < Legs.Num(); i++)
		{
			const FColor Color = Legs[i].bIsMoving ? FColor::Yellow : FColor::Green;
			DrawDebugSphere(GetWorld(), GetFootPosition(i), 5.0f, 8, Color, false, -1.0f, 0, 1.0f);
			const FColor TargetColor = Legs[i].bHasValidGroundTarget ? FColor::Cyan : FColor::Red;
			DrawDebugSphere(GetWorld(), Legs[i].TargetPosition, 4.0f, 8, TargetColor, false, -1.0f, 0, 1.0f);
			DrawDebugLine(GetWorld(), GetFootPosition(i), Legs[i].TargetPosition, TargetColor, false, -1.0f, 0, 0.5f);
			DrawDebugDirectionalArrow(
				GetWorld(),
				Legs[i].TargetPosition,
				Legs[i].TargetPosition + Legs[i].TargetSurfaceNormal * 20.0f,
				5.0f,
				TargetColor,
				false,
				-1.0f,
				0,
				1.0f);
		}
	}
}

void UQuadrupedComponent::InitializeDefaultLegs(float BodyLength, float BodyWidth)
{
	Legs.Empty();
	Legs.SetNum(4);

	Legs[0].DefaultOffset = FVector(BodyLength, -BodyWidth, 0.0f);
	Legs[0].GaitGroup = 0;

	Legs[1].DefaultOffset = FVector(BodyLength, BodyWidth, 0.0f);
	Legs[1].GaitGroup = 1;

	Legs[2].DefaultOffset = FVector(-BodyLength, -BodyWidth, 0.0f);
	Legs[2].GaitGroup = 1;

	Legs[3].DefaultOffset = FVector(-BodyLength, BodyWidth, 0.0f);
	Legs[3].GaitGroup = 0;

	if (GetOwner())
	{
		ResetLegPositionsToTargets();
	}
}

void UQuadrupedComponent::ResetLegPositionsToTargets()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	// Runtime foot positions are world-space values. Never use values inherited from a CDO,
	// duplicated PIE actor, saved map instance, or an earlier placement to bootstrap a new owner.
	for (FQuadrupedLegData& Leg : Legs)
	{
		Leg.CurrentPosition = FVector::ZeroVector;
		Leg.TargetPosition = FVector::ZeroVector;
		Leg.CurrentSurfaceNormal = FVector::UpVector;
		Leg.TargetSurfaceNormal = FVector::UpVector;
		Leg.bHasValidGroundTarget = false;
		Leg.PlacementScore = 0.0f;
		Leg.Phase = EQuadrupedFootPhase::Planted;
		Leg.StepStartPosition = FVector::ZeroVector;
		Leg.StepEndPosition = FVector::ZeroVector;
		Leg.StepEndSurfaceNormal = FVector::UpVector;
		Leg.StepProgress = 0.0f;
		Leg.bIsMoving = false;
		Leg.SupportComponent.Reset();
		Leg.SupportRelativePosition = FVector::ZeroVector;
		Leg.TargetSupportComponent.Reset();
		Leg.TargetSupportRelativePosition = FVector::ZeroVector;
		Leg.StepEndSupportComponent.Reset();
		Leg.StepEndSupportRelativePosition = FVector::ZeroVector;
	}

	const FTransform OwnerTransform = Owner->GetActorTransform();
	const float OwnerYaw = OwnerTransform.Rotator().Yaw;
	PredictedOwnerTransform = FTransform(
		FRotator(0.0f, OwnerYaw, 0.0f),
		OwnerTransform.GetLocation(),
		OwnerTransform.GetScale3D());
	PreviousOwnerYaw = OwnerYaw;
	bHasPreviousOwnerYaw = true;
	LastDiagnosticTargetStates.Reset();

	// Initial acquisition must probe the ideal locations directly. MaxStepDistance is a gait
	// constraint and must only apply after a world-space planted position has been established.
	UpdateLegTargets(false);
	for (FQuadrupedLegData& Leg : Legs)
	{
		Leg.CurrentPosition = Leg.TargetPosition;
		Leg.CurrentSurfaceNormal = Leg.TargetSurfaceNormal;
		Leg.StepStartPosition = Leg.CurrentPosition;
		Leg.StepEndPosition = Leg.CurrentPosition;
		Leg.StepEndSurfaceNormal = Leg.CurrentSurfaceNormal;
		Leg.StepProgress = 0.0f;
		Leg.bIsMoving = false;
		Leg.Phase = EQuadrupedFootPhase::Planted;
		Leg.PlacementScore = 0.0f;
		Leg.SupportComponent = Leg.TargetSupportComponent;
		Leg.SupportRelativePosition = Leg.TargetSupportRelativePosition;
	}
}

FVector UQuadrupedComponent::GetFootPosition(int32 LegIndex) const
{
	if (!Legs.IsValidIndex(LegIndex))
	{
		return FVector::ZeroVector;
	}

	const FQuadrupedLegData& Leg = Legs[LegIndex];

	if (Leg.bIsMoving && Leg.StepProgress > 0.0f)
	{
		const float HeightFactor = 4.0f * Leg.StepProgress * (1.0f - Leg.StepProgress);
		FVector Position = Leg.CurrentPosition;
		Position.Z += StepHeight * HeightFactor;
		return Position;
	}

	return Leg.CurrentPosition;
}

FVector UQuadrupedComponent::GetNextFootPosition(int32 LegIndex) const
{
	return Legs.IsValidIndex(LegIndex) ? Legs[LegIndex].TargetPosition : FVector::ZeroVector;
}

bool UQuadrupedComponent::HasValidNextFootPosition(int32 LegIndex) const
{
	return Legs.IsValidIndex(LegIndex) && Legs[LegIndex].bHasValidGroundTarget;
}

void UQuadrupedComponent::UpdateMotionPrediction(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		PredictedOwnerTransform = FTransform::Identity;
		return;
	}

	const FTransform OwnerTransform = Owner->GetActorTransform();
	const float CurrentYaw = OwnerTransform.Rotator().Yaw;
	float ObservedYawSpeed = 0.0f;
	if (bHasPreviousOwnerYaw && DeltaTime > UE_SMALL_NUMBER)
	{
		ObservedYawSpeed = FMath::FindDeltaAngleDegrees(PreviousOwnerYaw, CurrentYaw) / DeltaTime;
		ObservedYawSpeed = FMath::Clamp(ObservedYawSpeed, -MaxPredictedYawSpeed, MaxPredictedYawSpeed);
	}

	FVector HorizontalVelocity = Owner->GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	const FVector PredictedLocation = OwnerTransform.GetLocation() + HorizontalVelocity * LookAheadSeconds;
	const FRotator PredictedRotation(0.0f, CurrentYaw + ObservedYawSpeed * LookAheadSeconds, 0.0f);
	PredictedOwnerTransform = FTransform(PredictedRotation, PredictedLocation, OwnerTransform.GetScale3D());

	PreviousOwnerYaw = CurrentYaw;
	bHasPreviousOwnerYaw = true;
}

void UQuadrupedComponent::UpdatePlantedSupportPositions()
{
	for (FQuadrupedLegData& Leg : Legs)
	{
		if (!Leg.bIsMoving && Leg.SupportComponent.IsValid())
		{
			Leg.CurrentPosition = Leg.SupportComponent->GetComponentTransform().TransformPosition(Leg.SupportRelativePosition);
		}
	}
}

void UQuadrupedComponent::UpdateLegTargets(bool bLimitStepDistance)
{
	for (int32 i = 0; i < Legs.Num(); i++)
	{
		FQuadrupedLegData& Leg = Legs[i];
		const FVector IdealPosition = CalculateIdealFootPosition(i);
		FVector ProbePosition = IdealPosition;
		FVector PlanarStep = ProbePosition - Leg.CurrentPosition;
		PlanarStep.Z = 0.0f;
		if (bLimitStepDistance &&
			!Leg.CurrentPosition.IsNearlyZero() &&
			PlanarStep.SizeSquared() > FMath::Square(MaxStepDistance))
		{
			ProbePosition = Leg.CurrentPosition + PlanarStep.GetSafeNormal() * MaxStepDistance;
			ProbePosition.Z = IdealPosition.Z;
		}

		FHitResult GroundHit;
		const bool bHasGroundHit = TraceGround(ProbePosition, GroundHit);
		const bool bHasWalkableNormal = bHasGroundHit && GroundHit.ImpactNormal.Z >= MinGroundNormalZ;
		const bool bIsReachable = bHasWalkableNormal && IsCandidateReachable(Leg, GroundHit.ImpactPoint);
		Leg.bHasValidGroundTarget = bHasGroundHit && bHasWalkableNormal && bIsReachable;

		EQuadrupedTargetDiagnosticState DiagnosticState = EQuadrupedTargetDiagnosticState::Valid;
		if (!bHasGroundHit)
		{
			DiagnosticState = EQuadrupedTargetDiagnosticState::NoGroundHit;
		}
		else if (!bHasWalkableNormal)
		{
			DiagnosticState = EQuadrupedTargetDiagnosticState::GroundTooSteep;
		}
		else if (!bIsReachable)
		{
			DiagnosticState = EQuadrupedTargetDiagnosticState::OutOfReach;
		}

		const FVector PredictedHipPosition = PredictedOwnerTransform.TransformPosition(
			FVector(Leg.DefaultOffset.X, Leg.DefaultOffset.Y, 0.0f));
		const float ReachDistance = bHasGroundHit
			? FVector::Distance(PredictedHipPosition, GroundHit.ImpactPoint)
			: -1.0f;
		LogTargetDiagnostic(
			i,
			static_cast<uint8>(DiagnosticState),
			IdealPosition,
			ProbePosition,
			GroundHit,
			ReachDistance);

		if (Leg.bHasValidGroundTarget)
		{
			Leg.TargetPosition = GroundHit.ImpactPoint;
			Leg.TargetSurfaceNormal = GroundHit.ImpactNormal.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
			Leg.TargetSupportComponent = GroundHit.GetComponent();
			Leg.TargetSupportRelativePosition = Leg.TargetSupportComponent.IsValid()
				? Leg.TargetSupportComponent->GetComponentTransform().InverseTransformPosition(Leg.TargetPosition)
				: FVector::ZeroVector;
		}
		else
		{
			Leg.TargetPosition = ProbePosition;
			Leg.TargetSurfaceNormal = FVector::UpVector;
			Leg.TargetSupportComponent.Reset();
			Leg.TargetSupportRelativePosition = FVector::ZeroVector;
		}

		Leg.PlacementScore = CalculatePlacementError(Leg) / FMath::Max(StepThreshold, 1.0f);
	}
}

void UQuadrupedComponent::LogDiagnosticConfiguration() const
{
#if !UE_BUILD_SHIPPING
	if (!bLogDiagnostics || !GetOwner() || !GetWorld() || !GetWorld()->IsGameWorld())
	{
		return;
	}

	UE_LOG(
		LogQuadrupedDiagnostics,
		Display,
		TEXT("QuadrupedConfig Actor=%s Class=%s WorldType=%d Location=%s Velocity=%s Legs=%d LookAhead=%.3f StepThreshold=%.1f StepDuration=%.3f MaxStep=%.1f MaxReach=%.1f GroundStart=%.1f GroundDistance=%.1f ProbeRadius=%.1f MinNormalZ=%.2f TraceChannel=%d PairedGait=%s"),
		*GetOwner()->GetName(),
		*GetOwner()->GetClass()->GetPathName(),
		static_cast<int32>(GetWorld()->WorldType),
		*GetOwner()->GetActorLocation().ToCompactString(),
		*GetOwner()->GetVelocity().ToCompactString(),
		Legs.Num(),
		LookAheadSeconds,
		StepThreshold,
		StepDuration,
		MaxStepDistance,
		MaxLegReach,
		GroundCheckStartOffset,
		GroundCheckDistance,
		GroundProbeRadius,
		MinGroundNormalZ,
		static_cast<int32>(GroundTraceChannel.GetValue()),
		bMoveGaitGroupTogether ? TEXT("true") : TEXT("false"));
#endif
}

void UQuadrupedComponent::LogTargetDiagnostic(
	int32 LegIndex,
	uint8 TargetState,
	const FVector& IdealPosition,
	const FVector& ProbePosition,
	const FHitResult& GroundHit,
	float ReachDistance)
{
#if !UE_BUILD_SHIPPING
	if (!bLogDiagnostics || !GetOwner() || !GetWorld() || !GetWorld()->IsGameWorld() ||
		!Legs.IsValidIndex(LegIndex))
	{
		return;
	}

	if (LastDiagnosticTargetStates.Num() != Legs.Num())
	{
		LastDiagnosticTargetStates.Init(MAX_uint8, Legs.Num());
	}

	if (LastDiagnosticTargetStates[LegIndex] == TargetState)
	{
		return;
	}
	LastDiagnosticTargetStates[LegIndex] = TargetState;

	const FQuadrupedLegData& Leg = Legs[LegIndex];
	const EQuadrupedTargetDiagnosticState State =
		static_cast<EQuadrupedTargetDiagnosticState>(TargetState);
	const FString Message = FString::Printf(
		TEXT("QuadrupedTarget Actor=%s Class=%s WorldType=%d Leg=%d State=%s ActorLocation=%s Velocity=%s Offset=%s Current=%s Ideal=%s Probe=%s Hit=%s HitPoint=%s HitNormal=%s ReachDistance=%.1f MaxReach=%.1f Valid=%s"),
		*GetOwner()->GetName(),
		*GetOwner()->GetClass()->GetPathName(),
		static_cast<int32>(GetWorld()->WorldType),
		LegIndex,
		LexToString(State),
		*GetOwner()->GetActorLocation().ToCompactString(),
		*GetOwner()->GetVelocity().ToCompactString(),
		*Leg.DefaultOffset.ToCompactString(),
		*Leg.CurrentPosition.ToCompactString(),
		*IdealPosition.ToCompactString(),
		*ProbePosition.ToCompactString(),
		GroundHit.bBlockingHit ? TEXT("true") : TEXT("false"),
		*GroundHit.ImpactPoint.ToCompactString(),
		*GroundHit.ImpactNormal.ToCompactString(),
		ReachDistance,
		MaxLegReach,
		Leg.bHasValidGroundTarget ? TEXT("true") : TEXT("false"));
	if (State == EQuadrupedTargetDiagnosticState::Valid)
	{
		UE_LOG(LogQuadrupedDiagnostics, Display, TEXT("%s"), *Message);
	}
	else
	{
		UE_LOG(LogQuadrupedDiagnostics, Warning, TEXT("%s"), *Message);
	}
#endif
}

void UQuadrupedComponent::ProcessGaitCycle(float DeltaTime)
{
	if (Legs.ContainsByPredicate([](const FQuadrupedLegData& Leg) { return Leg.bIsMoving; }))
	{
		return;
	}

	int32 BestLegIndex = INDEX_NONE;
	float BestScore = 1.0f;
	for (int32 LegIndex = 0; LegIndex < Legs.Num(); ++LegIndex)
	{
		const FQuadrupedLegData& Leg = Legs[LegIndex];
		if (Leg.bHasValidGroundTarget && Leg.PlacementScore > BestScore)
		{
			BestScore = Leg.PlacementScore;
			BestLegIndex = LegIndex;
		}
	}

	if (BestLegIndex == INDEX_NONE)
	{
		return;
	}

	const int32 SelectedGroup = Legs[BestLegIndex].GaitGroup;
	StartStep(Legs[BestLegIndex]);
	if (!bMoveGaitGroupTogether)
	{
		return;
	}

	for (int32 LegIndex = 0; LegIndex < Legs.Num(); ++LegIndex)
	{
		if (LegIndex == BestLegIndex)
		{
			continue;
		}

		FQuadrupedLegData& GroupLeg = Legs[LegIndex];
		if (GroupLeg.GaitGroup == SelectedGroup &&
			GroupLeg.bHasValidGroundTarget &&
			GroupLeg.PlacementScore > GroupStepThresholdScale)
		{
			StartStep(GroupLeg);
		}
	}
}

void UQuadrupedComponent::InterpolateLegPositions(float DeltaTime)
{
	const float StepSpeed = 1.0f / FMath::Max(StepDuration, 0.01f);

	for (FQuadrupedLegData& Leg : Legs)
	{
		if (Leg.bIsMoving)
		{
			if (Leg.StepEndSupportComponent.IsValid())
			{
				Leg.StepEndPosition = Leg.StepEndSupportComponent->GetComponentTransform().TransformPosition(
					Leg.StepEndSupportRelativePosition);
			}

			Leg.StepProgress += DeltaTime * StepSpeed;

			if (Leg.StepProgress >= 1.0f)
			{
				FinishStep(Leg);
			}
			else
			{
				const float Alpha = FMath::Clamp(Leg.StepProgress, 0.0f, 1.0f);
				const float SmoothedAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
				Leg.CurrentPosition = FMath::Lerp(Leg.StepStartPosition, Leg.StepEndPosition, SmoothedAlpha);
			}
		}
	}
}

void UQuadrupedComponent::StartStep(FQuadrupedLegData& Leg)
{
	Leg.StepStartPosition = Leg.CurrentPosition;
	Leg.StepEndPosition = Leg.TargetPosition;
	Leg.StepEndSurfaceNormal = Leg.TargetSurfaceNormal;
	Leg.StepEndSupportComponent = Leg.TargetSupportComponent;
	Leg.StepEndSupportRelativePosition = Leg.TargetSupportRelativePosition;
	Leg.SupportComponent.Reset();
	Leg.bIsMoving = true;
	Leg.Phase = EQuadrupedFootPhase::Swinging;
	Leg.StepProgress = 0.0f;
}

void UQuadrupedComponent::FinishStep(FQuadrupedLegData& Leg)
{
	Leg.CurrentPosition = Leg.StepEndPosition;
	Leg.CurrentSurfaceNormal = Leg.StepEndSurfaceNormal;
	Leg.SupportComponent = Leg.StepEndSupportComponent;
	Leg.SupportRelativePosition = Leg.StepEndSupportRelativePosition;
	Leg.StepProgress = 0.0f;
	Leg.bIsMoving = false;
	Leg.Phase = EQuadrupedFootPhase::Planted;
}

bool UQuadrupedComponent::IsGaitGroupStepping(int32 GroupIndex) const
{
	for (const FQuadrupedLegData& Leg : Legs)
	{
		if (Leg.GaitGroup == GroupIndex && Leg.bIsMoving)
		{
			return true;
		}
	}
	return false;
}

bool UQuadrupedComponent::TraceGround(const FVector& WorldLocation, FHitResult& OutHit) const
{
	if (!GetWorld() || !GetOwner())
	{
		return false;
	}

	const FVector Start = WorldLocation + FVector(0.0f, 0.0f, GroundCheckStartOffset);
	const FVector End = WorldLocation - FVector(0.0f, 0.0f, GroundCheckDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = false;

	if (GroundProbeRadius > UE_SMALL_NUMBER)
	{
		return GetWorld()->SweepSingleByChannel(
			OutHit,
			Start,
			End,
			FQuat::Identity,
			GroundTraceChannel,
			FCollisionShape::MakeSphere(GroundProbeRadius),
			QueryParams);
	}

	return GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, GroundTraceChannel, QueryParams);
}

bool UQuadrupedComponent::IsCandidateReachable(
	const FQuadrupedLegData& Leg,
	const FVector& CandidatePosition) const
{
	const FVector PredictedHipPosition = PredictedOwnerTransform.TransformPosition(
		FVector(Leg.DefaultOffset.X, Leg.DefaultOffset.Y, 0.0f));
	return FVector::DistSquared(PredictedHipPosition, CandidatePosition) <= FMath::Square(MaxLegReach);
}

float UQuadrupedComponent::CalculatePlacementError(const FQuadrupedLegData& Leg) const
{
	const FVector Delta = Leg.TargetPosition - Leg.CurrentPosition;
	const float PlanarErrorSquared = FMath::Square(Delta.X) + FMath::Square(Delta.Y);
	const float WeightedVerticalErrorSquared = FMath::Square(Delta.Z * 0.35f);
	return FMath::Sqrt(PlanarErrorSquared + WeightedVerticalErrorSquared);
}

FVector UQuadrupedComponent::CalculateIdealFootPosition(int32 LegIndex) const
{
	if (!Legs.IsValidIndex(LegIndex) || !GetOwner())
	{
		return FVector::ZeroVector;
	}

	const FQuadrupedLegData& Leg = Legs[LegIndex];
	return PredictedOwnerTransform.TransformPosition(Leg.DefaultOffset);
}
