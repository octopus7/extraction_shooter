#include "Component/TunaSweeperOcclusionRevealSourceComponent.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperOcclusionRevealTypes.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Player/TunaSweeperPlayerController.h"

UTunaSweeperOcclusionRevealSourceComponent::UTunaSweeperOcclusionRevealSourceComponent()
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;

	RevealParameterCollection = TSoftObjectPtr<UMaterialParameterCollection>(
		FSoftObjectPath(TunaSweeperOcclusionReveal::ParameterCollectionObjectPath()));
}

void UTunaSweeperOcclusionRevealSourceComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveRevealParameterCollection();
	ForceRefreshRevealParameters();
}

void UTunaSweeperOcclusionRevealSourceComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	ForceRefreshRevealParameters();
}

void UTunaSweeperOcclusionRevealSourceComponent::ForceRefreshRevealParameters()
{
	UWorld* World = GetWorld();
	if (bUpdateOnlyInGameWorld && (!World || !World->IsGameWorld()))
	{
		return;
	}

	APlayerController* PlayerController = ResolveLocalPlayerController();
	if (bUpdateOnlyForLocalPlayer && !PlayerController)
	{
		return;
	}

	PushRevealParameters(PlayerController);
}

APlayerController* UTunaSweeperOcclusionRevealSourceComponent::ResolveLocalPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}

	if (APlayerController* OwnerPlayerController = Cast<APlayerController>(OwnerPawn->GetController()))
	{
		return OwnerPlayerController->IsLocalController() ? OwnerPlayerController : nullptr;
	}

	const UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator ControllerIt = World->GetPlayerControllerIterator(); ControllerIt; ++ControllerIt)
	{
		APlayerController* CandidatePlayerController = ControllerIt->Get();
		if (CandidatePlayerController &&
			CandidatePlayerController->IsLocalController() &&
			CandidatePlayerController->GetPawn() == OwnerPawn)
		{
			return CandidatePlayerController;
		}
	}

	return nullptr;
}

UMaterialParameterCollection* UTunaSweeperOcclusionRevealSourceComponent::ResolveRevealParameterCollection()
{
	if (CachedRevealParameterCollection)
	{
		return CachedRevealParameterCollection;
	}

	CachedRevealParameterCollection = RevealParameterCollection.LoadSynchronous();
	if (!CachedRevealParameterCollection && !bLoggedMissingCollection)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Occlusion reveal parameter collection is missing: %s"),
			TunaSweeperOcclusionReveal::ParameterCollectionObjectPath());
		bLoggedMissingCollection = true;
	}

	return CachedRevealParameterCollection;
}

float UTunaSweeperOcclusionRevealSourceComponent::ResolveCursorPlaneZ() const
{
	if (bUseCharacterWeaponAimPlaneForCursor)
	{
		if (const ATunaSweeperTopDownCharacter* Character = Cast<ATunaSweeperTopDownCharacter>(GetOwner()))
		{
			return Character->GetWeaponAimPlaneZ();
		}
	}

	const AActor* OwnerActor = GetOwner();
	return OwnerActor ? OwnerActor->GetActorLocation().Z : 0.0f;
}

bool UTunaSweeperOcclusionRevealSourceComponent::ResolveCursorWorldPoint(
	APlayerController* PlayerController,
	FVector& OutCursorWorldPoint) const
{
	if (!PlayerController)
	{
		return false;
	}

	const float PlaneZ = ResolveCursorPlaneZ();
	if (const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(PlayerController))
	{
		return TunaPlayerController->TryGetCursorWorldPointOnPlane(PlaneZ, OutCursorWorldPoint);
	}

	FVector WorldLocation = FVector::ZeroVector;
	FVector WorldDirection = FVector::ZeroVector;
	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY) ||
		!PlayerController->DeprojectScreenPositionToWorld(MouseX, MouseY, WorldLocation, WorldDirection) ||
		FMath::IsNearlyZero(WorldDirection.Z))
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

void UTunaSweeperOcclusionRevealSourceComponent::PushRevealParameters(APlayerController* PlayerController)
{
	UMaterialParameterCollection* Collection = ResolveRevealParameterCollection();
	const AActor* OwnerActor = GetOwner();
	if (!Collection || !OwnerActor)
	{
		return;
	}

	const FVector CharacterLocation = OwnerActor->GetActorLocation();
	FVector CursorLocation = CharacterLocation;
	const bool bHasCursorLocation = ResolveCursorWorldPoint(PlayerController, CursorLocation);
	if (!bHasCursorLocation)
	{
		CursorLocation = CharacterLocation;
	}

	UKismetMaterialLibrary::SetVectorParameterValue(
		this,
		Collection,
		TunaSweeperOcclusionReveal::CharacterCenterParameterName(),
		FLinearColor(CharacterLocation.X, CharacterLocation.Y, CharacterLocation.Z, 1.0f));
	UKismetMaterialLibrary::SetVectorParameterValue(
		this,
		Collection,
		TunaSweeperOcclusionReveal::CursorCenterParameterName(),
		FLinearColor(CursorLocation.X, CursorLocation.Y, CursorLocation.Z, bHasCursorLocation ? 1.0f : 0.0f));
	UKismetMaterialLibrary::SetScalarParameterValue(
		this,
		Collection,
		TunaSweeperOcclusionReveal::CharacterRadiusParameterName(),
		FMath::Max(0.0f, CharacterRevealRadiusCm));
	UKismetMaterialLibrary::SetScalarParameterValue(
		this,
		Collection,
		TunaSweeperOcclusionReveal::CursorRadiusParameterName(),
		FMath::Max(0.0f, CursorRevealRadiusCm));
	UKismetMaterialLibrary::SetScalarParameterValue(
		this,
		Collection,
		TunaSweeperOcclusionReveal::RevealFeatherParameterName(),
		FMath::Max(1.0f, RevealFeatherCm));
	UKismetMaterialLibrary::SetScalarParameterValue(
		this,
		Collection,
		TunaSweeperOcclusionReveal::RevealStrengthParameterName(),
		FMath::Clamp(RevealStrength, 0.0f, 1.0f));
	UKismetMaterialLibrary::SetScalarParameterValue(
		this,
		Collection,
		TunaSweeperOcclusionReveal::CursorValidParameterName(),
		bHasCursorLocation ? 1.0f : 0.0f);
}
