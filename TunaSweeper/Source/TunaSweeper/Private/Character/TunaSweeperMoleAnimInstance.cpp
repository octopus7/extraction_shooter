#include "Character/TunaSweeperMoleAnimInstance.h"
#include "Character/TunaSweeperMoleCompanionActor.h"
#include "Animation/AnimSequence.h"
#include "Engine/World.h"

void UTunaSweeperMoleAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	ActiveIdleMontage = nullptr;
	IdleDelayRemaining = -1.0f;
	LastIdleVariationIndex = INDEX_NONE;
}

bool UTunaSweeperMoleAnimInstance::PlayIdleVariation(int32 Index)
{
	if (!IdleVariations.IsValidIndex(Index) || !IdleVariations[Index]
		|| (ActiveIdleMontage && Montage_IsActive(ActiveIdleMontage))) return false;
	ActiveIdleMontage = PlaySlotAnimationAsDynamicMontage(
		IdleVariations[Index], UpperBodySlotName(), 0.3f, 0.4f, 1.0f, 1);
	if (!ActiveIdleMontage) return false;
	LastIdleVariationIndex = Index;
	IdleDelayRemaining = -1.0f;
	return true;
}

void UTunaSweeperMoleAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const auto* Mole = Cast<ATunaSweeperMoleCompanionActor>(GetOwningActor());
	if (!Mole || !GetWorld() || !GetWorld()->IsGameWorld() || DeltaSeconds <= 0.0f) return;
	if (ActiveIdleMontage)
	{
		if (Montage_IsActive(ActiveIdleMontage)) return;
		ActiveIdleMontage = nullptr;
		IdleDelayRemaining = -1.0f;
	}
	if (!Mole->bEnableIdleVariations || IdleVariations.IsEmpty()) return;
	if (IdleDelayRemaining < 0.0f)
	{
		const FVector2D Range = Mole->GetIdleVariationDelayRange();
		IdleDelayRemaining = FMath::FRandRange(Range.X, Range.Y);
		return;
	}
	IdleDelayRemaining -= DeltaSeconds;
	if (IdleDelayRemaining <= 0.0f)
	{
		PlayIdleVariation(FMath::RandHelper(IdleVariations.Num()));
		// A missing asset must not cause a retry on every frame.
		IdleDelayRemaining = -1.0f;
	}
}
