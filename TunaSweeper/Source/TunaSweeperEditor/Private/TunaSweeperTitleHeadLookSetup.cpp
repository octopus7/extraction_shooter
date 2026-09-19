// One-off migration, removed immediately after committing the verified asset.
#if WITH_DEV_AUTOMATION_TESTS
#include "AnimGraphNode_TunaSweeperTitleHeadLook.h"
#include "AnimGraphNode_RigidBody.h"
#include "Animation/AnimBlueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Subsystems/EditorAssetSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperTitleHeadLookSetup,
	"TunaSweeper.Setup.TitleHeadLook", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperTitleHeadLookSetup::RunTest(const FString& Parameters)
{
	auto* Blueprint = LoadObject<UAnimBlueprint>(nullptr,
		TEXT("/Game/Characters/Player/LunaMk2/Animations/ABP_LunaMk2.ABP_LunaMk2"));
	if (!TestNotNull(TEXT("Luna AnimBP"), Blueprint)) return false;
	TArray<UEdGraph*> Graphs;
	Blueprint->GetAllGraphs(Graphs);
	UAnimGraphNode_RigidBody* RigidBody = nullptr;
	for (auto* Graph : Graphs)
		for (UEdGraphNode* GraphNode : Graph->Nodes)
			if (auto* Candidate = Cast<UAnimGraphNode_RigidBody>(GraphNode))
				if (Candidate->Node.OverridePhysicsAsset && Candidate->Node.OverridePhysicsAsset->GetName() == TEXT("PA_LunaMk2_SideTail"))
				{
					if (!TestNull(TEXT("Only one side-tail physics node"), RigidBody)) return false;
					RigidBody = Candidate;
				}
	if (!TestNotNull(TEXT("Side-tail physics node"), RigidBody)) return false;
	auto Pin = [](UEdGraphNode* Node, EEdGraphPinDirection Direction) -> UEdGraphPin*
	{
		for (auto* Candidate : Node->Pins)
			if (Candidate->Direction == Direction && Candidate->PinType.PinSubCategoryObject == FComponentSpacePoseLink::StaticStruct()) return Candidate;
		return nullptr;
	};
	UEdGraphPin* Input = Pin(RigidBody, EGPD_Input);
	if (!Input || !TestEqual(TEXT("Physics has a single pose input"), Input->LinkedTo.Num(), 1)) return false;
	if (Cast<UAnimGraphNode_TunaSweeperTitleHeadLook>(Input->LinkedTo[0]->GetOwningNode())) return true;
	UEdGraph* Graph = RigidBody->GetGraph();
	Blueprint->Modify();
	Graph->Modify();
	UEdGraphPin* Previous = Input->LinkedTo[0];
	FGraphNodeCreator<UAnimGraphNode_TunaSweeperTitleHeadLook> Creator(*Graph);
	auto* HeadLook = Creator.CreateNode();
	HeadLook->NodePosX = RigidBody->NodePosX - 240;
	HeadLook->NodePosY = RigidBody->NodePosY;
	Creator.Finalize();
	Input->BreakAllPinLinks();
	if (!Graph->GetSchema()->TryCreateConnection(Previous, Pin(HeadLook, EGPD_Input))
		|| !Graph->GetSchema()->TryCreateConnection(Pin(HeadLook, EGPD_Output), Input))
	{
		AddError(TEXT("Head-look pose connection failed; asset has not been saved"));
		return false;
	}
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (!TestTrue(TEXT("AnimBP compiles"), Blueprint->Status != BS_Error && Blueprint->GeneratedClass)) return false;
	return TestTrue(TEXT("Save migrated AnimBP"), GEditor->GetEditorSubsystem<UEditorAssetSubsystem>()->SaveLoadedAsset(Blueprint, false));
}
#endif
