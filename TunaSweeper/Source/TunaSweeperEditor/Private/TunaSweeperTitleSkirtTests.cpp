#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Title/TunaSweeperTitlePresentationActor.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimBlueprint.h"
#include "AnimGraphNode_CopyPoseFromMesh.h"
#include "AnimGraphNode_RigidBody.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "SkeletalMeshAttributes.h"
#include "BoneWeights.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "AssetCompilingManager.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "ContentStreaming.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleSkirtFollowsBodyTest,
    "TunaSweeper.Title.Skirt.FollowsBody",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleSkirtFollowsBodyTest::RunTest(const FString& Parameters)
{
    auto* Actor = GetMutableDefault<ATunaSweeperTitlePresentationActor>();
    const auto* Body = Cast<USkeletalMeshComponent>(Actor->GetDefaultSubobjectByName(TEXT("BodyMesh")));
    const auto* Skirt = Cast<USkeletalMeshComponent>(Actor->GetDefaultSubobjectByName(TEXT("Skirt")));
    if (!TestNotNull(TEXT("Body component"), Body) || !TestNotNull(TEXT("Skirt component"), Skirt)) return false;
    TestEqual(TEXT("Skirt uses body component as pose source parent"), Skirt->GetAttachParent(), static_cast<USceneComponent*>(const_cast<USkeletalMeshComponent*>(Body)));
    TestEqual(TEXT("No bone socket transform is applied a second time"), Skirt->GetAttachSocketName(), NAME_None);
    TestTrue(TEXT("Skirt and body use the same component space"), Skirt->GetRelativeTransform().Equals(FTransform::Identity, 0.001f));
    const auto* Mesh = Skirt->GetSkeletalMeshAsset();
    const auto* BodyAsset = Body->GetSkeletalMeshAsset();
    if (!TestNotNull(TEXT("Skirt mesh"), Mesh) || !TestNotNull(TEXT("Body mesh"), BodyAsset)) return false;
    const auto& Ref = Mesh->GetRefSkeleton();
    const auto& BodyRef = BodyAsset->GetRefSkeleton();
    for (FName Name : {FName(TEXT("root")), FName(TEXT("pelvis")), FName(TEXT("spine_01")), FName(TEXT("spine_02"))})
    {
        int32 Index = Ref.FindBoneIndex(Name), BodyIndex = BodyRef.FindBoneIndex(Name);
        if (TestTrue(FString::Printf(TEXT("Common bone %s exists"), *Name.ToString()), Index != INDEX_NONE && BodyIndex != INDEX_NONE))
        {
            TestTrue(TEXT("Common local reference transforms match"), Ref.GetRefBonePose()[Index].Equals(BodyRef.GetRefBonePose()[BodyIndex], 0.001f));
            const int32 Parent = Ref.GetParentIndex(Index), BodyParent = BodyRef.GetParentIndex(BodyIndex);
            TestEqual(TEXT("Common parent chain matches"), Parent == INDEX_NONE ? NAME_None : Ref.GetBoneName(Parent), BodyParent == INDEX_NONE ? NAME_None : BodyRef.GetBoneName(BodyParent));
        }
    }
    TestTrue(TEXT("Independent skirt rig retained"), Ref.FindBoneIndex(TEXT("SkirtRoot")) != INDEX_NONE);
    const FMeshDescription* Description = Mesh->GetMeshDescription(0);
    if (!TestNotNull(TEXT("Editable source mesh is retained"), Description)) return false;
    FSkeletalMeshConstAttributes Attributes(*Description);
    auto Weights = Attributes.GetVertexSkinWeights();
    int32 WaistVertices = 0, PhysicsVertices = 0;
    for (FVertexID Vertex : Description->Vertices().GetElementIDs())
    {
        float Sum = 0, Waist = 0, Physics = 0;
        for (const auto& Weight : Weights.Get(Vertex))
        {
            const int32 Bone = Weight.GetBoneIndex();
            if (!TestTrue(TEXT("Skin bone index valid"), Bone >= 0 && Bone < Ref.GetRawBoneNum())) return false;
            const float Value = Weight.GetWeight();
            Sum += Value;
            const FString Name = Ref.GetBoneName(Bone).ToString();
            if (Name.StartsWith(TEXT("spine_")) || Name == TEXT("pelvis")) Waist += Value;
            if (Name.StartsWith(TEXT("skirt_"))) Physics += Value;
        }
        if (!TestTrue(TEXT("Skin weights normalized"), FMath::IsNearlyEqual(Sum, 1.f, 0.001f))) return false;
        WaistVertices += Waist > 0.1f;
        PhysicsVertices += Physics > 0.9f;
    }
    TestTrue(TEXT("Waist follows actual body bones"), WaistVertices > 20);
    TestTrue(TEXT("Hem retains independent skirt physics weights"), PhysicsVertices > 20);
    if (!TestNotNull(TEXT("Skirt physics asset"), Mesh->GetPhysicsAsset())) return false;
    for (const auto& PhysicsBody : Mesh->GetPhysicsAsset()->SkeletalBodySetups)
    {
        TestTrue(TEXT("Physics bodies retain independent skirt bone names"), PhysicsBody->BoneName.ToString().StartsWith(TEXT("skirt_")) || PhysicsBody->BoneName == TEXT("SkirtRoot"));
        TestTrue(TEXT("Physics body bone exists"), Ref.FindBoneIndex(PhysicsBody->BoneName) != INDEX_NONE);
    }
    auto* AnimBP = LoadObject<UAnimBlueprint>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/Skirt/ABP_LunaMk2_TitleSkirt"));
    if (!TestNotNull(TEXT("Title skirt animation asset"), AnimBP)) return false;
    TArray<UAnimGraphNode_CopyPoseFromMesh*> CopyNodes;
    TArray<UAnimGraphNode_RigidBody*> PhysicsNodes;
    FBlueprintEditorUtils::GetAllNodesOfClass(AnimBP, CopyNodes);
    FBlueprintEditorUtils::GetAllNodesOfClass(AnimBP, PhysicsNodes);
    TestEqual(TEXT("One body pose source"), CopyNodes.Num(), 1);
    TestEqual(TEXT("Skirt physics remains active"), PhysicsNodes.Num(), 1);
    if (CopyNodes.Num() == 1) TestTrue(TEXT("Copy pose resolves BodyMesh parent"), CopyNodes[0]->Node.bUseAttachedParent != 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleSkirtClearanceTest,
    "TunaSweeper.Title.Skirt.WaistClearance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleSkirtClearanceTest::RunTest(const FString& Parameters)
{
    auto* Body = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2"));
    auto* Skirt = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/Skirt/SKM_LunaMk2_TitleSkirt"));
    if (!Body || !Skirt || !Body->GetMeshDescription(0) || !Skirt->GetMeshDescription(0)) return false;
    const auto* Description = Body->GetMeshDescription(0);
    FSkeletalMeshConstAttributes BodyAttributes(*Description), SkirtAttributes(*Skirt->GetMeshDescription(0));
    const auto Positions = BodyAttributes.GetVertexPositions();
    const auto Weights = BodyAttributes.GetVertexSkinWeights();
    TArray<FTriangleID> TorsoTriangles;
    for (FTriangleID T : Description->Triangles().GetElementIDs())
    {
        bool bTorso = true;
        for (FVertexID V : Description->GetTriangleVertices(T))
        {
            float Torso = 0;
            for (const auto& W : Weights.Get(V))
            {
                const FString Name = Body->GetRefSkeleton().GetBoneName(W.GetBoneIndex()).ToString();
                if (Name.StartsWith(TEXT("spine_")) || Name == TEXT("pelvis") || Name == TEXT("cc_base_pelvis")) Torso += W.GetWeight();
            }
            bTorso &= Torso > 0.95f && Positions[V].Z > 78.f && Positions[V].Z < 112.f;
        }
        if (bTorso) TorsoTriangles.Add(T);
    }
    if (!TestTrue(TEXT("Torso collision surface found"), TorsoTriangles.Num() > 10)) return false;
    const auto& Ref = Body->GetRefSkeleton();
    int32 Bone = Ref.FindBoneIndex(TEXT("pelvis"));
    FTransform Pelvis = FTransform::Identity;
    for (; Bone != INDEX_NONE; Bone = Ref.GetParentIndex(Bone)) Pelvis *= Ref.GetRefBonePose()[Bone];
    int32 Checked = 0;
    for (FVertexID V : Skirt->GetMeshDescription(0)->Vertices().GetElementIDs())
    {
        const FVector P(SkirtAttributes.GetVertexPositions()[V]);
        if (P.Z <= 96) continue;
        double Best = TNumericLimits<double>::Max(), Clearance = 0;
        for (FTriangleID T : TorsoTriangles)
        {
            const auto TV = Description->GetTriangleVertices(T);
            const FVector A(Positions[TV[0]]), B(Positions[TV[1]]), C(Positions[TV[2]]);
            const FVector Q = FMath::ClosestPointOnTriangleToPoint(P, A, B, C);
            const double D = FVector::DistSquared(P, Q);
            if (D >= Best) continue;
            Best = D;
            FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
            if (FVector::DotProduct(Normal, FVector(Q.X, Q.Y - Pelvis.GetTranslation().Y, 0)) < 0) Normal *= -1;
            // At a shared edge the nearest point belongs to multiple face planes.
            // Measure distance to the actual surface, using the normal only for the side.
            Clearance = FMath::Sqrt(D) * (FVector::DotProduct(P - Q, Normal) < -0.00001 ? -1.0 : 1.0);
        }
        TestTrue(FString::Printf(TEXT("Upper waist vertex %d stays outside torso (%.3f cm)"), V.GetValue(), Clearance), Clearance >= 0.60);
        ++Checked;
    }
    TestTrue(TEXT("Upper waist was checked"), Checked > 20);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleSkirtLayersTest,
    "TunaSweeper.Title.Skirt.ApronLayers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleSkirtLayersTest::RunTest(const FString& Parameters)
{
    auto* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/Skirt/SKM_LunaMk2_TitleSkirt"));
    if (!Mesh || !Mesh->GetMeshDescription(0)) return false;
    const auto& Description = *Mesh->GetMeshDescription(0);
    FSkeletalMeshConstAttributes Attributes(Description);
    const auto Positions = Attributes.GetVertexPositions();
    auto Island = [&](int32 Seed)
    {
        TSet<FVertexID> Vertices{FVertexID(Seed)};
        bool bExpanded = true;
        while (bExpanded)
        {
            bExpanded = false;
            for (FTriangleID T : Description.Triangles().GetElementIDs())
            {
                const auto TV = Description.GetTriangleVertices(T);
                if (!Vertices.Contains(TV[0]) && !Vertices.Contains(TV[1]) && !Vertices.Contains(TV[2])) continue;
                for (FVertexID V : TV) if (!Vertices.Contains(V)) { Vertices.Add(V); bExpanded = true; }
            }
        }
        return Vertices;
    };
    // Source topology identifies the white front panel and black skirt as separate islands.
    const auto Apron = Island(2), Skirt = Island(101);
    if (!TestEqual(TEXT("Apron panel topology"), Apron.Num(), 97) || !TestEqual(TEXT("Black skirt topology"), Skirt.Num(), 196)) return false;
    TArray<FTriangleID> ApronTriangles, SkirtTriangles;
    for (FTriangleID T : Description.Triangles().GetElementIDs())
    {
        const FVertexID V = Description.GetTriangleVertices(T)[0];
        if (Apron.Contains(V)) ApronTriangles.Add(T);
        if (Skirt.Contains(V)) SkirtTriangles.Add(T);
    }
    int32 Intersections = 0;
    for (FTriangleID AT : ApronTriangles)
        for (FTriangleID ST : SkirtTriangles)
        {
            const auto AV = Description.GetTriangleVertices(AT), SV = Description.GetTriangleVertices(ST);
            FVector A[3], S[3]; FBox AB(ForceInit), SB(ForceInit);
            for (int32 K = 0; K < 3; ++K) { A[K] = FVector(Positions[AV[K]]); S[K] = FVector(Positions[SV[K]]); AB += A[K]; SB += S[K]; }
            if (AB.Max.Z <= 96 || SB.Max.Z <= 96 || !AB.Intersect(SB)) continue;
            bool bIntersects = false;
            for (int32 K = 0; K < 3; ++K)
            {
                FVector Hit, Normal;
                bIntersects |= FMath::SegmentTriangleIntersection(A[K], A[(K+1)%3], S[0], S[1], S[2], Hit, Normal) && Hit.Z > 96;
                bIntersects |= FMath::SegmentTriangleIntersection(S[K], S[(K+1)%3], A[0], A[1], A[2], Hit, Normal) && Hit.Z > 96;
            }
            Intersections += bIntersects;
        }
    TestEqual(TEXT("Upper apron does not intersect black skirt triangles"), Intersections, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleSkirtAnimatedPoseTest,
    "TunaSweeper.Title.Skirt.AnimatedPose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleSkirtAnimatedPoseTest::RunTest(const FString& Parameters)
{
    if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir() / TEXT("Maps/IntroMap.umap"), false, true)) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    ATunaSweeperTitlePresentationActor* Actor = nullptr;
    for (TActorIterator<ATunaSweeperTitlePresentationActor> It(World); It; ++It) { Actor = *It; break; }
    if (!TestNotNull(TEXT("Saved title actor"), Actor)) return false;
    USkeletalMeshComponent* Body = nullptr; USkeletalMeshComponent* Skirt = nullptr;
    for (auto* C : TInlineComponentArray<USkeletalMeshComponent*>(Actor))
    {
        if (C->GetName() == TEXT("BodyMesh")) Body = C;
        if (C->GetName() == TEXT("Skirt")) Skirt = C;
    }
    if (!Body || !Skirt) return false;
    TestEqual(TEXT("Serialized instance has no bone socket"), Skirt->GetAttachSocketName(), NAME_None);
    TestTrue(TEXT("Serialized instance shares body coordinates"), Skirt->GetRelativeTransform().Equals(FTransform::Identity));
    TestEqual(TEXT("Serialized instance uses corrected mesh"), Skirt->GetSkeletalMeshAsset()->GetName(), FString(TEXT("SKM_LunaMk2_TitleSkirt")));
    Actor->DispatchBeginPlay();
    Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Skirt->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    FAssetCompilingManager::Get().FinishAllCompilation();
    IStreamingManager::Get().StreamAllResources(5.f);
    auto* Camera = Actor->FindComponentByClass<UCameraComponent>();
    auto* Capture = NewObject<USceneCaptureComponent2D>(Actor);
    auto* Target = NewObject<UTextureRenderTarget2D>();
    Target->InitCustomFormat(1280, 720, PF_B8G8R8A8, false);
    Capture->TextureTarget = Target;
    Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
    Capture->RegisterComponentWithWorld(World);
    Capture->SetWorldTransform(Camera->GetComponentTransform());
    Capture->FOVAngle = Camera->FieldOfView;
    Capture->PostProcessSettings = Camera->PostProcessSettings;
    const auto& BodyRef = Body->GetSkeletalMeshAsset()->GetRefSkeleton();
    const auto& SkirtRef = Skirt->GetSkeletalMeshAsset()->GetRefSkeleton();
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("TitleSkirt");
    IFileManager::Get().MakeDirectory(*Directory, true);
    for (int32 Frame = 0; Frame <= 180; ++Frame)
    {
        Body->TickAnimation(1.f / 60.f, false); Body->RefreshBoneTransforms();
        Skirt->TickAnimation(1.f / 60.f, false); Skirt->RefreshBoneTransforms();
        if (Frame != 30 && Frame != 90 && Frame != 180) continue;
        for (FName Bone : {FName(TEXT("pelvis")), FName(TEXT("spine_01")), FName(TEXT("spine_02")), FName(TEXT("spine_03"))})
        {
            const int32 BI = BodyRef.FindBoneIndex(Bone), SI = SkirtRef.FindBoneIndex(Bone);
            if (!TestTrue(TEXT("Pose comparison bone exists"), BI >= 0 && SI >= 0)) return false;
            TestTrue(FString::Printf(TEXT("%s follows current body pose at frame %d"), *Bone.ToString(), Frame),
                Body->GetComponentSpaceTransforms()[BI].Equals(Skirt->GetComponentSpaceTransforms()[SI], 0.01f));
        }
        World->SendAllEndOfFrameUpdates(); FlushRenderingCommands();
        Capture->CaptureScene(); FlushRenderingCommands();
        FImage Pixels;
        if (TestTrue(TEXT("Animation capture readable"), FImageUtils::GetRenderTargetImage(Target, Pixels)))
            TestTrue(TEXT("Animation capture saved"), FImageUtils::SaveImageByExtension(*(Directory / FString::Printf(TEXT("title_%03d.png"), Frame)), Pixels));
        Capture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
        Capture->CaptureScene(); FlushRenderingCommands();
        if (TestTrue(TEXT("Unlit diagnostic capture readable"), FImageUtils::GetRenderTargetImage(Target, Pixels)))
            TestTrue(TEXT("Unlit diagnostic capture saved"), FImageUtils::SaveImageByExtension(*(Directory / FString::Printf(TEXT("title_basecolor_%03d.png"), Frame)), Pixels));
        Capture->HiddenComponents.Add(Body);
        Capture->CaptureScene(); FlushRenderingCommands();
        if (TestTrue(TEXT("Garment diagnostic capture readable"), FImageUtils::GetRenderTargetImage(Target, Pixels)))
            TestTrue(TEXT("Garment diagnostic capture saved"), FImageUtils::SaveImageByExtension(*(Directory / FString::Printf(TEXT("title_garment_%03d.png"), Frame)), Pixels));
        Capture->HiddenComponents.Remove(Body);
        Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
    }
    Capture->DestroyComponent();
    return true;
}
#endif
