#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UObject/Class.h"
#include "UObject/UObjectGlobals.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Game/TunaSweeperSafeSave.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Scenario/TunaSweeperEndingActor.h"
#include "Scenario/TunaSweeperEndingPresentation.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperFreePlayEndingContractTest,
    "TunaSweeper.FreePlayEnding.Contract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperFreePlayEndingContractTest::RunTest(const FString& Parameters)
{
    TestNotNull(TEXT("Placeable native ending actor exists"),
        FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperEndingActor")));
    return true;
}

namespace FreePlayEndingTests
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Instance, FWorldContext* Context)
        {
            auto Member = &FContextAccess::WorldContext;
            Instance->*Member = Context;
        }
    };
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperFreePlayEndingPersistenceTest,
    "TunaSweeper.FreePlayEnding.Persistence", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperFreePlayEndingPersistenceTest::RunTest(const FString& Parameters)
{
    if (!FPaths::ProjectSavedDir().Contains(TEXT("RaidStorageAutomation")))
    {
        AddWarning(TEXT("Use isolated -UserDir=<workspace>/Saved/RaidStorageAutomation/ for disk tests."));
        return true;
    }
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    FreePlayEndingTests::FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get(); World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT
    {
        Game->UGameInstance::Shutdown();
        World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        FreePlayEndingTests::FContextAccess::Attach(Game.Get(), nullptr);
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    Game->ResetPlayerSlotArrays();
    Game->ActiveSaveSlotIndex = 3;
    Game->bActiveSaveSlotDifficultySelected = true;
    Game->StorageSlots.SetNum(100);
    Game->RaidStorageSlots.SetNum(10);
    auto* Ending = World->SpawnActor<ATunaSweeperEndingActor>();
    Ending->DispatchBeginPlay();
    TestEqual(TEXT("Normal prompt"), Ending->GetInteractionDisplayName().ToString(),
        Game->ResolveLocalizedText(TEXT("ui.freeplay_ending.normal_prompt"), FText::GetEmpty()).ToString());
    const FGuid Carried = Game->CreateItemInstance(3004, 2);
    Game->PlayerInventorySlots[0].ItemUid = Carried;
    TestFalse(TEXT("Empty storage is normal ending"), Game->IsRaidStorageReadyForTrueEnding());
    TestFalse(TEXT("Cannot save unqualified true ending"), Game->SaveFreePlayEnding(true));
    for (auto& Slot : Game->RaidStorageSlots) Slot.ItemUid = Game->CreateItemInstance(3004, 10);
    Game->BroadcastInventoryStateChanged();
    TestEqual(TEXT("Full storage refreshes live prompt"), Ending->GetInteractionDisplayName().ToString(),
        Game->ResolveLocalizedText(TEXT("ui.freeplay_ending.true_prompt"), FText::GetEmpty()).ToString());
    TestTrue(TEXT("Ten unique full stacks unlock true ending"), Game->IsRaidStorageReadyForTrueEnding());
    auto& First = Game->ItemInstancesByUid[Game->RaidStorageSlots[0].ItemUid];
    First.Quantity = 9;
    Game->BroadcastInventoryStateChanged();
    TestEqual(TEXT("Removing tuna restores normal prompt"), Ending->GetInteractionDisplayName().ToString(),
        Game->ResolveLocalizedText(TEXT("ui.freeplay_ending.normal_prompt"), FText::GetEmpty()).ToString());
    TestFalse(TEXT("Ninety-nine is not enough"), Game->IsRaidStorageReadyForTrueEnding());
    First.Quantity = 10; First.ItemId = 6035;
    TestFalse(TEXT("Gourmet tuna is not normal tuna"), Game->IsRaidStorageReadyForTrueEnding());
    First.ItemId = 3004;
    const FGuid Last = Game->RaidStorageSlots[9].ItemUid;
    Game->RaidStorageSlots[9].ItemUid = Game->RaidStorageSlots[0].ItemUid;
    TestFalse(TEXT("Duplicate item references do not qualify"), Game->IsRaidStorageReadyForTrueEnding());
    Game->RaidStorageSlots[9].ItemUid.Invalidate();
    TestFalse(TEXT("Missing slot item does not qualify"), Game->IsRaidStorageReadyForTrueEnding());
    Game->RaidStorageSlots[9].ItemUid = Last;
    Game->TotalExperiencePoints = 100;
    Game->BeginRaidExperienceSession();
    Game->AddRaidExperience(30);
    Game->RetiredDemoSaveSlotIndex = 3;
    TestFalse(TEXT("Write failure prevents ending completion"), Game->SaveFreePlayEnding(true));
    TestFalse(TEXT("Failure rolls back completion flag"), Game->IsScenarioProgressFlagSet(TEXT("ending.freeplay.true_seen")));
    TestEqual(TEXT("Failure preserves pending experience"), Game->PendingRaidExperiencePoints, int64(30));
    TestEqual(TEXT("Failure rolls back total experience"), Game->TotalExperiencePoints, int64(100));
    Game->RetiredDemoSaveSlotIndex = INDEX_NONE;
    TestTrue(TEXT("Normal ending saves"), Game->SaveFreePlayEnding(false));
    TestTrue(TEXT("True ending saves"), Game->SaveFreePlayEnding(true));
    TestEqual(TEXT("Experience credited only once"), Game->TotalExperiencePoints, int64(130));
    TestEqual(TEXT("Save slot remains selected"), Game->ActiveSaveSlotIndex, 3);
    TestEqual(TEXT("Save is never retired"), Game->RetiredDemoSaveSlotIndex, INDEX_NONE);
    const FString SavePath = Game->GetSaveGameFilePath(Game->GetSaveGameSlotName(3));
    auto* Saved = Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadSaveFileWithRecovery(
        SavePath, {}, [](const USaveGame& Save) { return Save.IsA<UTunaSweeperSaveGame>(); }));
    if (!TestNotNull(TEXT("Ending save exists on disk"), Saved)) return false;
    TestEqual(TEXT("Carried items persist"), Saved->InventorySlots[0].ItemUid, Carried);
    TestEqual(TEXT("All storage items persist"), Saved->RaidStorageSlots[9].ItemUid, Last);
    TestTrue(TEXT("Both endings persisted"), Saved->CompletedScenarioFlags.Contains(TEXT("ending.freeplay.normal_seen")) &&
        Saved->CompletedScenarioFlags.Contains(TEXT("ending.freeplay.true_seen")));
    TestFalse(TEXT("Never writes destructive demo ending flag"), Saved->CompletedScenarioFlags.Contains(TEXT("demo.ending.farewell_seen")));
    Game->ItemInstancesByUid.Reset(); Game->RaidStorageSlots.Reset(); Game->CompletedScenarioFlags.Reset();
    TestTrue(TEXT("Continue reloads completed slot"), Game->LoadGameState());
    TestTrue(TEXT("Tuna not consumed by ending"), Game->IsRaidStorageReadyForTrueEnding());
    TestTrue(TEXT("Ending can be repeated after reload"), Game->SaveFreePlayEnding(true));
    TestEqual(TEXT("Repeated ending gives no extra XP"), Game->TotalExperiencePoints, int64(130));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperFreePlayEndingAssetsTest,
    "TunaSweeper.FreePlayEnding.Assets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperFreePlayEndingAssetsTest::RunTest(const FString& Parameters)
{
    auto* Class = LoadClass<ATunaSweeperEndingActor>(nullptr, TEXT("/Game/Interactables/Ending/BP_EndingInteraction.BP_EndingInteraction_C"));
    if (!TestNotNull(TEXT("Real ending Blueprint exists"), Class)) return false;
    const auto* Actor = Class->GetDefaultObject<ATunaSweeperEndingActor>();
    const auto* MarkerProperty = FindFProperty<FSoftClassProperty>(UTunaSweeperInteractableComponent::StaticClass(), TEXT("MarkerWidgetClass"));
    if (TestNotNull(TEXT("Marker class property"), MarkerProperty))
    {
        const auto* Marker = MarkerProperty->ContainerPtrToValuePtr<TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>>(Actor->GetInteractableComponent());
        TestFalse(TEXT("Ending BP has a visible interaction marker class"), Marker->IsNull());
    }
    TestEqual(TEXT("Blueprint uses ending interaction"), Actor->GetInteractionType(), ETunaSweeperInteractionType::Ending);
    if (!TestNotNull(TEXT("Normal story asset"), Actor->NormalEnding.Get()) ||
        !TestNotNull(TEXT("True story asset"), Actor->TrueEnding.Get())) return false;
    TestNotEqual(TEXT("Separate editable stories"), Actor->NormalEnding.Get(), Actor->TrueEnding.Get());
    TestEqual(TEXT("Normal placeholder has four lines"), Actor->NormalEnding->LineStringKeys.Num(), 4);
    TestEqual(TEXT("True placeholder has four lines"), Actor->TrueEnding->LineStringKeys.Num(), 4);
    TestNotEqual(TEXT("Different titles"), Actor->NormalEnding->TitleStringKey, Actor->TrueEnding->TitleStringKey);
    UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
    ON_SCOPE_EXIT { World->DestroyWorld(false); World->RemoveFromRoot(); };
    auto* Spawned = World->SpawnActor<ATunaSweeperEndingActor>(Class);
    TestNotNull(TEXT("Blueprint can be placed"), Spawned);
    if (Spawned) TestFalse(TEXT("No player cannot start or lock input"), Spawned->StartEnding(nullptr));
    return true;
}
#endif
