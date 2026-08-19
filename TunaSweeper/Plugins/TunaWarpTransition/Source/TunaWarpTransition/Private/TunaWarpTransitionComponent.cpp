#include "TunaWarpTransitionComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaWarpTransition, Log, All);

namespace TunaWarpTransition
{
	const FName WarpAmountParameter(TEXT("WarpAmount"));
	const FName CenterParameter(TEXT("CenterUV"));
	const FName RadiusParameter(TEXT("Radius"));
	const FName MinimumRadialScaleParameter(TEXT("MinimumRadialScale"));
	const FName StreakLengthParameter(TEXT("StreakLength"));
	const FName CoverAmountParameter(TEXT("CoverAmount"));
	const FName CoverColorParameter(TEXT("CoverColor"));
	const FName PlayerWorldPositionParameter(TEXT("PlayerWorldPosition"));
	const FName RimAmountParameter(TEXT("RimAmount"));
	const FName RimColorParameter(TEXT("RimColor"));
	const FName RimIntensityParameter(TEXT("RimIntensity"));
	const FName RimPowerParameter(TEXT("RimPower"));
	const FName EdgeStrengthParameter(TEXT("EdgeStrength"));
	const FName NormalEdgeScaleParameter(TEXT("NormalEdgeScale"));
	const FName DepthEdgeScaleParameter(TEXT("DepthEdgeScale"));
	const FName GlobalRimFractionParameter(TEXT("GlobalRimFraction"));
	const FName WaveRadiusParameter(TEXT("WaveRadiusCm"));
	const FName WaveHalfWidthParameter(TEXT("WaveHalfWidthCm"));
	const FName WaveSoftnessParameter(TEXT("WaveSoftnessCm"));

	float SmoothAlpha(const float Alpha)
	{
		const float Clamped = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return Clamped * Clamped * (3.0f - 2.0f * Clamped);
	}
}

UTunaWarpTransitionComponent::UTunaWarpTransitionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;

	WarpMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/TunaWarpTransition/Materials/MI_PP_TunaWarpRadial_Default.MI_PP_TunaWarpRadial_Default")));
	ArrivalRimMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/TunaWarpTransition/Materials/MI_PP_TunaWarpArrivalRim_Default.MI_PP_TunaWarpArrivalRim_Default")));
}

void UTunaWarpTransitionComponent::BeginPlay()
{
	Super::BeginPlay();
	InitializeEffectMaterials();
}

void UTunaWarpTransitionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelWarpTransition();
	DetachBlendables();
	if (ArrivalPointLight)
	{
		ArrivalPointLight->DestroyComponent();
		ArrivalPointLight = nullptr;
	}
	Super::EndPlay(EndPlayReason);
}

void UTunaWarpTransitionComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bTransitionActive)
	{
		return;
	}

	PhaseElapsed += FMath::Max(DeltaTime, 0.0f);
	if (Phase == ETransitionPhase::Closing)
	{
		const float Duration = FMath::Max(Style.CloseDuration, 0.01f);
		const float Alpha = TunaWarpTransition::SmoothAlpha(PhaseElapsed / Duration);
		UpdateClosing(Alpha);
		if (PhaseElapsed >= Duration)
		{
			ExecuteMidpointTeleport();
			Phase = ETransitionPhase::Opening;
			PhaseElapsed = 0.0f;
			UpdateOpening(0.0f);
		}
		return;
	}

	if (Phase == ETransitionPhase::Opening)
	{
		const float Duration = FMath::Max(Style.OpenDuration, 0.01f);
		const float Alpha = TunaWarpTransition::SmoothAlpha(PhaseElapsed / Duration);
		UpdateOpening(Alpha);
		if (PhaseElapsed >= Duration)
		{
			FinishTransition();
		}
	}
}

bool UTunaWarpTransitionComponent::PlayWarpTransition(
	AActor* ActorToTeleport,
	const FTransform& TargetTransform,
	const bool bUpdatePawnControlRotation)
{
	return PlayWarpTransitionNative(ActorToTeleport, TargetTransform, bUpdatePawnControlRotation, nullptr);
}

bool UTunaWarpTransitionComponent::PlayWarpTransitionNative(
	AActor* ActorToTeleport,
	const FTransform& TargetTransform,
	const bool bUpdatePawnControlRotation,
	TFunction<void(bool)> MidpointCallback)
{
	if (!ActorToTeleport || bTransitionActive)
	{
		return false;
	}

	PendingTeleportActor = ActorToTeleport;
	PendingTargetTransform = TargetTransform;
	PendingMidpointCallback = MoveTemp(MidpointCallback);
	bPendingControlRotationUpdate = bUpdatePawnControlRotation;
	bTeleportSucceeded = false;

	if (!InitializeEffectMaterials())
	{
		UE_LOG(LogTunaWarpTransition, Warning, TEXT("Warp materials or camera are unavailable; using an immediate teleport fallback."));
		OnTransitionStarted.Broadcast();
		bTeleportSucceeded = ExecuteImmediateTeleport(ActorToTeleport, TargetTransform, bUpdatePawnControlRotation);
		if (PendingMidpointCallback)
		{
			PendingMidpointCallback(bTeleportSucceeded);
			PendingMidpointCallback = nullptr;
		}
		OnTransitionMidpoint.Broadcast(bTeleportSucceeded);
		OnTransitionFinished.Broadcast(bTeleportSucceeded);
		PendingTeleportActor.Reset();
		return bTeleportSucceeded;
	}

	ResetMaterialParameters();
	AttachBlendables();
	ApplyInputLock();
	EnsureArrivalPointLight();
	SetArrivalPointLight(0.0f);

	bTransitionActive = true;
	Phase = ETransitionPhase::Closing;
	PhaseElapsed = 0.0f;
	SetComponentTickEnabled(true);
	UpdateClosing(0.0f);
	OnTransitionStarted.Broadcast();
	return true;
}

void UTunaWarpTransitionComponent::CancelWarpTransition()
{
	if (!bTransitionActive)
	{
		ReleaseInputLock();
		SetArrivalPointLight(0.0f);
		return;
	}

	bTransitionActive = false;
	Phase = ETransitionPhase::Idle;
	PhaseElapsed = 0.0f;
	SetComponentTickEnabled(false);
	ResetMaterialParameters();
	DetachBlendables();
	SetArrivalPointLight(0.0f);
	ReleaseInputLock();
	PendingTeleportActor.Reset();
	PendingMidpointCallback = nullptr;
}

bool UTunaWarpTransitionComponent::InitializeEffectMaterials()
{
	TargetCamera = FindTargetCamera();
	if (!TargetCamera)
	{
		return false;
	}

	if (!WarpMaterialInstance)
	{
		if (UMaterialInterface* LoadedWarpMaterial = WarpMaterial.LoadSynchronous())
		{
			WarpMaterialInstance = UMaterialInstanceDynamic::Create(LoadedWarpMaterial, this, TEXT("MID_TunaWarpRadial"));
		}
	}
	if (!ArrivalRimMaterialInstance)
	{
		if (UMaterialInterface* LoadedRimMaterial = ArrivalRimMaterial.LoadSynchronous())
		{
			ArrivalRimMaterialInstance = UMaterialInstanceDynamic::Create(LoadedRimMaterial, this, TEXT("MID_TunaWarpArrivalRim"));
		}
	}
	return WarpMaterialInstance && ArrivalRimMaterialInstance;
}

UCameraComponent* UTunaWarpTransitionComponent::FindTargetCamera() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TInlineComponentArray<UCameraComponent*> CameraComponents(Owner);
	for (UCameraComponent* Camera : CameraComponents)
	{
		if (Camera && Camera->IsActive())
		{
			return Camera;
		}
	}
	return CameraComponents.IsEmpty() ? nullptr : CameraComponents[0];
}

void UTunaWarpTransitionComponent::AttachBlendables()
{
	if (!TargetCamera)
	{
		return;
	}
	if (ArrivalRimMaterialInstance)
	{
		TargetCamera->AddOrUpdateBlendable(ArrivalRimMaterialInstance, 1.0f);
	}
	if (WarpMaterialInstance)
	{
		TargetCamera->AddOrUpdateBlendable(WarpMaterialInstance, 1.0f);
	}
}

void UTunaWarpTransitionComponent::DetachBlendables()
{
	if (!TargetCamera)
	{
		return;
	}
	if (WarpMaterialInstance)
	{
		TargetCamera->RemoveBlendable(WarpMaterialInstance);
	}
	if (ArrivalRimMaterialInstance)
	{
		TargetCamera->RemoveBlendable(ArrivalRimMaterialInstance);
	}
}

void UTunaWarpTransitionComponent::UpdateClosing(const float Alpha)
{
	if (!WarpMaterialInstance || !ArrivalRimMaterialInstance)
	{
		return;
	}

	WarpMaterialInstance->SetScalarParameterValue(TunaWarpTransition::WarpAmountParameter, Alpha);
	WarpMaterialInstance->SetScalarParameterValue(
		TunaWarpTransition::RadiusParameter,
		FMath::Lerp(Style.OpenRadius, Style.CollapsedRadius, Alpha));
	WarpMaterialInstance->SetScalarParameterValue(TunaWarpTransition::StreakLengthParameter, Style.StreakLength * Alpha);
	WarpMaterialInstance->SetScalarParameterValue(
		TunaWarpTransition::CoverAmountParameter,
		FMath::SmoothStep(0.72f, 1.0f, Alpha));
	ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::RimAmountParameter, 0.0f);
}

void UTunaWarpTransitionComponent::UpdateOpening(const float Alpha)
{
	if (!WarpMaterialInstance || !ArrivalRimMaterialInstance)
	{
		return;
	}

	const float Remaining = 1.0f - Alpha;
	WarpMaterialInstance->SetScalarParameterValue(TunaWarpTransition::WarpAmountParameter, Remaining);
	WarpMaterialInstance->SetScalarParameterValue(
		TunaWarpTransition::RadiusParameter,
		FMath::Lerp(Style.CollapsedRadius, Style.OpenRadius, Alpha));
	WarpMaterialInstance->SetScalarParameterValue(TunaWarpTransition::StreakLengthParameter, Style.StreakLength * Remaining);
	WarpMaterialInstance->SetScalarParameterValue(
		TunaWarpTransition::CoverAmountParameter,
		1.0f - FMath::SmoothStep(0.0f, 0.22f, Alpha));

	const float RimAmount = FMath::Pow(Remaining, 0.72f);
	ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::RimAmountParameter, RimAmount);
	ArrivalRimMaterialInstance->SetScalarParameterValue(
		TunaWarpTransition::WaveRadiusParameter,
		FMath::Lerp(0.0f, Style.WaveEndRadiusCm, Alpha));
	SetArrivalPointLight(RimAmount);
}

void UTunaWarpTransitionComponent::ExecuteMidpointTeleport()
{
	AActor* ActorToTeleport = PendingTeleportActor.Get();
	bTeleportSucceeded = ExecuteImmediateTeleport(ActorToTeleport, PendingTargetTransform, bPendingControlRotationUpdate);

	if (ArrivalRimMaterialInstance && ActorToTeleport)
	{
		ArrivalRimMaterialInstance->SetVectorParameterValue(
			TunaWarpTransition::PlayerWorldPositionParameter,
			FLinearColor(ActorToTeleport->GetActorLocation()));
	}
	if (PendingMidpointCallback)
	{
		PendingMidpointCallback(bTeleportSucceeded);
		PendingMidpointCallback = nullptr;
	}
	OnTransitionMidpoint.Broadcast(bTeleportSucceeded);
}

bool UTunaWarpTransitionComponent::ExecuteImmediateTeleport(
	AActor* ActorToTeleport,
	const FTransform& TargetTransform,
	const bool bUpdatePawnControlRotation) const
{
	if (!ActorToTeleport)
	{
		return false;
	}

	const FVector TargetLocation = TargetTransform.GetLocation();
	const FRotator TargetRotation = TargetTransform.Rotator();
	bool bWarped = ActorToTeleport->TeleportTo(TargetLocation, TargetRotation, false, true);
	if (!bWarped)
	{
		bWarped = ActorToTeleport->SetActorLocationAndRotation(
			TargetLocation,
			TargetRotation,
			false,
			nullptr,
			ETeleportType::TeleportPhysics);
	}

	if (bWarped && bUpdatePawnControlRotation)
	{
		if (const APawn* Pawn = Cast<APawn>(ActorToTeleport))
		{
			if (AController* Controller = Pawn->GetController())
			{
				Controller->SetControlRotation(TargetRotation);
			}
		}
	}
	return bWarped;
}

void UTunaWarpTransitionComponent::FinishTransition()
{
	const bool bFinalTeleportResult = bTeleportSucceeded;
	bTransitionActive = false;
	Phase = ETransitionPhase::Idle;
	PhaseElapsed = 0.0f;
	SetComponentTickEnabled(false);
	ResetMaterialParameters();
	DetachBlendables();
	SetArrivalPointLight(0.0f);
	ReleaseInputLock();
	PendingTeleportActor.Reset();
	PendingMidpointCallback = nullptr;
	OnTransitionFinished.Broadcast(bFinalTeleportResult);
}

void UTunaWarpTransitionComponent::ResetMaterialParameters()
{
	if (WarpMaterialInstance)
	{
		WarpMaterialInstance->SetScalarParameterValue(TunaWarpTransition::WarpAmountParameter, 0.0f);
		WarpMaterialInstance->SetVectorParameterValue(
			TunaWarpTransition::CenterParameter,
			FLinearColor(Style.ScreenCenter.X, Style.ScreenCenter.Y, 0.0f, 0.0f));
		WarpMaterialInstance->SetScalarParameterValue(TunaWarpTransition::RadiusParameter, Style.OpenRadius);
		WarpMaterialInstance->SetScalarParameterValue(
			TunaWarpTransition::MinimumRadialScaleParameter,
			Style.MinimumRadialScale);
		WarpMaterialInstance->SetScalarParameterValue(TunaWarpTransition::StreakLengthParameter, 0.0f);
		WarpMaterialInstance->SetScalarParameterValue(TunaWarpTransition::CoverAmountParameter, 0.0f);
		WarpMaterialInstance->SetVectorParameterValue(TunaWarpTransition::CoverColorParameter, Style.CoverColor);
	}

	if (ArrivalRimMaterialInstance)
	{
		ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::RimAmountParameter, 0.0f);
		ArrivalRimMaterialInstance->SetVectorParameterValue(TunaWarpTransition::RimColorParameter, Style.RimColor);
		ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::RimIntensityParameter, Style.RimIntensity);
		ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::RimPowerParameter, Style.RimPower);
		ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::EdgeStrengthParameter, Style.EdgeStrength);
		ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::NormalEdgeScaleParameter, Style.NormalEdgeScale);
		ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::DepthEdgeScaleParameter, Style.DepthEdgeScale);
		ArrivalRimMaterialInstance->SetScalarParameterValue(
			TunaWarpTransition::GlobalRimFractionParameter,
			Style.GlobalRimFraction);
		ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::WaveRadiusParameter, 0.0f);
		ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::WaveHalfWidthParameter, Style.WaveHalfWidthCm);
		ArrivalRimMaterialInstance->SetScalarParameterValue(TunaWarpTransition::WaveSoftnessParameter, Style.WaveSoftnessCm);
	}
}

void UTunaWarpTransitionComponent::ApplyInputLock()
{
	const APawn* Pawn = Cast<APawn>(PendingTeleportActor.Get());
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (!Controller)
	{
		return;
	}
	if (bLockMovementInput && !Controller->IsMoveInputIgnored())
	{
		Controller->SetIgnoreMoveInput(true);
		bAppliedMoveInputLock = true;
	}
	if (bLockLookInput && !Controller->IsLookInputIgnored())
	{
		Controller->SetIgnoreLookInput(true);
		bAppliedLookInputLock = true;
	}
}

void UTunaWarpTransitionComponent::ReleaseInputLock()
{
	const APawn* Pawn = Cast<APawn>(PendingTeleportActor.Get());
	AController* Controller = Pawn ? Pawn->GetController() : nullptr;
	if (Controller && bAppliedMoveInputLock)
	{
		Controller->SetIgnoreMoveInput(false);
	}
	if (Controller && bAppliedLookInputLock)
	{
		Controller->SetIgnoreLookInput(false);
	}
	bAppliedMoveInputLock = false;
	bAppliedLookInputLock = false;
}

void UTunaWarpTransitionComponent::EnsureArrivalPointLight()
{
	if (!Style.bEnableArrivalPointLight || ArrivalPointLight || !GetOwner())
	{
		return;
	}

	ArrivalPointLight = NewObject<UPointLightComponent>(GetOwner(), TEXT("TunaWarpArrivalPointLight"));
	if (!ArrivalPointLight)
	{
		return;
	}
	GetOwner()->AddInstanceComponent(ArrivalPointLight);
	if (USceneComponent* OwnerRoot = GetOwner()->GetRootComponent())
	{
		ArrivalPointLight->SetupAttachment(OwnerRoot);
	}
	ArrivalPointLight->SetRelativeLocation(Style.PointLightRelativeLocation);
	ArrivalPointLight->SetCastShadows(false);
	ArrivalPointLight->SetIntensityUnits(ELightUnits::Lumens);
	ArrivalPointLight->SetAttenuationRadius(Style.PointLightRadiusCm);
	ArrivalPointLight->SetLightColor(Style.PointLightColor);
	ArrivalPointLight->SetIntensity(0.0f);
	ArrivalPointLight->RegisterComponent();
}

void UTunaWarpTransitionComponent::SetArrivalPointLight(const float NormalizedIntensity)
{
	if (!ArrivalPointLight)
	{
		return;
	}
	ArrivalPointLight->SetRelativeLocation(Style.PointLightRelativeLocation);
	ArrivalPointLight->SetAttenuationRadius(Style.PointLightRadiusCm);
	ArrivalPointLight->SetLightColor(Style.PointLightColor);
	ArrivalPointLight->SetIntensity(
		Style.bEnableArrivalPointLight
			? Style.PointLightIntensityLumens * FMath::Clamp(NormalizedIntensity, 0.0f, 1.0f)
			: 0.0f);
}
