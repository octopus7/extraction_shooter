// One-off authoring tool. Commit with assets, then remove this file.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/BlendSpace.h"
#include "Animation/AnimInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "KismetAnimationLibrary.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "EdGraph/EdGraph.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace DirectionalAuthoring
{
const FString Folder = TEXT("/Game/Characters/Player/LunaMk2/Animations/");
bool Save(UObject* Asset)
{
    Asset->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Asset->GetOutermost(), Asset,
        *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
template<class T> T* Node(UEdGraph* Graph, int32 X, int32 Y)
{
    FGraphNodeCreator<T> Creator(*Graph); T* N = Creator.CreateNode();
    N->NodePosX = X; N->NodePosY = Y; Creator.Finalize(); return N;
}
void Compose(const FReferenceSkeleton& Ref, const TArray<FTransform>& Local, TArray<FTransform>& CS)
{
    CS = Local; for (int32 I = 1; I < CS.Num(); ++I) CS[I] *= CS[Ref.GetParentIndex(I)];
}
// Fixed-length analytic IK; knee pole stays in front of the character.
void Leg(const FReferenceSkeleton& Ref, TArray<FTransform>& Local, const TCHAR* Side,
    const FVector& Target, const FQuat& FootRotation, const FVector& Forward)
{
    const int32 Hip = Ref.FindBoneIndex(*FString::Printf(TEXT("thigh_%s"), Side));
    const int32 Knee = Ref.FindBoneIndex(*FString::Printf(TEXT("calf_%s"), Side));
    const int32 Foot = Ref.FindBoneIndex(*FString::Printf(TEXT("foot_%s"), Side));
    TArray<FTransform> CS; Compose(Ref, Local, CS);
    const FVector A = CS[Hip].GetLocation(), B = CS[Knee].GetLocation(), C = CS[Foot].GetLocation();
    const double Upper = FVector::Distance(A, B), Lower = FVector::Distance(B, C);
    const FVector Dir = (Target - A).GetSafeNormal();
    const double Distance = FMath::Clamp(FVector::Distance(A, Target), FMath::Abs(Upper - Lower) + .01, Upper + Lower - .01);
    const double Along = (Upper*Upper - Lower*Lower + Distance*Distance) / (2 * Distance);
    const FVector Pole = (Forward - Dir * FVector::DotProduct(Forward, Dir)).GetSafeNormal();
    const FVector NewKnee = A + Dir * Along + Pole * FMath::Sqrt(FMath::Max(0.0, Upper*Upper - Along*Along));
    const FVector NewFoot = A + Dir * Distance;
    FTransform HipCS = CS[Hip];
    HipCS.SetRotation((FQuat::FindBetweenNormals((B-A).GetSafeNormal(), (NewKnee-A).GetSafeNormal()) * HipCS.GetRotation()).GetNormalized());
    Local[Hip] = HipCS.GetRelativeTransform(CS[Ref.GetParentIndex(Hip)]);
    Compose(Ref, Local, CS);
    FTransform KneeCS = CS[Knee];
    KneeCS.SetRotation((FQuat::FindBetweenNormals((CS[Foot].GetLocation()-CS[Knee].GetLocation()).GetSafeNormal(), (NewFoot-NewKnee).GetSafeNormal()) * KneeCS.GetRotation()).GetNormalized());
    Local[Knee] = KneeCS.GetRelativeTransform(CS[Hip]);
    Compose(Ref, Local, CS);
    FTransform FootCS = CS[Foot]; FootCS.SetRotation(FootRotation);
    Local[Foot] = FootCS.GetRelativeTransform(CS[Knee]); Compose(Ref, Local, CS);
    const int32 IK = Ref.FindBoneIndex(*FString::Printf(TEXT("ik_foot_%s"), Side));
    if (IK != INDEX_NONE) Local[IK] = CS[Foot].GetRelativeTransform(CS[Ref.GetParentIndex(IK)]);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenerateDirectionalLocomotion, "TunaSweeper.Generate.DirectionalLocomotion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenerateDirectionalLocomotion::RunTest(const FString& Parameters)
{
    using namespace DirectionalAuthoring;
    auto* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2"));
    auto* Idle = LoadObject<UAnimSequence>(nullptr, *(Folder + TEXT("MF_Rifle_Idle_Hipfire1")));
    auto* OldBlend = LoadObject<UBlendSpace>(nullptr, *(Folder + TEXT("BS_RunWalk")));
    auto* BP = LoadObject<UAnimBlueprint>(nullptr, *(Folder + TEXT("ABP_LunaMk2")));
    auto* PlayerClass = LoadClass<ACharacter>(nullptr, TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
    if (!Mesh || !Idle || !OldBlend || !BP || !PlayerClass) return false;
    const TCHAR* Names[] = {TEXT("AS_Luna_WalkBackward"), TEXT("AS_Luna_StrafeLeft"), TEXT("AS_Luna_StrafeRight"), TEXT("BS_DirectionalWalk")};
    for (const TCHAR* Name : Names) if (FPackageName::DoesPackageExist(Folder + Name)) { AddError(TEXT("Destination already exists; refusing to overwrite")); return false; }
    const auto& Ref = Mesh->GetRefSkeleton(); TArray<FTransform> Base = Ref.GetRefBonePose();
    for (int32 I = 0; I < Base.Num(); ++I)
    {
        TArray<FTransform> Keys; Idle->GetDataModel()->GetBoneTrackTransforms(Ref.GetBoneName(I), Keys);
        if (!Keys.IsEmpty()) Base[I] = Keys[0];
    }
    const auto* Component = PlayerClass->GetDefaultObject<ACharacter>()->GetMesh();
    const FVector Forward = Component->GetRelativeRotation().UnrotateVector(FVector::ForwardVector).GetSafeNormal2D();
    const FVector Right = Component->GetRelativeRotation().UnrotateVector(FVector::RightVector).GetSafeNormal2D();
    TArray<FTransform> BaseCS; Compose(Ref, Base, BaseCS);
    const int32 Pelvis = Ref.FindBoneIndex(TEXT("pelvis"));
    const int32 Feet[] = {Ref.FindBoneIndex(TEXT("foot_l")), Ref.FindBoneIndex(TEXT("foot_r"))};
    if (Pelvis < 0 || Feet[0] < 0 || Feet[1] < 0) return false;
    TArray<UAnimSequence*> Clips; constexpr int32 Frames = 48;
    for (int32 Motion = 0; Motion < 3; ++Motion)
    {
        auto* Clip = NewObject<UAnimSequence>(CreatePackage(*(Folder + Names[Motion])), Names[Motion], RF_Public | RF_Standalone);
        Clip->SetSkeleton(Idle->GetSkeleton()); Clip->SetPreviewMesh(Mesh);
        auto& Controller = Clip->GetController(); Controller.InitializeModel();
        Controller.OpenBracket(FText::GetEmpty(), false); Controller.SetFrameRate(FFrameRate(60, 1), false); Controller.SetNumberOfFrames(Frames, false);
        TArray<TArray<FTransform>> Poses;
        const FVector Travel = Motion == 0 ? -Forward : (Motion == 1 ? -Right : Right);
        for (int32 Frame = 0; Frame <= Frames; ++Frame)
        {
            const double Phase = double(Frame % Frames) / Frames; TArray<FTransform> Local = Base;
            FTransform PelvisCS = BaseCS[Pelvis];
            PelvisCS.AddToTranslation(FVector(0, 0, -3.0 + .65 * FMath::Cos(4 * PI * Phase)) + Right * (1.2 * FMath::Sin(2 * PI * Phase)));
            Local[Pelvis] = PelvisCS.GetRelativeTransform(BaseCS[Ref.GetParentIndex(Pelvis)]);
            for (int32 Side = 0; Side < 2; ++Side)
            {
                const double P = FMath::Fmod(Phase + (Side == 0 ? 0.0 : .5), 1.0);
                const bool Swing = P >= .5; const double U = Swing ? (P-.5)*2 : P*2;
                const double Stride = Motion == 0 ? 38.0 : 22.0;
                const double Offset = Swing ? FMath::Lerp(-Stride*.5, Stride*.5, U*U*(3-2*U)) : FMath::Lerp(Stride*.5, -Stride*.5, U);
                FVector Target = BaseCS[Feet[Side]].GetLocation();
                if (Motion != 0)
                {
                    const double Lateral = FVector::DotProduct(Target - BaseCS[Pelvis].GetLocation(), Right);
                    Target += Right * ((Side == 0 ? -17.0 : 17.0) - Lateral);
                }
                Target += Travel * Offset; Target.Z += Swing ? 7.0 * FMath::Square(FMath::Sin(PI*U)) : 0.0;
                Leg(Ref, Local, Side == 0 ? TEXT("l") : TEXT("r"), Target, BaseCS[Feet[Side]].GetRotation(), Forward);
            }
            Poses.Add(MoveTemp(Local));
        }
        for (int32 Bone = 0; Bone < Base.Num(); ++Bone)
        {
            TArray<FVector> Positions, Scales; TArray<FQuat> Rotations;
            for (const auto& Pose : Poses) { Positions.Add(Pose[Bone].GetTranslation()); Rotations.Add(Pose[Bone].GetRotation()); Scales.Add(Pose[Bone].GetScale3D()); }
            if (!Controller.AddBoneCurve(Ref.GetBoneName(Bone), false) || !Controller.SetBoneTrackKeys(Ref.GetBoneName(Bone), Positions, Rotations, Scales, false)) return false;
        }
        Controller.NotifyPopulated(); Controller.CloseBracket(false);
        Clip->bEnableRootMotion = false; Clip->bForceRootLock = true;
        Clip->PostEditChange(); FAssetRegistryModule::AssetCreated(Clip); if (!Save(Clip)) return false; Clips.Add(Clip);
    }
    auto* Blend = NewObject<UBlendSpace>(CreatePackage(*(Folder + Names[3])), Names[3], RF_Public | RF_Standalone);
    Blend->SetSkeleton(OldBlend->GetSkeleton()); Blend->SetPreviewMesh(Mesh);
    auto& Direction = const_cast<FBlendParameter&>(Blend->GetBlendParameter(0));
    Direction.DisplayName = TEXT("Direction"); Direction.Min = -180; Direction.Max = 180; Direction.GridNum = 8; Direction.bWrapInput = true;
    auto& Speed = const_cast<FBlendParameter&>(Blend->GetBlendParameter(1));
    Speed.DisplayName = TEXT("Speed"); Speed.Min = 0; Speed.Max = 930; Speed.GridNum = 6;
    Blend->TargetWeightInterpolationSpeedPerSec = 8;
    for (const auto& Sample : OldBlend->GetBlendSamples()) Blend->AddSample(Sample.Animation, FVector(0, Sample.SampleValue.X, 0));
    for (float Angle : {-180.f, -90.f, 0.f, 90.f, 180.f}) Blend->AddSample(Idle, FVector(Angle, 0, 0));
    for (float Angle : {-180.f, -90.f, 90.f, 180.f})
    {
        UAnimSequence* Clip = FMath::Abs(Angle) == 180 ? Clips[0] : (Angle < 0 ? Clips[1] : Clips[2]);
        for (float Value : {200.f, 464.141312f, 600.f, 930.f})
        {
            const int32 Index = Blend->AddSample(Clip, FVector(Angle, Value, 0));
            auto& Sample = const_cast<FBlendSample&>(Blend->GetBlendSamples()[Index]);
            // A planted foot travels one stride in half of the 0.8-second loop.
            Sample.RateScale = Value / (FMath::Abs(Angle) == 180 ? 95.f : 55.f);
        }
    }
    auto* FastForward = OldBlend->GetBlendSamples()[0].Animation.Get();
    Blend->AddSample(FastForward, FVector(0, 600, 0)); Blend->AddSample(FastForward, FVector(0, 930, 0));
    Blend->ValidateSampleData(); Blend->ResampleData(); Blend->PostEditChange(); FAssetRegistryModule::AssetCreated(Blend); if (!Save(Blend)) return false;
    TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
    UAnimGraphNode_BlendSpacePlayer* Player = nullptr; UK2Node_VariableSet* SpeedSet = nullptr;
    for (auto* Graph : Graphs) for (UEdGraphNode* N : Graph->Nodes)
    {
        if (auto* P = Cast<UAnimGraphNode_BlendSpacePlayer>(N)) if (P->Node.GetBlendSpace() == OldBlend) Player = P;
        if (auto* V = Cast<UK2Node_VariableSet>(N)) if (V->VariableReference.GetMemberName() == TEXT("Speed") && BP->UbergraphPages.Contains(Graph)) SpeedSet = V;
    }
    if (!Player || !SpeedSet) { AddError(TEXT("Existing locomotion graph not found")); return false; }
    FEdGraphPinType Type; Type.PinCategory = UEdGraphSchema_K2::PC_Real; Type.PinSubCategory = UEdGraphSchema_K2::PC_Float;
    FBlueprintEditorUtils::AddMemberVariable(BP, TEXT("LocomotionDirection"), Type);
    auto* Graph = SpeedSet->GetGraph();
    auto Call = [&](UClass* Class, FName Name, int32 X, int32 Y)
    {
        auto* N = Node<UK2Node_CallFunction>(Graph, X, Y); N->SetFromFunction(Class->FindFunctionByName(Name)); N->ReconstructNode(); return N;
    };
    auto Connect = [&](UEdGraphNode* A, const TCHAR* Out, UEdGraphNode* B, const TCHAR* In)
    {
        auto* P = A->FindPin(Out); auto* Q = B->FindPin(In);
        return P && Q && A->GetGraph()->GetSchema()->TryCreateConnection(P, Q);
    };
    auto* Owner = Call(UAnimInstance::StaticClass(), TEXT("GetOwningActor"), 0, 500);
    auto* Velocity = Call(AActor::StaticClass(), TEXT("GetVelocity"), 230, 450);
    auto* Rotation = Call(AActor::StaticClass(), TEXT("K2_GetActorRotation"), 230, 650);
    auto* Calc = Call(UKismetAnimationLibrary::StaticClass(), TEXT("CalculateDirection"), 500, 500);
    auto* Set = Node<UK2Node_VariableSet>(Graph, SpeedSet->NodePosX + 250, SpeedSet->NodePosY);
    Set->VariableReference.SetSelfMember(TEXT("LocomotionDirection")); Set->ReconstructNode();
    auto* Then = SpeedSet->FindPin(UEdGraphSchema_K2::PN_Then);
    const auto Prior = Then->LinkedTo; Then->BreakAllPinLinks();
    for (auto* Pin : Prior) Graph->GetSchema()->TryCreateConnection(Set->FindPin(UEdGraphSchema_K2::PN_Then), Pin);
    if (!Connect(SpeedSet, TEXT("then"), Set, TEXT("execute")) || !Connect(Owner, TEXT("ReturnValue"), Velocity, TEXT("self"))
        || !Connect(Owner, TEXT("ReturnValue"), Rotation, TEXT("self")) || !Connect(Velocity, TEXT("ReturnValue"), Calc, TEXT("Velocity"))
        || !Connect(Rotation, TEXT("ReturnValue"), Calc, TEXT("BaseRotation")) || !Connect(Calc, TEXT("ReturnValue"), Set, TEXT("LocomotionDirection"))) return false;
    UEdGraphPin* SpeedSource = Player->FindPin(TEXT("X"))->LinkedTo.IsEmpty() ? nullptr : Player->FindPin(TEXT("X"))->LinkedTo[0];
    if (!SpeedSource) return false;
    Player->FindPin(TEXT("X"))->BreakAllPinLinks(); Player->Node.SetBlendSpace(Blend); Player->ReconstructNode();
    auto* Get = Node<UK2Node_VariableGet>(Player->GetGraph(), Player->NodePosX - 240, Player->NodePosY - 100);
    Get->VariableReference.SetSelfMember(TEXT("LocomotionDirection")); Get->ReconstructNode();
    if (!Connect(Get, TEXT("LocomotionDirection"), Player, TEXT("X")) || !Player->GetGraph()->GetSchema()->TryCreateConnection(SpeedSource, Player->FindPin(TEXT("Y")))) return false;
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP); FKismetEditorUtilities::CompileBlueprint(BP);
    return BP->Status != BS_Error && Save(BP);
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFinalizeDirectionalRates, "TunaSweeper.Generate.DirectionalRates", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFinalizeDirectionalRates::RunTest(const FString& Parameters)
{
    using namespace DirectionalAuthoring;
    auto* Blend = LoadObject<UBlendSpace>(nullptr, *(Folder + TEXT("BS_DirectionalWalk")));
    if (!Blend) return false;
    for (int32 I = 0; I < Blend->GetBlendSamples().Num(); ++I)
    {
        auto& Sample = const_cast<FBlendSample&>(Blend->GetBlendSamples()[I]);
        const FString Name = GetNameSafe(Sample.Animation);
        if (Name == TEXT("AS_Luna_WalkBackward") || Name.StartsWith(TEXT("AS_Luna_Strafe")))
        {
            Sample.RateScale = Sample.SampleValue.Y / (Name == TEXT("AS_Luna_WalkBackward") ? 95.f : 55.f);
            AddInfo(FString::Printf(TEXT("%s speed %.3f rate %.3f"), *Name, Sample.SampleValue.Y, Sample.RateScale));
        }
    }
    return Save(Blend);
}
#endif
