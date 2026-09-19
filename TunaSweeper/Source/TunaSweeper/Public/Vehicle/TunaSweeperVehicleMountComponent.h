#pragma once
#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TunaSweeperVehicleMountComponent.generated.h"
class ATunaSweeperTopDownCharacter;
class UAudioComponent;
class USoundBase;
class UTunaSweeperVehicleDismountWidget;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunaSweeperRiderChanged, ATunaSweeperTopDownCharacter*, Character);

/** Attach to a vehicle's seat socket. Possession stays on the player. */
UCLASS(ClassGroup=(TunaSweeper), meta=(BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperVehicleMountComponent : public UTunaSweeperInteractableComponent
{
	GENERATED_BODY()
public:
	UTunaSweeperVehicleMountComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UFUNCTION(BlueprintPure, Category="TunaSweeper|Vehicle")
	bool CanMount(const ATunaSweeperTopDownCharacter* Character) const;
	UFUNCTION(BlueprintCallable, Category="TunaSweeper|Vehicle")
	bool TryMount(ATunaSweeperTopDownCharacter* Character);
	UFUNCTION(BlueprintCallable, Category="TunaSweeper|Vehicle")
	bool TryDismount();
	UFUNCTION(BlueprintPure, Category="TunaSweeper|Vehicle")
	ATunaSweeperTopDownCharacter* GetRider() const { return Rider.Get(); }
	UFUNCTION(BlueprintPure, Category="TunaSweeper|Vehicle")
	bool IsDismountHintVisible() const { return bHintVisible; }
	bool FindDismountLocation(FVector& OutLocation) const;
	void ReleaseRiderForEndPlay();
	void UpdateStationaryHint(float DeltaTime, float Speed);
	void SetDriveInput(const FVector2D& Input);
	void SetBoostInput(bool bHeld);
	void ClearDriveInput();
	void UpdateDrivingAudio(float DeltaTime, float Speed, float RPMRatio, bool bBoosting);
	UPROPERTY(BlueprintAssignable, Category="TunaSweeper|Vehicle")
	FTunaSweeperRiderChanged OnMounted;
	UPROPERTY(BlueprintAssignable, Category="TunaSweeper|Vehicle")
	FTunaSweeperRiderChanged OnDismounted;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TunaSweeper|Vehicle")
	FTransform RiderRelativeTransform = FTransform::Identity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TunaSweeper|Vehicle", meta=(ClampMin="0.0"))
	float DismountHintDelay = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TunaSweeper|Vehicle", meta=(ClampMin="0.0"))
	float StationarySpeedThreshold = 5.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TunaSweeper|Vehicle", meta=(ClampMin="50.0"))
	float DismountDistance = 150.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TunaSweeper|Vehicle|Audio")
	TObjectPtr<USoundBase> EngineStartSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TunaSweeper|Vehicle|Audio")
	TObjectPtr<USoundBase> EngineIdleSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TunaSweeper|Vehicle|Audio")
	TObjectPtr<USoundBase> EngineStopSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TunaSweeper|Vehicle|Audio")
	TObjectPtr<USoundBase> EngineDriveSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="TunaSweeper|Vehicle|Audio")
	TObjectPtr<USoundBase> EngineBoostSound;
private:
	void ReleaseRider(const FVector& Location, bool bPlaySound);
	void StopEngineAudio();
	void StartIdleAudio();
	UPROPERTY(Transient)
	TWeakObjectPtr<ATunaSweeperTopDownCharacter> Rider;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> EngineAudio;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> DriveAudio;
	UPROPERTY(Transient)
	TObjectPtr<UAudioComponent> BoostAudio;
	float DriveAudioBlend = 0;
	float BoostAudioBlend = 0;
	bool bEngineRunning = false;
	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperVehicleDismountWidget> DismountWidget;
	FTimerHandle EngineStartTimer;
	FTransform LastOnFootTransform;
	FVector PreviousVehicleLocation = FVector::ZeroVector;
	TEnumAsByte<EMovementMode> SavedMovementMode = MOVE_Walking;
	uint8 SavedCustomMovementMode = 0;
	TEnumAsByte<ECollisionEnabled::Type> SavedCapsuleCollision = ECollisionEnabled::QueryAndPhysics;
	float StationarySeconds = 0.0f;
	bool bHintVisible = false;
};
