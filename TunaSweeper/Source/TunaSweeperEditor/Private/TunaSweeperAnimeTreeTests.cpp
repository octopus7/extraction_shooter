#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Environment/TunaSweeperAnimeTreeActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MaterialShared.h"
#include "MeshDescription.h"
#include "Editor.h"
#include "Tests/AutomationEditorCommon.h"
#include "AssetCompilingManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/DirectionalLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "ContentStreaming.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace AnimeTreeTests
{
UClass* TreeClass()
{
	return LoadClass<ATunaSweeperAnimeTreeActor>(nullptr, TEXT("/Game/Environment/AnimeTree/BP_AnimeFoliageTree.BP_AnimeFoliageTree_C"));
}
UStaticMeshComponent* Leaf(ATunaSweeperAnimeTreeActor* Tree)
{
    const auto Clumps = Tree->GetLeafClumps();
    return Clumps.IsEmpty() ? nullptr : Clumps[0];
}
UStaticMeshComponent* Shadow(ATunaSweeperAnimeTreeActor* Tree)
{
    TArray<UStaticMeshComponent*> Meshes; Tree->GetComponents(Meshes);
    for (auto* Mesh : Meshes) if (Mesh->ComponentHasTag(TEXT("AnimeTreeShadow"))) return Mesh;
    return nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimeTreeRimTest, "TunaSweeper.AnimeTree.WholeCanopyRim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAnimeTreeRimTest::RunTest(const FString& Parameters)
{
	UClass* Class = AnimeTreeTests::TreeClass();
	if (!TestNotNull(TEXT("Tree BP"), Class)) return false;
	auto* Tree = FAutomationEditorCommonUtils::CreateNewMap()->SpawnActor<ATunaSweeperAnimeTreeActor>(Class);
	auto CheckSharedFrame = [this, Tree]()
	{
		const auto Clumps = Tree->GetLeafClumps();
		if (Clumps.IsEmpty()) return false;
		// A point in space has one rim coordinate, regardless of the leaf component sampling it.
		for (const FVector Point : {FVector(0,0,330), FVector(120,-80,400)})
		{
			FVector Reference = FVector::ZeroVector;
			for (int32 I = 0; I < Clumps.Num(); ++I)
			{
				auto* C = Clumps[I]; const auto& Data = C->GetCustomPrimitiveData().Data;
				if (!TestTrue(TEXT("Whole-canopy rim frame is supplied to each component"), Data.Num() >= 16)) return false;
				const FVector P = C->GetComponentTransform().InverseTransformPosition(Tree->GetActorTransform().TransformPosition(Point));
				FVector Normalized;
				for (int32 Axis = 0; Axis < 3; ++Axis)
				{
					const int32 J = 4 + Axis * 4;
					Normalized[Axis] = P.X*Data[J]+P.Y*Data[J+1]+P.Z*Data[J+2]+Data[J+3];
				}
				if (I == 0) Reference = Normalized;
				else TestTrue(TEXT("Rim does not restart at clump boundaries"), Normalized.Equals(Reference, .0001));
				TestFalse(TEXT("Rim coordinates remain finite"), Normalized.ContainsNaN());
			}
		}
		// Independent route: transform a world ray through a clump, then its shared
		// coordinate mapping. It must match the MID's direct camera-ray transform.
		const auto& Data = Clumps[0]->GetCustomPrimitiveData().Data;
		auto* MID = CastChecked<UMaterialInstanceDynamic>(Clumps[0]->GetMaterial(0));
		const FName Names[] = {TEXT("RimViewX"), TEXT("RimViewY"), TEXT("RimViewZ")};
		for (const FVector WorldRay : {FVector(.3,-.5,.8), FVector(-.7,.6,.2)})
		{
			const FVector LocalRay = Clumps[0]->GetComponentTransform().InverseTransformVector(WorldRay);
			for (int32 Axis = 0; Axis < 3; ++Axis)
			{
				const int32 J = 4 + Axis*4;
				const double Expected = LocalRay.X*Data[J]+LocalRay.Y*Data[J+1]+LocalRay.Z*Data[J+2];
				const FLinearColor Row = MID->K2_GetVectorParameterValue(Names[Axis]);
				const double Actual = WorldRay.X*Row.R+WorldRay.Y*Row.G+WorldRay.Z*Row.B;
				TestTrue(TEXT("Camera rays stay in the same rim frame after rotated nonuniform scaling"), FMath::IsNearlyEqual(Actual,Expected,.000001));
			}
		}
		return true;
	};
	if (!CheckSharedFrame()) { Tree->Destroy(); return false; }
	auto* Leaf = AnimeTreeTests::Leaf(Tree);
	const auto Before = Leaf->GetCustomPrimitiveData().Data;
	Tree->GetLeafClumps().Last()->SetRelativeLocation(FVector(380,-180,490));
	CheckSharedFrame();
	TestFalse(TEXT("Moving an outer clump updates the shared rim envelope"), Before == Leaf->GetCustomPrimitiveData().Data);
	Tree->SetActorTransform(FTransform(FRotator(15,40,5), FVector(500,-200,50), FVector(.75,1.2,.85)));
	CheckSharedFrame();
	Tree->GradientGuide->SetRelativeScale3D(FVector::ZeroVector);
	CheckSharedFrame(); // Gradient-guide collapse must not invalidate the separate rim frame.
	auto* Material = CastChecked<UMaterialInstanceDynamic>(Leaf->GetMaterial(0));
	Tree->RimStrength = 0;
	Tree->RimWidth = .45f;
	Tree->RimBrightness = 2;
	Tree->RimColor = FLinearColor(.8f,.3f,.1f);
	Tree->RefreshTree();
	TestEqual(TEXT("Rim can be fully disabled"), Material->K2_GetScalarParameterValue(TEXT("RimStrength")), 0.f);
	TestEqual(TEXT("Rim width reaches the material"), Material->K2_GetScalarParameterValue(TEXT("RimWidth")), .45f);
	TestEqual(TEXT("Rim brightness reaches the material"), Material->K2_GetScalarParameterValue(TEXT("RimBrightness")), 2.f);
	TestEqual(TEXT("Rim color reaches the material"), Material->K2_GetVectorParameterValue(TEXT("RimColor")), Tree->RimColor);
	Tree->SetActorScale3D(FVector::ZeroVector);
	CheckSharedFrame();
	for (FName Name : {FName(TEXT("RimViewX")), FName(TEXT("RimViewY")), FName(TEXT("RimViewZ"))})
	{
		const FLinearColor Row = Material->K2_GetVectorParameterValue(Name);
		TestTrue(TEXT("Collapsed actor has a safe disabled rim frame"), Row.R == 0 && Row.G == 0 && Row.B == 0);
	}
	Tree->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimeTreeModularTest, "TunaSweeper.AnimeTree.Modular",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAnimeTreeModularTest::RunTest(const FString& Parameters)
{
	UClass* Class = AnimeTreeTests::TreeClass();
	if (!TestNotNull(TEXT("Tree BP"), Class)) return false;
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	auto* Tree = World->SpawnActor<ATunaSweeperAnimeTreeActor>(Class);
	TArray<UStaticMeshComponent*> Meshes; Tree->GetComponents(Meshes);
	TArray<UStaticMeshComponent*> Clumps;
	FBox Centers(ForceInit);
	for (auto* Mesh : Meshes) if (Mesh->ComponentHasTag(TEXT("AnimeTreeClump")))
	{
		Clumps.Add(Mesh); Centers += Mesh->GetRelativeLocation();
	}
	TestEqual(TEXT("Ten editable clump components replace the monolithic canopy"), Clumps.Num(), 10);
	if (Clumps.Num()) TestTrue(TEXT("Canopy centers fill X as well as Y"), Centers.GetSize().X > Centers.GetSize().Y * .65);
	TArray<USceneComponent*> Scenes; Tree->GetComponents(Scenes);
	TestTrue(TEXT("One shared editable gradient guide"), Scenes.ContainsByPredicate([](auto* C){return C->GetFName() == TEXT("GradientGuide");}));
	if (Clumps.IsEmpty()) return false;
	TSet<UStaticMesh*> MeshAssets;
	for (auto* Clump : Clumps)
	{
		MeshAssets.Add(Clump->GetStaticMesh());
		TestTrue(TEXT("All clumps share the tree material instance"), Clump->GetMaterial(0) == Clumps[0]->GetMaterial(0));
	}
	TestEqual(TEXT("Ten clumps reuse three mesh variants"), MeshAssets.Num(), 3);
	// Independent oracle: transformed positions must produce the same ramp in every clump.
	auto CheckRamp = [this, Tree](const FVector& TreePoint)
	{
		const FVector WorldPoint = Tree->GetActorTransform().TransformPosition(TreePoint);
		const float Expected = Tree->GradientGuide->GetComponentTransform().InverseTransformPosition(WorldPoint).X / Tree->GradientWidth;
		for (auto* Clump : Tree->GetLeafClumps())
		{
			const FVector P = Clump->GetComponentTransform().InverseTransformPosition(WorldPoint);
			const auto& Data = Clump->GetCustomPrimitiveData().Data;
			if (!TestTrue(TEXT("Clump has all four shared-gradient coefficients"), Data.Num() >= 4)) continue;
			TestTrue(TEXT("One tree-space gradient remains continuous across different clumps"),
				FMath::IsNearlyEqual(float(P.X*Data[0]+P.Y*Data[1]+P.Z*Data[2]+Data[3]), Expected, .0001f));
		}
	};
	CheckRamp(FVector(30, -40, 350));
	Tree->GradientGuide->SetRelativeLocationAndRotation(FVector(20, 30, 280), FRotator(35, 70, 0));
	Tree->GradientGuide->SetRelativeScale3D(FVector(1.3, 1, 1));
	Clumps[0]->SetRelativeLocationAndRotation(FVector(100, -40, 320), FRotator(10, 65, 15));
	Clumps[0]->SetRelativeScale3D(FVector(.7, 1.3, .9));
	CheckRamp(FVector(-40, 15, 305)); // Transform notifications work without manual RefreshTree.
	Tree->SetActorTransform(FTransform(FRotator(15,40,5), FVector(400,-200,50), FVector(1.2)));
	CheckRamp(FVector(35, 20, 360));
	Tree->SetActorScale3D(FVector(.75,1.2,.85));
	CheckRamp(FVector(35, 20, 360));

	auto* Added = NewObject<UStaticMeshComponent>(Tree);
	Tree->AddInstanceComponent(Added); Added->SetupAttachment(Tree->GetRootComponent());
	Added->ComponentTags.Add(TEXT("AnimeTreeClump")); Added->SetStaticMesh(Clumps[0]->GetStaticMesh());
	Added->SetRelativeLocation(FVector(80,80,320)); Added->RegisterComponent(); Tree->RefreshTree();
	TestEqual(TEXT("Adding a clump increases editable count"), Tree->GetLeafClumps().Num(), 11);
	auto ShadowCount = [Tree]()
	{
		TArray<UStaticMeshComponent*> All; Tree->GetComponents(All);
		return All.FilterByPredicate([](auto* C){return C->ComponentHasTag(TEXT("AnimeTreeShadow"));}).Num();
	};
	TestEqual(TEXT("Added clump gets one stable shadow"), ShadowCount(), 11);
	Tree->RefreshTree(); TestEqual(TEXT("Repeated refresh does not leak shadows"), ShadowCount(), 11);
	CheckRamp(FVector(20,10,310));
	Tree->RemoveInstanceComponent(Added); Added->DestroyComponent(); Tree->RefreshTree();
	TestEqual(TEXT("Deleting clump removes orphan shadow"), ShadowCount(), 10);
	Tree->GradientGuide->SetRelativeScale3D(FVector::ZeroVector);
	for (auto* Clump : Tree->GetLeafClumps()) for (float Value : Clump->GetCustomPrimitiveData().Data)
		TestTrue(TEXT("Collapsed guide gives finite shader values"), FMath::IsFinite(Value));
	Tree->RerunConstructionScripts();
	TestEqual(TEXT("Construction recreates the editable canopy"), Tree->GetLeafClumps().Num(), 10);
	TestEqual(TEXT("Construction recreates exactly one shadow per clump"), ShadowCount(), 10);
	Tree->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimeTreeAssetTest, "TunaSweeper.AnimeTree.Assets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAnimeTreeAssetTest::RunTest(const FString& Parameters)
{
	UClass* Class = AnimeTreeTests::TreeClass();
	if (!TestNotNull(TEXT("Placeable tree BP exists"), Class)) return false;
	FAssetCompilingManager::Get().FinishAllCompilation();
	auto* Tree = FAutomationEditorCommonUtils::CreateNewMap()->SpawnActor<ATunaSweeperAnimeTreeActor>(Class);
	TestEqual(TEXT("Requested wind default"), Tree->WindStrength, .75f);
	TestEqual(TEXT("Requested card scale default"), Tree->LeafCardScale, .8f);
	TestEqual(TEXT("Requested density default"), Tree->LeafDensity, .45f);
	TestFalse(TEXT("No per-frame CPU actor work"), Tree->PrimaryActorTick.bCanEverTick);
	if (!TestNotNull(TEXT("Trunk mesh"), Tree->Trunk->GetStaticMesh().Get()) ||
		!TestNotNull(TEXT("Foliage mesh"), AnimeTreeTests::Leaf(Tree)->GetStaticMesh().Get()) ||
		!TestNotNull(TEXT("Leaf material"), Tree->LeafMaterial.Get())) return false;
	TestEqual(TEXT("Shadow uses same card data"), AnimeTreeTests::Shadow(Tree)->GetStaticMesh(), AnimeTreeTests::Leaf(Tree)->GetStaticMesh());
	TestFalse(TEXT("Camera billboards do not cast unstable shadows"), bool(AnimeTreeTests::Leaf(Tree)->CastShadow));
	TestTrue(TEXT("Stable proxy casts shadows"), bool(AnimeTreeTests::Shadow(Tree)->CastShadow));
	TestFalse(TEXT("Shadow proxy is absent from main pass"), bool(AnimeTreeTests::Shadow(Tree)->bRenderInMainPass));
	TestTrue(TEXT("Bounds account for billboard expansion"), AnimeTreeTests::Leaf(Tree)->BoundsScale >= 2.f);
	TestEqual(TEXT("No foliage collision"), AnimeTreeTests::Leaf(Tree)->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	const UMaterial* Material = Tree->LeafMaterial->GetMaterial();
	TestNotNull(TEXT("Leaf color graph survives serialization"), Material->GetEditorOnlyData()->EmissiveColor.Expression);
	TestNotNull(TEXT("Leaf mask graph survives serialization"), Material->GetEditorOnlyData()->OpacityMask.Expression);
	TestNotNull(TEXT("Leaf motion graph survives serialization"), Material->GetEditorOnlyData()->WorldPositionOffset.Expression);
	TestFalse(TEXT("Color graph is active"), bool(Material->GetEditorOnlyData()->EmissiveColor.UseConstant));
	TestFalse(TEXT("Mask graph is active"), bool(Material->GetEditorOnlyData()->OpacityMask.UseConstant));
	AddInfo(FString::Printf(TEXT("Leaf graph has %d expressions"), Material->GetExpressions().Num()));
	TestEqual(TEXT("Leaf silhouette uses masking"), Material->BlendMode, BLEND_Masked);
	TestTrue(TEXT("Cards are double sided"), bool(Material->TwoSided));
	TestEqual(TEXT("Source alpha threshold retained"), Material->OpacityMaskClipValue, .48f);
	TestTrue(TEXT("Canopy shading is independent of camera and sun"), Material->GetShadingModels().HasShadingModel(MSM_Unlit));
	const UStaticMesh* Mesh = AnimeTreeTests::Leaf(Tree)->GetStaticMesh();
	const FMeshDescription* Data = Mesh->GetMeshDescription(0);
	if (!TestNotNull(TEXT("Card source attributes retained"), Data)) return false;
	TestEqual(TEXT("Reusable clump retains 288 source cards"), Data->Triangles().Num(), 576);
	TestTrue(TEXT("Custom pivot values are not quantized to half floats"), Mesh->GetSourceModel(0).BuildSettings.bUseFullPrecisionUVs);
	TestFalse(TEXT("Lightmap generation cannot replace card attribute UVs"), Mesh->GetSourceModel(0).BuildSettings.bGenerateLightmapUVs);
	const auto UV = Data->VertexInstanceAttributes().GetAttributesRef<FVector2f>(TEXT("TextureCoordinate"));
	if (!TestEqual(TEXT("Five UV channels carry atlas, pivot, phase, offsets and density"), UV.GetNumChannels(), 5)) return false;
	const auto Position = Data->GetVertexPositions();
	TSet<int32> Bunches;
	for (FVertexInstanceID I : Data->VertexInstances().GetElementIDs())
	{
		const FVector2f XY = UV.Get(I, 1), ZPhase = UV.Get(I, 2), Offset = UV.Get(I, 3), Card = UV.Get(I, 4);
		const FVector3f P = Position[Data->GetVertexInstanceVertex(I)];
		if (!TestTrue(TEXT("Card pivot reconstructs original rest position"), (FVector3f(XY.X, XY.Y + Offset.X, ZPhase.X + Offset.Y) - P).Size() < .001f)) return false;
		if (!TestTrue(TEXT("Density hash is a valid deterministic threshold"), Card.Y >= 0 && Card.Y < 1)) return false;
		Bunches.Add(FMath::RoundToInt(Card.X));
	}
	TestEqual(TEXT("Clump mesh has a single reusable bunch"), Bunches.Num(), 1);
	TestEqual(TEXT("Trunk topology retained"), Tree->Trunk->GetStaticMesh()->GetMeshDescription(0)->Triangles().Num(), 39876);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimeTreeParametersTest, "TunaSweeper.AnimeTree.Parameters",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAnimeTreeParametersTest::RunTest(const FString& Parameters)
{
	UClass* Class = AnimeTreeTests::TreeClass();
	if (!Class) return false;
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	auto* Tree = World->SpawnActor<ATunaSweeperAnimeTreeActor>(Class);
	auto* Other = World->SpawnActor<ATunaSweeperAnimeTreeActor>(Class);
	if (!Tree || !Other) return false;
	Tree->SetTreeParameters(1.5f, 1.4f, 1.f);
	auto* Leaves = Cast<UMaterialInstanceDynamic>(AnimeTreeTests::Leaf(Tree)->GetMaterial(0));
	auto* Shadow = Cast<UMaterialInstanceDynamic>(AnimeTreeTests::Shadow(Tree)->GetMaterial(0));
	if (!TestNotNull(TEXT("Visible material instance"), Leaves) || !TestNotNull(TEXT("Shadow material instance"), Shadow)) return false;
	for (auto* Material : {Leaves, Shadow})
	{
		TestEqual(TEXT("Upper wind reaches GPU"), Material->K2_GetScalarParameterValue(TEXT("WindStrength")), 1.5f);
		TestEqual(TEXT("Upper card scale reaches GPU"), Material->K2_GetScalarParameterValue(TEXT("LeafCardScale")), 1.4f);
		TestEqual(TEXT("Full canopy reaches GPU"), Material->K2_GetScalarParameterValue(TEXT("LeafDensity")), 1.f);
	}
	TestEqual(TEXT("Visible billboards use camera basis"), Leaves->K2_GetScalarParameterValue(TEXT("StableShadowProxy")), 0.f);
	TestEqual(TEXT("Shadow uses tree basis"), Shadow->K2_GetScalarParameterValue(TEXT("StableShadowProxy")), 1.f);
	FActorSpawnParameters CopyParams;
	CopyParams.Template = Tree;
	auto* Copy = World->SpawnActor<ATunaSweeperAnimeTreeActor>(Class, FTransform::Identity, CopyParams);
	if (!TestNotNull(TEXT("Duplicated tree"), Copy)) return false;
	Copy->SetTreeParameters(.25f, .4f, .2f);
	TestTrue(TEXT("Duplicated tree owns its material"), AnimeTreeTests::Leaf(Copy)->GetMaterial(0)->GetOuter() == Copy);
	TestEqual(TEXT("Changing a duplicate does not change the source"), Leaves->K2_GetScalarParameterValue(TEXT("LeafDensity")), 1.f);
	Tree->SetTreeParameters(-1.f, 0.f, -1.f);
	TestEqual(TEXT("Wind clamps at calm"), Leaves->K2_GetScalarParameterValue(TEXT("WindStrength")), 0.f);
	TestEqual(TEXT("Minimum size is below screenshot default"), Leaves->K2_GetScalarParameterValue(TEXT("LeafCardScale")), .2f);
	TestEqual(TEXT("Zero density supported"), Shadow->K2_GetScalarParameterValue(TEXT("LeafDensity")), 0.f);
	TestEqual(TEXT("Another placed tree retains its defaults"), CastChecked<UMaterialInstanceDynamic>(AnimeTreeTests::Leaf(Other)->GetMaterial(0))->K2_GetScalarParameterValue(TEXT("LeafDensity")), .45f);
	Tree->Destroy(); Other->Destroy(); Copy->Destroy();
	return true;
}

class FAnimeTreeCaptureCommand : public IAutomationLatentCommand
{
public:
	FAutomationTestBase* Test;
	UWorld* World;
	ATunaSweeperAnimeTreeActor* Tree;
	ADirectionalLight* Light;
	AStaticMeshActor* Ground;
	USceneCaptureComponent2D* Capture;
	int32 View = 0;
	double ReadyAt = 0;
	TArray<int32> GreenPixels;
	TArray<uint8> WindMask, CalmMask;
	TArray<FColor> ForwardGradient;
	TArray<FColor> RimReference;
	FString Directory = FPaths::ProjectSavedDir()/TEXT("AnimeTreeQA");

	virtual bool Update() override
	{
		if (ReadyAt == 0)
		{
			const float Angle = View == 1 ? 90.f : View == 2 ? 180.f : 0.f;
			Tree->SetTreeParameters(View == 7 || View == 8 ? 0.f : View == 4 ? 1.5f : .75f, View == 3 ? .2f : View == 4 ? 1.4f : .8f, View == 3 ? .15f : View == 4 ? 1.f : View == 5 ? 0.f : .45f);
			if (View == 9) Tree->SetActorTransform(FTransform(FRotator(10,45,5), FVector(30,20,0), FVector(.75,1.2,.85)));
			const FVector Focus(0,0,240);
			if (View >= 10) { Tree->SetActorTransform(FTransform::Identity); Tree->SetTreeParameters(0,.8f,.45f); }
            if (View == 11) { Tree->GradientGuide->SetRelativeRotation(FRotator(0,0,0)); Tree->GradientWidth=280; Tree->GradientStrength=1; Tree->ClumpShadingStrength=0; Tree->RefreshTree(); }
            if (View == 12) Tree->GradientGuide->SetRelativeRotation(FRotator(0,180,0));
            if (View == 13) { Tree->GradientStrength=0; Tree->RefreshTree(); }
            if (View == 14) { Tree->GradientStrength=.65f; Tree->ClumpShadingStrength=.7f; Tree->GradientWidth=400; Tree->GradientGuide->SetRelativeRotation(FVector(.32,-.54,.78).Rotation()); Tree->GetLeafClumps()[0]->SetRelativeLocation(FVector(100,-90,380)); Tree->RefreshTree(); }
			if (View == 15) Tree->RerunConstructionScripts();
			if (View >= 15) { Tree->RimStrength = View == 15 || View == 17 ? 0.f : 1.f; Tree->RefreshTree(); }
			const FVector ViewFocus = View >= 15 ? FVector(0,0,340) : Focus;
            const FVector Camera = ViewFocus + (View >= 17 ? FVector(1050,0,520) : View >= 10 ? FVector(0,0,1200) : FRotator(0,Angle,0).RotateVector(FVector(1050,0,520)));
			Capture->SetWorldLocationAndRotation(Camera,(ViewFocus-Camera).Rotation());
			// Let real renderer frames initialize GPU scene/material uniform buffers.
			ReadyAt = FPlatformTime::Seconds() + .6;
			return false;
		}
		if (FPlatformTime::Seconds() < ReadyAt) return false;
		World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
		Capture->CaptureScene(); FlushRenderingCommands();
		FImage Pixels;
		if (Test->TestTrue(TEXT("Tree render readable"), FImageUtils::GetRenderTargetImage(Capture->TextureTarget, Pixels)))
		{
			int32 Green = 0;
			TArray<uint8> Mask;
			for (const FColor& P : Pixels.AsBGRA8())
			{
				const bool bLeaf = P.G > 20 && P.G > P.R * 1.15f && P.G > P.B * 1.05f;
				Green += bLeaf; Mask.Add(bLeaf);
			}
			if (View == 0) WindMask = Mask;
			if (View == 7) CalmMask = Mask;
			if (View == 11) ForwardGradient = TArray<FColor>(Pixels.AsBGRA8());
			if (View == 15 || View == 17) RimReference = TArray<FColor>(Pixels.AsBGRA8());
			if ((View == 16 || View == 18) && RimReference.Num() == Mask.Num())
			{
				int32 Changed = 0, CentralChanged = 0, CentralLeaves = 0;
				for (int32 I = 0; I < Mask.Num(); ++I)
				{
					const FColor A = RimReference[I], B = Pixels.AsBGRA8()[I];
					if (!(A.G > 20 && A.G > A.R*1.15f && A.G > A.B*1.05f)) continue;
					const bool bChanged = FMath::Abs(int32(A.R)-B.R)+FMath::Abs(int32(A.G)-B.G)+FMath::Abs(int32(A.B)-B.B) > 12;
					Changed += bChanged;
					if (FMath::Abs(I%Pixels.SizeX-Pixels.SizeX/2) < 55 && FMath::Abs(I/Pixels.SizeX-Pixels.SizeY/2) < 55)
					{ ++CentralLeaves; CentralChanged += bChanged; }
				}
				Test->AddInfo(FString::Printf(TEXT("Rim view %d: %d changed leaf pixels; central %d/%d"), View, Changed, CentralChanged, CentralLeaves));
				Test->TestTrue(TEXT("Rim paints the visible outer canopy"), Changed > 500);
				Test->TestTrue(TEXT("Rim leaves the common crown center unchanged"), CentralLeaves > 100 && CentralChanged < CentralLeaves*.05f);
			}
			if (View == 12 && ForwardGradient.Num() == Mask.Num())
			{
				double ReversedSlope = 0;
				for (int32 I = 0; I < Mask.Num(); ++I) if (Mask[I])
					ReversedSlope += (int32(Pixels.AsBGRA8()[I].G) - int32(ForwardGradient[I].G)) * (I / Pixels.SizeX - Pixels.SizeY / 2);
				Test->TestTrue(TEXT("Rotating the common guide reverses the whole rendered ramp"), ReversedSlope > 100000);
			}
			if (View == 6 || View == 8)
			{
				const TArray<uint8>& Before = View == 6 ? WindMask : CalmMask;
				int32 Changed = 0;
				for (int32 I = 0; I < Mask.Num(); ++I) Changed += Before[I] != Mask[I];
				Test->AddInfo(FString::Printf(TEXT("View %d: %d changed leaf pixels over time"), View, Changed));
				if (View == 6) Test->TestTrue(TEXT("Default wind animates the rendered leaves over time"), Changed > 50);
				else Test->TestEqual(TEXT("Zero wind keeps leaf geometry still over time"), Changed, 0);
			}
			GreenPixels.Add(Green);
			Test->AddInfo(FString::Printf(TEXT("View %d: %d green leaf pixels"), View, Green));
			if (View < 3 || View >= 6) Test->TestTrue(TEXT("Colored alpha-masked canopy visible across views/transforms"), Green > 5000);
			IFileManager::Get().MakeDirectory(*Directory, true);
			Test->TestTrue(TEXT("Tree render saved"), FImageUtils::SaveImageByExtension(*(Directory/FString::Printf(TEXT("tree_%d.png"),View)), Pixels));
		}
		ReadyAt = 0;
		if (++View < 19) return false;
		if (GreenPixels.Num() == 19)
		{
			Test->TestTrue(TEXT("Larger, denser canopy increases visible leaf coverage"), GreenPixels[4] > GreenPixels[3] + 5000);
			Test->TestEqual(TEXT("Zero density hides all leaves in rendered image"), GreenPixels[5], 0);
		}
		Capture->DestroyComponent(); Tree->Destroy(); Light->Destroy(); Ground->Destroy();
		return true;
	}
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAnimeTreeRenderTest, "TunaSweeper.AnimeTree.Rendering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAnimeTreeRenderTest::RunTest(const FString& Parameters)
{
	if (!FApp::CanEverRender()) { AddInfo(TEXT("Render capture requires a real RHI; data tests remain available under NullRHI.")); return true; }
	UClass* Class = AnimeTreeTests::TreeClass();
	if (!TestNotNull(TEXT("Tree BP"), Class)) return false;
	UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
	auto* Tree = World->SpawnActor<ATunaSweeperAnimeTreeActor>(Class);
	auto* Light = World->SpawnActor<ADirectionalLight>();
	Light->SetActorRotation(FRotator(-50, 121, 0));
	Light->GetLightComponent()->SetIntensity(4.f);
	auto* Ground = World->SpawnActor<AStaticMeshActor>();
	Ground->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
	Ground->SetActorScale3D(FVector(200, 200, 1)); Ground->SetActorLocation(FVector(0,0,-2));
	auto* Capture = NewObject<USceneCaptureComponent2D>(Tree);
	Tree->AddInstanceComponent(Capture);
	auto* Target = NewObject<UTextureRenderTarget2D>();
	Target->InitCustomFormat(1024,1024,PF_B8G8R8A8,false);
	// The sRGB render target handles encoding; avoid a second gamma-only pass.
	Target->TargetGamma = 1.0f;
	Capture->TextureTarget = Target; Capture->CaptureSource = SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
	Capture->FOVAngle = 40; Capture->RegisterComponentWithWorld(World);
	Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
	Capture->PostProcessSettings.AutoExposureMethod = AEM_Manual;
	Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
	Capture->PostProcessSettings.AutoExposureBias = 0;
	Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
	Capture->ShowFlags.SetTonemapper(false);
	Capture->ShowFlags.SetEyeAdaptation(false);
	Capture->ShowFlags.SetAntiAliasing(false);
	FAssetCompilingManager::Get().FinishAllCompilation();
	IStreamingManager::Get().StreamAllResources(10.f);
	const FMaterialResource* LeafResource = Tree->LeafMaterial->GetMaterialResource(GMaxRHIShaderPlatform);
	if (!TestNotNull(TEXT("Leaf GPU material"), LeafResource) || !TestNotNull(TEXT("Leaf shader compiled"), LeafResource->GetGameThreadShaderMap())) return false;
	AddInfo(FString::Printf(TEXT("Compiled leaf UV scalars: %u"), LeafResource->GetGameThreadShaderMap()->GetNumUsedUVScalars()));
	auto Command = MakeShared<FAnimeTreeCaptureCommand>();
	Command->Test = this; Command->World = World; Command->Tree = Tree;
	Command->Light = Light; Command->Ground = Ground; Command->Capture = Capture;
	FAutomationTestFramework::Get().EnqueueLatentCommand(Command);
	return true;
}
#endif
