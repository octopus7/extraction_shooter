#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperShootingPracticeDummyActor.generated.h"

class USceneComponent;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UWidgetComponent;
class ATunaSweeperProjectile;

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
		float InHealthRecoverySeconds);

	bool IsHeadshotHit(const UPrimitiveComponent* HitComponent, const ATunaSweeperProjectile* Projectile) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Practice Dummy")
	float GetHealthFraction() const;

	// Zero removes the slot. Both slots follow the same whole-character defense rule as actual equipment.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Practice Dummy|Armor", meta = (ClampMin = "0", ClampMax = "4", UIMin = "0", UIMax = "4"))
	int32 BodyArmorTier = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Practice Dummy|Armor", meta = (ClampMin = "0", ClampMax = "4", UIMin = "0", UIMax = "4"))
	int32 HeadArmorTier = 0;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Practice Dummy|Armor")
	void ConfigurePracticeDummyArmor(int32 InBodyArmorTier, int32 InHeadArmorTier);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Practice Dummy|Armor")
	int32 GetBodyArmorItemId() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Practice Dummy|Armor")
	int32 GetHeadArmorItemId() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Practice Dummy|Armor")
	float GetEffectiveDefense(int32 PenetrationTier = 0) const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Practice Dummy", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float HealthRecoverySeconds = 2.0f;

private:
	void ConfigureHitComponent(UStaticMeshComponent* MeshComponent) const;
	void ApplyHitZoneColors();
	float ResolveDamageMultiplier(FDamageEvent const& DamageEvent, AActor* DamageCauser) const;
	bool IsHeadshotComponent(const UPrimitiveComponent* Component) const;
	void ApplyDummyDamage(float DamageAmount);
	void RefreshHealthBar();

	float CurrentHealth = 100.0f;
};
