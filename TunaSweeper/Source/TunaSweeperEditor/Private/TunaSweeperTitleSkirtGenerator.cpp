// One-off asset migration. Commit with generated assets, then remove this file.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#include "Title/TunaSweeperTitlePresentationActor.h"
#include "SkeletalMeshAttributes.h"
#include "BoneWeights.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Animation/Skeleton.h"
#include "Animation/AnimBlueprint.h"
#include "Animation/AnimBlueprintGeneratedClass.h"
#include "AnimGraphNode_CopyPoseFromMesh.h"
#include "AnimGraphNode_RigidBody.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "StaticMeshOperations.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetCompilingManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphSchema.h"
#include "UObject/UObjectIterator.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "FileHelpers.h"
#include "EngineUtils.h"
#include "RenderingThread.h"

namespace TitleSkirtMigration
{
TArray<FTransform> ComponentPose(const FReferenceSkeleton& Ref)
{
    TArray<FTransform> Result = Ref.GetRawRefBonePose();
    for (int32 I = 0; I < Result.Num(); ++I)
        if (Ref.GetParentIndex(I) != INDEX_NONE) Result[I] *= Result[Ref.GetParentIndex(I)];
    return Result;
}
TSharedPtr<FJsonObject> Dump(USkeletalMesh* Mesh)
{
    auto Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("asset"), Mesh->GetPathName());
    const auto& Ref = Mesh->GetRefSkeleton();
    const auto Pose = ComponentPose(Ref);
    TArray<TSharedPtr<FJsonValue>> Bones, Vertices, Triangles;
    for (int32 I = 0; I < Ref.GetRawBoneNum(); ++I)
    {
        auto Bone = MakeShared<FJsonObject>();
        Bone->SetStringField(TEXT("name"), Ref.GetBoneName(I).ToString());
        Bone->SetNumberField(TEXT("parent"), Ref.GetParentIndex(I));
        Bone->SetStringField(TEXT("local"), Ref.GetRefBonePose()[I].ToString());
        Bone->SetStringField(TEXT("component"), Pose[I].ToString());
        Bones.Add(MakeShared<FJsonValueObject>(Bone));
    }
    const auto* Description = Mesh->GetMeshDescription(0);
    FSkeletalMeshConstAttributes Attributes(*Description);
    const auto Positions = Attributes.GetVertexPositions();
    const auto Weights = Attributes.GetVertexSkinWeights();
    for (FVertexID V : Description->Vertices().GetElementIDs())
    {
        auto Vertex = MakeShared<FJsonObject>();
        const FVector3f P = Positions[V];
        Vertex->SetNumberField(TEXT("id"), V.GetValue());
        Vertex->SetArrayField(TEXT("p"), {MakeShared<FJsonValueNumber>(P.X), MakeShared<FJsonValueNumber>(P.Y), MakeShared<FJsonValueNumber>(P.Z)});
        TArray<TSharedPtr<FJsonValue>> W;
        for (const auto& Weight : Weights.Get(V))
            W.Add(MakeShared<FJsonValueArray>(TArray<TSharedPtr<FJsonValue>>{MakeShared<FJsonValueNumber>(Weight.GetBoneIndex()), MakeShared<FJsonValueNumber>(Weight.GetWeight())}));
        Vertex->SetArrayField(TEXT("w"), W);
        Vertices.Add(MakeShared<FJsonValueObject>(Vertex));
    }
    for (FTriangleID T : Description->Triangles().GetElementIDs())
    {
        TArray<TSharedPtr<FJsonValue>> V;
        for (FVertexID ID : Description->GetTriangleVertices(T)) V.Add(MakeShared<FJsonValueNumber>(ID.GetValue()));
        Triangles.Add(MakeShared<FJsonValueArray>(V));
    }
    Root->SetArrayField(TEXT("bones"), Bones);
    Root->SetArrayField(TEXT("vertices"), Vertices);
    Root->SetArrayField(TEXT("triangles"), Triangles);
    return Root;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInspectTitleSkirt,
    "TunaSweeper.Migrate.TitleSkirt.Inspect", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FInspectTitleSkirt::RunTest(const FString& Parameters)
{
    auto* Body = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2"));
    auto* Skirt = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/Luna/Skirt/Luna__Skirt_front"));
    if (!Body || !Skirt) return false;
    auto Root = MakeShared<FJsonObject>();
    Root->SetObjectField(TEXT("body"), TitleSkirtMigration::Dump(Body));
    Root->SetObjectField(TEXT("skirt"), TitleSkirtMigration::Dump(Skirt));
    auto* BPClass = LoadClass<ATunaSweeperTitlePresentationActor>(nullptr, TEXT("/Game/UI/Title/BP_TitlePresentationActor.BP_TitlePresentationActor_C"));
    auto* Actor = GEditor->GetEditorWorldContext().World()->SpawnActor<ATunaSweeperTitlePresentationActor>(BPClass);
    for (auto* C : TInlineComponentArray<USkeletalMeshComponent*>(Actor))
    {
        Root->SetStringField(C->GetName() + TEXT("_relative"), C->GetRelativeTransform().ToString());
        Root->SetStringField(C->GetName() + TEXT("_animation"), GetPathNameSafe(C->GetAnimClass()));
    }
    Actor->Destroy();
    FString Output;
    FJsonSerializer::Serialize(Root, TJsonWriterFactory<>::Create(&Output));
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir() / TEXT("TitleSkirt")), true);
    return FFileHelper::SaveStringToFile(Output, *(FPaths::ProjectSavedDir() / TEXT("TitleSkirt/source.json")));
}

namespace TitleSkirtMigration
{
constexpr const TCHAR* Folder = TEXT("/Game/Characters/Player/LunaMk2/Skirt/");
template<class T> T* Asset(T* Source, const TCHAR* Name)
{
    const FString Path = FString(Folder) + Name;
    if (FPackageName::DoesPackageExist(Path)) return LoadObject<T>(nullptr, *Path);
    auto* Package = CreatePackage(*Path);
    T* Result = Source ? DuplicateObject<T>(Source, Package, Name) : NewObject<T>(Package, Name, RF_Public | RF_Standalone);
    Result->SetFlags(RF_Public | RF_Standalone);
    FAssetRegistryModule::AssetCreated(Result);
    return Result;
}
bool Save(UObject* Object)
{
    Object->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Object->GetOutermost(), Object,
        *FPackageName::LongPackageNameToFilename(Object->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
float TorsoWeight(const FVertexBoneWeightsConst& Weights, const FReferenceSkeleton& Ref)
{
    float Result = 0;
    for (const auto& W : Weights)
    {
        FString Name = Ref.GetBoneName(W.GetBoneIndex()).ToString();
        if (Name.StartsWith(TEXT("spine_")) || Name == TEXT("pelvis") || Name == TEXT("cc_base_pelvis")) Result += W.GetWeight();
    }
    return Result;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGenerateTitleSkirt,
    "TunaSweeper.Migrate.TitleSkirt.Generate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGenerateTitleSkirt::RunTest(const FString& Parameters)
{
    using namespace TitleSkirtMigration;
    using namespace UE::AnimationCore;
    auto* Body = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2"));
    auto* Source = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/Luna/Skirt/Luna__Skirt_front"));
    auto* SourceBP = LoadObject<UAnimBlueprint>(nullptr, TEXT("/Game/Characters/Player/Luna/Skirt/Animations/ABP_Luna_Skirt"));
    if (!TestNotNull(TEXT("Body source"), Body) || !TestNotNull(TEXT("Skirt source"), Source) || !TestNotNull(TEXT("Physics graph source"), SourceBP)) return false;
    if (!TestEqual(TEXT("Migrate all source LODs"), Source->GetLODNum(), 1)) return false;
    const auto& BodyRef = Body->GetRefSkeleton();
    const auto& OldRef = Source->GetRefSkeleton();
    const auto BodyPose = ComponentPose(BodyRef);
    const auto OldPose = ComponentPose(OldRef);
    const int32 Pelvis = BodyRef.FindBoneIndex(TEXT("pelvis"));
    if (Pelvis == INDEX_NONE) return false;
    // Bake the inspected BP's alignment at the body's reference pose, never an animated socket transform.
    const FTransform OldOffset(FRotator(-90.0, 34.0, 0.0), FVector(8.866804, 3.277077, 1.104393));
    const FTransform OldToBody = OldOffset * BodyPose[Pelvis];
    FReferenceSkeleton NewRef;
    TArray<int32> Remap; Remap.SetNum(OldRef.GetRawBoneNum()); Remap[0] = Pelvis;
    {
        FReferenceSkeletonModifier Modifier(NewRef, nullptr);
        for (int32 I = 0; I < BodyRef.GetRawBoneNum(); ++I)
            Modifier.Add(BodyRef.GetRawRefBoneInfo()[I], BodyRef.GetRawRefBonePose()[I]);
        for (int32 I = 1; I < OldRef.GetRawBoneNum(); ++I)
        {
            const int32 OldParent = OldRef.GetParentIndex(I);
            const int32 Parent = OldParent == 0 ? Pelvis : Remap[OldParent];
            const FTransform Local = OldParent == 0 ? (OldPose[I] * OldToBody).GetRelativeTransform(BodyPose[Pelvis]) : OldRef.GetRawRefBonePose()[I];
            Remap[I] = NewRef.GetRawBoneNum();
            Modifier.Add(FMeshBoneInfo(OldRef.GetBoneName(I), OldRef.GetBoneName(I).ToString(), Parent), Local);
        }
    }
    FMeshDescription Description = *Source->GetMeshDescription(0);
    FStaticMeshOperations::ApplyTransform(Description, OldToBody, true);
    FSkeletalMeshAttributes Attributes(Description); Attributes.Register(true);
    Attributes.Bones().Reset(NewRef.GetRawBoneNum());
    for (int32 I = 0; I < NewRef.GetRawBoneNum(); ++I)
    {
        FBoneID Bone = Attributes.CreateBone();
        Attributes.GetBoneNames().Set(Bone, NewRef.GetBoneName(I));
        Attributes.GetBoneParentIndices().Set(Bone, NewRef.GetParentIndex(I));
        Attributes.GetBonePoses().Set(Bone, NewRef.GetRawRefBonePose()[I]);
    }
    const auto* BodyDescription = Body->GetMeshDescription(0);
    FSkeletalMeshConstAttributes BodyAttributes(*BodyDescription);
    const auto BodyPositions = BodyAttributes.GetVertexPositions();
    const auto BodyWeights = BodyAttributes.GetVertexSkinWeights();
    auto Positions = Attributes.GetVertexPositions();
    auto Weights = Attributes.GetVertexSkinWeights();
    TArray<FTriangleID> TorsoTriangles;
    for (FTriangleID T : BodyDescription->Triangles().GetElementIDs())
    {
        bool bTorso = true;
        for (FVertexID V : BodyDescription->GetTriangleVertices(T))
            bTorso &= TorsoWeight(BodyWeights.Get(V), BodyRef) > 0.95f && BodyPositions[V].Z > 78.f && BodyPositions[V].Z < 112.f;
        if (bTorso) TorsoTriangles.Add(T);
    }
    if (!TestTrue(TEXT("Body waist surface identified"), TorsoTriangles.Num() > 10)) return false;
    // This source has separate connected islands for the apron panel and its two frills.
    // Keep their layer outside the black skirt instead of projecting both onto one surface.
    TSet<FVertexID> ApronVertices{FVertexID(2), FVertexID(319), FVertexID(504)};
    bool bExpanded = true;
    while (bExpanded)
    {
        bExpanded = false;
        for (FTriangleID T : Description.Triangles().GetElementIDs())
        {
            const auto TV = Description.GetTriangleVertices(T);
            if (!ApronVertices.Contains(TV[0]) && !ApronVertices.Contains(TV[1]) && !ApronVertices.Contains(TV[2])) continue;
            for (FVertexID ID : TV)
                if (!ApronVertices.Contains(ID)) { ApronVertices.Add(ID); bExpanded = true; }
        }
    }
    if (!TestEqual(TEXT("Known source apron topology"), ApronVertices.Num(), 467)) return false;
    int32 Blended = 0, Cleared = 0;
    float MaxCorrection = 0;
    FBox Bounds(ForceInit);
    for (FVertexID V : Description.Vertices().GetElementIDs())
    {
        FVector P(Positions[V]);
        // Above the waist band use body skinning; preserve independent motion towards the hem.
        const float Alpha = FMath::SmoothStep(88.f, 96.f, static_cast<float>(P.Z));
        TMap<int32, float> Combined;
        for (const auto& W : Weights.Get(V)) Combined.FindOrAdd(Remap[W.GetBoneIndex()]) += W.GetWeight() * (1.f - Alpha);
        if (Alpha > 0)
        {
            const FVector OriginalPosition = P;
            const double TargetClearance = 0.66 + (ApronVertices.Contains(V) ? Alpha : 0.0);
            double Best = TNumericLimits<double>::Max();
            FTriangleID Nearest;
            FVector Closest = P;
            double Clearance = 0;
            for (int32 Iteration = 0; Iteration < 32; ++Iteration)
            {
            Best = TNumericLimits<double>::Max();
            for (FTriangleID T : TorsoTriangles)
            {
                const auto Triangle = BodyDescription->GetTriangleVertices(T);
                const FVector Q = FMath::ClosestPointOnTriangleToPoint(P, FVector(BodyPositions[Triangle[0]]), FVector(BodyPositions[Triangle[1]]), FVector(BodyPositions[Triangle[2]]));
                const double D = FVector::DistSquared(P, Q);
                if (D < Best) { Best = D; Nearest = T; Closest = Q; }
            }
            const auto Surface = BodyDescription->GetTriangleVertices(Nearest);
            const FVector A(BodyPositions[Surface[0]]), B(BodyPositions[Surface[1]]), C(BodyPositions[Surface[2]]);
            FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
            const FVector Radial(Closest.X, Closest.Y - BodyPose[Pelvis].GetTranslation().Y, 0);
            if (FVector::DotProduct(Normal, Radial) < 0) Normal *= -1;
            Clearance = FVector::DotProduct(P - Closest, Normal);
            if (OriginalPosition.Z <= 90 || Clearance >= TargetClearance - 0.02) break;
            // Reproject after each move: the nearest surface can change at a seam.
            P += Normal * (TargetClearance - Clearance);
            }
            if (!TestTrue(FString::Printf(TEXT("Final waist clearance vertex %d"), V.GetValue()), OriginalPosition.Z <= 90 || Clearance >= TargetClearance - 0.02)) return false;
            const float Correction = FVector::Distance(P, OriginalPosition);
            if (!TestTrue(TEXT("Clearance adjustment stays within garment scale"), Correction < 8.f)) return false;
            if (Correction > 0.001f) { ++Cleared; MaxCorrection = FMath::Max(MaxCorrection, Correction); }
            const auto Triangle = BodyDescription->GetTriangleVertices(Nearest);
            const FVector A(BodyPositions[Triangle[0]]), B(BodyPositions[Triangle[1]]), C(BodyPositions[Triangle[2]]);
            const FVector Bary = FMath::ComputeBaryCentric2D(Closest, A, B, C);
            for (int32 K = 0; K < 3; ++K)
                for (const auto& W : BodyWeights.Get(Triangle[K])) Combined.FindOrAdd(W.GetBoneIndex()) += W.GetWeight() * FMath::Max(0.0, Bary[K]) * Alpha;
            ++Blended;
        }
        TArray<FBoneWeight> NewWeights;
        for (const auto& Pair : Combined) if (Pair.Value > 0.0001f) NewWeights.Add(FBoneWeight(Pair.Key, Pair.Value));
        Weights.Set(V, FBoneWeights::Create(NewWeights));
        Positions[V] = FVector3f(P); Bounds += P;
    }
    auto* Mesh = Asset(Source, TEXT("SKM_LunaMk2_TitleSkirt"));
    auto* Skeleton = Asset<USkeleton>(nullptr, TEXT("SK_LunaMk2_TitleSkirt"));
    auto* Physics = Asset(Source->GetPhysicsAsset(), TEXT("PA_LunaMk2_TitleSkirt"));
    Mesh->SetSkeleton(Skeleton);
    Mesh->SetRefSkeleton(NewRef);
    Mesh->GetRefBasesInvMatrix().Reset(); Mesh->CalculateInvRefMatrices();
    Mesh->SetPhysicsAsset(Physics);
    Mesh->SetImportedBounds(FBoxSphereBounds(Bounds));
    Mesh->ModifyMeshDescription(0);
    Mesh->CreateMeshDescription(0, MoveTemp(Description)); Mesh->CommitMeshDescription(0);
    if (!TestTrue(TEXT("New skeleton built without modifying body skeleton"), Skeleton->RecreateBoneTree(Mesh))) return false;
    Skeleton->SetPreviewMesh(Mesh); Physics->SetPreviewMesh(Mesh);
    Mesh->PostEditChange();
    FAssetCompilingManager::Get().FinishAllCompilation();

    auto* BP = Asset(SourceBP, TEXT("ABP_LunaMk2_TitleSkirt"));
    BP->TargetSkeleton = Skeleton; BP->SetPreviewMesh(Mesh);
    TArray<UAnimGraphNode_RigidBody*> PhysicsNodes;
    FBlueprintEditorUtils::GetAllNodesOfClass(BP, PhysicsNodes);
    if (!TestEqual(TEXT("One existing physics node"), PhysicsNodes.Num(), 1)) return false;
    PhysicsNodes[0]->Node.OverridePhysicsAsset = Physics;
    TArray<UEdGraph*> Graphs; BP->GetAllGraphs(Graphs);
    for (UEdGraph* Graph : Graphs)
    {
        TArray<UEdGraphNode*> Nodes = Graph->Nodes;
        for (UEdGraphNode* Node : Nodes)
        {
            if (!Node->GetClass()->GetName().Contains(TEXT("RefPose"))) continue;
            TArray<UEdGraphPin*> Destinations;
            for (UEdGraphPin* Pin : Node->Pins) if (Pin->Direction == EGPD_Output) Destinations.Append(Pin->LinkedTo);
            FGraphNodeCreator<UAnimGraphNode_CopyPoseFromMesh> Creator(*Graph);
            auto* Copy = Creator.CreateNode(); Copy->Node.bUseAttachedParent = true;
            Copy->NodePosX = Node->NodePosX; Copy->NodePosY = Node->NodePosY;
            Creator.Finalize();
            FBlueprintEditorUtils::RemoveNode(BP, Node, true);
            for (UEdGraphPin* Pin : Copy->Pins) if (Pin->Direction == EGPD_Output)
                for (UEdGraphPin* Destination : Destinations)
                    if (!TestTrue(TEXT("Copy pose feeds existing physics chain"), Graph->GetSchema()->TryCreateConnection(Pin, Destination))) return false;
        }
    }
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);
    FKismetEditorUtilities::CompileBlueprint(BP);
    if (!TestTrue(TEXT("Pose graph compiles"), BP->Status != BS_Error)) return false;
    for (UObject* Object : {static_cast<UObject*>(Skeleton), static_cast<UObject*>(Physics), static_cast<UObject*>(Mesh), static_cast<UObject*>(BP)})
        if (!TestTrue(TEXT("Saved generated asset"), Save(Object))) return false;

    auto* TitleBP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/UI/Title/BP_TitlePresentationActor"));
    if (!TitleBP) return false;
    auto Configure = [&](USkeletalMeshComponent* C)
    {
        C->Modify(); C->SetSkeletalMeshAsset(Mesh); C->SetAnimInstanceClass(BP->GeneratedClass);
        C->SetAnimationMode(EAnimationMode::AnimationBlueprint);
        if (C->GetAttachParent())
        {
            if (C->IsRegistered()) C->AttachToComponent(C->GetAttachParent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
            else C->SetupAttachment(C->GetAttachParent(), NAME_None);
        }
        C->SetRelativeTransform(FTransform::Identity);
    };
    for (TObjectIterator<USkeletalMeshComponent> It; It; ++It)
        if (It->GetOutermost() == TitleBP->GetOutermost() && It->GetName().StartsWith(TEXT("Skirt"))) Configure(*It);
    FBlueprintEditorUtils::MarkBlueprintAsModified(TitleBP);
    FKismetEditorUtilities::CompileBlueprint(TitleBP);
    if (!TestTrue(TEXT("Title Blueprint compiles"), TitleBP->Status != BS_Error) || !Save(TitleBP)) return false;
    if (!FEditorFileUtils::LoadMap(FPackageName::LongPackageNameToFilename(TEXT("/Game/Maps/IntroMap"), FPackageName::GetMapPackageExtension()), false, true)) return false;
    UWorld* World = GEditor->GetEditorWorldContext().World();
    for (TActorIterator<ATunaSweeperTitlePresentationActor> It(World); It; ++It)
        for (auto* C : TInlineComponentArray<USkeletalMeshComponent*>(*It)) if (C->GetName() == TEXT("Skirt")) Configure(C);
    if (!TestTrue(TEXT("Saved title map instance migration"), FEditorFileUtils::SaveLevel(World->GetCurrentLevel()))) return false;
    auto Report = MakeShared<FJsonObject>();
    Report->SetNumberField(TEXT("body_blended_vertices"), Blended);
    Report->SetNumberField(TEXT("clearance_adjusted_vertices"), Cleared);
    Report->SetNumberField(TEXT("max_clearance_correction_cm"), MaxCorrection);
    Report->SetObjectField(TEXT("result"), Dump(Mesh));
    FString Output; FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Output));
    FFileHelper::SaveStringToFile(Output, *(FPaths::ProjectSavedDir() / TEXT("TitleSkirt/generated.json")));
    return true;
}
#endif
