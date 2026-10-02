#if WITH_DEV_AUTOMATION_TESTS
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperArmModule.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperCombatOwnershipTest,
    "TunaSweeper.Hopper.Combat.PhaseOwnershipAndFiniteAmmo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHopperCombatOwnershipTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (World && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    if (!TestNotNull(TEXT("Test world"), World)) return false;
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ATunaSweeperHopperEnemyCharacter* Hopper = World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(FVector(0,0,105), FRotator::ZeroRotator, Spawn);
    ATunaSweeperHopperEnemyCharacter* Target = World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(FVector(600,0,105), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Hopper"), Hopper) || !TestNotNull(TEXT("Target"), Target))
    {
        World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
        return false;
    }
    Hopper->bInitializingBase = false;
    Hopper->Phase = EHopperCombatPhase::Mech;
    TestFalse(TEXT("Mounted mech uses ranged AI"), Hopper->UsesMeleeAttack());
    Target->GetFactionComponent()->SetFactionId(TunaSweeperFactionIds::Enemy);
    const float InitialPilotHealth = Hopper->GetHealth();
    FDamageEvent Damage;
    TestEqual(TEXT("Friendly damage cannot reduce mech durability"), Hopper->TakeDamage(9999, Damage, nullptr, Target), 0.f);
    TestEqual(TEXT("Friendly target never fires"), Hopper->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::FriendlyTarget);
    TestEqual(TEXT("Lethal armor hit reports armor damage only"), Hopper->TakeDamage(9999, Damage, nullptr, nullptr), Hopper->MechMaxHealth);
    TestFalse(TEXT("Mech destruction does not kill pilot"), Hopper->IsDead());
    TestEqual(TEXT("Overkill does not bleed into pilot"), Hopper->GetHealth(), InitialPilotHealth);
    TestEqual(TEXT("Mech destruction enters disembark"), Hopper->GetCombatPhase(), EHopperCombatPhase::Disembarking);
    TestEqual(TEXT("Damage while exit is obstructed cannot bypass phase ownership"), Hopper->TakeDamage(9999, Damage, nullptr, nullptr), 0.f);
    TestEqual(TEXT("Transitions cannot fire"), Hopper->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::Blocked);
    const FVector BeforeBlockedExit = Hopper->GetActorLocation();
    Hopper->UpdateDisembark(2.f);
    TestEqual(TEXT("No navigation means wait instead of clipping"), Hopper->GetCombatPhase(), EHopperCombatPhase::Disembarking);
    TestEqual(TEXT("Failed exit search preserves capsule position"), Hopper->GetActorLocation(), BeforeBlockedExit);

    // Isolate the pilot combat phase from navigation and the visual timeline.
    Hopper->Phase = EHopperCombatPhase::PilotRanged;
    Hopper->PilotAmmo = 2;
    Hopper->PilotShotCooldown = 0.f;
    Target->GetFactionComponent()->SetFactionId(TunaSweeperFactionIds::Player);
    ATunaSweeperEnemyCharacter* BaseView = Hopper;
    TestFalse(TEXT("Pilot with ammunition uses ranged AI"), BaseView->UsesMeleeAttack());
    TestEqual(TEXT("AI base pointer sees finite magazine"), BaseView->GetEnemyWeaponRuntimeStatus().LoadedAmmo, 2);
    Hopper->SetActorRotation(FRotator(0,180,0));
    TestEqual(TEXT("Shot outside facing cone is blocked"), BaseView->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::Blocked);
    TestEqual(TEXT("Blocked aim cannot consume ammunition"), Hopper->GetPilotAmmo(), 2);
    Hopper->SetActorRotation(FRotator::ZeroRotator);
    TestEqual(TEXT("First shot consumes one round"), BaseView->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::Fired);
    TestEqual(TEXT("One round remains"), Hopper->GetPilotAmmo(), 1);
    TestEqual(TEXT("Last round fires"), BaseView->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::Fired);
    TestEqual(TEXT("Empty pilot changes to melee immediately"), Hopper->GetCombatPhase(), EHopperCombatPhase::PilotMelee);
    TestEqual(TEXT("Melee transition immediately applies pilot speed"), Hopper->GetCharacterMovement()->MaxWalkSpeed, Hopper->PilotMoveSpeed);
    TestTrue(TEXT("AI base pointer sees melee phase"), BaseView->UsesMeleeAttack());
    TestFalse(TEXT("Pilot has no renewable reserve"), BaseView->StartEnemyReload());
    TestEqual(TEXT("Empty pilot cannot spawn another projectile"), BaseView->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::OutOfAmmo);
    TestEqual(TEXT("Ammo cannot become negative"), Hopper->GetPilotAmmo(), 0);
    Hopper->TakeDamage(1, Damage, nullptr, nullptr);
    TestEqual(TEXT("After disembarking damage belongs to rabbit health"), Hopper->GetHealth(), InitialPilotHealth - 1.f);

    // Replacing an arm releases the previous actor instead of leaking it.
    Hopper->Phase = EHopperCombatPhase::Mech;
    Hopper->MechHealth = 10.f;
    TestTrue(TEXT("Arm module installs"), Hopper->SetArmModule(true, AHopperArmModule::StaticClass()));
    AHopperArmModule* OldArm = Hopper->GetArmModule(true);
    TestTrue(TEXT("Replacement installs"), Hopper->SetArmModule(true, AHopperArmModule::StaticClass()));
    TestTrue(TEXT("Old arm is destroyed"), OldArm && OldArm->IsActorBeingDestroyed());
    TestTrue(TEXT("Replacement is a different actor"), OldArm != Hopper->GetArmModule(true));
    TestEqual(TEXT("Equipped mech reports loaded readiness through base API"), BaseView->GetEnemyWeaponRuntimeStatus().LoadedAmmo, 1);
    Hopper->Phase = EHopperCombatPhase::Disembarking;
    TestFalse(TEXT("Arm cannot bypass its owner's transition gate"), Hopper->GetArmModule(true)->TryFireAt(Target));
    Hopper->Phase = EHopperCombatPhase::Mech;
    TestTrue(TEXT("Arm can be removed"), Hopper->SetArmModule(true, nullptr));
    TestNull(TEXT("Removed slot is empty"), Hopper->GetArmModule(true));
    TestEqual(TEXT("Empty mech has no virtual ammunition"), BaseView->GetEnemyWeaponRuntimeStatus().LoadedAmmo, 0);
    Hopper->Phase = EHopperCombatPhase::PilotMelee;
    Hopper->TakeDamage(9999, Damage, nullptr, nullptr);
    TestTrue(TEXT("Only final pilot lethal damage enters base enemy death"), Hopper->IsDead());
    TestEqual(TEXT("Final death is reflected in public phase"), Hopper->GetCombatPhase(), EHopperCombatPhase::Dead);
    TestEqual(TEXT("Repeated lethal damage is ignored"), Hopper->TakeDamage(9999, Damage, nullptr, nullptr), 0.f);
    TestEqual(TEXT("Dead actor cannot attack"), Hopper->TryFireProjectileAt(Target), ETunaSweeperEnemyFireResult::Blocked);
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    World->RemoveFromRoot();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperTransitionClearanceTest,
    "TunaSweeper.Hopper.Combat.BoardingAndSafeDismount",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHopperTransitionClearanceTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("Class default capsule is excluded from editor nav baking"),
        GetDefault<ATunaSweeperHopperEnemyCharacter>()->GetCapsuleComponent()->CanEverAffectNavigation());
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (World && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    if (!TestNotNull(TEXT("Transition world"), World)) return false;
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ATunaSweeperHopperEnemyCharacter* Hopper = World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(FVector(0,0,105), FRotator::ZeroRotator, Spawn);
    if (!TestNotNull(TEXT("Transition hopper"), Hopper))
    {
        World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
        World->RemoveFromRoot();
        return false;
    }
    Hopper->bInitializingBase = false;
    Hopper->BoardingSeconds = .5f;
    Hopper->GetCapsuleComponent()->SetCanEverAffectNavigation(true);
    Hopper->OnConstruction(Hopper->GetActorTransform());
    TestFalse(TEXT("Constructed capsule cannot carve a navigation hole"), Hopper->GetCapsuleComponent()->CanEverAffectNavigation());
    TestTrue(TEXT("Boarding suppresses AI attacks"), Hopper->IsStandardCombatSuppressed());
    Hopper->Tick(.49f);
    TestEqual(TEXT("Boarding waits for full animation duration"), Hopper->GetCombatPhase(), EHopperCombatPhase::Boarding);
    Hopper->Tick(.02f);
    TestEqual(TEXT("Elapsed boarding enters mech combat"), Hopper->GetCombatPhase(), EHopperCombatPhase::Mech);
    TestFalse(TEXT("Mounted AI attacks are enabled"), Hopper->IsStandardCombatSuppressed());
    const float StraightSpeed = Hopper->GetCharacterMovement()->MaxWalkSpeed;
    Hopper->SetActorRotation(FRotator(0, 10, 0));
    Hopper->Tick(.1f);
    TestTrue(TEXT("Turning consumes planted-foot movement budget"), Hopper->GetCharacterMovement()->MaxWalkSpeed < StraightSpeed);
    Hopper->SetActorRotation(FRotator::ZeroRotator);

    auto AddBox = [World](const FVector& Center, const FVector& Extent)
    {
        AActor* Actor = World->SpawnActor<AActor>();
        UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Box);
        Box->SetBoxExtent(Extent);
        Box->SetCollisionProfileName(TEXT("BlockAll"));
        Box->RegisterComponent();
        Actor->SetActorLocation(Center);
        return Box;
    };
    AddBox(FVector(0,0,-10), FVector(1000,1000,10));
    const FVector StartFeet(0,0,100), ExitFeet(200,0,0);
    TestTrue(TEXT("Clear walkable destination fits pilot capsule"), Hopper->IsPilotExitClear(ExitFeet));
    TestTrue(TEXT("Open dismount arc fits pilot capsule"), Hopper->IsPilotTransferClear(StartFeet, ExitFeet));
    UBoxComponent* Wall = AddBox(FVector(100,0,120), FVector(10,30,120));
    TestFalse(TEXT("Wall blocks dismount arc even with clear destination"), Hopper->IsPilotTransferClear(StartFeet, ExitFeet));
    Wall->GetOwner()->SetActorLocation(FVector(200,0,40));
    TestFalse(TEXT("New destination blocker invalidates capsule clearance"), Hopper->IsPilotExitClear(ExitFeet));
    Hopper->Phase = EHopperCombatPhase::Disembarking;
    Hopper->bExitInProgress = true;
    Hopper->ExitStartFeet = StartFeet;
    Hopper->ExitGroundDestination = ExitFeet;
    Hopper->PhaseSeconds = Hopper->DisembarkSeconds;
    const FVector OriginalCapsule = Hopper->GetActorLocation();
    Hopper->UpdateDisembark(.01f);
    TestEqual(TEXT("Dynamic obstruction retains transition"), Hopper->GetCombatPhase(), EHopperCombatPhase::Disembarking);
    TestEqual(TEXT("Dynamic obstruction cannot teleport capsule"), Hopper->GetActorLocation(), OriginalCapsule);
    Wall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Hopper->bExitInProgress = true;
    Hopper->UpdateDisembark(.01f);
    TestEqual(TEXT("Clear final handoff activates finite-ammo pilot"), Hopper->GetCombatPhase(), EHopperCombatPhase::PilotRanged);
    TestEqual(TEXT("Dismount immediately applies pilot movement speed"), Hopper->GetCharacterMovement()->MaxWalkSpeed, Hopper->PilotMoveSpeed);
    TestEqual(TEXT("Pilot capsule replaces mech capsule"), Hopper->GetCapsuleComponent()->GetUnscaledCapsuleRadius(), 16.f);
    TestEqual(TEXT("Pilot handoff refreshes navigation radius"), Hopper->GetNavAgentPropertiesRef().AgentRadius, 16.f);
    TestEqual(TEXT("Pilot handoff refreshes navigation height"), Hopper->GetNavAgentPropertiesRef().AgentHeight, 74.f);
    TestEqual(TEXT("Pilot capsule stands above verified ground"), Hopper->GetActorLocation(), ExitFeet + FVector(0,0,39));
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    World->RemoveFromRoot();
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperLinkedArmVisibilityTest,
    "TunaSweeper.Hopper.Combat.LinkedArmVisibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHopperLinkedArmVisibilityTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (World && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    if (!TestNotNull(TEXT("Visibility world"), World)) return false;
    ATunaSweeperHopperEnemyCharacter* Hopper = World->SpawnActor<ATunaSweeperHopperEnemyCharacter>();
    if (!TestNotNull(TEXT("Visibility hopper"), Hopper))
    {
        World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); return false;
    }
    UTunaSweeperVisionSubjectComponent* Vision = Hopper->FindComponentByClass<UTunaSweeperVisionSubjectComponent>();
    if (!TestNotNull(TEXT("Owner vision subject"), Vision) || !Hopper->SetArmModule(true, AHopperArmModule::StaticClass()))
    {
        World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); return false;
    }
    AHopperArmModule* Arm = Hopper->GetArmModule(true);
    TArray<UPrimitiveComponent*> Parts;
    Arm->GetComponents<UPrimitiveComponent>(Parts);
    TestTrue(TEXT("Arm has render primitives"), Parts.Num() > 0);
    if (Parts.IsEmpty()) { World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); return false; }
    UPrimitiveComponent* SpecialPart = Parts[0];
    SpecialPart->SetRenderInMainPass(false);
    SpecialPart->SetRenderInDepthPass(true);
    SpecialPart->SetCastShadow(false);
    // An unrelated module must not inherit this subject's visibility.
    AHopperArmModule* UnrelatedArm = World->SpawnActor<AHopperArmModule>();
    TArray<UPrimitiveComponent*> UnrelatedParts;
    UnrelatedArm->GetComponents<UPrimitiveComponent>(UnrelatedParts);
    Vision->ApplyVisionVisible(false);
    for (UPrimitiveComponent* Part : Parts)
        TestTrue(TEXT("Owner hiding suppresses all linked arm render passes"), !Part->bRenderInMainPass && !Part->bRenderInDepthPass && !Part->CastShadow);
    for (UPrimitiveComponent* Part : UnrelatedParts)
        TestTrue(TEXT("Unlinked actor keeps its main pass"), Part->bRenderInMainPass);
    Vision->ApplyVisionVisible(true);
    TestFalse(TEXT("Original disabled main pass is restored"), SpecialPart->bRenderInMainPass);
    TestTrue(TEXT("Original enabled depth pass is restored"), SpecialPart->bRenderInDepthPass);
    TestFalse(TEXT("Original disabled shadow is restored"), SpecialPart->CastShadow);
    Vision->ApplyVisionVisible(false);
    Vision->RemoveLinkedActor(Arm);
    TestTrue(TEXT("Unlink immediately restores cached depth flag"), SpecialPart->bRenderInDepthPass);
    TestFalse(TEXT("Unlink preserves original disabled main flag"), SpecialPart->bRenderInMainPass);
    Vision->ApplyVisionVisible(false);
    TestTrue(TEXT("Unlinked actor is no longer hidden by owner"), SpecialPart->bRenderInDepthPass);
    Vision->AddLinkedActor(Arm);
    TestFalse(TEXT("Linking into hidden subject immediately hides primitive"), SpecialPart->bRenderInDepthPass);
    TestTrue(TEXT("Replacing hidden arm succeeds"), Hopper->SetArmModule(true, AHopperArmModule::StaticClass()));
    Parts.Reset();
    Hopper->GetArmModule(true)->GetComponents<UPrimitiveComponent>(Parts);
    for (UPrimitiveComponent* Part : Parts)
        TestTrue(TEXT("Replacement starts hidden without a visible frame"), !Part->bRenderInMainPass && !Part->bRenderInDepthPass && !Part->CastShadow);
    Vision->ApplyVisionVisible(true);
    for (UPrimitiveComponent* Part : Parts)
        TestTrue(TEXT("Replacement restores original main pass"), Part->bRenderInMainPass);
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    World->RemoveFromRoot();
    return true;
}
#endif
