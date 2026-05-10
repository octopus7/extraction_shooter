#include "Game/TunaSweeperGameMode.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Player/TunaSweeperPlayerController.h"

ATunaSweeperGameMode::ATunaSweeperGameMode()
{
	DefaultPawnClass = ATunaSweeperTopDownCharacter::StaticClass();
	PlayerControllerClass = ATunaSweeperPlayerController::StaticClass();
}
