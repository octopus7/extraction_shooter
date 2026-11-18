#include "Player/TunaSweeperPlayerController.h"

#include "AI/Hopper/HopperArmModule.h"
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "InputKeyEventArgs.h"
#include "UI/TunaSweeperGameHudWidget.h"

namespace
{
	constexpr float StickNeutral = .12f;
	constexpr float EjectionBlendIn = .48f;
	constexpr float ReturnBlend = .45f;
}

bool ATunaSweeperPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
	const FKey& Key = Params.Key;
	const bool bAnalog = Key.IsAnalog();
	// Mouse deltas and wheel pulses have no held state. Gamepad axes return to neutral.
	if (!bAnalog || Key.IsGamepadKey())
	{
		const bool bReleased = bAnalog ? FMath::Abs(Params.AmountDepressed) <= StickNeutral : Params.Event == IE_Released;
		const bool bPressed = bAnalog || Params.Event == IE_Pressed || Params.Event == IE_DoubleClick || Params.Event == IE_Repeat;
		const bool bAwaitingRelease = HopperKeysAwaitingRelease.Contains(Key);
		if (bReleased)
		{
			PhysicalKeysDown.Remove(Key);
			HopperKeysAwaitingRelease.Remove(Key);
		}
		else if (bPressed)
		{
			// A fresh OS press after a focus loss may be the first event after an unobserved release.
			if (!IsHopperShotActive() && !bAnalog && Params.Event == IE_Pressed && !PhysicalKeysDown.Contains(Key))
				HopperKeysAwaitingRelease.Remove(Key);
			PhysicalKeysDown.Add(Key);
			if (IsHopperShotActive()) HopperKeysAwaitingRelease.Add(Key);
		}
		if (bAwaitingRelease && (bReleased || HopperKeysAwaitingRelease.Contains(Key))) return true;
	}
	return IsHopperShotActive() || Super::InputKey(Params);
}

void ATunaSweeperPlayerController::FlushPressedKeys()
{
	PhysicalKeysDown.Reset();
	Super::FlushPressedKeys();
}

void ATunaSweeperPlayerController::BuildInputStack(TArray<UInputComponent*>& InputStack)
{
	// Also discard action events that were already queued before the camera cut this frame.
	if (!IsHopperShotActive()) Super::BuildInputStack(InputStack);
}

bool ATunaSweeperPlayerController::IsViewingHopper(const AActor* Actor) const
{
	return IsHopperShotActive() && Actor && HopperSubject.Get() == Actor;
}

bool ATunaSweeperPlayerController::StartHopperShot(ATunaSweeperHopperEnemyCharacter* Hopper, bool bEjection, float Duration)
{
	auto* PlayerPawn = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (!IsLocalPlayerController() || !PlayerCameraManager || !PlayerPawn || PlayerPawn->IsDead() || !IsValid(Hopper)
		|| !Hopper->GetPresentation()->GetBodyRoot() || !Hopper->GetPresentation()->GetPilotMesh()
		|| IsDialogueSequenceActive() || IsInventoryUiOpen() || IsPauseMenuOpen() || IsHousingModeOpen()) return false;
	if (IsHopperShotActive() && (!IsViewingHopper(Hopper) || !bEjection || bHopperEjection)) return false;

	const bool bFirstShot = !IsHopperShotActive();
	InterruptedBoardingView.Reset();
	if (!bFirstShot && !bHopperReturning)
	{
		InterruptedBoardingView = HopperCamera->GetActorTransform();
		InterruptedBoardingFOV = HopperCamera->GetCameraComponent()->FieldOfView;
	}
	if (bFirstShot)
	{
		FActorSpawnParameters Spawn;
		Spawn.Owner = this;
		Spawn.ObjectFlags = RF_Transient;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		HopperCamera = GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), FTransform::Identity, Spawn);
		if (!HopperCamera) return false;
		HopperCamera->GetCameraComponent()->bConstrainAspectRatio = false;
		HopperCamera->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount = true;
		HopperCamera->GetCameraComponent()->PostProcessSettings.MotionBlurAmount = .1f;
		HopperReturnView = GetViewTarget();
		HopperSubject = Hopper;
		HopperCameraSide = FVector::DotProduct(PlayerCameraManager->GetCameraLocation() - Hopper->GetActorLocation(),
			Hopper->GetActorRightVector()) >= 0.f ? 1.f : -1.f;
		HopperKeysAwaitingRelease.Append(PhysicalKeysDown);
		if (GameHudWidget)
		{
			HopperPreviousHudVisibility = GameHudWidget->GetVisibility();
			GameHudWidget->SetVisibility(ESlateVisibility::Hidden);
		}
		bHopperPreviousCursor = bShowMouseCursor;
		bShowMouseCursor = false;
		SetIgnoreMoveInput(true);
		SetIgnoreLookInput(true);
		StopMovement();
		CancelPawnGameplayActions();
		PlayerPawn->StopJumping();
		PlayerPawn->ConsumeMovementInputVector();
		Super::FlushPressedKeys();
	}
	bHopperEjection = bEjection;
	bHopperReturning = false;
	HopperShotTime = 0.f;
	HopperShotDuration = Duration;
	FrameHopperShot();
	SetViewTargetWithBlend(HopperCamera, bEjection ? EjectionBlendIn : 0.f, VTBlend_Cubic, 0.f, true);
	return true;
}

void ATunaSweeperPlayerController::FrameHopperShot()
{
	const auto* Hopper = HopperSubject.Get();
	const auto* Stage = Hopper->GetPresentation();
	const FVector Ground = Stage->GetBodyRoot()->GetComponentLocation();
	const FVector Forward = Stage->GetBodyRoot()->GetForwardVector().GetSafeNormal2D();
	const FVector Side = Stage->GetBodyRoot()->GetRightVector().GetSafeNormal2D() * HopperCameraSide;
	const FVector Pilot = Stage->GetPilotMesh()->GetComponentLocation();
	FVector Aim;
	FVector Offset;
	float FieldOfView;
	if (bHopperEjection)
	{
		const float FlightTime = HopperShotTime - Stage->GetEjectionHatchDuration() - Stage->EjectionSlideSeconds;
		const float Follow = FMath::SmoothStep(0.f, .22f, FlightTime);
		Aim = FMath::Lerp(Ground + Forward * 23.f + FVector::UpVector * 158.f, Pilot + FVector::UpVector * 32.f, Follow);
		Offset = Forward * 302.f + Side * 239.f + FVector::UpVector * FMath::Lerp(159.f, 125.f, Follow);
		FieldOfView = FMath::Lerp(52.f, 56.f, Follow);
	}
	else
	{
		// Hold the wider two-shot while the pilot is on the ground and the upper guard rises.
		const float CloseIn = FMath::SmoothStep(Hopper->BoardingSeconds * .65f, Hopper->BoardingSeconds + .4f, HopperShotTime);
		Aim = Ground + Forward * FMath::Lerp(75.f, 17.f, CloseIn) + FVector::UpVector * FMath::Lerp(120.f, 156.f, CloseIn);
		Offset = Forward * FMath::Lerp(388.f, 290.f, CloseIn) + Side * FMath::Lerp(390.f, 245.f, CloseIn)
			+ FVector::UpVector * FMath::Lerp(245.f, 165.f, CloseIn);
		FieldOfView = FMath::Lerp(54.f, 50.f, CloseIn);
	}
	FCollisionQueryParams Params(SCENE_QUERY_STAT(HopperCameraClearance), false, Hopper);
	Params.AddIgnoredActor(GetPawn());
	Params.AddIgnoredActor(Hopper->GetArmModule(true));
	Params.AddIgnoredActor(Hopper->GetArmModule(false));
	FHitResult Hit;
	const bool bObstructed = GetWorld()->SweepSingleByChannel(Hit, Aim, Aim + Offset, FQuat::Identity, ECC_Camera,
		FCollisionShape::MakeSphere(10.f), Params);
	const FVector Eye = bObstructed ? Hit.Location : Aim + Offset;
	FTransform Pose((Aim - Eye).Rotation(), Eye);
	if (InterruptedBoardingView.IsSet())
	{
		const float Alpha = FMath::SmoothStep(0.f, EjectionBlendIn, HopperShotTime);
		FTransform Blended;
		Blended.Blend(InterruptedBoardingView.GetValue(), Pose, Alpha);
		Pose = Blended;
		FieldOfView = FMath::Lerp(InterruptedBoardingFOV, FieldOfView, Alpha);
		if (Alpha >= 1.f) InterruptedBoardingView.Reset();
	}
	HopperCamera->SetActorTransform(Pose);
	HopperCamera->GetCameraComponent()->SetFieldOfView(FieldOfView);
}

void ATunaSweeperPlayerController::UpdateHopperShot(float DeltaSeconds)
{
	AActor* ReturnTarget = HopperReturnView.IsValid() ? HopperReturnView.Get() : GetPawn();
	const auto* PlayerPawn = Cast<ATunaSweeperTopDownCharacter>(GetPawn());
	if (!PlayerCameraManager || !PlayerPawn || PlayerPawn->IsDead() || IsDialogueSequenceActive())
	{
		EndHopperShot(true);
		return;
	}
	if (bHopperReturning)
	{
		if (!PlayerCameraManager->PendingViewTarget.Target || GetViewTarget() != ReturnTarget) EndHopperShot(false);
		return;
	}
	// A different system has taken control of the player's view.
	if (GetViewTarget() != HopperCamera)
	{
		EndHopperShot(false);
		return;
	}
	HopperShotTime += DeltaSeconds;
	if (HopperSubject.IsValid() && HopperShotTime < HopperShotDuration)
	{
		FrameHopperShot();
		return;
	}
	bHopperReturning = true;
	SetViewTargetWithBlend(ReturnTarget, ReturnBlend, VTBlend_Cubic, 0.f, true);
}

void ATunaSweeperPlayerController::EndHopperShot(bool bRestoreView)
{
	if (!IsHopperShotActive()) return;
	if (bRestoreView && PlayerCameraManager && (GetViewTarget() == HopperCamera ||
		(bHopperReturning && PlayerCameraManager->ViewTarget.Target == HopperCamera && GetViewTarget() == HopperReturnView.Get())))
		SetViewTarget(HopperReturnView.IsValid() ? HopperReturnView.Get() : GetPawn());
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);
	if (GameHudWidget && GameHudWidget->GetVisibility() == ESlateVisibility::Hidden)
		GameHudWidget->SetVisibility(HopperPreviousHudVisibility);
	if (!bShowMouseCursor) bShowMouseCursor = bHopperPreviousCursor;
	HopperCamera->Destroy();
	HopperCamera = nullptr;
	HopperSubject.Reset();
	HopperReturnView.Reset();
	bHopperReturning = false;
}
