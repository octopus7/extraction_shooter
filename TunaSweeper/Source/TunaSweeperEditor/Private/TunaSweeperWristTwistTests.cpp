#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWristTwistTest, "TunaSweeper.Player.WristTwist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWristTwistTest::RunTest(const FString& Parameters)
{
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2"));
    if (!TestNotNull(TEXT("Player mesh loads"),Mesh)) return false;
    if (!TestNotNull(TEXT("Weighted twist bones have a post-process driver"),Mesh->GetPostProcessAnimBlueprint().Get())) return false;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Actor=World->SpawnActor<AActor>();
    auto MakeComponent=[&](bool Disable)
    {
        auto* C=NewObject<USkeletalMeshComponent>(Actor);
        C->SetSkeletalMeshAsset(Mesh);
        C->SetDisablePostProcessBlueprint(Disable);
        C->RegisterComponent();
        C->SetComponentTickEnabled(false);
        C->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        return C;
    };
    auto* Before=MakeComponent(true);
    auto* After=MakeComponent(false);
    const auto& Ref=Mesh->GetRefSkeleton();
    const TArray<FName> Driven={TEXT("cc_base_l_forearmtwist01"),TEXT("cc_base_l_forearmtwist02"),
        TEXT("cc_base_r_forearmtwist01"),TEXT("cc_base_r_forearmtwist02")};
    for (const FName Name : Driven)
        TestTrue(TEXT("Required twist bone exists"),Ref.FindBoneIndex(Name)!=INDEX_NONE);
    for (const FName Name : {FName(TEXT("hand_l")),FName(TEXT("hand_r"))})
        TestTrue(TEXT("Required hand bone exists"),Ref.FindBoneIndex(Name)!=INDEX_NONE);
    if (HasAnyErrors())
    { GEngine->DestroyWorldContext(World);World->DestroyWorld(false);return false; }
    for (auto* C : {Before,After}) { C->TickAnimation(0.f,false);C->RefreshBoneTransforms(); }
    const auto& RestBefore=Before->GetComponentSpaceTransforms();
    const auto& RestAfter=After->GetComponentSpaceTransforms();
    for (int32 I=0;I<FMath::Min(RestBefore.Num(),RestAfter.Num());++I)
        TestTrue(TEXT("Reference pose is unchanged"),RestBefore[I].Equals(RestAfter[I],.001));
    double MaxChange=0.;
    for (const TCHAR* ClipName : {TEXT("MF_Rifle_Idle_Hipfire1"),TEXT("Title/AS_LunaMk2_Title_A"),TEXT("Title/AS_LunaMk2_Title_C")})
    {
        auto* Clip=LoadObject<UAnimSequence>(nullptr,*(FString(TEXT("/Game/Characters/Player/LunaMk2/Animations/"))+ClipName));
        if (!TestNotNull(TEXT("Input clip loads"),Clip)) continue;
        Before->PlayAnimation(Clip,false);After->PlayAnimation(Clip,false);
        const int32 Samples=FMath::Max(1,FMath::CeilToInt(Clip->GetPlayLength()*30.f));
        for (int32 Sample=0;Sample<=Samples;++Sample)
        {
            ++GFrameCounter;
            for (auto* C : {Before,After})
            {
                C->SetPosition(Clip->GetPlayLength()*Sample/Samples,false);
                C->TickAnimation(0.f,false);C->RefreshBoneTransforms();
            }
            TestNotNull(TEXT("Post-process instance evaluates"),After->GetPostProcessInstance());
            const auto& A=Before->GetComponentSpaceTransforms();
            const auto& B=After->GetComponentSpaceTransforms();
            if (!TestEqual(TEXT("Before pose contains full skeleton"),A.Num(),Ref.GetNum()) ||
                !TestEqual(TEXT("After pose contains full skeleton"),B.Num(),Ref.GetNum())) break;
            for (int32 I=0;I<Ref.GetNum();++I)
            {
                TestFalse(TEXT("Pose remains finite"),B[I].ContainsNaN());
                if (!Driven.Contains(Ref.GetBoneName(I)))
                    TestTrue(*FString::Printf(TEXT("Input pose preserved: %s"),*Ref.GetBoneName(I).ToString()),A[I].Equals(B[I],.001));
            }
            for (const TCHAR* Side : {TEXT("l"),TEXT("r")})
            {
                int32 Hand=Ref.FindBoneIndex(*FString::Printf(TEXT("hand_%s"),Side));
                int32 Arm=Ref.GetParentIndex(Hand);
                FQuat Local=A[Arm].GetRotation().Inverse()*A[Hand].GetRotation();
                double HandRoll=(Local*Ref.GetRefBonePose()[Hand].GetRotation().Inverse()).Euler().X;
                for (int32 Segment=1;Segment<=2;++Segment)
                {
                    int32 Twist=Ref.FindBoneIndex(*FString::Printf(TEXT("cc_base_%s_forearmtwist%02d"),Side,Segment));
                    double Change=FMath::RadiansToDegrees(A[Twist].GetRotation().AngularDistance(B[Twist].GetRotation()));
                    MaxChange=FMath::Max(MaxChange,Change);
                    // Two serial bones distribute the hand roll into approximately thirds.
                    TestTrue(TEXT("Twist receives its share without doubling the parent contribution"),
                        FMath::Abs(Change-FMath::Abs(HandRoll)*Segment/3.)<.1);
                    const FQuat Correction=A[Arm].GetRotation().Inverse()*B[Twist].GetRotation()*
                        A[Twist].GetRotation().Inverse()*A[Arm].GetRotation();
                    // UE Euler roll uses the opposite sign to quaternion axis-angle X.
                    const FQuat Expected=FQuat::MakeFromEuler(FVector(HandRoll*Segment/3.,0.,0.));
                    TestTrue(TEXT("Correction follows the signed forearm axis"),Correction.AngularDistance(Expected)<.002);
                }
            }
        }
    }
    TestTrue(TEXT("Reproduction actually exercises substantial wrist twist"),MaxChange>20.);
    AddInfo(FString::Printf(TEXT("Maximum forearm correction %.3f degrees; hands and non-twist bones preserved"),MaxChange));
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
