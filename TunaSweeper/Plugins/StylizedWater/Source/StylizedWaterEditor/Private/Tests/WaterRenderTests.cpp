#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Tests/AutomationEditorCommon.h"
#include "AssetCompilingManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "StylizedWaterBodyActor.h"

namespace WaterReview
{
struct FScene
{
    UWorld* World=nullptr;
    AStaticMeshActor* Ground=nullptr;
    AStylizedWaterBodyActor* Water=nullptr;
    ASceneCapture2D* Camera=nullptr;
    UTextureRenderTarget2D* Target=nullptr;
    TMap<FString,FImage> Images;
    void Shot(FAutomationTestBase* Test,const FString& Name)
    {
        Camera->GetCaptureComponent2D()->CaptureScene();
        FlushRenderingCommands();
        FImage Raw;
        if(!Test->TestTrue(TEXT("Read UE render"),FImageUtils::GetRenderTargetImage(Target,Raw))) return;
        FImage Image; Raw.CopyTo(Image,ERawImageFormat::BGRA8,EGammaSpace::sRGB);
        uint8 MinBlue=255,MaxBlue=0;
        int64 BlueWaterPixels=0;
        for(auto& Pixel:Image.AsBGRA8())
        {
            Pixel.A=255; MinBlue=FMath::Min(MinBlue,Pixel.B); MaxBlue=FMath::Max(MaxBlue,Pixel.B);
            if(Pixel.B>Pixel.R+15 && Pixel.G>Pixel.R+10) ++BlueWaterPixels;
        }
        Test->TestTrue(TEXT("Capture contains blue water"),BlueWaterPixels>1000);
        const FString Folder=FPaths::ProjectSavedDir()/TEXT("WaterRebuild/Renders");
        IFileManager::Get().MakeDirectory(*Folder,true);
        Test->TestTrue(TEXT("Save rendered PNG"),FImageUtils::SaveImageByExtension(*(Folder/(Name+TEXT(".png"))),Image));
        Test->AddInfo(TEXT("Rendered ")+Name);
        Images.Add(Name,MoveTemp(Image));
    }
    double Difference(const TCHAR* A,const TCHAR* B) const
    {
        const auto* First=Images.Find(A); const auto* Second=Images.Find(B);
        if(!First || !Second || First->RawData.Num()!=Second->RawData.Num() || First->RawData.IsEmpty()) return -1;
        int64 Sum=0;
        for(int64 I=0;I<First->RawData.Num();++I) Sum+=FMath::Abs(int32(First->RawData[I])-int32(Second->RawData[I]));
        return double(Sum)/First->RawData.Num();
    }
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMaskWaterRenderTest,"StylizedWater.MaskWater.RenderMatrix",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FMaskWaterRenderTest::RunTest(const FString& Parameters)
{
    auto Scene=MakeShared<WaterReview::FScene>();
    Scene->World=FAutomationEditorCommonUtils::CreateNewMap();
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    auto* GroundMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/StylizedWater/Review/M_ReviewGround.M_ReviewGround"));
    if(!TestNotNull(TEXT("Review ground material"),GroundMaterial)) return false;
    Scene->Ground=Scene->World->SpawnActor<AStaticMeshActor>();
    Scene->Ground->GetStaticMeshComponent()->SetStaticMesh(Cube);
    Scene->Ground->GetStaticMeshComponent()->SetMaterial(0,GroundMaterial);
    Scene->Ground->SetActorScale3D(FVector(500,500,1));
    Scene->Ground->SetActorLocation(FVector(0,0,-220));
    Scene->Water=Scene->World->SpawnActor<AStylizedWaterBodyActor>();
    Scene->Camera=Scene->World->SpawnActor<ASceneCapture2D>();
    auto* Capture=Scene->Camera->GetCaptureComponent2D();
    Capture->bCaptureEveryFrame=false; Capture->bCaptureOnMovement=false;
    Capture->CaptureSource=SCS_FinalColorLDR; Capture->FOVAngle=60;
    Capture->ShowFlags.SetAntiAliasing(false); Capture->ShowFlags.SetMotionBlur(false);
    Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;
    Capture->PostProcessSettings.AutoExposureMethod=AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureBias=true;
    Capture->PostProcessSettings.AutoExposureBias=0;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
    Scene->Target=NewObject<UTextureRenderTarget2D>(Scene->Camera);
    Scene->Target->RenderTargetFormat=RTF_RGBA8;
    Scene->Target->InitAutoFormat(1600,1000); Scene->Target->UpdateResourceImmediate(true);
    Capture->TextureTarget=Scene->Target;

    // Each capture gets distinct engine frames so GPU scene updates, texture uploads and materials settle.
    auto Step=[this,Scene](FString Name,FVector Position,FVector LookAt,TFunction<void()> Setup)
    {
        FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FFunctionLatentCommand>(
            [this,Scene,Name,Position,LookAt,Setup,Frames=0,ReadyAfter=0.0]() mutable
            {
                if(Frames==0)
                {
                    Setup();
                    Scene->Camera->SetActorLocation(Position);
                    Scene->Camera->SetActorRotation((LookAt-Position).Rotation());
                    FAssetCompilingManager::Get().FinishAllCompilation();
                    GShaderCompilingManager->FinishAllCompilation();
                    Scene->World->SendAllEndOfFrameUpdates();
                    ReadyAfter=FPlatformTime::Seconds()+1.0;
                }
                if(++Frames<16 || FPlatformTime::Seconds()<ReadyAfter) return false;
                Scene->Shot(this,Name);
                return true;
            }));
    };
    const TCHAR* Names[]={TEXT("Lake"),TEXT("Beach"),TEXT("River")};
    for(int32 I=0;I<3;++I)
    {
        Step(FString(Names[I])+TEXT("_Near_Off"),FVector(0,-3800,1850),FVector::ZeroVector,[Scene,I]{
            Scene->Water->ApplyPreset(static_cast<EStylizedWaterPreset>(I));
            Scene->Water->AnimationSpeed=0; Scene->Water->bEnableSkyParallax=false; Scene->Water->RebuildSurface();
        });
        Step(FString(Names[I])+TEXT("_Far_Off"),FVector(0,-8800,2000),FVector::ZeroVector,[]{});
        Step(FString(Names[I])+TEXT("_Far_On"),FVector(0,-8800,2000),FVector::ZeroVector,[Scene]{
            Scene->Water->bEnableSkyParallax=true; Scene->Water->RebuildSurface();
        });
        Step(FString(Names[I])+TEXT("_Far_Moved"),FVector(900,-8400,2000),FVector::ZeroVector,[]{});
    }
    Step(TEXT("Lake_2x2_Shore"),FVector(0,-2600,550),FVector(0,-1400,0),[Scene]{
        Scene->Water->ApplyCalmLakePreset(); Scene->Water->bEnableSkyParallax=false; Scene->Water->AnimationSpeed=0;
        Scene->Water->GridResolution=FIntPoint(2,2); Scene->Water->RebuildSurface();
    });
    Step(TEXT("Lake_64x64_Shore"),FVector(0,-2600,550),FVector(0,-1400,0),[Scene]{
        Scene->Water->GridResolution=FIntPoint(64,64); Scene->Water->RebuildSurface();
    });
    Step(TEXT("Sky_Fixed_T0"),FVector(0,-8800,2000),FVector::ZeroVector,[Scene]{
        auto* W=Scene->Water; W->bEnableSkyParallax=true; W->ShoreRunup=0; W->FoamIntensity=0;
        W->RippleStrength=0; W->RefractionStrength=0; W->AnimationSpeed=1; W->RebuildSurface();
    });
    Step(TEXT("Sky_Fixed_T3"),FVector(0,-8800,2000),FVector::ZeroVector,[Scene]{Scene->World->Tick(LEVELTICK_All,3.f);});
    Step(TEXT("River_Flow_T0"),FVector(0,-3800,1850),FVector::ZeroVector,[Scene]{
        Scene->Water->ApplyFlowingRiverPreset(); Scene->Water->bEnableSkyParallax=false; Scene->Water->AnimationSpeed=1; Scene->Water->RebuildSurface();
    });
    Step(TEXT("River_Flow_T2"),FVector(0,-3800,1850),FVector::ZeroVector,[Scene]{Scene->World->Tick(LEVELTICK_All,2.f);});
    Step(TEXT("Beach_TerrainFilm_Close"),FVector(1100,-2150,330),FVector(0,-450,0),[Scene]{
        Scene->Ground->SetActorScale3D(FVector(140,140,1)); Scene->Ground->SetActorLocation(FVector(0,0,-72.75));
        Scene->Ground->SetActorRotation(FRotator(0,0,-2));
        Scene->Water->ApplyGentleBeachPreset(); Scene->Water->bEnableSkyParallax=false; Scene->Water->AnimationSpeed=0;
        Scene->Water->FitSurfaceToTerrain();
    });
    Step(TEXT("Beach_TerrainFilm_Grazing"),FVector(800,-4000,270),FVector(0,-400,0),[]{});
    FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FFunctionLatentCommand>([Scene,this]{
        double Still=Scene->Difference(TEXT("Sky_Fixed_T0"),TEXT("Sky_Fixed_T3"));
        double Flow=Scene->Difference(TEXT("River_Flow_T0"),TEXT("River_Flow_T2"));
        double Grid=Scene->Difference(TEXT("Lake_2x2_Shore"),TEXT("Lake_64x64_Shore"));
        TestTrue(TEXT("Fixed-camera sky has no scrolling (less than one byte mean noise)"),Still>=0 && Still<1);
        TestTrue(TEXT("River water changes with time"),Flow>0.01);
        TestTrue(TEXT("Mask shore independent of flat mesh density"),Grid>=0 && Grid<1);
        AddInfo(FString::Printf(TEXT("Image mean byte deltas: fixed sky %.6f; river %.6f; grid %.6f"),Still,Flow,Grid));
        AddInfo(Scene->Water->LastTerrainFit);
        Scene->Ground->Destroy(); Scene->Water->Destroy(); Scene->Camera->Destroy();
        return true;
    }));
    return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "FileHelpers.h"
#include "EngineUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSavedWaterRenderTest,"StylizedWater.MaskWater.SavedMapRenders",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSavedWaterRenderTest::RunTest(const FString& Parameters)
{
    const TCHAR* Maps[]={TEXT("/Game/Maps/DemoRaidMap"),TEXT("/Game/MainRaid/RaidMap")};
    for(int32 MapIndex=0;MapIndex<2;++MapIndex)
    {
        FString Map=Maps[MapIndex];
        auto Scene=MakeShared<WaterReview::FScene>();
        FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FFunctionLatentCommand>(
            [this,Scene,Map,MapIndex,Frames=0,ReadyAfter=0.0]() mutable
            {
                if(Frames==0)
                {
                    Scene->World=UEditorLoadingAndSavingUtils::LoadMap(Map);
                    if(!TestNotNull(TEXT("Migrated map loads"),Scene->World)) return true;
                    for(TActorIterator<AStylizedWaterBodyActor> It(Scene->World);It;++It) { Scene->Water=*It; break; }
                    if(!TestNotNull(TEXT("Saved water actor"),Scene->Water)) return true;
                    auto* W=Scene->Water;
                    TestEqual(TEXT("Native class replaces old BP"),W->GetClass(),AStylizedWaterBodyActor::StaticClass());
                    // Aim at the wet mask centroid rather than the center of a mostly dry actor rectangle.
                    FVector2D UV(.5,.5);
                    TArray64<uint8> Bytes;
                    if(W->BoundaryMask && W->BoundaryMask->Source.GetFormat()==TSF_RGBA16F && W->BoundaryMask->Source.GetMipData(Bytes,0))
                    {
                        const int32 NX=W->BoundaryMask->Source.GetSizeX(),NY=W->BoundaryMask->Source.GetSizeY();
                        const auto* Pixels=reinterpret_cast<const FFloat16Color*>(Bytes.GetData());
                        double XSum=0,YSum=0,Count=0;
                        for(int32 Y=0;Y<NY;++Y) for(int32 X=0;X<NX;++X) if(float(Pixels[Y*NX+X].R)>.5f) { XSum+=X+.5; YSum+=Y+.5; ++Count; }
                        if(Count>0) UV=FVector2D(XSum/Count/NX,YSum/Count/NY);
                    }
                    FVector LookAt=W->GetActorTransform().TransformPosition(FVector((UV.X-.5)*W->SurfaceSize.X,(UV.Y-.5)*W->SurfaceSize.Y,W->WaterLevelOffset));
                    FVector Position=LookAt+FVector(1700,-2100,1400);
                    Scene->Camera=Scene->World->SpawnActor<ASceneCapture2D>();
                    Scene->Camera->SetActorLocation(Position); Scene->Camera->SetActorRotation((LookAt-Position).Rotation());
                    auto* C=Scene->Camera->GetCaptureComponent2D();
                    C->bCaptureEveryFrame=false; C->bCaptureOnMovement=false; C->CaptureSource=SCS_FinalColorLDR; C->FOVAngle=55;
                    C->PostProcessSettings.bOverride_AutoExposureMethod=true; C->PostProcessSettings.AutoExposureMethod=AEM_Manual;
                    C->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true; C->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
                    Scene->Target=NewObject<UTextureRenderTarget2D>(Scene->Camera); Scene->Target->RenderTargetFormat=RTF_RGBA8;
                    Scene->Target->InitAutoFormat(1600,1000); Scene->Target->UpdateResourceImmediate(true); C->TextureTarget=Scene->Target;
                    FAssetCompilingManager::Get().FinishAllCompilation(); GShaderCompilingManager->FinishAllCompilation();
                    ReadyAfter=FPlatformTime::Seconds()+2;
                }
                if(++Frames<24 || FPlatformTime::Seconds()<ReadyAfter) return false;
                Scene->Shot(this,MapIndex==0?TEXT("DemoRaid_Migrated"):TEXT("Raid_Migrated"));
                Scene->Camera->Destroy();
                return true;
            }));
    }
    return true;
}
#endif
