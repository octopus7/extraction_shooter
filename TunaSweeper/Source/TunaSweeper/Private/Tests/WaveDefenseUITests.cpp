#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UI/Terra/WaveDefenseHUDGPT61Terra.h"
#include "UI/Terra/WaveDefenseTextHelpers.h"
#include "WaveDefense/WaveDefenseDirector.h"
#include "WaveDefense/WaveDefenseDefinition.h"
#include "AI/TunaSweeperEnemyCharacter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWaveDefenseHUDReuseTest, "TunaSweeper.WaveDefense.UI.ReuseAndResults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWaveDefenseHUDReuseTest::RunTest(const FString& Parameters)
{
    const FString Root = FPaths::ProjectSavedDir() / TEXT("Automation/WaveDefense") / FGuid::NewGuid().ToString();
    if (!TestTrue(TEXT("Isolated save directory"), TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(Root))) return false;
    ON_SCOPE_EXIT { TunaSweeperBuildFlavor::SetWaveDefenseTestSaveDirectory(FString()); };
    struct FContextAccess : UGameInstance { static void Attach(UGameInstance* Game,FWorldContext* Context) { auto Member=&FContextAccess::WorldContext; Game->*Member=Context; } };
    struct FHUDTickAccess : UWaveDefenseHUDGPT61Terra { static void TickHUD(UWaveDefenseHUDGPT61Terra* Widget) { auto Member=&FHUDTickAccess::NativeTick; (Widget->*Member)(FGeometry(),0.1f); } };
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false);
    FWorldContext& Context=GEngine->CreateNewWorldContext(EWorldType::Game); Context.SetCurrentWorld(World);
    auto* Game=NewObject<UTunaSweeperGameInstance>(GEngine); FContextAccess::Attach(Game,&Context); Context.OwningGameInstance=Game; World->SetGameInstance(Game); Game->UGameInstance::Init();
    ON_SCOPE_EXIT { Game->UGameInstance::Shutdown(); World->SetGameInstance(nullptr); Context.OwningGameInstance=nullptr; FContextAccess::Attach(Game,nullptr); World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
    auto* Director=World->SpawnActor<AWaveDefenseDirector>();
    UClass* Class=LoadClass<UWaveDefenseHUDGPT61Terra>(nullptr,TEXT("/Game/MainRaid/WaveDefense/UI/WBP_WaveDefenseHUD_GPT61Terra.WBP_WaveDefenseHUD_GPT61Terra_C"));
    if (!TestNotNull(TEXT("Saved HUD class"),Class)) return false;
    auto* HUD=CreateWidget<UWaveDefenseHUDGPT61Terra>(Game,Class);
    if (!TestNotNull(TEXT("HUD instance"),HUD)) return false;
    HUD->SetDirector(Director); HUD->TakeWidget();
    auto* Status=Cast<UTextBlock>(HUD->WidgetTree->FindWidget(TEXT("StatusText")));
    if (!TestNotNull(TEXT("Bound status"),Status)) return false;
    FHUDTickAccess::TickHUD(HUD);
    TestEqual(TEXT("Idle hides content"),Status->GetVisibility(),ESlateVisibility::Collapsed);
    auto* Definition=NewObject<UWaveDefenseDefinition>(); Definition->EncounterId=TEXT("test.ui");
    for (int32 I=0;I<6;++I) { FWaveDefenseWave Wave; Wave.AliveCap=1; FWaveDefenseEnemyEntry Enemy; Enemy.EnemyClass=ATunaSweeperEnemyCharacter::StaticClass(); Enemy.CombatProfile=TEXT("test.profile"); Enemy.Count=1; Wave.Enemies.Add(Enemy); Definition->Waves.Add(Wave); }
    TestTrue(TEXT("Begin first run"),Director->Run.Start(*Definition,FGuid::NewGuid()));
    FHUDTickAccess::TickHUD(HUD);
    TestEqual(TEXT("HUD reappears after idle"),Status->GetVisibility(),ESlateVisibility::HitTestInvisible);
    Director->Run.Abort(EWaveDefenseOutcome::Abandoned); Director->Run.Finish(); FHUDTickAccess::TickHUD(HUD);
    TestEqual(TEXT("Finished hides content"),Status->GetVisibility(),ESlateVisibility::Collapsed);
    Director->Run=FWaveDefenseRun(); TestTrue(TEXT("Begin second run"),Director->Run.Start(*Definition,FGuid::NewGuid())); FHUDTickAccess::TickHUD(HUD);
    TestEqual(TEXT("HUD reappears for next run"),Status->GetVisibility(),ESlateVisibility::HitTestInvisible);
    FWaveDefenseSnapshot Snapshot; Snapshot.Phase=EWaveDefensePhase::Resolving; Snapshot.Outcome=EWaveDefenseOutcome::Succeeded;
    TestTrue(TEXT("Uncommitted success is not displayed"),WaveDefenseText::Result(Game,Snapshot).IsEmpty());
    Snapshot.bRewardPending=true;
    TestEqual(TEXT("Claim error takes priority over pending reward"),WaveDefenseText::Message(Game,Snapshot,true,TEXT("ui.wave_defense.north_barrier")).ToString(),WaveDefenseText::Get(Game,TEXT("ui.wave_defense.action_failed")).ToString());
    return true;
}
#endif
