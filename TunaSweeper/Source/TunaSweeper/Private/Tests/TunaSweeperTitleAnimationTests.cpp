#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Title/TunaSweeperTitleAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Title/TunaSweeperTitlePresentationActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleMotionTimingTest,"TunaSweeper.Title.Animation.Timing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleMotionTimingTest::RunTest(const FString& Parameters)
{
	auto* Owner=NewObject<USkeletalMeshComponent>();
	auto* Anim=NewObject<UTunaSweeperTitleAnimInstance>(Owner);
	if (!TestNotNull(TEXT("Entrance asset"),Anim->Entrance.Get()) || !TestNotNull(TEXT("A to B asset"),Anim->AtoB.Get())
		|| !TestNotNull(TEXT("B to A asset"),Anim->BtoA.Get())) return false;
	Anim->MinimumIdleLoops=2; Anim->MaximumIdleLoops=2;
	Anim->RestartTitleAnimation(17);
	TestEqual(TEXT("Starts with entrance"),Anim->CurrentMotion,ETunaSweeperTitleMotion::Entrance);
	TestEqual(TEXT("Gaze does not undo entrance"),Anim->HeadLookAlpha,0.f);
	Anim->NativeUpdateAnimation(2.5f);
	TestEqual(TEXT("Entrance still owns the pose"),Anim->CurrentSequence.Get(),Anim->Entrance.Get());
	TestEqual(TEXT("Gaze held until final half second"),Anim->HeadLookAlpha,0.f);
	Anim->NativeUpdateAnimation(.25f);
	TestTrue(TEXT("Gaze fades in near front"),FMath::IsNearlyEqual(Anim->HeadLookAlpha,.5f,.001f));
	Anim->NativeUpdateAnimation(.25f);
	TestEqual(TEXT("C enters A at exact seam"),Anim->CurrentSequence.Get(),Anim->IdleA.Get());
	TestEqual(TEXT("Idle starts at zero"),Anim->CurrentSequenceTime,0.f);
	Anim->NativeUpdateAnimation(8.f);
	TestEqual(TEXT("A completes its loops before stepping"),Anim->CurrentSequence.Get(),Anim->AtoB.Get());
	Anim->NativeUpdateAnimation(Anim->AtoB->GetPlayLength());
	TestEqual(TEXT("Step lands in B"),Anim->CurrentSequence.Get(),Anim->IdleB.Get());
	Anim->NativeUpdateAnimation(8.f);
	TestEqual(TEXT("B completes loops before stepping back"),Anim->CurrentSequence.Get(),Anim->BtoA.Get());
	Anim->NativeUpdateAnimation(Anim->BtoA->GetPlayLength());
	TestEqual(TEXT("Returns to A without replaying C"),Anim->CurrentSequence.Get(),Anim->IdleA.Get());
	auto* Previous=Anim->CurrentSequence.Get();
	Anim->NativeUpdateAnimation(0.f);
	TestEqual(TEXT("Zero time does not reset sequence"),Anim->CurrentSequence.Get(),Previous);
	auto* Other=NewObject<UTunaSweeperTitleAnimInstance>(Owner);
	Anim->MinimumIdleLoops=2;Anim->MaximumIdleLoops=4;
	Anim->RestartTitleAnimation(812);Other->RestartTitleAnimation(812);
	for (int32 I=0;I<60;++I)
	{
		Anim->NativeUpdateAnimation(.5f);
		for (int32 J=0;J<5;++J) Other->NativeUpdateAnimation(.1f);
		TestEqual(TEXT("Timing independent of tick size"),Anim->CurrentMotion,Other->CurrentMotion);
		TestTrue(TEXT("Overshoot retained across clips"),FMath::IsNearlyEqual(Anim->CurrentSequenceTime,Other->CurrentSequenceTime,.001f));
	}
	TSet<int32> Durations;
	for (int32 Seed=1;Seed<32;++Seed)
	{
		Anim->RestartTitleAnimation(Seed);Anim->NativeUpdateAnimation(3.f);
		int32 Loops=0;
		while (Anim->CurrentMotion==ETunaSweeperTitleMotion::IdleA && Loops<5) {Anim->NativeUpdateAnimation(4.f);++Loops;}
		TestTrue(TEXT("Random idle duration stays within 2-4 loops"),Loops>=2&&Loops<=4);
		Durations.Add(Loops);
	}
	TestTrue(TEXT("Random idle duration varies"),Durations.Num()>1);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleMotionGraphPoseTest,"TunaSweeper.Title.Animation.EvaluatedPose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleMotionGraphPoseTest::RunTest(const FString& Parameters)
{
	auto* Class=LoadClass<UAnimInstance>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/ABP_LunaMk2_Title.ABP_LunaMk2_Title_C"));
	auto* Asset=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2"));
	if (!TestNotNull(TEXT("Dedicated title AnimBP"),Class) || !TestNotNull(TEXT("Character mesh"),Asset)) return false;
	UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	auto* Actor=World->SpawnActor<AActor>();
	auto* Body=NewObject<UTunaSweeperTitleSkeletalMeshComponent>(Actor);
	Actor->SetRootComponent(Body);Body->SetSkeletalMeshAsset(Asset);Body->SetAnimInstanceClass(Class);
	Body->SetTemporaryRelaxedArmPoseEnabled(false);Body->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Body->RegisterComponent();Body->SetComponentTickEnabled(false);
	auto* Reference=NewObject<USkeletalMeshComponent>(Actor);
	Reference->SetSkeletalMeshAsset(Asset);Reference->RegisterComponent();Reference->SetComponentTickEnabled(false);
	auto* Anim=Cast<UTunaSweeperTitleAnimInstance>(Body->GetAnimInstance());
	bool Valid=TestNotNull(TEXT("Native title timing instance"),Anim);
	TSet<ETunaSweeperTitleMotion> Seen;
	if (Valid)
	{
		Anim->MinimumIdleLoops=2;Anim->MaximumIdleLoops=2;Anim->RestartTitleAnimation(6);
		for (int32 Frame=0;Frame<1200 && Valid;++Frame)
		{
			++GFrameCounter;Body->TickAnimation(1.f/30.f,false);Body->RefreshBoneTransforms();
			Seen.Add(Anim->CurrentMotion);
			if (Frame%15!=0) continue;
			Reference->PlayAnimation(Anim->CurrentSequence,false);Reference->SetPosition(Anim->CurrentSequenceTime,false);
			Reference->TickAnimation(0.f,false);Reference->RefreshBoneTransforms();
			for (FName Bone:{FName("root"),FName("pelvis"),FName("spine_05"),FName("thigh_l"),FName("thigh_r"),FName("foot_l"),FName("foot_r"),FName("hand_l"),FName("hand_r")})
			{
				const auto Actual=Body->GetSocketTransform(Bone,RTS_Component),Expected=Reference->GetSocketTransform(Bone,RTS_Component);
				if (!Actual.Equals(Expected,.02f))
				{
					AddError(FString::Printf(TEXT("Graph pose differs from authored %s at %.3f: %s, error %.4f cm"),*GetNameSafe(Anim->CurrentSequence),Anim->CurrentSequenceTime,*Bone.ToString(),FVector::Distance(Actual.GetLocation(),Expected.GetLocation())));Valid=false;break;
				}
			}
		}
		TestEqual(TEXT("All five motion phases actually evaluate"),Seen.Num(),5);
	}
	GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
	return Valid&&!HasAnyErrors();
}
#endif
