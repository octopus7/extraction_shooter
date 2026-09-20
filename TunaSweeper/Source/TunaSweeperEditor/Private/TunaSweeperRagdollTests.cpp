#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "AssetCompilingManager.h"
#include "ContentStreaming.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "Engine/DamageEvents.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLunaRagdollArticulationTest,
    "TunaSweeper.Character.Ragdoll.Articulation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLunaRagdollArticulationTest::RunTest(const FString& Parameters)
{
    auto* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2"));
    if (!TestNotNull(TEXT("Player mesh"), Mesh)) return false;
    auto* Physics = Mesh->GetPhysicsAsset();
    if (!TestNotNull(TEXT("Player physics"), Physics)) return false;
    for (const auto& Body : Physics->SkeletalBodySetups)
        TestTrue(TEXT("Per-body solver settings are actually enabled"), Body->DefaultInstance.GetPositionSolverIterationCount() >= 16);
    for (const TCHAR* Side : {TEXT("l"), TEXT("r")})
    {
        const FName Thigh(*FString::Printf(TEXT("thigh_%s"), Side));
        const FName Calf(*FString::Printf(TEXT("calf_%s"), Side));
        const FName Foot(*FString::Printf(TEXT("foot_%s"), Side));
        const FName Upper(*FString::Printf(TEXT("upperarm_%s"), Side));
        const FName Lower(*FString::Printf(TEXT("lowerarm_%s"), Side));
        const FName Hand(*FString::Printf(TEXT("hand_%s"), Side));
        for (const FName Bone : {Thigh, Calf, Foot, Upper, Lower, Hand})
            TestTrue(FString::Printf(TEXT("Articulated body exists: %s"), *Bone.ToString()), Physics->FindBodyIndex(Bone) != INDEX_NONE);
        for (const auto& Pair : {TPair<FName, FName>(TEXT("pelvis"), Thigh), {Thigh, Calf}, {Calf, Foot}, {Upper, Lower}, {Lower, Hand}})
        {
            const UPhysicsConstraintTemplate* Found = nullptr;
            for (const auto& Joint : Physics->ConstraintSetup)
            {
                const auto& C = Joint->DefaultInstance;
                if ((C.ConstraintBone1 == Pair.Key && C.ConstraintBone2 == Pair.Value) ||
                    (C.ConstraintBone2 == Pair.Key && C.ConstraintBone1 == Pair.Value)) Found = Joint;
            }
            if (!TestNotNull(FString::Printf(TEXT("Anatomical joint %s -> %s"), *Pair.Key.ToString(), *Pair.Value.ToString()), Found)) continue;
            const auto& C = Found->DefaultInstance;
            TestTrue(TEXT("Joint cannot translate apart"), C.GetLinearXMotion() == LCM_Locked && C.GetLinearYMotion() == LCM_Locked && C.GetLinearZMotion() == LCM_Locked);
            if (Pair.Value == Calf || Pair.Value == Lower)
            {
                TestFalse(TEXT("Anatomical swing stop cannot yield like a spring"), C.ProfileInstance.ConeLimit.bSoftConstraint != 0);
                TestFalse(TEXT("Anatomical twist stop cannot yield like a spring"), C.ProfileInstance.TwistLimit.bSoftConstraint != 0);
                TestTrue(TEXT("Hinge resists lateral bending"), C.GetAngularSwing2Motion() == ACM_Locked || C.GetAngularSwing2Limit() <= 5.f);
                TestTrue(TEXT("Hinge resists twisting"), C.GetAngularTwistMotion() == ACM_Locked || C.GetAngularTwistLimit() <= 5.f);
            }
        }
    }
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLunaRagdollImpactTest,
    "TunaSweeper.Character.Ragdoll.ImpactStability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLunaRagdollImpactTest::RunTest(const FString& Parameters)
{
    auto* Asset = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2"));
    if (!TestNotNull(TEXT("Player mesh"), Asset)) return false;
    // Exercise gravity alone and the actual death impulse in opposite directions.
    for (const float Direction : {0.f, -1.f, 1.f})
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        auto* Ground = World->SpawnActor<AActor>();
        auto* Floor = NewObject<UBoxComponent>(Ground);
        Ground->SetRootComponent(Floor);
        Floor->SetBoxExtent(FVector(3000, 3000, 10));
        Floor->SetCollisionProfileName(TEXT("BlockAll"));
        Floor->RegisterComponent();
        Ground->SetActorLocation(FVector(0, 0, -10));
        auto* PlayerClass = LoadClass<ATunaSweeperTopDownCharacter>(nullptr,
            TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
        if (!TestNotNull(TEXT("Saved player blueprint"), PlayerClass)) return false;
        auto* Actor = World->SpawnActor<ATunaSweeperTopDownCharacter>(PlayerClass, FVector(0, 0, 180), FRotator::ZeroRotator);
        auto* Mesh = Actor->GetMesh();
        Mesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
        Mesh->RefreshBoneTransforms();
        World->InitializeActorsForPlay(FURL());
        World->GetWorldSettings()->NotifyBeginPlay();
        World->GetWorldSettings()->NotifyMatchStarted();
        World->BeginPlay();
        if (Direction == 0.f)
        {
            FindFProperty<FFloatProperty>(Actor->GetClass(), TEXT("DeathRagdollHorizontalImpulse"))->SetPropertyValue_InContainer(Actor, 0.f);
            FindFProperty<FFloatProperty>(Actor->GetClass(), TEXT("DeathRagdollUpwardImpulse"))->SetPropertyValue_InContainer(Actor, 0.f);
        }
        auto* DamageCauser = World->SpawnActor<AActor>();
        auto* DamageRoot = NewObject<USceneComponent>(DamageCauser);
        DamageCauser->SetRootComponent(DamageRoot);
        DamageRoot->RegisterComponent();
        DamageCauser->SetActorLocation(Actor->GetActorLocation() - Actor->GetActorForwardVector() * Direction * 300.f);
        TestTrue(TEXT("Directional lethal damage is accepted"), Actor->TakeDamage(10000.f, FDamageEvent(), nullptr, DamageCauser) > 0.f);
        // Keep the real death transition but suppress only the test's level-travel timer.
        World->GetTimerManager().ClearAllTimersForObject(Actor);
        TestTrue(TEXT("Real death path starts simulation"), Mesh->IsSimulatingPhysics(TEXT("pelvis")));
        for (const FBodyInstance* Body : Mesh->Bodies)
            TestTrue(TEXT("Death solver quality reaches the live bodies"), Body->GetPositionSolverIterationCount() >= 16);
        USceneCaptureComponent2D* Capture = nullptr;
        UTextureRenderTarget2D* Target = nullptr;
        if (FParse::Param(FCommandLine::Get(), TEXT("RagdollCapture")))
        {
            Capture = NewObject<USceneCaptureComponent2D>(Actor);
            Target = NewObject<UTextureRenderTarget2D>(Capture);
            Target->InitCustomFormat(640, 640, PF_B8G8R8A8, false);
            Capture->TextureTarget = Target;
            Capture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
            Capture->bCaptureEveryFrame = false;
            Capture->bCaptureOnMovement = false;
            Capture->FOVAngle = 35;
            Capture->RegisterComponentWithWorld(World);
            FAssetCompilingManager::Get().FinishAllCompilation();
            IStreamingManager::Get().StreamAllResources(5.f);
        }
        const FVector Start = Mesh->GetBodyInstance(TEXT("pelvis"))->GetUnrealWorldTransform().GetLocation();
        double MaxGap = 0, LateGap = 0, LateSpeed = 0, MaxHingeExcess = 0;
        FString WorstHinge;
        bool bFinite = true;
        for (int32 Frame = 0; Frame < 480; ++Frame)
        {
            ++GFrameCounter;
            World->Tick(LEVELTICK_All, 1.f / 60.f);
            FPlatformProcess::Sleep(0.001f);
            for (const FBodyInstance* Body : Mesh->Bodies)
            {
                bFinite &= !Body->GetUnrealWorldTransform().ContainsNaN();
                if (Frame > 420) LateSpeed = FMath::Max(LateSpeed, Body->GetUnrealWorldVelocity().Length());
            }
            for (const FConstraintInstance* Joint : Mesh->Constraints)
            {
                const FBodyInstance* Child = Mesh->GetBodyInstance(Joint->ConstraintBone1);
                const FBodyInstance* Parent = Mesh->GetBodyInstance(Joint->ConstraintBone2);
                if (!Child || !Parent) { bFinite = false; continue; }
                const FVector P1 = Child->GetUnrealWorldTransform().TransformPosition(Joint->Pos1);
                const FVector P2 = Parent->GetUnrealWorldTransform().TransformPosition(Joint->Pos2);
                const double Gap = FVector::Distance(P1, P2);
                MaxGap = FMath::Max(MaxGap, Gap);
                if (Frame > 420) LateGap = FMath::Max(LateGap, Gap);
                const FString Name = Joint->JointName.ToString();
                if (Name.StartsWith(TEXT("calf_")) || Name.StartsWith(TEXT("lowerarm_")))
                {
                    const double PreviousExcess = MaxHingeExcess;
                    MaxHingeExcess = FMath::Max(MaxHingeExcess, double(FMath::Abs(FMath::RadiansToDegrees(Joint->GetCurrentSwing1())) - Joint->GetAngularSwing1Limit()));
                    MaxHingeExcess = FMath::Max(MaxHingeExcess, double(FMath::Abs(FMath::RadiansToDegrees(Joint->GetCurrentSwing2())) - Joint->GetAngularSwing2Limit()));
                    MaxHingeExcess = FMath::Max(MaxHingeExcess, double(FMath::Abs(FMath::RadiansToDegrees(Joint->GetCurrentTwist())) - Joint->GetAngularTwistLimit()));
                    if (MaxHingeExcess > PreviousExcess)
                        WorstHinge = FString::Printf(TEXT("%s at frame %d: swing1 %.2f swing2 %.2f twist %.2f"), *Name, Frame,
                            FMath::RadiansToDegrees(Joint->GetCurrentSwing1()), FMath::RadiansToDegrees(Joint->GetCurrentSwing2()), FMath::RadiansToDegrees(Joint->GetCurrentTwist()));
                }
            }
            if (Capture && (Frame == 60 || Frame == 180 || Frame == 479))
            {
                const FVector Aim = Mesh->GetBodyInstance(TEXT("pelvis"))->GetUnrealWorldTransform().GetLocation();
                const FVector Camera = Aim + FVector(190, 260, 300);
                Capture->SetWorldLocationAndRotation(Camera, (Aim - Camera).Rotation());
                Mesh->MarkRenderDynamicDataDirty();
                World->SendAllEndOfFrameUpdates();
                FlushRenderingCommands();
                for (int32 Warmup = 0; Warmup < 3; ++Warmup) { Capture->CaptureScene(); FlushRenderingCommands(); }
                const FString Directory = FPaths::ProjectSavedDir() / TEXT("Automation/RagdollAudit/Capture");
                IFileManager::Get().MakeDirectory(*Directory, true);
                FImage Pixels;
                if (TestTrue(TEXT("Ragdoll capture readable"), FImageUtils::GetRenderTargetImage(Target, Pixels)))
                    TestTrue(TEXT("Ragdoll capture saved"), FImageUtils::SaveImageByExtension(
                        *(Directory / FString::Printf(TEXT("Impact_%.0f_%d.png"), Direction, Frame)), Pixels));
            }
        }
        const FVector End = Mesh->GetBodyInstance(TEXT("pelvis"))->GetUnrealWorldTransform().GetLocation();
        AddInfo(FString::Printf(TEXT("Impulse direction %.0f: joint gap peak %.3f cm / settled %.3f cm, settled speed %.3f cm/s, hinge excess %.3f deg, pelvis %s -> %s"),
            Direction, MaxGap, LateGap, LateSpeed, MaxHingeExcess, *Start.ToString(), *End.ToString()));
        AddInfo(TEXT("Peak hinge: ") + WorstHinge);
        TestTrue(TEXT("Gravity actually moved the ragdoll"), Start.Z - End.Z > 30.0);
        TestTrue(TEXT("All rigid transforms remain finite"), bFinite);
        TestTrue(TEXT("Body stays above ground"), End.Z > -5.0);
        TestTrue(TEXT("Impact does not pull joints apart"), MaxGap < 5.0);
        TestTrue(TEXT("Settled joints remain connected"), LateGap < 1.5);
        TestTrue(TEXT("Ragdoll settles instead of continually thrashing"), LateSpeed < 30.0);
        TestTrue(TEXT("Knees and elbows respect their angular stops"), MaxHingeExcess < 10.0);
        World->EndPlay(EEndPlayReason::Quit);
        World->DestroyWorld(false);
        GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
    }
    return !HasAnyErrors();
}
#endif
