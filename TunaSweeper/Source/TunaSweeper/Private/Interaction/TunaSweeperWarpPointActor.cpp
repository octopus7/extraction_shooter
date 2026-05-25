#include "Interaction/TunaSweeperWarpPointActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Materials/MaterialInterface.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

ATunaSweeperWarpPointActor::ATunaSweeperWarpPointActor()
{
	WarpPointId = NAME_None;
	TargetWarpPointId = NAME_None;
	ExitOffset = FVector(160.0f, 0.0f, 0.0f);
	bUseTargetRotation = true;
	WarpPointVisualScale = FVector(1.2f, 1.2f, 1.2f);
	WarpPointVisualRelativeLocation = FVector::ZeroVector;
	VisualMeshOverride = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere")));
	VisualMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Interaction/M_WarpPointEnergy.M_WarpPointEnergy")));

	if (VisualMesh)
	{
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		VisualMesh->SetGenerateOverlapEvents(false);
		VisualMesh->SetCastShadow(false);
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 145.0f));
		InteractableComponent->SetInteractionTypeAndDisplayName(
			ETunaSweeperInteractionType::WarpPoint,
			FText::FromString(TEXT("\uC0C1\uD638\uC791\uC6A9")));
	}
}

void ATunaSweeperWarpPointActor::ConfigureWarpPointDefaults(
	FName InWarpPointId,
	FName InTargetWarpPointId,
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	TSoftObjectPtr<UMaterialInterface> InVisualMaterial,
	TSoftObjectPtr<UStaticMesh> InVisualMesh,
	FVector InVisualScale,
	FVector InVisualRelativeLocation,
	FVector InExitOffset,
	bool bInUseTargetRotation)
{
	Modify();
	WarpPointId = InWarpPointId;
	TargetWarpPointId = InTargetWarpPointId;
	if (!InVisualMaterial.IsNull())
	{
		VisualMaterial = InVisualMaterial;
	}
	if (!InVisualMesh.IsNull())
	{
		VisualMeshOverride = InVisualMesh;
	}
	WarpPointVisualScale = InVisualScale;
	WarpPointVisualRelativeLocation = InVisualRelativeLocation;
	ExitOffset = InExitOffset;
	bUseTargetRotation = bInUseTargetRotation;
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::WarpPoint, InInteractionDisplayName, InMarkerWidgetClass);
	RefreshWarpPointVisual();
}

void ATunaSweeperWarpPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshWarpPointVisual();
}

void ATunaSweeperWarpPointActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshWarpPointVisual();
}

void ATunaSweeperWarpPointActor::RefreshWarpPointVisual()
{
	if (!VisualMesh)
	{
		return;
	}

	UStaticMesh* MeshToUse = VisualMeshOverride.IsNull()
		? LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"))
		: VisualMeshOverride.LoadSynchronous();
	if (MeshToUse)
	{
		VisualMesh->SetStaticMesh(MeshToUse);
	}

	if (!VisualMaterial.IsNull())
	{
		if (UMaterialInterface* LoadedMaterial = VisualMaterial.LoadSynchronous())
		{
			VisualMesh->SetMaterial(0, LoadedMaterial);
		}
	}

	VisualMesh->SetRelativeScale3D(WarpPointVisualScale);
	VisualMesh->SetRelativeLocation(WarpPointVisualRelativeLocation);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetGenerateOverlapEvents(false);
	VisualMesh->SetCastShadow(false);
}

ATunaSweeperWarpPointActor* ATunaSweeperWarpPointActor::ResolveTargetWarpPoint() const
{
	UWorld* World = GetWorld();
	if (!World || TargetWarpPointId.IsNone())
	{
		return nullptr;
	}

	for (TActorIterator<ATunaSweeperWarpPointActor> WarpPointIt(World); WarpPointIt; ++WarpPointIt)
	{
		ATunaSweeperWarpPointActor* Candidate = *WarpPointIt;
		if (Candidate && Candidate != this && Candidate->GetWarpPointId() == TargetWarpPointId)
		{
			return Candidate;
		}
	}

	return nullptr;
}

bool ATunaSweeperWarpPointActor::WarpInstigator(APawn* InstigatorPawn)
{
	if (!InstigatorPawn)
	{
		return false;
	}

	ATunaSweeperWarpPointActor* TargetWarpPoint = ResolveTargetWarpPoint();
	if (!TargetWarpPoint)
	{
		return false;
	}

	if (UPawnMovementComponent* MovementComponent = InstigatorPawn->GetMovementComponent())
	{
		MovementComponent->StopMovementImmediately();
	}

	if (AController* Controller = InstigatorPawn->GetController())
	{
		Controller->StopMovement();
	}

	const FRotator TargetRotation = bUseTargetRotation
		? TargetWarpPoint->GetActorRotation()
		: InstigatorPawn->GetActorRotation();
	const FVector TargetLocation =
		TargetWarpPoint->GetActorLocation() +
		TargetWarpPoint->GetActorRotation().RotateVector(TargetWarpPoint->ExitOffset);

	bool bWarped = InstigatorPawn->TeleportTo(TargetLocation, TargetRotation, false, true);
	if (!bWarped)
	{
		bWarped = InstigatorPawn->SetActorLocationAndRotation(
			TargetLocation,
			TargetRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	if (!bWarped)
	{
		return false;
	}

	if (bUseTargetRotation)
	{
		if (AController* Controller = InstigatorPawn->GetController())
		{
			Controller->SetControlRotation(TargetRotation);
		}
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyWarpPointUsed(
				GetWorld() ? FName(*GetWorld()->GetMapName()) : NAME_None,
				WarpPointId,
				TargetWarpPoint->GetWarpPointId());
		}
	}

	return true;
}
