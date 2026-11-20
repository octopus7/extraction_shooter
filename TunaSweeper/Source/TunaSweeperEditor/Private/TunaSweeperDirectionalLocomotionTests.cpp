#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace.h"
#include "Character/TunaSweeperPlayerAnimInstance.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tests/AutomationEditorCommon.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_BlendListByBool.h"
#include "AnimGraphNode_Slot.h"
#include "EdGraph/EdGraph.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDirectionalLocomotionTest,
    "TunaSweeper.Player.DirectionalLocomotion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDirectionalLocomotionTest::RunTest(const FString&)
{
    const FString Folder = TEXT("/Game/Characters/Player/LunaMk2/Animations/");
    auto* Blueprint = LoadObject<UAnimBlueprint>(nullptr, *(Folder + TEXT("ABP_LunaMk2")));
    auto* Blend = LoadObject<UBlendSpace>(nullptr, *(Folder + TEXT("Unarmed/BS_Luna_Unarmed")));
    if (!TestNotNull(TEXT("Player animation blueprint"), Blueprint) || !TestNotNull(TEXT("Unarmed blend space"), Blend))
        return false;

    // Odd columns are exactly halfway between the neighbouring cardinal clips.
    const TCHAR* Cardinal[] = { TEXT("Bwd"), TEXT("Left"), TEXT("Fwd"), TEXT("Right"), TEXT("Bwd") };
    for (int32 Axis = 0; Axis <= 8; ++Axis)
    {
        for (int32 Gait = 0; Gait <= 2; ++Gait)
        {
            TArray<FBlendSampleData> Samples;
            int32 Cache = INDEX_NONE;
            Blend->GetSamplesFromBlendInput(FVector(-180 + Axis * 45, Gait, 0), Samples, Cache, true);
            TMap<FString, float> Weights;
            for (const auto& Sample : Samples)
            {
                const auto* Clip = Blend->GetBlendSamples()[Sample.SampleDataIndex].Animation.Get();
                Weights.FindOrAdd(GetNameSafe(Clip)) += Sample.TotalWeight;
            }
            if (Gait == 0)
            {
                TestTrue(TEXT("Quinn ready idle at every angle"), Weights.FindRef(TEXT("AS_Luna_Quinn_Idle_Ready")) > .99f);
                continue;
            }
            const FString Prefix = Gait == 1 ? TEXT("AS_Luna_Quinn_Walk_") : TEXT("AS_Luna_Quinn_Jog_");
            const float ExpectedWeight = (Axis % 2) ? .5f : 1.f;
            for (int32 Index = Axis / 2; Index <= (Axis + 1) / 2; ++Index)
            {
                const FString ClipName = Prefix + Cardinal[Index];
                TestTrue(FString::Printf(TEXT("%s weight at %d degrees"), *ClipName, Axis * 45 - 180),
                    FMath::IsNearlyEqual(Weights.FindRef(ClipName), ExpectedWeight, .01f));
            }
        }
    }
    const TArray<FName>* Markers = Blend->GetUniqueMarkerNames();
    TestTrue(TEXT("Quinn moving samples synchronize on both feet"),
        Markers && Markers->Contains(TEXT("L")) && Markers->Contains(TEXT("R")));

    // Both locomotion paths must feed the same downstream montage slot.
    TArray<UEdGraph*> Graphs;
    Blueprint->GetAllGraphs(Graphs);
    bool bSlotConnected = false;
    bool bArmedMovementConnected = false;
    for (UEdGraph* Graph : Graphs)
    {
        for (UEdGraphNode* Node : Graph->Nodes)
        {
            if (auto* Slot = Cast<UAnimGraphNode_Slot>(Node))
            {
                const auto& Sources = Slot->FindPinChecked(TEXT("Source"))->LinkedTo;
                const auto* Select = Sources.Num() == 1 ? Cast<UAnimGraphNode_BlendListByBool>(Sources[0]->GetOwningNode()) : nullptr;
                if (Select)
                {
                    const auto& Unarmed = Select->FindPinChecked(TEXT("BlendPose_0"))->LinkedTo;
                    const auto* Player = Unarmed.Num() == 1 ? Cast<UAnimGraphNode_BlendSpacePlayer>(Unarmed[0]->GetOwningNode()) : nullptr;
                    bSlotConnected = Player && Player->Node.GetBlendSpace() == Blend &&
                        Select->FindPinChecked(TEXT("BlendPose_1"))->LinkedTo.Num() == 1;
                }
            }
            if (const auto* Player = Cast<UAnimGraphNode_BlendSpacePlayer>(Node))
                if (GetNameSafe(Player->Node.GetBlendSpace()) == TEXT("BS_RunWalk"))
                    bArmedMovementConnected = Player->FindPinChecked(TEXT("X"))->LinkedTo.Num() == 1;
        }
    }
    TestTrue(TEXT("Roll slot receives locomotion"), bSlotConnected);
    TestTrue(TEXT("Armed locomotion receives speed"), bArmedMovementConnected);
    TestTrue(TEXT("Blueprint compiles"), Blueprint->Status != BS_Error);

    auto* Class = LoadClass<ATunaSweeperTopDownCharacter>(nullptr,
        TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
    if (!TestNotNull(TEXT("Player class"), Class)) return false;
    UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
    auto* Character = World->SpawnActor<ATunaSweeperTopDownCharacter>(Class);
    if (!TestNotNull(TEXT("Player spawn"), Character)) return false;
    auto* Mesh = Character->GetMesh();
    auto* Anim = Cast<UTunaSweeperPlayerAnimInstance>(Mesh->GetAnimInstance());
    if (!TestNotNull(TEXT("Player uses directional animation instance"), Anim)) return false;
    Character->SetActorRotation(FRotator(0.f, 60.f, 0.f));
    Character->GetCharacterMovement()->Velocity = Character->GetActorRightVector() * Character->GetCharacterMovement()->MaxWalkSpeed;
    FVector FirstFoot = FVector::ZeroVector;
    float Travel = 0.f;
    for (int32 Frame = 0; Frame < 60; ++Frame)
    {
        ++GFrameCounter;
        Mesh->TickAnimation(1.f / 60.f, false);
        Mesh->RefreshBoneTransforms();
        const FVector Foot = Mesh->GetSocketTransform(TEXT("foot_l"), RTS_Component).GetLocation();
        if (Frame == 20) FirstFoot = Foot;
        if (Frame > 20) Travel = FMath::Max(Travel, float(FVector::Distance(FirstFoot, Foot)));
    }
    TestTrue(TEXT("Empty hands select unarmed locomotion"), Anim->bUseUnarmedLocomotion);
    TestTrue(TEXT("Direction follows character facing"), FMath::IsNearlyEqual(Anim->TravelAngle, 90.f, .01f));
    TestTrue(TEXT("Full walking speed selects walking row"), FMath::IsNearlyEqual(Anim->UnarmedGait, 1.f, .01f));
    TestTrue(TEXT("Evaluated foot moves across the loop on the actual player mesh"), Travel > 3.f);
    return !HasAnyErrors();
}
#endif
