#include "Subsystem/TunaSweeperImpactEffectSubsystem.h"

#include "Engine/World.h"
#include "Effect/TunaSweeperProjectileHitBurstActor.h"
#include "Effect/TunaSweeperProjectileHitEffectDataAsset.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Impact/TunaSweeperImpactEffectProfile.h"
#include "Impact/TunaSweeperImpactResponseProvider.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"

namespace
{
	bool DoesTagRuleMatch(const FTunaSweeperImpactTagRule& Rule, const FGameplayTagContainer& TargetTags)
	{
		return TargetTags.HasAll(Rule.RequiredTags) && !TargetTags.HasAny(Rule.BlockedTags);
	}
}

bool UTunaSweeperImpactEffectSubsystem::ResolveAndSpawnImpactEffect(
	FName ImpactProfileId,
	FName LegacyEffectId,
	const FHitResult& Hit,
	AActor* EffectOwner,
	APawn* EffectInstigator) const
{
	FTunaSweeperImpactResolveContext Context;
	if (const UPhysicalMaterial* PhysicalMaterial = Hit.PhysMaterial.Get())
	{
		Context.SurfaceType = PhysicalMaterial->SurfaceType;
	}

	if (AActor* HitActor = Hit.GetActor(); IsValid(HitActor) &&
		HitActor->GetClass()->ImplementsInterface(UTunaSweeperImpactResponseProvider::StaticClass()))
	{
		const FTunaSweeperImpactTargetResponse Response =
			ITunaSweeperImpactResponseProvider::Execute_GetImpactResponse(HitActor, Hit);
		if (Response.bOverrideSurface)
		{
			Context.SurfaceType = Response.SurfaceOverride;
		}
		Context.TargetResponseTags = Response.ResponseTags;
		Context.bSuppressDecal = Response.bSuppressDecal;
		Context.EffectScaleMultiplier = Response.EffectScaleMultiplier;
	}

	FTunaSweeperImpactEffectSpec Effect;
	if (const UTunaSweeperImpactEffectProfile* Profile = FindProfile(ImpactProfileId);
		Profile && ResolveImpactEffect(*Profile, Context, Effect))
	{
		SpawnResolvedEffect(Effect, Hit, Context);
		return true;
	}

	return SpawnLegacyEffect(LegacyEffectId, Hit, EffectOwner, EffectInstigator);
}

bool UTunaSweeperImpactEffectSubsystem::ResolveImpactEffect(
	const UTunaSweeperImpactEffectProfile& Profile,
	const FTunaSweeperImpactResolveContext& Context,
	FTunaSweeperImpactEffectSpec& OutEffect) const
{
	OutEffect = FTunaSweeperImpactEffectSpec();
	const FTunaSweeperImpactTagRule* BestTagRule = nullptr;
	for (const FTunaSweeperImpactTagRule& Rule : Profile.TagRules)
	{
		if (DoesTagRuleMatch(Rule, Context.TargetResponseTags) &&
			(!BestTagRule || Rule.Priority > BestTagRule->Priority))
		{
			BestTagRule = &Rule;
		}
	}
	if (BestTagRule)
	{
		OutEffect = BestTagRule->Effect;
		return !OutEffect.IsEmpty();
	}

	for (const FTunaSweeperImpactSurfaceRule& Rule : Profile.SurfaceRules)
	{
		if (Rule.SurfaceType == Context.SurfaceType)
		{
			OutEffect = Rule.Effect;
			return !OutEffect.IsEmpty();
		}
	}

	OutEffect = Profile.DefaultEffect;
	return !OutEffect.IsEmpty();
}

const UTunaSweeperImpactEffectProfile* UTunaSweeperImpactEffectSubsystem::FindProfile(FName ImpactProfileId) const
{
	if (ImpactProfileId.IsNone())
	{
		return nullptr;
	}

	const FString AssetName = ImpactProfileId.ToString();
	const FString AssetPath = FString::Printf(TEXT("/Game/Effects/Impact/%s.%s"), *AssetName, *AssetName);
	return LoadObject<UTunaSweeperImpactEffectProfile>(nullptr, *AssetPath);
}

void UTunaSweeperImpactEffectSubsystem::SpawnResolvedEffect(
	const FTunaSweeperImpactEffectSpec& Effect,
	const FHitResult& Hit,
	const FTunaSweeperImpactResolveContext& Context) const
{
	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

	const FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
	const FVector SpawnLocation = Hit.ImpactPoint + ImpactNormal * FMath::Max(0.0f, Effect.SurfaceOffsetCm);
	const FRotator SpawnRotation = ImpactNormal.Rotation();
	const float Scale = FMath::Max(0.0f, Effect.EffectScale * Context.EffectScaleMultiplier);

	if (UNiagaraSystem* NiagaraSystem = Effect.NiagaraSystem.LoadSynchronous())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, NiagaraSystem, SpawnLocation, SpawnRotation, FVector(Scale));
	}
	if (USoundBase* Sound = Effect.Sound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(World, Sound, SpawnLocation);
	}
	if (!Context.bSuppressDecal && Effect.bSpawnDecal)
	{
		if (UMaterialInterface* DecalMaterial = Effect.DecalMaterial.LoadSynchronous())
		{
			UGameplayStatics::SpawnDecalAtLocation(
				World,
				DecalMaterial,
				Effect.DecalSize * FMath::Max(Scale, KINDA_SMALL_NUMBER),
				SpawnLocation,
				SpawnRotation,
				FMath::Max(0.0f, Effect.DecalLifeSpanSeconds));
		}
	}
}

bool UTunaSweeperImpactEffectSubsystem::SpawnLegacyEffect(
	FName LegacyEffectId,
	const FHitResult& Hit,
	AActor* EffectOwner,
	APawn* EffectInstigator) const
{
	if (LegacyEffectId.IsNone())
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	FTunaSweeperProjectileHitEffectDefinition Definition;
	if (const UTunaSweeperGameInstance* GameInstance = World->GetGameInstance<UTunaSweeperGameInstance>())
	{
		GameInstance->TryGetProjectileHitEffectDefinition(LegacyEffectId, Definition);
	}

	TSubclassOf<ATunaSweeperProjectileHitBurstActor> EffectClass = Definition.EffectActorClass.LoadSynchronous();
	if (!EffectClass && LegacyEffectId == FName(TEXT("hit.red_burst")))
	{
		EffectClass = ATunaSweeperProjectileHitBurstActor::StaticClass();
		Definition.BurstColor = FLinearColor(1.0f, 0.03f, 0.0f, 1.0f);
		Definition.SpawnScale = FVector::OneVector;
		Definition.SurfaceOffsetCm = 1.0f;
	}
	if (!EffectClass)
	{
		return false;
	}

	const FVector ImpactNormal = Hit.ImpactNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = EffectOwner;
	SpawnParameters.Instigator = EffectInstigator;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	if (ATunaSweeperProjectileHitBurstActor* SpawnedActor = World->SpawnActor<ATunaSweeperProjectileHitBurstActor>(
		EffectClass,
		Hit.ImpactPoint + ImpactNormal * FMath::Max(0.0f, Definition.SurfaceOffsetCm),
		ImpactNormal.Rotation(),
		SpawnParameters))
	{
		SpawnedActor->SetActorScale3D(Definition.SpawnScale.IsNearlyZero() ? FVector::OneVector : Definition.SpawnScale);
		SpawnedActor->SetBurstColor(Definition.BurstColor);
		return true;
	}

	return false;
}
