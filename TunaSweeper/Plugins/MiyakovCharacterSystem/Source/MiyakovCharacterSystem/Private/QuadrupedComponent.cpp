// Fill out your copyright notice in the Description page of Project Settings.

#include "QuadrupedComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "QuadrupedCharacter.h"

UQuadrupedComponent::UQuadrupedComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UQuadrupedComponent::BeginPlay()
{
	Super::BeginPlay();

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
	UpdateMotionPrediction(0.0f);
	UpdateLegTargets();
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

void UQuadrupedComponent::UpdateLegTargets()
{
	for (int32 i = 0; i < Legs.Num(); i++)
	{
		FQuadrupedLegData& Leg = Legs[i];
		FVector ProbePosition = CalculateIdealFootPosition(i);
		FVector PlanarStep = ProbePosition - Leg.CurrentPosition;
		PlanarStep.Z = 0.0f;
		if (!Leg.CurrentPosition.IsNearlyZero() && PlanarStep.SizeSquared() > FMath::Square(MaxStepDistance))
		{
			ProbePosition = Leg.CurrentPosition + PlanarStep.GetSafeNormal() * MaxStepDistance;
			ProbePosition.Z = CalculateIdealFootPosition(i).Z;
		}

		FHitResult GroundHit;
		Leg.bHasValidGroundTarget =
			TraceGround(ProbePosition, GroundHit) &&
			GroundHit.ImpactNormal.Z >= MinGroundNormalZ &&
			IsCandidateReachable(Leg, GroundHit.ImpactPoint);

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
