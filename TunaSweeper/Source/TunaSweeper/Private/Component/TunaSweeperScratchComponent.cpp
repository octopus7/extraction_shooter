#include "Component/TunaSweeperScratchComponent.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/MeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
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
	constexpr uint8 OverlayRainbowSaturation = 112;
	constexpr uint8 AfterimageRainbowSaturation = 220;
	constexpr float RainbowStartHueDegrees = 180.0f;
	constexpr float RainbowCyclesPerRoll = 1.0f;
}

UTunaSweeperScratchComponent::UTunaSweeperScratchComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	ScratchOverlayMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Effects/M_PlayerScratchRainbowOverlay.M_PlayerScratchRainbowOverlay")));
	ScratchAfterimageMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Effects/M_PlayerScratchAfterimage.M_PlayerScratchAfterimage")));
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
	DestroyAllAfterimages();
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
	const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(GetOwner());
	UpdateRollRainbowProgress(PlayerCharacter);
	if (bDeveloperAlwaysSlowPresentationEnabled && PlayerCharacter && PlayerCharacter->IsRolling())
	{
		// Keep the preview alive throughout the roll, independent of near-miss and scratch gain.
		TriggerPresentation(RealTimeSeconds);
	}
	else if (bPresentationActive)
	{
		UpdatePresentation(RealTimeSeconds);
	}

	UpdateAfterimages(RealTimeSeconds);
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
		const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(GetOwner());
		if (PlayerCharacter && PlayerCharacter->IsRolling())
		{
			TriggerPresentation(RealTimeSeconds);
		}
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
		LastAfterimageLocation = Owner->GetActorLocation();
		LastAfterimageSpawnRealSeconds = RealTimeSeconds - FMath::Max(0.01f, AfterimageIntervalRealSeconds);
		bHasLastAfterimageLocation = true;
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
	const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(Owner);
	const bool bHoldForDeveloperRollPreview =
		bDeveloperAlwaysSlowPresentationEnabled && PlayerCharacter && PlayerCharacter->IsRolling();
	float EffectAlpha = 1.0f;
	if (RealTimeSeconds < PresentationStartRealSeconds + BlendInSeconds)
	{
		EffectAlpha = FMath::Clamp(
			static_cast<float>((RealTimeSeconds - PresentationStartRealSeconds) / BlendInSeconds),
			0.0f,
			1.0f);
	}
	else if (!bHoldForDeveloperRollPreview && RealTimeSeconds >= PresentationReleaseRealSeconds)
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

	PresentationEffectAlpha = EffectAlpha;

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
	PresentationEffectAlpha = 0.0f;
	bHasLastAfterimageLocation = false;
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
	(void)RealTimeSeconds;
	if (!ScratchOverlayMaterialInstance)
	{
		return;
	}

	const float HueDegrees = GetRollRainbowHueDegrees();
	const uint8 Hue = static_cast<uint8>(FMath::RoundToInt(HueDegrees / 360.0f * 255.0f));
	const FLinearColor RainbowColor = FLinearColor::MakeFromHSV8(
		Hue,
		TunaSweeperScratchPresentation::OverlayRainbowSaturation,
		255);
	ScratchOverlayMaterialInstance->SetVectorParameterValue(
		TunaSweeperScratchPresentation::ColorParameter,
		RainbowColor);
	ScratchOverlayMaterialInstance->SetScalarParameterValue(
		TunaSweeperScratchPresentation::AlphaParameter,
		FMath::Clamp(EffectAlpha * CharacterOverlayStrength, 0.0f, 1.0f));
	ScratchOverlayMaterialInstance->SetScalarParameterValue(
		TunaSweeperScratchPresentation::IntensityParameter,
		TunaSweeperScratchPresentation::EmissiveIntensity);
}

void UTunaSweeperScratchComponent::UpdateRollRainbowProgress(
	const ATunaSweeperTopDownCharacter* PlayerCharacter)
{
	const bool bIsRolling = PlayerCharacter && PlayerCharacter->IsRolling();
	if (bIsRolling)
	{
		RollRainbowProgress = PlayerCharacter->GetRollNormalizedProgress();
	}
	else if (bWasOwnerRolling)
	{
		// Keep the deterministic end color while the presentation blends out.
		RollRainbowProgress = 1.0f;
	}

	bWasOwnerRolling = bIsRolling;
}

float UTunaSweeperScratchComponent::GetRollRainbowHueDegrees() const
{
	return FMath::Fmod(
		TunaSweeperScratchPresentation::RainbowStartHueDegrees +
			FMath::Clamp(RollRainbowProgress, 0.0f, 1.0f) *
			TunaSweeperScratchPresentation::RainbowCyclesPerRoll * 360.0f,
		360.0f);
}

void UTunaSweeperScratchComponent::UpdateAfterimages(double RealTimeSeconds)
{
	const float SafeLifetime = FMath::Max(0.01f, AfterimageLifetimeRealSeconds);
	for (int32 AfterimageIndex = ActiveAfterimages.Num() - 1; AfterimageIndex >= 0; --AfterimageIndex)
	{
		FActiveAfterimage& Afterimage = ActiveAfterimages[AfterimageIndex];
		UPoseableMeshComponent* GhostMesh = Afterimage.Mesh.Get();
		UMaterialInstanceDynamic* GhostMaterial = Afterimage.Material.Get();
		const float AgeAlpha = FMath::Clamp(
			static_cast<float>((RealTimeSeconds - Afterimage.SpawnRealSeconds) / SafeLifetime),
			0.0f,
			1.0f);
		if (!GhostMesh || AgeAlpha >= 1.0f)
		{
			if (GhostMesh)
			{
				GhostMesh->DestroyComponent();
			}
			ActiveAfterimages.RemoveAtSwap(AfterimageIndex, 1, EAllowShrinking::No);
			continue;
		}

		if (GhostMaterial)
		{
			GhostMaterial->SetScalarParameterValue(
				TunaSweeperScratchPresentation::AlphaParameter,
				FMath::Square(1.0f - AgeAlpha));
		}
	}

	ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(GetOwner());
	if (!PlayerCharacter || !PlayerCharacter->IsRolling())
	{
		// Stop emitting when the roll finishes, but leave existing ghosts alive to fade naturally.
		bHasLastAfterimageLocation = false;
		return;
	}

	if (!bPresentationActive || PresentationEffectAlpha < 0.18f)
	{
		return;
	}

	const double SafeInterval = FMath::Max(0.01f, AfterimageIntervalRealSeconds);
	const FVector CurrentLocation = PlayerCharacter->GetActorLocation();
	const float MinimumTravelSquared = FMath::Square(FMath::Max(0.0f, AfterimageMinimumTravelCm));
	if (RealTimeSeconds - LastAfterimageSpawnRealSeconds >= SafeInterval &&
		(!bHasLastAfterimageLocation || FVector::DistSquared(CurrentLocation, LastAfterimageLocation) >= MinimumTravelSquared))
	{
		SpawnAfterimage(RealTimeSeconds);
		LastAfterimageSpawnRealSeconds = RealTimeSeconds;
		LastAfterimageLocation = CurrentLocation;
		bHasLastAfterimageLocation = true;
	}
}

void UTunaSweeperScratchComponent::SpawnAfterimage(double RealTimeSeconds)
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	UMaterialInterface* BaseAfterimageMaterial = ScratchAfterimageMaterial.LoadSynchronous();
	if (!Owner || !World || !BaseAfterimageMaterial)
	{
		return;
	}

	const float HueDegrees = GetRollRainbowHueDegrees();
	const uint8 Hue = static_cast<uint8>(FMath::RoundToInt(HueDegrees / 360.0f * 255.0f));
	const FLinearColor GhostColor = FLinearColor::MakeFromHSV8(
		Hue,
		TunaSweeperScratchPresentation::AfterimageRainbowSaturation,
		255);
	FVector CameraLocation = FVector::ZeroVector;
	FRotator CameraRotation = FRotator::ZeroRotator;
	const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	const bool bHasCameraView = PlayerController != nullptr;
	if (PlayerController)
	{
		PlayerController->GetPlayerViewPoint(CameraLocation, CameraRotation);
	}

	TArray<USkeletalMeshComponent*> SourceMeshes;
	Owner->GetComponents<USkeletalMeshComponent>(SourceMeshes);
	for (USkeletalMeshComponent* SourceMesh : SourceMeshes)
	{
		if (!SourceMesh || !SourceMesh->IsVisible() || !SourceMesh->GetSkeletalMeshAsset())
		{
			continue;
		}

		UPoseableMeshComponent* GhostMesh = NewObject<UPoseableMeshComponent>(Owner);
		UMaterialInstanceDynamic* GhostMaterial = UMaterialInstanceDynamic::Create(BaseAfterimageMaterial, GhostMesh);
		if (!GhostMesh || !GhostMaterial)
		{
			continue;
		}

		Owner->AddInstanceComponent(GhostMesh);
		GhostMesh->SetSkinnedAssetAndUpdate(SourceMesh->GetSkeletalMeshAsset(), true);
		FTransform GhostTransform = SourceMesh->GetComponentTransform();
		if (bHasCameraView)
		{
			const FVector AwayFromCamera = (GhostTransform.GetLocation() - CameraLocation).GetSafeNormal();
			GhostTransform.AddToTranslation(AwayFromCamera * FMath::Max(0.0f, AfterimageDepthBiasCm));
		}
		GhostMesh->SetWorldTransform(GhostTransform);
		GhostMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		GhostMesh->SetGenerateOverlapEvents(false);
		GhostMesh->SetCastShadow(false);
		GhostMesh->SetReceivesDecals(false);
		GhostMesh->SetComponentTickEnabled(false);
		GhostMesh->RegisterComponentWithWorld(World);
		GhostMesh->CopyPoseFromSkeletalComponent(SourceMesh);

		const int32 MaterialCount = FMath::Max(1, SourceMesh->GetNumMaterials());
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			GhostMesh->SetMaterial(MaterialIndex, GhostMaterial);
		}
		GhostMaterial->SetVectorParameterValue(TunaSweeperScratchPresentation::ColorParameter, GhostColor);
		GhostMaterial->SetScalarParameterValue(TunaSweeperScratchPresentation::AlphaParameter, 1.0f);
		GhostMaterial->SetScalarParameterValue(TunaSweeperScratchPresentation::IntensityParameter, 0.85f);
		ActiveAfterimages.Add({GhostMesh, GhostMaterial, RealTimeSeconds});
	}
}

void UTunaSweeperScratchComponent::DestroyAllAfterimages()
{
	for (FActiveAfterimage& Afterimage : ActiveAfterimages)
	{
		if (UPoseableMeshComponent* GhostMesh = Afterimage.Mesh.Get())
		{
			GhostMesh->DestroyComponent();
		}
	}
	ActiveAfterimages.Reset();
}
