#include "TunaSweeperGameInstanceShared.h"

#include "AI/TunaSweeperPetCompanionCharacter.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

ATunaSweeperPetCompanionCharacter* UTunaSweeperGameInstance::SpawnPetCompanionForPlayer(bool bReplaceExisting)
{
	return SpawnPetCompanionForPawn(GetWorld() ? UGameplayStatics::GetPlayerPawn(GetWorld(), 0) : nullptr, bReplaceExisting);
}

ATunaSweeperPetCompanionCharacter* UTunaSweeperGameInstance::SpawnPetCompanionForPawn(APawn* TargetPawn, bool bReplaceExisting)
{
	if (!TargetPawn)
	{
		return nullptr;
	}

	if (IsValid(CurrentPetCompanion))
	{
		if (!bReplaceExisting)
		{
			CurrentPetCompanion->SetFollowTarget(TargetPawn);
			return CurrentPetCompanion;
		}

		DespawnPetCompanion();
	}

	UWorld* World = TargetPawn->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	TSubclassOf<ATunaSweeperPetCompanionCharacter> CompanionClass = PetCompanionClass;
	if (!CompanionClass)
	{
		CompanionClass = ATunaSweeperPetCompanionCharacter::StaticClass();
	}

	const FVector TargetLocation = TargetPawn->GetActorLocation();
	FVector SpawnDirection = -TargetPawn->GetActorForwardVector();
	SpawnDirection.Z = 0.0f;
	if (!SpawnDirection.Normalize())
	{
		SpawnDirection = FVector(-1.0f, 0.0f, 0.0f);
	}

	const FVector SpawnLocation = TargetLocation + SpawnDirection * FMath::Max(0.0f, PetCompanionSpawnDistance);
	const FRotator SpawnRotation = TargetPawn->GetActorRotation();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = TargetPawn;
	SpawnParameters.Instigator = TargetPawn;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	CurrentPetCompanion = World->SpawnActor<ATunaSweeperPetCompanionCharacter>(
		CompanionClass,
		SpawnLocation,
		SpawnRotation,
		SpawnParameters);
	if (!CurrentPetCompanion)
	{
		UE_LOG(LogTunaSweeperGameInstance, Warning, TEXT("Failed to spawn pet companion for %s."), *GetNameSafe(TargetPawn));
		return nullptr;
	}

	if (!CurrentPetCompanion->GetController())
	{
		CurrentPetCompanion->SpawnDefaultController();
	}
	CurrentPetCompanion->SetFollowTarget(TargetPawn);

	return CurrentPetCompanion;
}

void UTunaSweeperGameInstance::DespawnPetCompanion()
{
	if (IsValid(CurrentPetCompanion))
	{
		CurrentPetCompanion->Destroy();
	}

	CurrentPetCompanion = nullptr;
}
