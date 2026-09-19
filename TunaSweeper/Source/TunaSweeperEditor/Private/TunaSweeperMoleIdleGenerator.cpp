// One-off generation: commit the assets with this source, then remove this file.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Character/TunaSweeperMoleAnimInstance.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/BlendSpace.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"
#include "Factories/AnimBlueprintFactory.h"
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "AnimGraphNode_Slot.h"
#include "AnimGraphNode_LayeredBoneBlend.h"
#include "K2Node_VariableGet.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace MoleIdleGeneration
{
const FString Folder = TEXT("/Game/Characters/NPC/Mole/");
bool Save(UObject* Asset)
{
	Asset->MarkPackageDirty();
	FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
	return UPackage::SavePackage(Asset->GetOutermost(), Asset,
		*FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
template<class T> T* Node(UEdGraph* Graph, int32 X, int32 Y)
{
	FGraphNodeCreator<T> Creator(*Graph);
	T* Result = Creator.CreateNode();
	Result->NodePosX = X; Result->NodePosY = Y;
	Creator.Finalize();
	return Result;
}
UEdGraphPin* Output(UEdGraphNode* Node)
{
	for (UEdGraphPin* Pin : Node->Pins) if (Pin->Direction == EGPD_Output) return Pin;
	return nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMoleIdleGenerator,
	"TunaSweeper.Generate.MoleIdleVariations", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperMoleIdleGenerator::RunTest(const FString& Parameters)
{
	using namespace MoleIdleGeneration;
	UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, *(Folder + TEXT("A_Mole_Idle_Breathe")));
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, *(Folder + TEXT("SKM_MoleDummy")));
	if (!TestNotNull(TEXT("Idle source"), Idle) || !TestNotNull(TEXT("Mole mesh"), Mesh)) return false;
	const TCHAR* Names[] = {TEXT("Sniff"), TEXT("ShoulderRoll"), TEXT("HeadTilt"), TEXT("PawWave")};
	// Fail before writing if any destination already exists.
	for (const TCHAR* Name : Names)
		if (FPackageName::DoesPackageExist(Folder + TEXT("A_Mole_Idle_") + Name)) { AddError(TEXT("Destination already exists")); return false; }
	if (FPackageName::DoesPackageExist(Folder + TEXT("ABP_MoleCompanion"))) { AddError(TEXT("AnimBP already exists")); return false; }
	const auto& Ref = Mesh->GetRefSkeleton();
	TArray<FTransform> ComponentRef = Ref.GetRefBonePose();
	for (int32 I = 1; I < ComponentRef.Num(); ++I) ComponentRef[I] *= ComponentRef[Ref.GetParentIndex(I)];
	TArray<TObjectPtr<UAnimSequence>> Clips;
	for (int32 Gesture = 0; Gesture < 4; ++Gesture)
	{
		const FString Name = TEXT("A_Mole_Idle_") + FString(Names[Gesture]);
		auto* Clip = DuplicateObject<UAnimSequence>(Idle, CreatePackage(*(Folder + Name)), *Name);
		Clip->SetFlags(RF_Public | RF_Standalone);
		for (const TCHAR* Bone : {TEXT("spine"), TEXT("head"), TEXT("upper_arm_L"), TEXT("upper_arm_R"), TEXT("forearm_L"), TEXT("forearm_R"), TEXT("hand_R")})
		{
			TArray<FTransform> Keys; Idle->GetDataModel()->GetBoneTrackTransforms(Bone, Keys);
			const int32 BoneIndex = Ref.FindBoneIndex(Bone);
			if (!TestTrue(TEXT("Authored bone is present"), BoneIndex != INDEX_NONE && !Keys.IsEmpty())) return false;
			TArray<FVector> Positions, Scales; TArray<FQuat> Rotations;
			for (int32 Frame = 0; Frame < Keys.Num(); ++Frame)
			{
				const float U = FMath::Clamp((Frame / 30.0f - .3f) / 3.4f, 0.0f, 1.0f);
				const float Envelope = FMath::Square(FMath::Sin(PI * U));
				const float Wave = FMath::Sin(2 * PI * U);
				FVector Degrees = FVector::ZeroVector; // component-space X/Y/Z rotation axes
				const FString B(Bone);
				if (Gesture == 0)
				{
					if (B == TEXT("head")) Degrees = FVector(7 + 3 * FMath::Sin(8 * PI * U), 0, 20 * Wave);
					if (B == TEXT("spine")) Degrees = FVector(3, 0, 4 * Wave);
				}
				else if (Gesture == 1)
				{
					if (B.StartsWith(TEXT("upper_arm"))) Degrees = FVector(18 * Wave, B.EndsWith(TEXT("L")) ? 12 : -12, 0);
					if (B.StartsWith(TEXT("forearm"))) Degrees.X = 15;
					if (B == TEXT("spine")) Degrees.X = -4;
				}
				else if (Gesture == 2)
				{
					if (B == TEXT("head")) Degrees = FVector(-5, 16 * Wave, 5);
					if (B == TEXT("spine")) Degrees.Y = -3 * Wave;
				}
				else
				{
					if (B == TEXT("upper_arm_R")) Degrees = FVector(35, -18, 0);
					if (B == TEXT("forearm_R")) Degrees.X = 45;
					if (B == TEXT("hand_R")) Degrees.Y = 25 * FMath::Sin(6 * PI * U);
					if (B == TEXT("head")) Degrees.Y = -6;
				}
				Degrees *= Envelope;
				const FQuat Delta = FQuat(FVector::UpVector, FMath::DegreesToRadians(Degrees.Z))
					* FQuat(FVector::RightVector, FMath::DegreesToRadians(Degrees.Y))
					* FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Degrees.X));
				const FQuat Basis = ComponentRef[BoneIndex].GetRotation();
				Positions.Add(Keys[Frame].GetTranslation()); Scales.Add(Keys[Frame].GetScale3D());
				Rotations.Add((Keys[Frame].GetRotation() * Basis.Inverse() * Delta * Basis).GetNormalized());
			}
			if (!Clip->GetController().SetBoneTrackKeys(Bone, Positions, Rotations, Scales, false)) return false;
		}
		Clip->bEnableRootMotion = false;
		Clip->bForceRootLock = Idle->bForceRootLock;
		Clip->RootMotionRootLock = Idle->RootMotionRootLock;
		Clip->PostEditChange(); FAssetRegistryModule::AssetCreated(Clip);
		if (!Save(Clip)) return false;
		Clips.Add(Clip);
	}
	USkeleton* Skeleton = Idle->GetSkeleton();
	Skeleton->SetSlotGroupName(UTunaSweeperMoleAnimInstance::UpperBodySlotName(), TEXT("MoleIdle"));
	if (!Save(Skeleton)) return false;
	auto* Factory = NewObject<UAnimBlueprintFactory>();
	Factory->ParentClass = UTunaSweeperMoleAnimInstance::StaticClass();
	Factory->TargetSkeleton = Skeleton; Factory->PreviewSkeletalMesh = Mesh;
	auto* BP = CastChecked<UAnimBlueprint>(Factory->FactoryCreateNew(UAnimBlueprint::StaticClass(),
		CreatePackage(*(Folder + TEXT("ABP_MoleCompanion"))), TEXT("ABP_MoleCompanion"), RF_Public | RF_Standalone, nullptr, GWarn));
	UEdGraph* Graph = nullptr;
	TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
	for (auto* Candidate : Graphs) if (Candidate->GetFName() == TEXT("AnimGraph")) Graph = Candidate;
	if (!TestNotNull(TEXT("Anim graph"), Graph)) return false;
	UAnimGraphNode_Root* Root = nullptr;
	for (UEdGraphNode* N : Graph->Nodes) if (auto* R = Cast<UAnimGraphNode_Root>(N)) Root = R;
	if (!TestNotNull(TEXT("Output pose"), Root)) return false;
	Root->NodePosX = 900;
	auto* Player = Node<UAnimGraphNode_BlendSpacePlayer>(Graph, -800, 0);
	Player->Node.SetBlendSpace(LoadObject<UBlendSpace>(nullptr, *(Folder + TEXT("BS_Mole_IdleTurn"))));
	for (auto& Pin : Player->ShowPinForProperties) if (Pin.PropertyName == TEXT("PlayRate")) Pin.bShowPin = true;
	Player->ReconstructNode();
	auto* Cache = Node<UAnimGraphNode_SaveCachedPose>(Graph, -400, 0);
	Cache->CacheName = TEXT("BreathingAndTurning");
	auto* Base = Node<UAnimGraphNode_UseCachedPose>(Graph, -200, 200);
	Base->SaveCachedPoseNode = Cache; Base->ReconstructNode();
	auto* Source = Node<UAnimGraphNode_UseCachedPose>(Graph, -200, 400);
	Source->SaveCachedPoseNode = Cache; Source->ReconstructNode();
	auto* Slot = Node<UAnimGraphNode_Slot>(Graph, 100, 400);
	Slot->Node.SlotName = UTunaSweeperMoleAnimInstance::UpperBodySlotName();
	Slot->Node.bAlwaysUpdateSourcePose = true;
	auto* Layer = Node<UAnimGraphNode_LayeredBoneBlend>(Graph, 500, 200);
	if (Layer->Node.BlendPoses.IsEmpty()) Layer->Node.AddPose();
	FBranchFilter Filter; Filter.BoneName = TEXT("spine"); Filter.BlendDepth = 1;
	Layer->Node.LayerSetup[0].BranchFilters = {Filter};
	Layer->Node.BlendWeights[0] = 1.0f;
	Layer->Node.bMeshSpaceRotationBlend = true;
	Layer->ReconstructNode();
	auto Connect = [&](UEdGraphNode* From, UEdGraphNode* To, const TCHAR* Input)
	{
		auto* A = Output(From); auto* B = To->FindPin(Input);
		return TestTrue(*FString::Printf(TEXT("Connect %s"), Input), A && B && Graph->GetSchema()->TryCreateConnection(A, B));
	};
	auto Variable = [&](FName Name, int32 Y)
	{
		auto* V = Node<UK2Node_VariableGet>(Graph, -1100, Y);
		V->VariableReference.SetSelfMember(Name); V->ReconstructNode(); return V;
	};
	if (!Connect(Variable(TEXT("TurnAmount"), 0), Player, TEXT("X"))
		|| !Connect(Variable(TEXT("TurnPlayRate"), 150), Player, TEXT("PlayRate"))
		|| !Connect(Player, Cache, TEXT("Pose")) || !Connect(Source, Slot, TEXT("Source"))
		|| !Connect(Base, Layer, TEXT("BasePose")) || !Connect(Slot, Layer, TEXT("BlendPoses_0"))
		|| !Connect(Layer, Root, TEXT("Result"))) return false;
	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
	FKismetEditorUtilities::CompileBlueprint(BP);
	if (!TestTrue(TEXT("AnimBP compiled"), BP->Status != BS_Error)) return false;
	CastChecked<UTunaSweeperMoleAnimInstance>(BP->GeneratedClass->GetDefaultObject())->IdleVariations = Clips;
	FAssetRegistryModule::AssetCreated(BP);
	return Save(BP);
}
#endif
