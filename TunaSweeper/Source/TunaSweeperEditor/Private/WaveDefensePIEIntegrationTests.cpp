#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseParticipantComponent.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

namespace WaveDefensePIE
{
    bool bStarted = false;
    TSet<FGuid> Kills;
    TWeakObjectPtr<AWaveDefenseDirector> Director;
}
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FWaveDefensePIEPlay, FAutomationTestBase*, Test);
bool FWaveDefensePIEPlay::Update()
{
    UWorld* World=GEditor->PlayWorld;
    APlayerController* Controller=World ? World->GetFirstPlayerController() : nullptr;
    auto* Player=Controller ? Cast<ATunaSweeperTopDownCharacter>(Controller->GetPawn()) : nullptr;
    if (!World || !Player)
    {
        if (FPlatformTime::Seconds()-StartTime < 45.0) return false;
        Test->AddError(TEXT("PIE player did not become ready")); return true;
    }
    if (!WaveDefensePIE::bStarted)
    {
        AWaveDefenseDirector* Director=nullptr;
        for (TActorIterator<AWaveDefenseDirector> It(World);It;++It) { Director=*It; break; }
        if (!Director) { Test->AddError(TEXT("PIE director missing")); return true; }
        Player->SetActorLocation(FVector(4400,0,190),false,nullptr,ETeleportType::TeleportPhysics);
        if (!Director->TryStart(Player))
        {
            if (FPlatformTime::Seconds()-StartTime < 45.0) return false;
            Test->AddError(TEXT("PIE run could not start")); return true;
        }
        WaveDefensePIE::Director=Director; WaveDefensePIE::bStarted=true;
        UGameplayStatics::SetGlobalTimeDilation(World,10.0f);
    }
    auto* Director=WaveDefensePIE::Director.Get();
    if (!Director) { Test->AddError(TEXT("PIE director disappeared")); return true; }
    const FWaveDefenseSnapshot Snapshot=Director->GetSnapshot();
    if (Snapshot.Phase==EWaveDefensePhase::Preparing || Snapshot.Phase==EWaveDefensePhase::Intermission) Director->SetReady();
    for (TActorIterator<ATunaSweeperEnemyCharacter> It(World);It;++It)
    {
        auto* Participant=It->FindComponentByClass<UWaveDefenseParticipantComponent>();
        if (Participant && Participant->GetDirector()==Director && !It->IsDead())
        {
            const FGuid Token=Participant->GetToken();
            UGameplayStatics::ApplyDamage(*It,1000000.0f,Controller,Player,nullptr);
            if (It->IsDead()) WaveDefensePIE::Kills.Add(Token);
        }
    }
    if (Snapshot.Phase==EWaveDefensePhase::Finished)
    {
        Test->TestEqual(TEXT("All 66 owned enemies defeated"),WaveDefensePIE::Kills.Num(),66);
        Test->TestEqual(TEXT("PIE outcome succeeded"),Snapshot.Outcome,EWaveDefenseOutcome::Succeeded);
        Test->TestTrue(TEXT("PIE reward pending"),Snapshot.bRewardPending);
        UGameplayStatics::SetGlobalTimeDilation(World,1.0f); return true;
    }
    if (FPlatformTime::Seconds()-StartTime >= 120.0)
    {
        Test->AddError(FString::Printf(TEXT("PIE timed out phase=%d wave=%d queued=%d alive=%d killed=%d"),int32(Snapshot.Phase),Snapshot.Wave,Snapshot.Queued,Snapshot.Alive,WaveDefensePIE::Kills.Num()));
        UGameplayStatics::SetGlobalTimeDilation(World,1.0f); return true;
    }
    return false;
}
DEFINE_LATENT_AUTOMATION_COMMAND(FWaveDefensePIECleanup);
bool FWaveDefensePIECleanup::Update()
{
    TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString());
    WaveDefensePIE::Director.Reset(); WaveDefensePIE::Kills.Reset(); WaveDefensePIE::bStarted=false;
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefensePIEIntegrationTest, "TunaSweeper.WaveDefense.Integration.SixWaves", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefensePIEIntegrationTest::RunTest(const FString& Parameters)
{
    const FString Root=FPaths::ProjectSavedDir()/TEXT("Automation/WaveDefense")/FGuid::NewGuid().ToString();
    if (!TestTrue(TEXT("Isolated save root"),TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root))) return false;
    if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir()/TEXT("MainRaid/RaidPlains.umap"),false,true)) { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); return false; }
    WaveDefensePIE::Kills.Reset(); WaveDefensePIE::Director.Reset(); WaveDefensePIE::bStarted=false;
    FRequestPlaySessionParams Play; Play.EditorPlaySettings=NewObject<ULevelEditorPlaySettings>(GetTransientPackage()); Play.EditorPlaySettings->NewWindowWidth=1280; Play.EditorPlaySettings->NewWindowHeight=720;
    GEditor->RequestPlaySession(Play);
    ADD_LATENT_AUTOMATION_COMMAND(FWaveDefensePIEPlay(this));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    ADD_LATENT_AUTOMATION_COMMAND(FWaveDefensePIECleanup());
    return true;
}
#endif
