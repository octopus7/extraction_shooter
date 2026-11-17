#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "Weapon/TunaSweeperTopAttackRocket.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaRocketItemUseTest, "TunaSweeper.Rocket.ItemUse",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaRocketItemUseTest::RunTest(const FString& Parameters)
{
	const FString SaveRoot = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense/Rocket") / FGuid::NewGuid().ToString();
	if (!TestTrue(TEXT("Isolated save root"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(SaveRoot))) return false;
	ON_SCOPE_EXIT { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); IFileManager::Get().DeleteDirectory(*SaveRoot, false, true); };
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{ auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
	};
	const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	World->SetGameInstance(Game.Get()); Context.OwningGameInstance = Game.Get();
	Game->bInventoryStateInitialized = true;
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT {
		Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game.Get(), nullptr); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
	};
	Game->ResetPlayerSlotArrays(); Game->ActiveSaveSlotIndex = 1;
	Game->bActiveSaveSlotDifficultySelected = true;
	Game->bSuppressSaveWritesForTests = true;
	Game->RocketProjectileClass = ATunaSweeperTopAttackRocket::StaticClass();
	FTunaSweeperItemInstance Item; Item.Uid = FGuid::NewGuid(); Item.ItemId = 6019; Item.Quantity = 1;
	Game->ItemInstancesByUid.Add(Item.Uid, Item); Game->PlayerInventorySlots[0].ItemUid = Item.Uid;
	FTunaSweeperItemSlotReference Slot; Slot.Source = ETunaSweeperItemSlotSource::Inventory; Slot.SlotIndex = 0;
	auto* Character = World->SpawnActor<ATunaSweeperTopDownCharacter>();
	if (!TestNotNull(TEXT("Character"), Character)) return false;
	TestTrue(TEXT("Legacy 6019 item is usable"), Game->CanUseItemInSlot(Slot, Character));
	TestTrue(TEXT("Use enters aim"), Character->StartItemUseFromSlot(Slot));
	TestTrue(TEXT("Aim active"), Character->IsRocketAiming());
	TestTrue(TEXT("Aiming retains item"), Game->ItemInstancesByUid.Contains(Item.Uid));
	Character->CancelActiveCancelableAction();
	TestFalse(TEXT("Cancel leaves aim"), Character->IsRocketAiming());
	TestTrue(TEXT("Cancel retains item"), Game->ItemInstancesByUid.Contains(Item.Uid));
	FTunaSweeperItemSlotReference QuickSlot; QuickSlot.Source = ETunaSweeperItemSlotSource::UsableQuickSlot; QuickSlot.SlotIndex = 0;
	TestTrue(TEXT("Rocket can occupy usable quick slot"), Game->CanSlotAcceptItem(QuickSlot, Item.Uid));
	Game->PlayerInventorySlots[0].Clear(); Game->UsableQuickSlots[0].ItemUid = Item.Uid;
	TestTrue(TEXT("Quick slot use enters aim"), Character->StartItemUseFromSlot(QuickSlot));
	Character->CancelActiveCancelableAction();
	TestTrue(TEXT("Quick slot cancel retains item"), Game->ItemInstancesByUid.Contains(Item.Uid));
	Game->UsableQuickSlots[0].Clear(); Game->PlayerInventorySlots[0].ItemUid = Item.Uid;
	TestFalse(TEXT("Changed UID cannot fire"), Game->TryLaunchRocketFromSlot(Slot, FGuid::NewGuid(), Character, nullptr, FVector(1000, 0, 0)));
	TestTrue(TEXT("Rejected shot retains item"), Game->ItemInstancesByUid.Contains(Item.Uid));
	Game->RocketProjectileClass.Reset();
	TestFalse(TEXT("Unavailable projectile cannot fire"), Game->TryLaunchRocketFromSlot(Slot, Item.Uid, Character, nullptr, FVector(1000, 0, 0)));
	TestTrue(TEXT("Unavailable projectile retains item"), Game->ItemInstancesByUid.Contains(Item.Uid));
	Game->RocketProjectileClass = ATunaSweeperTopAttackRocket::StaticClass();
	TestTrue(TEXT("Valid launch succeeds"), Game->TryLaunchRocketFromSlot(Slot, Item.Uid, Character, nullptr, FVector(1000, 0, 0)));
	TestFalse(TEXT("Successful launch consumes item"), Game->ItemInstancesByUid.Contains(Item.Uid));
	TestTrue(TEXT("Successful launch clears slot"), Game->PlayerInventorySlots[0].IsEmpty());
	TestFalse(TEXT("Second launch cannot duplicate"), Game->TryLaunchRocketFromSlot(Slot, Item.Uid, Character, nullptr, FVector(1000, 0, 0)));
	TestTrue(TEXT("Successful rocket launch awards Magic Wand"), Game->GetSubsystem<UTunaSweeperAchievementSubsystem>()->IsAchievementUnlocked(TEXT("20")));
	return true;
}
#endif
