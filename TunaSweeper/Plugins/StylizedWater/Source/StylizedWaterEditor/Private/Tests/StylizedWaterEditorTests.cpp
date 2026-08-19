#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetCompilingManager.h"
#include "Components/BoxComponent.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionSingleLayerWaterMaterialOutput.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialInstanceConstant.h"
#include "ProceduralMeshComponent.h"
#include "RHI.h"
#include "StylizedWaterBodyActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStylizedWaterGeneratedAssetsTest,
	"StylizedWater.Editor.GeneratedAssetsAndBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStylizedWaterGeneratedAssetsTest::RunTest(const FString& Parameters)
{
	UTexture2D* DepthGradientTexture = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/StylizedWater/Generated/Internal/T_WaterDepthGradient.T_WaterDepthGradient"));
	UMaterial* Material = LoadObject<UMaterial>(
		nullptr,
		TEXT("/StylizedWater/Generated/Internal/M_StylizedWaterSurface.M_StylizedWaterSurface"));
	UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(
		nullptr,
		TEXT("/StylizedWater/Generated/Internal/MI_StylizedWater_CalmAnime.MI_StylizedWater_CalmAnime"));
	UBlueprint* Blueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/StylizedWater/Generated/Internal/BP_StylizedWaterBody_Internal.BP_StylizedWaterBody_Internal"));

	TestNotNull(TEXT("Generated depth gradient texture exists"), DepthGradientTexture);
	TestNotNull(TEXT("Generated surface material exists"), Material);
	TestNotNull(TEXT("Generated material instance exists"), MaterialInstance);
	TestNotNull(TEXT("Generated internal Blueprint exists"), Blueprint);
	if (!DepthGradientTexture || !Material || !MaterialInstance || !Blueprint)
	{
		return false;
	}
	TestEqual(TEXT("Depth gradient texture is 256 pixels wide"), DepthGradientTexture->Source.GetSizeX(), static_cast<int64>(256));
	TestEqual(TEXT("Depth gradient texture is one pixel high"), DepthGradientTexture->Source.GetSizeY(), static_cast<int64>(1));

	TestEqual(TEXT("Surface material uses masked blending"), Material->BlendMode, BLEND_Masked);
	TestTrue(TEXT("Surface material uses Single Layer Water"), Material->GetShadingModels().HasShadingModel(MSM_SingleLayerWater));
	UMaterialExpressionSingleLayerWaterMaterialOutput* WaterOutput = nullptr;
	UMaterialExpressionTextureSampleParameter2D* DepthGradientSample = nullptr;
	for (UMaterialExpression* Expression : Material->GetExpressions())
	{
		if (UMaterialExpressionSingleLayerWaterMaterialOutput* Candidate = Cast<UMaterialExpressionSingleLayerWaterMaterialOutput>(Expression))
		{
			WaterOutput = Candidate;
		}
		if (UMaterialExpressionTextureSampleParameter2D* Candidate = Cast<UMaterialExpressionTextureSampleParameter2D>(Expression))
		{
			if (Candidate->ParameterName == TEXT("DepthGradientTexture"))
			{
				DepthGradientSample = Candidate;
			}
		}
	}
	TestNotNull(TEXT("Surface material has a Single Layer Water output"), WaterOutput);
	TestNotNull(TEXT("Surface material samples the generated depth gradient"), DepthGradientSample);
	TestTrue(
		TEXT("Depth gradient sample uses the generated texture"),
		DepthGradientSample && DepthGradientSample->Texture == DepthGradientTexture);
	TestTrue(
		TEXT("Depth style color drives the behind-water composite tint"),
		WaterOutput && Cast<UMaterialExpressionComponentMask>(WaterOutput->ColorScaleBehindWater.Expression) != nullptr);
	TestTrue(TEXT("Material instance parent is the generated material"), MaterialInstance->Parent == Material);
	TestTrue(
		TEXT("Internal Blueprint derives from the native water body"),
		Blueprint->GeneratedClass && Blueprint->GeneratedClass->IsChildOf(AStylizedWaterBodyActor::StaticClass()));
	TestEqual(TEXT("Internal Blueprint is compiled"), Blueprint->Status, BS_UpToDate);

	FAssetCompilingManager::Get().FinishAllCompilation();
	FMaterialResource* MaterialResource = Material->GetMaterialResource(GMaxRHIShaderPlatform);
	TestNotNull(TEXT("Surface material has a resource for the active shader platform"), MaterialResource);
	if (MaterialResource)
	{
		for (const FString& CompileError : MaterialResource->GetCompileErrors())
		{
			AddError(FString::Printf(TEXT("Surface material compile error: %s"), *CompileError));
		}
	}

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	TestNotNull(TEXT("Editor world is available"), World);
	if (!World || !Blueprint->GeneratedClass)
	{
		return false;
	}

	const FTransform WaterTransform(FRotator::ZeroRotator, FVector(1000000.0, 1000000.0, 1000000.0));
	AStylizedWaterBodyActor* WaterBody = World->SpawnActorDeferred<AStylizedWaterBodyActor>(
		Blueprint->GeneratedClass,
		WaterTransform,
		nullptr,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
	TestNotNull(TEXT("Internal Blueprint can spawn a water body"), WaterBody);
	if (!WaterBody)
	{
		return false;
	}
	WaterBody->bSampleTerrainOnRebuild = false;
	WaterBody->FinishSpawning(WaterTransform);

	WaterBody->ApplyPreset(EStylizedWaterPreset::GentleBeach, false);

	auto SpawnDepthFloor = [World](const FVector& Location) -> AActor*
	{
		FActorSpawnParameters FloorSpawnParameters;
		FloorSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		FloorSpawnParameters.ObjectFlags |= RF_Transient;
		AActor* FloorActor = World->SpawnActor<AActor>(AActor::StaticClass(), Location, FRotator::ZeroRotator, FloorSpawnParameters);
		if (!FloorActor)
		{
			return nullptr;
		}

		UBoxComponent* FloorCollision = NewObject<UBoxComponent>(FloorActor, TEXT("DepthFloor"));
		FloorActor->SetRootComponent(FloorCollision);
		FloorCollision->SetBoxExtent(FVector(1100.0, 2100.0, 10.0));
		FloorCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		FloorCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		FloorCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		FloorCollision->RegisterComponent();
		FloorActor->SetActorLocation(Location);
		return FloorActor;
	};

	const FVector WaterLocation = WaterBody->GetActorLocation();
	AActor* ShallowFloor = SpawnDepthFloor(WaterLocation + FVector(-1000.0, 0.0, -110.0));
	AActor* DeepFloor = SpawnDepthFloor(WaterLocation + FVector(1000.0, 0.0, -810.0));
	TestNotNull(TEXT("Shallow depth test floor spawned"), ShallowFloor);
	TestNotNull(TEXT("Deep depth test floor spawned"), DeepFloor);
	WaterBody->bSampleTerrainOnRebuild = true;
	WaterBody->RebuildAndBakeDepth();
	TestTrue(TEXT("Depth bake result reports successful trace hits"), WaterBody->LastDepthBakeResult.Contains(TEXT("hits")));
	TestNotNull(TEXT("Water body owns a procedural surface"), WaterBody->WaterSurface.Get());
	if (WaterBody->WaterSurface)
	{
		const FProcMeshSection* Section = WaterBody->WaterSurface->GetProcMeshSection(0);
		TestNotNull(TEXT("Water body creates mesh section zero"), Section);
		if (Section)
		{
			const int32 ExpectedVertexCount = (WaterBody->GridResolution.X + 1) * (WaterBody->GridResolution.Y + 1);
			TestEqual(TEXT("Generated grid vertex count matches the selected preset"), Section->ProcVertexBuffer.Num(), ExpectedVertexCount);
			TestEqual(
				TEXT("Generated grid triangle index count matches the selected preset"),
				Section->ProcIndexBuffer.Num(),
				WaterBody->GridResolution.X * WaterBody->GridResolution.Y * 6);

			uint8 MinimumEncodedDepth = MAX_uint8;
			uint8 MaximumEncodedDepth = 0;
			for (const FProcMeshVertex& Vertex : Section->ProcVertexBuffer)
			{
				MinimumEncodedDepth = FMath::Min(MinimumEncodedDepth, Vertex.Color.R);
				MaximumEncodedDepth = FMath::Max(MaximumEncodedDepth, Vertex.Color.R);
			}
			TestTrue(
				TEXT("Terrain depth bake produces visibly different encoded depths"),
				static_cast<int32>(MaximumEncodedDepth) - static_cast<int32>(MinimumEncodedDepth) > 50);
		}
		TestNotNull(TEXT("Generated water body connects its material automatically"), WaterBody->WaterSurface->GetMaterial(0));
	}

	if (ShallowFloor)
	{
		ShallowFloor->Destroy();
	}
	if (DeepFloor)
	{
		DeepFloor->Destroy();
	}
	WaterBody->Destroy();
	return !HasAnyErrors();
}

#endif
