#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TunaSweeperPlayerController.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATunaSweeperPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

private:
	bool GetMouseAimPointOnPlane(float PlaneZ, FVector& OutAimPoint) const;
};
