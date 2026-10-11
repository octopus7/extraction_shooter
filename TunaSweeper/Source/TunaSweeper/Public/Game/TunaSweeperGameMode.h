#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TunaSweeperGameMode.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATunaSweeperGameMode();
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	virtual AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName) override;

private:
	bool bLiftArrivalMissingStart = false;
};
