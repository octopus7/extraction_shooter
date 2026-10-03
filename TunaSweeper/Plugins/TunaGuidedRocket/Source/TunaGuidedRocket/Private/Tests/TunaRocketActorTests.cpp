#include "TunaGuidedRocket.h"
#include "TunaRocketConfig.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/StaticMeshActor.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "TunaRocketEffect.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaRocketLifecycleTest, "TunaGuidedRocket.Actor.Lifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaRocketLifecycleTest::RunTest(const FString& Parameters)
{
    const auto Init = UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true).CreateNavigation(false).CreateAISystem(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    World->SetGameInstance(NewObject<UGameInstance>(GEngine));
    World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();
    World->SetGameMode(FURL());
    auto Spawn = [&](UTunaRocketConfig* Config, AActor* Target = nullptr, AActor* Owner = nullptr)
    {
        const FTransform Transform(FRotator::ZeroRotator, FVector::ZeroVector);
        auto* Rocket = World->SpawnActorDeferred<ATunaGuidedRocket>(ATunaGuidedRocket::StaticClass(), Transform, Owner, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        Rocket->Configuration = Config;
        Rocket->TargetActor = Target;
        Rocket->FinishSpawning(Transform);
        Rocket->DispatchBeginPlay();
        return Rocket;
    };
    auto* Config = NewObject<UTunaRocketConfig>(World);
    Config->Settings.Lifetime = .25f;
    Config->Settings.GuidanceDuration = 0;
    auto* Rocket = Spawn(Config);
    Rocket->Tick(1);
    TestTrue(TEXT("A long frame triggers the fuse"), Rocket->HasDetonated());
    TestTrue(TEXT("Flight stops at fuse position, no full-frame overshoot"), Rocket->GetActorLocation().Equals(FVector(300, 0, 0), .02));
    TestTrue(TEXT("Age clips to fuse"), FMath::IsNearlyEqual(Rocket->GetFlightAge(), .25f));
    Rocket->Detonate();
    TestTrue(TEXT("Repeated detonation remains terminal"), Rocket->GetVelocity().IsZero());

    Config->Settings.Lifetime = std::numeric_limits<float>::quiet_NaN();
    Config->Settings.Speed = -2;
    auto* Invalid = Spawn(Config);
    Invalid->Tick(4);
    TestTrue(TEXT("Invalid DA still expires"), Invalid->HasDetonated());
    TestTrue(TEXT("Negative speed clamps to stationary"), Invalid->GetActorLocation().IsZero());

    Config->Settings = FTunaRocketSettings();
    Config->Settings.GuidanceDelay = 0;
    auto* Target = World->SpawnActor<AStaticMeshActor>(FVector(1000, 0, 0), FRotator::ZeroRotator);
    auto* Lost = Spawn(Config, Target);
    Target->Destroy();
    Lost->Tick(.1f);
    TestTrue(TEXT("Destroyed target ends guidance"), Lost->HasLostGuidance());
    Lost->Tick(.1f);
    TestTrue(TEXT("No target flies forward safely"), Lost->GetActorLocation().Equals(FVector(240, 0, 0), .02));
    Lost->Destroy();

    auto* Wall = World->SpawnActor<AActor>();
    auto* Box = NewObject<UBoxComponent>(Wall);
    Wall->SetRootComponent(Box);
    Box->SetBoxExtent(FVector(10, 500, 500));
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    Box->RegisterComponent();
    Wall->SetActorLocation(FVector(500, 0, 0));
    auto* Impact = Spawn(Config);
    Impact->Tick(1);
    TestTrue(TEXT("Swept flight hits thin wall during long frame"), Impact->HasDetonated());
    TestTrue(TEXT("Impact stays in front of wall"), Impact->GetActorLocation().X < 500);
    auto* Immune = Spawn(Config, nullptr, Wall);
    Immune->Tick(1);
    TestFalse(TEXT("Owner collision ignored"), Immune->HasDetonated());
    TestTrue(TEXT("Travels through owner"), Immune->GetActorLocation().X > 1100);
    Immune->Destroy();
    Box->SetCollisionObjectType(ECC_PhysicsBody);
    auto* PhysicsImpact = Spawn(Config);
    PhysicsImpact->Tick(1);
    TestTrue(TEXT("PhysicsBody obstacles also block rockets"), PhysicsImpact->HasDetonated());
    PhysicsImpact->Destroy();
    Wall->SetActorLocation(FVector(50, 0, 0));
    Box->SetCollisionProfileName(TEXT("Trigger"));
    auto* Victim = World->SpawnActor<APawn>();
    auto* VictimBox = NewObject<UBoxComponent>(Victim);
    Victim->SetRootComponent(VictimBox);
    VictimBox->SetBoxExtent(FVector(10));
    VictimBox->SetCollisionProfileName(TEXT("Pawn"));
    VictimBox->RegisterComponent();
    Victim->SetActorLocation(FVector(100, 0, 0));
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* Source = World->SpawnActor<APawn>();
    Source->Controller = Controller;
    Config->Settings.Damage = 5;
    Config->Settings.DamageRadius = 200;
    auto* Blast = Spawn(Config);
    Blast->SetInstigator(Source);
    Blast->SimpleEffectClass = ATunaRocketEffect::StaticClass();
    Blast->Detonate();
    TestTrue(TEXT("Trigger volumes do not shield explosion damage"), Victim->LastHitBy == Controller);
    auto CountEffects = [&]() { int32 Count = 0; for (TActorIterator<ATunaRocketEffect> It(World); It; ++It) ++Count; return Count; };
    TestEqual(TEXT("One explosion effect"), CountEffects(), 1);
    Blast->Detonate();
    TestEqual(TEXT("Repeated detonation creates no duplicate effect"), CountEffects(), 1);
    Victim->LastHitBy = nullptr;
    Box->SetCollisionProfileName(TEXT("BlockAll"));
    auto* Covered = Spawn(Config);
    Covered->SetInstigator(Source);
    Covered->Detonate();
    TestTrue(TEXT("Physical cover blocks blast"), Victim->LastHitBy == nullptr);
    World->DestroyWorld(false);
    GEngine->DestroyWorldContext(World);
    return true;
}
#endif
