#if WITH_DEV_AUTOMATION_TESTS
#include "Character/TunaSweeperMoleCompanionActor.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMoleCompanionVisualComponentsTest,
	"TunaSweeper.Character.Mole.VisualComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperMoleCompanionVisualComponentsTest::RunTest(const FString& Parameters)
{
	UClass* Class = LoadObject<UClass>(nullptr, TEXT("/Game/Characters/Mole/BP_Mole.BP_Mole_C"));
	ATunaSweeperMoleCompanionActor* Defaults = Class ? Cast<ATunaSweeperMoleCompanionActor>(Class->GetDefaultObject()) : nullptr;
	if (!TestNotNull(TEXT("BP_Mole has companion defaults"), Defaults)) return false;
	TestNull(TEXT("Old static mesh component is removed"), Defaults->GetDefaultSubobjectByName(TEXT("DummyMesh")));
	const USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Defaults->GetDefaultSubobjectByName(TEXT("SkeletalMesh")));
	if (!TestNotNull(TEXT("Skeletal component exists"), Mesh)) return false;
	if (!TestNotNull(TEXT("Skeletal mesh is assigned"), Mesh->GetSkeletalMeshAsset())) return false;
	TestEqual(TEXT("Uses SKM_MoleDummy"), Mesh->GetSkeletalMeshAsset()->GetName(), FString(TEXT("SKM_MoleDummy")));
	TestTrue(TEXT("Skeletal mesh is visible"), Mesh->IsVisible());
	TestFalse(TEXT("Skeletal mesh is not hidden in game"), Mesh->bHiddenInGame);
	TestEqual(TEXT("Keeps former visual forward axis"), Mesh->GetRelativeRotation().Yaw, -90.0);
	const UBlendSpace* Blend = Cast<UBlendSpace>(Mesh->AnimationData.AnimToPlay);
	if (!TestNotNull(TEXT("Serialized idle/turn blend is connected"), Blend)) return false;
	TestEqual(TEXT("Idle and both turn directions are connected"), Blend->GetBlendSamples().Num(), 3);
	for (const FBlendSample& Sample : Blend->GetBlendSamples())
	{
		if (!TestNotNull(TEXT("Blend sample animation exists"), Sample.Animation.Get())) continue;
		const FString Name = Sample.Animation->GetName();
		TestTrue(TEXT("Only breathing and directional stationary turning are used"), Name == TEXT("A_Mole_Idle_Breathe") || Name == TEXT("A_Mole_Turn_Left_InPlace") || Name == TEXT("A_Mole_Turn_Right_InPlace"));
		TestEqual(TEXT("Same skeleton"), Sample.Animation->GetSkeleton(), Mesh->GetSkeletalMeshAsset()->GetSkeleton());
	}
	for (float Input : {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f})
	{
		TArray<FBlendSampleData> Samples; int32 Cache = INDEX_NONE;
		TestTrue(TEXT("Blend has baked runtime interpolation data"), Blend->GetSamplesFromBlendInput(FVector(Input, 0, 0), Samples, Cache, true));
		TestTrue(TEXT("Runtime sampling returns a pose"), Samples.Num() > 0);
		if (Input == 0.0f || FMath::Abs(Input) == 1.0f)
		{
			const FString Expected = Input < 0 ? TEXT("A_Mole_Turn_Left_InPlace") : Input > 0 ? TEXT("A_Mole_Turn_Right_InPlace") : TEXT("A_Mole_Idle_Breathe");
			float ExpectedWeight = 0.0f;
			for (const FBlendSampleData& Sample : Samples)
			{
				if (Blend->GetBlendSamples()[Sample.SampleDataIndex].Animation->GetName() == Expected) ExpectedWeight += Sample.GetClampedWeight();
			}
			TestEqual(TEXT("Turn sign selects the matching footwork at full weight"), ExpectedWeight, 1.0f);
		}
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMoleTurnAnimationTest,
	"TunaSweeper.Character.Mole.TurnAnimation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperMoleTurnAnimationTest::RunTest(const FString& Parameters)
{
	using Mole = ATunaSweeperMoleCompanionActor;
	TestEqual(TEXT("No turn is idle"), Mole::ResolveTurnAnimationAmount(15,15,.1f), 0.0f);
	TestEqual(TEXT("Tiny gaze drift is idle"), Mole::ResolveTurnAnimationAmount(15,15.1f,.1f), 0.0f);
	TestEqual(TEXT("No time elapsed is idle"), Mole::ResolveTurnAnimationAmount(0,90,0), 0.0f);
	TestEqual(TEXT("Right turn steps"), Mole::ResolveTurnAnimationAmount(0,3,.1f), 1.0f);
	TestEqual(TEXT("Left turn steps"), Mole::ResolveTurnAnimationAmount(0,-3,.1f), -1.0f);
	TestEqual(TEXT("Left turn across yaw wrap keeps its direction"), Mole::ResolveTurnAnimationAmount(-179,179,.1f), -Mole::ResolveTurnAnimationAmount(0,2,.1f));
	TestEqual(TEXT("Yaw wrap uses short arc"), Mole::ResolveTurnAnimationAmount(179,-179,.1f), Mole::ResolveTurnAnimationAmount(0,2,.1f));
	TestTrue(TEXT("Settling turn blends to idle"), Mole::ResolveTurnAnimationAmount(0,1,.1f) > 0 && Mole::ResolveTurnAnimationAmount(0,1,.1f) < 1);
	const UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Characters/NPC/Mole/A_Mole_Idle_Breathe.A_Mole_Idle_Breathe"));
	if (!TestNotNull(TEXT("Breathing reference exists"), Idle)) return false;
	FTransform IdleRoot;
	Idle->GetBoneTransform(IdleRoot, FSkeletonPoseBoneIndex(0), FAnimExtractContext(0.0), true);
	for (const TCHAR* Name : {TEXT("Idle_Breathe"), TEXT("Turn_Left_InPlace"), TEXT("Turn_Right_InPlace")})
	{
		const FString Path = FString::Printf(TEXT("/Game/Characters/NPC/Mole/A_Mole_%s.A_Mole_%s"), Name, Name);
		const UAnimSequence* Clip = LoadObject<UAnimSequence>(nullptr, *Path);
		if (!TestNotNull(TEXT("Runtime clip exists"), Clip)) continue;
		if (!TestNotNull(TEXT("Runtime clip skeleton is saved"), Clip->GetSkeleton())) continue;
		TestFalse(TEXT("No runtime root motion"), Clip->bEnableRootMotion);
		FTransform First; Clip->GetBoneTransform(First, FSkeletonPoseBoneIndex(0), FAnimExtractContext(0.0), true);
		TestTrue(TEXT("Turn root matches idle so blending preserves body scale and heading"), First.Equals(IdleRoot, .001f));
		for (int32 Frame=0; Frame<Clip->GetNumberOfSampledKeys(); ++Frame)
		{
			FTransform Root; Clip->GetBoneTransform(Root, FSkeletonPoseBoneIndex(0), FAnimExtractContext(Frame/30.0), true);
			TestTrue(TEXT("Root stays fixed during idle and stepping"), Root.Equals(First, .001f));
		}
		const FReferenceSkeleton& Ref = Clip->GetSkeleton()->GetReferenceSkeleton();
		for (int32 Bone=0; Bone<Ref.GetNum(); ++Bone)
		{
			FTransform Start, End;
			Clip->GetBoneTransform(Start, FSkeletonPoseBoneIndex(Bone), FAnimExtractContext(0.0), true);
			Clip->GetBoneTransform(End, FSkeletonPoseBoneIndex(Bone), FAnimExtractContext(double(Clip->GetPlayLength())), true);
			TestTrue(TEXT("Runtime clip loop is seamless"), Start.Equals(End, .001f));
		}
	}
	for (const TCHAR* Name : {TEXT("Walk_InPlace"), TEXT("Walk_Forward")})
	{
		TestNotNull(TEXT("Unused walk clip remains available"), LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Characters/NPC/Mole/A_Mole_%s.A_Mole_%s"), Name, Name)));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMoleRuntimeTurnTest,
	"TunaSweeper.Character.Mole.RuntimeTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperMoleRuntimeTurnTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false,
		MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), TEXT("MoleTurnTest")));
	if (!TestNotNull(TEXT("Isolated test world"), World)) return false;
	UClass* Class = LoadObject<UClass>(nullptr, TEXT("/Game/Characters/Mole/BP_Mole.BP_Mole_C"));
	ATunaSweeperMoleCompanionActor* Actor = World->SpawnActor<ATunaSweeperMoleCompanionActor>(Class);
	if (TestNotNull(TEXT("Placed BP instance"), Actor))
	{
		USkeletalMeshComponent* Mesh = Actor->FindComponentByClass<USkeletalMeshComponent>();
		UAnimSingleNodeInstance* Anim = Mesh ? Mesh->GetSingleNodeInstance() : nullptr;
		if (TestNotNull(TEXT("Playback instance initialized"), Anim))
		{
			const FVector Position = Actor->GetActorLocation();
			Actor->SetActorRotation(FRotator(0, 90, 0));
			Actor->Tick(.1f);
			TestTrue(TEXT("Tracking yaw is limited so footwork can follow"), FMath::Abs(FMath::FindDeltaAngleDegrees(90.0f, Actor->GetActorRotation().Yaw)) <= 9.01f);
			FVector Input, Filtered; Anim->GetBlendSpaceState(Input, Filtered);
			TestTrue(TEXT("Returning from positive yaw drives left steps"), Input.X < 0);
			Actor->SetActorRotation(FRotator(0, -90, 0)); Actor->Tick(.1f);
			Anim->GetBlendSpaceState(Input, Filtered);
			TestTrue(TEXT("Returning from negative yaw drives right steps"), Input.X > 0);
			TestTrue(TEXT("Turning does not move actor"), Actor->GetActorLocation().Equals(Position));
			Actor->SetActorRotation(FRotator::ZeroRotator); Actor->Tick(.1f);
			Anim->GetBlendSpaceState(Input, Filtered);
			TestEqual(TEXT("Settled actor returns to breathing"), Input.X, 0.0);
			TestEqual(TEXT("Settled breathing plays at normal speed"), Anim->GetPlayRate(), 1.0f);
			TestEqual(TEXT("Playback still uses only idle/turn blend"), Anim->GetAnimationAsset()->GetName(), FString(TEXT("BS_Mole_IdleTurn")));
			TArray<FVector> DirectionPoses;
			for (float Direction : {-1.0f, 1.0f})
			{
				Anim->SetBlendSpacePosition(FVector(Direction, 0, 0));
				Anim->SetPosition(0.0f, false);
				FBox LeftFootMotion(ForceInit), RightFootMotion(ForceInit);
				for (int32 Frame = 0; Frame < 128; ++Frame)
				{
					// AnimInstanceProxy advances its native state once per engine frame.
					++GFrameCounter;
					Mesh->TickAnimation(1.0f / 60.0f, false);
					Mesh->RefreshBoneTransforms();
					const FVector Foot = Mesh->GetSocketTransform(TEXT("foot_L"), RTS_Component).GetLocation();
					LeftFootMotion += Foot;
					RightFootMotion += Mesh->GetSocketTransform(TEXT("foot_R"), RTS_Component).GetLocation();
					if (Frame == 30) DirectionPoses.Add(Foot);
				}
				// A directional turn may pivot on the inner foot; the stepping foot must move.
				AddInfo(FString::Printf(TEXT("Direction %.0f: left/right foot travel %.3f / %.3f cm"), Direction, LeftFootMotion.GetSize().Size(), RightFootMotion.GetSize().Size()));
				AddInfo(FString::Printf(TEXT("Direction %.0f: head %s, root %s"), Direction, *Mesh->GetSocketTransform(TEXT("head"), RTS_Component).ToString(), *Mesh->GetSocketTransform(TEXT("root"), RTS_Component).ToString()));
				TestTrue(TEXT("Turning retains full-sized skeletal pose"), Mesh->GetSocketTransform(TEXT("head"), RTS_Component).GetLocation().Z > 40.0f);
				TestTrue(TEXT("Evaluated turning pose visibly moves a stepping foot"), FMath::Max(LeftFootMotion.GetSize().Size(), RightFootMotion.GetSize().Size()) > 1.0f);
			}
			TestTrue(TEXT("Left and right produce different evaluated foot poses"), !DirectionPoses[0].Equals(DirectionPoses[1], 0.1f));
		}
	}
	World->DestroyWorld(false); World->RemoveFromRoot();
	return true;
}
#endif
