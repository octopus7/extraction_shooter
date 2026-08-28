#include "Component/TunaSweeperGazeTrackingComponent.h"

#include "Component/TunaSweeperGazePoseSink.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperGazeTracking, Log, All);

namespace
{
	bool HasExplicitComponentReference(const FComponentReference& Reference)
	{
		return Reference.OverrideComponent.IsValid() ||
			!Reference.ComponentProperty.IsNone() ||
			!Reference.PathToComponent.IsEmpty() ||
			Reference.OtherActor.IsValid();
	}
}

UTunaSweeperGazeTrackingComponent::UTunaSweeperGazeTrackingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UTunaSweeperGazeTrackingComponent::OnRegister()
{
	Super::OnRegister();
	RefreshComponentReferences();
	RefreshTickPrerequisite();
}

void UTunaSweeperGazeTrackingComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshComponentReferences();
	RefreshTickPrerequisite();
}

void UTunaSweeperGazeTrackingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	SubmitPoseRequest(0.0f, true);
	if (PrerequisiteMesh.IsValid())
	{
		PrerequisiteMesh->RemoveTickPrerequisiteComponent(this);
	}
	PrerequisiteMesh.Reset();
	Super::EndPlay(EndPlayReason);
}

void UTunaSweeperGazeTrackingComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	RefreshComponentReferences();
	RefreshTickPrerequisite();
	SubmitPoseRequest(DeltaTime);
	if (bDrawDebugGaze)
	{
		DrawDebugGaze();
	}
}

void UTunaSweeperGazeTrackingComponent::SetGazeEnabled(bool bEnabled)
{
	bGazeEnabled = bEnabled;
}

void UTunaSweeperGazeTrackingComponent::SetGazeWeight(float InWeight)
{
	GazeWeight = FMath::Clamp(InWeight, 0.0f, 1.0f);
}

void UTunaSweeperGazeTrackingComponent::SetGazeTargetWorldTransform(const FTransform& TargetTransform)
{
	SetWorldTransform(TargetTransform);
}

void UTunaSweeperGazeTrackingComponent::SetTrackedMesh(USkeletalMeshComponent* InTrackedMesh)
{
	RuntimeTrackedMesh = InTrackedMesh;
	TrackedMeshReference.OverrideComponent = InTrackedMesh;
	RefreshTickPrerequisite();
}

void UTunaSweeperGazeTrackingComponent::SetEyeTargetComponents(
	USceneComponent* InLeftEyeTarget,
	USceneComponent* InRightEyeTarget)
{
	RuntimeLeftEyeTarget = InLeftEyeTarget;
	RuntimeRightEyeTarget = InRightEyeTarget;
	LeftEyeTargetReference.OverrideComponent = InLeftEyeTarget;
	RightEyeTargetReference.OverrideComponent = InRightEyeTarget;
}

void UTunaSweeperGazeTrackingComponent::SetEyeBoneNames(
	FName InLeftEyeBoneName,
	FName InRightEyeBoneName)
{
	LeftEyeBoneName = InLeftEyeBoneName;
	RightEyeBoneName = InRightEyeBoneName;
}

void UTunaSweeperGazeTrackingComponent::SetEyeAxes(FVector InEyeAimAxis, FVector InEyeUpAxis)
{
	if (!InEyeAimAxis.IsNearlyZero())
	{
		EyeAimAxis = InEyeAimAxis.GetSafeNormal();
	}
	if (!InEyeUpAxis.IsNearlyZero())
	{
		EyeUpAxis = InEyeUpAxis.GetSafeNormal();
	}
}

void UTunaSweeperGazeTrackingComponent::ReturnToNeutral()
{
	bGazeEnabled = false;
}

void UTunaSweeperGazeTrackingComponent::ApplyRecommendedEyeTargetSpacing()
{
	RefreshComponentReferences();
	const float HalfSpacing = FMath::Max(0.0f, RecommendedEyeTargetSpacing) * 0.5f;
	if (RuntimeLeftEyeTarget.IsValid())
	{
		RuntimeLeftEyeTarget->SetRelativeLocation(FVector(0.0f, -HalfSpacing, 0.0f));
	}
	if (RuntimeRightEyeTarget.IsValid())
	{
		RuntimeRightEyeTarget->SetRelativeLocation(FVector(0.0f, HalfSpacing, 0.0f));
	}
}

USceneComponent* UTunaSweeperGazeTrackingComponent::GetLeftEyeTarget() const
{
	return RuntimeLeftEyeTarget.Get();
}

USceneComponent* UTunaSweeperGazeTrackingComponent::GetRightEyeTarget() const
{
	return RuntimeRightEyeTarget.Get();
}

void UTunaSweeperGazeTrackingComponent::RefreshComponentReferences()
{
	if (!RuntimeTrackedMesh.IsValid())
	{
		RuntimeTrackedMesh = ResolveTrackedMesh();
	}
	if (!RuntimeLeftEyeTarget.IsValid())
	{
		RuntimeLeftEyeTarget = ResolveSceneComponent(LeftEyeTargetReference, TEXT("LeftEyeTarget"));
	}
	if (!RuntimeRightEyeTarget.IsValid())
	{
		RuntimeRightEyeTarget = ResolveSceneComponent(RightEyeTargetReference, TEXT("RightEyeTarget"));
	}
}

void UTunaSweeperGazeTrackingComponent::RefreshTickPrerequisite()
{
	USkeletalMeshComponent* ResolvedMesh = RuntimeTrackedMesh.Get();
	if (PrerequisiteMesh.Get() == ResolvedMesh)
	{
		return;
	}

	if (PrerequisiteMesh.IsValid())
	{
		PrerequisiteMesh->RemoveTickPrerequisiteComponent(this);
	}
	PrerequisiteMesh = ResolvedMesh;
	if (ResolvedMesh)
	{
		ResolvedMesh->AddTickPrerequisiteComponent(this);
	}
}

void UTunaSweeperGazeTrackingComponent::SubmitPoseRequest(float DeltaTime, bool bForceNeutral)
{
	USkeletalMeshComponent* TrackedMesh = RuntimeTrackedMesh.Get();
	if (!TrackedMesh)
	{
		return;
	}

	ITunaSweeperGazePoseSink* PoseSink = Cast<ITunaSweeperGazePoseSink>(TrackedMesh);
	if (!PoseSink)
	{
		if (!bWarnedMissingPoseSink)
		{
			UE_LOG(
				LogTunaSweeperGazeTracking,
				Warning,
				TEXT("Gaze tracking mesh '%s' does not implement ITunaSweeperGazePoseSink."),
				*GetNameSafe(TrackedMesh));
			bWarnedMissingPoseSink = true;
		}
		return;
	}
	bWarnedMissingPoseSink = false;

	FTunaSweeperGazePoseRequest Request;
	Request.bEnabled = bGazeEnabled && !bForceNeutral;
	Request.bHasLeftTarget = Request.bEnabled && RuntimeLeftEyeTarget.IsValid();
	Request.bHasRightTarget = Request.bEnabled && RuntimeRightEyeTarget.IsValid();
	Request.LeftTargetWorldLocation = Request.bHasLeftTarget
		? RuntimeLeftEyeTarget->GetComponentLocation()
		: FVector::ZeroVector;
	Request.RightTargetWorldLocation = Request.bHasRightTarget
		? RuntimeRightEyeTarget->GetComponentLocation()
		: FVector::ZeroVector;
	Request.LeftEyeBoneName = LeftEyeBoneName;
	Request.RightEyeBoneName = RightEyeBoneName;
	Request.EyeAimAxis = EyeAimAxis;
	Request.EyeUpAxis = EyeUpAxis;
	Request.MaxYawDegrees = FMath::Max(0.0f, MaxYawDegrees);
	Request.MaxPitchUpDegrees = FMath::Max(0.0f, MaxPitchUpDegrees);
	Request.MaxPitchDownDegrees = FMath::Max(0.0f, MaxPitchDownDegrees);
	Request.TrackingInterpolationSpeed = FMath::Max(0.0f, TrackingInterpolationSpeed);
	Request.NeutralReturnInterpolationSpeed = FMath::Max(0.0f, NeutralReturnInterpolationSpeed);
	Request.MinimumTargetDistance = FMath::Max(0.0f, MinimumTargetDistance);
	Request.Weight = FMath::Clamp(GazeWeight, 0.0f, 1.0f);
	Request.DeltaSeconds = FMath::Max(0.0f, DeltaTime);
	PoseSink->SetGazePoseRequest(Request);
}

USkeletalMeshComponent* UTunaSweeperGazeTrackingComponent::ResolveTrackedMesh() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (HasExplicitComponentReference(TrackedMeshReference))
	{
		if (USkeletalMeshComponent* ReferencedMesh =
			Cast<USkeletalMeshComponent>(TrackedMeshReference.GetComponent(Owner)))
		{
			return ReferencedMesh;
		}
	}

	TArray<USkeletalMeshComponent*> MeshComponents;
	Owner->GetComponents(MeshComponents);
	for (USkeletalMeshComponent* MeshComponent : MeshComponents)
	{
		if (MeshComponent && MeshComponent->GetClass()->ImplementsInterface(UTunaSweeperGazePoseSink::StaticClass()))
		{
			return MeshComponent;
		}
	}
	return nullptr;
}

USceneComponent* UTunaSweeperGazeTrackingComponent::ResolveSceneComponent(
	const FComponentReference& Reference,
	FName FallbackName) const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (HasExplicitComponentReference(Reference))
	{
		if (USceneComponent* ReferencedComponent = Cast<USceneComponent>(Reference.GetComponent(Owner)))
		{
			return ReferencedComponent;
		}
	}

	TArray<USceneComponent*> SceneComponents;
	Owner->GetComponents(SceneComponents);
	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent && SceneComponent->GetFName() == FallbackName)
		{
			return SceneComponent;
		}
	}
	return nullptr;
}

void UTunaSweeperGazeTrackingComponent::DrawDebugGaze() const
{
	const UWorld* World = GetWorld();
	const USkeletalMeshComponent* TrackedMesh = RuntimeTrackedMesh.Get();
	if (!World || !TrackedMesh)
	{
		return;
	}

	DrawDebugSphere(World, GetComponentLocation(), 2.5f, 8, FColor::Cyan, false, 0.0f, 0, 0.5f);
	if (RuntimeLeftEyeTarget.IsValid())
	{
		const FVector EyeLocation = TrackedMesh->GetBoneLocation(LeftEyeBoneName);
		const FVector TargetLocation = RuntimeLeftEyeTarget->GetComponentLocation();
		DrawDebugSphere(World, TargetLocation, 2.0f, 8, FColor::Green, false, 0.0f, 0, 0.5f);
		DrawDebugLine(World, EyeLocation, TargetLocation, FColor::Green, false, 0.0f, 0, 0.5f);
	}
	if (RuntimeRightEyeTarget.IsValid())
	{
		const FVector EyeLocation = TrackedMesh->GetBoneLocation(RightEyeBoneName);
		const FVector TargetLocation = RuntimeRightEyeTarget->GetComponentLocation();
		DrawDebugSphere(World, TargetLocation, 2.0f, 8, FColor::Yellow, false, 0.0f, 0, 0.5f);
		DrawDebugLine(World, EyeLocation, TargetLocation, FColor::Yellow, false, 0.0f, 0, 0.5f);
	}
}
