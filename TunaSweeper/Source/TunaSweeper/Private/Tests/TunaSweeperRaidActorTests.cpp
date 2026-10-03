#include "Tests/TunaSweeperRaidActorTestActor.h"
#include "Components/SceneComponent.h"
ATunaSweeperRaidActorTestActor::ATunaSweeperRaidActorTestActor() { RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root")); }
void ATunaSweeperRaidActorTestActor::OnConstruction(const FTransform& Transform) { Super::OnConstruction(Transform); ConstructionValue = InstanceValue; if (bRemoveRootDuringConstruction && RootComponent) RootComponent->DestroyComponent(); }
void ATunaSweeperRaidActorTestActor::BeginPlay() { bObservedBindings = Peer != nullptr && GetAttachParentActor() == Peer && InstanceValue == 73; Super::BeginPlay(); }
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Raid/TunaSweeperRaidActorCatalog.h"
#include "Raid/TunaSweeperRaidPlacementAnchor.h"
#include "Subsystem/TunaSweeperRaidActorSubsystem.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperPersistentDoorActor.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"
namespace RaidActorTests
{
constexpr auto Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
struct FFixture
{
    UWorld* World;
    UTunaSweeperRaidActorCatalog* Catalog;
    ATunaSweeperRaidPlacementAnchor* Anchor;
    FFixture()
    {
        // A unique physical map with a real deterministic catalog path; no test-only injection API.
        static int32 Serial = 0;
        const FString MapName = FString::Printf(TEXT("RaidActorTest%d"), ++Serial);
        const auto IVS = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, FName(*MapName), CreatePackage(*(TEXT("/Temp/") + MapName)), true, ERHIFeatureLevel::Num, &IVS);
        GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        UPackage* Package = CreatePackage(*(TEXT("/Game/RaidRuntime/Catalogs/DA_") + MapName + TEXT("_Actors")));
        Catalog = NewObject<UTunaSweeperRaidActorCatalog>(Package, FName(*(TEXT("DA_") + MapName + TEXT("_Actors"))), RF_Public | RF_Standalone);
        Catalog->MapId = World->GetOutermost()->GetFName();
        FTunaSweeperRaidActorProfile& Profile = Catalog->Profiles.AddDefaulted_GetRef();
        Profile.ProfileId = TEXT("First"); Profile.ActorName = TEXT("OriginalSaveName"); Profile.ActorClass = ATunaSweeperRaidActorTestActor::StaticClass();
        Profile.Properties.Add({NAME_None, TEXT("InstanceValue"), TEXT("73")});
        Catalog->Placements.Add({3000, Profile.ProfileId});
        Anchor = AddAnchor(3000);
    }
    ATunaSweeperRaidPlacementAnchor* AddAnchor(int32 Id)
    {
        auto* A = World->SpawnActor<ATunaSweeperRaidPlacementAnchor>();
        FindFProperty<FIntProperty>(A->GetClass(), TEXT("PlacementId"))->SetPropertyValue_InContainer(A, Id);
        auto* Kind = FindFProperty<FEnumProperty>(A->GetClass(), TEXT("AnchorKind"));
        Kind->GetUnderlyingProperty()->SetIntPropertyValue(Kind->ContainerPtrToValuePtr<void>(A), int64(3));
        return A;
    }
    UTunaSweeperRaidActorSubsystem* Subsystem() { return World->GetSubsystem<UTunaSweeperRaidActorSubsystem>(); }
    ATunaSweeperRaidActorTestActor* Actor(FName Name = TEXT("OriginalSaveName")) { return FindObject<ATunaSweeperRaidActorTestActor>(World->PersistentLevel, *Name.ToString()); }
    int32 Count() { int32 N = 0; for (TActorIterator<ATunaSweeperRaidActorTestActor> It(World); It; ++It) ++N; return N; }
    ~FFixture() { if (World->HasBegunPlay()) World->EndPlay(EEndPlayReason::Quit); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); Catalog->ClearFlags(RF_Standalone); }
};
}
#define RAID_TEST(Class, Name) IMPLEMENT_SIMPLE_AUTOMATION_TEST(Class, "TunaSweeper.RaidActors." Name, RaidActorTests::Flags)
RAID_TEST(FRaidInvalid, "InvalidCatalogDoesNotSpawn")
bool FRaidInvalid::RunTest(const FString&) { RaidActorTests::FFixture F; F.Catalog->Placements.Add({3001, TEXT("Missing")}); F.AddAnchor(3001); AddExpectedError(TEXT("Raid actor catalog rejected"), EAutomationExpectedErrorFlags::Contains, 2); TestFalse(TEXT("Invalid rejected"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World)); TestEqual(TEXT("Zero mutation"), F.Count(), 0); TestFalse(TEXT("Invalid retry rejected"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World)); return true; }
RAID_TEST(FRaidOnce, "RepeatedInitializationSpawnsOnce")
bool FRaidOnce::RunTest(const FString&) { RaidActorTests::FFixture F; TestTrue(TEXT("First spawn"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World)); TestTrue(TEXT("Repeated success"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World)); TestEqual(TEXT("Exactly once"), F.Count(), 1); return true; }
RAID_TEST(FRaidNewWorld, "NewWorldSpawnsAgain")
bool FRaidNewWorld::RunTest(const FString&) { RaidActorTests::FFixture A; RaidActorTests::FFixture B; TestTrue(TEXT("World A"), A.Subsystem()->EnsureActorsSpawnedForWorld(A.World)); TestTrue(TEXT("World B"), B.Subsystem()->EnsureActorsSpawnedForWorld(B.World)); TestEqual(TEXT("A count"), A.Count(), 1); TestEqual(TEXT("B count"), B.Count(), 1); return true; }
RAID_TEST(FRaidTransform, "AnchorTransformWins")
bool FRaidTransform::RunTest(const FString&) { RaidActorTests::FFixture F; const FTransform T(FRotator(10,20,30), FVector(101,202,303), FVector(2,3,4)); F.Anchor->SetActorTransform(T); F.Subsystem()->EnsureActorsSpawnedForWorld(F.World); if (TestNotNull(TEXT("Spawned"), F.Actor())) TestTrue(TEXT("All transform channels"), F.Actor()->GetActorTransform().Equals(T)); return true; }
RAID_TEST(FRaidInstance, "InstanceOnlyPropertiesSurvive")
bool FRaidInstance::RunTest(const FString&) { RaidActorTests::FFixture F; F.Catalog->Profiles[0].Properties.Add({TEXT("Root"), TEXT("ComponentTags"), TEXT("(Preserved)")}); F.Subsystem()->EnsureActorsSpawnedForWorld(F.World); if (TestNotNull(TEXT("Spawned"), F.Actor())) { TestEqual(TEXT("EditInstanceOnly"), F.Actor()->InstanceValue, 73); TestEqual(TEXT("Construction consumed instance value"), F.Actor()->ConstructionValue, 73); TestTrue(TEXT("Component override"), F.Actor()->GetRootComponent()->ComponentHasTag(TEXT("Preserved"))); TestEqual(TEXT("Original name-derived save identity"), F.Actor()->GetFName(), FName(TEXT("OriginalSaveName"))); } return true; }
RAID_TEST(FRaidBeginPlay, "ActorReferencesResolveBeforeBeginPlay")
bool FRaidBeginPlay::RunTest(const FString&)
{
    RaidActorTests::FFixture F;
    auto Second = F.Catalog->Profiles[0]; Second.ProfileId = TEXT("Second"); Second.ActorName = TEXT("SecondOriginal"); F.Catalog->Profiles.Add(Second); F.Catalog->Placements.Add({3001, Second.ProfileId}); F.AddAnchor(3001);
    auto& P = F.Catalog->Profiles[0]; P.References.Add({NAME_None, TEXT("Peer"), INDEX_NONE, {3001, NAME_None, NAME_None}}); P.bHasAttachment = true; P.AttachmentParent.PlacementId = 3001;
    UGameInstance* GI = NewObject<UGameInstance>(GEngine);
    F.World->SetGameInstance(GI);
    FWorldContext& Context = GEngine->GetWorldContextFromWorldChecked(F.World); Context.OwningGameInstance = GI;
    FURL URL; URL.AddOption(TEXT("game=/Script/Engine.GameModeBase")); F.World->SetGameMode(URL);
    F.World->InitializeActorsForPlay(URL);
    F.World->BeginPlay(); // Actual world subsystem hook, before GameMode StartPlay.
    if (TestNotNull(TEXT("Hook spawned actor"), F.Actor())) { TestTrue(TEXT("World started actor"), F.Actor()->HasActorBegunPlay()); TestTrue(TEXT("BeginPlay observed final bindings"), F.Actor()->bObservedBindings); }
    return true;
}
RAID_TEST(FRaidRejectMatrix, "InvalidCatalogPreflightMatrix")
bool FRaidRejectMatrix::RunTest(const FString&)
{
    const TArray<TFunction<void(RaidActorTests::FFixture&)>> Cases = {
        [](auto& F) { F.Catalog->MapId = TEXT("/Other/SameAlias"); },
        [](auto& F) { const auto Copy = F.Catalog->Profiles[0]; F.Catalog->Profiles.Add(Copy); },
        [](auto& F) { const auto Copy = F.Catalog->Placements[0]; F.Catalog->Placements.Add(Copy); },
        [](auto& F) { F.AddAnchor(3000); },
        [](auto& F) { F.Catalog->Profiles[0].ActorClass.Reset(); },
        [](auto& F) { F.Catalog->Profiles[0].Properties.Add({NAME_None, TEXT("MissingField"), TEXT("1")}); },
        [](auto& F) { F.Catalog->Profiles[0].Properties.Add({TEXT("Root"), TEXT("RelativeLocation"), TEXT("(X=7,Y=0,Z=0)")}); },
        [](auto& F) { F.Catalog->Profiles[0].References.Add({NAME_None, TEXT("Peer"), INDEX_NONE, {3999, NAME_None, NAME_None}}); },
        [](auto& F) { F.Catalog->Profiles[0].bHasAttachment = true; F.Catalog->Profiles[0].AttachmentParent.PlacementId = 3000; },
        [](auto& F) { F.Catalog->Profiles[0].ActorName = F.Anchor->GetFName(); },
        [](auto& F) { F.Catalog->Placements.Empty(); },
        [](auto& F) { auto* K=FindFProperty<FEnumProperty>(F.Anchor->GetClass(), TEXT("AnchorKind")); K->GetUnderlyingProperty()->SetIntPropertyValue(K->ContainerPtrToValuePtr<void>(F.Anchor), int64(2)); }
    };
    AddExpectedError(TEXT("Raid actor catalog rejected"), EAutomationExpectedErrorFlags::Contains, Cases.Num());
    for (const auto& Mutate : Cases) { RaidActorTests::FFixture F; Mutate(F); TestFalse(TEXT("Malformed data rejected"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World)); TestEqual(TEXT("No authored actors created"), F.Count(), 0); }
    return true;
}
RAID_TEST(FRaidRollback, "ConstructionFailureRollsBackAndRetrySucceeds")
bool FRaidRollback::RunTest(const FString&)
{
    RaidActorTests::FFixture F;
    F.Catalog->Profiles[0].Properties.Add({NAME_None, TEXT("bRemoveRootDuringConstruction"), TEXT("True")});
    F.Catalog->Profiles[0].Properties.Add({TEXT("Root"), TEXT("ComponentTags"), TEXT("(Preserved)")});
    AddExpectedError(TEXT("Raid actor catalog rejected"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Missing final component rolls back"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World));
    TestEqual(TEXT("No residual actors"), F.Count(), 0);
    F.Catalog->Profiles[0].Properties[1].Value = TEXT("False");
    TestTrue(TEXT("Retry uses original name successfully"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World));
    TestEqual(TEXT("One actor after retry"), F.Count(), 1);
    return true;
}
RAID_TEST(FRaidLegacyWorld, "UnconvertedWorldAndMissingCatalog")
bool FRaidLegacyWorld::RunTest(const FString&)
{
    RaidActorTests::FFixture F; F.Catalog->Rename(nullptr, GetTransientPackage());
    AddExpectedError(TEXT("Missing catalog for authored anchors"), EAutomationExpectedErrorFlags::Contains, 1);
    TestFalse(TEXT("Authored anchors require catalog"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World));
    F.Anchor->Destroy();
    TestTrue(TEXT("Unconverted world remains valid"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World));
    TestEqual(TEXT("No extra actors"), F.Count(), 0); return true;
}
RAID_TEST(FRaidSaveIdentity, "OriginalActorNamePreservesDoorSaveKey")
bool FRaidSaveIdentity::RunTest(const FString&)
{
    RaidActorTests::FFixture F; auto& P = F.Catalog->Profiles[0]; P.ActorClass = ATunaSweeperPersistentDoorActor::StaticClass(); P.Properties.Empty();
    auto* GI = NewObject<UTunaSweeperGameInstance>(GEngine); F.World->SetGameInstance(GI);
    TestTrue(TEXT("Native door spawned"), F.Subsystem()->EnsureActorsSpawnedForWorld(F.World));
    auto* Door = FindObject<ATunaSweeperPersistentDoorActor>(F.World->PersistentLevel, TEXT("OriginalSaveName"));
    if (TestNotNull(TEXT("Original name retained"), Door))
    {
        Door->OpenDoor(false);
        FTunaSweeperWorldProgressSaveData State;
        TestTrue(TEXT("Door saves under original fallback object ID"), GI->TryGetWorldProgressState(TEXT("OriginalSaveName"), State));
    }
    return true;
}
#endif