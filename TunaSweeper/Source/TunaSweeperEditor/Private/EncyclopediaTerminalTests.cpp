#include "Misc/AutomationTest.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Tests/AutomationEditorCommon.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Encyclopedia/EncyclopediaTerminal.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Misc/Paths.h"

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FExerciseEncyclopediaTerminal, FAutomationTestBase*, Test);
bool FExerciseEncyclopediaTerminal::Update()
{
    UWorld* World = GEditor->PlayWorld;
    auto* Controller = World ? Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController()) : nullptr;
    auto* Player = Controller ? Cast<ATunaSweeperTopDownCharacter>(Controller->GetPawn()) : nullptr;
    if (!Player)
    {
        if (FPlatformTime::Seconds() - StartTime < 45) return false;
        Test->AddError(TEXT("PIE player did not become ready")); return true;
    }
    if (auto* Hud = Controller->GetGameHudWidget(); Hud && Hud->GetHudMode() != ETunaSweeperHudMode::None)
    { Hud->SetHudMode(ETunaSweeperHudMode::None); return false; }
    if (Controller->IsInventoryUiOpen())
    {
        if (FPlatformTime::Seconds() - StartTime < 45) return false;
        Test->AddError(TEXT("HUD did not finish closing")); return true;
    }
    UClass* Class = LoadClass<AEncyclopediaTerminal>(nullptr,
        TEXT("/Game/Interaction/BP_EncyclopediaTerminal.BP_EncyclopediaTerminal_C"));
    if (!Test->TestNotNull(TEXT("Saved terminal Blueprint"), Class)) return true;
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Terminal = World->SpawnActor<AEncyclopediaTerminal>(Class, Player->GetActorLocation() + FVector(30, 0, 0), FRotator::ZeroRotator, Params);
    if (!Test->TestNotNull(TEXT("Spawn BP only in transient PIE world"), Terminal)) return true;
    Test->TestFalse(TEXT("Cannot open without a terminal"), Controller->OpenEncyclopedia(nullptr));
    Test->TestTrue(TEXT("Normal interaction dispatch opens encyclopedia"), Terminal->RequestInteraction(Player));
    Test->TestTrue(TEXT("Modal opened"), Controller->IsEncyclopediaOpen());
    Test->TestTrue(TEXT("Movement locked"), Controller->IsMoveInputIgnored());
    Test->TestTrue(TEXT("Look locked"), Controller->IsLookInputIgnored());
    Test->TestTrue(TEXT("Standalone paused"), UGameplayStatics::IsGamePaused(World));
    Test->TestFalse(TEXT("Duplicate interaction blocked while modal open"), Terminal->RequestInteraction(Player));
    Controller->CloseEncyclopedia();
    Test->TestFalse(TEXT("Close releases movement lock"), Controller->IsMoveInputIgnored());
    Test->TestFalse(TEXT("Close releases look lock"), Controller->IsLookInputIgnored());
    Test->TestFalse(TEXT("Close releases pause"), UGameplayStatics::IsGamePaused(World));
    Terminal->SetActorLocation(Player->GetActorLocation() + FVector(10000, 0, 0));
    Test->TestFalse(TEXT("Out-of-range dispatch rejected"), Terminal->RequestInteraction(Player));
    Test->TestFalse(TEXT("Direct open also checks range"), Controller->OpenEncyclopedia(Terminal));
    Terminal->SetActorLocation(Player->GetActorLocation() + FVector(30, 0, 0));
    Test->TestTrue(TEXT("Reopen through interaction"), Terminal->RequestInteraction(Player));
    Terminal->SetActorLocation(Player->GetActorLocation() + FVector(10000, 0, 0));
    Controller->TickActor(0, LEVELTICK_PauseTick, Controller->PrimaryActorTick);
    Test->TestFalse(TEXT("Leaving range closes modal"), Controller->IsEncyclopediaOpen());
    Terminal->SetActorLocation(Player->GetActorLocation() + FVector(30, 0, 0));
    Test->TestTrue(TEXT("Open before terminal removal"), Terminal->RequestInteraction(Player));
    Terminal->Destroy(); Controller->TickActor(0, LEVELTICK_PauseTick, Controller->PrimaryActorTick);
    Test->TestFalse(TEXT("Destroyed terminal closes modal"), Controller->IsEncyclopediaOpen());
    Controller->CloseEncyclopedia();
    return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FRestoreEncyclopediaSaveRoot);
bool FRestoreEncyclopediaSaveRoot::Update()
{
    TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEncyclopediaTerminalInteractionTest, "TunaSweeper.Encyclopedia.TerminalInteraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FEncyclopediaTerminalInteractionTest::RunTest(const FString&)
{
    const FString Root = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense") / (TEXT("Encyclopedia-") + FGuid::NewGuid().ToString());
    if (!TestTrue(TEXT("Save writes isolated from user saves"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root))) return false;
    if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir() / TEXT("MainRaid/RaidPlains.umap"), false, true))
    { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); return false; }
    FRequestPlaySessionParams Play;
    Play.EditorPlaySettings = NewObject<ULevelEditorPlaySettings>(GetTransientPackage());
    GEditor->RequestPlaySession(Play);
    ADD_LATENT_AUTOMATION_COMMAND(FExerciseEncyclopediaTerminal(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    ADD_LATENT_AUTOMATION_COMMAND(FRestoreEncyclopediaSaveRoot());
    return true;
}
