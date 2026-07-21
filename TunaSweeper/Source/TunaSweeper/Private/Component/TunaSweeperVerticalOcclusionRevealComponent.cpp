#include "Component/TunaSweeperVerticalOcclusionRevealComponent.h"

#include "Components/MeshComponent.h"
#include "Effect/TunaSweeperOcclusionRevealSettingsDataAsset.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/TunaSweeperPlayerController.h"

namespace TunaSweeperVerticalOcclusionReveal
{
	const FName RevealActiveName(TEXT("VerticalRevealActive"));
	const FName RevealStartZName(TEXT("VerticalRevealStartZ"));
	const FName RevealFadeHeightName(TEXT("VerticalRevealFadeHeightCm"));
}

UTunaSweeperVerticalOcclusionRevealComponent::UTunaSweeperVerticalOcclusionRevealComponent()
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	VerticalRevealMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/M_OcclusionVerticalRevealMasked.M_OcclusionVerticalRevealMasked")));
}

void UTunaSweeperVerticalOcclusionRevealComponent::OnRegister()
{
	Super::OnRegister();
	ApplyVerticalRevealMaterial();
}

void UTunaSweeperVerticalOcclusionRevealComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyVerticalRevealMaterial();
}

void UTunaSweeperVerticalOcclusionRevealComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateRevealParameters();
}

void UTunaSweeperVerticalOcclusionRevealComponent::CollectRevealMeshes(TArray<UMeshComponent*>& OutMeshes) const
{
	OutMeshes.Reset();
	for (UMeshComponent* MeshComponent : RevealMeshes)
	{
		if (MeshComponent)
		{
			OutMeshes.AddUnique(MeshComponent);
		}
	}

	if (bAutoCollectOwnerMeshes && GetOwner())
	{
		TArray<UMeshComponent*> OwnerMeshes;
		GetOwner()->GetComponents(OwnerMeshes);
		for (UMeshComponent* MeshComponent : OwnerMeshes)
		{
			if (MeshComponent)
			{
				OutMeshes.AddUnique(MeshComponent);
			}
		}
	}
}

void UTunaSweeperVerticalOcclusionRevealComponent::ApplyVerticalRevealMaterial()
{
	UMaterialInterface* Material = VerticalRevealMaterial.LoadSynchronous();
	if (!Material)
	{
		return;
	}

	TArray<UMeshComponent*> Meshes;
	CollectRevealMeshes(Meshes);
	for (UMeshComponent* MeshComponent : Meshes)
	{
		if (!MeshComponent || MaterialStates.ContainsByPredicate([MeshComponent](const FVerticalOcclusionRevealMaterialState& State)
			{ return State.MeshComponent == MeshComponent; }))
		{
			continue;
		}

		FVerticalOcclusionRevealMaterialState& State = MaterialStates.AddDefaulted_GetRef();
		State.MeshComponent = MeshComponent;
		for (int32 MaterialIndex = 0; MaterialIndex < MeshComponent->GetNumMaterials(); ++MaterialIndex)
		{
			State.OriginalMaterials.Add(MeshComponent->GetMaterial(MaterialIndex));
			if (bOverrideAllMaterialSlots || MaterialIndex == 0)
			{
				if (UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateDynamicMaterialInstance(MaterialIndex, Material))
				{
					State.DynamicMaterials.Add(DynamicMaterial);
				}
			}
		}
	}
}

void UTunaSweeperVerticalOcclusionRevealComponent::RestoreOriginalMaterials()
{
	for (const FVerticalOcclusionRevealMaterialState& State : MaterialStates)
	{
		if (UMeshComponent* MeshComponent = State.MeshComponent.Get())
		{
			for (int32 MaterialIndex = 0; MaterialIndex < State.OriginalMaterials.Num(); ++MaterialIndex)
			{
				MeshComponent->SetMaterial(MaterialIndex, State.OriginalMaterials[MaterialIndex]);
			}
		}
	}
	MaterialStates.Reset();
}

bool UTunaSweeperVerticalOcclusionRevealComponent::ResolveCursorWorldPoint(
	APlayerController* PlayerController,
	float PlaneZ,
	FVector& OutCursorWorldPoint) const
{
	if (const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(PlayerController))
	{
		return TunaPlayerController->TryGetCursorWorldPointOnPlane(PlaneZ, OutCursorWorldPoint);
	}

	FVector WorldLocation;
	FVector WorldDirection;
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController || !PlayerController->GetMousePosition(MouseX, MouseY) ||
		!PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection) || FMath::IsNearlyZero(WorldDirection.Z))
	{
		return false;
	}

	const float DistanceToPlane = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
	if (DistanceToPlane < 0.0f)
	{
		return false;
	}

	OutCursorWorldPoint = WorldLocation + WorldDirection * DistanceToPlane;
	return true;
}

bool UTunaSweeperVerticalOcclusionRevealComponent::IsPointNearOwnerBoundsXY(const FVector& Point, float RadiusCm) const
{
	if (!GetOwner())
	{
		return false;
	}

	FVector BoundsOrigin;
	FVector BoundsExtent;
	GetOwner()->GetActorBounds(false, BoundsOrigin, BoundsExtent);
	const float OutsideX = FMath::Max(0.0f, FMath::Abs(Point.X - BoundsOrigin.X) - BoundsExtent.X);
	const float OutsideY = FMath::Max(0.0f, FMath::Abs(Point.Y - BoundsOrigin.Y) - BoundsExtent.Y);
	return FMath::Square(OutsideX) + FMath::Square(OutsideY) <= FMath::Square(RadiusCm);
}

void UTunaSweeperVerticalOcclusionRevealComponent::UpdateRevealParameters()
{
	UWorld* World = GetWorld();
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	const UTunaSweeperGameInstance* GameInstance = World ? World->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
	const UTunaSweeperOcclusionRevealSettingsDataAsset* Settings = GameInstance ? GameInstance->GetOcclusionRevealSettingsDataAsset() : nullptr;
	if (!Pawn || !Settings)
	{
		return;
	}

	const FVector PlayerLocation = Pawn->GetActorLocation();
	FVector CursorLocation = PlayerLocation;
	const bool bHasCursor = bUseCursorProximity && ResolveCursorWorldPoint(PlayerController, PlayerLocation.Z, CursorLocation);
	const float RadiusCm = ProximityRadiusOverrideCm > 0.0f ? ProximityRadiusOverrideCm : Settings->VerticalRevealProximityRadiusCm;
	const bool bShouldReveal =
		(bUsePlayerProximity && IsPointNearOwnerBoundsXY(PlayerLocation, RadiusCm)) ||
		(bHasCursor && IsPointNearOwnerBoundsXY(CursorLocation, RadiusCm));
	const float RevealStartZ = PlayerLocation.Z + Settings->VerticalRevealStartAbovePlayerCm;

	for (const FVerticalOcclusionRevealMaterialState& State : MaterialStates)
	{
		for (UMaterialInstanceDynamic* DynamicMaterial : State.DynamicMaterials)
		{
			if (DynamicMaterial)
			{
				DynamicMaterial->SetScalarParameterValue(TunaSweeperVerticalOcclusionReveal::RevealActiveName, bShouldReveal ? 1.0f : 0.0f);
				DynamicMaterial->SetScalarParameterValue(TunaSweeperVerticalOcclusionReveal::RevealStartZName, RevealStartZ);
				DynamicMaterial->SetScalarParameterValue(TunaSweeperVerticalOcclusionReveal::RevealFadeHeightName, Settings->VerticalRevealFadeHeightCm);
			}
		}
	}
}
