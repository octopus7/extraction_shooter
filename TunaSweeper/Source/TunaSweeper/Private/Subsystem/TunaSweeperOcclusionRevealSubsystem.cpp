#include "Subsystem/TunaSweeperOcclusionRevealSubsystem.h"

#include "Effect/TunaSweeperOcclusionRevealSettingsDataAsset.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Player/TunaSweeperPlayerController.h"

namespace TunaSweeperOcclusionReveal
{
	const TCHAR* CollectionPath = TEXT("/Game/Effects/MPC_OcclusionReveal.MPC_OcclusionReveal");
	const FName CharacterCenterName(TEXT("CharacterCenter"));
	const FName CursorCenterName(TEXT("CursorCenter"));
	const FName CursorValidName(TEXT("CursorValid"));
	const FName InnerRadiusName(TEXT("InnerRadiusCm"));
	const FName OuterRadiusName(TEXT("OuterRadiusCm"));
}

void UTunaSweeperOcclusionRevealSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateRevealParameters();
}

TStatId UTunaSweeperOcclusionRevealSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTunaSweeperOcclusionRevealSubsystem, STATGROUP_Tickables);
}

bool UTunaSweeperOcclusionRevealSubsystem::IsTickable() const
{
	return GetWorld() && GetWorld()->IsGameWorld();
}

bool UTunaSweeperOcclusionRevealSubsystem::ResolveCursorWorldPoint(
	APlayerController* PlayerController,
	float PlaneZ,
	FVector& OutCursorWorldPoint) const
{
	if (!PlayerController)
	{
		return false;
	}

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

void UTunaSweeperOcclusionRevealSubsystem::UpdateRevealParameters()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UMaterialParameterCollection* Collection = LoadObject<UMaterialParameterCollection>(nullptr, TunaSweeperOcclusionReveal::CollectionPath);
	APlayerController* PlayerController = World->GetFirstPlayerController();
	APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
	if (!Collection || !Pawn)
	{
		return;
	}

	const UTunaSweeperGameInstance* GameInstance = World->GetGameInstance<UTunaSweeperGameInstance>();
	const UTunaSweeperOcclusionRevealSettingsDataAsset* Settings = GameInstance
		? GameInstance->GetOcclusionRevealSettingsDataAsset()
		: nullptr;
	if (!Settings)
	{
		return;
	}

	const FVector CharacterLocation = Pawn->GetActorLocation();
	FVector CursorLocation = CharacterLocation;
	const bool bHasCursor = ResolveCursorWorldPoint(PlayerController, CharacterLocation.Z, CursorLocation);

	UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, TunaSweeperOcclusionReveal::CharacterCenterName,
		FLinearColor(CharacterLocation.X, CharacterLocation.Y, CharacterLocation.Z, 1.0f));
	UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, TunaSweeperOcclusionReveal::CursorCenterName,
		FLinearColor(CursorLocation.X, CursorLocation.Y, CursorLocation.Z, bHasCursor ? 1.0f : 0.0f));
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::CursorValidName, bHasCursor ? 1.0f : 0.0f);
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::InnerRadiusName, Settings->GetInnerRadiusCm());
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::OuterRadiusName, Settings->GetOuterRadiusCm());
}
