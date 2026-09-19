#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Title/TunaSweeperTitlePresentationActor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTitleHairInertiaTest,
	"TunaSweeper.Gaze.TitleHairInertia", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperTitleHairInertiaTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	USkeletalMesh* Asset = LoadObject<USkeletalMesh>(nullptr,
		TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2.SKM_LunaMk2"));
	UClass* AnimClass = LoadClass<UAnimInstance>(nullptr,
		TEXT("/Game/Characters/Player/LunaMk2/Animations/ABP_LunaMk2.ABP_LunaMk2_C"));
	if (!Asset || !AnimClass)
	{
		AddError(TEXT("Luna mesh or animation Blueprint failed to load"));
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
		return false;
	}
	auto MakeMesh = [&]()
	{
		AActor* Owner = World->SpawnActor<AActor>();
		auto* Mesh = NewObject<UTunaSweeperTitleSkeletalMeshComponent>(Owner);
		Owner->SetRootComponent(Mesh);
		Mesh->SetSkeletalMeshAsset(Asset);
		Mesh->SetAnimInstanceClass(AnimClass);
		Mesh->SetTemporaryRelaxedArmPoseEnabled(false);
		Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
		Mesh->RegisterComponent();
		Mesh->SetComponentTickEnabled(false);
		Mesh->SetDirectHeadLookRotation(0, 0);
		return Mesh;
	};
	auto* Moving = MakeMesh();
	auto* Stationary = MakeMesh();
	auto Step = [&]()
	{
		for (auto* Mesh : {Moving, Stationary})
		{
			Mesh->TickAnimation(1.0f / 60.0f, false);
			Mesh->RefreshBoneTransforms();
		}
		++GFrameCounter;
	};
	auto TipInHead = [](USkeletalMeshComponent* Mesh, const TCHAR* Bone)
	{
		return Mesh->GetSocketTransform(TEXT("head"), RTS_Component).InverseTransformPosition(
			Mesh->GetSocketTransform(Bone, RTS_Component).GetLocation());
	};
	for (int32 Frame = 0; Frame < 120; ++Frame) Step();
	double MaxDifference = 0;
	for (int32 Frame = 0; Frame < 45; ++Frame)
	{
		Moving->SetDirectHeadLookRotation(FMath::Min(Frame * 3.0f, 45.0f), 20.0f);
		Step();
		for (const TCHAR* Tip : {TEXT("sidetail_L_006"), TEXT("sidetail_R_006")})
		{
			MaxDifference = FMath::Max(MaxDifference,
				FVector::Distance(TipInHead(Moving, Tip), TipInHead(Stationary, Tip)));
		}
	}
	AddInfo(FString::Printf(TEXT("Head-relative hair response to head turn: %.4f cm"), MaxDifference));
	TestTrue(TEXT("Turning only the head produces hair motion relative to the head"), MaxDifference > 0.5);
	const FVector Aim = Moving->GetSocketTransform(TEXT("head"), RTS_Component).GetRotation().RotateVector(FVector::RightVector);
	TestTrue(TEXT("Head keeps the requested absolute yaw/pitch while hair simulates"),
		Aim.Equals(FVector(0.6644630244, 0.6644630244, 0.3420201433), 0.005));
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);
	return true;
}

#endif
