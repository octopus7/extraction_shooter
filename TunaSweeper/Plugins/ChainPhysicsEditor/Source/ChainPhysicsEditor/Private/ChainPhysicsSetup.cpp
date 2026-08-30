#include "ChainPhysicsSetup.h"

#include "AnimGraphNode_ComponentToLocalSpace.h"
#include "AnimGraphNode_LocalToComponentSpace.h"
#include "AnimGraphNode_RigidBody.h"
#include "AnimGraphNode_Root.h"
#include "Animation/AnimBlueprint.h"
#include "AnimationGraph.h"
#include "AnimationGraphSchema.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "PhysicsAssetUtils.h"
#include "PhysicsAssetGenerationSettings.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"

namespace
{
	using namespace ChainPhysicsSetup;

	struct FPresetValues
	{
		float RadiusRatio;
		float MinRadius;
		float MaxRadius;
		float LinearDamping;
		float AngularDamping;
		float MassScale;
		float RootSwing1;
		float RootSwing2;
		float RootTwist;
		float Swing1;
		float Swing2;
		float Twist;
		float RootSpring;
		float RootDriveDamping;
		float Spring;
		float DriveDamping;
	};

	FPresetValues GetPresetValues(const EPreset Preset)
	{
		switch (Preset)
		{
		case EPreset::Accessory:
			return { 0.24f, 1.5f, 3.0f, 1.0f, 7.0f, 0.35f, 12.0f, 16.0f, 8.0f, 22.0f, 26.0f, 12.0f, 80.0f, 12.0f, 55.0f, 8.0f };
		case EPreset::Cloth:
			return { 0.15f, 0.8f, 1.8f, 0.5f, 3.0f, 0.12f, 22.0f, 28.0f, 14.0f, 38.0f, 45.0f, 22.0f, 35.0f, 6.0f, 18.0f, 3.5f };
		default:
			return { 0.20f, 1.2f, 2.0f, 0.8f, 5.0f, 0.20f, 18.0f, 22.0f, 12.0f, 30.0f, 36.0f, 18.0f, 55.0f, 8.0f, 35.0f, 5.0f };
		}
	}

	bool SaveAsset(UObject* Asset)
	{
		UEditorAssetSubsystem* Subsystem = GEditor ? GEditor->GetEditorSubsystem<UEditorAssetSubsystem>() : nullptr;
		return Subsystem && Asset && Subsystem->SaveLoadedAsset(Asset, false);
	}

	UEdGraphPin* FindPosePin(UEdGraphNode* Node, const EEdGraphPinDirection Direction)
	{
		if (!Node)
		{
			return nullptr;
		}
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == Direction && UAnimationGraphSchema::IsPosePin(Pin->PinType))
			{
				return Pin;
			}
		}
		return nullptr;
	}

	UAnimationGraph* FindAnimationGraph(UAnimBlueprint* AnimBlueprint)
	{
		if (!AnimBlueprint)
		{
			return nullptr;
		}
		for (UEdGraph* FunctionGraph : AnimBlueprint->FunctionGraphs)
		{
			UAnimationGraph* Graph = Cast<UAnimationGraph>(FunctionGraph);
			if (Graph && Graph->GetFName() == FName(TEXT("AnimGraph")))
			{
				return Graph;
			}
		}
		return nullptr;
	}

	bool DoesNodeReachRoot(UEdGraphNode* StartNode, UAnimGraphNode_Root* RootNode)
	{
		TArray<UEdGraphNode*> Pending;
		TSet<const UEdGraphNode*> Visited;
		Pending.Add(StartNode);
		while (Pending.Num() > 0)
		{
			UEdGraphNode* Node = Pending.Pop(EAllowShrinking::No);
			if (!Node || Visited.Contains(Node))
			{
				continue;
			}
			if (Node == RootNode)
			{
				return true;
			}
			Visited.Add(Node);
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (!Pin || Pin->Direction != EGPD_Output || !UAnimationGraphSchema::IsPosePin(Pin->PinType))
				{
					continue;
				}
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					Pending.Add(LinkedPin ? LinkedPin->GetOwningNode() : nullptr);
				}
			}
		}
		return false;
	}

	TArray<UAnimGraphNode_RigidBody*> FindRigidBodyNodes(UAnimBlueprint* AnimBlueprint)
	{
		TArray<UAnimGraphNode_RigidBody*> Result;
		if (!AnimBlueprint)
		{
			return Result;
		}
		TArray<UEdGraph*> Graphs;
		AnimBlueprint->GetAllGraphs(Graphs);
		for (UEdGraph* Graph : Graphs)
		{
			for (UEdGraphNode* Node : Graph->Nodes)
			{
				if (UAnimGraphNode_RigidBody* RigidBody = Cast<UAnimGraphNode_RigidBody>(Node))
				{
					Result.Add(RigidBody);
				}
			}
		}
		return Result;
	}

	UPhysicsAsset* ResolvePhysicsAsset(const UAnimGraphNode_RigidBody* Node, const USkeletalMesh* Mesh)
	{
		if (!Node)
		{
			return nullptr;
		}
		return Node->Node.OverridePhysicsAsset
			? Node->Node.OverridePhysicsAsset.Get()
			: (Node->Node.bDefaultToSkeletalMeshPhysicsAsset && Mesh ? Mesh->GetPhysicsAsset() : nullptr);
	}

	bool IsConnected(UAnimBlueprint* AnimBlueprint, UAnimGraphNode_RigidBody* Node)
	{
		UAnimationGraph* Graph = FindAnimationGraph(AnimBlueprint);
		return Graph && DoesNodeReachRoot(Node, FBlueprintEditorUtils::GetAnimGraphRoot(Graph));
	}

	bool ContainsToken(const FString& Name, const TArray<FString>& Tokens)
	{
		for (const FString& Token : Tokens)
		{
			if (Name.Contains(Token))
			{
				return true;
			}
		}
		return false;
	}

	bool IsSecondaryBone(const FName BoneName)
	{
		static const TArray<FString> Positive = {
			TEXT("tail"), TEXT("hair"), TEXT("braid"), TEXT("pony"), TEXT("ribbon"), TEXT("skirt"),
			TEXT("cloth"), TEXT("cape"), TEXT("jiggle"), TEXT("tassel"), TEXT("strap"), TEXT("cord"),
			TEXT("chain"), TEXT("accessory")
		};
		static const TArray<FString> Excluded = {
			TEXT("finger"), TEXT("thumb"), TEXT("index"), TEXT("middle"), TEXT("ring"), TEXT("pinky"),
			TEXT("upperarm"), TEXT("lowerarm"), TEXT("thigh"), TEXT("calf"), TEXT("foot"), TEXT("toe"),
			TEXT("spine"), TEXT("clavicle"), TEXT("pelvis"), TEXT("tongue"), TEXT("eyelid"), TEXT("eyebrow")
		};
		const FString Lower = BoneName.ToString().ToLower();
		return ContainsToken(Lower, Positive) && !ContainsToken(Lower, Excluded);
	}

	void BuildChildren(const FReferenceSkeleton& RefSkeleton, TArray<TArray<int32>>& OutChildren)
	{
		OutChildren.SetNum(RefSkeleton.GetRawBoneNum());
		for (int32 Index = 0; Index < RefSkeleton.GetRawBoneNum(); ++Index)
		{
			const int32 Parent = RefSkeleton.GetParentIndex(Index);
			if (OutChildren.IsValidIndex(Parent))
			{
				OutChildren[Parent].Add(Index);
			}
		}
	}

	bool BuildManualChain(const USkeletalMesh* Mesh, FName RootBone, FChainCandidate& OutChain, FString& OutError)
	{
		if (!Mesh)
		{
			OutError = TEXT("No skeletal mesh is selected.");
			return false;
		}
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		TArray<TArray<int32>> Children;
		BuildChildren(Ref, Children);
		int32 Index = Ref.FindBoneIndex(RootBone);
		if (Index == INDEX_NONE)
		{
			OutError = FString::Printf(TEXT("Bone %s does not exist."), *RootBone.ToString());
			return false;
		}
		OutChain.RootBone = RootBone;
		OutChain.DetectionReason = TEXT("Manual root");
		OutChain.Confidence = 100;
		while (Index != INDEX_NONE)
		{
			OutChain.BoneNames.Add(Ref.GetBoneName(Index));
			if (Children[Index].Num() > 1)
			{
				OutError = FString::Printf(TEXT("%s branches; add each branch root separately."), *Ref.GetBoneName(Index).ToString());
				return false;
			}
			if (Children[Index].Num() == 0)
			{
				break;
			}
			Index = Children[Index][0];
			OutChain.TotalLength += Ref.GetRefBonePose()[Index].GetTranslation().Size();
		}
		if (OutChain.BoneNames.Num() < 2)
		{
			OutError = TEXT("A chain needs at least two bones.");
			return false;
		}
		OutChain.EndBone = OutChain.BoneNames.Last();
		return true;
	}

	FName FindAncestorBody(const USkeletalMesh* Mesh, const UPhysicsAsset* PhysicsAsset, FName BoneName)
	{
		if (!Mesh || !PhysicsAsset)
		{
			return NAME_None;
		}
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		int32 Index = Ref.FindBoneIndex(BoneName);
		while (Index != INDEX_NONE)
		{
			Index = Ref.GetParentIndex(Index);
			if (Index != INDEX_NONE && PhysicsAsset->FindBodyIndex(Ref.GetBoneName(Index)) != INDEX_NONE)
			{
				return Ref.GetBoneName(Index);
			}
		}
		return NAME_None;
	}

	bool HasConstraint(const UPhysicsAsset* PhysicsAsset, FName Child, FName Parent)
	{
		for (const UPhysicsConstraintTemplate* Constraint : PhysicsAsset->ConstraintSetup)
		{
			if (!Constraint)
			{
				continue;
			}
			const FConstraintInstance& Instance = Constraint->DefaultInstance;
			if ((Instance.ConstraintBone1 == Child && Instance.ConstraintBone2 == Parent)
				|| (Instance.ConstraintBone2 == Child && Instance.ConstraintBone1 == Parent))
			{
				return true;
			}
		}
		return false;
	}

	struct FInspection
	{
		UPhysicsAsset* Asset = nullptr;
		int32 Bodies = 0;
		int32 Simulated = 0;
		int32 Geometry = 0;
		int32 Constraints = 0;
		int32 ExpectedConstraints = 0;
		bool bComplete = false;
	};

	FInspection Inspect(const USkeletalMesh* Mesh, UPhysicsAsset* Asset, const FChainCandidate& Chain)
	{
		FInspection Result;
		Result.Asset = Asset;
		if (!Mesh || !Asset)
		{
			return Result;
		}
		for (FName Bone : Chain.BoneNames)
		{
			const int32 BodyIndex = Asset->FindBodyIndex(Bone);
			if (!Asset->SkeletalBodySetups.IsValidIndex(BodyIndex))
			{
				continue;
			}
			const USkeletalBodySetup* Body = Asset->SkeletalBodySetups[BodyIndex];
			++Result.Bodies;
			Result.Simulated += Body && Body->PhysicsType == PhysType_Simulated ? 1 : 0;
			Result.Geometry += Body && Body->AggGeom.GetElementCount() > 0 ? 1 : 0;
		}
		const FName Anchor = FindAncestorBody(Mesh, Asset, Chain.RootBone);
		for (int32 Index = 0; Index < Chain.BoneNames.Num(); ++Index)
		{
			const FName Parent = Index == 0 ? Anchor : Chain.BoneNames[Index - 1];
			if (!Parent.IsNone())
			{
				++Result.ExpectedConstraints;
				Result.Constraints += HasConstraint(Asset, Chain.BoneNames[Index], Parent) ? 1 : 0;
			}
		}
		Result.bComplete = Result.Bodies == Chain.BoneNames.Num()
			&& Result.Simulated == Chain.BoneNames.Num()
			&& Result.Geometry == Chain.BoneNames.Num()
			&& Result.Constraints == Result.ExpectedConstraints
			&& Result.ExpectedConstraints > 0;
		return Result;
	}

	void DetectExisting(FAnalysisResult& Analysis)
	{
		USkeletalMesh* Mesh = Analysis.SkeletalMesh.Get();
		TSet<UPhysicsAsset*> Assets;
		if (Mesh && Mesh->GetPhysicsAsset())
		{
			Assets.Add(Mesh->GetPhysicsAsset());
		}
		FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> PhysicsAssetData;
		Registry.Get().GetAssetsByClass(UPhysicsAsset::StaticClass()->GetClassPathName(), PhysicsAssetData, true);
		for (const FAssetData& Data : PhysicsAssetData)
		{
			UPhysicsAsset* PhysicsAsset = Cast<UPhysicsAsset>(Data.GetAsset());
			if (PhysicsAsset && PhysicsAsset->PreviewSkeletalMesh.Get() == Mesh)
			{
				Assets.Add(PhysicsAsset);
			}
		}
		for (FAnimBlueprintCandidate& Candidate : Analysis.AnimBlueprints)
		{
			for (UAnimGraphNode_RigidBody* Node : FindRigidBodyNodes(Candidate.AnimBlueprint.Get()))
			{
				UPhysicsAsset* Asset = ResolvePhysicsAsset(Node, Mesh);
				if (Asset)
				{
					Assets.Add(Asset);
					if (IsConnected(Candidate.AnimBlueprint.Get(), Node))
					{
						Candidate.bHasConnectedRigidBody = true;
						Candidate.DetectedPhysicsAsset = Asset;
					}
				}
			}
		}

		for (FChainCandidate& Chain : Analysis.Chains)
		{
			TArray<FInspection> Partial;
			TArray<FInspection> ConnectedComplete;
			for (UPhysicsAsset* Asset : Assets)
			{
				FInspection Inspection = Inspect(Mesh, Asset, Chain);
				if (Inspection.Bodies > 0 || Inspection.Constraints > 0)
				{
					Partial.Add(Inspection);
				}
				if (Inspection.bComplete)
				{
					for (const FAnimBlueprintCandidate& Anim : Analysis.AnimBlueprints)
					{
						if (Anim.bHasConnectedRigidBody && Anim.DetectedPhysicsAsset.Get() == Asset)
						{
							ConnectedComplete.Add(Inspection);
							break;
						}
					}
				}
			}
			if (ConnectedComplete.Num() == 1)
			{
				Chain.SetupState = ESetupState::Complete;
				Chain.DetectedPhysicsAsset = ConnectedComplete[0].Asset;
			}
			else if (ConnectedComplete.Num() > 1)
			{
				Chain.SetupState = ESetupState::Conflict;
			}
			else if (Partial.Num() > 0)
			{
				Partial.Sort([](const FInspection& A, const FInspection& B)
				{
					return A.Bodies + A.Constraints > B.Bodies + B.Constraints;
				});
				Chain.SetupState = ESetupState::Partial;
				Chain.DetectedPhysicsAsset = Partial[0].Asset;
			}
		}
	}

	FString DefaultAssetPath(const USkeletalMesh* Mesh)
	{
		FString BaseName = Mesh->GetName();
		BaseName.RemoveFromStart(TEXT("SKM_"));
		const FString AssetName = FString::Printf(TEXT("PA_%s_ChainPhysics"), *BaseName);
		const FString Folder = FPackageName::GetLongPackagePath(Mesh->GetOutermost()->GetName());
		return FString::Printf(TEXT("%s/%s.%s"), *Folder, *AssetName, *AssetName);
	}

	UPhysicsAsset* EnsureAsset(USkeletalMesh* Mesh, const FString& ObjectPath, bool& bCreated)
	{
		bCreated = false;
		if (UPhysicsAsset* Existing = LoadObject<UPhysicsAsset>(nullptr, *ObjectPath))
		{
			return Existing;
		}
		UPhysicsAsset* Source = Mesh ? Mesh->GetPhysicsAsset() : nullptr;
		const FString Package = FPackageName::ObjectPathToPackageName(ObjectPath);
		const FString Folder = FPackageName::GetLongPackagePath(Package);
		const FString Name = FPackageName::GetLongPackageAssetName(Package);
		UPhysicsAsset* Result = nullptr;
		if (Source)
		{
			FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			Result = Cast<UPhysicsAsset>(AssetTools.Get().DuplicateAsset(Name, Folder, Source));
		}
		else if (Mesh)
		{
			UPackage* NewPackage = CreatePackage(*Package);
			Result = NewObject<UPhysicsAsset>(NewPackage, *Name, RF_Public | RF_Standalone | RF_Transactional);
			FText CreationError;
			const FPhysAssetCreateParams Params = GetDefault<UPhysicsAssetGenerationSettings>()->CreateParams;
			if (!Result || !FPhysicsAssetUtils::CreateFromSkeletalMesh(Result, Mesh, Params, CreationError, false))
			{
				if (Result)
				{
					Result->ClearFlags(RF_Public | RF_Standalone);
				}
				return nullptr;
			}
			FAssetRegistryModule::AssetCreated(Result);
		}
		bCreated = Result != nullptr;
		return Result;
	}

	TArray<FTransform> ComponentPose(const USkeletalMesh* Mesh)
	{
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		const TArray<FTransform>& Local = Ref.GetRefBonePose();
		TArray<FTransform> Result;
		Result.SetNum(Local.Num());
		for (int32 Index = 0; Index < Local.Num(); ++Index)
		{
			const int32 Parent = Ref.GetParentIndex(Index);
			Result[Index] = Parent == INDEX_NONE ? Local[Index] : Local[Index] * Result[Parent];
		}
		return Result;
	}

	void PruneNewAsset(USkeletalMesh* Mesh, UPhysicsAsset* Asset, const TArray<const FChainCandidate*>& Chains, float RadiusScale)
	{
		UPhysicsAsset* Source = Mesh->GetPhysicsAsset();
		if (!Source)
		{
			Source = Asset;
		}
		TSet<FName> Keep;
		TArray<FVector> Points;
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		const TArray<FTransform> Pose = ComponentPose(Mesh);
		for (const FChainCandidate* Chain : Chains)
		{
			Keep.Add(FindAncestorBody(Mesh, Source, Chain->RootBone));
			for (FName Bone : Chain->BoneNames)
			{
				const int32 Index = Ref.FindBoneIndex(Bone);
				if (Pose.IsValidIndex(Index))
				{
					Points.Add(Pose[Index].GetTranslation());
				}
			}
		}
		const float DistanceSquared = FMath::Square(35.0f * FMath::Clamp(RadiusScale, 0.5f, 3.0f));
		for (const USkeletalBodySetup* Body : Source->SkeletalBodySetups)
		{
			const int32 Index = Body ? Ref.FindBoneIndex(Body->BoneName) : INDEX_NONE;
			if (!Pose.IsValidIndex(Index))
			{
				continue;
			}
			for (const FVector& Point : Points)
			{
				if (FVector::DistSquared(Pose[Index].GetTranslation(), Point) <= DistanceSquared)
				{
					Keep.Add(Body->BoneName);
					break;
				}
			}
		}
		while (Asset->ConstraintSetup.Num() > 0)
		{
			FPhysicsAssetUtils::DestroyConstraint(Asset, Asset->ConstraintSetup.Num() - 1);
		}
		for (int32 Index = Asset->SkeletalBodySetups.Num() - 1; Index >= 0; --Index)
		{
			USkeletalBodySetup* Body = Asset->SkeletalBodySetups[Index];
			if (!Body || !Keep.Contains(Body->BoneName))
			{
				FPhysicsAssetUtils::DestroyBody(Asset, Index);
				continue;
			}
			Body->PhysicsType = PhysType_Kinematic;
			Body->DefaultInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics, false);
		}
	}

	bool EnsureAnchor(USkeletalMesh* Mesh, UPhysicsAsset* Asset, FName Root, FName& OutAnchor)
	{
		OutAnchor = FindAncestorBody(Mesh, Asset, Root);
		if (!OutAnchor.IsNone())
		{
			return true;
		}
		UPhysicsAsset* Source = Mesh->GetPhysicsAsset();
		if (!Source)
		{
			Source = Asset;
		}
		OutAnchor = FindAncestorBody(Mesh, Source, Root);
		const int32 SourceIndex = Source ? Source->FindBodyIndex(OutAnchor) : INDEX_NONE;
		if (OutAnchor.IsNone() || !Source->SkeletalBodySetups.IsValidIndex(SourceIndex))
		{
			return false;
		}
		USkeletalBodySetup* Clone = DuplicateObject<USkeletalBodySetup>(Source->SkeletalBodySetups[SourceIndex], Asset);
		if (!Clone)
		{
			return false;
		}
		Clone->SetFlags(RF_Transactional);
		Clone->PhysicsType = PhysType_Kinematic;
		Clone->DefaultInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics, false);
		Asset->SkeletalBodySetups.Add(Clone);
		return true;
	}

	void ReplaceCollider(USkeletalBodySetup* Body, const USkeletalMesh* Mesh, const FChainCandidate& Chain, int32 ChainIndex, const FPresetValues& Values, float RadiusScale)
	{
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		Body->AggGeom.EmptyElements();
		if (Chain.BoneNames.IsValidIndex(ChainIndex + 1))
		{
			const int32 ChildIndex = Ref.FindBoneIndex(Chain.BoneNames[ChainIndex + 1]);
			const FVector Offset = Ref.GetRefBonePose()[ChildIndex].GetTranslation();
			const float Distance = Offset.Size();
			if (Distance > UE_SMALL_NUMBER)
			{
				const float Radius = FMath::Clamp(Distance * Values.RadiusRatio, Values.MinRadius, Values.MaxRadius) * RadiusScale;
				FKSphylElem Capsule;
				Capsule.Radius = Radius;
				Capsule.Length = FMath::Max(0.1f, Distance - 2.0f * Radius);
				Capsule.Center = Offset * 0.5f;
				Capsule.Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Offset / Distance).Rotator();
				Body->AggGeom.SphylElems.Add(Capsule);
				return;
			}
		}
		const int32 BoneIndex = Ref.FindBoneIndex(Chain.BoneNames[ChainIndex]);
		const float Incoming = Ref.GetRefBonePose()[BoneIndex].GetTranslation().Size();
		FKSphereElem Sphere;
		Sphere.Radius = FMath::Clamp(Incoming * Values.RadiusRatio, Values.MinRadius, Values.MaxRadius) * RadiusScale;
		Body->AggGeom.SphereElems.Add(Sphere);
	}

	void ConfigureConstraint(UPhysicsAsset* Asset, UPhysicsConstraintTemplate* Constraint, FName Child, FName Parent, bool bRoot, const FPresetValues& Values)
	{
		FConstraintInstance& Instance = Constraint->DefaultInstance;
		Instance.ConstraintBone1 = Child;
		Instance.ConstraintBone2 = Parent;
		Instance.SnapTransformsToDefault(EConstraintTransformComponentFlags::All, Asset);
		Instance.SetLinearXMotion(LCM_Locked);
		Instance.SetLinearYMotion(LCM_Locked);
		Instance.SetLinearZMotion(LCM_Locked);
		Instance.SetAngularSwing1Limit(ACM_Limited, bRoot ? Values.RootSwing1 : Values.Swing1);
		Instance.SetAngularSwing2Limit(ACM_Limited, bRoot ? Values.RootSwing2 : Values.Swing2);
		Instance.SetAngularTwistLimit(ACM_Limited, bRoot ? Values.RootTwist : Values.Twist);
		Instance.SetAngularDriveMode(EAngularDriveMode::SLERP);
		Instance.SetOrientationDriveSLERP(true);
		Instance.SetAngularVelocityDriveSLERP(true);
		Instance.SetAngularDriveParams(bRoot ? Values.RootSpring : Values.Spring, bRoot ? Values.RootDriveDamping : Values.DriveDamping, 0.0f);
		Instance.SetAngularDriveAccelerationMode(true);
		Constraint->SetDefaultProfile(Instance);
	}

	void RemoveSelected(UPhysicsAsset* Asset, const TSet<FName>& Bones)
	{
		for (int32 Index = Asset->ConstraintSetup.Num() - 1; Index >= 0; --Index)
		{
			const UPhysicsConstraintTemplate* Constraint = Asset->ConstraintSetup[Index];
			if (Constraint && (Bones.Contains(Constraint->DefaultInstance.ConstraintBone1) || Bones.Contains(Constraint->DefaultInstance.ConstraintBone2)))
			{
				FPhysicsAssetUtils::DestroyConstraint(Asset, Index);
			}
		}
		for (int32 Index = Asset->SkeletalBodySetups.Num() - 1; Index >= 0; --Index)
		{
			const USkeletalBodySetup* Body = Asset->SkeletalBodySetups[Index];
			if (Body && Bones.Contains(Body->BoneName))
			{
				FPhysicsAssetUtils::DestroyBody(Asset, Index);
			}
		}
	}

	bool ConfigureAsset(USkeletalMesh* Mesh, UPhysicsAsset* Asset, const TArray<const FChainCandidate*>& Chains, const FSetupOptions& Options, bool bCreated, bool& bModified, FString& Error)
	{
		Asset->Modify();
		if (bCreated)
		{
			PruneNewAsset(Mesh, Asset, Chains, Options.RadiusScale);
			bModified = true;
		}
		TSet<FName> SelectedBones;
		for (const FChainCandidate* Chain : Chains)
		{
			for (FName Bone : Chain->BoneNames)
			{
				SelectedBones.Add(Bone);
			}
		}
		if (Options.Mode == ESetupMode::Regenerate)
		{
			RemoveSelected(Asset, SelectedBones);
			bModified = true;
		}
		const FPresetValues Values = GetPresetValues(Options.Preset);
		FPhysAssetCreateParams Params;
		Params.GeomType = EFG_Sphyl;
		Params.bCreateConstraints = false;
		Params.bDisableCollisionsByDefault = false;
		for (const FChainCandidate* Chain : Chains)
		{
			FName Anchor;
			if (!EnsureAnchor(Mesh, Asset, Chain->RootBone, Anchor))
			{
				Error = FString::Printf(TEXT("No parent Physics Asset body was found for %s."), *Chain->RootBone.ToString());
				return false;
			}
			for (int32 ChainIndex = 0; ChainIndex < Chain->BoneNames.Num(); ++ChainIndex)
			{
				const FName BoneName = Chain->BoneNames[ChainIndex];
				int32 BodyIndex = Asset->FindBodyIndex(BoneName);
				const bool bNewBody = BodyIndex == INDEX_NONE;
				if (bNewBody)
				{
					BodyIndex = FPhysicsAssetUtils::CreateNewBody(Asset, BoneName, Params);
					bModified = true;
				}
				USkeletalBodySetup* Body = Asset->SkeletalBodySetups.IsValidIndex(BodyIndex) ? Asset->SkeletalBodySetups[BodyIndex] : nullptr;
				if (!Body)
				{
					Error = FString::Printf(TEXT("Failed to create %s body."), *BoneName.ToString());
					return false;
				}
				Body->PhysicsType = PhysType_Simulated;
				Body->DefaultInstance.LinearDamping = Values.LinearDamping;
				Body->DefaultInstance.AngularDamping = Values.AngularDamping;
				Body->DefaultInstance.MassScale = Values.MassScale;
				Body->DefaultInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics, false);
				if (bNewBody || Options.Mode == ESetupMode::Regenerate || Body->AggGeom.GetElementCount() == 0)
				{
					ReplaceCollider(Body, Mesh, *Chain, ChainIndex, Values, Options.RadiusScale);
					Body->InvalidatePhysicsData();
					Body->CreatePhysicsMeshes();
					bModified = true;
				}
			}
			for (int32 ChainIndex = 0; ChainIndex < Chain->BoneNames.Num(); ++ChainIndex)
			{
				const FName Child = Chain->BoneNames[ChainIndex];
				const FName Parent = ChainIndex == 0 ? Anchor : Chain->BoneNames[ChainIndex - 1];
				int32 ConstraintIndex = Asset->FindConstraintIndex(Child);
				const bool bNewConstraint = ConstraintIndex == INDEX_NONE;
				if (bNewConstraint)
				{
					ConstraintIndex = FPhysicsAssetUtils::CreateNewConstraint(Asset, Child);
					bModified = true;
				}
				UPhysicsConstraintTemplate* Constraint = Asset->ConstraintSetup.IsValidIndex(ConstraintIndex) ? Asset->ConstraintSetup[ConstraintIndex] : nullptr;
				if (!Constraint)
				{
					Error = FString::Printf(TEXT("Failed to create %s constraint."), *Child.ToString());
					return false;
				}
				if (bNewConstraint || !HasConstraint(Asset, Child, Parent) || Options.Mode == ESetupMode::Regenerate)
				{
					ConfigureConstraint(Asset, Constraint, Child, Parent, ChainIndex == 0, Values);
					bModified = true;
				}
				Asset->DisableCollision(Asset->FindBodyIndex(Child), Asset->FindBodyIndex(Parent));
			}
		}
		const TArray<FName> Bones = SelectedBones.Array();
		for (int32 A = 0; A < Bones.Num(); ++A)
		{
			for (int32 B = A + 1; B < Bones.Num(); ++B)
			{
				Asset->DisableCollision(Asset->FindBodyIndex(Bones[A]), Asset->FindBodyIndex(Bones[B]));
			}
		}
		Asset->PreviewSkeletalMesh = Mesh;
		Asset->UpdateBodySetupIndexMap();
		Asset->UpdateBoundsBodiesArray();
		Asset->RefreshPhysicsAssetChange();
		if (bModified)
		{
			FMetaData& MetaData = Asset->GetOutermost()->GetMetaData();
			MetaData.SetValue(Asset, TEXT("ChainPhysicsEditor.Version"), TEXT("1"));
			MetaData.SetValue(Asset, TEXT("ChainPhysicsEditor.SourceMesh"), *Mesh->GetPathName());
			TArray<FString> Roots;
			for (const FChainCandidate* Chain : Chains)
			{
				Roots.Add(Chain->RootBone.ToString());
			}
			MetaData.SetValue(Asset, TEXT("ChainPhysicsEditor.Roots"), *FString::Join(Roots, TEXT(",")));
			Asset->MarkPackageDirty();
			if (!SaveAsset(Asset))
			{
				Error = TEXT("Failed to save the Physics Asset.");
				return false;
			}
		}
		return true;
	}

	void ConfigureRigidBodyNode(UAnimGraphNode_RigidBody* Node, UPhysicsAsset* Asset)
	{
		Node->Node.OverridePhysicsAsset = Asset;
		Node->Node.bDefaultToSkeletalMeshPhysicsAsset = false;
		Node->Node.bUseDefaultAsSimulated = false;
		Node->Node.SimulationSpace = ESimulationSpace::ComponentSpace;
		Node->Node.SimSpaceSettings.WorldAlpha = 0.8f;
		Node->Node.SimSpaceSettings.VelocityScaleZ = 0.75f;
		Node->Node.SimSpaceSettings.DampingAlpha = 1.0f;
		Node->Node.CachedBoundsScale = 1.25f;
		Node->Node.bEnableWorldGeometry = false;
		Node->Node.bUseExternalClothCollision = false;
		Node->Node.bTransferBoneVelocities = true;
		Node->Node.bForceDisableCollisionBetweenConstraintBodies = true;
		Node->Node.EvaluationResetTime = 0.5f;
		Node->Node.Alpha = 1.0f;
		Node->NodeComment = TEXT("Chain Physics Editor (tool managed)");
	}

	bool ConfigureAnimBlueprint(UAnimBlueprint* AnimBlueprint, UPhysicsAsset* Asset, bool& bModified, FString& Error)
	{
		UAnimationGraph* Graph = FindAnimationGraph(AnimBlueprint);
		UAnimGraphNode_Root* Root = Graph ? FBlueprintEditorUtils::GetAnimGraphRoot(Graph) : nullptr;
		if (!Graph || !Root)
		{
			Error = FString::Printf(TEXT("%s has no usable AnimGraph root."), *AnimBlueprint->GetName());
			return false;
		}
		UAnimGraphNode_RigidBody* RigidBody = nullptr;
		for (UAnimGraphNode_RigidBody* Candidate : FindRigidBodyNodes(AnimBlueprint))
		{
			if (ResolvePhysicsAsset(Candidate, nullptr) == Asset)
			{
				RigidBody = Candidate;
				if (DoesNodeReachRoot(Candidate, Root))
				{
					ConfigureRigidBodyNode(Candidate, Asset);
					return true;
				}
				break;
			}
		}
		UEdGraphPin* RootInput = FindPosePin(Root, EGPD_Input);
		if (!RootInput || RootInput->LinkedTo.Num() != 1)
		{
			Error = TEXT("AnimGraph root must have exactly one input pose.");
			return false;
		}
		AnimBlueprint->Modify();
		Graph->Modify();
		UEdGraphPin* PreviousOutput = RootInput->LinkedTo[0];
		PreviousOutput->BreakLinkTo(RootInput);
		FGraphNodeCreator<UAnimGraphNode_LocalToComponentSpace> L2CCreator(*Graph);
		UAnimGraphNode_LocalToComponentSpace* L2C = L2CCreator.CreateNode();
		L2C->NodePosX = Root->NodePosX - 650;
		L2C->NodePosY = Root->NodePosY;
		L2CCreator.Finalize();
		if (!RigidBody)
		{
			FGraphNodeCreator<UAnimGraphNode_RigidBody> Creator(*Graph);
			RigidBody = Creator.CreateNode();
			Creator.Finalize();
		}
		else
		{
			FindPosePin(RigidBody, EGPD_Input)->BreakAllPinLinks();
			FindPosePin(RigidBody, EGPD_Output)->BreakAllPinLinks();
		}
		RigidBody->NodePosX = Root->NodePosX - 400;
		RigidBody->NodePosY = Root->NodePosY;
		ConfigureRigidBodyNode(RigidBody, Asset);
		FGraphNodeCreator<UAnimGraphNode_ComponentToLocalSpace> C2LCreator(*Graph);
		UAnimGraphNode_ComponentToLocalSpace* C2L = C2LCreator.CreateNode();
		C2L->NodePosX = Root->NodePosX - 150;
		C2L->NodePosY = Root->NodePosY;
		C2LCreator.Finalize();
		const UEdGraphSchema* Schema = Graph->GetSchema();
		if (!Schema
			|| !Schema->TryCreateConnection(PreviousOutput, FindPosePin(L2C, EGPD_Input))
			|| !Schema->TryCreateConnection(FindPosePin(L2C, EGPD_Output), FindPosePin(RigidBody, EGPD_Input))
			|| !Schema->TryCreateConnection(FindPosePin(RigidBody, EGPD_Output), FindPosePin(C2L, EGPD_Input))
			|| !Schema->TryCreateConnection(FindPosePin(C2L, EGPD_Output), RootInput))
		{
			Schema->TryCreateConnection(PreviousOutput, RootInput);
			Error = TEXT("Failed to connect the Rigid Body node chain.");
			return false;
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(AnimBlueprint);
		FKismetEditorUtilities::CompileBlueprint(AnimBlueprint);
		if (AnimBlueprint->Status == BS_Error || !AnimBlueprint->GeneratedClass)
		{
			Error = FString::Printf(TEXT("%s failed to compile."), *AnimBlueprint->GetName());
			return false;
		}
		AnimBlueprint->MarkPackageDirty();
		if (!SaveAsset(AnimBlueprint))
		{
			Error = FString::Printf(TEXT("Failed to save %s."), *AnimBlueprint->GetName());
			return false;
		}
		bModified = true;
		return true;
	}
}

namespace ChainPhysicsSetup
{
	bool AnalyzeSkeletalMesh(USkeletalMesh* Mesh, FAnalysisResult& OutResult)
	{
		OutResult = FAnalysisResult();
		OutResult.SkeletalMesh = Mesh;
		if (!Mesh || !Mesh->GetSkeleton())
		{
			OutResult.Summary = TEXT("Select a skeletal mesh with a valid Skeleton.");
			return false;
		}
		const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
		TArray<TArray<int32>> Children;
		BuildChildren(Ref, Children);
		for (int32 BoneIndex = 0; BoneIndex < Ref.GetRawBoneNum(); ++BoneIndex)
		{
			if (!IsSecondaryBone(Ref.GetBoneName(BoneIndex)))
			{
				continue;
			}
			const int32 Parent = Ref.GetParentIndex(BoneIndex);
			if (Parent != INDEX_NONE && IsSecondaryBone(Ref.GetBoneName(Parent)))
			{
				continue;
			}
			FChainCandidate Chain;
			Chain.RootBone = Ref.GetBoneName(BoneIndex);
			Chain.DetectionReason = TEXT("Name + linear hierarchy");
			Chain.Confidence = 90;
			int32 Current = BoneIndex;
			while (Current != INDEX_NONE)
			{
				Chain.BoneNames.Add(Ref.GetBoneName(Current));
				TArray<int32> NamedChildren;
				for (int32 Child : Children[Current])
				{
					if (IsSecondaryBone(Ref.GetBoneName(Child)))
					{
						NamedChildren.Add(Child);
					}
				}
				if (NamedChildren.Num() != 1)
				{
					break;
				}
				Current = NamedChildren[0];
				Chain.TotalLength += Ref.GetRefBonePose()[Current].GetTranslation().Size();
			}
			if (Chain.BoneNames.Num() >= 2)
			{
				Chain.EndBone = Chain.BoneNames.Last();
				OutResult.Chains.Add(MoveTemp(Chain));
			}
		}
		FAssetRegistryModule& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> Assets;
		Registry.Get().GetAssetsByClass(UAnimBlueprint::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Data : Assets)
		{
			UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>(Data.GetAsset());
			if (AnimBlueprint && AnimBlueprint->TargetSkeleton == Mesh->GetSkeleton())
			{
				FAnimBlueprintCandidate Candidate;
				Candidate.AnimBlueprint = AnimBlueprint;
				Candidate.bSelected = AnimBlueprint->GetPreviewMesh() == Mesh;
				OutResult.AnimBlueprints.Add(Candidate);
			}
		}
		if (OutResult.AnimBlueprints.Num() == 1)
		{
			OutResult.AnimBlueprints[0].bSelected = true;
		}
		DetectExisting(OutResult);
		OutResult.Summary = FString::Printf(TEXT("Detected %d chain candidate(s) and %d compatible Anim Blueprint(s)."), OutResult.Chains.Num(), OutResult.AnimBlueprints.Num());
		return true;
	}

	bool AddManualChain(FAnalysisResult& Analysis, FName RootBone, FString& OutError)
	{
		for (const FChainCandidate& Chain : Analysis.Chains)
		{
			if (Chain.RootBone == RootBone)
			{
				OutError = TEXT("That root is already listed.");
				return false;
			}
		}
		FChainCandidate Chain;
		if (!BuildManualChain(Analysis.SkeletalMesh.Get(), RootBone, Chain, OutError))
		{
			return false;
		}
		Analysis.Chains.Add(MoveTemp(Chain));
		DetectExisting(Analysis);
		return true;
	}

	FSetupResult SetupSelectedChains(FAnalysisResult& Analysis, const FSetupOptions& Options)
	{
		FSetupResult Result;
		USkeletalMesh* Mesh = Analysis.SkeletalMesh.Get();
		TArray<const FChainCandidate*> Chains;
		TArray<UAnimBlueprint*> AnimBlueprints;
		for (const FChainCandidate& Chain : Analysis.Chains)
		{
			if (Chain.bSelected)
			{
				Chains.Add(&Chain);
			}
		}
		for (const FAnimBlueprintCandidate& Candidate : Analysis.AnimBlueprints)
		{
			if (Candidate.bSelected && Candidate.AnimBlueprint.IsValid())
			{
				AnimBlueprints.Add(Candidate.AnimBlueprint.Get());
			}
		}
		if (!Mesh || Chains.Num() == 0 || AnimBlueprints.Num() == 0)
		{
			Result.Message = TEXT("Select a mesh, at least one chain, and at least one Anim Blueprint.");
			return Result;
		}
		if (Options.Mode == ESetupMode::Repair)
		{
			TSet<UPhysicsAsset*> CompleteAssets;
			for (const FChainCandidate* Chain : Chains)
			{
				if (Chain->SetupState == ESetupState::Conflict)
				{
					Result.Message = TEXT("A selected chain has conflicting complete setups. Inspect the detected assets, then use Regenerate selected to make an explicit replacement.");
					return Result;
				}
				if (Chain->SetupState == ESetupState::Complete && Chain->DetectedPhysicsAsset.IsValid())
				{
					CompleteAssets.Add(Chain->DetectedPhysicsAsset.Get());
				}
			}
			if (CompleteAssets.Num() > 1)
			{
				Result.Message = TEXT("The selected chains are already complete in different Physics Assets. Use a single output asset and Regenerate selected only after reviewing those setups.");
				return Result;
			}
		}
		UPhysicsAsset* CompleteAsset = Chains[0]->SetupState == ESetupState::Complete ? Chains[0]->DetectedPhysicsAsset.Get() : nullptr;
		bool bUpToDate = CompleteAsset != nullptr && Options.Mode == ESetupMode::Repair;
		for (const FChainCandidate* Chain : Chains)
		{
			bUpToDate &= Chain->SetupState == ESetupState::Complete && Chain->DetectedPhysicsAsset.Get() == CompleteAsset;
		}
		for (UAnimBlueprint* AnimBlueprint : AnimBlueprints)
		{
			bool bConnected = false;
			for (UAnimGraphNode_RigidBody* Node : FindRigidBodyNodes(AnimBlueprint))
			{
				bConnected |= ResolvePhysicsAsset(Node, Mesh) == CompleteAsset && IsConnected(AnimBlueprint, Node);
			}
			bUpToDate &= bConnected;
		}
		if (bUpToDate)
		{
			Result.bSucceeded = true;
			Result.PhysicsAsset = CompleteAsset;
			Result.Message = TEXT("Selected chains are already up to date.");
			return Result;
		}
		FString ObjectPath = Options.OutputPhysicsAssetObjectPath;
		if (ObjectPath.IsEmpty())
		{
			for (const FChainCandidate* Chain : Chains)
			{
				UPhysicsAsset* Existing = Chain->DetectedPhysicsAsset.Get();
				if (Existing && Existing != Mesh->GetPhysicsAsset())
				{
					ObjectPath = Existing->GetPathName();
					break;
				}
			}
			if (ObjectPath.IsEmpty())
			{
				ObjectPath = DefaultAssetPath(Mesh);
			}
		}
		bool bCreated = false;
		UPhysicsAsset* Asset = EnsureAsset(Mesh, ObjectPath, bCreated);
		if (!Asset)
		{
			Result.Message = TEXT("A source Physics Asset is required to create the dedicated asset.");
			return Result;
		}
		FString Error;
		if (!ConfigureAsset(Mesh, Asset, Chains, Options, bCreated, Result.bModified, Error))
		{
			Result.Message = Error;
			return Result;
		}
		for (UAnimBlueprint* AnimBlueprint : AnimBlueprints)
		{
			if (!ConfigureAnimBlueprint(AnimBlueprint, Asset, Result.bModified, Error))
			{
				Result.Message = Error;
				return Result;
			}
		}
		Result.bSucceeded = true;
		Result.PhysicsAsset = Asset;
		Result.Message = FString::Printf(TEXT("Configured %d chain(s) and %d Anim Blueprint(s) with %s."), Chains.Num(), AnimBlueprints.Num(), *Asset->GetName());
		AnalyzeSkeletalMesh(Mesh, Analysis);
		return Result;
	}

	FText GetSetupStateText(ESetupState State)
	{
		switch (State)
		{
		case ESetupState::Partial: return NSLOCTEXT("ChainPhysicsEditor", "Partial", "Partial / Repair");
		case ESetupState::Complete: return NSLOCTEXT("ChainPhysicsEditor", "Complete", "Up to date");
		case ESetupState::Conflict: return NSLOCTEXT("ChainPhysicsEditor", "Conflict", "Conflict");
		default: return NSLOCTEXT("ChainPhysicsEditor", "Unset", "Not configured");
		}
	}

	FLinearColor GetSetupStateColor(ESetupState State)
	{
		switch (State)
		{
		case ESetupState::Partial: return FLinearColor(1.0f, 0.65f, 0.1f);
		case ESetupState::Complete: return FLinearColor(0.2f, 0.85f, 0.35f);
		case ESetupState::Conflict: return FLinearColor(1.0f, 0.2f, 0.2f);
		default: return FLinearColor(0.65f, 0.65f, 0.65f);
		}
	}
}
