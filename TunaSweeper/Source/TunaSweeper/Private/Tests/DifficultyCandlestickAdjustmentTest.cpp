#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/Button.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UI/DifficultySelectionWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDifficultyCandlestickAdjustmentTest, "TunaSweeper.Difficulty.CandlestickAdjustment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDifficultyCandlestickAdjustmentTest::RunTest(const FString& Parameters)
{
	const FString SaveRoot = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense/Difficulty") / FGuid::NewGuid().ToString();
	if (!TestTrue(TEXT("Isolated save directory"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(SaveRoot))) return false;
	ON_SCOPE_EXIT {
		TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory({});
		IFileManager::Get().DeleteDirectory(*SaveRoot, false, true);
	};
	struct FContextAccess : UGameInstance
	{
		static void Attach(UGameInstance* Instance, FWorldContext* Context)
		{ auto Member = &FContextAccess::WorldContext; Instance->*Member = Context; }
	};
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
	Context.SetCurrentWorld(World);
	TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
	FContextAccess::Attach(Game.Get(), &Context);
	Context.OwningGameInstance = Game.Get();
	World->SetGameInstance(Game.Get());
	Game->bInventoryStateInitialized = true;
	Game->UGameInstance::Init();
	ON_SCOPE_EXIT {
		Game->UGameInstance::Shutdown();
		World->SetGameInstance(nullptr);
		Context.OwningGameInstance = nullptr;
		FContextAccess::Attach(Game.Get(), nullptr);
		World->DestroyWorld(false);
		GEngine->DestroyWorldContext(World);
		World->RemoveFromRoot();
	};
	Game->ResetPlayerSlotArrays();
	Game->ActiveSaveSlotIndex = 1;
	if (!TestTrue(TEXT("Initial persisted difficulty"), Game->SetActiveSaveSlotDifficultyStage(3, true))) return false;
	UClass* MenuClass = LoadClass<UTunaSweeperIntroMenuWidget>(nullptr, TEXT("/Game/UI/WBP_IntroMenu.WBP_IntroMenu_C"));
	if (!TestNotNull(TEXT("Shipped menu class"), MenuClass)) return false;
	TSharedPtr<SWidget> MenuSlate;
	auto Open = [&]()
	{
		auto* Menu = CreateWidget<UTunaSweeperIntroMenuWidget>(Game.Get(), MenuClass);
		Menu->PrepareForDifficultyAdjustment();
		MenuSlate = Menu->TakeWidget();
		return Menu;
	};
	auto* Menu = Open();
	TestNotNull(TEXT("Authored difficulty selection child"), Cast<UDifficultySelectionWidget>(Menu->DifficultySelectPanel));
	TestEqual(TEXT("Shows active slot stage"), Menu->SelectedDifficultyStage, 3);
	if (!TestNotNull(TEXT("Stage one button"), Menu->DifficultyFarmingButton.Get()) ||
		!TestNotNull(TEXT("Apply"), Menu->DifficultyStartButton.Get()) ||
		!TestNotNull(TEXT("Cancel"), Menu->DifficultyBackButton.Get())) return false;
	int32 CloseCount = 0;
	Menu->OnDifficultyAdjustmentClosed.AddLambda([&CloseCount]() { ++CloseCount; });
	Menu->DifficultyFarmingButton->OnClicked.Broadcast();
	Menu->DifficultyFarmingButton->OnClicked.Broadcast();
	TestEqual(TEXT("Selection remains pending until Apply"), Game->GetActiveSaveSlotDifficultyStage(), 3);
	Menu->DifficultyBackButton->OnClicked.Broadcast();
	TestEqual(TEXT("Cancel closes exactly once"), CloseCount, 1);
	Menu->OnDifficultyAdjustmentClosed.Clear();
	Menu = Open();
	Menu->OnDifficultyAdjustmentClosed.AddLambda([&CloseCount]() { ++CloseCount; });
	Menu->DifficultyFarmingButton->OnClicked.Broadcast();
	Game->bSuppressSaveWritesForTests = true;
	Menu->DifficultyStartButton->OnClicked.Broadcast();
	TestEqual(TEXT("Failed save leaves current difficulty intact"), Game->GetActiveSaveSlotDifficultyStage(), 3);
	TestEqual(TEXT("Failed save keeps dialog open"), CloseCount, 1);
	Game->bSuppressSaveWritesForTests = false;
	Menu->DifficultyStartButton->OnClicked.Broadcast();
	TestEqual(TEXT("Apply changes current stage"), Game->GetActiveSaveSlotDifficultyStage(), 1);
	TestEqual(TEXT("Apply closes exactly once"), CloseCount, 2);
	Menu->OnDifficultyAdjustmentClosed.Clear();
	TestTrue(TEXT("Change runtime value without saving"), Game->SetActiveSaveSlotDifficultyStage(2, false));
	TestTrue(TEXT("Reload isolated active slot"), Game->LoadGameState());
	TestEqual(TEXT("Applied difficulty survives reload"), Game->GetActiveSaveSlotDifficultyStage(), 1);
	TestTrue(TEXT("No level travel from adjustment"), Context.TravelURL.IsEmpty() && !Menu->bStartTravelPending);
	return !HasAnyErrors();
}
#endif
