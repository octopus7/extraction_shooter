#include "Subsystem/TunaSweeperRaidActorSubsystem.h"
#include "Raid/TunaSweeperRaidActorCatalog.h"
#include "Raid/RaidLevelIdentity.h"
#include "Components/SceneComponent.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/PackageName.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperRaidActors, Log, All);
namespace
{
UObject* ComponentOrActor(AActor* Actor, FName Name)
{
    if (!Actor || Name.IsNone()) return Actor;
    for (UActorComponent* Component : Actor->GetComponents())
        if (Component && Component->GetFName() == Name) return Component;
    return nullptr;
}
UObject* ProfileTemplate(UClass* Class, FName ComponentName)
{
    AActor* CDO = Class ? Cast<AActor>(Class->GetDefaultObject()) : nullptr;
    if (UObject* Native = ComponentOrActor(CDO, ComponentName)) return Native;
    for (UClass* Parent = Class; Parent; Parent = Parent->GetSuperClass())
        if (auto* BP = Cast<UBlueprintGeneratedClass>(Parent); BP && BP->SimpleConstructionScript)
            for (USCS_Node* Node : BP->SimpleConstructionScript->GetAllNodes())
                if (Node && Node->GetVariableName() == ComponentName)
                    return Node->GetActualComponentTemplate(Cast<UBlueprintGeneratedClass>(Class));
    return nullptr;
}
// Imports operate only on trusted cooked catalog data. Validate on independent property storage first.
bool HasLevelObject(FProperty* Property, const void* Value)
{
    if (auto* Soft = CastField<FSoftObjectProperty>(Property); Soft && !Soft->GetPropertyValue(Value).IsNull() &&
        (Soft->PropertyClass->IsChildOf(AActor::StaticClass()) || Soft->PropertyClass->IsChildOf(UActorComponent::StaticClass()))) return true;
    if (auto* Interface = CastField<FInterfaceProperty>(Property))
    {
        UObject* Ref = Interface->GetPropertyValue(Value).GetObject();
        return Ref && (Ref->IsA<AActor>() || Ref->IsA<UActorComponent>());
    }
    if (auto* Object = CastField<FObjectPropertyBase>(Property))
    {
        UObject* Ref = Object->GetObjectPropertyValue(Value);
        return Ref && (Ref->IsA<AActor>() || Ref->IsA<UActorComponent>());
    }
    if (auto* Array = CastField<FArrayProperty>(Property))
    {
        FScriptArrayHelper Helper(Array, Value);
        for (int32 I = 0; I < Helper.Num(); ++I) if (HasLevelObject(Array->Inner, Helper.GetRawPtr(I))) return true;
    }
    if (auto* Struct = CastField<FStructProperty>(Property))
        for (TFieldIterator<FProperty> It(Struct->Struct); It; ++It)
            if (HasLevelObject(*It, It->ContainerPtrToValuePtr<void>(Value))) return true;
    // Maps and sets can contain references but have no binding address in this version.
    if (CastField<FMapProperty>(Property) || CastField<FSetProperty>(Property)) return true;
    return false;
}
bool ImportProperty(UObject* Object, const FTunaSweeperRaidActorProperty& Override, bool bApply)
{
    if (!Object) return false;
    FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), Override.PropertyName);
    if (!Property || !Property->HasAnyPropertyFlags(CPF_Edit) || Property->HasAnyPropertyFlags(CPF_Transient | CPF_DisableEditOnInstance | CPF_EditConst) ||
        Property->ArrayDim != 1 || CastField<FDelegateProperty>(Property) || CastField<FMulticastDelegateProperty>(Property)) return false;
    // Anchor transform and attachment are applied separately, never via property text.
    if (Object->IsA<AActor>() && Override.PropertyName == TEXT("RootComponent")) return false;
    if (auto* Scene = Cast<USceneComponent>(Object); Scene && Scene->GetOwner() && Scene->GetOwner()->GetRootComponent() == Scene &&
        (Override.PropertyName == TEXT("RelativeLocation") || Override.PropertyName == TEXT("RelativeRotation") || Override.PropertyName == TEXT("RelativeScale3D"))) return false;
    if (Object->IsA<USceneComponent>() && (Override.PropertyName == TEXT("AttachParent") || Override.PropertyName == TEXT("AttachSocketName"))) return false;
    void* Scratch = FMemory::Malloc(Property->GetSize(), Property->GetMinAlignment());
    Property->InitializeValue(Scratch);
    const TCHAR* End = Property->ImportText_Direct(*Override.Value, Scratch, Object, PPF_None);
    const bool bValid = End && FString(End).TrimStartAndEnd().IsEmpty() && !HasLevelObject(Property, Scratch);
    if (bValid && bApply) Property->CopyCompleteValue(Property->ContainerPtrToValuePtr<void>(Object), Scratch);
    Property->DestroyValue(Scratch); FMemory::Free(Scratch);
    return bValid;
}
FObjectPropertyBase* ReferenceProperty(UObject* Object, const FTunaSweeperRaidActorReference& Binding, void*& Address)
{
    if (!Object) return nullptr;
    FProperty* Property = FindFProperty<FProperty>(Object->GetClass(), Binding.PropertyName);
    if (!Property || !Property->HasAnyPropertyFlags(CPF_Edit) || Property->HasAnyPropertyFlags(CPF_Transient | CPF_EditConst | CPF_DisableEditOnInstance) || Property->ArrayDim != 1) return nullptr;
    Address = Property->ContainerPtrToValuePtr<void>(Object);
    if (Binding.ArrayIndex != INDEX_NONE)
    {
        auto* Array = CastField<FArrayProperty>(Property);
        if (!Array) return nullptr;
        FScriptArrayHelper Helper(Array, Address);
        if (!Helper.IsValidIndex(Binding.ArrayIndex)) return nullptr;
        Address = Helper.GetRawPtr(Binding.ArrayIndex); Property = Array->Inner;
    }
    return CastField<FObjectPropertyBase>(Property);
}
}

bool UTunaSweeperRaidActorSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
    return Type == EWorldType::Game || Type == EWorldType::PIE;
}
FName UTunaSweeperRaidActorSubsystem::GetPhysicalMapId(const UWorld* World)
{
    if (!World) return NAME_None;
    FString Package = World->GetOutermost()->GetName();
    FString Short = FPackageName::GetShortName(Package);
    if (Short.StartsWith(TEXT("UEDPIE_")))
    {
        const int32 Separator = Short.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, 7);
        if (Separator != INDEX_NONE) Short.RightChopInline(Separator + 1);
        Package = FPackageName::GetLongPackagePath(Package) / Short;
    }
    return FName(*Package);
}
FString UTunaSweeperRaidActorSubsystem::GetCatalogObjectPath(FName PhysicalMapId)
{
    const FString AssetName = TEXT("DA_") + FPackageName::GetShortName(PhysicalMapId.ToString()) + TEXT("_Actors");
    return TEXT("/Game/RaidRuntime/Catalogs/") + AssetName + TEXT(".") + AssetName;
}
void UTunaSweeperRaidActorSubsystem::OnWorldBeginPlay(UWorld& World)
{
    Super::OnWorldBeginPlay(World);
    EnsureActorsSpawnedForWorld(&World);
}
bool UTunaSweeperRaidActorSubsystem::EnsureActorsSpawnedForWorld(UWorld* World)
{

    if (!World || !World->IsGameWorld()) return true;
    if (World != GetWorld()) return false;
    if (bSpawned) return true;
    auto Reject = [World](const FString& Reason)
    {
        UE_LOG(LogTunaSweeperRaidActors, Error, TEXT("Raid actor catalog rejected for %s: %s"), *World->GetPathName(), *Reason);
        return false;
    };
    TMap<int32, ATunaSweeperRaidPlacementAnchor*> Anchors;
    TArray<FRaidPlacementDescriptor> Descriptors;
    for (TActorIterator<ATunaSweeperRaidPlacementAnchor> It(World); It; ++It)
    {
        Descriptors.Add({It->GetPlacementId(), It->GetAnchorKind(), It->AllowsDuplicatePlacementId()});
        if (It->GetAnchorKind() == ETunaSweeperRaidPlacementAnchorKind::AuthoredActor) Anchors.Add(It->GetPlacementId(), *It);
    }
    const FName MapId = GetPhysicalMapId(World);
    const FString AssetPath = GetCatalogObjectPath(MapId);
    auto* Catalog = FindObject<UTunaSweeperRaidActorCatalog>(nullptr, *AssetPath);
    if (!Catalog && FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(AssetPath))) Catalog = LoadObject<UTunaSweeperRaidActorCatalog>(nullptr, *AssetPath);
    if (!Catalog) return Anchors.IsEmpty() ? true : Reject(TEXT("Missing catalog for authored anchors"));
    if (Catalog->MapId != MapId) return Reject(TEXT("Physical MapId mismatch"));
    if (World->HasBegunPlay()) return Reject(TEXT("Initialization must precede world BeginPlay"));
    const auto Issues = ValidateRaidPlacementStructure(Descriptors);
    if (!Issues.IsEmpty()) return Reject(Issues[0].StringKey.ToString());
    TMap<FName, const FTunaSweeperRaidActorProfile*> Profiles;
    TMap<FName, UClass*> Classes;
    for (const auto& Profile : Catalog->Profiles)
    {
        if (Profile.ProfileId.IsNone() || Profile.ActorName.IsNone() || Profiles.Contains(Profile.ProfileId)) return Reject(TEXT("Invalid or duplicate profile identity"));
        UClass* Class = Profile.ActorClass.LoadSynchronous();
        if (!Class || !Class->IsChildOf(AActor::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) || Class->IsChildOf(ATunaSweeperRaidPlacementAnchor::StaticClass())) return Reject(TEXT("Invalid actor class"));
        Profiles.Add(Profile.ProfileId, &Profile); Classes.Add(Profile.ProfileId, Class);
        TSet<FString> OverrideKeys;
        for (const auto& Property : Profile.Properties)
        {
            const FString Key = Property.ComponentName.ToString() + TEXT(".") + Property.PropertyName.ToString();
            if (OverrideKeys.Contains(Key) || !ImportProperty(ProfileTemplate(Class, Property.ComponentName), Property, false)) return Reject(TEXT("Invalid property override: ") + Key);
            OverrideKeys.Add(Key);
        }
    }
    TMap<int32, const FTunaSweeperRaidActorProfile*> PlacementProfiles;
    TSet<FName> ActorNames;
    for (const auto& Placement : Catalog->Placements)
    {
        const auto* const* Profile = Profiles.Find(Placement.ProfileId);
        if (Placement.PlacementId < 3000 || PlacementProfiles.Contains(Placement.PlacementId) || !Anchors.Contains(Placement.PlacementId) || !Profile) return Reject(TEXT("Invalid placement, anchor kind, or missing profile"));
        if (ActorNames.Contains((*Profile)->ActorName) || FindObjectFast<UObject>(World->PersistentLevel, (*Profile)->ActorName)) return Reject(TEXT("Original actor name collision"));
        ActorNames.Add((*Profile)->ActorName); PlacementProfiles.Add(Placement.PlacementId, *Profile);
    }
    if (PlacementProfiles.Num() != Anchors.Num()) return Reject(TEXT("Authored anchor has no placement"));
    TMap<FName, AActor*> Environment;
    for (TActorIterator<AActor> It(World); It; ++It)
        if (!It->IsA<ATunaSweeperRaidPlacementAnchor>())
        {
            if (Environment.Contains(It->GetFName())) Environment[It->GetFName()] = nullptr;
            else Environment.Add(It->GetFName(), *It);
        }
    auto TargetTemplate = [&](const FTunaSweeperRaidActorTarget& Target) -> UObject*
    {
        if ((Target.PlacementId > 0) == !Target.EnvironmentActorName.IsNone()) return nullptr;
        if (Target.PlacementId > 0)
        {
            const auto* const* P = PlacementProfiles.Find(Target.PlacementId);
            return P ? ProfileTemplate(Classes[(*P)->ProfileId], Target.ComponentName) : nullptr;
        }
        return ComponentOrActor(Environment.FindRef(Target.EnvironmentActorName), Target.ComponentName);
    };
    for (const auto& Pair : PlacementProfiles)
    {
        const auto& Profile = *Pair.Value;
        TSet<FString> BindingKeys;
        for (const auto& Binding : Profile.References)
        {
            const FString Key = FString::Printf(TEXT("%s.%s[%d]"), *Binding.ComponentName.ToString(), *Binding.PropertyName.ToString(), Binding.ArrayIndex);
            if (BindingKeys.Contains(Key)) return Reject(TEXT("Duplicate reference binding"));
            BindingKeys.Add(Key);
            UObject* Target = TargetTemplate(Binding.Target); void* Address = nullptr;
            auto* Property = ReferenceProperty(ProfileTemplate(Classes[Profile.ProfileId], Binding.ComponentName), Binding, Address);
            if (!Property || !Target || !Target->IsA(Property->PropertyClass)) return Reject(TEXT("Invalid reference binding"));
        }
        if (Profile.bHasAttachment)
        {
            UObject* Target = TargetTemplate(Profile.AttachmentParent);
            USceneComponent* Component = Cast<USceneComponent>(Target);
            if (auto* Actor = Cast<AActor>(Target)) Component = Actor->GetRootComponent();
            if (!Component || Profile.AttachmentParent.PlacementId == Pair.Key || (!Profile.AttachmentSocket.IsNone() && !Component->DoesSocketExist(Profile.AttachmentSocket))) return Reject(TEXT("Invalid attachment target or socket"));
            TSet<int32> Seen; int32 Current = Pair.Key;
            while (const auto* const* P = PlacementProfiles.Find(Current))
            {
                if (Seen.Contains(Current)) return Reject(TEXT("Attachment cycle"));
                Seen.Add(Current); if (!(*P)->bHasAttachment) break; Current = (*P)->AttachmentParent.PlacementId;
            }
        }
    }
    TMap<int32, AActor*> Spawned;
    auto Rollback = [&]()
    {
        for (const auto& Pair : Spawned) if (IsValid(Pair.Value)) { Pair.Value->Destroy(); Pair.Value->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional); }
    };
    auto FailSpawn = [&](const TCHAR* Reason) { Rollback(); return Reject(Reason); };
    // All objects exist before construction; all construction finishes before reference resolution.
    for (const auto& Pair : PlacementProfiles)
    {
        FActorSpawnParameters Params; Params.Name = Pair.Value->ActorName; Params.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;
        Params.OverrideLevel = World->PersistentLevel; Params.bDeferConstruction = true; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        AActor* Actor = World->SpawnActor<AActor>(Classes[Pair.Value->ProfileId], Anchors[Pair.Key]->GetActorTransform(), Params);
        if (!Actor) return FailSpawn(TEXT("Deferred spawn failed"));
        Spawned.Add(Pair.Key, Actor);
        for (const auto& Property : Pair.Value->Properties)
            if (Property.ComponentName.IsNone() && !ImportProperty(Actor, Property, true)) return FailSpawn(TEXT("Actor property import failed"));
    }
    for (const auto& Pair : Spawned) Pair.Value->FinishSpawning(Anchors[Pair.Key]->GetActorTransform());
    for (const auto& Pair : Spawned)
    {
        if (!IsValid(Pair.Value)) return FailSpawn(TEXT("Construction destroyed actor"));
        for (const auto& Property : PlacementProfiles[Pair.Key]->Properties)
            if (!ImportProperty(ComponentOrActor(Pair.Value, Property.ComponentName), Property, true)) return FailSpawn(TEXT("Final component/property resolution failed"));
        for (UActorComponent* Component : Pair.Value->GetComponents()) if (Component && Component->IsRegistered()) Component->ReregisterComponent();
        Pair.Value->SetActorTransform(Anchors[Pair.Key]->GetActorTransform());
    }
    auto TargetObject = [&](const FTunaSweeperRaidActorTarget& Target) -> UObject*
    {
        return ComponentOrActor(Target.PlacementId > 0 ? Spawned.FindRef(Target.PlacementId) : Environment.FindRef(Target.EnvironmentActorName), Target.ComponentName);
    };
    for (const auto& Pair : Spawned)
    {
        const auto& Profile = *PlacementProfiles[Pair.Key];

        for (const auto& Binding : Profile.References)
        {
            void* Address = nullptr; auto* Property = ReferenceProperty(ComponentOrActor(Pair.Value, Binding.ComponentName), Binding, Address); UObject* Target = TargetObject(Binding.Target);
            if (!Property || !Target || !Target->IsA(Property->PropertyClass)) return FailSpawn(TEXT("Final reference resolution failed"));
            Property->SetObjectPropertyValue(Address, Target);
        }
        if (Profile.bHasAttachment)
        {
            UObject* Target = TargetObject(Profile.AttachmentParent); USceneComponent* Component = Cast<USceneComponent>(Target);
            if (auto* Actor = Cast<AActor>(Target)) Component = Actor->GetRootComponent();
            if (!Component || !Pair.Value->AttachToComponent(Component, FAttachmentTransformRules::KeepWorldTransform, Profile.AttachmentSocket)) return FailSpawn(TEXT("Attachment failed"));
        }
    }
    bSpawned = true;
    return true;
}
