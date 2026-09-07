#pragma once

#include "CoreMinimal.h"
#include "Combat/TunaSweeperBurnTypes.h"
#include "Components/ActorComponent.h"
#include "TunaSweeperBurnComponent.generated.h"

class AController;
class UNiagaraComponent;
class UNiagaraSystem;

/** Enemy-only, authoritative damage over time. Only the presentation flag is replicated. */
UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperBurnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperBurnComponent();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "TunaSweeper|Burn")
	bool TryApplyBurn(const FTunaSweeperBurnSpec& BurnSpec, AController* EventInstigator, AActor* DamageCauser);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "TunaSweeper|Burn")
	void ClearBurn();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Burn")
	bool IsBurning() const { return bIsBurning; }

	/** Remaining time is authoritative local state, not replicated to clients. */
	float GetRemainingSeconds() const { return static_cast<float>(RemainingSeconds); }
	float GetDamagePerTick() const { return ActiveDamagePerTick; }
	static bool CanBurnActor(const AActor* Actor);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Burn|Presentation")
	TSoftObjectPtr<UNiagaraSystem> BurningEffect;

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Burn|Presentation")
	FVector BurningEffectOffset = FVector(0.0f, 0.0f, -20.0f);

	UPROPERTY(EditDefaultsOnly, Category = "TunaSweeper|Burn|Presentation", meta = (ClampMin = "0.01", ClampMax = "10"))
	float BurningEffectScale = 0.65f;

private:
	UFUNCTION()
	void OnRep_IsBurning();

	void RefreshBurningEffect();
	void StopBurningEffect();
	AActor* ResolveDamageSource() const;
	bool CanApplyFromSource(const AActor* SourceActor) const;
	void ApplyBurnTick();

	UPROPERTY(ReplicatedUsing = OnRep_IsBurning)
	bool bIsBurning = false;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> BurningEffectComponent;

	TWeakObjectPtr<AController> BurnInstigator;
	TWeakObjectPtr<AActor> BurnDamageSource;
	double RemainingSeconds = 0.0;
	double TickAccumulator = 0.0;
	float ActiveDamagePerTick = 0.0f;
};
