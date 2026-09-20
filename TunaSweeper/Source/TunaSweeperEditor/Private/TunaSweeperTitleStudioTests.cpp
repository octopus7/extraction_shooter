#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Title/TunaSweeperTitlePresentationActor.h"
#include "Title/TunaSweeperTitleStudioActor.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "ImageUtils.h"
#include "AssetCompilingManager.h"
#include "RenderingThread.h"
#include "ContentStreaming.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "SceneView.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleStudioSeparationTest,
	"TunaSweeper.Title.Studio.SeparationAndBackdrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTitleStudioSeparationTest::RunTest(const FString& Parameters)
{
	if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir() / TEXT("Maps/IntroMap.umap"), false, true)) return false;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	ATunaSweeperTitlePresentationActor* Character = nullptr;
	ATunaSweeperTitleStudioActor* Studio = nullptr;
	for (TActorIterator<ATunaSweeperTitlePresentationActor> It(World); It; ++It) Character = *It;
	for (TActorIterator<ATunaSweeperTitleStudioActor> It(World); It; ++It) Studio = *It;
	if (!TestNotNull(TEXT("Character BP placed"), Character) || !TestNotNull(TEXT("Studio BP placed"), Studio)) return false;
	TestEqual(TEXT("Independent studio references the character camera"), Studio->PresentationActor.Get(), Character);
	// The camera owns an editor-only visualization mesh; only the former set parts must be absent.
	for (auto* Part : TInlineComponentArray<UStaticMeshComponent*>(Character))
		TestFalse(TEXT("Character has no set geometry"), Part->GetName() == TEXT("BackWall") ||
			Part->GetName() == TEXT("LeftWall") || Part->GetName() == TEXT("RightWall") || Part->GetName() == TEXT("Floor"));
	TestNull(TEXT("Character has no studio lighting"), Character->FindComponentByClass<ULightComponent>());
	TestNotNull(TEXT("Lighting is owned by studio"), Studio->FindComponentByClass<ULightComponent>());
	UStaticMeshComponent* Backdrop = nullptr;
	for (UStaticMeshComponent* Part : TInlineComponentArray<UStaticMeshComponent*>(Studio))
	{
		if (Part->GetFName() == TEXT("MatteBackdrop")) Backdrop = Part;
		else TestFalse(TEXT("Studio walls/floor do not occlude the lake"), Part->IsVisible());
	}
	if (!TestNotNull(TEXT("Matte backdrop"), Backdrop)) return false;
	TestNotNull(TEXT("Saved backdrop material"), Backdrop->GetMaterial(0));
	if (TestNotNull(TEXT("Curved screen mesh"), Backdrop->GetStaticMesh().Get()))
	{
		TestEqual(TEXT("Saved curved screen asset"), Backdrop->GetStaticMesh()->GetFName(), FName(TEXT("SM_TitleCurvedScreen")));
		TestTrue(TEXT("Screen has actual depth curvature"), Backdrop->GetStaticMesh()->GetBoundingBox().GetSize().Z > 10.f);
	}
	if (UMaterial* Material = Backdrop->GetMaterial(0)->GetMaterial())
	{
		TestTrue(TEXT("Matte has no lighting or shape shadows"), Material->GetShadingModels().HasShadingModel(MSM_Unlit));
		TestEqual(TEXT("Matte avoids opaque emissive GBuffer"), Material->GetBlendMode(), BLEND_Translucent);
	}
	auto* Camera = Character->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("Character camera retained"), Camera)) return false;
	TestTrue(TEXT("Title explicitly controls exposure"), Camera->PostProcessSettings.bOverride_AutoExposureMethod);
	TestEqual(TEXT("Title exposure cannot adapt over time"), Camera->PostProcessSettings.AutoExposureMethod.GetValue(), AEM_Manual);
	TestFalse(TEXT("Exposure is independent of physical camera defaults"), bool(Camera->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure));
	const FRotator OriginalRotation = Camera->GetRelativeRotation();
	const float OriginalAspect = Camera->AspectRatio;
	const bool OriginalConstrain = Camera->bConstrainAspectRatio;
	const bool OriginalOverride = Camera->bOverrideAspectRatioAxisConstraint;
	Camera->AspectRatio = 16.f / 9.f;
	Camera->bConstrainAspectRatio = false;
	Camera->bOverrideAspectRatioAxisConstraint = false;
	const FVector2D WideSize = ATunaSweeperTitleStudioActor::CalculateBackdropSize(Camera, FIntPoint(2520, 1080), AspectRatio_MaintainYFOV);
	const float BaselineWidth = 6000.f * FMath::Tan(FMath::DegreesToRadians(Camera->FieldOfView * 0.5f));
	TestTrue(TEXT("Ultrawide viewport expands horizontal FOV with MaintainYFOV"), WideSize.X >= BaselineWidth * 2520.f / 1920.f);
	for (float Aspect : {16.f / 9.f, 4.f / 3.f, 21.f / 9.f})
	{
		Camera->AspectRatio = Aspect;
		Studio->Tick(0.f);
		const FVector Delta = Backdrop->GetComponentLocation() - Camera->GetComponentLocation();
		TestTrue(TEXT("Backdrop centered behind character"), Delta.GetSafeNormal().Equals(Camera->GetForwardVector(), 0.001f));
		const float RequiredWidth = 6000.f * FMath::Tan(FMath::DegreesToRadians(Camera->FieldOfView * 0.5f));
		TestTrue(TEXT("Backdrop covers width"), Backdrop->GetComponentScale().X * 100.f >= RequiredWidth);
		TestTrue(TEXT("Backdrop covers height"), Backdrop->GetComponentScale().Y * 100.f >= RequiredWidth / Aspect);
	}
	Camera->SetRelativeRotation(FRotator(-1.f, -38.f, 0.f));
	Studio->Tick(0.f);
	TestTrue(TEXT("Backdrop follows submenu camera"),
		(Backdrop->GetComponentLocation() - Camera->GetComponentLocation()).GetSafeNormal().Equals(Camera->GetForwardVector(), 0.001f));
	Camera->SetRelativeRotation(OriginalRotation);
	Camera->AspectRatio = 16.f / 9.f;
	Studio->Tick(0.f);
	FAssetCompilingManager::Get().FinishAllCompilation();
	IStreamingManager::Get().StreamAllResources(5.f);
	auto* Capture = NewObject<USceneCaptureComponent2D>(Studio);
	auto* Target = NewObject<UTextureRenderTarget2D>();
	Target->InitCustomFormat(1280, 720, PF_B8G8R8A8, false);
	Capture->TextureTarget = Target;
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;
	Capture->RegisterComponentWithWorld(World);
	Capture->SetWorldTransform(Camera->GetComponentTransform());
	Capture->FOVAngle = Camera->FieldOfView;
	Capture->PostProcessSettings = Camera->PostProcessSettings;
	Capture->CaptureScene();
	FlushRenderingCommands();
	FImage Pixels;
	if (TestTrue(TEXT("Title capture readable"), FImageUtils::GetRenderTargetImage(Target, Pixels)))
		TestTrue(TEXT("Title capture saved"), FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() / TEXT("Screenshots/TitleMatteLake.png")), Pixels));
	Capture->DestroyComponent();
	// Render the same camera at genuinely different output aspect ratios, keeping
	// its reference aspect fixed, just as a resized game viewport does.
	for (FIntPoint Resolution : {FIntPoint(1280,720), FIntPoint(960,720), FIntPoint(2560,720), FIntPoint(720,1280)})
	{
		auto* AspectCapture = NewObject<USceneCaptureComponent2D>(Studio);
		auto* AspectTarget = NewObject<UTextureRenderTarget2D>();
		AspectTarget->InitCustomFormat(Resolution.X, Resolution.Y, PF_B8G8R8A8, false);
		AspectCapture->TextureTarget = AspectTarget;
		AspectCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		AspectCapture->bCaptureEveryFrame = false;
		AspectCapture->bCaptureOnMovement = false;
		AspectCapture->RegisterComponentWithWorld(World);
		AspectCapture->SetWorldTransform(Camera->GetComponentTransform());
		AspectCapture->PostProcessSettings = Camera->PostProcessSettings;
		FMinimalViewInfo View;
		Camera->GetCameraView(0.f, View);
		FSceneViewProjectionData Projection;
		const FIntRect Rect(0,0,Resolution.X,Resolution.Y);
		Projection.SetViewRectangle(Rect);
		FMinimalViewInfo::CalculateProjectionMatrixGivenViewRectangle(View, AspectRatio_MaintainYFOV, Rect, Projection);
		AspectCapture->bUseCustomProjectionMatrix = true;
		AspectCapture->CustomProjectionMatrix = Projection.ProjectionMatrix;
		const FVector2D Size = ATunaSweeperTitleStudioActor::CalculateBackdropSize(Camera, Resolution, AspectRatio_MaintainYFOV);
		Backdrop->SetWorldScale3D(FVector(Size.X/100.f, Size.Y/100.f, FMath::Min(Size.X,Size.Y)/100.f));
		AspectCapture->CaptureScene();
		FlushRenderingCommands();
		FImage AspectPixels;
		if (TestTrue(TEXT("Aspect capture readable"), FImageUtils::GetRenderTargetImage(AspectTarget, AspectPixels)))
		{
			TestTrue(TEXT("Aspect capture saved"), FImageUtils::SaveImageByExtension(*(FPaths::ProjectSavedDir() /
				FString::Printf(TEXT("Screenshots/TitleProjected_%dx%d.png"),Resolution.X,Resolution.Y)), AspectPixels));
			AspectCapture->HiddenActors.Add(Character);
			AspectCapture->CaptureScene();
			FlushRenderingCommands();
			FImageUtils::GetRenderTargetImage(AspectTarget, AspectPixels);
			if (AspectPixels.Format == ERawImageFormat::BGRA8)
			{
				const FColor* Colors = reinterpret_cast<const FColor*>(AspectPixels.RawData.GetData());
				for (FIntPoint Corner : {FIntPoint(2,2),FIntPoint(Resolution.X-3,2),FIntPoint(2,Resolution.Y-3),FIntPoint(Resolution.X-3,Resolution.Y-3)})
				{
					const FColor Color = Colors[Corner.Y*Resolution.X+Corner.X];
					TestTrue(TEXT("Projected matte covers viewport corners"), FMath::Max3(Color.R,Color.G,Color.B)>3);
				}
			}
		}
		AspectCapture->DestroyComponent();
	}
	Camera->AspectRatio = OriginalAspect;
	Camera->bConstrainAspectRatio = OriginalConstrain;
	Camera->bOverrideAspectRatioAxisConstraint = OriginalOverride;
	Studio->Tick(0.f);
	return true;
}
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FCheckTitleRuntimeExposure, FAutomationTestBase*, Test, FString, CaptureName);
bool FCheckTitleRuntimeExposure::Update()
{
	UWorld* World = GEditor->PlayWorld;
	if (!Test->TestNotNull(TEXT("Title PIE world"), World)) return true;
	APlayerController* Controller = World->GetFirstPlayerController();
	if (!Test->TestNotNull(TEXT("Title PIE controller"), Controller)) return true;
	const FPostProcessSettings& Settings = Controller->PlayerCameraManager->GetCameraCacheView().PostProcessSettings;
	Test->TestEqual(TEXT("Runtime view retains manual exposure"), Settings.AutoExposureMethod.GetValue(), AEM_Manual);
	Test->TestEqual(TEXT("Runtime fixed exposure compensation"), Settings.AutoExposureBias, 3.f);
	FViewport* Viewport = GEditor->GetPIEViewport();
	if (!Test->TestNotNull(TEXT("Title PIE viewport"), Viewport)) return true;
	TArray<FColor> Pixels;
	if (Test->TestTrue(TEXT("Runtime title pixels"), Viewport->ReadPixels(Pixels)))
	{
		const FIntPoint Size = Viewport->GetSizeXY();
		TArray<uint8> Png;
		FImageUtils::CompressImageArray(Size.X, Size.Y, Pixels, Png);
		Test->TestTrue(TEXT("Runtime title capture saved"), FFileHelper::SaveArrayToFile(Png,
			*(FPaths::ProjectSavedDir() / TEXT("Screenshots") / CaptureName)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleRuntimeExposureTest,
	"TunaSweeper.Title.Studio.RuntimeExposure",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleRuntimeExposureTest::RunTest(const FString& Parameters)
{
	if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir() / TEXT("Maps/IntroMap.umap"), false, true)) return false;
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckTitleRuntimeExposure(this, TEXT("TitleRuntime_2s.png")));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(5.f));
	ADD_LATENT_AUTOMATION_COMMAND(FCheckTitleRuntimeExposure(this, TEXT("TitleRuntime_7s.png")));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
