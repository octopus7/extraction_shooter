// Fill out your copyright notice in the Description page of Project Settings.

#include "QuadrupedComponent.h"

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

	UpdateLegTargets();
	ProcessGaitCycle(DeltaTime);
	InterpolateLegPositions(DeltaTime);

	if (bDrawDebug)
	{
		for (int32 i = 0; i < Legs.Num(); i++)
		{
			const FColor Color = Legs[i].bIsMoving ? FColor::Yellow : FColor::Green;
			DrawDebugSphere(GetWorld(), GetFootPosition(i), 5.0f, 8, Color, false, -1.0f, 0, 1.0f);
			DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetFootPosition(i), FColor::White, false, -1.0f, 0, 0.5f);
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
	UpdateLegTargets();
	for (FQuadrupedLegData& Leg : Legs)
	{
		Leg.CurrentPosition = Leg.TargetPosition;
		Leg.StepStartPosition = Leg.CurrentPosition;
		Leg.StepEndPosition = Leg.CurrentPosition;
		Leg.StepProgress = 0.0f;
		Leg.bIsMoving = false;
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

void UQuadrupedComponent::UpdateLegTargets()
{
	for (int32 i = 0; i < Legs.Num(); i++)
	{
		const FVector IdealPosition = CalculateIdealFootPosition(i);
		FVector GroundPosition;

		if (TraceGround(IdealPosition, GroundPosition))
		{
			Legs[i].TargetPosition = GroundPosition;
		}
		else
		{
			Legs[i].TargetPosition = IdealPosition;
		}
	}
}

void UQuadrupedComponent::ProcessGaitCycle(float DeltaTime)
{
	for (int32 i = 0; i < Legs.Num(); i++)
	{
		FQuadrupedLegData& Leg = Legs[i];

		if (Leg.bIsMoving)
		{
			continue;
		}

		const float Distance = FVector::Dist(Leg.CurrentPosition, Leg.TargetPosition);

		if (Distance > StepThreshold)
		{
			const int32 OtherGroup = (Leg.GaitGroup == 0) ? 1 : 0;

			if (!IsGaitGroupStepping(OtherGroup))
			{
				for (FQuadrupedLegData& GroupLeg : Legs)
				{
					if (GroupLeg.GaitGroup == Leg.GaitGroup)
					{
						const float GroupDistance = FVector::Dist(GroupLeg.CurrentPosition, GroupLeg.TargetPosition);
						if (GroupDistance > StepThreshold * 0.5f)
						{
							GroupLeg.StepStartPosition = GroupLeg.CurrentPosition;
							GroupLeg.StepEndPosition = GroupLeg.TargetPosition;
							GroupLeg.bIsMoving = true;
							GroupLeg.StepProgress = 0.0f;
						}
					}
				}
				break;
			}
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
			Leg.StepProgress += DeltaTime * StepSpeed;

			if (Leg.StepProgress >= 1.0f)
			{
				Leg.CurrentPosition = Leg.StepEndPosition;
				Leg.StepProgress = 0.0f;
				Leg.bIsMoving = false;
			}
			else
			{
				const float Alpha = FMath::Clamp(Leg.StepProgress, 0.0f, 1.0f);
				Leg.CurrentPosition = FMath::Lerp(Leg.StepStartPosition, Leg.StepEndPosition, Alpha);
			}
		}
	}
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

bool UQuadrupedComponent::TraceGround(const FVector& WorldLocation, FVector& OutGroundPosition) const
{
	if (!GetWorld() || !GetOwner())
	{
		return false;
	}

	const FVector Start = WorldLocation + FVector(0.0f, 0.0f, GroundCheckStartOffset);
	const FVector End = WorldLocation - FVector(0.0f, 0.0f, GroundCheckDistance);

	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
	{
		OutGroundPosition = HitResult.Location;
		return true;
	}

	return false;
}

FVector UQuadrupedComponent::CalculateIdealFootPosition(int32 LegIndex) const
{
	if (!Legs.IsValidIndex(LegIndex) || !GetOwner())
	{
		return FVector::ZeroVector;
	}

	const FQuadrupedLegData& Leg = Legs[LegIndex];
	const FTransform OwnerTransform = GetOwner()->GetActorTransform();
	const FVector WorldOffset = OwnerTransform.TransformVector(Leg.DefaultOffset);
	return GetOwner()->GetActorLocation() + WorldOffset;
}
