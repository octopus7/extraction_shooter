#include "Character/TunaSweeperLedRobotCharacterActor.h"

#include "Component/TunaSweeperLedExpressionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperLedRobotCharacterActor::ATunaSweeperLedRobotCharacterActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(SceneRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCastShadow(true);

	BodyCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BodyCollision"));
	BodyCollision->SetupAttachment(SceneRoot);
	BodyCollision->SetCapsuleSize(BodyCollisionRadius, BodyCollisionHalfHeight);
	BodyCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BodyCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BodyCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BodyCollision->SetGenerateOverlapEvents(false);
	BodyCollision->CanCharacterStepUpOn = ECB_No;

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

void ATunaSweeperLedRobotCharacterActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePlayerLookAt(DeltaSeconds);
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

void ATunaSweeperLedRobotCharacterActor::ConfigureExpressionDemo(bool bEnabled, float InIntervalSeconds)
{
	bExpressionDemoMode = bEnabled;
	ExpressionDemoIntervalSeconds = FMath::Max(0.1f, InIntervalSeconds);
	if (ExpressionComponent)
	{
		ExpressionComponent->SetDemoExpressionIntervalSeconds(ExpressionDemoIntervalSeconds);
		ExpressionComponent->SetDemoModeEnabled(bExpressionDemoMode);
	}
}

void ATunaSweeperLedRobotCharacterActor::SetExpressionDemoModeEnabled(bool bEnabled)
{
	ConfigureExpressionDemo(bEnabled, ExpressionDemoIntervalSeconds);
}

void ATunaSweeperLedRobotCharacterActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshRobotVisuals();
}

void ATunaSweeperLedRobotCharacterActor::BeginPlay()
{
	Super::BeginPlay();
	IdleActorRotation = GetActorRotation();
	PendingLookAtYaw = IdleActorRotation.Yaw;
	LookAtReactionDelay = FMath::FRandRange(LookAtMinReactionDelay, FMath::Max(LookAtMinReactionDelay, LookAtMaxReactionDelay));
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

	if (BodyCollision)
	{
		BodyCollision->SetRelativeLocation(BodyRelativeLocation);
		BodyCollision->SetCapsuleSize(
			FMath::Max(1.0f, BodyCollisionRadius),
			FMath::Max(1.0f, BodyCollisionHalfHeight));
		BodyCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BodyCollision->SetCollisionObjectType(ECC_WorldDynamic);
		BodyCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		BodyCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		BodyCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		BodyCollision->SetGenerateOverlapEvents(false);
		BodyCollision->CanCharacterStepUpOn = ECB_No;
	}

	if (ExpressionComponent)
	{
		ExpressionComponent->SetRelativeLocation(FaceRelativeLocation);
		ExpressionComponent->SetRelativeRotation(FaceRelativeRotation);
		ExpressionComponent->ConfigureExpressionSource(ExpressionPresetFilePath, InitialExpressionName);
		ExpressionComponent->SetDemoExpressionIntervalSeconds(ExpressionDemoIntervalSeconds);
		ExpressionComponent->SetDemoModeEnabled(bExpressionDemoMode);
	}
}

void ATunaSweeperLedRobotCharacterActor::UpdatePlayerLookAt(float DeltaSeconds)
{
	if (!bLookAtNearbyPlayer || DeltaSeconds <= 0.0f)
	{
		return;
	}

	float PlayerYaw = 0.0f;
	float PlayerDistance2D = 0.0f;
	const bool bHasPlayer = TryGetPlayerLookYaw(PlayerYaw, PlayerDistance2D);
	const float StartDistance = FMath::Max(0.0f, LookAtStartDistance);
	const float StopDistance = FMath::Max(StartDistance, LookAtStopDistance);

	if (!bIsLookingAtPlayer)
	{
		if (bHasPlayer && PlayerDistance2D <= StartDistance)
		{
			if (!bLookAtReactionPending)
			{
				bLookAtReactionPending = true;
				LookAtReactionElapsed = 0.0f;
				LookAtReactionDelay = FMath::FRandRange(
					FMath::Max(0.0f, LookAtMinReactionDelay),
					FMath::Max(LookAtMinReactionDelay, LookAtMaxReactionDelay));
			}

			LookAtReactionElapsed += DeltaSeconds;
			if (LookAtReactionElapsed >= LookAtReactionDelay)
			{
				bIsLookingAtPlayer = true;
				bLookAtReactionPending = false;
				LookAtRefreshElapsed = LookAtTargetRefreshInterval;
			}
		}
		else
		{
			bLookAtReactionPending = false;
			LookAtReactionElapsed = 0.0f;
		}
	}
	else if (!bHasPlayer || PlayerDistance2D >= StopDistance)
	{
		bIsLookingAtPlayer = false;
		bLookAtReactionPending = false;
		LookAtReactionElapsed = 0.0f;
	}

	float DesiredYaw = IdleActorRotation.Yaw;
	float InterpSpeed = LookAtReturnInterpolationSpeed;
	if (bIsLookingAtPlayer && bHasPlayer)
	{
		LookAtRefreshElapsed += DeltaSeconds;
		if (LookAtRefreshElapsed >= FMath::Max(0.01f, LookAtTargetRefreshInterval))
		{
			PendingLookAtYaw = PlayerYaw;
			LookAtRefreshElapsed = 0.0f;
		}

		DesiredYaw = PendingLookAtYaw + ResolveNonMechanicalYawOffset(DeltaSeconds);
		InterpSpeed = LookAtInterpolationSpeed;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation(CurrentRotation.Pitch, DesiredYaw, CurrentRotation.Roll);
	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaSeconds,
		FMath::Max(0.0f, InterpSpeed));
	SetActorRotation(NewRotation);
}

bool ATunaSweeperLedRobotCharacterActor::TryGetPlayerLookYaw(float& OutYaw, float& OutDistance2D) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn)
	{
		return false;
	}

	const FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
	const FVector ToPlayer2D(ToPlayer.X, ToPlayer.Y, 0.0f);
	OutDistance2D = ToPlayer2D.Size();
	if (OutDistance2D <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutYaw = ToPlayer2D.Rotation().Yaw;
	return true;
}

float ATunaSweeperLedRobotCharacterActor::ResolveNonMechanicalYawOffset(float DeltaSeconds)
{
	const float MaxOffset = FMath::Max(0.0f, LookAtYawOffsetDegrees);
	if (MaxOffset <= 0.0f)
	{
		LookAtYawOffset = 0.0f;
		LookAtYawOffsetTarget = 0.0f;
		return 0.0f;
	}

	LookAtYawOffsetRefreshElapsed += DeltaSeconds;
	if (LookAtYawOffsetRefreshElapsed >= 1.2f)
	{
		LookAtYawOffsetTarget = FMath::FRandRange(-MaxOffset, MaxOffset);
		LookAtYawOffsetRefreshElapsed = 0.0f;
	}

	LookAtYawOffset = FMath::FInterpTo(LookAtYawOffset, LookAtYawOffsetTarget, DeltaSeconds, 1.2f);
	return LookAtYawOffset;
}
