#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TunaSweeperATVActor.generated.h"
class UBoxComponent;
class USkeletalMeshComponent;
class UTunaSweeperVehicleMountComponent;
class UChaosWheeledVehicleMovementComponent;
class ATunaSweeperTopDownCharacter;
class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMesh;
class USoundBase;

UENUM(BlueprintType)
enum class ETunaSweeperATVDamageState : uint8 { Healthy, Damaged, Critical, Destroyed };

/** Four-wheel Chaos ATV controlled by its rider without changing player possession. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperATVActor : public APawn
{
	GENERATED_BODY()
public:
	ATunaSweeperATVActor();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;
	UFUNCTION(BlueprintPure, Category="ATV|Durability")
	ETunaSweeperATVDamageState GetDamageState() const;
	UFUNCTION(BlueprintPure, Category="ATV|Durability")
	float GetDurabilityRatio() const;
	UFUNCTION(BlueprintPure, Category="ATV|Durability")
	bool IsVehicleDestroyed() const { return bVehicleDestroyed; }
	/** Requested emission; previously emitted particles may still be draining. */
	bool IsDamageSmokeEmitting(bool bHeavy) const { return bHeavy ? bHeavySmokeEmitting : bLightSmokeEmitting; }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Durability", meta=(ClampMin="1"))
	float MaxDurability = 300.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="ATV|Durability")
	float CurrentDurability = 300.0f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="ATV|Durability")
	bool bVehicleDestroyed = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Durability", meta=(ClampMin="0", ClampMax="1"))
	float SmokeDurabilityRatio = 0.65f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Durability", meta=(ClampMin="0", ClampMax="1"))
	float HeavySmokeDurabilityRatio = 0.30f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Effects", meta=(ClampMin="0"))
	float HitSmokeDuration = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Effects", meta=(ClampMin="0"))
	float WreckSmokeDuration = 20.0f;
	/** Seconds after destruction; zero plays the visual immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Effects", meta=(ClampMin="0", Units="s"))
	float DestructionExplosionDelay = 3.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Effects")
	TSoftObjectPtr<USoundBase> DestructionExplosionSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Effects")
	TSoftObjectPtr<UNiagaraSystem> DestructionExplosionSystem;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="ATV|Effects")
	bool bDestructionExplosionTriggered = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV|Effects")
	TObjectPtr<UNiagaraComponent> DestructionExplosion;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Effects", meta=(ClampMin="1"))
	float DebrisLifetime = 30.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV|Effects")
	TObjectPtr<UNiagaraComponent> LightDamageSmoke;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV|Effects")
	TObjectPtr<UNiagaraComponent> HeavyDamageSmoke;
	/** Meshes are authored around wheel_FL, wheel_RR and handlebar respectively. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ATV|Effects")
	TArray<TObjectPtr<UStaticMesh>> DetachedPartMeshes;
	void SetDriveInput(const FVector2D& Input);
	void SetBoostInput(bool bHeld);
	void ClearDriveInput();
	bool CanAcceptDriveInput() const;
	FVector2D GetDriveInput() const { return DriveInput; }
	bool IsBoosting() const { return bBoostHeld && DriveInput.Y > 0.0f; }
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV")
	TObjectPtr<UChaosWheeledVehicleMovementComponent> VehicleMovement;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Driving", meta=(ClampMin="100"))
	float NormalTopSpeed = 1000.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Driving", meta=(ClampMin="100"))
	float BoostTopSpeed = 1600.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Driving", meta=(ClampMin="100"))
	float ReverseTopSpeed = 400.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Driving", meta=(ClampMin="1"))
	float NormalEngineTorque = 38.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ATV|Driving", meta=(ClampMin="1"))
	float BoostEngineTorque = 58.0f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV")
	TObjectPtr<UBoxComponent> ChassisCollision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV")
	TObjectPtr<USkeletalMeshComponent> VehicleMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ATV")
	TObjectPtr<UTunaSweeperVehicleMountComponent> MountComponent;
private:
	void CreateDamageComponents();
	void UpdateDamageSmoke(float DeltaSeconds);
	void DestroyVehicle();
	void PlayDestructionExplosion();
	FTimerHandle DestructionExplosionTimer;
	TArray<TWeakObjectPtr<AActor>> DetachedParts;
	float HitSmokeRemaining = 0;
	float WreckSmokeElapsed = 0;
	bool bProcessingDamage = false;
	bool bLightSmokeEmitting = false;
	bool bHeavySmokeEmitting = false;
	void ConfigureContactCollision();
	UFUNCTION()
	void HandleRiderChanged(ATunaSweeperTopDownCharacter* Rider);
	FVector2D DriveInput = FVector2D::ZeroVector;
	bool bBoostHeld = false;
};
