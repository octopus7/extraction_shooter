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
	auto* Camera = Character->FindComponentByClass<UCameraComponent>();
	if (!TestNotNull(TEXT("Character camera retained"), Camera)) return false;
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
	Camera->AspectRatio = OriginalAspect;
	Camera->bConstrainAspectRatio = OriginalConstrain;
	Camera->bOverrideAspectRatioAxisConstraint = OriginalOverride;
	Studio->Tick(0.f);
	return true;
}
#endif
