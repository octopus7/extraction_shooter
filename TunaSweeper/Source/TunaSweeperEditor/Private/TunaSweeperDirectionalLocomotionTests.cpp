#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "EdGraph/EdGraph.h"
#include "Tests/AutomationEditorCommon.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimInstance.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDirectionalLocomotionTest,
    "TunaSweeper.Player.DirectionalLocomotion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDirectionalLocomotionTest::RunTest(const FString& Parameters)
{
    const FString Folder = TEXT("/Game/Characters/Player/LunaMk2/Animations/");
    auto* Blend = LoadObject<UBlendSpace>(nullptr, *(Folder + TEXT("BS_DirectionalWalk")));
    if (!TestNotNull(TEXT("Directional locomotion blend exists"), Blend)) return false;
    const TCHAR* Names[] = {TEXT("AS_Luna_WalkBackward"), TEXT("AS_Luna_StrafeLeft"), TEXT("AS_Luna_StrafeRight")};
    const float Angles[] = {180.f, -90.f, 90.f};
    for (int32 I = 0; I < 3; ++I)
    {
        auto* Clip = LoadObject<UAnimSequence>(nullptr, *(Folder + Names[I]));
        if (!TestNotNull(Names[I], Clip)) return false;
        TestTrue(TEXT("Clip has a usable duration"), Clip->GetPlayLength() > .3f);
        TestFalse(TEXT("In-place motion must not move the capsule"), Clip->bEnableRootMotion);
        TArray<FName> Bones; Clip->GetDataModel()->GetBoneTrackNames(Bones);
        for (FName Bone : Bones)
        {
            TArray<FTransform> Keys; Clip->GetDataModel()->GetBoneTrackTransforms(Bone, Keys);
            for (const FTransform& Key : Keys) if (Key.ContainsNaN()) { AddError(TEXT("Invalid bone transform")); return false; }
            if (Keys.Num() > 1) TestTrue(TEXT("Motion loops without a pose jump: ") + Bone.ToString(), Keys[0].Equals(Keys.Last(), .002f));
        }
        for (float Speed : {200.f, 464.141312f, 600.f})
        {
            TArray<FBlendSampleData> Samples; int32 Index = INDEX_NONE;
            Blend->GetSamplesFromBlendInput(FVector(Angles[I], Speed, 0), Samples, Index, true);
            float Weight = 0;
            for (const auto& Sample : Samples) if (Blend->GetBlendSamples()[Sample.SampleDataIndex].Animation == Clip) Weight += Sample.TotalWeight;
            TestTrue(TEXT("Cardinal movement selects its directional clip"), Weight > .99f);
        }
        // Stance covers half the cycle. Incorrect rate scale makes a planted foot slide.
        for (const auto& Sample : Blend->GetBlendSamples())
            if (Sample.Animation == Clip)
            {
                const float StanceSpeed = I == 0 ? 95.f : 55.f;
                TestTrue(TEXT("Playback compensates capsule travel during stance"),
                    FMath::IsNearlyEqual(Sample.RateScale * StanceSpeed, float(Sample.SampleValue.Y), .01f));
            }
    }
    auto* BP = LoadObject<UAnimBlueprint>(nullptr, *(Folder + TEXT("ABP_LunaMk2")));
    if (!TestNotNull(TEXT("Player animation blueprint"), BP)) return false;
    bool Connected = false;
    TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
    for (auto* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Player = Cast<UAnimGraphNode_BlendSpacePlayer>(Node))
            if (Player->Node.GetBlendSpace() == Blend)
                Connected = Player->FindPin(TEXT("X"))->LinkedTo.Num() > 0 && Player->FindPin(TEXT("Y"))->LinkedTo.Num() > 0;
    TestTrue(TEXT("Player graph consumes direction and speed"), Connected);
    TestTrue(TEXT("Player animation blueprint compiles"), BP->Status != BS_Error);
    // Exercise the serialized event graph on the actual player, including actor-relative yaw.
    UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
    UClass* PlayerClass = LoadClass<ACharacter>(nullptr, TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
    auto* Character = World->SpawnActor<ACharacter>(PlayerClass);
    if (!TestNotNull(TEXT("Player spawned for graph evaluation"), Character)) return false;
    auto* Mesh = Character->GetMesh(); Mesh->InitAnim(true);
    auto* Instance = Mesh->GetAnimInstance();
    if (!TestNotNull(TEXT("Player animation instance"), Instance)) return false;
    auto* Direction = FindFProperty<FFloatProperty>(Instance->GetClass(), TEXT("LocomotionDirection"));
    if (!TestNotNull(TEXT("Runtime direction variable"), Direction)) return false;
    struct FCase { float Yaw; FVector Velocity; float Expected; };
    const FCase Cases[] = {{0, FVector(200,0,0), 0}, {0, FVector(0,200,0), 90},
        {0, FVector(0,-200,0), -90}, {0, FVector(-200,0,0), 180},
        {90, FVector(0,200,0), 0}, {90, FVector(-200,0,0), 90},
        {90, FVector(200,0,0), -90}, {0, FVector(0,0,0), 0}};
    for (const auto& Case : Cases)
    {
        Character->SetActorRotation(FRotator(0, Case.Yaw, 0));
        Character->GetCharacterMovement()->Velocity = Case.Velocity;
        Instance->UpdateAnimation(.016f, false);
        const float Actual = Direction->GetPropertyValue_InContainer(Instance);
        TestTrue(TEXT("Movement direction follows character facing"), FMath::Abs(FMath::FindDeltaAngleDegrees(Actual, Case.Expected)) < .1f);
    }
    Character->Destroy();
    return !HasAnyErrors();
}
#endif
