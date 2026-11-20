#pragma once

#include "AI/TunaSweeperQuadrupedEnemyCharacter.h"
#include "ForestQuadbotEnemy.generated.h"

class UMaterialInterface;
class USkeletalMesh;
class UStaticMesh;

USTRUCT(BlueprintType)
struct FForestQuadbotPanel
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category="Armor")
	TObjectPtr<UStaticMesh> Shard = nullptr;

	UPROPERTY(EditDefaultsOnly, Category="Armor")
	FName Bone;

	/** Pivot of the shard in the skeletal mesh's reference pose, in centimetres. */
	UPROPERTY(EditDefaultsOnly, Category="Armor")
	FVector ReferenceCenter = FVector::ZeroVector;
};

UCLASS(Blueprintable)
class FORESTQUADBOT_API AForestQuadbotEnemy : public ATunaSweeperQuadrupedEnemyCharacter
{
	GENERATED_BODY()

public:
	AForestQuadbotEnemy();
	virtual float TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent, AController* InstigatorController, AActor* Causer) override;

	UFUNCTION(BlueprintCallable, Category="Quadbot|Appearance")
	void SetScorchAmount(float Amount);

	UFUNCTION(BlueprintPure, Category="Quadbot|Appearance")
	float GetScorchAmount() const { return ScorchAmount; }

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void OnDeathPresentationStarted() override;

	UPROPERTY(EditDefaultsOnly, Category="Quadbot|Appearance")
	TObjectPtr<USkeletalMesh> RobotModel;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Quadbot|Appearance")
	TObjectPtr<UMaterialInterface> ShellMaterial;

	/** UV channel 1 stores a one-based panel index; the core uses a negative index. */
	UPROPERTY(EditDefaultsOnly, Category="Quadbot|Destruction")
	TArray<FForestQuadbotPanel> BreakawayPanels;

private:
	void ApplyAppearance();
	bool EjectPanel(const FForestQuadbotPanel& Panel);
	float ScorchAmount = 0.0f;
};
