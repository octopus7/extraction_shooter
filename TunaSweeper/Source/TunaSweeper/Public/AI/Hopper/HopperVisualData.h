#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HopperVisualData.generated.h"
class UStaticMesh;
class USkeletalMesh;
class UAnimSequence;
class UNiagaraSystem;
class UMaterialInterface;
USTRUCT(BlueprintType)
struct FHopperMeshPart
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere) FName PartName;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> Mesh;
};
USTRUCT(BlueprintType)
struct FHopperPartMotion
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere) FName PartName;
 UPROPERTY(EditAnywhere) TArray<FTransform> Frames;
};
USTRUCT(BlueprintType)
struct FHopperEjectionPull
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotAnimation;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> HandleMesh;
 /** Seat-local grip poses sampled uniformly across PilotAnimation, starting at the ready position. */
 UPROPERTY(EditAnywhere) TArray<FTransform> HandlePoses;
 /** Normalized time when the pilot finishes the downstroke, shatters the failing shield and fires the seat. */
 UPROPERTY(EditAnywhere, meta=(ClampMin="0", ClampMax="1")) float TripPhase = .78f;
};
USTRUCT(BlueprintType)
struct FHopperWreckEffects
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere, meta=(Units="cm")) FVector Origin = FVector(22,0,107);
 /** A short burst at the moment the shield breaks. */
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UNiagaraSystem> RadialBlast;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UMaterialInterface> BlastFlashMaterial;
 /** Sustained flames on the empty chassis after seat separation. */
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UNiagaraSystem> Fire;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UNiagaraSystem> MetalDebris;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UNiagaraSystem> OilSpray;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UMaterialInterface> Scorch;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UMaterialInterface> OilPool;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UMaterialInterface> OilSplash;
 bool IsConfigured() const { return !RadialBlast.IsNull() || !Fire.IsNull(); }
};
/** Rest geometry is centimeters, +X forward, +Z up, at ground origin. */
UCLASS(BlueprintType)
class TUNASWEEPER_API UHopperVisualData : public UDataAsset
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere) TArray<FHopperMeshPart> Parts;
 UPROPERTY(EditAnywhere) TArray<FHopperPartMotion> HatchMotion;
 /** Wait for player sight and frame boarding and seat ejection with the encounter camera. */
 UPROPERTY(EditAnywhere, Category="Encounter") bool bUseEncounterCamera = false;
 /** Time for a full hatch cycle in one direction. Zero starts transfers with the hatch already open. */
 UPROPERTY(EditAnywhere, Category="Hatch", meta=(ClampMin="0", Units="s")) float HatchTravelSeconds = 0.f;
 UPROPERTY(EditAnywhere, Category="Hatch", meta=(ClampMin="0", Units="cm")) float BoardingStartDistance = 85.f;
 /** Optional visual field fitted to the closed frame, with field and spark material slots. */
 UPROPERTY(EditAnywhere, Category="Hatch") TSoftObjectPtr<UStaticMesh> CockpitShieldMesh;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<USkeletalMesh> PilotMesh;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> PilotGun;
 UPROPERTY(EditAnywhere) FTransform GunTransform;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotBoarding;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotSeated;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotDisembark;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotIdle;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotWalk;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotFire;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotMelee;
 UPROPERTY(EditAnywhere) FVector SeatLocation = FVector(17,0,119.7806);
 /** Distance from the locked seat to the outer end of the boarding rail. */
 UPROPERTY(EditAnywhere, Category="Seat", meta=(ClampMin="0", Units="cm")) float SeatBoardingReach = 0.f;
 /** Vertical stroke between rail height and the seat lock. */
 UPROPERTY(EditAnywhere, Category="Seat", meta=(ClampMin="0", Units="cm")) float SeatLockDrop = 0.f;
 /** Center of the positive-Y nozzle mouth in seat space. The second nozzle mirrors it across Y. */
 UPROPERTY(EditAnywhere, Category="Seat", meta=(Units="cm")) FVector SeatExhaustNozzle = FVector(0,17,-18);
 UPROPERTY(EditAnywhere, Category="Seat") TSoftObjectPtr<UNiagaraSystem> SeatExhaustSystem;
 /** In-place climb from ground to the two open treads and the lowered seat. */
 UPROPERTY(EditAnywhere, Category="Boarding") TSoftObjectPtr<UAnimSequence> PilotLadderBoarding;
 /** Pilot root positions when both boots contact each tread, in fully open mech space. */
 UPROPERTY(EditAnywhere, Category="Boarding", meta=(Units="cm")) FVector LowerTreadLanding = FVector::ZeroVector;
 UPROPERTY(EditAnywhere, Category="Boarding", meta=(Units="cm")) FVector MiddleTreadLanding = FVector::ZeroVector;
 UPROPERTY(EditAnywhere, Category="Boarding", meta=(ClampMin="0", Units="cm")) float SeatPickupLift = 0.f;
 /** Positive-Y contact points in chassis space. The other rail mirrors Y. */
 UPROPERTY(EditAnywhere, Category="Seat") FVector RailLockContact = FVector::ZeroVector;
 UPROPERTY(EditAnywhere, Category="Seat") FVector RailExitContact = FVector::ZeroVector;
 UPROPERTY(EditAnywhere, Category="Seat", meta=(ClampMin="0", Units="cm")) float RailReleaseTravel = 0.f;
 UPROPERTY(EditAnywhere, Category="Seat") TSoftObjectPtr<UNiagaraSystem> SeatLockSparks;
 UPROPERTY(EditAnywhere, Category="Seat") TSoftObjectPtr<UNiagaraSystem> SeatEjectSparks;
 UPROPERTY(EditAnywhere, Category="Seat") FHopperEjectionPull EjectionPull;
 /** Rigid guard parts released together for vertical ejection. The first part carries the others. */
 UPROPERTY(EditAnywhere, Category="Destruction") TArray<FName> EjectionGuardParts;
 UPROPERTY(EditAnywhere, Category="Destruction") FHopperWreckEffects WreckEffects;
 UPROPERTY(EditAnywhere) FVector LeftArmMount = FVector(-5.8716,56.7287,157.1782);
 UPROPERTY(EditAnywhere) FVector RightArmMount = FVector(-5.8716,-56.7287,157.1782);
 UPROPERTY(EditAnywhere) FVector LeftHip = FVector(-4.5166,39.0687,85.8);
 UPROPERTY(EditAnywhere) FVector LeftKnee = FVector(-5.307,44.658,53.7965);
 UPROPERTY(EditAnywhere) FVector LeftAnkle = FVector(-.2032,44.2628,18.089);
 UPROPERTY(EditAnywhere) FVector RightHip = FVector(-4.5166,-39.0687,85.8);
 UPROPERTY(EditAnywhere) FVector RightKnee = FVector(-5.307,-44.658,53.7965);
 UPROPERTY(EditAnywhere) FVector RightAnkle = FVector(-.2032,-44.2628,18.089);
};
