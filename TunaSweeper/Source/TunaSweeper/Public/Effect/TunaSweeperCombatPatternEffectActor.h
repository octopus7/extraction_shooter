#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperCombatPatternEffectActor.generated.h"

class UMaterialInterface;
class UProceduralMeshComponent;
class UTunaSweeperVisionSubjectComponent;

UENUM(BlueprintType)
enum class ETunaSweeperCombatPatternEffect : uint8
{
	Summon,
	MissileLaunch,
	MissileTrail,
	Impact,
	ChargeTrail,
	RobotUnfold,
	RobotDeath
};

/** Small, reusable visual bursts. No collision, damage, lights, or persistent particle components. */
UCLASS(BlueprintType, Blueprintable, Transient, NotPlaceable)
class TUNASWEEPER_API ATunaSweeperCombatPatternEffectActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperCombatPatternEffectActor();
	virtual void Tick(float DeltaSeconds) override;

	/** Direction is in world space; Radius is a visual extent in centimetres, not a damage radius. */
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat Pattern|Effect")
	void InitializeEffect(ETunaSweeperCombatPatternEffect Kind, float Radius, FVector Direction);

	/** Rebuild a visual sample without changing runtime age, tick, or lifetime (for artist previews). */
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat Pattern|Effect")
	void PreviewAtNormalizedAge(float NormalizedAge);

	/** Returns null on dedicated servers. Spawn at ground level for rings and at the exhaust for trails. */
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Combat Pattern|Effect", meta = (WorldContext = "World"))
	static ATunaSweeperCombatPatternEffectActor* Spawn(UWorld* World, ETunaSweeperCombatPatternEffect Kind,
		const FVector& Location, float Radius, const FVector& Direction, AActor* EffectOwner = nullptr);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Combat Pattern|Effect")
	float GetEffectDuration() const { return Duration; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProceduralMeshComponent> EffectMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTunaSweeperVisionSubjectComponent> VisionSubjectComponent;

	// Hard references retain both vertex-colour shaders in packaged builds.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Combat Pattern|Effect")
	TObjectPtr<UMaterialInterface> GlowMaterial;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TunaSweeper|Combat Pattern|Effect")
	TObjectPtr<UMaterialInterface> SmokeMaterial;

private:
	struct FParticle
	{
		FVector Origin = FVector::ZeroVector;
		FVector Velocity = FVector::ZeroVector;
		float Size = 1.0f;
		float Phase = 0.0f;
	};

	void UpdateGeometry();
	void SeedParticles();

	ETunaSweeperCombatPatternEffect EffectKind = ETunaSweeperCombatPatternEffect::Impact;
	FVector EffectDirection = FVector::UpVector;
	float EffectRadius = 100.0f;
	float Elapsed = 0.0f;
	float Duration = 0.8f;
	bool bInitialized = false;
	TArray<FParticle> Sparks;
	TArray<FParticle> Smoke;
};
