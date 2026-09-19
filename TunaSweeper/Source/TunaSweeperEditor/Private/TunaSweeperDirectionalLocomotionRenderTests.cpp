#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/DirectionalLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "AssetCompilingManager.h"
#include "ContentStreaming.h"
#include "RenderingThread.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDirectionalLocomotionRenderTest,
    "TunaSweeper.Player.DirectionalPoseCapture", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDirectionalLocomotionRenderTest::RunTest(const FString& Parameters)
{
    UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
    UClass* Class = LoadClass<ACharacter>(nullptr, TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
    auto* Player = World->SpawnActor<ACharacter>(Class, FVector(0,0,65), FRotator::ZeroRotator);
    if (!Player) return false;
    auto* Mesh = Player->GetMesh();
    Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Mesh->bEnableUpdateRateOptimizations = false;
    auto* Light = World->SpawnActor<ADirectionalLight>(FVector(0,0,400), FRotator(-40,140,0));
    Light->GetLightComponent()->SetIntensity(5);
    auto* Fill = World->SpawnActor<ADirectionalLight>(FVector(0,0,400), FRotator(-25,-40,0));
    Fill->GetLightComponent()->SetIntensity(2);
    auto* Capture = NewObject<USceneCaptureComponent2D>(Player);
    auto* Target = NewObject<UTextureRenderTarget2D>();
    Target->InitCustomFormat(640,640,PF_B8G8R8A8,false);
    Capture->TextureTarget = Target; Capture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
    Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
    Capture->RegisterComponentWithWorld(World);
    const FVector Camera(310,-270,210), Aim(0,0,65);
    Capture->SetWorldLocationAndRotation(Camera, (Aim-Camera).Rotation()); Capture->FOVAngle = 35;
    Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
    Capture->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
    Capture->PostProcessSettings.AutoExposureBias = 1;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    FAssetCompilingManager::Get().FinishAllCompilation(); IStreamingManager::Get().StreamAllResources(5.f);
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("DirectionalLocomotion/Capture");
    IFileManager::Get().MakeDirectory(*Directory, true);
    for (const TCHAR* Name : {TEXT("AS_Luna_WalkBackward"), TEXT("AS_Luna_StrafeLeft"), TEXT("AS_Luna_StrafeRight")})
    {
        auto* Clip = LoadObject<UAnimSequence>(nullptr, *(FString(TEXT("/Game/Characters/Player/LunaMk2/Animations/")) + Name));
        if (!Clip) return false;
        Mesh->PlayAnimation(Clip, false); Mesh->SetPlayRate(0);
        FVector FirstFoot;
        for (int32 Frame = 0; Frame < 4; ++Frame)
        {
            ++GFrameCounter;
            Mesh->SetPosition(Frame * .2f, false); Mesh->TickAnimation(0, false); Mesh->RefreshBoneTransforms();
            const FVector Foot = Mesh->GetSocketLocation(TEXT("foot_l"));
            if (Frame == 0) FirstFoot = Foot;
            if (Frame == 2) TestTrue(TEXT("Evaluated clip moves the foot through its stride"), FVector::Distance(Foot, FirstFoot) > 10);
            for (auto* Component : TInlineComponentArray<USkeletalMeshComponent*>(Player))
                if (Component != Mesh) { Component->TickAnimation(0, false); Component->RefreshBoneTransforms(); }
            Mesh->MarkRenderDynamicDataDirty();
            World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
            for (int32 Warmup = 0; Warmup < 8; ++Warmup) { Capture->CaptureScene(); FlushRenderingCommands(); }
            FImage Pixels;
            if (TestTrue(TEXT("Pose render is readable"), FImageUtils::GetRenderTargetImage(Target, Pixels)))
                TestTrue(TEXT("Pose render saved"), FImageUtils::SaveImageByExtension(*(Directory / FString::Printf(TEXT("%s_%d.png"), Name, Frame)), Pixels));
            AddInfo(FString::Printf(TEXT("%s phase %d foot %s"), Name, Frame, *Foot.ToString()));
        }
    }
    Capture->DestroyComponent(); Player->Destroy(); Light->Destroy(); Fill->Destroy();
    return !HasAnyErrors();
}
#endif
