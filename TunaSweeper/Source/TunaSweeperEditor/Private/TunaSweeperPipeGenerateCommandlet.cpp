#include "TunaSweeperPipeGenerateCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/BodySetup.h"
#include "RawMesh.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace
{
const FString Folder = TEXT("/Game/Interaction/BunkerPipe/");
bool Save(UObject* Asset)
{
    FAssetRegistryModule::AssetCreated(Asset);
    Asset->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Asset->GetOutermost(), Asset,
        *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
UMaterial* Material(const TCHAR* Name, FLinearColor Color)
{
    auto* M = NewObject<UMaterial>(CreatePackage(*(Folder + Name)), Name, RF_Public | RF_Standalone);
    auto* C = NewObject<UMaterialExpressionConstant3Vector>(M);
    C->Constant = Color;
    M->GetExpressionCollection().AddExpression(C);
    M->GetEditorOnlyData()->BaseColor.Expression = C;
    M->GetEditorOnlyData()->Roughness.Constant = 0.65f;
    M->GetEditorOnlyData()->Roughness.UseConstant = true;
    M->PostEditChange();
    return Save(M) ? M : nullptr;
}
UStaticMesh* Cylinder(const TCHAR* Name, UMaterial* Mat)
{
    auto* Mesh = NewObject<UStaticMesh>(CreatePackage(*(Folder + Name)), Name, RF_Public | RF_Standalone);
    FRawMesh Raw;
    constexpr int32 Sides = 32;
    for (int32 I = 0; I < Sides; ++I)
    {
        const float Angle = 2.f * PI * I / Sides;
        Raw.VertexPositions.Add(FVector3f(20.f * FMath::Cos(Angle), 20.f * FMath::Sin(Angle), 0.f));
        Raw.VertexPositions.Add(FVector3f(20.f * FMath::Cos(Angle), 20.f * FMath::Sin(Angle), 240.f));
    }
    Raw.VertexPositions.Add(FVector3f(0, 0, 0));
    Raw.VertexPositions.Add(FVector3f(0, 0, 240));
    auto Triangle = [&](int32 A, int32 B, int32 C)
    {
        for (int32 V : { A, B, C })
        {
            Raw.WedgeIndices.Add(V);
            Raw.WedgeTexCoords[0].Add(FVector2f(Raw.VertexPositions[V].X / 40.f + .5f, Raw.VertexPositions[V].Z / 240.f));
        }
        Raw.FaceMaterialIndices.Add(0);
        Raw.FaceSmoothingMasks.Add(0);
    };
    for (int32 I = 0; I < Sides; ++I)
    {
        int32 A = I * 2, B = ((I + 1) % Sides) * 2;
        Triangle(A, B, A + 1); Triangle(B, B + 1, A + 1);
        Triangle(Sides * 2, B, A); Triangle(Sides * 2 + 1, A + 1, B + 1);
    }
    auto& Source = Mesh->AddSourceModel();
    Source.BuildSettings.bRecomputeNormals = true;
    Source.BuildSettings.bRecomputeTangents = true;
    Source.BuildSettings.bGenerateLightmapUVs = false;
    Source.SaveRawMesh(Raw);
    Mesh->GetStaticMaterials().Add(FStaticMaterial(Mat));
    Mesh->Build(false);
    Mesh->CreateBodySetup();
    FKBoxElem Box; Box.Center = FVector(0, 0, 120); Box.X = 40; Box.Y = 40; Box.Z = 240;
    Mesh->GetBodySetup()->AggGeom.BoxElems.Add(Box);
    Mesh->GetBodySetup()->CollisionTraceFlag = CTF_UseSimpleAsComplex;
    Mesh->GetBodySetup()->CreatePhysicsMeshes();
    Mesh->PostEditChange();
    return Save(Mesh) ? Mesh : nullptr;
}
UBlueprint* Blueprint(const TCHAR* Name, UClass* Parent)
{
    return FKismetEditorUtilities::CreateBlueprint(Parent, CreatePackage(*(Folder + Name)), Name,
        BPTYPE_Normal, UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass());
}
}
int32 UTunaSweeperPipeGenerateCommandlet::Main(const FString& Params)
{
    if (FPackageName::DoesPackageExist(Folder + TEXT("BP_BunkerPipe_Broken"))) return 1;
    auto* Red = Material(TEXT("M_BunkerPipe_Broken_Red"), FLinearColor(.65f, .025f, .015f));
    auto* Gray = Material(TEXT("M_BunkerPipe_Repaired_Gray"), FLinearColor(.32f, .32f, .32f));
    if (!Red || !Gray) return 2;
    auto* Broken = Cylinder(TEXT("SM_BunkerPipe_Broken"), Red);
    auto* Repaired = Cylinder(TEXT("SM_BunkerPipe_Repaired"), Gray);
    if (!Broken || !Repaired) return 3;
    auto* DoneBP = Blueprint(TEXT("BP_BunkerPipe_Repaired"), AStaticMeshActor::StaticClass());
    auto* Done = CastChecked<AStaticMeshActor>(DoneBP->GeneratedClass->GetDefaultObject());
    Done->GetStaticMeshComponent()->SetStaticMesh(Repaired);
    Done->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
    Done->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
    FKismetEditorUtilities::CompileBlueprint(DoneBP);
    if (!Save(DoneBP)) return 4;
    auto* BP = Blueprint(TEXT("BP_BunkerPipe_Broken"), ATunaSweeperWorldProgressActor::StaticClass());
    auto* CDO = CastChecked<ATunaSweeperWorldProgressActor>(BP->GeneratedClass->GetDefaultObject());
    CDO->ConfigureWorldProgressDefaults(NAME_None, TEXT("bunker_pipe"), FText::FromString(TEXT("손상된 배관")),
        FText::FromString(TEXT("수리하기")), 6005, 1, 0, FText::FromString(TEXT("방수 테이프")),
        FVector(20, 20, 120), TSoftClassPtr<AActor>(DoneBP->GeneratedClass),
        NAME_None, NAME_None, TEXT("item.waterproof_tape"));
    FindFProperty<FObjectProperty>(CDO->GetClass(), TEXT("ProgressVisualMesh"))->SetObjectPropertyValue_InContainer(CDO, Broken);
    *FindFProperty<FStructProperty>(CDO->GetClass(), TEXT("BlockingBoxOffset"))->ContainerPtrToValuePtr<FVector>(CDO) = FVector(0, 0, 120);
    *FindFProperty<FNameProperty>(CDO->GetClass(), TEXT("ObjectiveEventId"))->ContainerPtrToValuePtr<FName>(CDO) = TEXT("demo.bunker_pipe.repair");
    FKismetEditorUtilities::CompileBlueprint(BP);
    if (!Save(BP)) return 5;
    UE_LOG(LogTemp, Display, TEXT("Bunker pipe: saved two meshes, two materials, two Blueprints."));
    return 0;
}
