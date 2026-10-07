#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperDebugArmoryActor.generated.h"

/** Development-only supply interaction. In Shipping the rack remains a scenery prop. */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperDebugArmoryActor : public ATunaSweeperInteractableActor
{

	GENERATED_BODY()

public:
	ATunaSweeperDebugArmoryActor();
	virtual void PostInitializeComponents() override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debug Armory")
	static bool IsArmoryEnabled();

	static bool IsAllowedItem(const FTunaSweeperItemDefinition& Item);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debug Armory")
	bool GetSupplyCatalog(TArray<FTunaSweeperItemDefinition>& OutItems) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debug Armory")
	bool CanUseArmory(APawn* InstigatorPawn) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debug Armory")
	bool TrySupplyItem(APawn* InstigatorPawn, int32 ItemId, int32 Quantity);
};
