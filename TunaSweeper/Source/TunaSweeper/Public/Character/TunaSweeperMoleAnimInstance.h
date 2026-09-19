#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TunaSweeperMoleAnimInstance.generated.h"

class UAnimSequence;

/** The AnimBP caches breathing/turning once, then masks the variation slot above spine. */
UCLASS(Transient, Blueprintable)
class TUNASWEEPER_API UTunaSweeperMoleAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "Mole|Animation")
	float TurnAmount = 0.0f;
	UPROPERTY(BlueprintReadOnly, Transient, Category = "Mole|Animation")
	float TurnPlayRate = 1.0f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mole|Idle")
	TArray<TObjectPtr<UAnimSequence>> IdleVariations;

	/** Also used to preview an individual gesture without changing the base footwork. */
	UFUNCTION(BlueprintCallable, Category = "Mole|Idle")
	bool PlayIdleVariation(int32 Index);

	static FName UpperBodySlotName() { return TEXT("MoleUpperBody"); }
	float GetIdleDelayRemaining() const { return IdleDelayRemaining; }
	int32 GetLastIdleVariationIndex() const { return LastIdleVariationIndex; }

private:
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> ActiveIdleMontage;
	float IdleDelayRemaining = -1.0f;
	int32 LastIdleVariationIndex = INDEX_NONE;
};
