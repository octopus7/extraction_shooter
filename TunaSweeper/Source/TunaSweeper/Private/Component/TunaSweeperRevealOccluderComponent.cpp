#include "Component/TunaSweeperRevealOccluderComponent.h"

#include "Component/TunaSweeperOcclusionRevealTypes.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

UTunaSweeperRevealOccluderComponent::UTunaSweeperRevealOccluderComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTunaSweeperRevealOccluderComponent::OnRegister()
{
	Super::OnRegister();
	ApplyRevealOccluderSettings();
}

void UTunaSweeperRevealOccluderComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyRevealOccluderSettings();
}

void UTunaSweeperRevealOccluderComponent::ApplyRevealOccluderSettings()
{
	TArray<UPrimitiveComponent*> MeshesToApply;
	CollectRevealMeshes(MeshesToApply);
	for (UPrimitiveComponent* PrimitiveComponent : MeshesToApply)
	{
		ApplyRevealDataToPrimitive(PrimitiveComponent);
	}
}

void UTunaSweeperRevealOccluderComponent::ConfigureRevealOccluderSettings(
	float InRevealIntensity,
	float InCharacterRadiusScale,
	float InCursorRadiusScale,
	float InPatternScale)
{
	RevealIntensity = FMath::Clamp(InRevealIntensity, 0.0f, 1.0f);
	CharacterRadiusScale = FMath::Max(0.0f, InCharacterRadiusScale);
	CursorRadiusScale = FMath::Max(0.0f, InCursorRadiusScale);
	PatternScale = FMath::Max(0.1f, InPatternScale);
	ApplyRevealOccluderSettings();
}

void UTunaSweeperRevealOccluderComponent::RegisterRevealMesh(UPrimitiveComponent* PrimitiveComponent)
{
	if (!PrimitiveComponent)
	{
		return;
	}

	RevealMeshes.AddUnique(PrimitiveComponent);
	ApplyRevealDataToPrimitive(PrimitiveComponent);
}

void UTunaSweeperRevealOccluderComponent::UnregisterRevealMesh(UPrimitiveComponent* PrimitiveComponent)
{
	RevealMeshes.Remove(PrimitiveComponent);
}

void UTunaSweeperRevealOccluderComponent::ClearRevealMeshes()
{
	RevealMeshes.Reset();
}

#if WITH_EDITOR
void UTunaSweeperRevealOccluderComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RevealIntensity = FMath::Clamp(RevealIntensity, 0.0f, 1.0f);
	CharacterRadiusScale = FMath::Max(0.0f, CharacterRadiusScale);
	CursorRadiusScale = FMath::Max(0.0f, CursorRadiusScale);
	PatternScale = FMath::Max(0.1f, PatternScale);
	ApplyRevealOccluderSettings();
}
#endif

void UTunaSweeperRevealOccluderComponent::CollectRevealMeshes(TArray<UPrimitiveComponent*>& OutMeshes) const
{
	OutMeshes.Reset();
	for (UPrimitiveComponent* PrimitiveComponent : RevealMeshes)
	{
		if (PrimitiveComponent)
		{
			OutMeshes.AddUnique(PrimitiveComponent);
		}
	}

	if (!bAutoCollectOwnerPrimitiveComponents)
	{
		return;
	}

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	TArray<UPrimitiveComponent*> OwnerPrimitiveComponents;
	OwnerActor->GetComponents(OwnerPrimitiveComponents);
	for (UPrimitiveComponent* PrimitiveComponent : OwnerPrimitiveComponents)
	{
		if (PrimitiveComponent)
		{
			OutMeshes.AddUnique(PrimitiveComponent);
		}
	}
}

void UTunaSweeperRevealOccluderComponent::ApplyRevealDataToPrimitive(UPrimitiveComponent* PrimitiveComponent) const
{
	if (!PrimitiveComponent)
	{
		return;
	}

	const float ClampedRevealIntensity = FMath::Clamp(RevealIntensity, 0.0f, 1.0f);
	const float ClampedCharacterRadiusScale = FMath::Max(0.0f, CharacterRadiusScale);
	const float ClampedCursorRadiusScale = FMath::Max(0.0f, CursorRadiusScale);
	const float ClampedPatternScale = FMath::Max(0.1f, PatternScale);

	PrimitiveComponent->SetCustomPrimitiveDataFloat(
		TunaSweeperOcclusionReveal::RevealIntensityPrimitiveDataIndex,
		ClampedRevealIntensity);
	PrimitiveComponent->SetCustomPrimitiveDataFloat(
		TunaSweeperOcclusionReveal::CharacterRadiusScalePrimitiveDataIndex,
		ClampedCharacterRadiusScale);
	PrimitiveComponent->SetCustomPrimitiveDataFloat(
		TunaSweeperOcclusionReveal::CursorRadiusScalePrimitiveDataIndex,
		ClampedCursorRadiusScale);
	PrimitiveComponent->SetCustomPrimitiveDataFloat(
		TunaSweeperOcclusionReveal::PatternScalePrimitiveDataIndex,
		ClampedPatternScale);

	PrimitiveComponent->SetDefaultCustomPrimitiveDataFloat(
		TunaSweeperOcclusionReveal::RevealIntensityPrimitiveDataIndex,
		ClampedRevealIntensity);
	PrimitiveComponent->SetDefaultCustomPrimitiveDataFloat(
		TunaSweeperOcclusionReveal::CharacterRadiusScalePrimitiveDataIndex,
		ClampedCharacterRadiusScale);
	PrimitiveComponent->SetDefaultCustomPrimitiveDataFloat(
		TunaSweeperOcclusionReveal::CursorRadiusScalePrimitiveDataIndex,
		ClampedCursorRadiusScale);
	PrimitiveComponent->SetDefaultCustomPrimitiveDataFloat(
		TunaSweeperOcclusionReveal::PatternScalePrimitiveDataIndex,
		ClampedPatternScale);
}
