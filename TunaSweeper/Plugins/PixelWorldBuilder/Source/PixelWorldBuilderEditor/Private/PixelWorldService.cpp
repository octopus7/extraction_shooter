#include "PixelWorldService.h"
#include "PixelWorldCore.h"
#include "Editor.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeComponent.h"
#include "LandscapeLayerInfoObject.h"
#include "LevelUtils.h"
#include "ScopedTransaction.h"
#include "Misc/ScopedSlowTask.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

namespace
{
    const FName RootTag(TEXT("PixelWorldBuilder.Root"));
    const FString BatchPrefix(TEXT("PixelWorldBuilder.Batch."));

    FName BatchTag(const AActor* Actor)
    {
        if(Actor)
            for(FName Tag:Actor->Tags)
                if(Tag.ToString().StartsWith(BatchPrefix)) return Tag;
        return NAME_None;
    }
    void Identify(AActor* Actor,FName Batch,const TCHAR* LabelKey,const FString& Id)
    {
        Actor->Tags.Add(Batch);
        FFormatNamedArguments Args; Args.Add(TEXT("Id"),FText::FromString(Id));
        Actor->SetActorLabel(FText::Format(PixelWorld::Text(LabelKey),Args).ToString());
    }
    void DestroyBatch(const TArray<AActor*>& Actors,AActor* Root)
    {
        // Children first. Never traverse untagged attachments or unrelated level actors.
        for(AActor* A:Actors)
            if(IsValid(A) && A!=Root && !A->IsActorBeingDestroyed()) A->GetWorld()->EditorDestroyActor(A,true);
        if(IsValid(Root) && !Root->IsActorBeingDestroyed()) Root->GetWorld()->EditorDestroyActor(Root,true);
    }
}

bool PixelWorld::IsRoot(const AActor* Actor)
{
    return IsValid(Actor) && !Actor->IsActorBeingDestroyed() && Actor->ActorHasTag(RootTag) && !BatchTag(Actor).IsNone();
}

TArray<AActor*> PixelWorld::OwnedActors(AActor* Root)
{
    TArray<AActor*> Result;
    if(!IsRoot(Root) || !Root->GetLevel()) return Result;
    const FName Batch=BatchTag(Root);
    for(AActor* A:Root->GetLevel()->Actors)
        if(IsValid(A) && !A->IsActorBeingDestroyed() && A->ActorHasTag(Batch)) Result.Add(A);
    return Result;
}

bool PixelWorld::Generate(UWorld* World,const UPixelWorldProfile& Profile,AActor* ReplaceRoot,AActor*& OutRoot,FText& Error)
{
    OutRoot=nullptr;
    if(!GEditor || GEditor->PlayWorld || !World || World->WorldType!=EWorldType::Editor || World->IsPartitionedWorld() ||
        !World->GetCurrentLevel() || FLevelUtils::IsLevelLocked(World->GetCurrentLevel()) || !FLevelUtils::IsLevelVisible(World->GetCurrentLevel()))
    { Error=Text(TEXT("Error.World")); return false; }
    if(ReplaceRoot && (!IsRoot(ReplaceRoot) || ReplaceRoot->GetWorld()!=World || ReplaceRoot->GetLevel()!=World->GetCurrentLevel()))
    { Error=Text(TEXT("Error.Selection")); return false; }
    if(ReplaceRoot)
    {
        const FName Batch=BatchTag(ReplaceRoot);
        for(AActor* Actor:ReplaceRoot->GetLevel()->Actors)
            if(Actor!=ReplaceRoot && IsRoot(Actor) && Actor->ActorHasTag(Batch))
            { Error=Text(TEXT("Error.DuplicateBatch")); return false; }
    }
    FPixelWorldPlan Plan;
    if(!BuildPlan(Profile,Plan,Error)) return false;
    // Planning/validation does not change the world. The old batch survives every
    // validation failure, cancellation, or replacement-construction failure.
    const auto Previous=OwnedActors(ReplaceRoot);
    FScopedTransaction Transaction(Text(TEXT("Tool.Transaction")));
    ULevel* Level=World->GetCurrentLevel(); Level->Modify();
    FActorSpawnParameters Params;
    Params.OverrideLevel=Level; Params.ObjectFlags=RF_Transactional;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AActor* Root=World->SpawnActor<AActor>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
    if(!Root) { Transaction.Cancel(); Error=Text(TEXT("Error.Spawn")); return false; }
    const FString Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);
    const FName Batch(*(BatchPrefix+Id));
    Root->Tags.Add(RootTag); Identify(Root,Batch,TEXT("Tool.RootLabel"),Id.Left(8));
    auto* Scene=NewObject<USceneComponent>(Root,NAME_None,RF_Transactional);
    Scene->SetMobility(EComponentMobility::Static);
    Root->AddInstanceComponent(Scene); Root->SetRootComponent(Scene); Scene->RegisterComponent();
    TArray<AActor*> Created={Root};
    auto Fail=[&](const TCHAR* Key)
    {
        DestroyBatch(Created,Root);
        Transaction.Cancel();
        Error=Text(Key); return false;
    };
    FScopedSlowTask Progress(float(Plan.Placements.Num()+1),Text(TEXT("Tool.Transaction")));
    Progress.MakeDialog(true);
    if(Progress.ShouldCancel()) return Fail(TEXT("Error.Cancelled"));

    auto* Landscape=World->SpawnActor<ALandscape>(Plan.LandscapeOrigin,FRotator::ZeroRotator,Params);
    if(!Landscape) return Fail(TEXT("Error.Spawn"));
    Created.Add(Landscape);
    Identify(Landscape,Batch,TEXT("Tool.LandscapeLabel"),Id.Left(8));
    Landscape->SetOwner(Root);
    Landscape->SetActorScale3D(Plan.LandscapeScale);
    Landscape->LandscapeMaterial=Profile.LandscapeMaterial;
    TMap<FGuid,TArray<uint16>> Heights;
    Heights.Add(FGuid(),MoveTemp(Plan.Heights));
    TArray<FLandscapeImportLayerInfo> LayerData;
    for(int32 I=0;I<Profile.Rules.Num();++I)
    {
        if(auto* Layer=Profile.Rules[I].LandscapeLayer.Get())
        {
            auto& Data=LayerData.Emplace_GetRef(Layer->GetLayerName());
            Data.LayerInfo=Layer; Data.LayerData=MoveTemp(Plan.LayerWeights[I]);
        }
    }
    TMap<FGuid,TArray<FLandscapeImportLayerInfo>> Layers;
    Layers.Add(FGuid(),MoveTemp(LayerData));
    Landscape->Import(FGuid::NewGuid(),0,0,Plan.VerticesX-1,Plan.VerticesY-1,1,63,Heights,
        *Profile.ColorPng.FilePath,Layers,ELandscapeImportAlphamapType::Additive,TArrayView<const FLandscapeLayer>());
    if(Landscape->LandscapeComponents.IsEmpty()) return Fail(TEXT("Error.Spawn"));
    if(auto* Info=Landscape->GetLandscapeInfo()) Info->UpdateLayerInfoMap(Landscape);
    Landscape->UpdateAllComponentMaterialInstances(true);
    Landscape->AttachToActor(Root,FAttachmentTransformRules::KeepWorldTransform);
    Landscape->MarkPackageDirty();
    Progress.EnterProgressFrame(1);

    TMap<FIntPoint,UHierarchicalInstancedStaticMeshComponent*> MeshGroups;
    for(const auto& Placement:Plan.Placements)
    {
        if(Progress.ShouldCancel()) return Fail(TEXT("Error.Cancelled"));
        const auto& V=Profile.Rules[Placement.RuleIndex].Vegetation[Placement.VegetationIndex];
        if(V.ActorClass)
        {
            AActor* Actor=World->SpawnActor<AActor>(V.ActorClass,Placement.Transform,Params);
            if(!IsValid(Actor) || Actor->IsActorBeingDestroyed()) return Fail(TEXT("Error.Spawn"));
            Created.Add(Actor); Actor->SetOwner(Root);
            Identify(Actor,Batch,TEXT("Tool.VegetationLabel"),Id.Left(8));
            Actor->AttachToActor(Root,FAttachmentTransformRules::KeepWorldTransform);
            Actor->MarkPackageDirty();
        }
        else
        {
            const FIntPoint Key(Placement.RuleIndex,Placement.VegetationIndex);
            auto*& Component=MeshGroups.FindOrAdd(Key);
            if(!Component)
            {
                Component=NewObject<UHierarchicalInstancedStaticMeshComponent>(Root,NAME_None,RF_Transactional);
                Component->SetMobility(EComponentMobility::Static);
                Component->SetStaticMesh(V.Mesh);
                Component->SetCollisionEnabled(V.bCollision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
                Component->SetCollisionProfileName(V.bCollision?FName(TEXT("BlockAll")):FName(TEXT("NoCollision")));
                Component->bAutoRebuildTreeOnInstanceChanges=false;
                Root->AddInstanceComponent(Component);
                Component->SetupAttachment(Scene);
                Component->RegisterComponent();
            }
            Component->AddInstance(Placement.Transform,true);
        }
        Progress.EnterProgressFrame(1);
    }
    for(const auto& Pair:MeshGroups)
    {
        Pair.Value->bAutoRebuildTreeOnInstanceChanges=true;
        Pair.Value->BuildTreeIfOutdated(false,true);
    }
    if(Progress.ShouldCancel()) return Fail(TEXT("Error.Cancelled"));
    // Commit replacement only once every new output exists.
    if(ReplaceRoot) DestroyBatch(Previous,ReplaceRoot);
    Root->MarkPackageDirty(); Level->MarkPackageDirty();
    OutRoot=Root;
    return true;
}
