#if WITH_DEV_AUTOMATION_TESTS
#include "Editor.h"
#include "FileHelpers.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Interaction/TunaSweeperAchievementLocationTrigger.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Game/TunaSweeperGameInstance.h"

namespace ElephantLakeTests
{
struct FRun
{
    FAutomationTestBase* Test;
    int32 Stage = 0;
    double Started = FPlatformTime::Seconds();
    double Changed = Started;
    bool Tick()
    {
        if (FPlatformTime::Seconds()-Started > 60)
        { Test->AddError(TEXT("Elephant lake PIE timed out")); return true; }
        UWorld* World=GEditor->PlayWorld;
        if (!World || !World->GetFirstPlayerController()) return false;
        auto* PC=World->GetFirstPlayerController();
        auto* Pawn=Cast<ATunaSweeperTopDownCharacter>(PC->GetPawn());
        auto* Game=World->GetGameInstance<UTunaSweeperGameInstance>();
        if (!Pawn || !Game) return false;
        auto* Achievements=Game->GetSubsystem<UTunaSweeperAchievementSubsystem>();
        ATunaSweeperAchievementLocationTrigger* Trigger=nullptr;
        for (TActorIterator<ATunaSweeperAchievementLocationTrigger> It(World); It; ++It)
            if (It->GetActorLabel()==TEXT("Forest_ElephantLake_Achievement13")) Trigger=*It;
        if (!Test->TestNotNull(TEXT("Saved lake trigger in PIE"),Trigger)) return true;
        if (!Test->TestNotNull(TEXT("Achievement subsystem"),Achievements)) return true;
        auto* Box=Trigger->FindComponentByClass<UBoxComponent>();
        const FVector Center=Trigger->GetActorLocation();
        if (Stage==0)
        {
            Test->TestFalse(TEXT("Forest entry alone does not unlock 13"),Achievements->IsAchievementUnlocked(TEXT("13")));
            Test->TestTrue(TEXT("Trigger uses the saved shore bounds"),Box && Box->GetUnscaledBoxExtent().Equals(FVector(1300,850,300)));
            // Non-player overlaps cannot consume the one-shot trigger.
            if (Box) Box->OnComponentBeginOverlap.Broadcast(Box,World->SpawnActor<AActor>(),nullptr,0,false,FHitResult());
            Test->TestFalse(TEXT("Non-player overlap remains locked"),Achievements->IsAchievementUnlocked(TEXT("13")));
            Pawn->SetActorLocation(Center+FVector(0,-1200,0),false,nullptr,ETeleportType::TeleportPhysics);
            Stage=1;Changed=FPlatformTime::Seconds();return false;
        }
        if (FPlatformTime::Seconds()-Changed<1) return false;
        if (Stage==1)
        {
            Test->TestFalse(TEXT("Standing outside the lake remains locked"),Achievements->IsAchievementUnlocked(TEXT("13")));
            Pawn->SetActorLocation(Center+FVector(0,-700,0),false,nullptr,ETeleportType::TeleportPhysics);
            Pawn->GetCapsuleComponent()->UpdateOverlaps();
            Stage=2;Changed=FPlatformTime::Seconds();return false;
        }
        Test->TestTrue(TEXT("Real player overlap unlocks 13"),Achievements->IsAchievementUnlocked(TEXT("13")));
        Test->TestTrue(TEXT("Lake LocationId was recorded"),Achievements->HasReachedLocation(TEXT("location.forest.elephant_lake")));
        Pawn->SetActorLocation(Center+FVector(0,-1200,0),false,nullptr,ETeleportType::TeleportPhysics);
        Pawn->SetActorLocation(Center+FVector(0,-700,0),false,nullptr,ETeleportType::TeleportPhysics);
        Test->TestTrue(TEXT("Re-entry preserves unlocked state"),Achievements->IsAchievementUnlocked(TEXT("13")));
        return true;
    }
};
}
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FElephantLakeRun,TSharedPtr<ElephantLakeTests::FRun>,Run);
bool FElephantLakeRun::Update() { return Run->Tick(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperElephantLakePlayTest,"TunaSweeper.ElephantLake.PlayerEntry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperElephantLakePlayTest::RunTest(const FString& Parameters)
{
    if (!FPaths::ProjectSavedDir().Contains(TEXT("ElephantLakeAutomation")))
    { AddError(TEXT("Requires a fresh isolated -UserDir containing ElephantLakeAutomation; never run against player saves.")); return false; }
    if (!TestTrue(TEXT("Load authored forest"),FEditorFileUtils::LoadMap(FPaths::ProjectContentDir()/TEXT("MainRaid/RaidForest.umap"),false,true))) return false;
    FRequestPlaySessionParams Play;
    Play.EditorPlaySettings=NewObject<ULevelEditorPlaySettings>(GetTransientPackage());
    GEditor->RequestPlaySession(Play);
    auto Run=MakeShared<ElephantLakeTests::FRun>();Run->Test=this;
    ADD_LATENT_AUTOMATION_COMMAND(FElephantLakeRun(Run));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
