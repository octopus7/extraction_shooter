#if WITH_DEV_AUTOMATION_TESTS

#include "Engine/Engine.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Character/TunaSweeperMoleCompanionActor.h"
#include "Character/TunaSweeperQuestPresenceNpcActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/StrongObjectPtr.h"
#include "UI/TunaSweeperQuestWidget.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Blueprint/WidgetTree.h"
#include "UI/ItemContainerWidget.h"

namespace TunaSweeperQuestData
{
	bool ParseObjective(const TSharedPtr<FJsonObject>& JsonObject, FTunaSweeperObjectiveDefinition& OutObjective);
}

namespace TunaSweeperUndergroundStorageTests
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperUndergroundStorageTest,
	"TunaSweeper.Inventory.UndergroundStorage",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperUndergroundStorageTest::RunTest(const FString& Parameters)
{
	using namespace TunaSweeperUndergroundStorageTests;
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	// Initialize subsystems without reading/writing the user's save slots.
	Game->bInventoryStateInitialized = true;
	Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex;
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT
	{
		Game->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	Game->ResetPlayerSlotArrays();
	auto* Quests = Game->GetSubsystem<UTunaSweeperQuestSubsystem>();
	if (!TestNotNull(TEXT("Quest subsystem"), Quests)) return false;

    Game->StorageSlots.SetNum(100);
    Quests->QuestDefinitions.Reset();
    Quests->QuestProgressById.Reset();
    FTunaSweeperQuestDefinition Definition;
    Definition.QuestId = 14;
    Definition.bCompleteOnAccept = true;
    Quests->QuestDefinitions.Add(14, Definition);
    TestEqual(TEXT("Physical storage remains 100"), Game->GetStorageSlotCapacity(), 100);
    TestEqual(TEXT("Accessible capacity follows edition"), Game->GetAccessibleStorageSlotCount(), TunaSweeperBuildFlavor::IsDemo() ? 100 : 20);
    if (TunaSweeperBuildFlavor::IsDemo()) return true;
    auto* Panel = CreateWidget<UStorageContainerWidget>(Game.Get());
    if (!TestNotNull(TEXT("Storage widget fixture"), Panel)) return false;
    auto* Body = Panel->WidgetTree->ConstructWidget<UVerticalBox>();
    Panel->WidgetTree->RootWidget = Body;
    Panel->ContainerTileView = Panel->WidgetTree->ConstructWidget<UTileView>();
    Body->AddChild(Panel->ContainerTileView);
    Panel->SetStorageView();
    Panel->UItemContainerWidget::NativeConstruct();
    ON_SCOPE_EXIT { Panel->UItemContainerWidget::NativeDestruct(); };
    TestEqual(TEXT("Locked UI exposes only 20 tiles"), Panel->ContainerTileView->GetListItems().Num(), 20);
    TestNotNull(TEXT("Fault panel exists"), Panel->StorageFaultPanel.Get());
    if (Panel->StorageFaultPanel) TestEqual(TEXT("Fault panel visible"), Panel->StorageFaultPanel->GetVisibility(), ESlateVisibility::HitTestInvisible);
    FTunaSweeperItemInstance Item;
    Item.Uid = FGuid::NewGuid(); Item.ItemId = 3004; Item.Quantity = 2;
    Game->ItemInstancesByUid.Add(Item.Uid, Item);
    Game->StorageSlots[25].ItemUid = Item.Uid;
    FTunaSweeperItemSlotReference Locked, Surface;
    Locked.Source = Surface.Source = ETunaSweeperItemSlotSource::Storage;
    Locked.SlotIndex = 25; Surface.SlotIndex = 0;
    TestFalse(TEXT("Cannot move a locked saved item"), Game->CanMoveItemBetweenSlots(Locked, Surface));
    TestFalse(TEXT("Cannot deposit into a locked slot"), Game->CanSlotAcceptItem(Locked, Item.Uid));
    TestEqual(TEXT("Locked item excluded from quest count"), Game->CountQuestSubmissionItemById(3004), 0);
    TestEqual(TEXT("Locked item excluded from crafting count"), Game->CountWorkbenchIngredientItemById(3004), 0);
    FTunaSweeperItemStack Requirement; Requirement.ItemId = 3004; Requirement.Quantity = 1;
    TestFalse(TEXT("Quest consumption cannot take locked items"), Game->TryConsumeQuestItems({Requirement}, [] {}, false));
    FTunaSweeperItemInstance Removed;
    TestFalse(TEXT("Locked item cannot be removed"), Game->RemoveItemFromSlot(Locked, Removed));
    Game->CompactStorageSlots();
    TestEqual(TEXT("Compaction preserves locked position"), Game->StorageSlots[25].ItemUid, Item.Uid);
    TestTrue(TEXT("Compaction leaves surface empty"), Game->StorageSlots[0].IsEmpty());
    FTunaSweeperNumericQuestProgressSaveData Progress;
    Progress.QuestId = 14; Progress.State = ETunaSweeperQuestState::RewardAvailable;
    Quests->QuestProgressById.Add(14, Progress);
    TestFalse(TEXT("Unclaimed reward does not unlock underground"), Game->IsUndergroundStorageUnlocked());
    {
        // Exercise the claim while keeping the account-global achievement publisher inactive.
        TGuardValue<TEnumAsByte<EWorldType::Type>> ReportingGuard(World->WorldType, EWorldType::Editor);
        TestTrue(TEXT("Reward claim succeeds"), Quests->ClaimQuestReward(14));
    }
    TestEqual(TEXT("Claim immediately expands open UI"), Panel->ContainerTileView->GetListItems().Num(), 100);
    if (Panel->StorageFaultPanel) TestEqual(TEXT("Claim hides fault panel"), Panel->StorageFaultPanel->GetVisibility(), ESlateVisibility::Collapsed);
    TestTrue(TEXT("Claim immediately unlocks underground"), Game->IsUndergroundStorageUnlocked());
    TestEqual(TEXT("Restored capacity"), Game->GetAccessibleStorageSlotCount(), 100);
    TestEqual(TEXT("Saved underground items become usable"), Game->CountQuestSubmissionItemById(3004), 2);
    TestTrue(TEXT("Saved item can move after restoration"), Game->CanMoveItemBetweenSlots(Locked, Surface));
    TArray<FTunaSweeperNumericQuestProgressSaveData> SavedProgress;
    int32 Tracked = 0, Coins = 0;
    Quests->ExportQuestProgressForSave(SavedProgress, Tracked, Coins);
    Quests->LoadQuestProgressFromSave({}, 0, 0);
    TestEqual(TEXT("Uncompleted save relocks capacity"), Game->GetAccessibleStorageSlotCount(), 20);
    TestEqual(TEXT("Relock preserves saved item"), Game->StorageSlots[25].ItemUid, Item.Uid);
    Quests->LoadQuestProgressFromSave(SavedProgress, Tracked, Coins);
    TestEqual(TEXT("Completed save restores capacity"), Game->GetAccessibleStorageSlotCount(), 100);
    return true;
}
#endif
