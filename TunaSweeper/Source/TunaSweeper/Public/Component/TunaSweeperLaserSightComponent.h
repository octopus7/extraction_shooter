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

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Laser Sight")
	void SetBeamEnd(const FVector& InBeamEnd);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Laser Sight")
	FVector GetBeamEnd() const { return BeamEnd; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Laser Sight|Debug")
	void SetLaserSightDebugEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Laser Sight|Debug")
	bool IsLaserSightDebugEnabled() const { return bLaserSightDebugEnabled; }

protected:
	virtual void BeginPlay() override;

private:
	void ApplyLaserSightParameters();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Laser Sight", meta = (AllowPrivateAccess = "true"))
	FVector BeamEnd = FVector(200.0f, 0.0f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Laser Sight|Debug", meta = (AllowPrivateAccess = "true"))
	bool bLaserSightDebugEnabled = false;

	UPROPERTY(Transient)
	bool bLaserSightEnabled = false;
};
