#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "ForestScatterRegion.h"
#include "PCGForestScatterPoints.h"

#include "Data/PCGPointArrayData.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "Engine/StaticMesh.h"
#include "Internationalization/StringTableCore.h"
#include "Internationalization/StringTableRegistry.h"
#include "Materials/MaterialInterface.h"
#include "MeshSelectors/PCGMeshSelectorByAttribute.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGObjectPropertyOverride.h"
#include "Misc/AutomationTest.h"
#include "PCGEdge.h"
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGPin.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterEditorTextTest, "TunaSweeper.ForestScatter.Content.GraphEditorStrings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterEditorTextTest::RunTest(const FString&)
{
	for (const FName TableId : {FName(TEXT("ForestScatter.Editor")), FName(TEXT("ForestScatter.Editor.ko"))})
	{
		const FStringTableConstPtr Table = FStringTableRegistry::Get().FindStringTable(TableId);
		if (!TestTrue(*TableId.ToString(), Table.IsValid()))
		{
			continue;
		}
		for (const TCHAR* Key : {TEXT("Node.Points"), TEXT("Warning.GridTooLarge"), TEXT("Warning.NoMapGuide"), TEXT("Warning.NoLandscape")})
		{
			FString Source;
			TestTrue(Key, Table->GetSourceString(Key, Source) && !Source.IsEmpty());
		}
	}
	TestTrue(TEXT("The graph node resolves its title by string table key"),
		GetDefault<UPCGForestScatterPointsSettings>()->GetDefaultNodeTitle().IsFromStringTable());
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterContentTest, "TunaSweeper.ForestScatter.Content.GraphAndVegetation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterContentTest::RunTest(const FString&)
{
	const UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, TEXT("/Game/Environment/ForestScatter/PCG_ForestScatter.PCG_ForestScatter"));
	if (!TestNotNull(TEXT("Saved planting graph"), Graph))
	{
		return false;
	}

	TestTrue(TEXT("Available in Draw Spline Surface"), Graph->ToolData.bIsPreset && Graph->ToolData.CompatibleToolTags.Contains(TEXT("SplineSurfaceTool")));
	TestTrue(TEXT("Drawing creates a configurable planting region"), Graph->ToolData.InitialActorClassToSpawn == AForestScatterRegion::StaticClass());
	const UPCGNode* Points = nullptr;
	TArray<const UPCGNode*> Spawners;
	for (const UPCGNode* Node : Graph->GetNodes())
	{
		if (Node && Node->GetSettings())
		{
			if (Node->GetSettings()->IsA<UPCGForestScatterPointsSettings>())
			{
				Points = Node;
			}
			else if (Node->GetSettings()->IsA<UPCGStaticMeshSpawnerSettings>())
			{
				Spawners.Add(Node);
			}
		}
	}
	if (!TestNotNull(TEXT("Planting points"), Points) || !TestEqual(TEXT("Ground cover and flowers have native spawners"), Spawners.Num(), 3))
	{
		return false;
	}
	TSet<FName> SpawnedOutputs;
	for (const UPCGNode* Spawner : Spawners)
	{
		const UPCGStaticMeshSpawnerSettings* Settings = CastChecked<UPCGStaticMeshSpawnerSettings>(Spawner->GetSettings());
		const UPCGMeshSelectorByAttribute* Selector = Cast<UPCGMeshSelectorByAttribute>(Settings->MeshSelectorParameters);
		const UPCGPin* MeshInput = Spawner->GetInputPin(PCGPinConstants::DefaultInputLabel);
		if (!TestNotNull(TEXT("Spawner selects the regional mesh"), Selector) || !TestNotNull(TEXT("Spawner point input"), MeshInput))
		{
			return false;
		}
		TestEqual(TEXT("Point mesh attribute"), Selector->AttributeName, FName(TEXT("Mesh")));
		if (MeshInput->Edges.ContainsByPredicate([Points, MeshInput](const UPCGEdge* Edge)
		{
			return Edge && Edge->GetOtherPin(MeshInput) == Points->GetOutputPin(TEXT("Boulders"));
		}))
		{
			SpawnedOutputs.Add(TEXT("Boulders"));
			TestTrue(TEXT("Boulders block queries and physics"), !Selector->TemplateDescriptor.bUseDefaultCollision &&
				Selector->TemplateDescriptor.BodyInstance.GetCollisionProfileName() == FName(TEXT("BlockAll")));
			continue;
		}
		TestTrue(TEXT("Ground cover does not intercept players"), !Selector->TemplateDescriptor.bUseDefaultCollision &&
			Selector->TemplateDescriptor.BodyInstance.GetCollisionEnabled(false) == ECollisionEnabled::NoCollision);
		const FName SourceLabel = Selector->bUseAttributeMaterialOverrides ? FName(TEXT("Flowers")) : PCGPinConstants::DefaultOutputLabel;
		SpawnedOutputs.Add(SourceLabel);
		TestTrue(TEXT("Spawner receives its matching output"), MeshInput->Edges.ContainsByPredicate([Points, MeshInput, SourceLabel](const UPCGEdge* Edge)
		{
			return Edge && Edge->GetOtherPin(MeshInput) == Points->GetOutputPin(SourceLabel);
		}));
		const FPCGObjectPropertyOverrideDescription* Shadow = Settings->StaticMeshComponentPropertyOverrides.FindByPredicate([](const FPCGObjectPropertyOverrideDescription& Override)
		{
			return Override.PropertyTarget == GET_MEMBER_NAME_STRING_CHECKED(FPCGSoftISMComponentDescriptor, bCastShadow);
		});
		if (Selector->bUseAttributeMaterialOverrides)
		{
			TestTrue(TEXT("Only the second flower slot takes the petal color"),
				Selector->MaterialOverrideAttributes == TArray<FName>{TEXT("FoliageMaterial"), TEXT("PetalMaterial")});
			TestNull(TEXT("Flowers keep their descriptor's shadow setting"), Shadow);
		}
		else if (TestNotNull(TEXT("Ground cover reads its layer's shadow setting"), Shadow))
		{
			for (bool bCastShadow : {false, true})
			{
				UPCGPointArrayData* Data = NewObject<UPCGPointArrayData>();
				Data->SetNumPoints(1);
				Data->MutableMetadata()->CreateAttribute<bool>(TEXT("CastFoliageShadow"), bCastShadow, false, false);
				FPCGSoftISMComponentDescriptor Descriptor = Selector->TemplateDescriptor;
				Descriptor.bCastShadow = !bCastShadow;
				FPCGObjectOverrides Overrides(&Descriptor);
				Overrides.Initialize({*Shadow}, &Descriptor, Data, nullptr);
				TestTrue(TEXT("The layer's boolean applies to the instanced mesh descriptor"),
					Overrides.IsValid() && Overrides.Apply(0) && Descriptor.bCastShadow == bCastShadow);
			}
		}
	}
	TestEqual(TEXT("All output types are connected"), SpawnedOutputs.Num(), 3);

	const AForestScatterRegion* Defaults = GetDefault<AForestScatterRegion>();
	TestNull(TEXT("Saved tree settings have no PCG input"), Defaults->GetClass()->FindPropertyByName(TEXT("Trees")));
	TestNull(TEXT("Hand-planted hydrangeas have no regional density setting"), Defaults->GetClass()->FindPropertyByName(TEXT("Hydrangeas")));
	TestEqual(TEXT("New regions use the reduced grass density per 100 square metres"), Defaults->Grass.InstancesPer100SquareMeters, 35.0f);
	TestTrue(TEXT("Large stones are sparser than gravel"), Defaults->Boulders.InstancesPer100SquareMeters > 0.0f &&
		Defaults->Boulders.InstancesPer100SquareMeters < Defaults->Gravel.InstancesPer100SquareMeters * 0.1f);
	TSet<FName> Meshes;
	const auto SeedSlots = Defaults->GetGroundCoverLayers();
	TestTrue(TEXT("Existing regions retain their shrub, flower and grass seeds"),
		SeedSlots[0] == &Defaults->Shrubs && SeedSlots[1] == &Defaults->Flowers && SeedSlots[2] == &Defaults->Grass);
	TestNull(TEXT("The former hydrangea seed is retired"), SeedSlots[3]);
	TestTrue(TEXT("Existing regions retain their gravel and boulder seeds"),
		SeedSlots[4] == &Defaults->Gravel && SeedSlots[5] == &Defaults->Boulders);
	for (const FForestScatterLayer* Layer : SeedSlots)
	{
		if (!Layer)
		{
			continue;
		}
		const UStaticMesh* Mesh = Layer->Mesh.LoadSynchronous();
		if (!TestNotNull(TEXT("Default ground cover mesh"), Mesh))
		{
			continue;
		}
		Meshes.Add(Mesh->GetFName());
		TestTrue(*Mesh->GetName(), Mesh->GetNumLODs() > 0 && Mesh->GetBounds().BoxExtent.Z > 0);
		const UMaterialInterface* Base = Mesh->GetMaterial(0);
		if (TestNotNull(*Mesh->GetName(), Base))
		{
			const bool bFlower = Layer == &Defaults->Flowers;
			TestTrue(*FString::Printf(TEXT("%s material supports its geometry"), *Mesh->GetName()),
				Base->GetBlendMode() == (bFlower ? BLEND_Masked : BLEND_Opaque));
			if (Layer != &Defaults->Gravel && Layer != &Defaults->Boulders)
			{
				TestTrue(*FString::Printf(TEXT("Leaves on %s are visible from both sides"), *Mesh->GetName()), Base->IsTwoSided());
			}
		}
	}
	for (const TCHAR* Name : {TEXT("SM_ForestShrub"), TEXT("SM_ForestFlower"), TEXT("SM_ForestLawnGrass"), TEXT("SM_ForestGravel"), TEXT("SM_ForestBoulder")})
	{
		TestTrue(Name, Meshes.Contains(FName(Name)));
	}
	TestFalse(TEXT("The shared graph never seeds the hand-planted hydrangea mesh"), Meshes.Contains(TEXT("SM_ForestHydrangea")));
	TestEqual(TEXT("Only the five ground cover meshes enter PCG"), Meshes.Num(), 5);
	const UStaticMesh* Flower = Defaults->Flowers.Mesh.Get();
	if (TestNotNull(TEXT("Shared flower mesh"), Flower))
	{
		TestTrue(TEXT("The flower separates foliage and petals"), Flower->GetStaticMaterials().Num() == 2 &&
			Flower->GetMaterial(0) && Flower->GetMaterial(1) && Flower->GetMaterial(0) != Flower->GetMaterial(1));
	}
	TSet<FName> PetalMaterials;
	for (const TSoftObjectPtr<UMaterialInterface>& Petal : Defaults->FlowerPetalMaterials)
	{
		if (const UMaterialInterface* Material = Petal.LoadSynchronous())
		{
			PetalMaterials.Add(Material->GetFName());
			TestTrue(*Material->GetName(), Material->GetBlendMode() == BLEND_Masked && Material->IsTwoSided());
		}
	}
	for (const FName Color : {FName(TEXT("MI_ForestPetalWhite")), FName(TEXT("MI_ForestPetalYellow")), FName(TEXT("MI_ForestPetalRose")), FName(TEXT("MI_ForestPetalLilac"))})
	{
		TestTrue(*Color.ToString(), PetalMaterials.Contains(Color));
	}
	return !HasAnyErrors();
}

#endif
