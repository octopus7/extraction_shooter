#include "Impact/TunaSweeperImpactEffectProfile.h"

#include "NiagaraSystem.h"

UTunaSweeperImpactEffectProfile::UTunaSweeperImpactEffectProfile()
{
	DefaultEffect.NiagaraSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
		TEXT("/Game/BallisticsVFX/Particles/Impacts/DynamicImpacts/_generic/NS_Generic_Impact.NS_Generic_Impact")));

	FTunaSweeperImpactSurfaceRule ConcreteRule;
	ConcreteRule.SurfaceType = SurfaceType3;
	ConcreteRule.Effect = DefaultEffect;
	ConcreteRule.Effect.NiagaraSystem = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
		TEXT("/Game/BallisticsVFX/Particles/Impacts/DynamicImpacts/Concrete/NS_Concrete_impact_2_Dyn.NS_Concrete_impact_2_Dyn")));
	SurfaceRules.Add(ConcreteRule);
}
