#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TunaSweeperTitleAnimInstance.generated.h"

class UAnimSequence;

UENUM(BlueprintType)
enum class ETunaSweeperTitleMotion : uint8
{
	Entrance, IdleA, AtoB, IdleB, BtoA
};

/** Owns title playback timing. The dedicated AnimBP evaluates the selected pose before gaze/physics. */
UCLASS(Transient, Blueprintable)
class TUNASWEEPER_API UTunaSweeperTitleAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UTunaSweeperTitleAnimInstance();
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category="TunaSweeper|Title|Animation")
	void RestartTitleAnimation(int32 RandomSeed);

	UPROPERTY(Transient, BlueprintReadOnly, Category="TunaSweeper|Title|Animation")
	TObjectPtr<UAnimSequence> CurrentSequence;
	UPROPERTY(Transient, BlueprintReadOnly, Category="TunaSweeper|Title|Animation")
	float CurrentSequenceTime = 0.f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="TunaSweeper|Title|Animation")
	float HeadLookAlpha = 0.f;
	UPROPERTY(Transient, BlueprintReadOnly, Category="TunaSweeper|Title|Animation")
	ETunaSweeperTitleMotion CurrentMotion = ETunaSweeperTitleMotion::Entrance;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TunaSweeper|Title|Animation")
	TObjectPtr<UAnimSequence> Entrance;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TunaSweeper|Title|Animation")
	TObjectPtr<UAnimSequence> IdleA;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TunaSweeper|Title|Animation")
	TObjectPtr<UAnimSequence> IdleB;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TunaSweeper|Title|Animation")
	TObjectPtr<UAnimSequence> AtoB;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TunaSweeper|Title|Animation")
	TObjectPtr<UAnimSequence> BtoA;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TunaSweeper|Title|Animation", meta=(ClampMin="1", ClampMax="20"))
	int32 MinimumIdleLoops = 2;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TunaSweeper|Title|Animation", meta=(ClampMin="1", ClampMax="20"))
	int32 MaximumIdleLoops = 4;
private:
	void EnterMotion(ETunaSweeperTitleMotion Motion);
	FRandomStream Random;
	int32 IdleLoopsRemaining = 0;
	double SequenceTime = 0.;
};
