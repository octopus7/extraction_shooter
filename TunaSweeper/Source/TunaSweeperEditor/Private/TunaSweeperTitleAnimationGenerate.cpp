// One-off asset creation; removed in the commit immediately following generated assets.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Title/TunaSweeperTitleAnimInstance.h"
#include "Title/TunaSweeperTitlePresentationActor.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimSequence.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_SequenceEvaluator.h"
#include "AnimGraphNode_RigidBody.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_SkeletalControlBase.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "Factories/AnimBlueprintFactory.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "FileHelpers.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenerateTitleAnimBP,"TunaSweeper.Title.Animation.Generate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenerateTitleAnimBP::RunTest(const FString& Parameters)
{
	const FString Path=TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/ABP_LunaMk2_Title");
	auto Save=[](UObject* Asset)
	{
		auto* Package=Asset->GetOutermost();FSavePackageArgs Args;
		Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
		return UPackage::SavePackage(Package,Asset,*FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension()),Args);
	};
	auto* Source=LoadObject<UAnimBlueprint>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/Animations/ABP_LunaMk2"));
	if (!TestNotNull(TEXT("Original player ABP"),Source)) return false;
	TArray<UAnimGraphNode_RigidBody*> Bodies;FBlueprintEditorUtils::GetAllNodesOfClass(Source,Bodies);
	TArray<UAnimGraphNode_SkeletalControlBase*> Controls;FBlueprintEditorUtils::GetAllNodesOfClass(Source,Controls);
	UAnimGraphNode_SkeletalControlBase* SourceLook=nullptr;
	for (auto* Node:Controls) if (Node->GetClass()->GetName().Contains(TEXT("TitleHeadLook"))) SourceLook=Node;
	if (!TestNotNull(TEXT("Existing gaze node"),SourceLook) || !TestEqual(TEXT("Existing hair physics node"),Bodies.Num(),1)) return false;
	auto* BP=LoadObject<UAnimBlueprint>(nullptr,*Path);
	if (!BP)
	{
		auto* Factory=NewObject<UAnimBlueprintFactory>();Factory->ParentClass=UTunaSweeperTitleAnimInstance::StaticClass();Factory->TargetSkeleton=Source->TargetSkeleton;
		BP=Cast<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(),CreatePackage(*Path),TEXT("ABP_LunaMk2_Title"),RF_Public|RF_Standalone,nullptr,GWarn));
		FAssetRegistryModule::AssetCreated(BP);
	}
	if (!TestNotNull(TEXT("Title ABP created"),BP)) return false;
	BP->ParentClass=UTunaSweeperTitleAnimInstance::StaticClass();BP->TargetSkeleton=Source->TargetSkeleton;
	UEdGraph* Graph=nullptr;
	for (const auto& G:BP->FunctionGraphs) if (G->GetFName()==TEXT("AnimGraph")) Graph=G.Get();
	if (!Graph)
	{
		Graph=FBlueprintEditorUtils::CreateNewGraph(BP,TEXT("AnimGraph"),UAnimationGraph::StaticClass(),UAnimationGraphSchema::StaticClass());
		BP->FunctionGraphs.Add(Graph);
	}
	for (const auto& Node:TArray<TObjectPtr<UEdGraphNode>>(Graph->Nodes)) FBlueprintEditorUtils::RemoveNode(BP,Node.Get(),true);
	auto Make=[&]<typename T>(int32 X,int32 Y)
	{
		FGraphNodeCreator<T> Creator(*Graph);auto* Node=Creator.CreateNode();Node->NodePosX=X;Node->NodePosY=Y;Creator.Finalize();return Node;
	};
	auto* Eval=Make.operator()<UAnimGraphNode_SequenceEvaluator>(0,0);
	Eval->SetAnimationAsset(GetDefault<UTunaSweeperTitleAnimInstance>()->Entrance);
	Eval->Node.SetShouldLoop(false);Eval->Node.SetTeleportToExplicitTime(true);
	for (auto& Pin:Eval->ShowPinForProperties) if (Pin.PropertyName==TEXT("Sequence")||Pin.PropertyName==TEXT("ExplicitTime")) Pin.bShowPin=true;
	Eval->ReconstructNode();
	auto* Local=Make.operator()<UAnimGraphNode_LocalToComponentSpace>(300,0);
	auto* Look=DuplicateObject<UAnimGraphNode_SkeletalControlBase>(SourceLook,Graph,TEXT("TitleHeadLook"));
	Look->CreateNewGuid();Look->NodePosX=550;Look->NodePosY=0;
	for (auto* Pin:Look->Pins) Pin->LinkedTo.Reset();
	Graph->AddNode(Look,false,false);
	for (auto& Pin:Look->ShowPinForProperties) if (Pin.PropertyName==TEXT("Alpha")) Pin.bShowPin=true;
	Look->ReconstructNode();
	auto* Physics=Make.operator()<UAnimGraphNode_RigidBody>(800,0);Physics->Node=Bodies[0]->Node;Physics->ReconstructNode();
	auto* ToLocal=Make.operator()<UAnimGraphNode_ComponentToLocalSpace>(1100,0);
	auto* Root=Make.operator()<UAnimGraphNode_Root>(1400,0);
	auto Out=[](UEdGraphNode* Node)->UEdGraphPin* {for (auto* Pin:Node->Pins) if (Pin->Direction==EGPD_Output) return Pin;return nullptr;};
	auto Link=[&](UEdGraphPin* A,UEdGraphPin* B)
	{
		return TestTrue(TEXT("Graph connection"),A&&B&&Graph->GetSchema()->TryCreateConnection(A,B));
	};
	if (!Link(Out(Eval),Local->FindPin(TEXT("LocalPose"))) || !Link(Out(Local),Look->FindPin(TEXT("ComponentPose")))
		|| !Link(Out(Look),Physics->FindPin(TEXT("ComponentPose"))) || !Link(Out(Physics),ToLocal->FindPin(TEXT("ComponentPose")))
		|| !Link(Out(ToLocal),Root->FindPin(TEXT("Result")))) return false;
	auto Bind=[&](FName Name,UEdGraphPin* Target,int32 X,int32 Y)
	{
		FGraphNodeCreator<UK2Node_VariableGet> Creator(*Graph);auto* Var=Creator.CreateNode();Var->VariableReference.SetSelfMember(Name);Var->NodePosX=X;Var->NodePosY=Y;Creator.Finalize();
		return Link(Out(Var),Target);
	};
	if (!Bind(TEXT("CurrentSequence"),Eval->FindPin(TEXT("Sequence")),-320,0)
		|| !Bind(TEXT("CurrentSequenceTime"),Eval->FindPin(TEXT("ExplicitTime")),-320,140)
		|| !Bind(TEXT("HeadLookAlpha"),Look->FindPin(TEXT("Alpha")),300,180)) return false;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);FKismetEditorUtilities::CompileBlueprint(BP);
	if (!TestTrue(TEXT("Title ABP compiles"),BP->Status!=BS_Error) || !TestTrue(TEXT("Title ABP saved"),Save(BP))) return false;
	auto* TitleBP=LoadObject<UBlueprint>(nullptr,TEXT("/Game/UI/Title/BP_TitlePresentationActor"));
	if (!TestNotNull(TEXT("Title actor BP"),TitleBP)) return false;
	auto* CDO=Cast<ATunaSweeperTitlePresentationActor>(TitleBP->GeneratedClass->GetDefaultObject());
	auto SetClass=[&](ATunaSweeperTitlePresentationActor* Actor)
	{
		auto* Mesh=Actor->FindComponentByClass<UTunaSweeperTitleSkeletalMeshComponent>();
		if (Mesh) {Mesh->SetAnimationMode(EAnimationMode::AnimationBlueprint);Mesh->SetAnimInstanceClass(BP->GeneratedClass);}
	};
	SetClass(CDO);FBlueprintEditorUtils::MarkBlueprintAsModified(TitleBP);FKismetEditorUtilities::CompileBlueprint(TitleBP);SetClass(CastChecked<ATunaSweeperTitlePresentationActor>(TitleBP->GeneratedClass->GetDefaultObject()));
	if (!TestTrue(TEXT("Title actor saved"),Save(TitleBP))) return false;
	if (!TestTrue(TEXT("IntroMap loaded"),FEditorFileUtils::LoadMap(TEXT("/Game/Maps/IntroMap"),false,true))) return false;
	auto* World=GEditor->GetEditorWorldContext().World();int32 Count=0;
	for (TActorIterator<ATunaSweeperTitlePresentationActor> It(World);It;++It) {SetClass(*It);It->MarkPackageDirty();++Count;}
	TestTrue(TEXT("Title actor found"),Count>0);
	TestTrue(TEXT("IntroMap binding saved"),FEditorFileUtils::SaveLevel(World->PersistentLevel));
	return !HasAnyErrors();
}
#endif
