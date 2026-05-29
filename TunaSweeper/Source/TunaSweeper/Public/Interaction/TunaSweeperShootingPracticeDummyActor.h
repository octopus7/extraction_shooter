#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperShootingPracticeDummyActor.generated.h"

class USceneComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UWidgetComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperShootingPracticeDummyActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperShootingPracticeDummyActor();

	virtual float TakeDamage(
		float DamageAmount,
		struct FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Practice Dummy")
	void ConfigurePracticeDummyDefaults(
		float InMaxHealth,
		float InCriticalDamageMultiplier,
		float InHeadshotDamageMultiplier,
		float InHealthRecoverySeconds);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Practice Dummy")
	float GetHealthFraction() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> CriticalPlateMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HeadshotPlateMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> HealthBarWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Practice Dummy", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Practice Dummy", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinimumHealth = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Practice Dummy", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float CriticalDamageMultiplier = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Practice Dummy", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float HeadshotDamageMultiplier = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Practice Dummy", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float HealthRecoverySeconds = 2.0f;

private:
	void ConfigureHitComponent(UStaticMeshComponent* MeshComponent) const;
	void ApplyHitZoneColors();
	float ResolveDamageMultiplier(FDamageEvent const& DamageEvent, AActor* DamageCauser) const;
	bool IsHeadshotComponent(const UPrimitiveComponent* Component) const;
	bool IsCriticalComponent(const UPrimitiveComponent* Component) const;
	void ApplyDummyDamage(float DamageAmount);
	void RefreshHealthBar();

	float CurrentHealth = 100.0f;
};
