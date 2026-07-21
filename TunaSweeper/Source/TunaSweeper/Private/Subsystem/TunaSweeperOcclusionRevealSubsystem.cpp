#include "Subsystem/TunaSweeperOcclusionRevealSubsystem.h"

#include "Effect/TunaSweeperOcclusionRevealSettingsDataAsset.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Materials/MaterialParameterCollection.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Math/RotationMatrix.h"

namespace TunaSweeperOcclusionReveal
{
	const TCHAR* CollectionPath = TEXT("/Game/Effects/MPC_OcclusionReveal.MPC_OcclusionReveal");
	const FName CharacterCenterScreenName(TEXT("CharacterCenterScreen"));
	const FName CursorCenterScreenName(TEXT("CursorCenterScreen"));
	const FName CharacterValidName(TEXT("CharacterValid"));
	const FName CursorValidName(TEXT("CursorValid"));
	const FName CharacterInnerRadiusName(TEXT("CharacterInnerRadiusScreen"));
	const FName CharacterOuterRadiusName(TEXT("CharacterOuterRadiusScreen"));
	const FName CursorInnerRadiusName(TEXT("CursorInnerRadiusScreen"));
	const FName CursorOuterRadiusName(TEXT("CursorOuterRadiusScreen"));
	const FName ViewportHeightOverWidthName(TEXT("ViewportHeightOverWidth"));
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

bool UTunaSweeperOcclusionRevealSubsystem::ProjectWorldRevealCircle(
	APlayerController* PlayerController,
	const FVector& WorldCenter,
	float InnerRadiusCm,
	float OuterRadiusCm,
	FScreenRevealCircle& OutCircle) const
{
	if (!PlayerController)
	{
		return false;
	}

	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);
	if (ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return false;
	}

	FVector2D CenterPixel;
	if (!PlayerController->ProjectWorldLocationToScreen(WorldCenter, CenterPixel, true))
	{
		return false;
	}

	const FRotator CameraRotation = PlayerController->PlayerCameraManager
		? PlayerController->PlayerCameraManager->GetCameraRotation()
		: PlayerController->GetControlRotation();
	const FVector CameraRight = FRotationMatrix(CameraRotation).GetUnitAxis(EAxis::Y);

	auto ProjectRadius = [PlayerController, WorldCenter, CenterPixel, CameraRight](float RadiusCm, float& OutRadiusPixels)
	{
		FVector2D RadiusPixel;
		if (!PlayerController->ProjectWorldLocationToScreen(WorldCenter + CameraRight * RadiusCm, RadiusPixel, true))
		{
			return false;
		}

		OutRadiusPixels = FVector2D::Distance(CenterPixel, RadiusPixel);
		return OutRadiusPixels > KINDA_SMALL_NUMBER;
	};

	float InnerRadiusPixels = 0.0f;
	float OuterRadiusPixels = 0.0f;
	if (!ProjectRadius(InnerRadiusCm, InnerRadiusPixels) || !ProjectRadius(OuterRadiusCm, OuterRadiusPixels))
	{
		return false;
	}

	OutCircle.CenterUv = FVector2D(CenterPixel.X / ViewportWidth, CenterPixel.Y / ViewportHeight);
	OutCircle.InnerRadiusScreenWidth = InnerRadiusPixels / ViewportWidth;
	OutCircle.OuterRadiusScreenWidth = OuterRadiusPixels / ViewportWidth;
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
	FScreenRevealCircle CharacterCircle;
	FScreenRevealCircle CursorCircle;
	const bool bHasCharacterCircle = ProjectWorldRevealCircle(
		PlayerController, CharacterLocation, Settings->GetInnerRadiusCm(), Settings->GetOuterRadiusCm(), CharacterCircle);
	const bool bHasCursorCircle = bHasCursor && ProjectWorldRevealCircle(
		PlayerController, CursorLocation, Settings->GetInnerRadiusCm(), Settings->GetOuterRadiusCm(), CursorCircle);
	int32 ViewportWidth = 0;
	int32 ViewportHeight = 0;
	PlayerController->GetViewportSize(ViewportWidth, ViewportHeight);

	UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, TunaSweeperOcclusionReveal::CharacterCenterScreenName,
		FLinearColor(CharacterCircle.CenterUv.X, CharacterCircle.CenterUv.Y, 0.0f, 1.0f));
	UKismetMaterialLibrary::SetVectorParameterValue(this, Collection, TunaSweeperOcclusionReveal::CursorCenterScreenName,
		FLinearColor(CursorCircle.CenterUv.X, CursorCircle.CenterUv.Y, 0.0f, 1.0f));
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::CharacterValidName, bHasCharacterCircle ? 1.0f : 0.0f);
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::CursorValidName, bHasCursorCircle ? 1.0f : 0.0f);
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::CharacterInnerRadiusName, CharacterCircle.InnerRadiusScreenWidth);
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::CharacterOuterRadiusName, CharacterCircle.OuterRadiusScreenWidth);
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::CursorInnerRadiusName, CursorCircle.InnerRadiusScreenWidth);
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::CursorOuterRadiusName, CursorCircle.OuterRadiusScreenWidth);
	UKismetMaterialLibrary::SetScalarParameterValue(this, Collection, TunaSweeperOcclusionReveal::ViewportHeightOverWidthName,
		ViewportWidth > 0 ? static_cast<float>(ViewportHeight) / ViewportWidth : 1.0f);
}
