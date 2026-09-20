#include "Title/TunaSweeperTitlePresentationActor.h"

#include "Animation/AnimInstance.h"
#include "Component/TunaSweeperGazeTrackingComponent.h"
#include "ReferenceSkeleton.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "UObject/ConstructorHelpers.h"

namespace TunaSweeperTitlePresentation
{
	const FName LunaMk2LeftEyeBoneName(TEXT("eye_l"));
	const FName LunaMk2RightEyeBoneName(TEXT("eye_r"));
}

void UTunaSweeperTitleSkeletalMeshComponent::SetDirectHeadLookRotation(float YawDegrees, float PitchDegrees)
{
	DirectHeadLookYaw = YawDegrees;
	DirectHeadLookPitch = PitchDegrees;
	bHasDirectHeadLookTarget = true;
}

void UTunaSweeperTitleSkeletalMeshComponent::ClearDirectHeadLookRotation()
{
	bHasDirectHeadLookTarget = false;
	DirectHeadLookYaw = 0.0f;
	DirectHeadLookPitch = 0.0f;
}

bool UTunaSweeperTitleSkeletalMeshComponent::GetDirectHeadLookRequest(
	float& OutYaw, float& OutPitch, FName& OutHeadBone, FName& OutRootBone) const
{
	OutYaw = DirectHeadLookYaw;
	OutPitch = DirectHeadLookPitch;
	OutHeadBone = HeadBoneName;
	OutRootBone = HeadLookRootBoneName;
	return bApplyDirectHeadLook && bHasDirectHeadLookTarget;
}

void UTunaSweeperTitleSkeletalMeshComponent::SetTemporaryRelaxedArmPose(
	float BlendAlpha,
	float MotionPhaseSeconds)
{
	TemporaryRelaxedArmBlendAlpha = FMath::Clamp(BlendAlpha, 0.0f, 1.0f);
	TemporaryRelaxedArmMotionPhase = MotionPhaseSeconds;
}

void UTunaSweeperTitleSkeletalMeshComponent::SetTemporaryRelaxedArmPoseEnabled(bool bEnabled)
{
	bApplyTemporaryRelaxedArms = bEnabled;
}

void UTunaSweeperTitleSkeletalMeshComponent::FinalizeBoneTransform()
{
	ApplyTemporaryRelaxedArmPoseToEditablePose();
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
	BodyMesh->AddTickPrerequisiteActor(this);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
	BodyMesh->SetTemporaryRelaxedArmPoseEnabled(false);

	FaceMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FaceMesh"));
	FaceMesh->SetupAttachment(BodyMesh, FaceAttachmentSocketName);
	FaceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FaceMesh->SetGenerateOverlapEvents(false);
	FaceMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	SkirtMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Skirt"));
	// The title skirt is skinned in BodyMesh space; Copy Pose supplies pelvis/spine motion.
	SkirtMesh->SetupAttachment(BodyMesh);
	SkirtMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkirtMesh->SetGenerateOverlapEvents(false);
	SkirtMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;

	HeadLookTarget = CreateDefaultSubobject<USceneComponent>(TEXT("HeadLookTarget"));
	HeadLookTarget->SetupAttachment(SceneRoot);

	GazeTracking = CreateDefaultSubobject<UTunaSweeperGazeTrackingComponent>(TEXT("GazeTracking"));
	GazeTracking->SetupAttachment(SceneRoot);
	GazeTracking->AddTickPrerequisiteActor(this);
	GazeTracking->SetEyeBoneNames(
		TunaSweeperTitlePresentation::LunaMk2LeftEyeBoneName,
		TunaSweeperTitlePresentation::LunaMk2RightEyeBoneName);
	GazeTracking->SetEyeAxes(FVector::RightVector, FVector::UpVector);

	LeftEyeTarget = CreateDefaultSubobject<USceneComponent>(TEXT("LeftEyeTarget"));
	LeftEyeTarget->SetupAttachment(GazeTracking);
	LeftEyeTarget->SetRelativeLocation(FVector(0.0f, -3.2f, 0.0f));

	RightEyeTarget = CreateDefaultSubobject<USceneComponent>(TEXT("RightEyeTarget"));
	RightEyeTarget->SetupAttachment(GazeTracking);
	RightEyeTarget->SetRelativeLocation(FVector(0.0f, 3.2f, 0.0f));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> BodyMeshFinder(
		TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2.SKM_LunaMk2"));
	if (BodyMeshFinder.Succeeded())
	{
		BodyMesh->SetSkeletalMeshAsset(BodyMeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> BodyAnimClassFinder(
		TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/ABP_LunaMk2_Title"));
	if (BodyAnimClassFinder.Succeeded())
	{
		BodyMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		BodyMesh->SetAnimInstanceClass(BodyAnimClassFinder.Class);
	}

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkirtMeshFinder(
		TEXT("/Game/Characters/Player/LunaMk2/Skirt/SKM_LunaMk2_TitleSkirt.SKM_LunaMk2_TitleSkirt"));
	if (SkirtMeshFinder.Succeeded())
	{
		SkirtMesh->SetSkeletalMeshAsset(SkirtMeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> SkirtAnimClassFinder(
		TEXT("/Game/Characters/Player/LunaMk2/Skirt/ABP_LunaMk2_TitleSkirt"));
	if (SkirtAnimClassFinder.Succeeded())
	{
		SkirtMesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);
		SkirtMesh->SetAnimInstanceClass(SkirtAnimClassFinder.Class);
	}

	SkirtBodyCollisionProxyPhysicsAsset = TSoftObjectPtr<UPhysicsAsset>(FSoftObjectPath(
		TEXT("/Game/Characters/Player/Luna/Skirt/PA_Luna_SkirtBodyProxy.PA_Luna_SkirtBodyProxy")));

	UActorComponent* BlueprintEditableComponents[] = {
		TitleCamera,
		CharacterAnchor,
		BodyMesh,
		FaceMesh,
		SkirtMesh,
		HeadLookTarget,
		GazeTracking,
		LeftEyeTarget,
		RightEyeTarget};
	for (UActorComponent* Component : BlueprintEditableComponents)
	{
		if (Component)
		{
			Component->bEditableWhenInherited = true;
		}
	}

	ApplyDesignTransforms();
	ConfigureTitleExposure();
}

void ATunaSweeperTitlePresentationActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ConfigureTitleExposure();
	ConfigureSkirtAttachment();
	if (BodyMesh)
	{
		BodyMesh->SetTemporaryRelaxedArmPoseEnabled(false);
	}

	if (FaceMesh && BodyMesh)
	{
		FaceMesh->AttachToComponent(
			BodyMesh,
			FAttachmentTransformRules::KeepRelativeTransform,
			FaceAttachmentSocketName);
	}
	if (GazeTracking)
	{
		GazeTracking->SetTrackedMesh(BodyMesh);
		GazeTracking->SetEyeTargetComponents(LeftEyeTarget, RightEyeTarget);
		GazeTracking->SetEyeBoneNames(
			TunaSweeperTitlePresentation::LunaMk2LeftEyeBoneName,
			TunaSweeperTitlePresentation::LunaMk2RightEyeBoneName);
		GazeTracking->SetEyeAxes(FVector::RightVector, FVector::UpVector);
	}
}

void ATunaSweeperTitlePresentationActor::BeginPlay()
{
	Super::BeginPlay();
	ConfigureTitleExposure();
	if (TitleCamera)
	{
		MainMenuCameraLocation = TitleCamera->GetRelativeLocation();
		MainMenuCameraRotation = TitleCamera->GetRelativeRotation();
		CameraFieldOfView = TitleCamera->FieldOfView;
	}
	if (CharacterAnchor)
	{
		CharacterRelativeLocation = CharacterAnchor->GetRelativeLocation();
		CharacterRelativeRotation = CharacterAnchor->GetRelativeRotation();
	}
	ConfigureSkirtAttachment();
	if (BodyMesh)
	{
		BodyMesh->SetTemporaryRelaxedArmPoseEnabled(false);
	}
	if (GazeTracking)
	{
		GazeTracking->SetTrackedMesh(BodyMesh);
		GazeTracking->SetEyeTargetComponents(LeftEyeTarget, RightEyeTarget);
		GazeTracking->SetEyeBoneNames(
			TunaSweeperTitlePresentation::LunaMk2LeftEyeBoneName,
			TunaSweeperTitlePresentation::LunaMk2RightEyeBoneName);
		GazeTracking->SetEyeAxes(FVector::RightVector, FVector::UpVector);
		GazeTracking->SetGazeEnabled(
			!bTemporarilyDisableHeadAndEyeCorrection && bEnableEyeCursorTracking);
	}
	if (bTemporarilyDisableHeadAndEyeCorrection && BodyMesh)
	{
		CurrentHeadLookYaw = 0.0f;
		CurrentHeadLookPitch = 0.0f;
		BodyMesh->ClearDirectHeadLookRotation();
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
	if (bTemporarilyDisableHeadAndEyeCorrection)
	{
		CurrentHeadLookYaw = 0.0f;
		CurrentHeadLookPitch = 0.0f;
		if (BodyMesh)
		{
			BodyMesh->ClearDirectHeadLookRotation();
		}
		if (GazeTracking)
		{
			GazeTracking->SetGazeEnabled(false);
		}
	}
	else if (bCharacterPresentationEnabled && bMainMenuPresentationActive)
	{
		UpdateCursorLook(DeltaSeconds);
	}
}

void ATunaSweeperTitlePresentationActor::SetMainMenuPresentationActive(bool bActive)
{
	bMainMenuPresentationActive = bActive;
	if (GazeTracking)
	{
		GazeTracking->SetGazeEnabled(
			bActive && !bTemporarilyDisableHeadAndEyeCorrection && bEnableEyeCursorTracking);
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
	if (MorphTargetName.IsNone())
	{
		return;
	}

	const float ClampedWeight = FMath::Clamp(Weight, 0.0f, 1.0f);
	if (BodyMesh)
	{
		BodyMesh->SetMorphTarget(MorphTargetName, ClampedWeight);
	}
	if (FaceMesh && FaceMesh->GetSkeletalMeshAsset())
	{
		FaceMesh->SetMorphTarget(MorphTargetName, ClampedWeight);
	}
}

void ATunaSweeperTitlePresentationActor::ClearFacialWeights()
{
	if (BodyMesh)
	{
		BodyMesh->ClearMorphTargets();
	}
	if (FaceMesh)
	{
		FaceMesh->ClearMorphTargets();
	}
}

FVector ATunaSweeperTitlePresentationActor::GetHeadLookTargetLocation() const
{
	return HeadLookTarget ? HeadLookTarget->GetComponentLocation() : GetActorLocation();
}

FVector ATunaSweeperTitlePresentationActor::CalculateCursorTargetWorldLocation(
	const FVector& CursorRayOrigin,
	const FVector& CursorRayDirection,
	const FVector& EyeCenterWorldLocation,
	float FrontOffset,
	float MinimumDistance)
{
	const FVector NormalizedDirection = CursorRayDirection.GetSafeNormal();
	if (NormalizedDirection.IsNearlyZero())
	{
		return CursorRayOrigin;
	}

	const float EyeDepth = FVector::DotProduct(EyeCenterWorldLocation - CursorRayOrigin, NormalizedDirection);
	const float TargetDistance = FMath::Max(MinimumDistance, EyeDepth - FMath::Max(0.0f, FrontOffset));
	return CursorRayOrigin + NormalizedDirection * TargetDistance;
}

void ATunaSweeperTitlePresentationActor::ConfigureTitleExposure()
{
	if (!TitleCamera) return;
	FPostProcessSettings& Settings = TitleCamera->PostProcessSettings;
	Settings.bOverride_AutoExposureMethod = true;
	Settings.AutoExposureMethod = AEM_Manual;
	Settings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Settings.AutoExposureApplyPhysicalCameraExposure = false;
	Settings.bOverride_AutoExposureBias = true;
	Settings.AutoExposureBias = TitleExposureCompensation;
	Settings.bOverride_LocalExposureHighlightContrastScale = true;
	Settings.LocalExposureHighlightContrastScale = 1.0f;
	Settings.bOverride_LocalExposureShadowContrastScale = true;
	Settings.LocalExposureShadowContrastScale = 1.0f;
	TitleCamera->PostProcessBlendWeight = 1.0f;
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

void ATunaSweeperTitlePresentationActor::ConfigureSkirtAttachment()
{
	if (!BodyMesh || !SkirtMesh)
	{
		return;
	}

	SkirtMesh->AttachToComponent(
		BodyMesh,
		FAttachmentTransformRules::SnapToTargetIncludingScale);
	// Copy Pose must read this frame's body transforms before its own RigidBody evaluation.
	SkirtMesh->AddTickPrerequisiteComponent(BodyMesh);
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
	if (BodyMesh && !bEnableHeadCursorTracking)
	{
		BodyMesh->ClearDirectHeadLookRotation();
	}
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!PlayerController || !TitleCamera || !BodyMesh)
	{
		return;
	}

	FVector CursorRayOrigin = FVector::ZeroVector;
	FVector CursorRayDirection = FVector::ForwardVector;
	if (!PlayerController->DeprojectMousePositionToWorld(CursorRayOrigin, CursorRayDirection))
	{
		return;
	}

	const FVector LeftEyeLocation = BodyMesh->GetBoneLocation(TunaSweeperTitlePresentation::LunaMk2LeftEyeBoneName);
	const FVector RightEyeLocation = BodyMesh->GetBoneLocation(TunaSweeperTitlePresentation::LunaMk2RightEyeBoneName);
	const FVector EyeCenter = (LeftEyeLocation + RightEyeLocation) * 0.5f;
	const FVector WorldTarget = CalculateCursorTargetWorldLocation(
		CursorRayOrigin,
		CursorRayDirection,
		EyeCenter,
		CursorTargetFrontOffset,
		MinimumCursorTargetDistance);

	const FVector HeadLocation = BodyMesh->GetBoneLocation(TEXT("Head"));
	const FVector LocalLookDirection = BodyMesh->GetComponentTransform()
		.InverseTransformVectorNoScale(WorldTarget - HeadLocation)
		.GetSafeNormal();
	if (LocalLookDirection.IsNearlyZero())
	{
		return;
	}
	const float TargetYaw = FMath::Clamp(
		FMath::RadiansToDegrees(FMath::Atan2(LocalLookDirection.X, LocalLookDirection.Y)),
		-MaxHeadLookYaw,
		MaxHeadLookYaw);
	const float HorizontalMagnitude = FVector2D(LocalLookDirection.X, LocalLookDirection.Y).Size();
	const float TargetPitch = FMath::Clamp(
		FMath::RadiansToDegrees(FMath::Atan2(LocalLookDirection.Z, HorizontalMagnitude)),
		-MaxHeadLookPitch,
		MaxHeadLookPitch);
	const float DesiredHeadYaw = bEnableHeadCursorTracking ? TargetYaw : 0.0f;
	const float DesiredHeadPitch = bEnableHeadCursorTracking ? TargetPitch : 0.0f;
	CurrentHeadLookYaw = FMath::FInterpTo(
		CurrentHeadLookYaw, DesiredHeadYaw, DeltaSeconds, HeadLookInterpolationSpeed);
	CurrentHeadLookPitch = FMath::FInterpTo(
		CurrentHeadLookPitch, DesiredHeadPitch, DeltaSeconds, HeadLookInterpolationSpeed);
	if (bEnableHeadCursorTracking)
	{
		BodyMesh->SetDirectHeadLookRotation(CurrentHeadLookYaw, CurrentHeadLookPitch);
	}

	if (HeadLookTarget)
	{
		HeadLookTarget->SetWorldLocation(WorldTarget);
	}

	if (GazeTracking)
	{
		GazeTracking->SetGazeEnabled(bEnableEyeCursorTracking);
		GazeTracking->SetGazeTargetWorldTransform(FTransform(CursorRayDirection.Rotation(), WorldTarget));
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
