#include "SogSplatComponent.h"

#include "DynamicMeshBuilder.h"
#include "Engine/Engine.h"
#include "LocalVertexFactory.h"
#include "Materials/Material.h"
#include "Materials/MaterialRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "SceneManagement.h"
#include "SogAsset.h"
#include "SogDecoder.h"
#include "StaticMeshResources.h"

struct FSogSplatRenderVertex
{
	FVector3f Position = FVector3f::ZeroVector;
	FVector2f UV = FVector2f::ZeroVector;
	FColor Color = FColor::White;
};

struct FSogSplatRenderData
{
	FSogSplatRenderData()
		: LocalBox(EForceInit::ForceInit)
	{
	}

	TArray<FSogSplatRenderVertex> Vertices;
	TArray<uint32> Indices;
	FBox LocalBox;
	int32 SplatCount = 0;

	bool IsValid() const
	{
		return SplatCount > 0 && Vertices.Num() > 0 && Indices.Num() > 0 && LocalBox.IsValid;
	}
};

namespace
{
	const TCHAR* DefaultSogMaterialPath = TEXT("/SogSplat/Materials/M_SogSoftEllipse.M_SogSoftEllipse");

	const FVector BaseCardCorners[4] = {
		FVector(-50.0, -50.0, 0.0),
		FVector(50.0, -50.0, 0.0),
		FVector(50.0, 50.0, 0.0),
		FVector(-50.0, 50.0, 0.0)
	};

	const FVector2f BaseCardUvs[4] = {
		FVector2f(0.0f, 0.0f),
		FVector2f(1.0f, 0.0f),
		FVector2f(1.0f, 1.0f),
		FVector2f(0.0f, 1.0f)
	};

	class FSogSplatSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FSogSplatSceneProxy(
			const USogSplatComponent* InComponent,
			const TSharedPtr<FSogSplatRenderData, ESPMode::ThreadSafe>& InRenderData,
			UMaterialInterface* InMaterial)
			: FPrimitiveSceneProxy(InComponent)
			, MaterialToUse(InMaterial ? InMaterial : UMaterial::GetDefaultMaterial(MD_Surface))
			, MaterialRelevance(MaterialToUse->GetRelevance_Concurrent(GetScene().GetShaderPlatform()))
			, VertexFactory(GetScene().GetFeatureLevel(), "FSogSplatSceneProxy")
		{
			check(InRenderData.IsValid());
			check(InRenderData->IsValid());

			const int32 NumVertices = InRenderData->Vertices.Num();
			VertexBuffers.PositionVertexBuffer.Init(NumVertices);
			VertexBuffers.StaticMeshVertexBuffer.Init(NumVertices, 1);
			VertexBuffers.ColorVertexBuffer.Init(NumVertices);

			const FVector3f TangentX(1.0f, 0.0f, 0.0f);
			const FVector3f TangentY(0.0f, 1.0f, 0.0f);
			const FVector3f TangentZ(0.0f, 0.0f, 1.0f);
			for (int32 VertexIndex = 0; VertexIndex < NumVertices; ++VertexIndex)
			{
				const FSogSplatRenderVertex& SourceVertex = InRenderData->Vertices[VertexIndex];
				VertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex) = SourceVertex.Position;
				VertexBuffers.StaticMeshVertexBuffer.SetVertexTangents(VertexIndex, TangentX, TangentY, TangentZ);
				VertexBuffers.StaticMeshVertexBuffer.SetVertexUV(VertexIndex, 0, SourceVertex.UV);
				VertexBuffers.ColorVertexBuffer.VertexColor(VertexIndex) = SourceVertex.Color;
			}

			IndexBuffer.Indices = InRenderData->Indices;
			NumPrimitives = IndexBuffer.Indices.Num() / 3;
			LocalBounds = FBoxSphereBounds(InRenderData->LocalBox);
			bSinglePassGDME = true;

			FSogSplatSceneProxy* Self = this;
			ENQUEUE_RENDER_COMMAND(FSogSplatSceneProxyInit)(
				[Self](FRHICommandListImmediate& RHICmdList)
				{
					Self->VertexBuffers.PositionVertexBuffer.InitResource(RHICmdList);
					Self->VertexBuffers.StaticMeshVertexBuffer.InitResource(RHICmdList);
					Self->VertexBuffers.ColorVertexBuffer.InitResource(RHICmdList);

					FLocalVertexFactory::FDataType VertexData;
					Self->VertexBuffers.PositionVertexBuffer.BindPositionVertexBuffer(&Self->VertexFactory, VertexData);
					Self->VertexBuffers.StaticMeshVertexBuffer.BindTangentVertexBuffer(&Self->VertexFactory, VertexData);
					Self->VertexBuffers.StaticMeshVertexBuffer.BindPackedTexCoordVertexBuffer(&Self->VertexFactory, VertexData);
					Self->VertexBuffers.StaticMeshVertexBuffer.BindLightMapVertexBuffer(&Self->VertexFactory, VertexData, 0);
					Self->VertexBuffers.ColorVertexBuffer.BindColorVertexBuffer(&Self->VertexFactory, VertexData);
					Self->VertexFactory.SetData(RHICmdList, VertexData);
					Self->VertexFactory.InitResource(RHICmdList);

					Self->IndexBuffer.InitResource(RHICmdList);
				});
		}

		virtual ~FSogSplatSceneProxy() override
		{
			IndexBuffer.ReleaseResource();
			VertexFactory.ReleaseResource();
			VertexBuffers.PositionVertexBuffer.ReleaseResource();
			VertexBuffers.StaticMeshVertexBuffer.ReleaseResource();
			VertexBuffers.ColorVertexBuffer.ReleaseResource();
		}

		virtual void GetDynamicMeshElements(
			const TArray<const FSceneView*>& Views,
			const FSceneViewFamily& ViewFamily,
			uint32 VisibilityMap,
			FMeshElementCollector& Collector) const override
		{
			if (NumPrimitives <= 0)
			{
				return;
			}

			FMaterialRenderProxy* MaterialProxy = MaterialToUse->GetRenderProxy();
			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
			{
				if ((VisibilityMap & (1 << ViewIndex)) == 0)
				{
					continue;
				}

				FMeshBatch& Mesh = Collector.AllocateMesh();
				Mesh.VertexFactory = &VertexFactory;
				Mesh.MaterialRenderProxy = MaterialProxy;
				Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
				Mesh.CastShadow = false;
				Mesh.Type = PT_TriangleList;
				Mesh.DepthPriorityGroup = static_cast<ESceneDepthPriorityGroup>(GetDepthPriorityGroup(Views[ViewIndex]));
				Mesh.bCanApplyViewModeOverrides = false;
				Mesh.bDisableBackfaceCulling = true;
				Mesh.bUseAsOccluder = false;
				Mesh.bUseForDepthPass = false;

				FMeshBatchElement& BatchElement = Mesh.Elements[0];
				BatchElement.IndexBuffer = &IndexBuffer;
				BatchElement.FirstIndex = 0;
				BatchElement.NumPrimitives = NumPrimitives;
				BatchElement.MinVertexIndex = 0;
				BatchElement.MaxVertexIndex = VertexBuffers.PositionVertexBuffer.GetNumVertices() - 1;

				FDynamicPrimitiveUniformBuffer& DynamicPrimitiveUniformBuffer = Collector.AllocateOneFrameResource<FDynamicPrimitiveUniformBuffer>();
				DynamicPrimitiveUniformBuffer.Set(
					Collector.GetRHICommandList(),
					GetLocalToWorld(),
					GetLocalToWorld(),
					GetBounds(),
					LocalBounds,
					false,
					false,
					AlwaysHasVelocity());
				BatchElement.PrimitiveUniformBufferResource = &DynamicPrimitiveUniformBuffer.UniformBuffer;

				Collector.AddMesh(ViewIndex, Mesh);
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = IsShown(View);
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = false;
			Result.bRenderInMainPass = ShouldRenderInMainPass();
			Result.bUsesLightingChannels = GetLightingChannelMask() != GetDefaultLightingChannelMask();
			Result.bRenderCustomDepth = ShouldRenderCustomDepth();
			MaterialRelevance.SetPrimitiveViewRelevance(Result);
			Result.bVelocityRelevance = DrawsVelocity() && Result.bOpaque && Result.bRenderInMainPass;
			return Result;
		}

		virtual bool CanBeOccluded() const override
		{
			return !MaterialRelevance.bDisableDepthTest;
		}

		virtual uint32 GetMemoryFootprint() const override
		{
			return sizeof(*this) + GetAllocatedSize();
		}

		uint32 GetAllocatedSize() const
		{
			return static_cast<uint32>(FPrimitiveSceneProxy::GetAllocatedSize() + IndexBuffer.Indices.GetAllocatedSize());
		}

	private:
		FStaticMeshVertexBuffers VertexBuffers;
		FDynamicMeshIndexBuffer32 IndexBuffer;
		UMaterialInterface* MaterialToUse = nullptr;
		FMaterialRelevance MaterialRelevance;
		FLocalVertexFactory VertexFactory;
		FBoxSphereBounds LocalBounds = FBoxSphereBounds(EForceInit::ForceInit);
		int32 NumPrimitives = 0;
	};
}

USogSplatComponent::USogSplatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetCanEverAffectNavigation(false);
	SetGenerateOverlapEvents(false);
	SetCustomNavigableGeometry(EHasCustomNavigableGeometry::No);
	CastShadow = false;
	bCastDynamicShadow = false;
	bAffectDistanceFieldLighting = false;
	bReceivesDecals = false;
	bUseAsOccluder = false;
#if WITH_EDITORONLY_DATA
	bSelectable = false;
	bWantsEditorEffects = false;
#endif
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
	RenderData.Reset();
	CachedLocalBounds = FBoxSphereBounds(EForceInit::ForceInit);

	ApplyDefaultMaterial();

	if (!SourceAsset)
	{
		bInstancesBuilt = true;
		UpdateBounds();
		MarkRenderStateDirty();
		return false;
	}

	if (!SourceAsset->EnsureSplatsDecoded(LastError))
	{
		bInstancesBuilt = true;
		UpdateBounds();
		MarkRenderStateDirty();
		return false;
	}

	const TArray<FSogSplatInstance>& Splats = SourceAsset->GetSplats();
	if (Splats.IsEmpty())
	{
		bInstancesBuilt = true;
		UpdateBounds();
		MarkRenderStateDirty();
		return false;
	}

	const int32 Stride = FMath::Max(1, InstanceStride);
	const int32 MaxCount = FMath::Max(0, MaxRenderedInstances);
	const int32 AvailableSplatCount = FMath::DivideAndRoundUp(Splats.Num(), Stride);
	const int32 EstimatedSplatCount = MaxCount > 0 ? FMath::Min(MaxCount, AvailableSplatCount) : AvailableSplatCount;
	if (EstimatedSplatCount <= 0)
	{
		bInstancesBuilt = true;
		UpdateBounds();
		MarkRenderStateDirty();
		return false;
	}

	const int64 EstimatedVertexCount = static_cast<int64>(EstimatedSplatCount) * 4;
	const int64 EstimatedIndexCount = static_cast<int64>(EstimatedSplatCount) * 6;
	if (EstimatedVertexCount > TNumericLimits<int32>::Max() || EstimatedIndexCount > TNumericLimits<int32>::Max())
	{
		LastError = NSLOCTEXT("SogSplat", "RenderDataTooLarge", "SOG render data is too large for one mesh section.");
		bInstancesBuilt = true;
		UpdateBounds();
		MarkRenderStateDirty();
		return false;
	}

	TSharedPtr<FSogSplatRenderData, ESPMode::ThreadSafe> NewRenderData = MakeShared<FSogSplatRenderData, ESPMode::ThreadSafe>();
	NewRenderData->Vertices.Reserve(static_cast<int32>(EstimatedVertexCount));
	NewRenderData->Indices.Reserve(static_cast<int32>(EstimatedIndexCount));

	for (int32 SplatIndex = 0; SplatIndex < Splats.Num(); SplatIndex += Stride)
	{
		if (MaxCount > 0 && NewRenderData->SplatCount >= MaxCount)
		{
			break;
		}

		const FSogSplatInstance& Splat = Splats[SplatIndex];
		const FColor VertexColor = Splat.Color.ToFColor(false);
		const uint32 BaseVertexIndex = static_cast<uint32>(NewRenderData->Vertices.Num());

		for (int32 CornerIndex = 0; CornerIndex < 4; ++CornerIndex)
		{
			const FVector CornerPosition = Splat.Transform.TransformPosition(BaseCardCorners[CornerIndex]);
			FSogSplatRenderVertex& Vertex = NewRenderData->Vertices.AddDefaulted_GetRef();
			Vertex.Position = FVector3f(CornerPosition);
			Vertex.UV = BaseCardUvs[CornerIndex];
			Vertex.Color = VertexColor;
			NewRenderData->LocalBox += CornerPosition;
		}

		NewRenderData->Indices.Add(BaseVertexIndex + 0);
		NewRenderData->Indices.Add(BaseVertexIndex + 1);
		NewRenderData->Indices.Add(BaseVertexIndex + 2);
		NewRenderData->Indices.Add(BaseVertexIndex + 0);
		NewRenderData->Indices.Add(BaseVertexIndex + 2);
		NewRenderData->Indices.Add(BaseVertexIndex + 3);
		++NewRenderData->SplatCount;
	}

	if (!NewRenderData->IsValid())
	{
		bInstancesBuilt = true;
		UpdateBounds();
		MarkRenderStateDirty();
		return false;
	}

	RenderedInstanceCount = NewRenderData->SplatCount;
	CachedLocalBounds = FBoxSphereBounds(NewRenderData->LocalBox);
	RenderData = MoveTemp(NewRenderData);
	UpdateBounds();
	MarkRenderStateDirty();
	bInstancesBuilt = true;
	return true;
}

FPrimitiveSceneProxy* USogSplatComponent::CreateSceneProxy()
{
	if (!RenderData.IsValid() || !RenderData->IsValid())
	{
		return nullptr;
	}

	return new FSogSplatSceneProxy(this, RenderData, ResolveMaterial());
}

FBoxSphereBounds USogSplatComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	return CachedLocalBounds.SphereRadius > 0.0
		? CachedLocalBounds.TransformBy(LocalToWorld)
		: FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.0);
}

int32 USogSplatComponent::GetNumMaterials() const
{
	return 1;
}

void USogSplatComponent::ApplyDefaultMaterial()
{
	if (UMaterialInterface* Material = ResolveMaterial())
	{
		if (GetMaterial(0) != Material)
		{
			SetMaterial(0, Material);
		}
	}
}

UMaterialInterface* USogSplatComponent::ResolveMaterial() const
{
	if (MaterialOverride)
	{
		return MaterialOverride;
	}

	if (UMaterialInterface* ExistingMaterial = GetMaterial(0))
	{
		return ExistingMaterial;
	}

	if (SourceAsset && !SourceAsset->DefaultMaterial.IsNull())
	{
		if (UMaterialInterface* AssetMaterial = SourceAsset->DefaultMaterial.LoadSynchronous())
		{
			return AssetMaterial;
		}
	}

	if (UMaterialInterface* DefaultMaterial = LoadObject<UMaterialInterface>(nullptr, DefaultSogMaterialPath))
	{
		return DefaultMaterial;
	}

	return UMaterial::GetDefaultMaterial(MD_Surface);
}

#if WITH_EDITOR
void USogSplatComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(USogSplatComponent, SourceAsset)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(USogSplatComponent, InstanceStride)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(USogSplatComponent, MaxRenderedInstances)
		|| PropertyName == GET_MEMBER_NAME_CHECKED(USogSplatComponent, MaterialOverride))
	{
		bInstancesBuilt = false;
		RebuildInstances();
	}
}
#endif
