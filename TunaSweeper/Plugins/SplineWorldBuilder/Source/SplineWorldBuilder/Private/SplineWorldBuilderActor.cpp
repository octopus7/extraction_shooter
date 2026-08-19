#include "SplineWorldBuilderActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Materials/MaterialInterface.h"
#include "SplineWorldBuilderJunctionActor.h"
#include "SplineWorldBuilderProfile.h"

namespace
{
	FVector FlatDirection(const FVector& Direction)
	{
		return FVector(Direction.X, Direction.Y, 0.0).GetSafeNormal();
	}

	FRotator YawRotation(const FVector& Direction)
	{
		const FVector Flat = FlatDirection(Direction);
		return Flat.IsNearlyZero() ? FRotator::ZeroRotator : FRotator(0.0, Flat.Rotation().Yaw, 0.0);
	}
}

ASplineWorldBuilderActor::ASplineWorldBuilderActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetupAttachment(SceneRoot);
	Spline->SetClosedLoop(false);

	StraightInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("StraightInstances"));
	StraightInstances->SetupAttachment(SceneRoot);

	CornerInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CornerInstances"));
	CornerInstances->SetupAttachment(SceneRoot);

	EndInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("EndInstances"));
	EndInstances->SetupAttachment(SceneRoot);

	// The actor and spline remain freely editable. A later explicit bake may convert
	// the generated output to static components, but preview components must share
	// the movable root's mobility or Unreal will reject their attachment.
	StraightInstances->SetMobility(EComponentMobility::Movable);
	CornerInstances->SetMobility(EComponentMobility::Movable);
	EndInstances->SetMobility(EComponentMobility::Movable);
}

void ASplineWorldBuilderActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bAutoRebuild)
	{
		RebuildGenerated();
	}
}

int32 ASplineWorldBuilderActor::GetStraightInstanceCount() const
{
	return StraightInstances ? StraightInstances->GetInstanceCount() : 0;
}

int32 ASplineWorldBuilderActor::GetCornerInstanceCount() const
{
	return CornerInstances ? CornerInstances->GetInstanceCount() : 0;
}

int32 ASplineWorldBuilderActor::GetEndInstanceCount() const
{
	return EndInstances ? EndInstances->GetInstanceCount() : 0;
}

bool ASplineWorldBuilderActor::GetStraightInstanceTransform(
	const int32 InstanceIndex,
	FTransform& OutTransform,
	const bool bWorldSpace) const
{
	return StraightInstances && StraightInstances->GetInstanceTransform(InstanceIndex, OutTransform, bWorldSpace);
}

void ASplineWorldBuilderActor::ConfigureGeneratedComponent(
	UHierarchicalInstancedStaticMeshComponent* Component,
	UStaticMesh* Mesh) const
{
	if (!Component)
	{
		return;
	}

	Component->ClearInstances();
	Component->SetStaticMesh(Mesh);
	if (Profile && Profile->MaterialOverride)
	{
		Component->SetMaterial(0, Profile->MaterialOverride);
	}
	else
	{
		Component->EmptyOverrideMaterials();
	}
	Component->SetVisibility(Mesh != nullptr, true);
}

bool ASplineWorldBuilderActor::IsCornerPoint(const int32 PointIndex) const
{
	if (!Spline || !Profile)
	{
		return false;
	}

	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	const bool bClosed = Spline->IsClosedLoop();
	if (NumPoints < 3 || (!bClosed && (PointIndex <= 0 || PointIndex >= NumPoints - 1)))
	{
		return false;
	}

	const int32 PreviousIndex = PointIndex > 0 ? PointIndex - 1 : NumPoints - 1;
	const int32 NextIndex = PointIndex + 1 < NumPoints ? PointIndex + 1 : 0;
	const FVector Previous = Spline->GetLocationAtSplinePoint(PreviousIndex, ESplineCoordinateSpace::World);
	const FVector Current = Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
	const FVector Next = Spline->GetLocationAtSplinePoint(NextIndex, ESplineCoordinateSpace::World);
	const FVector Incoming = FlatDirection(Current - Previous);
	const FVector Outgoing = FlatDirection(Next - Current);
	if (Incoming.IsNearlyZero() || Outgoing.IsNearlyZero())
	{
		return false;
	}

	const double TurnDegrees = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
		static_cast<double>(FVector::DotProduct(Incoming, Outgoing)), -1.0, 1.0)));
	return TurnDegrees >= Profile->CornerAngleThreshold;
}

double ASplineWorldBuilderActor::GetTrimAtPoint(const int32 PointIndex) const
{
	if (!Spline || !Profile)
	{
		return 0.0;
	}

	const int32 LastPointIndex = Spline->GetNumberOfSplinePoints() - 1;
	if (!Spline->IsClosedLoop() && PointIndex == 0)
	{
		return StartJunction ? Profile->JunctionTrimDistance :
			(Profile->bPlaceFreeEndPieces && Profile->EndMesh ? Profile->EndTrimDistance : 0.0);
	}
	if (!Spline->IsClosedLoop() && PointIndex == LastPointIndex)
	{
		return EndJunction ? Profile->JunctionTrimDistance :
			(Profile->bPlaceFreeEndPieces && Profile->EndMesh ? Profile->EndTrimDistance : 0.0);
	}
	return IsCornerPoint(PointIndex) && Profile->bPlaceCornerPieces && Profile->CornerMesh
		? Profile->CornerTrimDistance
		: 0.0;
}

void ASplineWorldBuilderActor::AddCornerInstance(const int32 PointIndex)
{
	if (!CornerInstances || !Spline || !IsCornerPoint(PointIndex))
	{
		return;
	}

	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	const int32 PreviousIndex = PointIndex > 0 ? PointIndex - 1 : NumPoints - 1;
	const int32 NextIndex = PointIndex + 1 < NumPoints ? PointIndex + 1 : 0;
	const FVector Current = Spline->GetLocationAtSplinePoint(PointIndex, ESplineCoordinateSpace::World);
	FVector ArmA = FlatDirection(Spline->GetLocationAtSplinePoint(PreviousIndex, ESplineCoordinateSpace::World) - Current);
	FVector ArmB = FlatDirection(Spline->GetLocationAtSplinePoint(NextIndex, ESplineCoordinateSpace::World) - Current);
	if (FVector::CrossProduct(ArmA, ArmB).Z < 0.0)
	{
		Swap(ArmA, ArmB);
	}

	CornerInstances->AddInstance(FTransform(YawRotation(ArmA), Current), true);
}

void ASplineWorldBuilderActor::AddEndInstance(const bool bAtStart)
{
	if (!EndInstances || !Spline || !Profile || !Profile->bPlaceFreeEndPieces)
	{
		return;
	}

	const double Distance = bAtStart ? 0.0 : Spline->GetSplineLength();
	const FVector Location = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
	FVector Direction = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
	if (!bAtStart)
	{
		Direction *= -1.0;
	}
	EndInstances->AddInstance(FTransform(YawRotation(Direction), Location), true);
}

void ASplineWorldBuilderActor::RebuildGenerated()
{
	ConfigureGeneratedComponent(StraightInstances, Profile ? Profile->StraightMesh.Get() : nullptr);
	ConfigureGeneratedComponent(CornerInstances, Profile ? Profile->CornerMesh.Get() : nullptr);
	ConfigureGeneratedComponent(EndInstances, Profile ? Profile->EndMesh.Get() : nullptr);

	if (!Spline || !Profile || !Profile->StraightMesh || Profile->ModuleLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	Spline->UpdateSpline();
	const int32 NumPoints = Spline->GetNumberOfSplinePoints();
	if (NumPoints < 2)
	{
		return;
	}

	const bool bClosed = Spline->IsClosedLoop();
	const int32 NumSegments = bClosed ? NumPoints : NumPoints - 1;
	const double TotalLength = Spline->GetSplineLength();

	for (int32 SegmentIndex = 0; SegmentIndex < NumSegments; ++SegmentIndex)
	{
		const int32 StartPointIndex = SegmentIndex;
		const int32 EndPointIndex = (SegmentIndex + 1) % NumPoints;
		const double SegmentStart = Spline->GetDistanceAlongSplineAtSplinePoint(StartPointIndex);
		const double SegmentEnd = bClosed && EndPointIndex == 0
			? TotalLength
			: Spline->GetDistanceAlongSplineAtSplinePoint(EndPointIndex);
		const double StartDistance = SegmentStart + GetTrimAtPoint(StartPointIndex);
		const double EndDistance = SegmentEnd - GetTrimAtPoint(EndPointIndex);
		const double AvailableLength = EndDistance - StartDistance;
		if (AvailableLength <= KINDA_SMALL_NUMBER)
		{
			continue;
		}

		int32 ModuleCount = FMath::Max(1, FMath::RoundToInt(AvailableLength / Profile->ModuleLength));
		double ModuleScale = AvailableLength / (static_cast<double>(ModuleCount) * Profile->ModuleLength);
		if (Profile->bAllowLengthScaling)
		{
			while (ModuleCount > 1 && ModuleScale < Profile->MinimumLengthScale)
			{
				--ModuleCount;
				ModuleScale = AvailableLength / (static_cast<double>(ModuleCount) * Profile->ModuleLength);
			}
			while (ModuleScale > Profile->MaximumLengthScale)
			{
				++ModuleCount;
				ModuleScale = AvailableLength / (static_cast<double>(ModuleCount) * Profile->ModuleLength);
			}
		}
		else
		{
			ModuleCount = FMath::Max(1, FMath::FloorToInt(AvailableLength / Profile->ModuleLength));
			ModuleScale = 1.0;
		}

		const double OccupiedLength = static_cast<double>(ModuleCount) * Profile->ModuleLength * ModuleScale;
		const double LeadingGap = FMath::Max(0.0, (AvailableLength - OccupiedLength) * 0.5);
		const double StepLength = Profile->ModuleLength * ModuleScale;
		for (int32 ModuleIndex = 0; ModuleIndex < ModuleCount; ++ModuleIndex)
		{
			const double Distance = StartDistance + LeadingGap + (static_cast<double>(ModuleIndex) + 0.5) * StepLength;
			FTransform ModuleTransform = Spline->GetTransformAtDistanceAlongSpline(
				Distance,
				ESplineCoordinateSpace::World,
				false);
			ModuleTransform.SetScale3D(FVector(ModuleScale, 1.0, 1.0));
			StraightInstances->AddInstance(ModuleTransform, true);
		}
	}

	if (Profile->bPlaceCornerPieces && Profile->CornerMesh)
	{
		for (int32 PointIndex = 0; PointIndex < NumPoints; ++PointIndex)
		{
			AddCornerInstance(PointIndex);
		}
	}

	if (!bClosed && Profile->bPlaceFreeEndPieces && Profile->EndMesh)
	{
		if (!StartJunction)
		{
			AddEndInstance(true);
		}
		if (!EndJunction)
		{
			AddEndInstance(false);
		}
	}
}
