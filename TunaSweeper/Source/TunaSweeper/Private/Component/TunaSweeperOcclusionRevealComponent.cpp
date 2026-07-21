#include "Component/TunaSweeperOcclusionRevealComponent.h"

#include "Components/MeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

UTunaSweeperOcclusionRevealComponent::UTunaSweeperOcclusionRevealComponent()
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = false;
	RevealMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/Effects/M_OcclusionRevealMasked.M_OcclusionRevealMasked")));
}

void UTunaSweeperOcclusionRevealComponent::OnRegister()
{
	Super::OnRegister();
	ApplyOcclusionRevealMaterial();
}

void UTunaSweeperOcclusionRevealComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyOcclusionRevealMaterial();
}

void UTunaSweeperOcclusionRevealComponent::RegisterRevealMesh(UMeshComponent* MeshComponent)
{
	if (MeshComponent)
	{
		RevealMeshes.AddUnique(MeshComponent);
		ApplyOcclusionRevealMaterial();
	}
}

void UTunaSweeperOcclusionRevealComponent::CollectRevealMeshes(TArray<UMeshComponent*>& OutMeshes) const
{
	OutMeshes.Reset();
	for (UMeshComponent* MeshComponent : RevealMeshes)
	{
		if (MeshComponent)
		{
			OutMeshes.AddUnique(MeshComponent);
		}
	}

	if (bAutoCollectOwnerMeshes)
	{
		if (const AActor* Owner = GetOwner())
		{
			TArray<UMeshComponent*> OwnerMeshes;
			Owner->GetComponents(OwnerMeshes);
			for (UMeshComponent* MeshComponent : OwnerMeshes)
			{
				if (MeshComponent)
				{
					OutMeshes.AddUnique(MeshComponent);
				}
			}
		}
	}
}

void UTunaSweeperOcclusionRevealComponent::ApplyOcclusionRevealMaterial()
{
	UMaterialInterface* Material = RevealMaterial.LoadSynchronous();
	if (!Material)
	{
		return;
	}

	TArray<UMeshComponent*> Meshes;
	CollectRevealMeshes(Meshes);
	for (UMeshComponent* MeshComponent : Meshes)
	{
		if (!MeshComponent)
		{
			continue;
		}

		if (!OriginalMaterialStates.ContainsByPredicate([MeshComponent](const FOriginalMaterialState& State)
			{ return State.MeshComponent == MeshComponent; }))
		{
			FOriginalMaterialState& NewState = OriginalMaterialStates.AddDefaulted_GetRef();
			NewState.MeshComponent = MeshComponent;
			const int32 MaterialCount = MeshComponent->GetNumMaterials();
			for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
			{
				NewState.Materials.Add(MeshComponent->GetMaterial(MaterialIndex));
			}
		}

		const int32 MaterialCount = FMath::Max(1, MeshComponent->GetNumMaterials());
		for (int32 MaterialIndex = 0; MaterialIndex < MaterialCount; ++MaterialIndex)
		{
			if (bOverrideAllMaterialSlots || MaterialIndex == 0)
			{
				MeshComponent->SetMaterial(MaterialIndex, Material);
			}
		}
	}
}

void UTunaSweeperOcclusionRevealComponent::RestoreOriginalMaterials()
{
	for (const FOriginalMaterialState& State : OriginalMaterialStates)
	{
		if (UMeshComponent* MeshComponent = State.MeshComponent.Get())
		{
			for (int32 MaterialIndex = 0; MaterialIndex < State.Materials.Num(); ++MaterialIndex)
			{
				MeshComponent->SetMaterial(MaterialIndex, State.Materials[MaterialIndex]);
			}
		}
	}
	OriginalMaterialStates.Reset();
}
