#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Templates/SubclassOf.h"
#include "TunaSweeperHeadphoneListenerComponent.generated.h"

struct FTunaSweeperNoiseEvent;
class UTunaSweeperHeadphoneRippleWidget;

UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperHeadphoneListenerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperHeadphoneListenerComponent();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Noise")
	bool IsHeadphoneEquipped() const { return bHeadphoneEquipped; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Noise")
	void RefreshEquippedHeadphone();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise")
	TSubclassOf<UTunaSweeperHeadphoneRippleWidget> RippleWidgetClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DefaultHearingRange = 1600.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DefaultSensitivity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float DefaultMinStrength = 0.12f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float SelfNoiseIgnoreDistance = 120.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinVisualNoiseDistance = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Noise", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RippleCooldownSeconds = 0.08f;

private:
	void BindDelegates();
	void UnbindDelegates();
	void HandleNoiseReported(const FTunaSweeperNoiseEvent& NoiseEvent);
	UTunaSweeperHeadphoneRippleWidget* EnsureRippleWidget();
	void RemoveRippleWidget();
	bool ShouldIgnoreNoiseSource(const FTunaSweeperNoiseEvent& NoiseEvent) const;
	bool IsListenerPawnReady() const;

	bool bHeadphoneEquipped = false;
	float CachedHearingRange = 0.0f;
	float CachedSensitivity = 0.0f;
	float CachedMinStrength = 0.0f;
	float LastRippleSpawnTimeSeconds = -1000.0f;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperHeadphoneRippleWidget> RippleWidget;
};
