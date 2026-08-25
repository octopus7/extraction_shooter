#include "WallCopingSplineActor.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

AWallCopingSplineActor::AWallCopingSplineActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("CopingSpline"));
	Spline->SetupAttachment(SceneRoot);
	Spline->SetClosedLoop(false);
	Spline->ClearSplinePoints(false);
	Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
	Spline->AddSplinePoint(FVector(300.0, 0.0, 0.0), ESplineCoordinateSpace::Local, false);
	Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
	Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
	Spline->UpdateSpline();

	CopingInstances = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("CopingInstances"));
	CopingInstances->SetupAttachment(SceneRoot);
	CopingInstances->SetMobility(EComponentMobility::Movable);
	CopingInstances->SetCollisionProfileName(TEXT("BlockAll"));
	CopingInstances->SetCanEverAffectNavigation(false);
}

void AWallCopingSplineActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bAutoRebuild)
	{
		RebuildCoping();
	}
}

int32 AWallCopingSplineActor::GetInstanceCount() const
{
	return CopingInstances ? CopingInstances->GetInstanceCount() : 0;
}

double AWallCopingSplineActor::GetResolvedModuleLength() const
{
	if (bUseMeshBoundsForModuleLength && CopingMesh)
	{
		const double BoundsLength = CopingMesh->GetBoundingBox().GetSize().X;
		if (BoundsLength > KINDA_SMALL_NUMBER)
		{
			return BoundsLength;
		}
	}
	return FMath::Max(ModuleLength, 1.0);
}

bool AWallCopingSplineActor::GetInstanceTransform(
	const int32 InstanceIndex,
	FTransform& OutTransform,
	const bool bWorldSpace) const
{
	return CopingInstances && CopingInstances->GetInstanceTransform(InstanceIndex, OutTransform, bWorldSpace);
}

void AWallCopingSplineActor::RebuildCoping()
{
	if (!CopingInstances)
	{
		return;
	}

	CopingInstances->ClearInstances();
	CopingInstances->SetStaticMesh(CopingMesh);
	if (MaterialOverride)
	{
		CopingInstances->SetMaterial(0, MaterialOverride);
	}
	else
	{
		CopingInstances->EmptyOverrideMaterials();
	}
	CopingInstances->SetVisibility(CopingMesh != nullptr, true);

	if (!Spline || !CopingMesh || Spline->GetNumberOfSplinePoints() < 2)
	{
		return;
	}

	Spline->UpdateSpline();
	const double SplineLength = Spline->GetSplineLength();
	const double ResolvedLength = GetResolvedModuleLength();
	if (SplineLength <= KINDA_SMALL_NUMBER || ResolvedLength <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const double SafeGap = FMath::Max(Gap, -ResolvedLength + 1.0);
	const double StepLength = ResolvedLength + SafeGap;
	int32 ModuleCount = FMath::FloorToInt((SplineLength + SafeGap) / StepLength);
	const bool bShortSpline = ModuleCount <= 0;
	if (bShortSpline)
	{
		if (!bPlaceSingleModuleOnShortSpline)
		{
			return;
		}
		ModuleCount = 1;
	}

	const double OccupiedLength = static_cast<double>(ModuleCount) * ResolvedLength +
		static_cast<double>(FMath::Max(0, ModuleCount - 1)) * SafeGap;
	double LeadingDistance = AlignmentPolicy == EWallCopingAlignmentPolicy::Centered
		? (SplineLength - OccupiedLength) * 0.5
		: 0.0;
	if (bShortSpline)
	{
		LeadingDistance = (SplineLength - ResolvedLength) * 0.5;
	}

	FRandomStream Random(Seed);
	for (int32 ModuleIndex = 0; ModuleIndex < ModuleCount; ++ModuleIndex)
	{
		const double Distance = FMath::Clamp(
			LeadingDistance + ResolvedLength * 0.5 + static_cast<double>(ModuleIndex) * StepLength,
			0.0,
			SplineLength);
		FVector Location = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		Location.Z += VerticalOffset;

		FRotator Rotation;
		if (bFollowSplinePitchAndRoll)
		{
			Rotation = Spline->GetRotationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local);
		}
		else
		{
			const FVector Direction = Spline->GetDirectionAtDistanceAlongSpline(
				Distance,
				ESplineCoordinateSpace::Local);
			Rotation = FRotator(0.0, Direction.Rotation().Yaw, 0.0);
		}
		Rotation.Yaw += Random.FRandRange(-MaxYawJitterDegrees, MaxYawJitterDegrees);

		const double InstanceWidthScale = WidthScale *
			(1.0 + Random.FRandRange(-MaxWidthScaleVariation, MaxWidthScaleVariation));
		const double InstanceHeightScale = HeightScale *
			(1.0 + Random.FRandRange(-MaxHeightScaleVariation, MaxHeightScaleVariation));
		const FVector Scale(1.0, InstanceWidthScale, InstanceHeightScale);
		CopingInstances->AddInstance(FTransform(Rotation, Location, Scale), false);
	}
}
