#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "TunaSweeperATVActor.generated.h"
class UBoxComponent;
class USkeletalMeshComponent;
class UTunaSweeperVehicleMountComponent;
class UChaosWheeledVehicleMovementComponent;
class ATunaSweeperTopDownCharacter;

/** Four-wheel Chaos ATV controlled by its rider without changing player possession. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperATVActor : public APawn
{
	GENERATED_BODY()
public:
	ATunaSweeperATVActor();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual UPawnMovementComponent* GetMovementComponent() const override;
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
	UFUNCTION()
	void HandleRiderChanged(ATunaSweeperTopDownCharacter* Rider);
	FVector2D DriveInput = FVector2D::ZeroVector;
	bool bBoostHeld = false;
};
