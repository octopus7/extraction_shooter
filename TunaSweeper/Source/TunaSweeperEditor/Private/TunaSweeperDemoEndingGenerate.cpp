#include "Misc/AutomationTest.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Camera/CameraComponent.h"
#include "Character/TunaSweeperMoleCompanionActor.h"
#include "GameFramework/PlayerStart.h"
#include "Interaction/TunaSweeperFoodWarehouseActor.h"
#include "Scenario/TunaSweeperDemoEndingActor.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "FileHelpers.h"
#include "Editor.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Misc/PackageName.h"
#include "PhysicsEngine/BodySetup.h"
#include "RawMesh.h"
#include "UObject/SavePackage.h"

namespace
{
bool SaveEndingAsset(UObject* Asset)
{
    FAssetRegistryModule::AssetCreated(Asset);
    Asset->MarkPackageDirty();
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Asset->GetOutermost(),Asset,
        *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),Args);
}
UBlueprint* MakeEndingBP(const TCHAR* Name,UClass* Parent)
{
    return FKismetEditorUtilities::CreateBlueprint(Parent,CreatePackage(*(FString(TEXT("/Game/Interaction/DemoEnding/"))+Name)),
        Name,BPTYPE_Normal,UBlueprint::StaticClass(),UBlueprintGeneratedClass::StaticClass());
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaDemoEndingGenerate,"TunaSweeper.DemoMaintenance.Generate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaDemoEndingGenerate::RunTest(const FString&)
{
    const FString Folder(TEXT("/Game/Interaction/DemoEnding/"));
    if (FPackageName::DoesPackageExist(Folder+TEXT("BP_FoodWarehouse"))) return false;
    auto* Mat = NewObject<UMaterial>(CreatePackage(*(Folder+TEXT("M_FoodWarehouse"))),TEXT("M_FoodWarehouse"),RF_Public|RF_Standalone);
    auto* Color = NewObject<UMaterialExpressionConstant3Vector>(Mat);
    Color->Constant = FLinearColor(.29f,.16f,.065f);
    Mat->GetExpressionCollection().AddExpression(Color);
    Mat->GetEditorOnlyData()->BaseColor.Expression = Color;
    Mat->PostEditChange();
    if (!SaveEndingAsset(Mat)) return false;
    auto* Cube = LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    FRawMesh Unit; Cube->GetSourceModel(0).LoadRawMesh(Unit);
    FRawMesh Raw;
    auto Box = [&](FVector3f Center,FVector3f Size)
    {
        uint32 Base = Raw.VertexPositions.Num();
        for (auto P : Unit.VertexPositions) Raw.VertexPositions.Add(P*Size/100.f+Center);
        for (uint32 I : Unit.WedgeIndices) Raw.WedgeIndices.Add(Base+I);
        Raw.WedgeTexCoords[0].Append(Unit.WedgeTexCoords[0]);
        for (int32 I=0; I<Unit.FaceMaterialIndices.Num(); ++I) { Raw.FaceMaterialIndices.Add(0); Raw.FaceSmoothingMasks.Add(0); }
    };
    // A simple open-front food cupboard: replace its one authored mesh later.
    Box(FVector3f(0,35,90),FVector3f(180,10,180));
    Box(FVector3f(-85,0,90),FVector3f(10,80,180));
    Box(FVector3f(85,0,90),FVector3f(10,80,180));
    for (float Z : {5.f,65.f,125.f,180.f}) Box(FVector3f(0,0,Z),FVector3f(180,80,10));
    Box(FVector3f(0,0,192),FVector3f(200,96,14));
    auto* Mesh = NewObject<UStaticMesh>(CreatePackage(*(Folder+TEXT("SM_FoodWarehouse_Placeholder"))),TEXT("SM_FoodWarehouse_Placeholder"),RF_Public|RF_Standalone);
    auto& Source = Mesh->AddSourceModel();
    Source.BuildSettings.bRecomputeNormals=true; Source.BuildSettings.bRecomputeTangents=true;
    Source.BuildSettings.bGenerateLightmapUVs=false; Source.SaveRawMesh(Raw);
    Mesh->GetStaticMaterials().Add(FStaticMaterial(Mat)); Mesh->Build(false);
    Mesh->CreateBodySetup();
    FKBoxElem Collision; Collision.Center=FVector(0,0,100); Collision.X=200; Collision.Y=96; Collision.Z=200;
    Mesh->GetBodySetup()->AggGeom.BoxElems.Add(Collision); Mesh->GetBodySetup()->CreatePhysicsMeshes();
    Mesh->PostEditChange(); if (!SaveEndingAsset(Mesh)) return false;
    auto* WarehouseBP=MakeEndingBP(TEXT("BP_FoodWarehouse"),ATunaSweeperFoodWarehouseActor::StaticClass());
    auto* Warehouse=CastChecked<ATunaSweeperFoodWarehouseActor>(WarehouseBP->GeneratedClass->GetDefaultObject());
    Warehouse->WarehouseMesh->SetStaticMesh(Mesh);
    FKismetEditorUtilities::CompileBlueprint(WarehouseBP);
    if (!SaveEndingAsset(WarehouseBP)) return false;
    auto* SceneBP=MakeEndingBP(TEXT("BP_DemoDinnerEnding"),ATunaSweeperDemoEndingActor::StaticClass());
    auto* Scene=CastChecked<ATunaSweeperDemoEndingActor>(SceneBP->GeneratedClass->GetDefaultObject());
    Scene->FarewellIllustration=TSoftObjectPtr<UTexture2D>(FSoftObjectPath(TEXT("/Game/UI/DemoEnding/T_DemoFarewell.T_DemoFarewell")));
    auto Line=[&](const TCHAR* Speaker,const TCHAR* Words)
    {
        FTunaSweeperDialogueLine L; L.SpeakerName=FText::FromString(Speaker); L.DialogueText=FText::FromString(Words); Scene->DinnerDialogue.Add(L);
    };
    Line(TEXT("루나"),TEXT("찾았다. 마지막 한 캔."));
    Line(TEXT("두더지"),TEXT("수고했어. 오늘은 여기 앉아서 같이 먹자."));
    Line(TEXT("루나"),TEXT("수리 부품보다 이게 더 귀하게 느껴지네."));
    Line(TEXT("두더지"),TEXT("물도 나오고 배관도 고쳤으니, 이제 마음 놓고 먹어도 돼."));
    Line(TEXT("루나"),TEXT("오늘은 네가 진짜 영웅이다."));
    Line(TEXT("두더지"),TEXT("취수 시설을 수리한 건 너야."));
    Line(TEXT("루나"),TEXT("아니. 참치캔을 챙긴 쪽이 영웅이야."));
    Line(TEXT("두더지"),TEXT("그럼 영웅이랑 반씩 나눠 먹자."));
    FKismetEditorUtilities::CompileBlueprint(SceneBP); if (!SaveEndingAsset(SceneBP)) return false;
    if (!FEditorFileUtils::LoadMap(TEXT("/Game/Maps/BunkerMap"),false,true)) return false;
    UWorld* World=GEditor->GetEditorWorldContext().World();
    AActor* Table=nullptr;
    for (TActorIterator<AActor> It(World); It; ++It)
        if (It->GetActorLabel()==TEXT("SM_KitchenTable")) { Table=*It; break; }
    if (!TestNotNull(TEXT("User's kitchen table"),Table)) return false;
    FVector Origin=Table->GetActorLocation();
    float LunaZ=Origin.Z+88.f, MoleZ=Origin.Z;
    for (TActorIterator<APlayerStart> It(World); It; ++It) { LunaZ=It->GetActorLocation().Z; break; }
    for (TActorIterator<ATunaSweeperMoleCompanionActor> It(World); It; ++It) { MoleZ=It->GetActorLocation().Z; break; }
    auto* Placed=World->SpawnActor<ATunaSweeperDemoEndingActor>(SceneBP->GeneratedClass,Origin,Table->GetActorRotation());
    Placed->SetActorLabel(TEXT("BP_DemoDinnerEnding"));
    FVector LP=Placed->LunaPosition->GetComponentLocation(); LP.Z=LunaZ; Placed->LunaPosition->SetWorldLocation(LP);
    FVector MP=Placed->MolePosition->GetComponentLocation(); MP.Z=MoleZ; Placed->MolePosition->SetWorldLocation(MP);
    if (!FEditorFileUtils::SaveLevel(World->PersistentLevel)) return false;
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaDemoWarehousePlacement,"TunaSweeper.DemoMaintenance.PlaceWarehouse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaDemoWarehousePlacement::RunTest(const FString&)
{
    UClass* BP=LoadClass<ATunaSweeperFoodWarehouseActor>(nullptr,TEXT("/Game/Interaction/DemoEnding/BP_FoodWarehouse.BP_FoodWarehouse_C"));
    if (!BP || !FEditorFileUtils::LoadMap(TEXT("/Game/Maps/DemoBoxRaidMap"),false,true)) return false;
    UWorld* World=GEditor->GetEditorWorldContext().World();
    for (TActorIterator<ATunaSweeperFoodWarehouseActor> It(World); It; ++It) return true;
    FVector Location(400,0,0);
    for (TActorIterator<APlayerStart> It(World); It; ++It)
    {
        Location=It->GetActorLocation()+FVector(400,0,-90);
        break;
    }
    FHitResult Hit;
    if (World->LineTraceSingleByChannel(Hit,Location+FVector(0,0,250),Location-FVector(0,0,2000),ECC_Visibility))
        Location.Z=Hit.ImpactPoint.Z;
    auto* Placed=World->SpawnActor<ATunaSweeperFoodWarehouseActor>(BP,Location,FRotator(0,90,0));
    if (!Placed) return false;
    Placed->SetActorLabel(TEXT("BP_FoodWarehouse"));
    return FEditorFileUtils::SaveLevel(World->PersistentLevel);
}
