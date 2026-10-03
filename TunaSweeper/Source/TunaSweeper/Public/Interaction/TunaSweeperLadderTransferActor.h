#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperLadderTransferActor.generated.h"

class ATunaSweeperTopDownCharacter;

/** A paired, same-world ladder endpoint. ArrivalPoint is authored at the walkable floor. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLadderTransferActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLadderTransferActor();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Ladder")
	void SetTargetEndpoint(ATunaSweeperLadderTransferActor* InTargetEndpoint);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Ladder")
	USceneComponent* GetArrivalPoint() const { return ArrivalPoint; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Ladder")
	bool CanTransferPlayer(APawn* InstigatorPawn) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Ladder")
	bool TryTransferPlayer(APawn* InstigatorPawn);

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "TunaSweeper|Ladder")
	TObjectPtr<ATunaSweeperLadderTransferActor> TargetEndpoint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Ladder")
	TObjectPtr<USceneComponent> ArrivalPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Ladder", meta = (ClampMin = "0.0", Units = "cm"))
	float MaxSourceHeightDifferenceCm = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Ladder", meta = (ClampMin = "0.0", Units = "cm"))
	float ArrivalClearanceCm = 2.0f;

private:
	bool ResolveTransferDestination(APawn* InstigatorPawn, FVector& OutLocation, FRotator& OutRotation) const;
};
