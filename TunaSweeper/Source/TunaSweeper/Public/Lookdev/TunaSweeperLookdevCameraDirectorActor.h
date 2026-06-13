#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperLookdevCameraDirectorActor.generated.h"

class ACameraActor;
class ANiagaraActor;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperLookdevCameraDirectorActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperLookdevCameraDirectorActor();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Lookdev")
	void ConfigureLookdev(ACameraActor* InForcedCameraActor, ANiagaraActor* InNiagaraEffectActor);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Lookdev")
	TObjectPtr<ACameraActor> ForcedCameraActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Lookdev")
	TObjectPtr<ANiagaraActor> NiagaraEffectActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Lookdev", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float EffectRestartDelaySeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Lookdev")
	bool bRestartEffectOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Lookdev")
	bool bLockViewTargetEveryTick = true;

private:
	void ForceCameraView() const;
	void RestartNiagaraEffect();

	float ElapsedSeconds = 0.0f;
	bool bRestartedEffect = false;
};
