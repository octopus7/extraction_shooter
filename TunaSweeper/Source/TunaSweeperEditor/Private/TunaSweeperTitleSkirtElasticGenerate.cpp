#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_RigidBody.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleSkirtElasticGenerate,"TunaSweeper.Title.Skirt.ElasticGenerate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleSkirtElasticGenerate::RunTest(const FString& Parameters)
{
    auto* PA=LoadObject<UPhysicsAsset>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/Skirt/PA_LunaMk2_TitleSkirt"));
    auto* BP=LoadObject<UAnimBlueprint>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/Skirt/ABP_LunaMk2_TitleSkirt"));
    if (!PA || !BP) return false;
    const bool Apply=FParse::Param(FCommandLine::Get(),TEXT("ApplyTitleSkirtElastic"));
    float Spring=800.f,Damping=50.f;
    FParse::Value(FCommandLine::Get(),TEXT("TitleSkirtSpring="),Spring);
    FParse::Value(FCommandLine::Get(),TEXT("TitleSkirtDamping="),Damping);
    for (const auto& Joint : PA->ConstraintSetup)
    {
        auto& CI=Joint->DefaultInstance;
        float S,D,F;CI.GetAngularDriveParams(S,D,F);
        AddInfo(FString::Printf(TEXT("%s: drive=%d spring=%.2f damping=%.2f limits=%.1f/%.1f/%.1f"),*CI.JointName.ToString(),CI.IsAngularOrientationDriveEnabled(),S,D,CI.GetAngularSwing1Limit(),CI.GetAngularSwing2Limit(),CI.GetAngularTwistLimit()));
        if (!Apply) continue;
        CI.SetAngularDriveMode(EAngularDriveMode::TwistAndSwing);
        CI.SetOrientationDriveTwistAndSwing(true,true);
        CI.SetAngularVelocityDriveTwistAndSwing(true,true);
        CI.SetAngularDriveAccelerationMode(true);
        CI.SetAngularDriveParams(Spring,Damping,0.f);
        CI.SetAngularOrientationTarget(FQuat::Identity);
        CI.SetAngularVelocityTarget(FVector::ZeroVector);
        CI.SetAngularSwing1Limit(ACM_Limited,18.f);
        CI.SetAngularSwing2Limit(ACM_Limited,18.f);
        CI.SetAngularTwistLimit(ACM_Limited,8.f);
        Joint->SetDefaultProfile(CI);
        Joint->MarkPackageDirty();
    }
    for (const auto& Body : PA->SkeletalBodySetups)
    {
        AddInfo(FString::Printf(TEXT("%s damping=%.2f/%.2f"),*Body->BoneName.ToString(),Body->DefaultInstance.LinearDamping,Body->DefaultInstance.AngularDamping));
        if (Apply) {Body->DefaultInstance.LinearDamping=2.f;Body->DefaultInstance.AngularDamping=6.f;}
    }
    TArray<UAnimGraphNode_RigidBody*> Nodes;
    FBlueprintEditorUtils::GetAllNodesOfClass(BP,Nodes);
    for (auto* Node : Nodes)
    {
        AddInfo(FString::Printf(TEXT("RigidBody space=%d worldAlpha=%.2f gravity=%s"),int32(Node->Node.SimulationSpace),Node->Node.SimSpaceSettings.WorldAlpha,*Node->Node.OverrideWorldGravity.ToString()));
        if (Apply)
        {
            // A quiet title presentation needs less inertial overshoot than gameplay.
            Node->Node.SimSpaceSettings.WorldAlpha=.2f;
            Node->Node.SimSpaceSettings.DampingAlpha=0.f;
            Node->Node.bOverrideWorldGravity=true;
            Node->Node.OverrideWorldGravity=FVector(0,0,-300.f);
        }
    }
    if (Apply)
    {
        FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
        FKismetEditorUtilities::CompileBlueprint(BP);
        for (UObject* Asset : {static_cast<UObject*>(PA),static_cast<UObject*>(BP)})
        {
            UPackage* Package=Asset->GetOutermost();Package->MarkPackageDirty();
            FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;Args.SaveFlags=SAVE_NoError;
            if (!UPackage::SavePackage(Package,Asset,*FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension()),Args)) return false;
        }
    }
    return !HasAnyErrors();
}
#endif
