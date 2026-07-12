#pragma once

#include "CoreMinimal.h"
#include "Impact/TunaSweeperImpactEffectTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperImpactEffectSubsystem.generated.h"

class UTunaSweeperImpactEffectProfile;

UCLASS()
class TUNASWEEPER_API UTunaSweeperImpactEffectSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	bool ResolveAndSpawnImpactEffect(
		FName ImpactProfileId,
		FName LegacyEffectId,
		const FHitResult& Hit,
		AActor* EffectOwner,
		APawn* EffectInstigator) const;

	bool ResolveImpactEffect(
		const UTunaSweeperImpactEffectProfile& Profile,
		const FTunaSweeperImpactResolveContext& Context,
		FTunaSweeperImpactEffectSpec& OutEffect) const;

private:
	const UTunaSweeperImpactEffectProfile* FindProfile(FName ImpactProfileId) const;
	void SpawnResolvedEffect(
		const FTunaSweeperImpactEffectSpec& Effect,
		const FHitResult& Hit,
		const FTunaSweeperImpactResolveContext& Context) const;
	bool SpawnLegacyEffect(FName LegacyEffectId, const FHitResult& Hit, AActor* EffectOwner, APawn* EffectInstigator) const;
};
