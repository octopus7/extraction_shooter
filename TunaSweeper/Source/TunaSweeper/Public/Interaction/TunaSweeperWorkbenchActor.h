#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperWorkbenchActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperWorkbenchActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperWorkbenchActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Workbench")
	int32 GetWorkbenchId() const { return WorkbenchId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Workbench")
	UTunaSweeperInteractableComponent* GetCraftInteractableComponent() const { return InteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Workbench")
	UTunaSweeperInteractableComponent* GetDismantleInteractableComponent() const { return DismantleInteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Workbench")
	UTunaSweeperInteractableComponent* GetBlueprintRegisterInteractableComponent() const { return BlueprintRegisterInteractableComponent; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	void SetWorkbenchId(int32 InWorkbenchId);

	void ConfigureWorkbenchDefaults(int32 InWorkbenchId);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Workbench", meta = (ClampMin = "1", UIMin = "1"))
	int32 WorkbenchId = 1;

	// Repair is intentionally reserved for a future workbench mode; no repair behavior is implemented yet.

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> DismantleInteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperInteractableComponent> BlueprintRegisterInteractableComponent;
};
