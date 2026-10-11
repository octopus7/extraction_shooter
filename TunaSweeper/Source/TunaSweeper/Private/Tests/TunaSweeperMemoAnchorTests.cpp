#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Raid/TunaSweeperRaidPlacementAnchor.h"
#include "UObject/UnrealType.h"
#include "Interaction/TunaSweeperMemoAnchor.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperMemoSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMemoAnchorContractTest,
    "TunaSweeper.Memo.IndependentAnchorContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperMemoAnchorContractTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Shared placement kinds no longer include Memo"),
        StaticEnum<ETunaSweeperRaidPlacementAnchorKind>()->GetValueByNameString(TEXT("Memo")), int64(INDEX_NONE));
    UClass* MemoClass = FindObject<UClass>(nullptr, TEXT("/Script/TunaSweeper.TunaSweeperMemoAnchor"));
    if (!TestNotNull(TEXT("Dedicated native memo anchor exists"), MemoClass)) return false;
    TestFalse(TEXT("Memo is independent of raid placement"), MemoClass->IsChildOf(ATunaSweeperRaidPlacementAnchor::StaticClass()));
    TestNotNull(TEXT("Memo identity is authored directly"), FindFProperty<FIntProperty>(MemoClass, TEXT("MemoId")));
    TestNull(TEXT("No second placement identity"), FindFProperty<FProperty>(MemoClass, TEXT("PlacementId")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMemoAssetTest,
    "TunaSweeper.Memo.SavedBlueprint",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperMemoAssetTest::RunTest(const FString& Parameters)
{
    auto* BP = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Interaction/BP_MemoAnchor.BP_MemoAnchor"));
    if (!TestNotNull(TEXT("Dedicated saved Blueprint loads"), BP)) return false;
    TestEqual(TEXT("New native parent"), BP->ParentClass.Get(), ATunaSweeperMemoAnchor::StaticClass());
    auto* Default = BP->GeneratedClass->GetDefaultObject<ATunaSweeperMemoAnchor>();
    auto* Mesh = Default->FindComponentByClass<UStaticMeshComponent>();
    if (!TestNotNull(TEXT("Visible authored mesh"), Mesh)) return false;
    if (!TestNotNull(TEXT("Low-poly mesh loads"), Mesh->GetStaticMesh().Get())) return false;
    TestEqual(TEXT("Low-poly asset used"), Mesh->GetStaticMesh()->GetPathName(),
        FString(TEXT("/Game/Meshes/Props/MemoStorageDevice/SM_MemoStorageDevice_LowPoly.SM_MemoStorageDevice_LowPoly")));
    TestEqual(TEXT("Native proportions preserved"), Mesh->GetRelativeScale3D(), FVector::OneVector);
    TestEqual(TEXT("Mesh materials retained"), Mesh->GetNumMaterials(), 4);
    TestEqual(TEXT("Author must choose an identity"), Default->GetMemoId(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMemoLifecycleTest,
    "TunaSweeper.Memo.CollectionPersistenceAndVisibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperMemoLifecycleTest::RunTest(const FString& Parameters)
{
    struct FContextAccess : UGameInstance
    {
        static void Attach(UGameInstance* Game, FWorldContext* Context)
        { auto Member = &FContextAccess::WorldContext; Game->*Member = Context; }
    };
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    auto* Game = NewObject<UTunaSweeperGameInstance>(GEngine);
    FContextAccess::Attach(Game, &Context);
    Context.OwningGameInstance = Game;
    World->SetGameInstance(Game);
    Game->bInventoryStateInitialized = true;
    // Exercise a real failed save without writing any user save files.
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
    Game->UGameInstance::Init();
    ON_SCOPE_EXIT {
        World->DestroyWorld(false);
        Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr);
        Context.OwningGameInstance = nullptr; FContextAccess::Attach(Game, nullptr);
        GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    {
        const auto OriginalLanguage = Game->GetCurrentTextLanguage();
        ON_SCOPE_EXIT { Game->SetCurrentTextLanguage(OriginalLanguage, false); };
        auto* Memos = Game->GetSubsystem<UTunaSweeperMemoSubsystem>();
        if (!TestNotNull(TEXT("Memo subsystem exists"), Memos)) return false;
        TestNull(TEXT("Definitions no longer expose per-memo title keys"),
            FindFProperty<FProperty>(FTunaSweeperMemoDefinition::StaticStruct(), TEXT("TitleStringKey")));
        Game->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::Korean, false);
        const int32 Ids[] = {1, 10, 60};
        const TCHAR* Titles[] = {TEXT("메모 001"), TEXT("메모 010"), TEXT("메모 060")};
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Ids); ++Index)
        {
            FTunaSweeperMemoDefinition Definition;
            TestTrue(TEXT("Body-only definition loads"), Memos->TryGetMemoDefinition(Ids[Index], Definition));
            TestEqual(TEXT("Shared Korean title has three-digit number"), Definition.Title.ToString(), FString(Titles[Index]));
        }
        Game->SetCurrentTextLanguage(ETunaSweeperItemTextLanguage::English, false);
        FTunaSweeperMemoDefinition Definition;
        Memos->TryGetMemoDefinition(10, Definition);
        TestEqual(TEXT("Title refreshes after language switch"), Definition.Title.ToString(), FString(TEXT("Memo 010")));
        TArray<FTunaSweeperMemoListEntry> Entries;
        Memos->GetMemoListEntries(Entries);
        TestEqual(TEXT("All reserved memos remain listed"), Entries.Num(), 60);
        const auto* Entry = Entries.FindByPredicate([](const auto& Item) { return Item.MemoId == 10; });
        if (TestNotNull(TEXT("Unacquired memo 10 remains listed"), Entry))
        {
            TestFalse(TEXT("Reading title does not acquire memo"), Entry->bAcquired);
            TestEqual(TEXT("List and detail share title"), Entry->Title.ToString(), Definition.Title.ToString());
        }
    }
    auto Spawn = [World](int32 Id)
    {
        auto* Actor = World->SpawnActor<ATunaSweeperMemoAnchor>();
        FindFProperty<FIntProperty>(Actor->GetClass(), TEXT("MemoId"))->SetPropertyValue_InContainer(Actor, Id);
        Actor->DispatchBeginPlay();
        return Actor;
    };
    auto* First = Spawn(1);
    auto* Duplicate = Spawn(1);
    TestTrue(TEXT("Placed valid memo is collectible without JSON placement"), First->CanCollectMemo());
    TestFalse(TEXT("Uncollected memo is visible"), First->IsHidden());
    TestFalse(TEXT("Failed persistence rejects collection"), First->CollectMemo());
    TestFalse(TEXT("Failed save rolls back acquired identity"), Game->IsMemoAcquired(1));
    TestTrue(TEXT("Failed save leaves retry available"), First->CanCollectMemo());
    TestFalse(TEXT("Failed save leaves mesh visible"), First->IsHidden());
    TestTrue(TEXT("Record acquisition in isolated runtime"), Game->MarkMemoAcquired(1, false));
    TestFalse(TEXT("Collected memo cannot be collected twice"), First->CanCollectMemo());
    TestTrue(TEXT("Collection hides actor"), First->IsHidden());
    TestFalse(TEXT("Collection disables collision"), First->GetActorEnableCollision());
    TestTrue(TEXT("Same-id instances hide together"), Duplicate->IsHidden());
    TestTrue(TEXT("Re-entered placement stays hidden"), Spawn(1)->IsHidden());
    TestFalse(TEXT("Repeat collection rejected"), First->CollectMemo());
    TestTrue(TEXT("Missing definition is hidden"), Spawn(999999)->IsHidden());
    TestTrue(TEXT("Unassigned memo is hidden"), Spawn(0)->IsHidden());

    auto* Save = NewObject<UTunaSweeperSaveGame>();
    Game->GetAcquiredMemoIds(Save->AcquiredMemoIds);
    TArray<uint8> Bytes;
    TestTrue(TEXT("Acquired identity serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes));
    auto* Reloaded = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("Acquisition round trip"), Reloaded)) return false;
    TestTrue(TEXT("Acquisition survives save serialization"), Reloaded->AcquiredMemoIds.Contains(1));
    Game->AcquiredMemoIds.Reset();
    Game->OnMemoStateChanged.Broadcast();
    TestFalse(TEXT("Different uncollected slot restores placement"), First->IsHidden());
    for (int32 Id : Reloaded->AcquiredMemoIds) Game->AcquiredMemoIds.Add(Id);
    Game->OnMemoStateChanged.Broadcast();
    TestTrue(TEXT("Restored acquired save hides existing placement"), First->IsHidden());
    return true;
}
#endif
