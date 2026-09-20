#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/Actor.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleSkirtElasticRecoveryTest,"TunaSweeper.Title.Skirt.ElasticRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleSkirtElasticRecoveryTest::RunTest(const FString& Parameters)
{
    auto* BodyAsset=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2"));
    auto* SkirtAsset=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/Skirt/SKM_LunaMk2_TitleSkirt"));
    auto* Class=LoadClass<UAnimInstance>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/Skirt/ABP_LunaMk2_TitleSkirt.ABP_LunaMk2_TitleSkirt_C"));
    auto* C=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/AS_LunaMk2_Title_C"));
    auto* A=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/AS_LunaMk2_Title_A"));
    if (!BodyAsset || !SkirtAsset || !Class || !C || !A) return false;
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Actor=World->SpawnActor<AActor>();
    auto* Body=NewObject<USkeletalMeshComponent>(Actor);
    Actor->SetRootComponent(Body);Body->SetSkeletalMeshAsset(BodyAsset);Body->RegisterComponent();Body->SetComponentTickEnabled(false);
    auto* Skirt=NewObject<USkeletalMeshComponent>(Actor);
    Skirt->SetupAttachment(Body);Skirt->SetSkeletalMeshAsset(SkirtAsset);Skirt->SetAnimInstanceClass(Class);
    Skirt->RegisterComponent();Skirt->SetComponentTickEnabled(false);
    Body->VisibilityBasedAnimTickOption=Skirt->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    const auto& Ref=SkirtAsset->GetRefSkeleton();
    TArray<int32> Bones;
    for (int32 I=0;I<Ref.GetNum();++I) if (Ref.GetBoneName(I).ToString().StartsWith(TEXT("skirt_"))) Bones.Add(I);
    TestTrue(TEXT("Simulated hem bones available"),Bones.Num()>8);
    TArray<FTransform> Previous;
    double LateStep=0,PeakStep=0,LateAngle=0;
    Body->PlayAnimation(C,false);
    for (int32 Frame=0;Frame<660;++Frame)
    {
        ++GFrameCounter;
        if (Frame==180) {Body->PlayAnimation(A,false);Body->SetPosition(0,false);Body->SetPlayRate(0);}
        Body->TickAnimation(1.f/60.f,false);Body->RefreshBoneTransforms();
        Skirt->TickAnimation(1.f/60.f,false);Skirt->RefreshBoneTransforms();
        const auto& Transforms=Skirt->GetComponentSpaceTransforms();
        for (int32 Bone : Bones)
        {
            if (!TestFalse(TEXT("Simulation stays finite"),Transforms[Bone].ContainsNaN())) break;
            if (Previous.Num()==Transforms.Num())
            {
                const double Step=FVector::Distance(Previous[Bone].GetTranslation(),Transforms[Bone].GetTranslation());
                PeakStep=FMath::Max(PeakStep,Step);
                if (Frame>=600) LateStep=FMath::Max(LateStep,Step);
            }
            if (Frame>=600)
            {
                const int32 Parent=Ref.GetParentIndex(Bone);
                const FTransform Local=Transforms[Bone].GetRelativeTransform(Transforms[Parent]);
                LateAngle=FMath::Max(LateAngle,FMath::RadiansToDegrees(double(Local.GetRotation().AngularDistance(Ref.GetRefBonePose()[Bone].GetRotation()))));
            }
        }
        Previous=Transforms;
    }
    AddInfo(FString::Printf(TEXT("Elastic recovery: peak step %.4f cm, settled step %.6f cm, settled local deviation %.3f degrees"),PeakStep,LateStep,LateAngle));
    TestTrue(TEXT("Skirt retains motion during entrance"),PeakStep>.05);
    TestTrue(TEXT("Skirt settles without persistent jitter"),LateStep<.02);
    TestTrue(TEXT("Skirt returns near its authored shape"),LateAngle<30.);
    GEngine->DestroyWorldContext(World);World->DestroyWorld(false);
    return !HasAnyErrors();
}
#endif
