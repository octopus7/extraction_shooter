#include "Title/TunaSweeperTitleStudioActor.h"
#include "Title/TunaSweeperTitlePresentationActor.h"
#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/LocalPlayer.h"
#include "SceneView.h"
#include "GameFramework/PlayerController.h"
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

ATunaSweeperTitleStudioActor::ATunaSweeperTitleStudioActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;
	SetCanBeDamaged(false);
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
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
	AmbientLight = CreateDefaultSubobject<USkyLightComponent>(TEXT("AmbientLight"));
	AmbientLight->SetupAttachment(SceneRoot);
	AmbientLight->SetMobility(EComponentMobility::Movable);
	AmbientLight->SetIntensity(0.8f);

	CharacterKeyLight = CreateDefaultSubobject<USpotLightComponent>(TEXT("CharacterKeyLight"));
	CharacterKeyLight->SetupAttachment(SceneRoot);
	CharacterKeyLight->SetMobility(EComponentMobility::Movable);
	CharacterKeyLight->SetIntensity(5200.0f);
	CharacterKeyLight->SetAttenuationRadius(1150.0f);
	CharacterKeyLight->SetLightColor(FLinearColor(1.0f, 0.82f, 0.68f));
	CharacterKeyLight->SetInnerConeAngle(20.0f);
	CharacterKeyLight->SetOuterConeAngle(28.0f);
	CharacterKeyLight->SetSourceRadius(22.0f);

	EmptyWallLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("EmptyWallLight"));
	EmptyWallLight->SetupAttachment(SceneRoot);
	EmptyWallLight->SetMobility(EComponentMobility::Movable);
	EmptyWallLight->SetIntensity(3200.0f);
	EmptyWallLight->SetAttenuationRadius(1050.0f);
	EmptyWallLight->SetLightColor(FLinearColor(0.48f, 0.68f, 1.0f));
	// Fill lifts the shaded side without adding a second hard self-shadow.
	EmptyWallLight->SetCastShadows(false);
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
	MatteBackdrop = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MatteBackdrop"));
	MatteBackdrop->SetupAttachment(SceneRoot);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	MatteBackdrop->SetStaticMesh(Plane.Object);
	MatteBackdrop->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MatteBackdrop->SetGenerateOverlapEvents(false);
	MatteBackdrop->SetMobility(EComponentMobility::Movable);
	MatteBackdrop->CastShadow = false;
	MatteBackdrop->bAffectDynamicIndirectLighting = false;
	MatteBackdrop->bAffectDistanceFieldLighting = false;
	for (UStaticMeshComponent* Part : {BackWall.Get(), LeftWall.Get(), RightWall.Get(), Floor.Get()})
	{
		Part->SetVisibility(false);
		Part->CastShadow = false;
	}
}

void ATunaSweeperTitleStudioActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateBackdrop();
}

void ATunaSweeperTitleStudioActor::BeginPlay()
{
	Super::BeginPlay();
	if (IsValid(PresentationActor)) AddTickPrerequisiteActor(PresentationActor);
	UpdateBackdrop();
}

void ATunaSweeperTitleStudioActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateBackdrop();
}

void ATunaSweeperTitleStudioActor::UpdateBackdrop()
{
	for (UStaticMeshComponent* Part : {BackWall.Get(), LeftWall.Get(), RightWall.Get(), Floor.Get()})
	{
		Part->SetVisibility(bShowStudioGeometry);
		Part->SetCastShadow(bShowStudioGeometry);
	}
	MatteBackdrop->SetVisibility(!bShowStudioGeometry);
	if (!IsValid(PresentationActor)) return;
	const auto* Body = PresentationActor->FindComponentByClass<UTunaSweeperTitleSkeletalMeshComponent>();
	if (Body)
	{
		// Aim at the mesh, not the presentation actor's camera-relative origin.
		// The cone covers the full idle/entrance motion while concentrating VSM resolution.
		const FVector Target = Body->GetComponentLocation() + FVector(0.0f, 0.0f, 100.0f);
		CharacterKeyLight->SetWorldRotation((Target - CharacterKeyLight->GetComponentLocation()).Rotation());
	}
	UCameraComponent* Camera = PresentationActor->FindComponentByClass<UCameraComponent>();
	if (!Camera) return;
	if (Body)
	{
		// Keep the face in focus as the title camera moves between menu views.
		const FVector FocusPoint = Body->GetSocketLocation(TEXT("head"));
		FPostProcessSettings& Settings = Camera->PostProcessSettings;
		Settings.bOverride_DepthOfFieldFocalDistance = true;
		Settings.DepthOfFieldFocalDistance = FMath::Max(1.0f, FVector::DotProduct(
			FocusPoint - Camera->GetComponentLocation(), Camera->GetForwardVector()));
		Settings.bOverride_DepthOfFieldFstop = true;
		Settings.DepthOfFieldFstop = FMath::Clamp(BackdropFStop, 1.0f, 22.0f);
		Settings.bOverride_DepthOfFieldSensorWidth = true;
		Settings.DepthOfFieldSensorWidth = 36.0f;
	}
	FIntPoint ViewportSize(1920, FMath::RoundToInt(1920.f / FMath::Max(Camera->AspectRatio, 0.1f)));
	EAspectRatioAxisConstraint AxisConstraint = GetDefault<ULocalPlayer>()->AspectRatioAxisConstraint;
	if (const UWorld* World = GetWorld())
	{
		if (const APlayerController* Controller = World->GetFirstPlayerController())
		{
			int32 Width = 0, Height = 0;
			Controller->GetViewportSize(Width, Height);
			if (Width > 0 && Height > 0) ViewportSize = FIntPoint(Width, Height);
			if (const ULocalPlayer* Player = Controller->GetLocalPlayer()) AxisConstraint = Player->AspectRatioAxisConstraint;
		}
	}
	constexpr float Distance = 3000.0f;
	const FVector2D Size = CalculateBackdropSize(Camera, ViewportSize, AxisConstraint);
	// Plane X/Y span camera right/down; the normal points back toward the lens.
	MatteBackdrop->SetWorldLocation(Camera->GetComponentLocation() + Camera->GetForwardVector() * Distance);
	MatteBackdrop->SetWorldRotation(FRotationMatrix::MakeFromXY(Camera->GetRightVector(), -Camera->GetUpVector()).ToQuat());
	MatteBackdrop->SetWorldScale3D(FVector(Size.X / 100.0f, Size.Y / 100.0f, FMath::Min(Size.X, Size.Y) / 100.0f));
}

FVector2D ATunaSweeperTitleStudioActor::CalculateBackdropSize(const UCameraComponent* Camera,
	FIntPoint ViewportSize, EAspectRatioAxisConstraint AxisConstraint)
{
	FMinimalViewInfo View;
	View.FOV = Camera->FieldOfView;
	View.AspectRatio = Camera->AspectRatio;
	View.bConstrainAspectRatio = Camera->bConstrainAspectRatio;
	if (Camera->bOverrideAspectRatioAxisConstraint) View.AspectRatioAxisConstraint = Camera->AspectRatioAxisConstraint;
	FSceneViewProjectionData Projection;
	const FIntRect Rect(0, 0, FMath::Max(ViewportSize.X, 1), FMath::Max(ViewportSize.Y, 1));
	Projection.SetViewRectangle(Rect);
	FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle(View, AxisConstraint, Rect, Projection);
	const float ViewWidth = 6000.f / Projection.ProjectionMatrix.M[0][0];
	const float ViewHeight = 6000.f / Projection.ProjectionMatrix.M[1][1];
	// Projection mapping handles image framing; geometry provides an overscanned curved screen.
	return FVector2D(ViewWidth, ViewHeight) * 1.08f;
}
