#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperTopAttackRocket.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UDecalComponent;
class UStaticMesh;
class UMaterialInterface;

UENUM(BlueprintType)
enum class ETopAttackRocketPhase : uint8 { Ascent, Guidance, Dive, Detonated };

/** Single-use item 6019 projectile. Presentation assets and flight tuning live in its BP. */
UCLASS(Blueprintable)
class TUNASWEEPER_API ATunaSweeperTopAttackRocket : public AActor
{
	GENERATED_BODY()
public:
	ATunaSweeperTopAttackRocket();
	static constexpr int32 ItemId = 6019;
	static bool IsValidTarget(const AActor* Source, const AActor* Candidate);
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	UFUNCTION(BlueprintPure, Category="Rocket") ETopAttackRocketPhase GetFlightPhase() const { return Phase; }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rocket", meta=(ExposeOnSpawn=true)) TObjectPtr<AActor> TargetActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rocket", meta=(ExposeOnSpawn=true)) FVector TargetLocation = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Rocket", meta=(ExposeOnSpawn=true)) bool bTargetLocked = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rocket") TObjectPtr<USphereComponent> Collision;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rocket") TObjectPtr<UStaticMeshComponent> RocketMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rocket") TObjectPtr<UStaticMeshComponent> Exhaust;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rocket") TObjectPtr<UDecalComponent> GuidanceDecal;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Rocket") TObjectPtr<UStaticMeshComponent> Blast;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<UStaticMesh> BodyMesh;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<UStaticMesh> ExhaustShape;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<UStaticMesh> BlastShape;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<UMaterialInterface> ExhaustMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") TObjectPtr<UMaterialInterface> GuidanceMaterial;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Presentation") FVector ExhaustOffset = FVector(-40, 0, 0);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Flight", meta=(ClampMin="0.05")) float AscentSeconds = 0.8f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Flight", meta=(ClampMin="100")) float AscentHeight = 1800.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Flight", meta=(ClampMin="0.05")) float GuidanceSeconds = 0.8f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Flight", meta=(ClampMin="0.05")) float DiveSeconds = 0.45f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Flight", meta=(ClampMin="0")) float SearchRadius = 3500.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0")) float Damage = 200.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage", meta=(ClampMin="1")) float DamageRadius = 300.f;
protected:
	virtual void BeginPlay() override;
private:
	void UpdateTarget();
	void UpdateGuidanceDecal();
	void Detonate();
	FVector GroundBelow(const FVector& Point) const;
	TWeakObjectPtr<AActor> ActiveTarget;
	FVector LaunchLocation;
	FVector AscentEnd;
	FVector DiveStart;
	FVector ImpactLocation;
	float Age = 0.f;
	float BlastAge = 0.f;
	ETopAttackRocketPhase Phase = ETopAttackRocketPhase::Ascent;
};
