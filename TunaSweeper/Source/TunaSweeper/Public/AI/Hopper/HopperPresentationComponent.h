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
class UMaterialBillboardComponent;
class UProceduralMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UHopperOilImpactReceiver;
struct FStreamableHandle;

/** Presentation only: authoritative health/phase/attacks remain on the enemy. */
UCLASS(ClassGroup=(Hopper), meta=(BlueprintSpawnableComponent))
class TUNASWEEPER_API UHopperPresentationComponent : public UActorComponent
{
 GENERATED_BODY()
public:
 UHopperPresentationComponent();
 void Initialize(UHopperVisualData* Data);
 USceneComponent* GetArmSocket(bool bLeft) const;
 void WaitBesideMech();
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
 void StartSeatEjection(UNiagaraSystem* MechEffect=nullptr, const FVector& ImpactTravel=FVector::ZeroVector);
 float StartDestructionEffects(UNiagaraSystem* MechEffect, const FVector& ImpactTravel=FVector::ZeroVector);
 bool IsWaitingForShieldBreak() const;
 float GetHatchOpenAlpha() const { return HatchOpen; }
 float GetEjectionHatchDuration() const;
 float GetEjectionLaunchTime() const;
 float GetEjectionPullStartTime() const;
 float GetEjectionPullTriggerTime() const;
 float GetSeatDockDuration() const;
 float GetBoardingClimbEndTime(float ClimbSeconds) const;
 bool IsBoardingInProgress() const { return bBoarding && bTransferring; }
 float GetPilotTravelAlpha(float Elapsed) const;
 FVector GetTransferPosition(const FVector& Start, const FVector& End, float Alpha, bool bIsBoarding) const;
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
 enum class EShieldStage : uint8 { Off, Deploying, Stable, Failing, Fragments };
 void InitializeCockpitShield();
 void InitializeBlastFlash();
 void FadeBlastFlash(float DeltaSeconds);
 void TickCockpitShield(float Dt);
 float GetShieldWarningDuration() const;
 void EmitDestructionEffect(UNiagaraSystem* Effect);
 void EmitWreckEffects();
 void PreloadWreckEffects();
 void PrepareWreckMaterials();
 void ApplyHullScorch(const FTransform& Chassis);
 void UpdateWreckBurn(float FlightTime);
 void TickMech(float Dt);
 void TickTransfer(float Dt);
 void BeginTransfer(float Duration, bool bIsBoarding);
 void AttachPilotToSeat();
 void TickLadderBoarding();
 void EmitRailSparks(bool bEjection);
 FVector GetSeatDockOffset(float DockTime) const;
 static constexpr float RailTravelSeconds = .52f;
 static constexpr float RailStopSeconds = .10f;
 static constexpr float SeatLiftSeconds = .30f;
 static constexpr float PickupLiftSeconds = .48f;
 static constexpr float PickupStopSeconds = .10f;
 void ApplyLeg(int32 Side,const FTransform& BodyWorld);
 void PlayClip(UAnimSequence* Clip,bool bLoop,bool bOverrideAction=false);
 void SetHatchOpen(float Alpha);
 void InitializeEjectionHandle();
 void TickEjectionPull();
 void ReleaseEjectionGuard();
 void UpdateEjectionHatch();
 void TickSeatEjection(float LaunchTime);
 void CreateEjectionExhaust();
 void ClearEjectionExhaust();
 FVector EjectionPosition(float FlightTime) const;
 float CapsuleHalfHeight() const;
 UPROPERTY(Transient) TObjectPtr<UHopperVisualData> VisualData;
 UPROPERTY(Transient) TObjectPtr<USceneComponent> BodyRoot;
 UPROPERTY(Transient) TObjectPtr<USceneComponent> Seat;
 UPROPERTY(Transient) TObjectPtr<USceneComponent> LeftSocket;
 UPROPERTY(Transient) TObjectPtr<USceneComponent> RightSocket;
 UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> CockpitShield;
 UPROPERTY(Transient) TObjectPtr<UMaterialBillboardComponent> BlastFlash;
 UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> BlastCoreMaterial;
 float BlastFlashRemaining = 0.f;
 UPROPERTY(Transient) TObjectPtr<UNiagaraSystem> PendingDestructionEffect;
 UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> ScorchedHullMaterials;
 UPROPERTY(Transient) TArray<TObjectPtr<UHopperOilImpactReceiver>> OilReceivers;
 TSharedPtr<FStreamableHandle> WreckAssetHandle;
 FVector WreckSprayDirection = FVector::ZeroVector;
 bool bWreckBurstPlayed = false;
 bool bWreckFireStarted = false;
 bool bSeatFlightFinished = false;
 EShieldStage ShieldStage = EShieldStage::Off;
 float ShieldAge = 0, ShieldCoverage = 0, EjectionShieldDelay = 0;
 UPROPERTY(Transient) TObjectPtr<USkeletalMeshComponent> Pilot;
 UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> Gun;
 UPROPERTY(Transient) TMap<FName,TObjectPtr<UStaticMeshComponent>> Parts;
 UPROPERTY(Transient) TObjectPtr<UAnimSequence> CurrentClip;
 UPROPERTY(Transient) TObjectPtr<UProceduralMeshComponent> EjectionFlame;
 UPROPERTY(Transient) TArray<TObjectPtr<UNiagaraComponent>> SeatJets;
 UPROPERTY(Transient) TObjectPtr<UNiagaraSystem> LoadedSeatExhaust;
 UPROPERTY(Transient) TObjectPtr<UAnimSequence> LadderBoardingClip;
 UPROPERTY(Transient) TObjectPtr<UStaticMeshComponent> EjectionHandle;
 UPROPERTY(Transient) TObjectPtr<UAnimSequence> EjectionPullClip;
 UPROPERTY(Transient) TObjectPtr<UNiagaraSystem> LoadedLockSparks;
 UPROPERTY(Transient) TObjectPtr<UNiagaraSystem> LoadedEjectSparks;
 UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> EjectionSmoke;
 UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> EjectionSmokeMIDs;
 TArray<float> SmokeBirthTimes;
 TArray<FVector> SmokeOrigins;
 FTransform EjectionLaunchTransform;
 TWeakObjectPtr<AActor> ReleasedGuard;
 float GuardReleaseTime = -1.f;
 FVector EjectionSeatStart = FVector::ZeroVector;
 float EjectionTime = -1.f, EjectionInitialHatch = 0.f, NextSmokeTime = 0.f;
 int32 NextSmokeIndex = 0;
 bool bSeatLaunched = false;
 bool bEjectionPullFinished = false;
 bool bAwaitingEjectionPull = false;
 bool bRailReleaseFlashed = false;
 TunaSweeperHopper::FBipedGait Gait;
 FVector TransferStart = FVector::ZeroVector;
 FVector TransferEnd = FVector::ZeroVector;
 float BodyCrouch = 0;
 float TransferTime = 0, TransferDuration = 1, ActionRemaining = 0, HatchOpen = 0;
 float HatchLeadTime = 0, TransferInitialHatch = 0;
 bool bTransferring = false, bBoarding = false, bSeated = true, bMechActive = false, bWreck = false;
};
