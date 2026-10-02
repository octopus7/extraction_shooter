#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "LoopRailTrain.h"
#include "LoopRailTrack.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"
#include "ImageUtils.h"
#include "AssetCompilingManager.h"
#include "ShaderCompiler.h"
#include "MaterialShared.h"
#include "ContentStreaming.h"
#include "RenderingThread.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoopRailRenderTest,"LoopRail.Visual.Blockout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLoopRailRenderTest::RunTest(const FString& Parameters)
{
    if (!FApp::CanEverRender()) { AddInfo(TEXT("Visual capture requires an RHI; runtime tests run separately with NullRHI.")); return true; }
    UWorld* World=FAutomationEditorCommonUtils::CreateNewMap();
    auto* Train=World->SpawnActor<ALoopRailTrain>();
    auto* Ground=World->SpawnActor<AStaticMeshActor>();
    Ground->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    Ground->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/LoopRail/Materials/M_PreviewGround.M_PreviewGround")));
    Ground->SetActorLocation(FVector(-1470,0,-75)); Ground->SetActorScale3D(FVector(65,35,1));
    auto* Sun=World->SpawnActor<ADirectionalLight>(FVector(0,0,1000),FRotator(-50,-40,0)); Sun->GetLightComponent()->SetIntensity(5);
    auto* Fill=World->SpawnActor<ADirectionalLight>(FVector(0,0,1000),FRotator(-30,145,0)); Fill->GetLightComponent()->SetIntensity(2);
    auto* Capture=NewObject<USceneCaptureComponent2D>(Train);
    auto* Target=NewObject<UTextureRenderTarget2D>(); Target->InitCustomFormat(1440,900,PF_B8G8R8A8,false);
    Capture->TextureTarget=Target; Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame=false; Capture->bCaptureOnMovement=false;
    Capture->RegisterComponentWithWorld(World); Capture->ProjectionType=ECameraProjectionMode::Orthographic;
    Capture->PostProcessSettings.bOverride_AutoExposureMethod=true; Capture->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true; Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
    Capture->PostProcessSettings.bOverride_AutoExposureBias=true; Capture->PostProcessSettings.AutoExposureBias=1;
    FAssetCompilingManager::Get().FinishAllCompilation(); IStreamingManager::Get().StreamAllResources(5);
    const FString Dir=FPaths::ProjectSavedDir()/TEXT("LoopRail/Preview"); IFileManager::Get().MakeDirectory(*Dir,true);
    auto Shot=[&](const TCHAR* Name,FVector Aim,FVector Offset,float Width)
    {
        // Newly enabled optional meshes may still have deferred shader jobs, even after asset compilation.
        UMaterialInterface::SubmitRemainingJobsForWorld(World);
        if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
        IStreamingManager::Get().StreamAllResources(5);
        Capture->SetWorldLocationAndRotation(Aim+Offset,(-Offset).Rotation()); Capture->OrthoWidth=Width;
        ++GFrameCounter; World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
        for(int32 I=0;I<8;++I) { Capture->CaptureScene(); FlushRenderingCommands(); }
        FImage Image;
        if(TestTrue(TEXT("Read rendered train preview"),FImageUtils::GetRenderTargetImage(Target,Image)))
            TestTrue(TEXT("Write preview image"),FImageUtils::SaveImageByExtension(*(Dir/(FString(Name)+TEXT(".png"))),Image));
    };
    Shot(TEXT("Train"),FVector(-1470,0,80),FVector(1900,-3100,2900),5000);
    Shot(TEXT("Carriage"),Train->GetVehicleFloor(1)->GetComponentLocation()+FVector(0,0,50),FVector(650,-1000,1400),1250);
    Shot(TEXT("Locomotive"),FVector(0,0,130),FVector(750,-1000,850),1300);
    Shot(TEXT("Connection"),Train->GetConnectionFloor(1)->GetComponentLocation()+FVector(0,0,20),FVector(400,-650,750),650);
    Train->bShowCarriageRoofs=true; Train->RebuildTrain();
    FAssetCompilingManager::Get().FinishAllCompilation(); IStreamingManager::Get().StreamAllResources(5);
    Shot(TEXT("Roof"),Train->GetVehicleFloor(1)->GetComponentLocation()+FVector(0,0,100),FVector(650,-1000,1400),1250);
    UMaterialInterface* RoofMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/LoopRail/Materials/M_CarriageRoofDetail.M_CarriageRoofDetail"));
    const FMaterialResource* Resource=RoofMaterial ? RoofMaterial->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
    TestTrue(TEXT("Optional roof renders with its compiled surface material"),Resource && Resource->IsCompilationFinished() && Resource->GetGameThreadShaderMap());
    auto* Track=World->SpawnActor<ALoopRailTrack>();
    Train->bShowCarriageRoofs=false; Train->Track=Track; Train->StartDistance=12000; Train->RebuildTrain(); Train->ResetTrain();
    Shot(TEXT("ConnectionCurve"),Train->GetConnectionFloor(1)->GetComponentLocation()+FVector(0,0,30),FVector(400,-650,850),800);
    Capture->DestroyComponent();
    return true;
}
#endif
