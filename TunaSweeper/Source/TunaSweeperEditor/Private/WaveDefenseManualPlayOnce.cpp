#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
namespace WaveDefenseManual { bool bReady=false; bool bStarted=false; int32 Difficulty=1; double Start=0; int32 LastWave=0; FString Timeline; }
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaveDefenseManualWatch,FAutomationTestBase*,Test);
bool FWaveDefenseManualWatch::Update()
{
    UWorld* World=GEditor->PlayWorld;
    auto* Controller=World ? Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController()) : nullptr;
    auto* Player=Controller ? Cast<ATunaSweeperTopDownCharacter>(Controller->GetPawn()) : nullptr;
    auto* Game=World ? World->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
    AWaveDefenseDirector* Director=nullptr;
    if (World) for (TActorIterator<AWaveDefenseDirector> It(World);It;++It) { Director=*It; break; }
    if (!World || !Player || !Game || !Director)
    {
        if (FPlatformTime::Seconds()-StartTime < 60.0) return false;
        Test->AddError(TEXT("Manual PIE fixture unavailable")); return true;
    }
    if (!WaveDefenseManual::bReady)
    {
        Game->SetActiveSaveSlotDifficultyStage(WaveDefenseManual::Difficulty,false);
        Test->TestTrue(TEXT("Fixture rifle equipped"),Game->AddItemToPreferredAvailableSlot(1002,1));
        Test->TestTrue(TEXT("Fixture rifle ammunition"),Game->AddItemToFirstAvailableInventorySlot(2002,720));
        Test->TestTrue(TEXT("Fixture first aid"),Game->AddItemToFirstAvailableInventorySlot(4002,5));
        Player->SetActorLocation(FVector(4100,0,190),false,nullptr,ETeleportType::TeleportPhysics);
        Test->TestTrue(TEXT("Open terminal panel"),Controller->OpenWaveDefensePanel(Director));
        WaveDefenseManual::bReady=true;
    }
    const auto Snapshot=Director->GetSnapshot();
    if (Snapshot.Phase != EWaveDefensePhase::Idle && !WaveDefenseManual::bStarted)
    { WaveDefenseManual::bStarted=true; WaveDefenseManual::Start=FPlatformTime::Seconds(); }
    if (WaveDefenseManual::bStarted && Snapshot.Wave != WaveDefenseManual::LastWave)
    {
        WaveDefenseManual::LastWave=Snapshot.Wave;
        const float Health=Player->GetVitalsComponent() ? Player->GetVitalsComponent()->GetVitalsState().Health : -1.0f;
        WaveDefenseManual::Timeline+=FString::Printf(TEXT("wave %d: %.1f seconds; core=%d; player=%.1f; reserve_ammo=%d; first_aid=%d\n"),Snapshot.Wave,FPlatformTime::Seconds()-WaveDefenseManual::Start,Snapshot.CoreHealth,Health,Game->CountInventoryItemById(2002),Game->CountInventoryItemById(4002));
    }
    if (WaveDefenseManual::bStarted && Snapshot.Phase==EWaveDefensePhase::Finished)
    {
        const float Health=Player->GetVitalsComponent() ? Player->GetVitalsComponent()->GetVitalsState().Health : -1.0f;
        const FString Result=FString::Printf(TEXT("difficulty=%d\noutcome=%d\nduration_seconds=%.1f\ncore_health=%d\nplayer_health=%.1f\nammo_remaining=%d\n%s"),WaveDefenseManual::Difficulty,int32(Snapshot.Outcome),FPlatformTime::Seconds()-WaveDefenseManual::Start,Snapshot.CoreHealth,Health,Game->CountInventoryItemById(2002),*WaveDefenseManual::Timeline);
        const FString Path=FPaths::ProjectSavedDir()/TEXT("WaveDefense")/FString::Printf(TEXT("manual_stage_%d.txt"),WaveDefenseManual::Difficulty);
        Test->TestTrue(TEXT("Manual result saved"),FFileHelper::SaveStringToFile(Result,*Path)); return true;
    }
    if (FPlatformTime::Seconds()-StartTime >= 1500.0) { Test->AddError(TEXT("Manual session timed out")); return true; }
    return false;
}
DEFINE_LATENT_AUTOMATION_COMMAND(FWaveDefenseManualCleanup);
bool FWaveDefenseManualCleanup::Update() { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); return true; }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseManualPlayOnce, "TunaSweeper.Manual.WaveDefense.PlayOnce", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseManualPlayOnce::RunTest(const FString& Parameters)
{
    const FString Root=FPaths::ProjectSavedDir()/TEXT("Automation/WaveDefense")/FGuid::NewGuid().ToString();
    if (!TestTrue(TEXT("Isolated save root"),TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root))) return false;
    if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir()/TEXT("MainRaid/RaidPlains.umap"),false,true)) { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); return false; }
    WaveDefenseManual::Difficulty=1; FParse::Value(FCommandLine::Get(),TEXT("WaveDefenseManualDifficulty="),WaveDefenseManual::Difficulty);
    WaveDefenseManual::Difficulty=FMath::Clamp(WaveDefenseManual::Difficulty,1,3); WaveDefenseManual::bReady=false; WaveDefenseManual::bStarted=false; WaveDefenseManual::LastWave=0; WaveDefenseManual::Timeline.Reset();
    FRequestPlaySessionParams Play; Play.EditorPlaySettings=NewObject<ULevelEditorPlaySettings>(GetTransientPackage()); Play.EditorPlaySettings->NewWindowWidth=1280; Play.EditorPlaySettings->NewWindowHeight=720; Play.EditorPlaySettings->LastExecutedPlayModeType=EPlayModeType::PlayMode_InEditorFloating;
    GEditor->RequestPlaySession(Play);
    ADD_LATENT_AUTOMATION_COMMAND(FWaveDefenseManualWatch(this)); ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); ADD_LATENT_AUTOMATION_COMMAND(FWaveDefenseManualCleanup()); return true;
}
#endif
