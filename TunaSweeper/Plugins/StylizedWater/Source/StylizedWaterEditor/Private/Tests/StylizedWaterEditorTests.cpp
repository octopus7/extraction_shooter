#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "StylizedWaterBodyActor.h"
#include "Tests/AutomationEditorCommon.h"
#include "AssetCompilingManager.h"
#include "MaterialShared.h"
#include "RHI.h"
#include "ShaderCompiler.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaskWaterAssetsTest,"StylizedWater.MaskWater.AssetsAndTopology",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMaskWaterAssetsTest::RunTest(const FString& Parameters)
{
    FAssetCompilingManager::Get().FinishAllCompilation();
    for(const TCHAR* Path:{TEXT("/StylizedWater/MaskWater/M_WaterMask.M_WaterMask"),TEXT("/StylizedWater/SkyParallax/M_WaterMaskSky.M_WaterMaskSky")})
    {
        auto* M=LoadObject<UMaterial>(nullptr,Path);
        if(!TestNotNull(TEXT("Saved material loads"),M)) return false;
        TestEqual(TEXT("Continuous translucent blend"),M->BlendMode,BLEND_Translucent);
        TestTrue(TEXT("Predictable painted shading"),M->GetShadingModels().HasShadingModel(MSM_Unlit));
        M->ForceRecompileForRendering(EMaterialShaderPrecompileMode::Synchronous);
        FAssetCompilingManager::Get().FinishAllCompilation();
        GShaderCompilingManager->FinishAllCompilation();
        auto* Resource=M->GetMaterialResource(GMaxRHIShaderPlatform);
        if(TestNotNull(TEXT("Material resource exists"),Resource))
        {
            TestEqual(TEXT("Material has no shader errors"),Resource->GetCompileErrors().Num(),0);
            TestTrue(TEXT("Material shader map is complete"),Resource->IsGameThreadShaderMapComplete());
        }
    }
    auto* World=FAutomationEditorCommonUtils::CreateNewMap();
    auto* Body=World->SpawnActor<AStylizedWaterBodyActor>();
    for(auto Preset:{EStylizedWaterPreset::CalmLake,EStylizedWaterPreset::GentleBeach,EStylizedWaterPreset::FlowingRiver})
    {
        Body->ApplyPreset(Preset);
        TestNotNull(TEXT("Preset mask"),Body->BoundaryMask.Get());
        if(Body->BoundaryMask) { TestFalse(TEXT("Mask is linear data"),Body->BoundaryMask->SRGB); TestEqual(TEXT("Mask source width"),Body->BoundaryMask->Source.GetSizeX(),int64(1024)); }
        Body->GridResolution=FIntPoint(2,2); Body->RebuildSurface();
        auto* Section=Body->WaterSurface->GetProcMeshSection(0);
        if(TestNotNull(TEXT("Surface exists"),Section)) TestEqual(TEXT("No shore triangle culling even at 2x2"),Section->ProcIndexBuffer.Num(),24);
    }
    Body->bEnableSkyParallax=false; Body->RebuildSurface();
    auto* Base=Cast<UMaterialInstanceDynamic>(Body->WaterSurface->GetMaterial(0));
    TestTrue(TEXT("Off selects base-only material"),Base && Base->Parent->GetPathName().Contains(TEXT("/MaskWater/")));
    Body->bEnableSkyParallax=true; Body->RebuildSurface();
    auto* Sky=Cast<UMaterialInstanceDynamic>(Body->WaterSurface->GetMaterial(0));
    TestTrue(TEXT("On selects isolated sky material"),Sky && Sky->Parent->GetPathName().Contains(TEXT("/SkyParallax/")));
    Body->Destroy();
    return true;
}
#endif
