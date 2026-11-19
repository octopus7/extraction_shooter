#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "EdGraph/EdGraph.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDirectionalLocomotionTest,
    "TunaSweeper.Player.DirectionalLocomotion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDirectionalLocomotionTest::RunTest(const FString& Parameters)
{
    const FString Folder = TEXT("/Game/Characters/Player/LunaMk2/Animations/");
    auto* BP = LoadObject<UAnimBlueprint>(nullptr, *(Folder + TEXT("ABP_LunaMk2")));
    if (!TestNotNull(TEXT("Player animation blueprint"), BP)) return false;
    auto* OriginalBlend = LoadObject<UBlendSpace>(nullptr, *(Folder + TEXT("BS_RunWalk")));
    bool Connected = false;
    TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
    for (auto* Graph : Graphs) for (UEdGraphNode* Node : Graph->Nodes)
        if (auto* Player = Cast<UAnimGraphNode_BlendSpacePlayer>(Node))
        {
            if (OriginalBlend && Player->Node.GetBlendSpace() == OriginalBlend)
                Connected = Player->FindPin(TEXT("X")) && Player->FindPin(TEXT("X"))->LinkedTo.Num() > 0;
        }
    TestTrue(TEXT("Player graph uses original speed-only locomotion"), Connected);
    TestTrue(TEXT("Player animation blueprint compiles"), BP->Status != BS_Error);
    return !HasAnyErrors();
}
#endif
