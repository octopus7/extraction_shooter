#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Subsystem/TunaSweeperRailDisplaySubsystem.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Interaction/TunaSweeperRailInteractionComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "LoopRailStation.h"
#include "LoopRailTrack.h"
#include "LoopRailTrain.h"
#include "UObject/StrongObjectPtr.h"

namespace RailTests
{
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance, FWorldContext* Context)
    { auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
};
UTunaSweeperRailInteractionComponent* AddInteraction(ALoopRailStation* Station, bool bTravel)
{
    auto* Component = NewObject<UTunaSweeperRailInteractionComponent>(Station);
    Component->bTravelInteraction = bTravel;
    Station->AddInstanceComponent(Component);
    Component->SetupAttachment(Station->ArrivalMarkerPivot);
    Component->SetRelativeLocation(FVector(0, 0, 110));
    Component->RegisterComponent();
    return Component;
}
ALoopRailStation* AddStation(UWorld* World, ALoopRailTrack* Track, int32 Index)
{
    auto* Station = World->SpawnActor<ALoopRailStation>();
    Station->Track = Track;
    Station->StationId = FName(*FString::Printf(TEXT("test.rail.%d"), Index));
    Station->SetActorTransform(Track->Sample(1000 + Index * Track->GetLength() / 3));
    Station->OnConstruction(Station->GetActorTransform());
    return Station;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRailInteractionTest, "TunaSweeper.Rail.LubricationPersistenceAndQuest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRailInteractionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    RailTests::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get(); World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex; // Never touch user saves, and exercise save refusal.
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        World->DestroyWorld(false); Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr; RailTests::FContextAccess::Attach(Game.Get(), nullptr);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->WorldProgressStatesById.Reset(); Game->ResetPlayerSlotArrays();
    auto* Quest = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
    if (!TestNotNull(TEXT("Quest subsystem"), Quest)) return false;
    Quest->QuestDefinitions.Reset(); Quest->QuestProgressById.Reset(); Quest->bQuestDataLoaded = true;
    auto* Track = World->SpawnActor<ALoopRailTrack>();
    TArray<UTunaSweeperRailInteractionComponent*> Oil, Travel;
    FTunaSweeperQuestDefinition Definition; Definition.QuestId = 900238;
    for (int32 I = 0; I < 3; ++I)
    {
        auto* Station = RailTests::AddStation(World, Track, I);
        Oil.Add(RailTests::AddInteraction(Station, false)); Travel.Add(RailTests::AddInteraction(Station, true));
        FTunaSweeperObjectiveDefinition Objective;
        Objective.ObjectiveId = Station->StationId; Objective.Type = ETunaSweeperObjectiveType::WorldProgressCompleted;
        Objective.WorldProgressId = UTunaSweeperRailInteractionComponent::ProgressId(Station);
        Definition.Objectives.Add(Objective);
    }
    Quest->QuestDefinitions.Add(Definition.QuestId, Definition);
    for (auto* Component : Travel) TestFalse(TEXT("All stations initially locked"), Component->IsTravelUnlocked());
    TestFalse(TEXT("No oil gives no completion"), Oil[0]->Lubricate(false));
    TestTrue(TEXT("Supply oil"), Game->AddItemToFirstAvailableInventorySlot(6014, 3));
    TestFalse(TEXT("Failed save rejects oil use"), Oil[0]->Lubricate(true));
    TestEqual(TEXT("Failed save refunds all oil"), Game->CountInventoryItemById(6014), 3);
    TestFalse(TEXT("Failed save never unlocks travel"), Travel[1]->IsTravelUnlocked());
    TestTrue(TEXT("Oil works before accepting quest"), Oil[0]->Lubricate(false));
    TestEqual(TEXT("Exactly one oil consumed"), Game->CountInventoryItemById(6014), 2);
    TestFalse(TEXT("Same station cannot charge again"), Oil[0]->Lubricate(false));
    for (auto* Component : Travel) TestTrue(TEXT("First oil immediately unlocks every station"), Component->CanOffer());
    TestTrue(TEXT("Later quest acceptance recognizes saved lubrication"), Quest->AcceptQuest(Definition.QuestId));
    TestEqual(TEXT("First station is not full quest completion"), Quest->GetQuestState(Definition.QuestId), ETunaSweeperQuestState::Accepted);
    TArray<FTunaSweeperObjectiveProgressView> Objectives;
    Quest->GetQuestObjectiveProgress(Definition.QuestId, Objectives);
    TestTrue(TEXT("First objective already complete"), Objectives.Num() == 3 && Objectives[0].bCompleted);
    TestTrue(TEXT("Second station remains repairable after unlock"), Oil[1]->Lubricate(false));
    TestTrue(TEXT("Third station completes independently"), Oil[2]->Lubricate(false));
    TestEqual(TEXT("All three world records complete quest"), Quest->GetQuestState(Definition.QuestId), ETunaSweeperQuestState::RewardAvailable);
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Game->WorldProgressStatesById.GenerateValueArray(Save->WorldProgressStates);
    TArray<uint8> SaveBytes;
    TestTrue(TEXT("Station records serialize through the actual save format"), UGameplayStatics::SaveGameToMemory(Save, SaveBytes));
    auto* RestoredSave = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(SaveBytes));
    if (!TestNotNull(TEXT("Serialized world records reload"), RestoredSave)) return false;
    Game->WorldProgressStatesById.Reset();
    TestFalse(TEXT("Fresh world state remains locked even with completed quest"), Travel[2]->IsTravelUnlocked());
    for (const auto& State : RestoredSave->WorldProgressStates) Game->WorldProgressStatesById.Add(State.ObjectId, State);
    auto* Reloaded = RailTests::AddInteraction(CastChecked<ALoopRailStation>(Travel[2]->GetOwner()), true);
    TestTrue(TEXT("Restored world records unlock recreated component"), Reloaded->IsTravelUnlocked());
    for (int32 I = 0; I < 3; ++I)
        TestEqual(TEXT("Only forward next station including wraparound"), Travel[I]->GetNextStation(),
            Cast<ALoopRailStation>(Travel[(I + 1) % 3]->GetOwner()));
    CastChecked<ALoopRailStation>(Travel[1]->GetOwner())->bEnabled = false;
    TestFalse(TEXT("Disabled next station never skips to opposite station"), Travel[0]->CanOffer());
    auto* NewStation = RailTests::AddStation(World, Track, 3);
    auto* RailSubsystem = World->GetSubsystem<UTunaSweeperRailDisplaySubsystem>();
    if (!TestNotNull(TEXT("Project rail subsystem exists"), RailSubsystem)) return false;
    RailSubsystem->OnWorldBeginPlay(*World);
    TArray<UTunaSweeperRailInteractionComponent*> AutoInteractions;
    NewStation->GetComponents(AutoInteractions);
    TestEqual(TEXT("Placed station gets both monument interactions automatically"), AutoInteractions.Num(), 2);
    for (auto* Interaction : AutoInteractions)
        TestEqual(TEXT("Interaction attaches to countdown monument"), Interaction->GetAttachParent(), NewStation->ArrivalMarkerPivot.Get());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperRailTravelTest, "TunaSweeper.Rail.TravelArrivalAndCancellation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperRailTravelTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    Game->bInventoryStateInitialized = true; World->SetGameInstance(Game.Get());
    ON_SCOPE_EXIT { World->DestroyWorld(false); World->SetGameInstance(nullptr); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
    auto* Track = World->SpawnActor<ALoopRailTrack>();
    auto* Source = RailTests::AddStation(World, Track, 0);
    auto* Target = RailTests::AddStation(World, Track, 1);
    RailTests::AddStation(World, Track, 2);
    auto* Travel = RailTests::AddInteraction(Source, true);
    auto* Train = World->SpawnActor<ALoopRailTrain>();
    Train->Track = Track; Train->RebuildTrain(); Train->ResetTrain();
    FTunaSweeperWorldProgressSaveData Record;
    Record.ObjectId = Travel->ProgressId(Source); Record.State = ETunaSweeperWorldProgressState::Completed; Record.ProgressQuantity = 1;
    Game->WorldProgressStatesById.Add(Record.ObjectId, Record);
    auto* Character = World->SpawnActor<ATunaSweeperTopDownCharacter>();
    if (!TestNotNull(TEXT("Player character"), Character)) return false;
    const FVector Start = Source->ArrivalMarkerPivot->GetComponentLocation() - Source->GetActorForwardVector() * 180.f +
        FVector(0, 0, Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3.f);
    Character->SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
    auto* Controller = World->SpawnActor<APlayerController>();
    Controller->Possess(Character);
    ALoopRailStation* ResolvedStation; ALoopRailTrain* ResolvedTrain; FVector Destination;
    TestTrue(TEXT("Safe platform arrival resolves"), Travel->ResolveTravel(Character, ResolvedStation, ResolvedTrain, Destination));
    TestTrue(TEXT("Black-screen transfer arrives"), Travel->ExecuteTravel(Character));
    TestTrue(TEXT("Character appears at next platform"), Character->GetActorLocation().Equals(Destination, 1.f));
    TestTrue(TEXT("Train is already stopped"), Train->IsDwelling());
    TestEqual(TEXT("Train speed is zero"), Train->GetSpeed(), 0.f);
    TestTrue(TEXT("Train head matches target stop"), FMath::Abs(Train->GetHeadDistance() - Target->GetStopDistance()) < 1.f);
    double Eta = -1;
    TestTrue(TEXT("Stopped train ETA is available"), Train->TryGetArrivalSeconds(Target, Eta));
    TestEqual(TEXT("Arrival display shows zero"), Eta, 0.0);
    const double StopDistance = Train->GetHeadDistance();
    Train->Tick(.2f);
    TestEqual(TEXT("Fade-in retains stationary train"), Train->GetHeadDistance(), StopDistance);
    for (int32 I = 0; I < 500; ++I) Train->Tick(.02f);
    TestTrue(TEXT("Normal service resumes after dwell"), Train->GetSpeed() > 0);
    Character->SetActorLocation(Start, false, nullptr, ETeleportType::TeleportPhysics);
    Target->bEnabled = false;
    const double BeforeInvalid = Train->GetHeadDistance();
    TestFalse(TEXT("Invalid destination cannot move player or train"), Travel->ExecuteTravel(Character));
    TestEqual(TEXT("Invalid destination leaves train untouched"), Train->GetHeadDistance(), BeforeInvalid);
    TestTrue(TEXT("Invalid destination leaves player untouched"), Character->GetActorLocation().Equals(Start));
    Target->bEnabled = true;
    Target->Platform->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TestFalse(TEXT("Missing platform floor cancels travel"), Travel->ExecuteTravel(Character));
    Travel->bTravelling = true; Travel->PendingPawn = Character; Travel->PendingController = Controller;
    Controller->SetIgnoreMoveInput(true); Controller->SetIgnoreLookInput(true);
    Travel->FinishTravel();
    TestFalse(TEXT("Cancellation releases movement"), Controller->IsMoveInputIgnored());
    TestFalse(TEXT("Cancellation releases look"), Controller->IsLookInputIgnored());
    TestFalse(TEXT("Cancellation clears active transfer"), Travel->bTravelling);
    return true;
}
#endif
