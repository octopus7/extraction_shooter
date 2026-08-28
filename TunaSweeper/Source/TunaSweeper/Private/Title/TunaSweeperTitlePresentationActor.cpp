#include "Title/TunaSweeperTitlePresentationActor.h"

#include "Animation/AnimInstance.h"
#include "Component/TunaSweeperGazeTrackingComponent.h"
#include "ReferenceSkeleton.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/ConstructorHelpers.h"

namespace TunaSweeperTitlePresentation
{
	void ConfigureWallComponent(
		UStaticMeshComponent* Component,
		UStaticMesh* CubeMesh,
		UMaterialInterface* WallMaterial)
	{
		if (!Component)
		{
			return;
		}

		Component->SetStaticMesh(CubeMesh);
		Component->SetMaterial(0, WallMaterial);
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetGenerateOverlapEvents(false);
		Component->SetMobility(EComponentMobility::Movable);
		Component->CastShadow = true;
	}
}

void UTunaSweeperTitleSkeletalMeshComponent::SetDirectHeadLookRotation(float YawDegrees, float PitchDegrees)
{
	DirectHeadLookYaw = YawDegrees;
	DirectHeadLookPitch = PitchDegrees;
}

void UTunaSweeperTitleSkeletalMeshComponent::SetTemporaryRelaxedArmPose(
	float BlendAlpha,
	float MotionPhaseSeconds)
{
	TemporaryRelaxedArmBlendAlpha = FMath::Clamp(BlendAlpha, 0.0f, 1.0f);
	TemporaryRelaxedArmMotionPhase = MotionPhaseSeconds;
}

void UTunaSweeperTitleSkeletalMeshComponent::SetGazePoseRequest(const FTunaSweeperGazePoseRequest& Request)
{
	CurrentGazePoseRequest = Request;
}

void UTunaSweeperTitleSkeletalMeshComponent::FinalizeBoneTransform()
{
	ApplyTemporaryRelaxedArmPoseToEditablePose();
	ApplyDirectHeadLookToEditablePose();
	ApplyEyeGazeToEditablePose();
	Super::FinalizeBoneTransform();
}

bool UTunaSweeperTitleSkeletalMeshComponent::IsBoneDescendantOf(int32 BoneIndex, int32 ParentBoneIndex) const
{
	const USkeletalMesh* TitleMeshAsset = GetSkeletalMeshAsset();
	if (!TitleMeshAsset || BoneIndex < 0 || ParentBoneIndex < 0)
	{
		return false;
	}

	const FReferenceSkeleton& ReferenceSkeleton = TitleMeshAsset->GetRefSkeleton();
	int32 CurrentBoneIndex = BoneIndex;
	while (CurrentBoneIndex != INDEX_NONE)
	{
		if (CurrentBoneIndex == ParentBoneIndex)
		{
			return true;
		}
		CurrentBoneIndex = ReferenceSkeleton.GetParentIndex(CurrentBoneIndex);
	}
	return false;
}

void UTunaSweeperTitleSkeletalMeshComponent::ApplyTemporaryRelaxedArmPoseToEditablePose()
{
	if (!bApplyTemporaryRelaxedArms || TemporaryRelaxedArmBlendAlpha <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const USkeletalMesh* TitleMeshAsset = GetSkeletalMeshAsset();
	if (!TitleMeshAsset)
	{
		return;
	}

	TArray<FTransform>& ComponentSpaceTransforms = GetEditableComponentSpaceTransforms();
	const FReferenceSkeleton& ReferenceSkeleton = TitleMeshAsset->GetRefSkeleton();
	if (ComponentSpaceTransforms.IsEmpty())
	{
		return;
	}

	ApplyRelaxedArmBranch(
		ComponentSpaceTransforms,
		ReferenceSkeleton,
		TEXT("upperarm_l"),
		TEXT("lowerarm_l"),
		0.0f);
	ApplyRelaxedArmBranch(
		ComponentSpaceTransforms,
		ReferenceSkeleton,
		TEXT("upperarm_r"),
		TEXT("lowerarm_r"),
		PI);
}

void UTunaSweeperTitleSkeletalMeshComponent::ApplyRelaxedArmBranch(
	TArray<FTransform>& ComponentSpaceTransforms,
	const FReferenceSkeleton& ReferenceSkeleton,
	FName UpperArmBoneName,
	FName LowerArmBoneName,
	float SidePhaseOffset) const
{
	const int32 UpperArmBoneIndex = ReferenceSkeleton.FindBoneIndex(UpperArmBoneName);
	const int32 LowerArmBoneIndex = ReferenceSkeleton.FindBoneIndex(LowerArmBoneName);
	if (!ComponentSpaceTransforms.IsValidIndex(UpperArmBoneIndex) ||
		!ComponentSpaceTransforms.IsValidIndex(LowerArmBoneIndex))
	{
		return;
	}

	const int32 UpperArmParentIndex = ReferenceSkeleton.GetParentIndex(UpperArmBoneIndex);
	if (!ComponentSpaceTransforms.IsValidIndex(UpperArmParentIndex))
	{
		return;
	}

	const TArray<FTransform>& ReferenceLocalPose = ReferenceSkeleton.GetRefBonePose();
	TArray<FTransform> TargetTransforms = ComponentSpaceTransforms;
	for (int32 BoneIndex = UpperArmBoneIndex; BoneIndex < TargetTransforms.Num(); ++BoneIndex)
	{
		if (!IsBoneDescendantOf(BoneIndex, UpperArmBoneIndex))
		{
			continue;
		}

		const int32 ParentIndex = ReferenceSkeleton.GetParentIndex(BoneIndex);
		if (!ReferenceLocalPose.IsValidIndex(BoneIndex) || !TargetTransforms.IsValidIndex(ParentIndex))
		{
			continue;
		}
		TargetTransforms[BoneIndex] = ReferenceLocalPose[BoneIndex] * TargetTransforms[ParentIndex];
	}

	const FVector ShoulderLocation = TargetTransforms[UpperArmBoneIndex].GetLocation();
	const FVector ReferenceArmDirection =
		(TargetTransforms[LowerArmBoneIndex].GetLocation() - ShoulderLocation).GetSafeNormal();
	FVector OutwardDirection = FVector(ReferenceArmDirection.X, ReferenceArmDirection.Y, 0.0f).GetSafeNormal();
	if (OutwardDirection.IsNearlyZero())
	{
		OutwardDirection = FVector::RightVector;
	}

	const float SlowSway = FMath::Sin(TemporaryRelaxedArmMotionPhase * 0.55f + SidePhaseOffset) * 0.025f;
	const FVector DesiredArmDirection =
		(-FVector::UpVector + OutwardDirection * (0.10f + SlowSway)).GetSafeNormal();
	const FQuat ArmDropDelta = FQuat::FindBetweenNormals(ReferenceArmDirection, DesiredArmDirection);

	for (int32 BoneIndex = UpperArmBoneIndex; BoneIndex < TargetTransforms.Num(); ++BoneIndex)
	{
		if (!IsBoneDescendantOf(BoneIndex, UpperArmBoneIndex))
		{
			continue;
		}

		FTransform& TargetTransform = TargetTransforms[BoneIndex];
		TargetTransform.SetLocation(
			ShoulderLocation + ArmDropDelta.RotateVector(TargetTransform.GetLocation() - ShoulderLocation));
		TargetTransform.SetRotation((ArmDropDelta * TargetTransform.GetRotation()).GetNormalized());

		FTransform& CurrentTransform = ComponentSpaceTransforms[BoneIndex];
		CurrentTransform.SetLocation(FMath::Lerp(
			CurrentTransform.GetLocation(),
			TargetTransform.GetLocation(),
			TemporaryRelaxedArmBlendAlpha));
		CurrentTransform.SetRotation(FQuat::Slerp(
			CurrentTransform.GetRotation(),
			TargetTransform.GetRotation(),
			TemporaryRelaxedArmBlendAlpha).GetNormalized());
	}
}

void UTunaSweeperTitleSkeletalMeshComponent::ApplyDirectHeadLookToEditablePose()
{
	if (!bApplyDirectHeadLook ||
		(FMath::IsNearlyZero(DirectHeadLookYaw, 0.01f) && FMath::IsNearlyZero(DirectHeadLookPitch, 0.01f)))
	{
		return;
	}

	int32 HeadBoneIndex = GetBoneIndex(HeadBoneName);
	if (HeadBoneIndex == INDEX_NONE)
	{
		HeadBoneIndex = GetBoneIndex(TEXT("head"));
	}
	if (HeadBoneIndex == INDEX_NONE)
	{
		return;
	}

	TArray<FTransform>& ComponentSpaceTransforms = GetEditableComponentSpaceTransforms();
	if (!ComponentSpaceTransforms.IsValidIndex(HeadBoneIndex))
	{
		return;
	}

	const FTransform OriginalHeadTransform = ComponentSpaceTransforms[HeadBoneIndex];
	const FVector HeadLocation = OriginalHeadTransform.GetLocation();
	const FQuat YawDelta(FVector::UpVector, FMath::DegreesToRadians(-DirectHeadLookYaw));
	const FQuat PitchDelta(FVector::ForwardVector, FMath::DegreesToRadians(DirectHeadLookPitch));
	const FQuat LookDelta = (PitchDelta * YawDelta).GetNormalized();

	for (int32 BoneIndex = 0; BoneIndex < ComponentSpaceTransforms.Num(); ++BoneIndex)
	{
		if (!IsBoneDescendantOf(BoneIndex, HeadBoneIndex))
		{
			continue;
		}

		FTransform& BoneTransform = ComponentSpaceTransforms[BoneIndex];
		BoneTransform.SetLocation(HeadLocation + LookDelta.RotateVector(BoneTransform.GetLocation() - HeadLocation));
		BoneTransform.SetRotation((LookDelta * BoneTransform.GetRotation()).GetNormalized());
	}
}

void UTunaSweeperTitleSkeletalMeshComponent::ApplyEyeGazeToEditablePose()
{
	if (!bApplyDirectEyeGaze)
	{
		CurrentLeftEyeYawDegrees = 0.0f;
		CurrentLeftEyePitchDegrees = 0.0f;
		CurrentRightEyeYawDegrees = 0.0f;
		CurrentRightEyePitchDegrees = 0.0f;
		return;
	}

	TArray<FTransform>& ComponentSpaceTransforms = GetEditableComponentSpaceTransforms();
	if (ComponentSpaceTransforms.IsEmpty())
	{
		return;
	}

	ApplyEyeGazeBranch(
		ComponentSpaceTransforms,
		CurrentGazePoseRequest.LeftEyeBoneName,
		CurrentGazePoseRequest.LeftTargetWorldLocation,
		CurrentGazePoseRequest.bEnabled && CurrentGazePoseRequest.bHasLeftTarget,
		CurrentLeftEyeYawDegrees,
		CurrentLeftEyePitchDegrees);
	ApplyEyeGazeBranch(
		ComponentSpaceTransforms,
		CurrentGazePoseRequest.RightEyeBoneName,
		CurrentGazePoseRequest.RightTargetWorldLocation,
		CurrentGazePoseRequest.bEnabled && CurrentGazePoseRequest.bHasRightTarget,
		CurrentRightEyeYawDegrees,
		CurrentRightEyePitchDegrees);
}

void UTunaSweeperTitleSkeletalMeshComponent::ApplyEyeGazeBranch(
	TArray<FTransform>& ComponentSpaceTransforms,
	FName EyeBoneName,
	const FVector& TargetWorldLocation,
	bool bHasTarget,
	float& InOutCurrentYawDegrees,
	float& InOutCurrentPitchDegrees)
{
	const int32 EyeBoneIndex = GetBoneIndex(EyeBoneName);
	if (!ComponentSpaceTransforms.IsValidIndex(EyeBoneIndex))
	{
		InOutCurrentYawDegrees = 0.0f;
		InOutCurrentPitchDegrees = 0.0f;
		return;
	}

	const FTransform BaseEyeTransform = ComponentSpaceTransforms[EyeBoneIndex];
	const FTransform MeshWorldTransform = GetComponentTransform();
	const FVector EyeWorldLocation = MeshWorldTransform.TransformPosition(BaseEyeTransform.GetLocation());
	const FVector TargetOffsetWorld = TargetWorldLocation - EyeWorldLocation;
	const float MinimumDistance = FMath::Max(0.0f, CurrentGazePoseRequest.MinimumTargetDistance);
	bool bHasValidDirection = bHasTarget && TargetOffsetWorld.SizeSquared() >= FMath::Square(MinimumDistance);

	float TargetYawDegrees = 0.0f;
	float TargetPitchDegrees = 0.0f;
	if (bHasValidDirection)
	{
		const FVector DesiredComponentDirection =
			MeshWorldTransform.InverseTransformVectorNoScale(TargetOffsetWorld).GetSafeNormal();
		bHasValidDirection = TunaSweeperGaze::SolveClampedLookAngles(
			BaseEyeTransform.GetRotation(),
			CurrentGazePoseRequest.EyeAimAxis,
			CurrentGazePoseRequest.EyeUpAxis,
			DesiredComponentDirection,
			CurrentGazePoseRequest.MaxYawDegrees,
			CurrentGazePoseRequest.MaxPitchUpDegrees,
			CurrentGazePoseRequest.MaxPitchDownDegrees,
			TargetYawDegrees,
			TargetPitchDegrees);
	}

	if (bHasValidDirection)
	{
		const float Weight = FMath::Clamp(CurrentGazePoseRequest.Weight, 0.0f, 1.0f);
		TargetYawDegrees *= Weight;
		TargetPitchDegrees *= Weight;
	}
	else
	{
		TargetYawDegrees = 0.0f;
		TargetPitchDegrees = 0.0f;
	}

	const float InterpolationSpeed = bHasValidDirection
		? CurrentGazePoseRequest.TrackingInterpolationSpeed
		: CurrentGazePoseRequest.NeutralReturnInterpolationSpeed;
	const float InterpolationAlpha = TunaSweeperGaze::CalculateExponentialInterpolationAlpha(
		InterpolationSpeed,
		CurrentGazePoseRequest.DeltaSeconds);
	InOutCurrentYawDegrees = FMath::Lerp(InOutCurrentYawDegrees, TargetYawDegrees, InterpolationAlpha);
	InOutCurrentPitchDegrees = FMath::Lerp(InOutCurrentPitchDegrees, TargetPitchDegrees, InterpolationAlpha);

	if (FMath::IsNearlyZero(InOutCurrentYawDegrees, 0.01f) &&
		FMath::IsNearlyZero(InOutCurrentPitchDegrees, 0.01f))
	{
		return;
	}

	const FQuat LookDelta = TunaSweeperGaze::BuildLookDelta(
		BaseEyeTransform.GetRotation(),
		CurrentGazePoseRequest.EyeAimAxis,
		CurrentGazePoseRequest.EyeUpAxis,
		InOutCurrentYawDegrees,
		InOutCurrentPitchDegrees);
	const FVector EyeLocation = BaseEyeTransform.GetLocation();
	for (int32 BoneIndex = 0; BoneIndex < ComponentSpaceTransforms.Num(); ++BoneIndex)
	{
		if (!IsBoneDescendantOf(BoneIndex, EyeBoneIndex))
		{
			continue;
		}

		FTransform& BoneTransform = ComponentSpaceTransforms[BoneIndex];
		BoneTransform.SetLocation(
			EyeLocation + LookDelta.RotateVector(BoneTransform.GetLocation() - EyeLocation));
		BoneTransform.SetRotation((LookDelta * BoneTransform.GetRotation()).GetNormalized());
	}
}

ATunaSweeperTitlePresentationActor::ATunaSweeperTitlePresentationActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetCanBeDamaged(false);
	bFindCameraComponentWhenViewTarget = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	TitleCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TitleCamera"));
	TitleCamera->SetupAttachment(SceneRoot);
	TitleCamera->SetAutoActivate(true);
	TitleCamera->SetFieldOfView(CameraFieldOfView);

	CharacterAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("CharacterAnchor"));
	CharacterAnchor->SetupAttachment(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UTunaSweeperTitleSkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(CharacterAnchor);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	FaceMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FaceMesh"));
	FaceMesh->SetupAttachment(BodyMesh, FaceAttachmentSocketName);
	FaceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FaceMesh->SetGenerateOverlapEvents(false);
	FaceMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	SkirtMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skirt"));
	SkirtMesh->SetupAttachment(CharacterAnchor);
	SkirtMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkirtMesh->SetGenerateOverlapEvents(false);
	SkirtMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	HeadLookTarget = CreateDefaultSubobject<USceneComponent>(TEXT("HeadLookTarget"));
	HeadLookTarget->SetupAttachment(SceneRoot);

	GazeTracking = CreateDefaultSubobject<UTunaSweeperGazeTrackingComponent>(TEXT("GazeTracking"));
	GazeTracking->SetupAttachment(SceneRoot);
	GazeTracking->AddTickPrerequisiteActor(this);

	LeftEyeTarget = CreateDefaultSubobject<USceneComponent>(TEXT("LeftEyeTarget"));
	LeftEyeTarget->SetupAttachment(GazeTracking);
	LeftEyeTarget->SetRelativeLocation(FVector(0.0f, -3.2f, 0.0f));

	RightEyeTarget = CreateDefaultSubobject<USceneComponent>(TEXT("RightEyeTarget"));
	RightEyeTarget->SetupAttachment(GazeTracking);
	RightEyeTarget->SetRelativeLocation(FVector(0.0f, 3.2f, 0.0f));

	BackWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackWall"));
	BackWall->SetupAttachment(SceneRoot);
	LeftWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftWall"));
	LeftWall->SetupAttachment(SceneRoot);
	RightWall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightWall"));
	RightWall->SetupAttachment(SceneRoot);
	Floor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Floor"));
	Floor->SetupAttachment(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMaterialFinder(
		TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	UStaticMesh* CubeMesh = CubeMeshFinder.Succeeded() ? CubeMeshFinder.Object : nullptr;
	UMaterialInterface* WallMaterial = WallMaterialFinder.Succeeded() ? WallMaterialFinder.Object : nullptr;
	TunaSweeperTitlePresentation::ConfigureWallComponent(BackWall, CubeMesh, WallMaterial);
	TunaSweeperTitlePresentation::ConfigureWallComponent(LeftWall, CubeMesh, WallMaterial);
	TunaSweeperTitlePresentation::ConfigureWallComponent(RightWall, CubeMesh, WallMaterial);
	TunaSweeperTitlePresentation::ConfigureWallComponent(Floor, CubeMesh, WallMaterial);

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMeshFinder(
		TEXT("/Game/Characters/Player/Luna/SKM_Luna.SKM_Luna"));
	if (BodyMeshFinder.Succeeded())
	{
		BodyMesh->SetSkeletalMeshAsset(BodyMeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> BodyAnimClassFinder(
		TEXT("/Game/Characters/Player/Luna/Animations/ABP_Luna"));
	if (BodyAnimClassFinder.Succeeded())
	{
		BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		BodyMesh->SetAnimInstanceClass(BodyAnimClassFinder.Class);
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkirtMeshFinder(
		TEXT("/Game/Characters/Player/Luna/Skirt/Luna__Skirt_front.Luna__Skirt_front"));
	if (SkirtMeshFinder.Succeeded())
	{
		SkirtMesh->SetSkeletalMeshAsset(SkirtMeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> SkirtAnimClassFinder(
		TEXT("/Game/Characters/Player/Luna/Skirt/Animations/ABP_Luna_Skirt"));
	if (SkirtAnimClassFinder.Succeeded())
	{
		SkirtMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		SkirtMesh->SetAnimInstanceClass(SkirtAnimClassFinder.Class);
	}

	SkirtBodyCollisionProxyPhysicsAsset = TSoftObjectPtr<UPhysicsAsset>(FSoftObjectPath(
		TEXT("/Game/Characters/Player/Luna/Skirt/PA_Luna_SkirtBodyProxy.PA_Luna_SkirtBodyProxy")));

	AmbientLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("AmbientLight"));
	AmbientLight->SetupAttachment(SceneRoot);
	AmbientLight->SetMobility(EComponentMobility::Movable);
	AmbientLight->SetIntensity(0.8f);

	CharacterKeyLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CharacterKeyLight"));
	CharacterKeyLight->SetupAttachment(SceneRoot);
	CharacterKeyLight->SetMobility(EComponentMobility::Movable);
	CharacterKeyLight->SetIntensity(5200.0f);
	CharacterKeyLight->SetAttenuationRadius(1150.0f);
	CharacterKeyLight->SetLightColor(FLinearColor(1.0f, 0.82f, 0.68f));

	EmptyWallLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EmptyWallLight"));
	EmptyWallLight->SetupAttachment(SceneRoot);
	EmptyWallLight->SetMobility(EComponentMobility::Movable);
	EmptyWallLight->SetIntensity(3200.0f);
	EmptyWallLight->SetAttenuationRadius(1050.0f);
	EmptyWallLight->SetLightColor(FLinearColor(0.48f, 0.68f, 1.0f));

	ApplyDesignTransforms();
}

void ATunaSweeperTitlePresentationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyDesignTransforms();

	if (FaceMesh && BodyMesh)
	{
		FaceMesh->AttachToComponent(
			BodyMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			FaceAttachmentSocketName);
	}
	if (GazeTracking)
	{
		GazeTracking->SetTrackedMesh(BodyMesh);
		GazeTracking->SetEyeTargetComponents(LeftEyeTarget, RightEyeTarget);
	}
}

void ATunaSweeperTitlePresentationActor::BeginPlay()
{
	Super::BeginPlay();
	if (GazeTracking)
	{
		GazeTracking->SetTrackedMesh(BodyMesh);
		GazeTracking->SetEyeTargetComponents(LeftEyeTarget, RightEyeTarget);
		GazeTracking->SetGazeEnabled(bEnableEyeCursorTracking);
	}
	ConfigureSkirtExternalPhysicsCollision();
	SetMainMenuPresentationActive(true);
}

void ATunaSweeperTitlePresentationActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CurrentRelaxedArmBlendAlpha = FMath::FInterpTo(
		CurrentRelaxedArmBlendAlpha,
		1.0f,
		DeltaSeconds,
		1.6f);
	if (BodyMesh)
	{
		BodyMesh->SetTemporaryRelaxedArmPose(CurrentRelaxedArmBlendAlpha, GetGameTimeSinceCreation());
	}

	UpdateCamera(DeltaSeconds);
	UpdateCharacterPresentationState();
	if (bCharacterPresentationEnabled && bMainMenuPresentationActive)
	{
		UpdateCursorLook(DeltaSeconds);
	}
}

void ATunaSweeperTitlePresentationActor::SetMainMenuPresentationActive(bool bActive)
{
	bMainMenuPresentationActive = bActive;
	if (GazeTracking)
	{
		GazeTracking->SetGazeEnabled(bActive && bEnableEyeCursorTracking);
	}
	if (bActive)
	{
		SetCharacterPresentationEnabled(true);
	}
	EnsureTitleCameraViewTarget();
}

void ATunaSweeperTitlePresentationActor::ApplyRecommendedPresentationLayout()
{
	CharacterRelativeLocation = FVector(120.0f, 210.0f, 0.0f);
	CharacterRelativeRotation = FRotator(0.0f, 105.0f, 0.0f);
	MainMenuCameraLocation = FVector(-500.0f, 0.0f, 155.0f);
	MainMenuCameraRotation = FRotator(-3.2f, 16.5f, 0.0f);
	SubMenuCameraLocation = MainMenuCameraLocation;
	SubMenuCameraRotation = FRotator(-1.0f, -38.0f, 0.0f);
	CameraFieldOfView = 14.0f;
	ApplyDesignTransforms();
}

void ATunaSweeperTitlePresentationActor::SetFacialWeight(FName MorphTargetName, float Weight)
{
	if (FaceMesh && !MorphTargetName.IsNone())
	{
		FaceMesh->SetMorphTarget(MorphTargetName, FMath::Clamp(Weight, 0.0f, 1.0f));
	}
}

void ATunaSweeperTitlePresentationActor::ClearFacialWeights()
{
	if (FaceMesh)
	{
		FaceMesh->ClearMorphTargets();
	}
}

FVector ATunaSweeperTitlePresentationActor::GetHeadLookTargetLocation() const
{
	return HeadLookTarget ? HeadLookTarget->GetComponentLocation() : GetActorLocation();
}

void ATunaSweeperTitlePresentationActor::ApplyDesignTransforms()
{
	if (TitleCamera)
	{
		TitleCamera->SetRelativeLocation(MainMenuCameraLocation);
		TitleCamera->SetRelativeRotation(MainMenuCameraRotation);
		TitleCamera->SetFieldOfView(CameraFieldOfView);
	}
	if (CharacterAnchor)
	{
		CharacterAnchor->SetRelativeLocation(CharacterRelativeLocation);
		CharacterAnchor->SetRelativeRotation(CharacterRelativeRotation);
	}

	if (BackWall)
	{
		BackWall->SetRelativeLocation(FVector(300.0f, 0.0f, 200.0f));
		BackWall->SetRelativeScale3D(FVector(0.2f, 12.0f, 4.0f));
	}
	if (LeftWall)
	{
		LeftWall->SetRelativeLocation(FVector(-300.0f, -600.0f, 200.0f));
		LeftWall->SetRelativeScale3D(FVector(12.0f, 0.2f, 4.0f));
	}
	if (RightWall)
	{
		RightWall->SetRelativeLocation(FVector(-300.0f, 600.0f, 200.0f));
		RightWall->SetRelativeScale3D(FVector(12.0f, 0.2f, 4.0f));
	}
	if (Floor)
	{
		Floor->SetRelativeLocation(FVector(-300.0f, 0.0f, -25.0f));
		Floor->SetRelativeScale3D(FVector(12.0f, 12.0f, 0.5f));
	}

	if (CharacterKeyLight)
	{
		CharacterKeyLight->SetRelativeLocation(FVector(-240.0f, 330.0f, 360.0f));
	}
	if (EmptyWallLight)
	{
		EmptyWallLight->SetRelativeLocation(FVector(-80.0f, -420.0f, 300.0f));
	}
}

void ATunaSweeperTitlePresentationActor::ConfigureSkirtExternalPhysicsCollision()
{
	UPhysicsAsset* ProxyPhysicsAsset = SkirtBodyCollisionProxyPhysicsAsset.LoadSynchronous();
	if (!BodyMesh || !SkirtMesh || !ProxyPhysicsAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("Title Luna skirt body collision proxy could not be loaded."));
		return;
	}

	SkirtMesh->AddClothCollisionSource(BodyMesh, ProxyPhysicsAsset);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Registered title Luna skirt external collision source: %s (%s)."),
		*BodyMesh->GetName(),
		*ProxyPhysicsAsset->GetName());
}

void ATunaSweeperTitlePresentationActor::UpdateCamera(float DeltaSeconds)
{
	if (!TitleCamera)
	{
		return;
	}

	const FVector TargetLocation = bMainMenuPresentationActive ? MainMenuCameraLocation : SubMenuCameraLocation;
	const FRotator TargetRotation = bMainMenuPresentationActive ? MainMenuCameraRotation : SubMenuCameraRotation;
	TitleCamera->SetRelativeLocation(FMath::VInterpTo(
		TitleCamera->GetRelativeLocation(), TargetLocation, DeltaSeconds, CameraBlendSpeed));
	TitleCamera->SetRelativeRotation(FMath::RInterpTo(
		TitleCamera->GetRelativeRotation(), TargetRotation, DeltaSeconds, CameraBlendSpeed));
}

void ATunaSweeperTitlePresentationActor::UpdateCursorLook(float DeltaSeconds)
{
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !TitleCamera)
	{
		return;
	}

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (ViewportWidth <= 0 || ViewportHeight <= 0 || !PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const float NormalizedX = FMath::Clamp((MouseX / static_cast<float>(ViewportWidth) - 0.5f) * 2.0f, -1.0f, 1.0f);
	const float NormalizedY = FMath::Clamp((MouseY / static_cast<float>(ViewportHeight) - 0.5f) * 2.0f, -1.0f, 1.0f);
	const float TargetYaw = NormalizedX * MaxHeadLookYaw;
	const float TargetPitch = -NormalizedY * MaxHeadLookPitch;
	const float DesiredHeadYaw = bEnableHeadCursorTracking ? TargetYaw : 0.0f;
	const float DesiredHeadPitch = bEnableHeadCursorTracking ? TargetPitch : 0.0f;
	CurrentHeadLookYaw = FMath::FInterpTo(
		CurrentHeadLookYaw, DesiredHeadYaw, DeltaSeconds, HeadLookInterpolationSpeed);
	CurrentHeadLookPitch = FMath::FInterpTo(
		CurrentHeadLookPitch, DesiredHeadPitch, DeltaSeconds, HeadLookInterpolationSpeed);
	if (BodyMesh)
	{
		BodyMesh->SetDirectHeadLookRotation(CurrentHeadLookYaw, CurrentHeadLookPitch);
	}

	const FVector CameraLocation = TitleCamera->GetComponentLocation();
	const float HorizontalOffset = FMath::Tan(FMath::DegreesToRadians(CurrentHeadLookYaw)) * HeadLookTargetDistance;
	const float VerticalOffset = FMath::Tan(FMath::DegreesToRadians(CurrentHeadLookPitch)) * HeadLookTargetDistance;
	const FVector WorldTarget = CameraLocation + TitleCamera->GetForwardVector() * HeadLookTargetDistance +
		TitleCamera->GetRightVector() * HorizontalOffset + TitleCamera->GetUpVector() * VerticalOffset;
	if (HeadLookTarget)
	{
		HeadLookTarget->SetWorldLocation(WorldTarget);
	}

	if (GazeTracking)
	{
		const float EyeHorizontalOffset =
			FMath::Tan(FMath::DegreesToRadians(TargetYaw)) * HeadLookTargetDistance;
		const float EyeVerticalOffset =
			FMath::Tan(FMath::DegreesToRadians(TargetPitch)) * HeadLookTargetDistance;
		const FVector EyeWorldTarget = CameraLocation + TitleCamera->GetForwardVector() * HeadLookTargetDistance +
			TitleCamera->GetRightVector() * EyeHorizontalOffset +
			TitleCamera->GetUpVector() * EyeVerticalOffset;
		GazeTracking->SetGazeEnabled(bEnableEyeCursorTracking);
		GazeTracking->SetGazeTargetWorldTransform(FTransform(TitleCamera->GetComponentQuat(), EyeWorldTarget));
	}
	ReceiveHeadLookUpdated(CurrentHeadLookYaw, CurrentHeadLookPitch, WorldTarget);
}

void ATunaSweeperTitlePresentationActor::UpdateCharacterPresentationState()
{
	if (bMainMenuPresentationActive || !bCharacterPresentationEnabled || !TitleCamera || !CharacterAnchor)
	{
		return;
	}

	const FVector ToCharacter =
		(CharacterAnchor->GetComponentLocation() - TitleCamera->GetComponentLocation()).GetSafeNormal();
	const float CameraDot = FVector::DotProduct(TitleCamera->GetForwardVector(), ToCharacter);
	const float HideThreshold = FMath::Cos(FMath::DegreesToRadians(CameraFieldOfView * 0.5f + 12.0f));
	if (CameraDot < HideThreshold)
	{
		SetCharacterPresentationEnabled(false);
	}
}

void ATunaSweeperTitlePresentationActor::SetCharacterPresentationEnabled(bool bEnabled)
{
	if (bCharacterPresentationEnabled == bEnabled)
	{
		return;
	}

	bCharacterPresentationEnabled = bEnabled;
	USkeletalMeshComponent* MeshComponents[] = {BodyMesh.Get(), FaceMesh.Get(), SkirtMesh.Get()};
	for (USkeletalMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}
		MeshComponent->SetVisibility(bEnabled, true);
		MeshComponent->SetComponentTickEnabled(bEnabled);
		MeshComponent->bPauseAnims = !bEnabled;
	}
}

void ATunaSweeperTitlePresentationActor::EnsureTitleCameraViewTarget()
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (PlayerController->GetViewTarget() != this)
		{
			PlayerController->SetViewTarget(this);
		}
	}
}
