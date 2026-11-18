#include "PCGForestScatterPoints.h"

#include "ForestScatterEditorText.h"
#include "ForestScatterGeometry.h"
#include "ForestScatterMask.h"
#include "ForestScatterRegion.h"

#include "CollisionQueryParams.h"
#include "Components/SplineComponent.h"
#include "Data/PCGBasePointData.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Helpers/PCGHelpers.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeProxy.h"
#include "Materials/MaterialInterface.h"
#include "Metadata/PCGMetadata.h"
#include "PCGContext.h"
#include "PCGElement.h"

namespace
{
	const FName FlowersPin(TEXT("Flowers"));

	struct FForestScatterLayerWork
	{
		FForestScatterGrid Grid;
		FSoftObjectPath Mesh;
		int32 Seed;
		bool bUsePetalColors = false;
	};

	struct FForestScatterPlacement
	{
		FVector Location;
		float Yaw;
		float Scale;
		int32 Seed;
	};

	struct FForestScatterContext : FPCGContext
	{
		FForestScatterBoundary Boundary;
		FForestScatterMask Mask;
		FForestBuildingFootprints Buildings;
		TWeakObjectPtr<UWorld> World;
		FBox LandscapeBounds{ForceInit};
		FCollisionObjectQueryParams LandscapeObjectTypes;
		TArray<FHitResult> GroundHits;
		TArray<FForestScatterLayerWork, TInlineAllocator<6>> Layers;
		TArray<FForestScatterPlacement> Placements;
		TArray<FSoftObjectPath> PetalMaterials;
		double FlowerPatchSize = 0.0;
		double BuildingTolerance = 0.0;
		int32 CurrentLayer = 0;
		int32 NextCell = 0;
	};

	void SampleCell(FForestScatterContext& Context, const FForestScatterLayerWork& Layer, int32 CellIndex)
	{
		const FForestScatterSample Sample = Layer.Grid.Sample(CellIndex, Layer.Seed);
		if (!Context.Boundary.Contains(Sample.XY))
		{
			return;
		}

		UWorld* World = Context.World.Get();
		const FVector Start(Sample.XY, Context.LandscapeBounds.Max.Z + 1.0);
		const FVector End(Sample.XY, Context.LandscapeBounds.Min.Z - 1.0);
		if (!World || !Context.LandscapeBounds.IsInsideOrOnXY(Start))
		{
			return;
		}

		// The editor adds a hole-free Visibility shape to the terrain. Query its real collision object
		// channels and take the first landscape hit, passing through unrelated geometry on those channels.
		static const FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(ForestScatterGround), /*bTraceComplex=*/true);
		World->LineTraceMultiByObjectType(Context.GroundHits, Start, End, Context.LandscapeObjectTypes, TraceParams);
		const FHitResult* LandscapeHit = Context.GroundHits.FindByPredicate([](const FHitResult& Hit)
		{
			return Cast<ULandscapeHeightfieldCollisionComponent>(Hit.GetComponent()) != nullptr;
		});
		if (!LandscapeHit)
		{
			return;
		}
		const FVector Ground = LandscapeHit->ImpactPoint;
		if (Context.Mask.IsGreen(Ground) && !Context.Buildings.TouchesGround(Ground, Context.BuildingTolerance))
		{
			Context.Placements.Add({Ground, Sample.Yaw, Sample.Scale, Sample.Seed});
		}
	}

	void AssignPetalMaterials(const FForestScatterContext& Context, int32 Seed, UPCGBasePointData& Points)
	{
		UPCGMetadata* Metadata = Points.MutableMetadata();
		// A null override on slot 0 preserves the flower mesh's leaf and stem material.
		Metadata->CreateAttribute<FSoftObjectPath>(TEXT("FoliageMaterial"), FSoftObjectPath(), false, false);
		FPCGMetadataAttribute<FSoftObjectPath>* Petal = Metadata->CreateAttribute<FSoftObjectPath>(TEXT("PetalMaterial"), FSoftObjectPath(), false, false);
		TArray<int64, TInlineAllocator<4>> ColorEntries;
		for (const FSoftObjectPath& Material : Context.PetalMaterials)
		{
			const int64 Entry = Metadata->AddEntry();
			Petal->SetValue(Entry, Material);
			ColorEntries.Add(Entry);
		}
		// Flowers of the same color share a metadata entry and an instanced mesh descriptor.
		Points.AllocateProperties(EPCGPointNativeProperties::MetadataEntry);
		TPCGValueRange<int64> PointEntries = Points.GetMetadataEntryValueRange(false);
		for (int32 Index = 0; Index < Context.Placements.Num(); ++Index)
		{
			const FVector2D Position(Context.Placements[Index].Location);
			PointEntries[Index] = ColorEntries[ForestScatter::FlowerMaterialIndex(Position, Context.FlowerPatchSize, Seed, ColorEntries.Num())];
		}
	}

	void WriteLayer(FForestScatterContext& Context, const FForestScatterLayerWork& Layer)
	{
		if (Context.Placements.IsEmpty())
		{
			return;
		}
		UPCGBasePointData* Points = FPCGContext::NewPointData_AnyThread(&Context);
		Points->SetNumPoints(Context.Placements.Num());
		Points->AllocateProperties(EPCGPointNativeProperties::Transform | EPCGPointNativeProperties::Seed);
		TPCGValueRange<FTransform> Transforms = Points->GetTransformValueRange(false);
		TPCGValueRange<int32> Seeds = Points->GetSeedValueRange(false);
		for (int32 Index = 0; Index < Context.Placements.Num(); ++Index)
		{
			const FForestScatterPlacement& Placement = Context.Placements[Index];
			Transforms[Index] = FTransform(FRotator(0, Placement.Yaw, 0), Placement.Location, FVector(Placement.Scale));
			Seeds[Index] = Placement.Seed;
		}
		// Every point in this data item uses the same mesh. The attribute default covers all its point entries.
		Points->MutableMetadata()->CreateAttribute<FSoftObjectPath>(TEXT("Mesh"), Layer.Mesh, false, false);
		if (Layer.bUsePetalColors)
		{
			AssignPetalMaterials(Context, Layer.Seed, *Points);
		}
		FPCGTaggedData& Output = Context.OutputData.TaggedData.Emplace_GetRef();
		Output.Pin = Layer.bUsePetalColors ? FlowersPin : PCGPinConstants::DefaultOutputLabel;
		Output.Data = Points;
		Context.Placements.Reset();
	}

	class FPCGForestScatterPointsElement final : public IPCGElementWithCustomContext<FForestScatterContext>
	{
	public:
		virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override { return true; }
		// Decal pixels, landscape collision and building transforms are read afresh on each generation.
		virtual bool IsCacheable(const UPCGSettings* Settings) const override { return false; }
		virtual bool ShouldComputeFullOutputDataCrc(FPCGContext* Context) const override { return true; }

	protected:
		virtual bool PrepareDataInternal(FPCGContext* InContext) const override
		{
			FForestScatterContext* Context = static_cast<FForestScatterContext*>(InContext);
			const AForestScatterRegion* Region = Cast<AForestScatterRegion>(Context->GetTargetActor(nullptr));
			if (!Region || !Region->Boundary || !Region->GetWorld() || !Context->Boundary.Build(*Region->Boundary))
			{
				return true;
			}

			const auto SeedSlots = Region->GetGroundCoverLayers();
			for (int32 Index = 0; Index < SeedSlots.Num(); ++Index)
			{
				const FForestScatterLayer* Settings = SeedSlots[Index];
				if (!Settings || Settings->Mesh.IsNull() || Settings->InstancesPer100SquareMeters <= 0.0f)
				{
					continue;
				}
				FForestScatterLayerWork Layer;
				if (!Layer.Grid.Build(Context->Boundary.Bounds, Settings->InstancesPer100SquareMeters))
				{
					PCGE_LOG(Warning, GraphAndLog, ForestScatterEditorText::Get(TEXT("Warning.GridTooLarge")));
					continue;
				}
				// Offset zero belonged to trees in the first version of the graph.
				Layer.Seed = PCGHelpers::ComputeSeed(Context->GetSeed(), Index + 1);
				Layer.Mesh = Settings->Mesh.ToSoftObjectPath();
				if (Settings == &Region->Flowers && !Region->FlowerPetalMaterials.IsEmpty())
				{
					const UStaticMesh* Flower = Settings->Mesh.LoadSynchronous();
					if (Flower && Flower->GetStaticMaterials().IsValidIndex(1))
					{
						for (const TSoftObjectPtr<UMaterialInterface>& Material : Region->FlowerPetalMaterials)
						{
							if (!Material.IsNull())
							{
								Context->PetalMaterials.Add(Material.ToSoftObjectPath());
							}
						}
						Context->FlowerPatchSize = Region->FlowerColorPatchSize;
						Layer.bUsePetalColors = !Context->PetalMaterials.IsEmpty();
					}
				}
				Context->Layers.Add(MoveTemp(Layer));
			}
			if (Context->Layers.IsEmpty())
			{
				return true;
			}
			if (!Context->Mask.Build(Region->GetWorld(), Region->ProjectionDecals))
			{
				PCGE_LOG(Warning, GraphAndLog, ForestScatterEditorText::Get(TEXT("Warning.NoMapGuide")));
				Context->Layers.Reset();
				return true;
			}

			Context->World = Region->GetWorld();
			for (TActorIterator<ALandscapeProxy> It(Region->GetWorld()); It; ++It)
			{
				for (const ULandscapeHeightfieldCollisionComponent* Collision : It->CollisionComponents)
				{
					if (IsValid(Collision) && Collision->IsRegistered() && Collision->IsQueryCollisionEnabled())
					{
						const FBox Bounds = Collision->Bounds.GetBox();
						if (Context->Boundary.Bounds.Intersect(FBox2D(FVector2D(Bounds.Min), FVector2D(Bounds.Max))))
						{
							Context->LandscapeBounds += Bounds;
							Context->LandscapeObjectTypes.AddObjectTypesToQuery(Collision->GetCollisionObjectType());
						}
					}
				}
			}
			if (!Context->LandscapeBounds.IsValid)
			{
				PCGE_LOG(Warning, GraphAndLog, ForestScatterEditorText::Get(TEXT("Warning.NoLandscape")));
				Context->Layers.Reset();
				return true;
			}
			Context->BuildingTolerance = Region->BuildingGroundTolerance;
			for (const AActor* Building : Region->GroundedBuildings)
			{
				if (IsValid(Building) && Building != Region && Building->GetWorld() == Region->GetWorld())
				{
					Context->Buildings.AddBuilding(*Building);
				}
			}
			return true;
		}

		virtual bool ExecuteInternal(FPCGContext* InContext) const override
		{
			FForestScatterContext& Context = *static_cast<FForestScatterContext*>(InContext);
			while (Context.CurrentLayer < Context.Layers.Num())
			{
				const FForestScatterLayerWork& Layer = Context.Layers[Context.CurrentLayer];
				while (Context.NextCell < Layer.Grid.Num())
				{
					SampleCell(Context, Layer, Context.NextCell++);
					if ((Context.NextCell % 256) == 0 && Context.ShouldStop())
					{
						return false;
					}
				}
				WriteLayer(Context, Layer);
				++Context.CurrentLayer;
				Context.NextCell = 0;
			}
			return true;
		}
	};
}

TArray<FPCGPinProperties> UPCGForestScatterPointsSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Pins = DefaultPointOutputPinProperties();
	Pins.Emplace(FlowersPin, EPCGDataType::Point);
	return Pins;
}

#if WITH_EDITOR
FText UPCGForestScatterPointsSettings::GetDefaultNodeTitle() const
{
	return ForestScatterEditorText::Get(TEXT("Node.Points"));
}
#endif

FPCGElementPtr UPCGForestScatterPointsSettings::CreateElement() const
{
	return MakeShared<FPCGForestScatterPointsElement>();
}
