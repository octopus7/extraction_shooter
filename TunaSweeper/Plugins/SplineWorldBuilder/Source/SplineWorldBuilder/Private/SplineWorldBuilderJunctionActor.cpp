#include "SplineWorldBuilderJunctionActor.h"

#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "SplineWorldBuilderActor.h"
#include "SplineWorldBuilderProfile.h"

namespace
{
	FVector FlatSafeNormal(const FVector& Direction)
	{
		return FVector(Direction.X, Direction.Y, 0.0).GetSafeNormal();
	}

	double DirectionYaw(const FVector& Direction)
	{
		const FVector Flat = FlatSafeNormal(Direction);
		return Flat.IsNearlyZero() ? 0.0 : Flat.Rotation().Yaw;
	}
}

ASplineWorldJunctionActor::ASplineWorldJunctionActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	JunctionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("JunctionMesh"));
	JunctionMesh->SetupAttachment(SceneRoot);
	JunctionMesh->SetMobility(EComponentMobility::Movable);
}

void ASplineWorldJunctionActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bAutoRebuild)
	{
		RebuildGenerated();
	}
}

UStaticMesh* ASplineWorldJunctionActor::GetDisplayedJunctionMesh() const
{
	return JunctionMesh ? JunctionMesh->GetStaticMesh() : nullptr;
}

bool ASplineWorldJunctionActor::GetConnectionDirection(
	const FSplineWorldJunctionConnection& Connection,
	FVector& OutDirection) const
{
	OutDirection = FVector::ZeroVector;
	if (!Connection.Chain || !Connection.Chain->GetBuilderSpline())
	{
		return false;
	}

	USplineComponent* ChainSpline = Connection.Chain->GetBuilderSpline();
	const double Distance = Connection.Endpoint == ESplineWorldEndpoint::Start
		? 0.0
		: ChainSpline->GetSplineLength();
	OutDirection = ChainSpline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
	if (Connection.Endpoint == ESplineWorldEndpoint::End)
	{
		OutDirection *= -1.0;
	}
	OutDirection = FlatSafeNormal(OutDirection);
	return !OutDirection.IsNearlyZero();
}

FRotator ASplineWorldJunctionActor::ResolveRotation(
	const ESplineWorldJunctionType Type,
	const TArray<FVector>& Directions) const
{
	if (Directions.IsEmpty())
	{
		return FRotator::ZeroRotator;
	}

	double Yaw = DirectionYaw(Directions[0]);
	if (Type == ESplineWorldJunctionType::Corner && Directions.Num() >= 2)
	{
		FVector ArmA = Directions[0];
		FVector ArmB = Directions[1];
		if (FVector::CrossProduct(ArmA, ArmB).Z < 0.0)
		{
			Swap(ArmA, ArmB);
		}
		Yaw = DirectionYaw(ArmA);
	}
	else if (Type == ESplineWorldJunctionType::Tee && Directions.Num() >= 3)
	{
		int32 PairA = 0;
		int32 PairB = 1;
		double MostOpposedDot = 1.0;
		for (int32 A = 0; A < Directions.Num(); ++A)
		{
			for (int32 B = A + 1; B < Directions.Num(); ++B)
			{
				const double Dot = FVector::DotProduct(Directions[A], Directions[B]);
				if (Dot < MostOpposedDot)
				{
					MostOpposedDot = Dot;
					PairA = A;
					PairB = B;
				}
			}
		}

		int32 BranchIndex = 0;
		while (BranchIndex == PairA || BranchIndex == PairB)
		{
			++BranchIndex;
		}
		// Test T mesh uses local +/-X for the through road and local +Y for the branch.
		Yaw = DirectionYaw(Directions[BranchIndex]) - 90.0;
	}

	return FRotator(0.0, Yaw, 0.0);
}

void ASplineWorldJunctionActor::RebuildGenerated()
{
	ResolvedJunctionType = ESplineWorldJunctionType::None;
	if (!JunctionMesh)
	{
		return;
	}

	JunctionMesh->SetStaticMesh(nullptr);
	JunctionMesh->EmptyOverrideMaterials();
	JunctionMesh->SetVisibility(false, true);
	if (!Profile)
	{
		return;
	}

	TArray<FVector> Directions;
	for (const FSplineWorldJunctionConnection& Connection : Connections)
	{
		FVector Direction;
		if (GetConnectionDirection(Connection, Direction))
		{
			Directions.Add(Direction);
		}
	}

	UStaticMesh* Mesh = nullptr;
	switch (Directions.Num())
	{
	case 0:
		ResolvedJunctionType = ESplineWorldJunctionType::None;
		break;
	case 1:
		ResolvedJunctionType = ESplineWorldJunctionType::End;
		Mesh = Profile->EndMesh;
		break;
	case 2:
	{
		const double Dot = FVector::DotProduct(Directions[0], Directions[1]);
		const double StraightThreshold = -FMath::Cos(FMath::DegreesToRadians(Profile->StraightJunctionTolerance));
		ResolvedJunctionType = Dot <= StraightThreshold
			? ESplineWorldJunctionType::Straight
			: ESplineWorldJunctionType::Corner;
		Mesh = ResolvedJunctionType == ESplineWorldJunctionType::Straight
			? Profile->StraightMesh.Get()
			: Profile->CornerMesh.Get();
		break;
	}
	case 3:
		ResolvedJunctionType = ESplineWorldJunctionType::Tee;
		Mesh = Profile->TJunctionMesh;
		break;
	case 4:
		ResolvedJunctionType = ESplineWorldJunctionType::Cross;
		Mesh = Profile->CrossJunctionMesh;
		break;
	default:
		ResolvedJunctionType = ESplineWorldJunctionType::Unsupported;
		Mesh = Profile->CrossJunctionMesh;
		break;
	}

	JunctionMesh->SetStaticMesh(Mesh);
	if (Profile->MaterialOverride)
	{
		JunctionMesh->SetMaterial(0, Profile->MaterialOverride);
	}
	JunctionMesh->SetRelativeRotation(ResolveRotation(ResolvedJunctionType, Directions));
	JunctionMesh->SetVisibility(Mesh != nullptr, true);
}
