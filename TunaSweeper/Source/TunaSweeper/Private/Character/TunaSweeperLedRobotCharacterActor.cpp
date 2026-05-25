#include "Character/TunaSweeperLedRobotCharacterActor.h"

#include "Component/TunaSweeperLedExpressionComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperLedRobotCharacterActor::ATunaSweeperLedRobotCharacterActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(SceneRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCastShadow(true);

	ExpressionComponent = CreateDefaultSubobject<UTunaSweeperLedExpressionComponent>(TEXT("ExpressionComponent"));
	ExpressionComponent->SetupAttachment(SceneRoot);

	BodyMeshOverride = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	ExpressionPresetFilePath = TEXT("Data/LedExpressionPresets.txt");

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void ATunaSweeperLedRobotCharacterActor::ConfigureRobotDefaults(
	FName InRobotId,
	const FString& InExpressionPresetFilePath,
	FName InInitialExpressionName,
	FLinearColor InLedColor,
	FLinearColor InOffColor,
	float InLedPitch,
	float InLedRadius,
	TSoftObjectPtr<UMaterialInterface> InBodyMaterial)
{
	RobotId = InRobotId.IsNone() ? RobotId : InRobotId;
	if (!InExpressionPresetFilePath.TrimStartAndEnd().IsEmpty())
	{
		ExpressionPresetFilePath = InExpressionPresetFilePath.TrimStartAndEnd();
	}
	if (!InInitialExpressionName.IsNone())
	{
		InitialExpressionName = InInitialExpressionName;
	}
	if (!InBodyMaterial.IsNull())
	{
		BodyMaterial = InBodyMaterial;
	}

	if (ExpressionComponent)
	{
		ExpressionComponent->ConfigureExpressionSource(ExpressionPresetFilePath, InitialExpressionName);
		ExpressionComponent->ConfigureLedAppearance(InLedColor, InOffColor, InLedPitch, InLedRadius);
		ExpressionComponent->SetExpressionByName(InitialExpressionName);
	}

	RefreshRobotVisuals();
}

bool ATunaSweeperLedRobotCharacterActor::SetExpressionByName(FName ExpressionName)
{
	return ExpressionComponent ? ExpressionComponent->SetExpressionByName(ExpressionName) : false;
}

void ATunaSweeperLedRobotCharacterActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshRobotVisuals();
}

void ATunaSweeperLedRobotCharacterActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshRobotVisuals();
	SetExpressionByName(InitialExpressionName);
}

void ATunaSweeperLedRobotCharacterActor::RefreshRobotVisuals()
{
	if (BodyMesh)
	{
		UStaticMesh* MeshToUse = BodyMeshOverride.IsNull()
			? LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"))
			: BodyMeshOverride.LoadSynchronous();
		if (MeshToUse)
		{
			BodyMesh->SetStaticMesh(MeshToUse);
		}

		if (!BodyMaterial.IsNull())
		{
			if (UMaterialInterface* LoadedBodyMaterial = BodyMaterial.LoadSynchronous())
			{
				BodyMesh->SetMaterial(0, LoadedBodyMaterial);
			}
		}

		BodyMesh->SetRelativeLocation(BodyRelativeLocation);
		BodyMesh->SetRelativeScale3D(BodyScale);
		BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BodyMesh->SetGenerateOverlapEvents(false);
	}

	if (ExpressionComponent)
	{
		ExpressionComponent->SetRelativeLocation(FaceRelativeLocation);
		ExpressionComponent->SetRelativeRotation(FaceRelativeRotation);
		ExpressionComponent->ConfigureExpressionSource(ExpressionPresetFilePath, InitialExpressionName);
	}
}
