#if WITH_DEV_AUTOMATION_TESTS
#include "Character/TunaSweeperMoleCompanionActor.h"
#include "Animation/AnimSequence.h"
#include "Character/TunaSweeperMoleAnimInstance.h"
#include "Animation/AnimMontage.h"
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
		UTunaSweeperMoleAnimInstance* Anim = Mesh ? Cast<UTunaSweeperMoleAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
		Actor->bEnableIdleVariations = false;
		if (TestNotNull(TEXT("Playback instance initialized"), Anim))
		{
			const FVector Position = Actor->GetActorLocation();
			Actor->SetActorRotation(FRotator(0, 90, 0));
			Actor->Tick(.1f);
			TestTrue(TEXT("Tracking yaw is limited so footwork can follow"), FMath::Abs(FMath::FindDeltaAngleDegrees(90.0f, Actor->GetActorRotation().Yaw)) <= 9.01f);
			FVector Input(Anim->TurnAmount, 0, 0);
			TestTrue(TEXT("Returning from positive yaw drives left steps"), Input.X < 0);
			Actor->SetActorRotation(FRotator(0, -90, 0)); Actor->Tick(.1f);
			Input.X = Anim->TurnAmount;
			TestTrue(TEXT("Returning from negative yaw drives right steps"), Input.X > 0);
			TestTrue(TEXT("Turning does not move actor"), Actor->GetActorLocation().Equals(Position));
			Actor->SetActorRotation(FRotator::ZeroRotator); Actor->Tick(.1f);
			Input.X = Anim->TurnAmount;
			TestEqual(TEXT("Settled actor returns to breathing"), Input.X, 0.0);
			TestEqual(TEXT("Settled breathing plays at normal speed"), Anim->TurnPlayRate, 1.0f);
			TestEqual(TEXT("Playback uses the upper-body animation graph"), Anim->GetClass()->GetName(), FString(TEXT("ABP_MoleCompanion_C")));
			TArray<FVector> DirectionPoses;
			for (float Direction : {-1.0f, 1.0f})
			{
				Anim->TurnAmount = Direction;
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMoleIdleVariationTest,
	"TunaSweeper.Character.Mole.IdleVariations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperMoleIdleVariationTest::RunTest(const FString& Parameters)
{
	for (const TCHAR* Name : {TEXT("Sniff"), TEXT("ShoulderRoll"), TEXT("HeadTilt"), TEXT("PawWave")})
	{
		const FString Path = FString::Printf(TEXT("/Game/Characters/NPC/Mole/A_Mole_Idle_%s"), Name);
		TestNotNull(TEXT("Authored upper-body idle exists"), LoadObject<UAnimSequence>(nullptr, *Path));
	}
	UClass* Class = LoadClass<UAnimInstance>(nullptr, TEXT("/Game/Characters/NPC/Mole/ABP_MoleCompanion.ABP_MoleCompanion_C"));
	TestNotNull(TEXT("Mole has an animation graph supporting upper-body slots"), Class);
	if (!Class) return false;
	const auto* Defaults = Cast<UTunaSweeperMoleAnimInstance>(Class->GetDefaultObject());
	if (!TestNotNull(TEXT("Native idle scheduler"), Defaults)) return false;
	TestEqual(TEXT("Four gestures are configured"), Defaults->IdleVariations.Num(), 4);
	const auto* MoleDefaults = GetDefault<ATunaSweeperMoleCompanionActor>();
	TestEqual(TEXT("Default minimum rest is four seconds"), MoleDefaults->IdleVariationMinDelay, 4.0f);
	TestEqual(TEXT("Default maximum rest is fifteen seconds"), MoleDefaults->IdleVariationMaxDelay, 15.0f);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	UClass* MoleClass = LoadClass<ATunaSweeperMoleCompanionActor>(nullptr, TEXT("/Game/Characters/Mole/BP_Mole.BP_Mole_C"));
	for (int32 Gesture = 0; Gesture < 4; ++Gesture)
	{
		auto* BaseActor = World->SpawnActor<ATunaSweeperMoleCompanionActor>(MoleClass);
		auto* OverlayActor = World->SpawnActor<ATunaSweeperMoleCompanionActor>(MoleClass);
		BaseActor->bEnableIdleVariations = OverlayActor->bEnableIdleVariations = false;
		auto* BaseMesh = BaseActor->FindComponentByClass<USkeletalMeshComponent>();
		auto* OverlayMesh = OverlayActor->FindComponentByClass<USkeletalMeshComponent>();
		auto* BaseAnim = Cast<UTunaSweeperMoleAnimInstance>(BaseMesh->GetAnimInstance());
		auto* OverlayAnim = Cast<UTunaSweeperMoleAnimInstance>(OverlayMesh->GetAnimInstance());
		if (!TestNotNull(TEXT("Placed actor uses slot AnimBP"), BaseAnim) || !TestNotNull(TEXT("Second actor uses slot AnimBP"), OverlayAnim)) break;
		BaseAnim->TurnAmount = OverlayAnim->TurnAmount = Gesture % 2 ? -1.0f : 1.0f;
		TestTrue(TEXT("Gesture starts in slot"), OverlayAnim->PlayIdleVariation(Gesture));
		TestFalse(TEXT("A gesture cannot overwrite an active gesture"), OverlayAnim->PlayIdleVariation((Gesture + 1) % 4));
		float MaxLowerDifference = 0, MaxUpperDifference = 0;
		for (int32 Frame = 0; Frame < 300; ++Frame)
		{
			++GFrameCounter;
			for (auto* M : {BaseMesh, OverlayMesh}) { M->TickAnimation(1.0f / 60.0f, false); M->RefreshBoneTransforms(); }
			for (const TCHAR* Bone : {TEXT("pelvis"), TEXT("foot_L"), TEXT("foot_R")})
			{
				const FTransform A = BaseMesh->GetSocketTransform(Bone, RTS_Component), B = OverlayMesh->GetSocketTransform(Bone, RTS_Component);
				MaxLowerDifference = FMath::Max(MaxLowerDifference, float(FVector::Distance(A.GetLocation(), B.GetLocation())));
				TestTrue(TEXT("Slot preserves lower-body rotation and scale"), A.GetRotation().Equals(B.GetRotation(), .001f) && A.GetScale3D().Equals(B.GetScale3D(), .001f));
			}
			for (const TCHAR* Bone : {TEXT("head"), TEXT("hand_L"), TEXT("hand_R")})
			{
				const FTransform A = BaseMesh->GetSocketTransform(Bone, RTS_Component), B = OverlayMesh->GetSocketTransform(Bone, RTS_Component);
				MaxUpperDifference = FMath::Max(MaxUpperDifference, float(FMath::RadiansToDegrees(A.GetRotation().AngularDistance(B.GetRotation()))));
			}
		}
		AddInfo(FString::Printf(TEXT("Gesture %d: lower-body difference %.5f cm, upper-body rotation difference %.2f degrees"), Gesture, MaxLowerDifference, MaxUpperDifference));
		TestTrue(TEXT("Slot leaves turning footwork unchanged"), MaxLowerDifference < .01f);
		TestTrue(TEXT("Each gesture visibly animates the upper body"), MaxUpperDifference > 5.0f);
		TestFalse(TEXT("Gesture is a one-shot, not a loop"), OverlayAnim->Montage_IsPlaying(nullptr));
		TestTrue(TEXT("After gesture the upper body returns to base pose"), BaseMesh->GetSocketTransform(TEXT("head"), RTS_Component).Equals(OverlayMesh->GetSocketTransform(TEXT("head"), RTS_Component), .01f));
		BaseActor->Destroy(); OverlayActor->Destroy();
	}
	// Observe real montage start/end transitions with a short BP-configured rest.
	auto* Actor = World->SpawnActor<ATunaSweeperMoleCompanionActor>(MoleClass);
	Actor->IdleVariationMinDelay = .2f; Actor->IdleVariationMaxDelay = .4f;
	auto* Mesh = Actor->FindComponentByClass<USkeletalMeshComponent>();
	auto* Anim = Cast<UTunaSweeperMoleAnimInstance>(Mesh->GetAnimInstance());
	if (Anim)
	{
		bool WasPlaying = false; int32 Starts = 0; float Rest = 0;
		for (int32 Frame = 0; Frame < 720; ++Frame)
		{
			++GFrameCounter; Mesh->TickAnimation(1.0f / 60.0f, false); Mesh->RefreshBoneTransforms();
			const bool Playing = Anim->Montage_IsActive(nullptr);
			if (!WasPlaying) Rest += 1.0f / 60.0f;
			if (Playing && !WasPlaying)
			{
				++Starts;
				TestTrue(TEXT("BP rest bounds apply initially and after every gesture"), Rest >= .18f && Rest <= .45f);
				TestTrue(TEXT("Random selection stays within four gestures"), Anim->GetLastIdleVariationIndex() >= 0 && Anim->GetLastIdleVariationIndex() < 4);
			}
			if (!Playing && WasPlaying) Rest = 0;
			WasPlaying = Playing;
		}
		TestTrue(TEXT("Occasional playback repeats after a completed gesture"), Starts >= 2);
		Actor->IdleVariationMinDelay = 9; Actor->IdleVariationMaxDelay = 3;
		TestEqual(TEXT("Reversed BP bounds are normalized safely"), Actor->GetIdleVariationDelayRange(), FVector2D(9, 9));
	}
	World->DestroyWorld(false); World->RemoveFromRoot();
	return true;
}
#endif
