#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/Hopper/HopperGait.h"
#include "HopperPresentationComponent.generated.h"
class UBoxComponent;
class UHopperVisualData;
class USceneComponent;
class UStaticMeshComponent;
class USkeletalMeshComponent;
class UAnimSequence;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UProceduralMeshComponent;

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
 UBoxComponent* CreateMechHitBox(UStaticMeshComponent* Part);
 void DisableMechHitBoxes();
 void StartSeatEjection();
 float GetHatchOpenAlpha() const { return HatchOpen; }
 float GetMaxStableWalkSpeed() const;
 virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
 UFUNCTION(BlueprintPure,Category="Hopper") USkeletalMeshComponent* GetPilotMesh() const { return Pilot; }
 UFUNCTION(BlueprintPure,Category="Hopper") USceneComponent* GetBodyRoot() const { return BodyRoot; }
 UPROPERTY(EditAnywhere,Category="Hopper|Gait",meta=(ClampMin="0.1")) float StepDuration = 0.42f;
 UPROPERTY(EditAnywhere,Category="Hopper|Gait",meta=(ClampMin="0")) float StepHeight = 16.f;
 UPROPERTY(EditAnywhere,Category="Hopper|Gait",meta=(ClampMin="1")) float StrideTrigger = 25.f;
 UPROPERTY(EditAnywhere,Category="Hopper|Gait") TObjectPtr<class USoundBase> FootfallSound;
 UPROPERTY(EditAnywhere,Category="Hopper|Gait") float FootfallVolume = .7f;
 UPROPERTY(EditAnywhere,Category="Hopper|Ejection",meta=(ClampMin="0.05",Units="s")) float EjectionHatchSeconds = .65f;
 UPROPERTY(EditAnywhere,Category="Hopper|Ejection",meta=(ClampMin="0.05",Units="s")) float EjectionSlideSeconds = .55f;
 UPROPERTY(EditAnywhere,Category="Hopper|Ejection",meta=(ClampMin="0",Units="cm")) float EjectionSlideDistance = 90.f;
 UPROPERTY(EditAnywhere,Category="Hopper|Ejection",meta=(ClampMin="0.1",Units="s")) float EjectionFlightSeconds = 3.f;
 UPROPERTY(EditAnywhere,Category="Hopper|Ejection") float EjectionForwardSpeed = 160.f;
 UPROPERTY(EditAnywhere,Category="Hopper|Ejection") float EjectionUpSpeed = 420.f;
 UPROPERTY(EditAnywhere,Category="Hopper|Ejection") float EjectionUpAcceleration = 900.f;
 UPROPERTY(EditAnywhere,Category="Hopper|Ejection") TSoftObjectPtr<UMaterialInterface> EjectionSmokeMaterial;
 UPROPERTY(EditAnywhere,Category="Hopper|Ejection") TSoftObjectPtr<UMaterialInterface> EjectionFlameMaterial;
protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
 void TickMech(float Dt);
 void TickTransfer(float Dt);
 void ApplyLeg(int32 Side,const FTransform& BodyWorld);
 void PlayClip(UAnimSequence* Clip,bool bLoop,bool bOverrideAction=false);
 void SetHatchOpen(float Alpha);
 void TickSeatEjection(float Dt);
 void CreateEjectionExhaust();
 void ClearEjectionExhaust();
 FVector EjectionPosition(float FlightTime) const;
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
 UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> EjectionFlame;
 UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> EjectionSmoke;
 UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> EjectionSmokeMIDs;
 TArray<float> SmokeBirthTimes;
 TArray<FVector> SmokeOrigins;
 FTransform EjectionLaunchTransform;
 FVector EjectionSeatStart = FVector::ZeroVector;
 float EjectionTime = -1.f, EjectionInitialHatch = 0.f, NextSmokeTime = 0.f;
 int32 NextSmokeIndex = 0;
 bool bSeatLaunched = false;
 TunaSweeperHopper::FBipedGait Gait;
 FVector TransferStart = FVector::ZeroVector;
 FVector TransferEnd = FVector::ZeroVector;
 float BodyCrouch = 0;
 float TransferTime = 0, TransferDuration = 1, ActionRemaining = 0, HatchOpen = 0;
 bool bTransferring = false, bBoarding = false, bSeated = true, bMechActive = false, bWreck = false;
};
