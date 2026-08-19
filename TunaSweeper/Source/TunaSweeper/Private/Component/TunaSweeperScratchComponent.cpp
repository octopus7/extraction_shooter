#include "Component/TunaSweeperScratchComponent.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/MeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"

namespace TunaSweeperScratchPresentation
{
	const FName ColorParameter(TEXT("ScratchColor"));
	const FName AlphaParameter(TEXT("EffectAlpha"));
	const FName IntensityParameter(TEXT("ScratchIntensity"));
	constexpr float EmissiveIntensity = 2.2f;
	constexpr uint8 RainbowSaturation = 92;
	constexpr float RainbowCyclesPerSecond = 1.7f;
}

UTunaSweeperScratchComponent::UTunaSweeperScratchComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	ScratchOverlayMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Effects/M_PlayerScratchRainbowOverlay.M_PlayerScratchRainbowOverlay")));
}

void UTunaSweeperScratchComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentScratch = FMath::Clamp(CurrentScratch, 0.0f, GetMaxScratch());
	OnScratchChanged.Broadcast(CurrentScratch, GetMaxScratch());
	SetDeveloperAlwaysSlowPresentationEnabled(
		ATunaSweeperPlayerController::GetDeveloperAlwaysSlowPresentationPreference());
}

void UTunaSweeperScratchComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	RestorePresentation();
	Super::EndPlay(EndPlayReason);
}

void UTunaSweeperScratchComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	(void)DeltaTime;

	const double RealTimeSeconds = FPlatformTime::Seconds();
	if (bDeveloperAlwaysSlowPresentationEnabled && !bPresentationActive)
	{
		TriggerPresentation(RealTimeSeconds);
	}
	else if (bPresentationActive)
	{
		UpdatePresentation(RealTimeSeconds);
	}
}

bool UTunaSweeperScratchComponent::TryConsumeScratch(float Amount)
{
	const float SafeAmount = FMath::Max(0.0f, Amount);
	if (SafeAmount <= 0.0f || CurrentScratch + KINDA_SMALL_NUMBER < SafeAmount)
	{
		return false;
	}

	SetCurrentScratch(CurrentScratch - SafeAmount);
	return true;
}

void UTunaSweeperScratchComponent::ResetScratch()
{
	SetCurrentScratch(0.0f);
}

void UTunaSweeperScratchComponent::SetDeveloperAlwaysSlowPresentationEnabled(bool bEnabled)
{
	if (bDeveloperAlwaysSlowPresentationEnabled == bEnabled)
	{
		return;
	}

	bDeveloperAlwaysSlowPresentationEnabled = bEnabled;
	const double RealTimeSeconds = FPlatformTime::Seconds();
	if (bEnabled)
	{
		TriggerPresentation(RealTimeSeconds);
	}
	else if (bPresentationActive)
	{
		PresentationReleaseRealSeconds = RealTimeSeconds;
		UpdatePresentation(RealTimeSeconds);
	}
}

bool UTunaSweeperScratchComponent::TryRegisterNearMiss(
	AActor* AttackSource,
	int32 AttackId,
	ETunaSweeperNearMissAttackType AttackType,
	float ClearanceCm,
	bool bWouldHaveHit)
{
	ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(GetOwner());
	if (!PlayerCharacter || PlayerCharacter->IsDead() || !PlayerCharacter->IsRolling() || !AttackSource)
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		if (const UTunaSweeperFactionSubsystem* FactionSubsystem = World->GetSubsystem<UTunaSweeperFactionSubsystem>();
			FactionSubsystem && !FactionSubsystem->CanApplyCombatEffect(AttackSource, PlayerCharacter))
		{
			return false;
		}
	}

	if (const int32* PreviousAttackId = LastProcessedAttackIds.Find(AttackSource);
		PreviousAttackId && *PreviousAttackId == AttackId)
	{
		return false;
	}
	LastProcessedAttackIds.Add(AttackSource, AttackId);

	const float BaseGain = AttackType == ETunaSweeperNearMissAttackType::Melee
		? MeleeScratchGain
		: ProjectileScratchGain;
	const float Gain = BaseGain * (bWouldHaveHit ? FMath::Max(1.0f, PerfectDodgeGainMultiplier) : 1.0f);
	SetCurrentScratch(CurrentScratch + Gain);

	OnNearMiss.Broadcast(AttackSource, AttackType, bWouldHaveHit, ClearanceCm);
	const double RealTimeSeconds = FPlatformTime::Seconds();
	if (RealTimeSeconds - LastPresentationRealSeconds >= FMath::Max(0.0f, PresentationCooldownRealSeconds))
	{
		TriggerPresentation(RealTimeSeconds);
	}
	return true;
}

void UTunaSweeperScratchComponent::SetCurrentScratch(float NewValue)
{
	const float ClampedValue = FMath::Clamp(NewValue, 0.0f, GetMaxScratch());
	if (FMath::IsNearlyEqual(CurrentScratch, ClampedValue))
	{
		return;
	}

	CurrentScratch = ClampedValue;
	OnScratchChanged.Broadcast(CurrentScratch, GetMaxScratch());
}

void UTunaSweeperScratchComponent::TriggerPresentation(double RealTimeSeconds)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	if (!bPresentationActive)
	{
		SavedWorldTimeDilation = World->GetWorldSettings()
			? FMath::Max(0.0001f, World->GetWorldSettings()->TimeDilation)
			: 1.0f;
		SavedOwnerCustomTimeDilation = FMath::Max(0.0001f, Owner->CustomTimeDilation);
		PresentationStartRealSeconds = RealTimeSeconds;
		ApplyOverlayToCharacterMeshes();
		bPresentationActive = true;
	}

	PresentationReleaseRealSeconds = RealTimeSeconds + FMath::Max(0.0f, SlowMotionHoldRealSeconds);
	LastPresentationRealSeconds = RealTimeSeconds;
	UpdatePresentation(RealTimeSeconds);
}

void UTunaSweeperScratchComponent::UpdatePresentation(double RealTimeSeconds)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!bPresentationActive || !Owner || !World)
	{
		RestorePresentation();
		return;
	}

	const float BlendInSeconds = FMath::Max(0.001f, SlowMotionBlendInRealSeconds);
	const float BlendOutSeconds = FMath::Max(0.001f, SlowMotionBlendOutRealSeconds);
	float EffectAlpha = 1.0f;
	if (RealTimeSeconds < PresentationStartRealSeconds + BlendInSeconds)
	{
		EffectAlpha = FMath::Clamp(
			static_cast<float>((RealTimeSeconds - PresentationStartRealSeconds) / BlendInSeconds),
			0.0f,
			1.0f);
	}
	else if (!bDeveloperAlwaysSlowPresentationEnabled && RealTimeSeconds >= PresentationReleaseRealSeconds)
	{
		EffectAlpha = 1.0f - FMath::Clamp(
			static_cast<float>((RealTimeSeconds - PresentationReleaseRealSeconds) / BlendOutSeconds),
			0.0f,
			1.0f);
	}

	if (EffectAlpha <= KINDA_SMALL_NUMBER && RealTimeSeconds >= PresentationReleaseRealSeconds)
	{
		RestorePresentation();
		return;
	}

	const float SlowScale = FMath::Clamp(WorldSlowMotionScale, 0.01f, 1.0f);
	const float CurrentWorldDilation = FMath::Lerp(SavedWorldTimeDilation, SavedWorldTimeDilation * SlowScale, EffectAlpha);
	UGameplayStatics::SetGlobalTimeDilation(World, CurrentWorldDilation);

	const float DesiredPlayerRelativeScale = FMath::Lerp(1.0f, FMath::Max(0.01f, PlayerEffectiveTimeScale), EffectAlpha);
	Owner->CustomTimeDilation = FMath::Clamp(
		SavedOwnerCustomTimeDilation * SavedWorldTimeDilation * DesiredPlayerRelativeScale /
			FMath::Max(0.0001f, CurrentWorldDilation),
		0.01f,
		10.0f);

	UpdateOverlayMaterial(EffectAlpha, RealTimeSeconds);
}

void UTunaSweeperScratchComponent::RestorePresentation()
{
	if (bPresentationActive)
	{
		if (UWorld* World = GetWorld())
		{
			UGameplayStatics::SetGlobalTimeDilation(World, SavedWorldTimeDilation);
		}
		if (AActor* Owner = GetOwner())
		{
			Owner->CustomTimeDilation = SavedOwnerCustomTimeDilation;
		}
	}

	RestoreCharacterMeshOverlays();
	bPresentationActive = false;
}

void UTunaSweeperScratchComponent::ApplyOverlayToCharacterMeshes()
{
	AActor* Owner = GetOwner();
	UMaterialInterface* OverlayMaterial = ScratchOverlayMaterial.LoadSynchronous();
	if (!Owner || !OverlayMaterial)
	{
		return;
	}

	ScratchOverlayMaterialInstance = UMaterialInstanceDynamic::Create(OverlayMaterial, this);
	if (!ScratchOverlayMaterialInstance)
	{
		return;
	}

	ActiveOverlayMeshes.Reset();
	SavedOverlayMaterials.Reset();
	TArray<UMeshComponent*> MeshComponents;
	Owner->GetComponents<UMeshComponent>(MeshComponents);
	for (UMeshComponent* MeshComponent : MeshComponents)
	{
		if (!MeshComponent)
		{
			continue;
		}

		ActiveOverlayMeshes.Add(MeshComponent);
		SavedOverlayMaterials.Add(MeshComponent->GetOverlayMaterial());
		MeshComponent->SetOverlayMaterial(ScratchOverlayMaterialInstance);
		MeshComponent->SetOverlayMaterialMaxDrawDistance(0.0f);
	}
}

void UTunaSweeperScratchComponent::RestoreCharacterMeshOverlays()
{
	for (int32 MeshIndex = 0; MeshIndex < ActiveOverlayMeshes.Num(); ++MeshIndex)
	{
		if (UMeshComponent* MeshComponent = ActiveOverlayMeshes[MeshIndex])
		{
			UMaterialInterface* SavedMaterial = SavedOverlayMaterials.IsValidIndex(MeshIndex)
				? SavedOverlayMaterials[MeshIndex].Get()
				: nullptr;
			MeshComponent->SetOverlayMaterial(SavedMaterial);
		}
	}

	ActiveOverlayMeshes.Reset();
	SavedOverlayMaterials.Reset();
	ScratchOverlayMaterialInstance = nullptr;
}

void UTunaSweeperScratchComponent::UpdateOverlayMaterial(float EffectAlpha, double RealTimeSeconds)
{
	if (!ScratchOverlayMaterialInstance)
	{
		return;
	}

	const float HueDegrees = FMath::Fmod(
		static_cast<float>(RealTimeSeconds) * TunaSweeperScratchPresentation::RainbowCyclesPerSecond * 360.0f,
		360.0f);
	const uint8 Hue = static_cast<uint8>(FMath::RoundToInt(HueDegrees / 360.0f * 255.0f));
	const FLinearColor RainbowColor = FLinearColor::MakeFromHSV8(
		Hue,
		TunaSweeperScratchPresentation::RainbowSaturation,
		255);
	ScratchOverlayMaterialInstance->SetVectorParameterValue(
		TunaSweeperScratchPresentation::ColorParameter,
		RainbowColor);
	ScratchOverlayMaterialInstance->SetScalarParameterValue(
		TunaSweeperScratchPresentation::AlphaParameter,
		FMath::Clamp(EffectAlpha, 0.0f, 1.0f));
	ScratchOverlayMaterialInstance->SetScalarParameterValue(
		TunaSweeperScratchPresentation::IntensityParameter,
		TunaSweeperScratchPresentation::EmissiveIntensity);
}
