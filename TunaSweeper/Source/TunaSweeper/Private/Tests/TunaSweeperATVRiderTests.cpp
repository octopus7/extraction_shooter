#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/SkeletalMeshComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"
#include "Components/DirectionalLightComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperATVRiderTest,
	"TunaSweeper.Vehicle.RiderPose", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperATVRiderTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
	auto* ATV = World->SpawnActor<ATunaSweeperATVActor>();
	ATV->VehicleMesh->SetSimulatePhysics(false);
	UClass* PlayerClass = LoadClass<ATunaSweeperTopDownCharacter>(nullptr, TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
	if (!TestNotNull(TEXT("Player BP loads"), PlayerClass)) return false;
	FActorSpawnParameters Spawn;
	Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(PlayerClass, FVector(0,140,90), FRotator::ZeroRotator, Spawn);
	auto* Controller = World->SpawnActor<APlayerController>();
	Controller->Possess(Player);
	auto* Mesh = Player->GetMesh();
	UClass* OriginalClass = Mesh->GetAnimClass();
	const auto OriginalTickGroup = Mesh->PrimaryComponentTick.TickGroup;
	const auto OriginalVisibilityTick = Mesh->VisibilityBasedAnimTickOption;
	const bool bOriginalUpdateRate = Mesh->bEnableUpdateRateOptimizations;
	auto* Mount = ATV->MountComponent.Get();
	Mount->EngineStartSound = Mount->EngineIdleSound = Mount->EngineStopSound = nullptr;
	Mount->EngineDriveSound = Mount->EngineBoostSound = nullptr;
	TestTrue(TEXT("Mount succeeds"), Mount->RequestInteraction(Player));
	TestTrue(TEXT("Rider animation replaces on-foot graph while mounted"), Mesh->GetAnimClass() && Mesh->GetAnimClass()->GetName().Contains(TEXT("ATVRider")));
	Mesh->TickAnimation(1.0f / 60, false);
	Mesh->RefreshBoneTransforms();
	const FVector Pelvis = Mesh->GetSocketLocation(TEXT("pelvis"));
	TestTrue(TEXT("Pelvis sits above the seat"), Pelvis.Z > Mount->GetComponentLocation().Z);
	for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
	{
		const FString Suffix(Side);
		const float HandError = FVector::Distance(Mesh->GetSocketLocation(FName(*(TEXT("hand_")+Suffix))), ATV->VehicleMesh->GetSocketLocation(FName(*(TEXT("grip_")+Suffix))));
		const float FootError = FVector::Distance(Mesh->GetSocketLocation(FName(*(TEXT("foot_")+Suffix))), ATV->VehicleMesh->GetSocketLocation(FName(*(TEXT("foot_")+Suffix))));
		AddInfo(FString::Printf(TEXT("%s hand/grip %.2f cm, ankle/footrest %.2f cm"), Side, HandError, FootError));
		TestTrue(TEXT("Hands reach handle grips"), HandError < 18);
		TestTrue(TEXT("Feet rest on footplates"), FootError < 10);
	}
	if (FParse::Param(FCommandLine::Get(), TEXT("ATVRiderPreview")))
	{
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		auto* LightActor = World->SpawnActor<AActor>();
		auto* Light = NewObject<UDirectionalLightComponent>(LightActor);
		LightActor->SetRootComponent(Light);
		Light->SetIntensity(4);
		Light->RegisterComponent();
		Light->SetWorldRotation(FRotator(-35,-30,0));
		World->SendAllEndOfFrameUpdates();
		auto* CaptureActor = World->SpawnActor<AActor>();
		auto* Capture = NewObject<USceneCaptureComponent2D>(CaptureActor);
		CaptureActor->SetRootComponent(Capture);
		Capture->RegisterComponent();
		Capture->bCaptureEveryFrame = false;
		Capture->bCaptureOnMovement = false;
		Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		auto* Target = NewObject<UTextureRenderTarget2D>();
		Target->ClearColor = FLinearColor(.12f,.14f,.17f);
		Target->InitAutoFormat(1000,1000);
		Capture->TextureTarget = Target;
		Capture->FOVAngle = 38;
		const FVector Views[] = {FVector(240,-280,180), FVector(-80,-350,140), FVector(-250,-240,200)};
		for (int32 View=0; View<3; ++View)
		{
			Capture->SetWorldLocation(Views[View]);
			Capture->SetWorldRotation((FVector(0,0,90)-Capture->GetComponentLocation()).Rotation());
			Light->SetWorldRotation(Capture->GetComponentRotation());
			World->SendAllEndOfFrameUpdates();
			FlushRenderingCommands();
			Capture->CaptureScene();
			FlushRenderingCommands();
			TArray<FColor> Pixels;
			Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
			TArray64<uint8> PNG;
			FImageUtils::PNGCompressImageArray(1000,1000,Pixels,PNG);
			FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir()/FString::Printf(TEXT("ATVRigWork/RiderPose%d.png"), View)));
		}
	}
	Mount->ReleaseRiderForEndPlay();
	TestEqual(TEXT("Dismount restores original animation Blueprint"), Mesh->GetAnimClass(), OriginalClass);
	TestEqual(TEXT("Dismount restores mesh tick group"), Mesh->PrimaryComponentTick.TickGroup, OriginalTickGroup);
	TestEqual(TEXT("Dismount restores visibility ticking"), Mesh->VisibilityBasedAnimTickOption, OriginalVisibilityTick);
	TestEqual(TEXT("Dismount restores animation update rate"), Mesh->bEnableUpdateRateOptimizations, bOriginalUpdateRate);
	return true;
}
#endif
