#pragma once

#include "CoreMinimal.h"
#include "NiagaraComponent.h"
#include "TunaSweeperLaserSightComponent.generated.h"

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperLaserSightComponent : public UNiagaraComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperLaserSightComponent();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Laser Sight")
	void SetLaserSightEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Laser Sight")
	bool IsLaserSightEnabled() const { return bLaserSightEnabled; }

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(Transient)
	bool bLaserSightEnabled = false;
};
