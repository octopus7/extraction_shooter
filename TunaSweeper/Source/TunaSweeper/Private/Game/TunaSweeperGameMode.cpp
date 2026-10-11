#include "Game/TunaSweeperGameMode.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperRaidArrivalSubsystem.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Engine/PlayerStartPIE.h"

ATunaSweeperGameMode::ATunaSweeperGameMode()
{
	DefaultPawnClass = ATunaSweeperTopDownCharacter::StaticClass();
	PlayerControllerClass = ATunaSweeperPlayerController::StaticClass();
}

AActor* ATunaSweeperGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	if (UGameInstance* Instance = GetGameInstance())
	{
		if (auto* Arrival = Instance->GetSubsystem<UTunaSweeperRaidArrivalSubsystem>())
		{
			const auto Group = Arrival->ConsumeArrival(GetWorld());
			if (Group != ETunaSweeperRaidArrivalGroup::None)
			{
				TArray<APlayerStart*> Starts;
				for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It) Starts.Add(*It);
				FRandomStream Random(FMath::Rand());
				APlayerStart* Start = Arrival->SelectRandomStart(Starts, Group, Random);
				bLiftArrivalMissingStart = Start == nullptr;
				if (!Start) UE_LOG(LogTemp, Warning, TEXT("Lift arrival has no PlayerStart for group %d in %s"), static_cast<int32>(Group), *GetWorld()->GetMapName());
				return Start;
			}
		}
	}
	// Dedicated lift arrivals must not change ordinary travel/PIE starts. Preserve
	// the engine's clear-then-teleportable preference within the ordinary pool.
	UClass* PawnClass = GetDefaultPawnClassForController(Player);
	APawn* PawnToFit = PawnClass ? PawnClass->GetDefaultObject<APawn>() : nullptr;
	TArray<APlayerStart*> ClearStarts;
	TArray<APlayerStart*> AdjustableStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		APlayerStart* Start = *It;
		if (Start->IsA<ATunaSweeperRaidPlayerStart>()) continue;
		if (Start->IsA<APlayerStartPIE>()) return Start;
		FVector Location = Start->GetActorLocation();
		const FRotator Rotation = Start->GetActorRotation();
		if (!GetWorld()->EncroachingBlockingGeometry(PawnToFit, Location, Rotation)) ClearStarts.Add(Start);
		else if (GetWorld()->FindTeleportSpot(PawnToFit, Location, Rotation)) AdjustableStarts.Add(Start);
	}
	const TArray<APlayerStart*>& Starts = ClearStarts.IsEmpty() ? AdjustableStarts : ClearStarts;
	return Starts.IsEmpty() ? nullptr : Starts[FMath::RandRange(0, Starts.Num() - 1)];
}

AActor* ATunaSweeperGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	if (bLiftArrivalMissingStart) return nullptr;
	AActor* Start = Super::FindPlayerStart_Implementation(Player, IncomingName);
	// GameModeBase substitutes world origin for nullptr; a missing lift group must
	// remain an authoring failure instead of silently spawning outside its region.
	return bLiftArrivalMissingStart ? nullptr : Start;
}
