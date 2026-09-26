#if WITH_DEV_AUTOMATION_TESTS

#include "BossLab/TunaSweeperBossLabGameMode.h"
#include "BossLab/TunaSweeperBossLabSubsystem.h"
#include "BossLab/TunaSweeperModularBoss.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Combat/TunaSweeperBossEncounter.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "Interaction/TunaSweeperWarpPointActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "UI/TunaSweeperBossLabWidget.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Widgets/SViewport.h"

namespace TunaBossLabPlayTests
{
	struct FState
	{
		FAutomationTestBase* Test = nullptr;
		TWeakObjectPtr<UWorld> EditorWorld;
		TSubclassOf<AGameModeBase> OriginalMode;
		TWeakObjectPtr<UTunaSweeperGameInstance> Instance;
		TWeakObjectPtr<UWorld> PlayWorld;
		TWeakObjectPtr<ACameraActor> WorkshopCamera;
		FTransform WorkshopCameraTransform;
		FVector WorkshopViewLocation = FVector::ZeroVector;
		FRotator WorkshopViewRotation = FRotator::ZeroRotator;
		FDelegateHandle CleanupHandle;
		double EndWaitStarted = 0.0;
		bool bSawWorldCleanup = false;
		bool bSessionRestoredBeforeCleanup = false;
		bool bOriginalDirty = false;
		FGuid DraftId;
		~FState() { FWorldDelegates::OnWorldCleanup.Remove(CleanupHandle); }
	};
	void Capture(const FString& Name)
	{
		TArray<FColor> Pixels;
		FIntVector Size = FIntVector::ZeroValue;
		const TSharedPtr<SViewport> ViewportWidget = GEngine->GetGameViewportWidget();
		const bool bCapturedSlate = ViewportWidget.IsValid() && FSlateApplication::IsInitialized() &&
			FSlateApplication::Get().TakeScreenshot(ViewportWidget.ToSharedRef(), Pixels, Size);
		if (!bCapturedSlate)
		{
			FViewport* Viewport = GEditor->GetPIEViewport();
			if (!Viewport || !Viewport->ReadPixels(Pixels)) return;
			Size = FIntVector(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, 0);
		}
		TArray64<uint8> Png;
		FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("Screenshots/BossLab");
		IFileManager::Get().MakeDirectory(*Directory, true);
		FFileHelper::SaveArrayToFile(Png, *(Directory / Name));
	}
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FBossLabPlayStep,
	TSharedPtr<TunaBossLabPlayTests::FState>, State, int32, Step);
bool FBossLabPlayStep::Update()
{
	auto* Test = State->Test;
	if (Step == 4)
	{
		// FEndPlayMapCommand only queues teardown; actor EndPlay happens on a later editor tick.
		if (GEditor->PlayWorld)
		{
			if (State->EndWaitStarted == 0.0) State->EndWaitStarted = FPlatformTime::Seconds();
			if (FPlatformTime::Seconds() - State->EndWaitStarted < 10.0) return false;
			Test->AddError(TEXT("PIE did not finish teardown within ten seconds."));
		}
		Test->TestTrue(TEXT("PIE reached world cleanup after actor EndPlay"), State->bSawWorldCleanup);
		Test->TestTrue(TEXT("Ending PIE restores the combat-test inventory backup before world cleanup"), State->bSessionRestoredBeforeCleanup);
		FWorldDelegates::OnWorldCleanup.Remove(State->CleanupHandle);
		State->Instance.Reset();
		if (State->EditorWorld.IsValid())
		{
			State->EditorWorld->GetWorldSettings()->DefaultGameMode = State->OriginalMode;
			State->EditorWorld->GetOutermost()->SetDirtyFlag(State->bOriginalDirty);
		}
		return true;
	}
	UWorld* World = GEditor->PlayWorld;
	if (!Test->TestNotNull(TEXT("Boss lab PIE world exists"), World)) return true;
	auto* Mode = World->GetAuthGameMode<ATunaSweeperBossLabGameMode>();
	auto* PC = Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController());
	if (!Test->TestNotNull(TEXT("Native boss lab mode overrides the authored test mode"), Mode) ||
		!Test->TestNotNull(TEXT("Normal gameplay controller is available"), PC)) return true;
	auto* GI = World->GetGameInstance<UTunaSweeperGameInstance>();
	auto* Lab = GI ? GI->GetSubsystem<UTunaSweeperBossLabSubsystem>() : nullptr;
	if (!Test->TestNotNull(TEXT("Boss library session exists without selecting a save"), Lab)) return true;
	if (Step == 0)
	{
		State->Instance = GI;
		State->PlayWorld = World;
		State->CleanupHandle = FWorldDelegates::OnWorldCleanup.AddLambda(
			[WeakState = TWeakPtr<TunaBossLabPlayTests::FState>(State)](UWorld* CleaningWorld, bool, bool)
			{
				if (const auto Pinned = WeakState.Pin(); Pinned && Pinned->PlayWorld.Get() == CleaningWorld)
				{
					Pinned->bSawWorldCleanup = true;
					Pinned->bSessionRestoredBeforeCleanup = Pinned->Instance.IsValid() && !Pinned->Instance->IsCombatTestSession();
				}
			});
		State->DraftId = Lab->GetDraft().BossId;
		State->WorkshopCamera = Cast<ACameraActor>(PC->GetViewTarget());
		if (Test->TestNotNull(TEXT("Workshop uses its preview camera"), State->WorkshopCamera.Get()))
			State->WorkshopCameraTransform = State->WorkshopCamera->GetActorTransform();
		if (Test->TestNotNull(TEXT("Workshop has a camera manager"), PC->PlayerCameraManager.Get()))
		{
			State->WorkshopViewLocation = PC->PlayerCameraManager->GetCameraLocation();
			State->WorkshopViewRotation = PC->PlayerCameraManager->GetCameraRotation();
		}
		Test->TestTrue(TEXT("Editor entry protects gameplay saves"), GI->IsCombatTestSession());
		Test->TestFalse(TEXT("Workshop begins outside battle"), Mode->IsBattleActive());
		Test->TestNotNull(TEXT("Preview assembly created"), Mode->GetBoss());
		int32 OldEncounters = 0, OldPortals = 0;
		for (TActorIterator<ATunaSweeperBossEncounter> It(World); It; ++It) ++OldEncounters;
		for (TActorIterator<ATunaSweeperWarpPointActor> It(World); It; ++It) ++OldPortals;
		Test->TestEqual(TEXT("Legacy encounter triggers removed before BeginPlay"), OldEncounters, 0);
		Test->TestEqual(TEXT("Legacy portal routing removed"), OldPortals, 0);
		TArray<UUserWidget*> Widgets;
		UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UTunaSweeperBossLabWidget::StaticClass(), true);
		Test->TestEqual(TEXT("Workshop has one live widget"), Widgets.Num(), 1);
		TunaBossLabPlayTests::Capture(TEXT("Workshop.png"));
		Mode->StartBattle();
		Test->TestTrue(TEXT("First attempt starts"), Mode->IsBattleActive());
		APawn* SpawnedPawn = PC->GetPawn();
		Test->TestNotNull(TEXT("First attempt supplies a gameplay pawn"), SpawnedPawn);
	}
	else if (Step == 1)
	{
		TunaBossLabPlayTests::Capture(TEXT("Battle.png"));
		auto* HUD = PC->GetGameHudWidget();
		if (Test->TestNotNull(TEXT("Combat HUD exists"), HUD))
		{
			HUD->SetHudMode(ETunaSweeperHudMode::Inventory);
			Test->TestTrue(TEXT("Regression precondition: inventory captures gameplay"), PC->IsInventoryUiOpen());
		}
		Mode->StopBattle();
		Test->TestFalse(TEXT("Stop returns to workshop"), Mode->IsBattleActive());
		Test->TestEqual(TEXT("Stopping retains draft identity"), Lab->GetDraft().BossId, State->DraftId);
		Mode->StartBattle();
		Test->TestTrue(TEXT("Second attempt starts after inventory was open"), Mode->IsBattleActive());
	}
	else if (Step == 2)
	{
		Test->TestFalse(TEXT("New attempt releases old inventory input capture"), PC->IsInventoryUiOpen());
		Test->TestFalse(TEXT("New attempt accepts movement"), PC->IsMoveInputIgnored());
		Test->TestFalse(TEXT("New attempt accepts aiming"), PC->IsLookInputIgnored());
		if (auto* Boss = Mode->GetBoss())
			Test->TestEqual(TEXT("New attempt restores full core health"), Boss->GetCoreHealth(), Boss->GetCoreMaxHealth());
		Mode->StopBattle();
		Test->TestTrue(TEXT("Save protection remains while workshop is open"), GI->IsCombatTestSession());
	}
	else if (Step == 3)
	{
		Test->TestFalse(TEXT("Repeated stop leaves workshop idle"), Mode->IsBattleActive());
		Test->TestEqual(TEXT("Repeated attempts keep the same draft"), Lab->GetDraft().BossId, State->DraftId);
		Test->TestEqual(TEXT("Returned workshop retains the preview view target"), PC->GetViewTarget(), static_cast<AActor*>(State->WorkshopCamera.Get()));
		if (Test->TestNotNull(TEXT("Preview camera survives repeated battles"), State->WorkshopCamera.Get()))
			Test->TestTrue(TEXT("Returned preview camera keeps its original placement"), State->WorkshopCamera->GetActorTransform().Equals(State->WorkshopCameraTransform, 0.1f));
		if (Test->TestNotNull(TEXT("Returned workshop has a camera manager"), PC->PlayerCameraManager.Get()))
		{
			Test->TestEqual(TEXT("Camera manager follows the returned preview target"), PC->PlayerCameraManager->GetViewTarget(), static_cast<AActor*>(State->WorkshopCamera.Get()));
			Test->TestTrue(TEXT("Returned rendered camera location matches initial workshop"), PC->PlayerCameraManager->GetCameraLocation().Equals(State->WorkshopViewLocation, 0.1f));
			Test->TestTrue(TEXT("Returned rendered camera rotation matches initial workshop"), PC->PlayerCameraManager->GetCameraRotation().Equals(State->WorkshopViewRotation, 0.1f));
		}
		TunaBossLabPlayTests::Capture(TEXT("WorkshopAfterBattle.png"));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperBossLabPlayTest,
	"TunaSweeper.BossLab.Play.RoundTrip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperBossLabPlayTest::RunTest(const FString& Parameters)
{
	if (!GEditor || GEditor->PlayWorld) { AddError(TEXT("Boss lab smoke requires an editor outside PIE.")); return false; }
	if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir() / TEXT("Maps/BossCombatTestMap.umap"), false, true)) return false;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!World) return false;
	auto State = MakeShared<TunaBossLabPlayTests::FState>();
	State->Test = this; State->EditorWorld = World;
	State->OriginalMode = World->GetWorldSettings()->DefaultGameMode;
	State->bOriginalDirty = World->GetOutermost()->IsDirty();
	World->GetWorldSettings()->DefaultGameMode = ATunaSweeperBossLabGameMode::StaticClass();
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.f));
	ADD_LATENT_AUTOMATION_COMMAND(FBossLabPlayStep(State, 0));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(2.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FBossLabPlayStep(State, 1));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FBossLabPlayStep(State, 2));
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.4f));
	ADD_LATENT_AUTOMATION_COMMAND(FBossLabPlayStep(State, 3));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	ADD_LATENT_AUTOMATION_COMMAND(FBossLabPlayStep(State, 4));
	return true;
}

#endif
