#include "Debug/TunaSweeperMovementCheatSubsystem.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"

extern "C" DLLEXPORT FTunaSweeperMovementCheatControl TunaSweeperMovementCheatControl;
FTunaSweeperMovementCheatControl TunaSweeperMovementCheatControl;
static_assert(sizeof(FTunaSweeperMovementCheatControl) == 32);
static_assert(STRUCT_OFFSET(FTunaSweeperMovementCheatControl, EnableToken) == 16);

namespace
{
	constexpr uint32 EnableToken = 0x54534D43;
	constexpr float Speeds[] = { 1.0f, 3.0f, 8.0f };
	bool IsValidPosition(const FVector& Position)
	{
		return !Position.ContainsNaN() && Position.GetAbsMax() <= 100000000.0;
	}
}

void UTunaSweeperMovementCheatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	TunaSweeperMovementCheatControl.ProcessId = FPlatformProcess::GetCurrentProcessId();
}

void UTunaSweeperMovementCheatSubsystem::Deinitialize()
{
	SpeedIndex = 0;
	Super::Deinitialize();
}

TStatId UTunaSweeperMovementCheatSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTunaSweeperMovementCheatSubsystem, STATGROUP_Tickables);
}

bool UTunaSweeperMovementCheatSubsystem::IsEnabled() const
{
	const UWorld* World = GetWorld();
	return World && World->IsGameWorld() && World->GetNetMode() == NM_Standalone &&
		TunaSweeperMovementCheatControl.EnableToken == EnableToken;
}

void UTunaSweeperMovementCheatSubsystem::Tick(float DeltaTime)
{
	if (!IsEnabled()) SpeedIndex = 0;
}

void UTunaSweeperMovementCheatSubsystem::CycleSpeed()
{
	if (IsEnabled()) SpeedIndex = (SpeedIndex + 1) % UE_ARRAY_COUNT(Speeds);
}

float UTunaSweeperMovementCheatSubsystem::GetSpeedMultiplier() const
{
	return IsEnabled() ? Speeds[SpeedIndex] : 1.0f;
}

bool UTunaSweeperMovementCheatSubsystem::IsCombatSuppressed() const
{
	return GetSpeedMultiplier() > 1.0f;
}

bool UTunaSweeperMovementCheatSubsystem::IsCombatSuppressedFor(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* Instance = World ? World->GetGameInstance() : nullptr;
	const auto* Cheats = Instance ? Instance->GetSubsystem<UTunaSweeperMovementCheatSubsystem>() : nullptr;
	return Cheats && Cheats->IsCombatSuppressed();
}

FName UTunaSweeperMovementCheatSubsystem::GetCurrentMapId(const UObject* WorldContext)
{
	return FName(*UGameplayStatics::GetCurrentLevelName(WorldContext, true));
}

bool UTunaSweeperMovementCheatSubsystem::TryTeleportToMapPosition(FName ExpectedMapId, const FVector& WorldPosition)
{
	if (!IsEnabled() || ExpectedMapId.IsNone() || ExpectedMapId != GetCurrentMapId(this) || !IsValidPosition(WorldPosition)) return false;
	UWorld* World = GetWorld();
	auto* Player = Cast<ATunaSweeperTopDownCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!World || !Player || Player->IsDead() ||
		Player->GetAttachParentActor() || !Player->GetCapsuleComponent()) return false;
	FCollisionQueryParams Query(SCENE_QUERY_STAT(MovementCheatTeleport), false, Player);
	FHitResult Hit;
	const FVector Target = WorldPosition;
	if (!World->LineTraceSingleByChannel(Hit, Target + FVector(0, 0, 100000), Target - FVector(0, 0, 100000), ECC_Visibility, Query) ||
		!Player->GetCharacterMovement()->IsWalkable(Hit)) return false;
	const float HalfHeight = Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const float Radius = Player->GetCapsuleComponent()->GetScaledCapsuleRadius();
	const FVector Destination = Hit.ImpactPoint + FVector(0, 0, HalfHeight + 3.0f);
	if (World->OverlapBlockingTestByChannel(Destination, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Radius, HalfHeight), Query)) return false;
	if (!Player->TeleportTo(Destination, Player->GetActorRotation(), false, false)) return false;
	Player->GetCharacterMovement()->StopMovementImmediately();
	return true;
}
