// One-off seam repair: commit with its plan and generated asset, then remove both.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/SkeletalMesh.h"
#include "SkeletalMeshAttributes.h"
#include "BoneWeights.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "AssetCompilingManager.h"
#include "Algo/Reverse.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTitleSkirtSeamGenerator,
    "TunaSweeper.Migrate.TitleSkirtSeam", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTitleSkirtSeamGenerator::RunTest(const FString& Parameters)
{
    using namespace UE::AnimationCore;
    auto* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/Skirt/SKM_LunaMk2_TitleSkirt"));
    if (!Mesh || !Mesh->GetMeshDescription(0)) return false;
    FString Text;
    if (!FFileHelper::LoadFileToString(Text, *(FPaths::ProjectDir() / TEXT("Source/TunaSweeperEditor/Private/TunaSweeperTitleSkirtSeamPlan.json")))) return false;
    TSharedPtr<FJsonObject> Plan;
    if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Plan)) return false;
    const FMeshDescription Source = *Mesh->GetMeshDescription(0);
    if (!TestEqual(TEXT("Expected source topology"), Source.Vertices().Num(), static_cast<int32>(Plan->GetNumberField(TEXT("source_vertex_count")))) ||
        !TestEqual(TEXT("Expected source triangles"), Source.Triangles().Num(), static_cast<int32>(Plan->GetNumberField(TEXT("source_triangle_count")))) ||
        !TestEqual(TEXT("All LODs handled"), Mesh->GetLODNum(), 1) || !TestTrue(TEXT("No morph targets discarded"), Mesh->GetMorphTargets().IsEmpty())) return false;
    FSkeletalMeshConstAttributes Old(Source);
    FMeshDescription Result;
    FSkeletalMeshAttributes New(Result); New.Register();
    auto Join = Result.VertexAttributes().RegisterAttribute<int32>(TEXT("TitleApronJoin"), 1, 0, EMeshAttributeFlags::None);
    auto Part = Result.TriangleAttributes().RegisterAttribute<int32>(TEXT("TitleSkirtPart"), 1, 0, EMeshAttributeFlags::None);
    const auto& Ref = Mesh->GetRefSkeleton();
    for (int32 I = 0; I < Ref.GetRawBoneNum(); ++I)
    {
        const FBoneID Bone = New.CreateBone();
        New.GetBoneNames()[Bone] = Ref.GetBoneName(I);
        New.GetBoneParentIndices()[Bone] = Ref.GetParentIndex(I);
        New.GetBonePoses()[Bone] = Ref.GetRawRefBonePose()[I];
    }
    const int32 UVChannels = Old.GetVertexInstanceUVs().GetNumChannels();
    New.GetVertexInstanceUVs().SetNumChannels(UVChannels);
    for (FPolygonGroupID Group : Source.PolygonGroups().GetElementIDs())
    {
        Result.CreatePolygonGroupWithID(Group);
        New.GetPolygonGroupMaterialSlotNames()[Group] = Old.GetPolygonGroupMaterialSlotNames()[Group];
    }
    for (const auto& Item : Plan->GetArrayField(TEXT("vertices")))
    {
        const auto& V = Item->AsObject(); const FVertexID ID(static_cast<int32>(V->GetNumberField(TEXT("id"))));
        Result.CreateVertexWithID(ID);
        const auto& P = V->GetArrayField(TEXT("p"));
        New.GetVertexPositions()[ID] = FVector3f(P[0]->AsNumber(), P[1]->AsNumber(), P[2]->AsNumber());
        TArray<FBoneWeight> Weights;
        for (const auto& W : V->GetArrayField(TEXT("w")))
            Weights.Add(FBoneWeight(static_cast<int32>(W->AsArray()[0]->AsNumber()), static_cast<float>(W->AsArray()[1]->AsNumber())));
        New.GetVertexSkinWeights().Set(ID, FBoneWeights::Create(Weights));
        Join[ID] = static_cast<int32>(V->GetNumberField(TEXT("join")));
    }
    TMap<int32, int32> Remap;
    for (const auto& Pair : Plan->GetArrayField(TEXT("remap")))
        Remap.Add(static_cast<int32>(Pair->AsArray()[0]->AsNumber()), static_cast<int32>(Pair->AsArray()[1]->AsNumber()));
    auto Mapped = [&](FVertexID V) { const auto* Found = Remap.Find(V.GetValue()); return Found ? FVertexID(*Found) : V; };
    struct FSplit { FVertexID Vertex; float Alpha; };
    auto EdgeKey = [](int32 A, int32 B) { return (static_cast<uint64>(FMath::Min(A, B)) << 32) | static_cast<uint32>(FMath::Max(A, B)); };
    TMap<uint64, TArray<FSplit>> Splits;
    for (const auto& Item : Plan->GetArrayField(TEXT("splits")))
    {
        const auto& Object = Item->AsObject(); const auto& E = Object->GetArrayField(TEXT("edge"));
        auto& List = Splits.FindOrAdd(EdgeKey(E[0]->AsNumber(), E[1]->AsNumber()));
        for (const auto& P : Object->GetArrayField(TEXT("points")))
            List.Add({FVertexID(static_cast<int32>(P->AsArray()[0]->AsNumber())), static_cast<float>(P->AsArray()[1]->AsNumber())});
    }
    struct FCorner { FVertexID Vertex; FVector3f Bary; };
    int32 TriangleIndex = 0;
    for (FTriangleID T : Source.Triangles().GetElementIDs())
    {
        const int32 PartID = Plan->GetArrayField(TEXT("parts"))[TriangleIndex++]->AsNumber();
        const auto TV = Source.GetTriangleVertices(T); const auto VI = Source.GetTriangleVertexInstances(T);
        const FPolygonGroupID Group = Source.GetTrianglePolygonGroup(T);
        TArray<FCorner> Boundary;
        for (int32 K = 0; K < 3; ++K)
        {
            FVector3f Bary(0); Bary[K] = 1; Boundary.Add({Mapped(TV[K]), Bary});
            const int32 Next = (K + 1) % 3;
            if (const auto* List = Splits.Find(EdgeKey(TV[K].GetValue(), TV[Next].GetValue())))
            {
                TArray<FSplit> Ordered = *List;
                if (TV[K].GetValue() > TV[Next].GetValue()) Algo::Reverse(Ordered);
                for (const auto& Split : Ordered)
                {
                    const float Alpha = TV[K].GetValue() < TV[Next].GetValue() ? Split.Alpha : 1.f - Split.Alpha;
                    FVector3f Blend(0); Blend[K] = 1.f - Alpha; Blend[Next] = Alpha;
                    Boundary.Add({Split.Vertex, Blend});
                }
            }
        }
        auto Emit = [&](const FCorner& A, const FCorner& B, const FCorner& C)
        {
            TArray<FVertexInstanceID, TFixedAllocator<3>> Corners;
            for (const FCorner* Corner : {&A, &B, &C})
            {
                const FVertexInstanceID ID = Result.CreateVertexInstance(Corner->Vertex); Corners.Add(ID);
                FVector3f Normal(0), Tangent(0); FVector4f Color(0); float Sign = 0;
                for (int32 K = 0; K < 3; ++K)
                {
                    const float W = Corner->Bary[K];
                    Normal += Old.GetVertexInstanceNormals()[VI[K]] * W;
                    Tangent += Old.GetVertexInstanceTangents()[VI[K]] * W;
                    Color += Old.GetVertexInstanceColors()[VI[K]] * W;
                    Sign += Old.GetVertexInstanceBinormalSigns()[VI[K]] * W;
                }
                New.GetVertexInstanceNormals()[ID] = Normal.GetSafeNormal();
                New.GetVertexInstanceTangents()[ID] = Tangent.GetSafeNormal();
                New.GetVertexInstanceColors()[ID] = Color;
                New.GetVertexInstanceBinormalSigns()[ID] = Sign < 0 ? -1.f : 1.f;
                for (int32 Channel = 0; Channel < UVChannels; ++Channel)
                {
                    FVector2f UV(0);
                    for (int32 K = 0; K < 3; ++K) UV += Old.GetVertexInstanceUVs().Get(VI[K], Channel) * Corner->Bary[K];
                    New.GetVertexInstanceUVs().Set(ID, Channel, UV);
                }
            }
            const FTriangleID Added = Result.CreateTriangle(Group, Corners);
            Part[Added] = PartID;
        };
        if (Boundary.Num() == 3) Emit(Boundary[0], Boundary[1], Boundary[2]);
        else
        {
            const FVertexID Center = Result.CreateVertex(); FVector3f P(0); TMap<int32, float> Blend;
            for (FVertexID V : TV)
            {
                P += New.GetVertexPositions()[Mapped(V)] / 3.f;
                for (const auto& W : New.GetVertexSkinWeights().Get(Mapped(V))) Blend.FindOrAdd(W.GetBoneIndex()) += W.GetWeight() / 3.f;
            }
            New.GetVertexPositions()[Center] = P;
            TArray<FBoneWeight> W; for (const auto& Pair : Blend) W.Add(FBoneWeight(Pair.Key, Pair.Value));
            New.GetVertexSkinWeights().Set(Center, FBoneWeights::Create(W));
            FCorner Middle{Center, FVector3f(1.f / 3.f)};
            for (int32 K = 0; K < Boundary.Num(); ++K) Emit(Middle, Boundary[K], Boundary[(K + 1) % Boundary.Num()]);
        }
    }
    FBox Bounds(ForceInit);
    for (FVertexID V : Result.Vertices().GetElementIDs()) Bounds += FVector(New.GetVertexPositions()[V]);
    Mesh->Modify(); Mesh->SetImportedBounds(FBoxSphereBounds(Bounds));
    Mesh->ModifyMeshDescription(0); Mesh->CreateMeshDescription(0, MoveTemp(Result)); Mesh->CommitMeshDescription(0);
    Mesh->PostEditChange(); FAssetCompilingManager::Get().FinishAllCompilation();
    Mesh->MarkPackageDirty(); FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    return TestTrue(TEXT("Welded title mesh saved"), UPackage::SavePackage(Mesh->GetOutermost(), Mesh,
        *FPackageName::LongPackageNameToFilename(Mesh->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args));
}
#endif
