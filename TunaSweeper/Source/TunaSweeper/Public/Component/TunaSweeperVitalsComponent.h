#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TunaSweeperVitalsComponent.generated.h"

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperVitalsState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Food = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxFood = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Hydration = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxHydration = 100.0f;

	void Normalize();
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperVitalsDelta
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals")
	float Health = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals")
	float Food = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals")
	float Hydration = 0.0f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperVitalsDepletionRates
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HealthPerSecond = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float FoodPerSecond = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float HydrationPerSecond = 0.04f;

	void ClampNonNegative();
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperVitalsDepletionMultipliers
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Health = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Food = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vitals", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float Hydration = 1.0f;

	void ClampNonNegative();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunaSweeperVitalsChangedSignature, const FTunaSweeperVitalsState&, VitalsState);

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperVitalsComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperVitalsComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(BlueprintAssignable, Category = "TunaSweeper|Vitals")
	FTunaSweeperVitalsChangedSignature OnVitalsChanged;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Vitals")
	const FTunaSweeperVitalsState& GetVitalsState() const { return VitalsState; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Vitals")
	FTunaSweeperVitalsDepletionRates GetEffectiveDepletionRates() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vitals")
	void ApplyVitalsDelta(const FTunaSweeperVitalsDelta& Delta);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vitals")
	void ApplyConsumableVitalsEffect(const FTunaSweeperVitalsDelta& Effect);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vitals")
	void ApplyActionVitalsCost(const FTunaSweeperVitalsDelta& Cost);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "TunaSweeper|Vitals")
	void ServerApplyVitalsDelta(const FTunaSweeperVitalsDelta& Delta);

	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "TunaSweeper|Vitals")
	void ServerApplyActionVitalsCost(const FTunaSweeperVitalsDelta& Cost);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vitals")
	void SetBaseDepletionRates(const FTunaSweeperVitalsDepletionRates& NewBaseRates);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vitals")
	void SetDepletionRateAdditions(const FTunaSweeperVitalsDepletionRates& NewAdditionalRates);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vitals")
	void SetDepletionRateMultipliers(const FTunaSweeperVitalsDepletionMultipliers& NewMultipliers);

protected:
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_VitalsState, Category = "TunaSweeper|Vitals")
	FTunaSweeperVitalsState VitalsState;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_DepletionSettings, Category = "TunaSweeper|Vitals")
	FTunaSweeperVitalsDepletionRates BaseDepletionRates;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_DepletionSettings, Category = "TunaSweeper|Vitals")
	FTunaSweeperVitalsDepletionRates AdditionalDepletionRates;

	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_DepletionSettings, Category = "TunaSweeper|Vitals")
	FTunaSweeperVitalsDepletionMultipliers DepletionRateMultipliers;

	UFUNCTION()
	void OnRep_VitalsState();

	UFUNCTION()
	void OnRep_DepletionSettings();

private:
	bool HasAuthority() const;
	void ApplyVitalsDeltaInternal(const FTunaSweeperVitalsDelta& Delta);
	void BroadcastVitalsChanged();
};
