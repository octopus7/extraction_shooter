#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Interfaces/IPluginManager.h"
#include "PixelWorldProfile.h"
#include "PixelWorldService.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "LandscapeEdit.h"
#include "LandscapeEditLayer.h"
#include "LandscapeLayerInfoObject.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionLandscapeLayerBlend.h"
#include "Subsystems/EditorActorSubsystem.h"

namespace
{
    int32 CountInstances(UWorld* World)
    {
        int32 Count=0;
        for(TActorIterator<AActor> It(World);It;++It)
        {
            if(!PixelWorld::IsRoot(*It)) continue;
            TArray<UHierarchicalInstancedStaticMeshComponent*> Components; It->GetComponents(Components);
            for(auto* C:Components) Count+=C->GetInstanceCount();
        }
        return Count;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPixelWorldGenerationTest,"PixelWorldBuilder.World.GenerateReplaceUndo",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPixelWorldGenerationTest::RunTest(const FString&)
{
    UWorld* World=FAutomationEditorCommonUtils::CreateNewMap();
    auto* P=NewObject<UPixelWorldProfile>();
    P->ColorPng.FilePath=IPluginManager::Get().FindPlugin(TEXT("PixelWorldBuilder"))->GetBaseDir()/TEXT("Resources/Tests/rgb.png");
    P->MetersPerPixel=10;
    const FColor Colors[]={ FColor(128,64,32),FColor(17,129,253),FColor(1,2,3),FColor(254,127,63) };
    P->Rules.SetNum(4);
    for(int32 I=0;I<4;++I)
    {
        auto& R=P->Rules[I]; R.Red=Colors[I].R; R.Green=Colors[I].G; R.Blue=Colors[I].B;
        auto& V=R.Vegetation.AddDefaulted_GetRef();
        V.Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
        V.DensityPer100SquareMeters=2;
    }
    auto& ActorEntry=P->Rules[0].Vegetation.AddDefaulted_GetRef();
    ActorEntry.ActorClass=AStaticMeshActor::StaticClass(); ActorEntry.DensityPer100SquareMeters=1;
    AActor* Unrelated=World->SpawnActor<AActor>();
    AActor* Root=nullptr; FText Error;
    if(!TestTrue(TEXT("Generate actual Landscape and vegetation"),PixelWorld::Generate(World,*P,nullptr,Root,Error))) return false;
    TestTrue(TEXT("Root is identified"),PixelWorld::IsRoot(Root));
    int32 Instances=0,Landscapes=0,ActorPlants=0;
    for(auto* A:PixelWorld::OwnedActors(Root))
    {
        if(auto* L=Cast<ALandscape>(A))
        {
            ++Landscapes;
            TestEqual(TEXT("Landscape has one component"),L->LandscapeComponents.Num(),1);
            TestEqual(TEXT("Map centered"),L->GetActorLocation(),FVector(-1000,-1000,0));
        }
        if(Cast<AStaticMeshActor>(A)) ++ActorPlants;
        TArray<UHierarchicalInstancedStaticMeshComponent*> Components; A->GetComponents(Components);
        for(auto* C:Components) Instances+=C->GetInstanceCount();
    }
    TestEqual(TEXT("One Landscape"),Landscapes,1);
    TestEqual(TEXT("Mesh scatter stored as instances"),Instances,8);
    TestEqual(TEXT("Actor backend works"),ActorPlants,1);
    P->Rules.RemoveAt(3);
    AActor* FailedRoot=nullptr;
    TestFalse(TEXT("Bad rebuild is rejected"),PixelWorld::Generate(World,*P,Root,FailedRoot,Error));
    TestTrue(TEXT("Failed rebuild preserves previous world"),IsValid(Root) && !Root->IsActorBeingDestroyed());
    P->Rules.AddDefaulted(); P->Rules[3].Red=254; P->Rules[3].Green=127; P->Rules[3].Blue=63;
    AActor* Replacement=nullptr;
    TestTrue(TEXT("Rebuild succeeds"),PixelWorld::Generate(World,*P,Root,Replacement,Error));
    TestTrue(TEXT("Unrelated actor survives"),IsValid(Unrelated) && !Unrelated->IsActorBeingDestroyed());
    int32 Roots=0;
    for(TActorIterator<AActor> It(World);It;++It) Roots+=PixelWorld::IsRoot(*It)?1:0;
    TestEqual(TEXT("Rebuild does not duplicate batches"),Roots,1);
    TestEqual(TEXT("Rebuild updates instances"),CountInstances(World),6);
    TestTrue(TEXT("Undo replacement"),GEditor->UndoTransaction());
    Roots=0; for(TActorIterator<AActor> It(World);It;++It) Roots+=PixelWorld::IsRoot(*It)?1:0;
    TestEqual(TEXT("Undo restores original batch"),Roots,1);
    TestEqual(TEXT("Undo restores original HISM instances"),CountInstances(World),8);
    TestTrue(TEXT("Redo replacement"),GEditor->RedoTransaction());
    TestEqual(TEXT("Redo restores new HISM instances"),CountInstances(World),6);
    TestTrue(TEXT("Unrelated actor survives undo/redo"),IsValid(Unrelated) && !Unrelated->IsActorBeingDestroyed());
    const FString SavedMap=FPaths::ProjectSavedDir()/TEXT("Automation/PixelWorldBuilderRoundTrip.umap");
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(SavedMap),true);
    TestTrue(TEXT("Generated standard actors can be saved"),FEditorFileUtils::SaveMap(World,SavedMap));
    FAutomationEditorCommonUtils::CreateNewMap();
    if(TestTrue(TEXT("Generated map reopens"),FEditorFileUtils::LoadMap(SavedMap,false,false)))
    {
        World=GEditor->GetEditorWorldContext().World(); Roots=0;
        AActor* LoadedRoot=nullptr;
        for(TActorIterator<AActor> It(World);It;++It) if(PixelWorld::IsRoot(*It)) { ++Roots; LoadedRoot=*It; }
        TestEqual(TEXT("Batch identity survives save/load"),Roots,1);
        TestEqual(TEXT("Instances survive save/load"),CountInstances(World),6);
        TestTrue(TEXT("Generated objects retain batch membership"),PixelWorld::OwnedActors(LoadedRoot).Num()>=2);
    }
    FAutomationEditorCommonUtils::CreateNewMap();
    IFileManager::Get().Delete(*SavedMap);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPixelWorldLayersTest,"PixelWorldBuilder.World.LayerWeights",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPixelWorldLayersTest::RunTest(const FString&)
{
    UWorld* World=FAutomationEditorCommonUtils::CreateNewMap();
    auto* P=NewObject<UPixelWorldProfile>();
    P->ColorPng.FilePath=IPluginManager::Get().FindPlugin(TEXT("PixelWorldBuilder"))->GetBaseDir()/TEXT("Resources/Tests/rgb.png");
    const FColor Colors[]={ FColor(128,64,32),FColor(17,129,253),FColor(1,2,3),FColor(254,127,63) };
    P->Rules.SetNum(4);
    auto* Material=NewObject<UMaterial>();
    auto* Blend=NewObject<UMaterialExpressionLandscapeLayerBlend>(Material);
    Blend->Material=Material;
    Material->GetExpressionCollection().AddExpression(Blend);
    Material->GetEditorOnlyData()->BaseColor.Expression=Blend;
    for(int32 I=0;I<4;++I)
    {
        auto& R=P->Rules[I]; R.Red=Colors[I].R; R.Green=Colors[I].G; R.Blue=Colors[I].B;
        R.LandscapeLayer=NewObject<ULandscapeLayerInfoObject>();
        const FName LayerName(*FString::Printf(TEXT("TestLayer%d"),I));
        R.LandscapeLayer->SetLayerName(LayerName,false);
        auto& Input=Blend->Layers.AddDefaulted_GetRef(); Input.LayerName=LayerName;
        Input.BlendType=LB_WeightBlend; Input.ConstLayerInput=FVector(I/4.0,0.5,0.2);
    }
    Material->PostEditChange(); P->LandscapeMaterial=Material;
    AActor* Root=nullptr; FText Error;
    if(!TestTrue(TEXT("Generate weighted landscape"),PixelWorld::Generate(World,*P,nullptr,Root,Error)))
    { AddInfo(Error.ToString()); return false; }
    ALandscape* Landscape=nullptr;
    for(AActor* A:PixelWorld::OwnedActors(Root)) if(auto* L=Cast<ALandscape>(A)) Landscape=L;
    if(!TestNotNull(TEXT("Landscape output"),Landscape)) return false;
    // Import writes the source weights into the default edit layer. Final weights
    // are merged asynchronously on the GPU, which is unavailable under NullRHI.
    const ULandscapeEditLayerBase* BaseLayer=Landscape->GetEditLayerConst(0);
    if(!TestNotNull(TEXT("Imported edit layer"),BaseLayer)) return false;
    FLandscapeEditDataInterface Edit(Landscape->GetLandscapeInfo(),BaseLayer->GetGuid(),false);
    TArray<uint8> Values; Values.SetNumZeroed(64*64);
    Edit.GetWeightDataFast(P->Rules[0].LandscapeLayer,0,0,63,63,Values.GetData(),64);
    TestEqual(TEXT("NW RGB paints NW layer"),int32(Values[63]),255);
    TestEqual(TEXT("SW RGB does not receive NW layer"),int32(Values[0]),0);
    Edit.GetWeightDataFast(P->Rules[3].LandscapeLayer,0,0,63,63,Values.GetData(),64);
    TestEqual(TEXT("SE RGB paints SE layer"),int32(Values[63*64]),255);
    P->Rules[1].LandscapeLayer=P->Rules[0].LandscapeLayer;
    AActor* Bad=nullptr;
    TestFalse(TEXT("Duplicate layer names rejected"),PixelWorld::Generate(World,*P,Root,Bad,Error));
    TestTrue(TEXT("Layer validation leaves old generation intact"),PixelWorld::IsRoot(Root));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPixelWorldDuplicateTest,"PixelWorldBuilder.World.DuplicateBatchSafety",EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPixelWorldDuplicateTest::RunTest(const FString&)
{
    UWorld* World=FAutomationEditorCommonUtils::CreateNewMap();
    auto* P=NewObject<UPixelWorldProfile>();
    P->ColorPng.FilePath=IPluginManager::Get().FindPlugin(TEXT("PixelWorldBuilder"))->GetBaseDir()/TEXT("Resources/Tests/rgb.png");
    const FColor Colors[]={ FColor(128,64,32),FColor(17,129,253),FColor(1,2,3),FColor(254,127,63) };
    P->Rules.SetNum(4);
    for(int32 I=0;I<4;++I) { P->Rules[I].Red=Colors[I].R; P->Rules[I].Green=Colors[I].G; P->Rules[I].Blue=Colors[I].B; }
    FText Error; AActor* Original=nullptr;
    if(!TestTrue(TEXT("Create original batch"),PixelWorld::Generate(World,*P,nullptr,Original,Error))) return false;
    AActor* Duplicate=GEditor->GetEditorSubsystem<UEditorActorSubsystem>()->DuplicateActor(Original,World,FVector(1000,0,0));
    if(!TestTrue(TEXT("UE duplicate retains root identity"),PixelWorld::IsRoot(Duplicate))) return false;
    AActor* Replacement=nullptr;
    TestFalse(TEXT("Ambiguous duplicate batch cannot be regenerated"),PixelWorld::Generate(World,*P,Original,Replacement,Error));
    TestTrue(TEXT("Original remains intact"),PixelWorld::IsRoot(Original));
    TestTrue(TEXT("Duplicated root remains intact"),PixelWorld::IsRoot(Duplicate));
    return true;
}
#endif
