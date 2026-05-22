#include "SogSplatComponent.h"

#include "Materials/Material.h"
#include "SogAsset.h"
#include "SogDecoder.h"

namespace
{
	const TCHAR* DefaultCardMeshPath = TEXT("/Engine/BasicShapes/Plane.Plane");
	const TCHAR* DefaultSogMaterialPath = TEXT("/SogSplat/Materials/M_SogSoftEllipse.M_SogSoftEllipse");
}

USogSplatComponent::USogSplatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
	SetGenerateOverlapEvents(false);
	bCastDynamicShadow = false;
	bAffectDistanceFieldLighting = false;
	bReceivesDecals = false;
	NumCustomDataFloats = 4;
	CardMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultCardMeshPath));
}

void USogSplatComponent::SetSogAsset(USogAsset* InAsset)
{
	if (SourceAsset == InAsset)
	{
		return;
	}

	SourceAsset = InAsset;
	bInstancesBuilt = false;
	RebuildInstances();
}

bool USogSplatComponent::LoadSogFile(const FString& FilePath, FSogDecodeOptions Options)
{
	LastError = FText::GetEmpty();

	USogAsset* RuntimeAsset = NewObject<USogAsset>(this, NAME_None, RF_Transient);
	if (!RuntimeAsset)
	{
		LastError = NSLOCTEXT("SogSplat", "RuntimeAssetCreateFailed", "Failed to create transient SOG asset.");
		return false;
	}

	FText DecodeError;
	if (!FSogDecoder::DecodeFileToAsset(FilePath, Options, RuntimeAsset, DecodeError))
	{
		LastError = DecodeError;
		return false;
	}

	SourceAsset = RuntimeAsset;
	bInstancesBuilt = false;
	return RebuildInstances();
}

void USogSplatComponent::OnRegister()
{
	Super::OnRegister();

	if (!bInstancesBuilt)
	{
		RebuildInstances();
	}
}

bool USogSplatComponent::RebuildInstances()
{
	LastError = FText::GetEmpty();
	RenderedInstanceCount = 0;

	ClearInstances();
	ApplyDefaultMeshAndMaterial();
	SetNumCustomDataFloats(4);

	if (!SourceAsset)
	{
		bInstancesBuilt = true;
		return false;
	}

	const TArray<FSogSplatInstance>& Splats = SourceAsset->GetSplats();
	if (Splats.IsEmpty())
	{
		bInstancesBuilt = true;
		return false;
	}

	const int32 Stride = FMath::Max(1, InstanceStride);
	const int32 MaxCount = FMath::Max(0, MaxRenderedInstances);
	const int32 EstimatedCount = MaxCount > 0 ? MaxCount : FMath::DivideAndRoundUp(Splats.Num(), Stride);

	TArray<FTransform> InstanceTransforms;
	TArray<FLinearColor> InstanceColors;
	InstanceTransforms.Reserve(EstimatedCount);
	InstanceColors.Reserve(EstimatedCount);

	for (int32 SplatIndex = 0; SplatIndex < Splats.Num(); SplatIndex += Stride)
	{
		if (MaxCount > 0 && InstanceTransforms.Num() >= MaxCount)
		{
			break;
		}

		InstanceTransforms.Add(Splats[SplatIndex].Transform);
		InstanceColors.Add(Splats[SplatIndex].Color);
	}

	if (InstanceTransforms.IsEmpty())
	{
		bInstancesBuilt = true;
		return false;
	}

	PreAllocateInstancesMemory(InstanceTransforms.Num());
	AddInstances(InstanceTransforms, false, false, false);

	PerInstanceSMCustomData.SetNumUninitialized(InstanceColors.Num() * 4);
	for (int32 InstanceIndex = 0; InstanceIndex < InstanceColors.Num(); ++InstanceIndex)
	{
		const FLinearColor& Color = InstanceColors[InstanceIndex];
		const int32 BaseIndex = InstanceIndex * 4;
		PerInstanceSMCustomData[BaseIndex + 0] = Color.R;
		PerInstanceSMCustomData[BaseIndex + 1] = Color.G;
		PerInstanceSMCustomData[BaseIndex + 2] = Color.B;
		PerInstanceSMCustomData[BaseIndex + 3] = Color.A;
	}

	RenderedInstanceCount = InstanceTransforms.Num();
	BuildTreeIfOutdated(false, true);
	MarkRenderStateDirty();
	bInstancesBuilt = true;
	return true;
}

void USogSplatComponent::ApplyDefaultMeshAndMaterial()
{
	UStaticMesh* Mesh = CardMesh.IsNull() ? nullptr : CardMesh.LoadSynchronous();
	if (!Mesh)
	{
		Mesh = LoadObject<UStaticMesh>(nullptr, DefaultCardMeshPath);
	}

	if (Mesh && GetStaticMesh() != Mesh)
	{
		SetStaticMesh(Mesh);
	}

	UMaterialInterface* Material = MaterialOverride;
	if (!Material && SourceAsset && !SourceAsset->DefaultMaterial.IsNull())
	{
		Material = SourceAsset->DefaultMaterial.LoadSynchronous();
	}
	if (!Material)
	{
		Material = LoadObject<UMaterialInterface>(nullptr, DefaultSogMaterialPath);
	}
	if (!Material)
	{
		Material = UMaterial::GetDefaultMaterial(MD_Surface);
	}

	if (Material)
	{
		SetMaterial(0, Material);
	}
}

#if WITH_EDITOR
void USogSplatComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USogSplatComponent, SourceAsset)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(USogSplatComponent, InstanceStride)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(USogSplatComponent, MaxRenderedInstances)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(USogSplatComponent, MaterialOverride)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(USogSplatComponent, CardMesh))
	{
		bInstancesBuilt = false;
		RebuildInstances();
	}
}
#endif
