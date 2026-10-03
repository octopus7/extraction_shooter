// Read-only serialized baseline for raid-level migration and standalone authoring.
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SceneComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/Level.h"
#include "Engine/LevelScriptBlueprint.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "HAL/IConsoleManager.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UObject/UnrealType.h"

namespace RaidLevelAudit
{
static TSharedPtr<FJsonObject> SnapshotObject(const UObject* Object)
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("path"), Object->GetPathName());
    Result->SetStringField(TEXT("class"), Object->GetClass()->GetPathName());
    Result->SetStringField(TEXT("package"), Object->GetOutermost()->GetName());
    TArray<TSharedPtr<FJsonValue>> ClassChain;
    for (const UClass* Class = Object->GetClass(); Class; Class = Class->GetSuperClass())
    {
        ClassChain.Add(MakeShared<FJsonValueString>(Class->GetPathName()));
    }
    Result->SetArrayField(TEXT("class_chain"), ClassChain);
    TSharedPtr<FJsonObject> Properties = MakeShared<FJsonObject>();
    TSharedPtr<FJsonObject> EnumValues = MakeShared<FJsonObject>();
    for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
    {
        const FProperty* Property = *It;
        if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_SkipSerialization))
        {
            continue;
        }
        const void* Value = Property->ContainerPtrToValuePtr<void>(Object);
        FString Text;
        Property->ExportTextItem_Direct(Text, Value, nullptr, const_cast<UObject*>(Object), PPF_None);
        Properties->SetStringField(Property->GetName(), Text);
        if (const FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
        {
            EnumValues->SetNumberField(Property->GetName(), static_cast<double>(EnumProperty->GetUnderlyingProperty()->GetUnsignedIntPropertyValue(Value)));
        }
    }
    Result->SetObjectField(TEXT("properties"), Properties);
    Result->SetObjectField(TEXT("enum_values"), EnumValues);
    return Result;
}

static TSharedPtr<FJsonObject> SnapshotActor(const AActor* Actor)
{
    TSharedPtr<FJsonObject> Result = SnapshotObject(Actor);
    Result->SetStringField(TEXT("name"), Actor->GetName());
    Result->SetStringField(TEXT("label"), Actor->GetActorLabel());
    Result->SetStringField(TEXT("guid"), Actor->GetActorGuid().ToString());
    Result->SetStringField(TEXT("transform"), Actor->GetActorTransform().ToString());
    Result->SetStringField(TEXT("level"), Actor->GetLevel()->GetOutermost()->GetName());
    TArray<TSharedPtr<FJsonValue>> Tags;
    for (const FName Tag : Actor->Tags)
    {
        Tags.Add(MakeShared<FJsonValueString>(Tag.ToString()));
    }
    Result->SetArrayField(TEXT("tags"), Tags);
    Result->SetStringField(TEXT("attachment_parent_actor"), Actor->GetAttachParentActor() ? Actor->GetAttachParentActor()->GetPathName() : TEXT(""));
    Result->SetStringField(TEXT("attachment_socket"), Actor->GetAttachParentSocketName().ToString());
    TArray<TSharedPtr<FJsonValue>> Components;
    TInlineComponentArray<UActorComponent*> ActorComponents;
    Actor->GetComponents(ActorComponents);
    for (const UActorComponent* Component : ActorComponents)
    {
        if (Component && !Component->HasAnyFlags(RF_Transient))
        {
            TSharedPtr<FJsonObject> ComponentJson = SnapshotObject(Component);
            ComponentJson->SetStringField(TEXT("name"), Component->GetName());
            if (const USceneComponent* Scene = Cast<USceneComponent>(Component))
            {
                ComponentJson->SetStringField(TEXT("attach_parent"), Scene->GetAttachParent() ? Scene->GetAttachParent()->GetPathName() : TEXT(""));
                ComponentJson->SetStringField(TEXT("attach_socket"), Scene->GetAttachSocketName().ToString());
                ComponentJson->SetStringField(TEXT("relative_transform"), Scene->GetRelativeTransform().ToString());
            }
            Components.Add(MakeShared<FJsonValueObject>(ComponentJson));
        }
    }
    Result->SetArrayField(TEXT("components"), Components);
    return Result;
}

static TSharedPtr<FJsonObject> SnapshotLevelBlueprint(ULevel* Level)
{
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    ULevelScriptBlueprint* Blueprint = Level->GetLevelScriptBlueprint(true);
    if (!Blueprint)
    {
        Result->SetBoolField(TEXT("exists"), false);
        return Result;
    }
    Result = SnapshotObject(Blueprint);
    Result->SetBoolField(TEXT("exists"), true);
    TArray<TSharedPtr<FJsonValue>> Graphs;
    TArray<UEdGraph*> AllGraphs;
    AllGraphs.Append(Blueprint->UbergraphPages);
    AllGraphs.Append(Blueprint->FunctionGraphs);
    AllGraphs.Append(Blueprint->MacroGraphs);
    for (const UEdGraph* Graph : AllGraphs)
    {
        if (!Graph) continue;
        TSharedPtr<FJsonObject> GraphJson = SnapshotObject(Graph);
        TArray<TSharedPtr<FJsonValue>> Nodes;
        for (const UEdGraphNode* Node : Graph->Nodes)
        {
            if (Node) Nodes.Add(MakeShared<FJsonValueObject>(SnapshotObject(Node)));
        }
        GraphJson->SetArrayField(TEXT("nodes"), Nodes);
        Graphs.Add(MakeShared<FJsonValueObject>(GraphJson));
    }
    Result->SetArrayField(TEXT("graphs"), Graphs);
    return Result;
}

static void Run(const TArray<FString>& Args)
{
    if (Args.Num() != 1 || !GEditor)
    {
        UE_LOG(LogTemp, Error, TEXT("RaidLevelAudit requires exactly one output path and GEditor"));
        return;
    }
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World || !World->PersistentLevel)
    {
        UE_LOG(LogTemp, Error, TEXT("RaidLevelAudit has no loaded editor world"));
        return;
    }
    TSharedPtr<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("map"), World->GetOutermost()->GetName());
    Root->SetStringField(TEXT("world_class"), World->GetClass()->GetPathName());
    Root->SetBoolField(TEXT("uses_external_actors"), World->PersistentLevel->IsUsingExternalActors());
    TArray<TSharedPtr<FJsonValue>> Levels;
    TArray<TSharedPtr<FJsonValue>> Actors;
    for (ULevel* Level : World->GetLevels())
    {
        if (!Level) continue;
        TSharedPtr<FJsonObject> LevelJson = MakeShared<FJsonObject>();
        LevelJson->SetStringField(TEXT("package"), Level->GetOutermost()->GetName());
        LevelJson->SetObjectField(TEXT("blueprint"), SnapshotLevelBlueprint(Level));
        Levels.Add(MakeShared<FJsonValueObject>(LevelJson));
        for (AActor* Actor : Level->Actors)
        {
            if (Actor && !Actor->HasAnyFlags(RF_Transient) && !Actor->GetOutermost()->HasAnyFlags(RF_Transient))
            {
                Actors.Add(MakeShared<FJsonValueObject>(SnapshotActor(Actor)));
            }
        }
    }
    Root->SetArrayField(TEXT("levels"), Levels);
    Root->SetArrayField(TEXT("actors"), Actors);
    FString Text;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Text);
    if (!FJsonSerializer::Serialize(Root.ToSharedRef(), Writer) || !FFileHelper::SaveStringToFile(Text, *Args[0], FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
    {
        UE_LOG(LogTemp, Error, TEXT("RaidLevelAudit failed to write %s"), *Args[0]);
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("RaidLevelAudit wrote %s (%d actors, %d levels)"), *Args[0], Actors.Num(), Levels.Num());
}
static FAutoConsoleCommand Command(TEXT("TunaSweeper.RaidLevelAudit"), TEXT("Write read-only raid map property baseline"), FConsoleCommandWithArgsDelegate::CreateStatic(&Run));
}
