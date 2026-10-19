#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperEjectionTest, "TunaSweeper.Hopper.Combat.SeatEjection",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperEjectionTest::RunTest(const FString& Parameters)
{
 auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
  .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,
  MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("HopperEjectionTest")),GetTransientPackage(),true,ERHIFeatureLevel::Num,&Values);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
 auto* Hopper=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>();
 auto* P=Hopper->GetPresentation();
 auto* Pilot=P->GetPilotMesh();
 UStaticMeshComponent* Cabin=nullptr;
 TArray<UStaticMeshComponent*> Meshes; Hopper->GetComponents(Meshes);
 for (auto* Mesh:Meshes) if(Mesh->GetName().StartsWith(TEXT("Cabin_Interior"))) Cabin=Mesh;
 if(!TestNotNull(TEXT("Existing cabin/seat assembly"),Cabin)) return false;
 const FVector Start=Pilot->GetComponentLocation(), CabinStart=Cabin->GetComponentLocation();
 Hopper->TakeDamage(10000.f,FDamageEvent(),nullptr,nullptr);
 TestTrue(TEXT("Defeat is immediate, independent of escape animation"),Hopper->IsDead());
 TestEqual(TEXT("No second combat phase"),Hopper->GetCombatPhase(),EHopperCombatPhase::Dead);
 P->TickComponent(.4f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Pilot remains still while hatch opens"),Pilot->GetComponentLocation().Equals(Start,.01f));
 P->TickComponent(.65f,LEVELTICK_All,nullptr);
 const FVector Slide=Pilot->GetComponentLocation()-Start;
 TestTrue(TEXT("Seat slides forward before launch"),Slide.X>20.f && FMath::Abs(Slide.Z)<1.f);
 TestTrue(TEXT("Cabin and seated pilot slide together"),(Cabin->GetComponentLocation()-CabinStart).Equals(Slide,.01f));
 P->TickComponent(.5f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Pilot and seat ascend after slide"),Pilot->GetComponentLocation().Z>Start.Z+30.f);
 TestNull(TEXT("Seat assembly detaches from the mech"),Pilot->GetAttachParent()->GetAttachParent());
 TestEqual(TEXT("Hatch stays open on wreck"),P->GetHatchOpenAlpha(),1.f);
 P->TickComponent(10.f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Escaped pilot is hidden after departing"),Pilot->bHiddenInGame);
 TestTrue(TEXT("Escaped seat is hidden after departing"),Cabin->bHiddenInGame);
 TestFalse(TEXT("Mech wreck remains"),Hopper->IsActorBeingDestroyed());
 return true;
}
#endif
