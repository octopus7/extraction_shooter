#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/Hopper/HopperGait.h"
#include "HopperPresentationComponent.generated.h"
class UHopperVisualData;
class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAnimSequence;

/** Presentation only: authoritative health/phase/attacks remain on the enemy. */
UCLASS(ClassGroup=(Hopper), meta=(BlueprintSpawnableComponent))
class TUNASWEEPER_API UHopperPresentationComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UHopperPresentationComponent();
 void Initialize(UHopperVisualData* Data);
 USceneComponent* GetArmSocket(bool bLeft) const;
 void StartBoarding(float Duration);
 void SeatPilot();
 void StartDisembark(const FVector& GroundDestination, float Duration);
 void FinishDisembark();
 void PauseTransfer() { bTransferring=false; }
 void SetPilotLocomotion(bool bWalking);
 void PlayPilotFire();
 void PlayPilotMelee();
 FVector GetPilotMuzzleLocation() const;
 void SetGunVisible(bool bVisible);
 void SetMechActive(bool bActive);
 float GetMaxStableWalkSpeed() const;
 virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
 UFUNCTION(BlueprintPure,Category="Hopper") USkeletalMeshComponent* GetPilotMesh() const { return Pilot; }
 UFUNCTION(BlueprintPure,Category="Hopper") USceneComponent* GetBodyRoot() const { return BodyRoot; }
 UPROPERTY(EditAnywhere,Category="Hopper|Gait",meta=(ClampMin="0.1")) float StepDuration = 0.42f;
 UPROPERTY(EditAnywhere,Category="Hopper|Gait",meta=(ClampMin="0")) float StepHeight = 16.f;
 UPROPERTY(EditAnywhere,Category="Hopper|Gait",meta=(ClampMin="1")) float StrideTrigger = 25.f;
 UPROPERTY(EditAnywhere,Category="Hopper|Gait") TObjectPtr<class USoundBase> FootfallSound;
 UPROPERTY(EditAnywhere,Category="Hopper|Gait") float FootfallVolume = .7f;
protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 void TickMech(float Dt);
 void TickTransfer(float Dt);
 void ApplyLeg(int32 Side,const FTransform& BodyWorld);
 void PlayClip(UAnimSequence* Clip,bool bLoop,bool bOverrideAction=false);
 void SetHatchOpen(float Alpha);
 float CapsuleHalfHeight() const;
 UPROPERTY(Transient) TObjectPtr<UHopperVisualData> VisualData;
 UPROPERTY(Transient) TObjectPtr<USceneComponent> BodyRoot;
 UPROPERTY(Transient) TObjectPtr<USceneComponent> Seat;
 UPROPERTY(Transient) TObjectPtr<USceneComponent> LeftSocket;
 UPROPERTY(Transient) TObjectPtr<USceneComponent> RightSocket;
 UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Pilot;
 UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Gun;
 UPROPERTY(Transient) TMap<FName,TObjectPtr<UStaticMeshComponent>> Parts;
 UPROPERTY(Transient) TObjectPtr<UAnimSequence> CurrentClip;
 TunaSweeperHopper::FBipedGait Gait;
 FVector TransferStart = FVector::ZeroVector;
 FVector TransferEnd = FVector::ZeroVector;
 float BodyCrouch = 0;
 float TransferTime = 0, TransferDuration = 1, ActionRemaining = 0, HatchOpen = 0;
 bool bTransferring = false, bBoarding = false, bSeated = true, bMechActive = false, bWreck = false;
};
