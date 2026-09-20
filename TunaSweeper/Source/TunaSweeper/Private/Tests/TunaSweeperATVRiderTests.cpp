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
	TestTrue(TEXT("Mount succeeds"), Mount->TryMount(Player));
	TestTrue(TEXT("Rider animation replaces on-foot graph while mounted"), Mesh->GetAnimClass() && Mesh->GetAnimClass()->GetName().Contains(TEXT("ATVRider")));
	Mesh->TickAnimation(1.0f / 60, false);
	Mesh->RefreshBoneTransforms();
	const FVector Pelvis = Mesh->GetSocketLocation(TEXT("pelvis"));
	const FVector Torso = Mesh->GetSocketLocation(TEXT("neck_01")) - Pelvis;
	const float TorsoLean = FMath::RadiansToDegrees(FMath::Atan2(Torso.X, Torso.Z));
	TestTrue(TEXT("Rider has a relaxed forward lean instead of folding over the bars"), TorsoLean > 0 && TorsoLean < 25);
	auto JointAngle = [&](FName Root, FName Joint, FName End)
	{
		const FVector JointPosition = Mesh->GetSocketLocation(Joint);
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(
			(Mesh->GetSocketLocation(Root) - JointPosition).GetSafeNormal(),
			(Mesh->GetSocketLocation(End) - JointPosition).GetSafeNormal()), -1.0, 1.0)));
	};
	for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
	{
		const FString Suffix(Side);
		const float ElbowAngle = JointAngle(FName(*(TEXT("upperarm_")+Suffix)), FName(*(TEXT("lowerarm_")+Suffix)), FName(*(TEXT("hand_")+Suffix)));
		const float KneeAngle = JointAngle(FName(*(TEXT("thigh_")+Suffix)), FName(*(TEXT("calf_")+Suffix)), FName(*(TEXT("foot_")+Suffix)));
		const FVector Knee = Mesh->GetSocketLocation(FName(*(TEXT("calf_")+Suffix)));
		const FVector Ankle = Mesh->GetSocketLocation(FName(*(TEXT("foot_")+Suffix)));
		const FVector Shoulder = Mesh->GetSocketLocation(FName(*(TEXT("upperarm_")+Suffix)));
		const FVector Wrist = Mesh->GetSocketLocation(FName(*(TEXT("hand_")+Suffix)));
		AddInfo(FString::Printf(TEXT("%s elbow %.2f deg, knee %.2f deg, torso lean %.2f deg"), Side, ElbowAngle, KneeAngle, TorsoLean));
		TestTrue(TEXT("Elbows stay comfortably bent without locking or folding shut"), ElbowAngle > 65 && ElbowAngle < 155);
		TestTrue(TEXT("Hands sit below the shoulders so the rider can relax the arms"), Wrist.Z < Shoulder.Z - 3);
		TestTrue(TEXT("Knees keep a seated bend instead of stretching to the footplates"), KneeAngle > 80 && KneeAngle < 130);
		TestTrue(TEXT("Knees point forward and ankles hang below them"), Knee.X > Pelvis.X + 15 && Ankle.Z < Knee.Z - 25);
		TestTrue(TEXT("Knees remain close enough to the body for a relaxed straddle"), FMath::Abs(Knee.Y - Pelvis.Y) < 22);
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
		for (int32 View=0; View<6; ++View)
		{
			// The current vehicle is wider than the intended rider posture. Also
			// render the rider alone so the future remodel does not hide leg QA.
			if (View == 3) Capture->HideComponent(ATV->VehicleMesh);
			Capture->SetWorldLocation(Views[View % 3]);
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
			const FString Name = View < 3 ? FString::Printf(TEXT("RiderPose%d.png"), View) : FString::Printf(TEXT("RiderPoseBody%d.png"), View-3);
			FFileHelper::SaveArrayToFile(PNG, *(FPaths::ProjectSavedDir()/TEXT("ATVRigWork")/Name));
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
