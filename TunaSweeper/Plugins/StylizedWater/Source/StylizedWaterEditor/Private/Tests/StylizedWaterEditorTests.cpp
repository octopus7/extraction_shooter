#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetCompilingManager.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "MaterialShared.h"
#include "Materials/Material.h"
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
	UMaterial* Material = LoadObject<UMaterial>(
		nullptr,
		TEXT("/StylizedWater/Generated/Internal/M_StylizedWaterSurface.M_StylizedWaterSurface"));
	UMaterialInstanceConstant* MaterialInstance = LoadObject<UMaterialInstanceConstant>(
		nullptr,
		TEXT("/StylizedWater/Generated/Internal/MI_StylizedWater_CalmAnime.MI_StylizedWater_CalmAnime"));
	UBlueprint* Blueprint = LoadObject<UBlueprint>(
		nullptr,
		TEXT("/StylizedWater/Generated/Internal/BP_StylizedWaterBody_Internal.BP_StylizedWaterBody_Internal"));

	TestNotNull(TEXT("Generated surface material exists"), Material);
	TestNotNull(TEXT("Generated material instance exists"), MaterialInstance);
	TestNotNull(TEXT("Generated internal Blueprint exists"), Blueprint);
	if (!Material || !MaterialInstance || !Blueprint)
	{
		return false;
	}

	TestEqual(TEXT("Surface material uses masked blending"), Material->BlendMode, BLEND_Masked);
	TestTrue(TEXT("Surface material uses Single Layer Water"), Material->GetShadingModels().HasShadingModel(MSM_SingleLayerWater));
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

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;
	AStylizedWaterBodyActor* WaterBody = World->SpawnActor<AStylizedWaterBodyActor>(
		Blueprint->GeneratedClass,
		FVector(1000000.0, 1000000.0, 1000000.0),
		FRotator::ZeroRotator,
		SpawnParameters);
	TestNotNull(TEXT("Internal Blueprint can spawn a water body"), WaterBody);
	if (!WaterBody)
	{
		return false;
	}

	WaterBody->ApplyPreset(EStylizedWaterPreset::GentleBeach, false);
	WaterBody->RebuildWithoutTerrainTrace();
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
		}
		TestNotNull(TEXT("Generated water body connects its material automatically"), WaterBody->WaterSurface->GetMaterial(0));
	}

	WaterBody->Destroy();
	return !HasAnyErrors();
}

#endif
