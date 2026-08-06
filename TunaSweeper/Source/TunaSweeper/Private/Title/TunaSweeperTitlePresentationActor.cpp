#include "Title/TunaSweeperTitlePresentationActor.h"

#include "Animation/AnimInstance.h"
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

void UTunaSweeperTitleSkeletalMeshComponent::FinalizeBoneTransform()
{
	ApplyDirectHeadLookToEditablePose();
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

	HeadLookTarget = CreateDefaultSubobject<USceneComponent>(TEXT("HeadLookTarget"));
	HeadLookTarget->SetupAttachment(SceneRoot);

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
}

void ATunaSweeperTitlePresentationActor::BeginPlay()
{
	Super::BeginPlay();
	SetMainMenuPresentationActive(true);
}

void ATunaSweeperTitlePresentationActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

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
	CurrentHeadLookYaw = FMath::FInterpTo(
		CurrentHeadLookYaw, TargetYaw, DeltaSeconds, HeadLookInterpolationSpeed);
	CurrentHeadLookPitch = FMath::FInterpTo(
		CurrentHeadLookPitch, TargetPitch, DeltaSeconds, HeadLookInterpolationSpeed);
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
	USkeletalMeshComponent* MeshComponents[] = {BodyMesh.Get(), FaceMesh.Get()};
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
