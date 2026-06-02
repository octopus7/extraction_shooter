#include "Component/TunaSweeperWeaponCombatComponent.h"

#include "Engine/World.h"

UTunaSweeperWeaponCombatComponent::UTunaSweeperWeaponCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UTunaSweeperWeaponCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	BroadcastReloadProgressIfNeeded(true);
}

void UTunaSweeperWeaponCombatComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	DecaySpreadRecoil(DeltaTime);
	BroadcastReloadProgressIfNeeded();
}

void UTunaSweeperWeaponCombatComponent::ConfigureSpreadRecoilDefinition(
	FName InWeaponTypeTag,
	const FTunaSweeperWeaponSpreadRecoilDefinition& InDefinition)
{
	if (InWeaponTypeTag.IsNone())
	{
		ClearSpreadRecoilDefinition();
		return;
	}

	const bool bWeaponTypeChanged = SpreadRecoilWeaponTypeTag != InWeaponTypeTag;
	SpreadRecoilDefinition = InDefinition;
	SpreadRecoilDefinition.WeaponTypeTag = InWeaponTypeTag;
	SpreadRecoilDefinition.IncreasePerShot = FMath::Max(0.0f, SpreadRecoilDefinition.IncreasePerShot);
	SpreadRecoilDefinition.MinimumSpreadHalfAngleDegrees =
		FMath::Max(0.0f, SpreadRecoilDefinition.MinimumSpreadHalfAngleDegrees);
	SpreadRecoilDefinition.MaximumSpreadHalfAngleDegrees = FMath::Max(
		SpreadRecoilDefinition.MinimumSpreadHalfAngleDegrees,
		SpreadRecoilDefinition.MaximumSpreadHalfAngleDegrees);
	SpreadRecoilDefinition.DecreasePerSecond = FMath::Max(0.0f, SpreadRecoilDefinition.DecreasePerSecond);

	SpreadRecoilWeaponTypeTag = InWeaponTypeTag;
	bHasSpreadRecoilDefinition = true;
	if (bWeaponTypeChanged)
	{
		SpreadRecoilOffsetDegrees = FVector2D::ZeroVector;
	}
}

void UTunaSweeperWeaponCombatComponent::ClearSpreadRecoilDefinition()
{
	bHasSpreadRecoilDefinition = false;
	SpreadRecoilWeaponTypeTag = NAME_None;
	SpreadRecoilDefinition = FTunaSweeperWeaponSpreadRecoilDefinition();
	SpreadRecoilOffsetDegrees = FVector2D::ZeroVector;
}

void UTunaSweeperWeaponCombatComponent::ResetSpreadRecoil()
{
	SpreadRecoilOffsetDegrees = FVector2D::ZeroVector;
}

float UTunaSweeperWeaponCombatComponent::GetSpreadHalfAngleDegrees() const
{
	if (!bHasSpreadRecoilDefinition)
	{
		return 0.0f;
	}

	return FMath::Clamp(
		FMath::Max(SpreadRecoilDefinition.MinimumSpreadHalfAngleDegrees, SpreadRecoilOffsetDegrees.Size()),
		SpreadRecoilDefinition.MinimumSpreadHalfAngleDegrees,
		SpreadRecoilDefinition.MaximumSpreadHalfAngleDegrees);
}

void UTunaSweeperWeaponCombatComponent::AddSpreadRecoilShot()
{
	if (!bHasSpreadRecoilDefinition)
	{
		return;
	}

	const float KickMagnitude = FMath::Max(0.0f, SpreadRecoilDefinition.IncreasePerShot);
	if (KickMagnitude <= 0.0f)
	{
		return;
	}

	const float KickAngleRadians = FMath::FRandRange(0.0f, 2.0f * PI);
	SpreadRecoilOffsetDegrees += FVector2D(FMath::Cos(KickAngleRadians), FMath::Sin(KickAngleRadians)) * KickMagnitude;

	const float MaxMagnitude = FMath::Max(0.0f, SpreadRecoilDefinition.MaximumSpreadHalfAngleDegrees);
	const float CurrentMagnitude = SpreadRecoilOffsetDegrees.Size();
	if (MaxMagnitude > 0.0f && CurrentMagnitude > MaxMagnitude)
	{
		SpreadRecoilOffsetDegrees *= MaxMagnitude / CurrentMagnitude;
	}
}

bool UTunaSweeperWeaponCombatComponent::StartReload(float ReloadSeconds)
{
	const float SafeReloadSeconds = FMath::Max(0.01f, ReloadSeconds);
	if (bIsReloading)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	ReloadStartWorldSeconds = World ? World->GetTimeSeconds() : 0.0f;
	ReloadDurationSeconds = SafeReloadSeconds;
	LastBroadcastReloadProgress = -1.0f;
	bIsReloading = true;
	OnReloadStateChanged.Broadcast(true, ReloadDurationSeconds);
	BroadcastReloadProgressIfNeeded(true);
	return true;
}

void UTunaSweeperWeaponCombatComponent::FinishReload()
{
	if (!bIsReloading)
	{
		return;
	}

	LastBroadcastReloadProgress = 1.0f;
	OnReloadProgressChanged.Broadcast(1.0f);
	bIsReloading = false;
	ReloadStartWorldSeconds = 0.0f;
	ReloadDurationSeconds = 0.0f;
	OnReloadStateChanged.Broadcast(false, 0.0f);
}

void UTunaSweeperWeaponCombatComponent::CancelReload()
{
	if (!bIsReloading)
	{
		return;
	}

	bIsReloading = false;
	ReloadStartWorldSeconds = 0.0f;
	ReloadDurationSeconds = 0.0f;
	LastBroadcastReloadProgress = 0.0f;
	OnReloadProgressChanged.Broadcast(0.0f);
	OnReloadStateChanged.Broadcast(false, 0.0f);
}

bool UTunaSweeperWeaponCombatComponent::HasReloadFinished() const
{
	return bIsReloading && GetReloadProgress() >= 1.0f - KINDA_SMALL_NUMBER;
}

float UTunaSweeperWeaponCombatComponent::GetReloadProgress() const
{
	if (!bIsReloading || ReloadDurationSeconds <= 0.0f)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : ReloadStartWorldSeconds;
	return FMath::Clamp((CurrentTime - ReloadStartWorldSeconds) / ReloadDurationSeconds, 0.0f, 1.0f);
}

void UTunaSweeperWeaponCombatComponent::DecaySpreadRecoil(float DeltaSeconds)
{
	if (!bHasSpreadRecoilDefinition || DeltaSeconds <= 0.0f)
	{
		return;
	}

	const float CurrentMagnitude = SpreadRecoilOffsetDegrees.Size();
	if (CurrentMagnitude <= KINDA_SMALL_NUMBER)
	{
		SpreadRecoilOffsetDegrees = FVector2D::ZeroVector;
		return;
	}

	const float NewMagnitude = FMath::Max(
		0.0f,
		CurrentMagnitude - SpreadRecoilDefinition.DecreasePerSecond * DeltaSeconds);
	if (NewMagnitude <= KINDA_SMALL_NUMBER)
	{
		SpreadRecoilOffsetDegrees = FVector2D::ZeroVector;
		return;
	}

	SpreadRecoilOffsetDegrees *= NewMagnitude / CurrentMagnitude;
}

void UTunaSweeperWeaponCombatComponent::BroadcastReloadProgressIfNeeded(bool bForceBroadcast)
{
	if (!bIsReloading)
	{
		return;
	}

	const float ReloadProgress = GetReloadProgress();
	if (bForceBroadcast || !FMath::IsNearlyEqual(ReloadProgress, LastBroadcastReloadProgress, 0.01f))
	{
		LastBroadcastReloadProgress = ReloadProgress;
		OnReloadProgressChanged.Broadcast(ReloadProgress);
	}
}
