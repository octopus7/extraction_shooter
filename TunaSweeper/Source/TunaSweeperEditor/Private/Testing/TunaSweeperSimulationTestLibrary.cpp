#include "Testing/TunaSweeperSimulationTestLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
bool UTunaSweeperSimulationTestLibrary::ConfigureSimulationPlayer(APlayerController* Controller, APawn* Pawn)
{
 if (!IsValid(Controller) || !IsValid(Pawn) || !Controller->HasAuthority() ||
  Controller->GetWorld() != Pawn->GetWorld() || !Pawn->GetWorld() ||
  Pawn->GetWorld()->WorldType != EWorldType::PIE)
 {
  return false;
 }
 if (APlayerState* State = Controller->PlayerState)
 {
  State->SetIsOnlyASpectator(false);
  State->SetIsSpectator(false);
 }
 Controller->Possess(Pawn);
 return Controller->GetPawn() == Pawn;
}
