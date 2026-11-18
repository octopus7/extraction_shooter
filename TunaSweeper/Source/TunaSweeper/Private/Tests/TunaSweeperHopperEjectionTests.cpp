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
 TestNull(TEXT("Original Hopper has no shield presentation"),Hopper->FindComponentByTag<UStaticMeshComponent>(TEXT("HopperCockpitShield")));
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperMk2HatchTest, "TunaSweeper.Hopper.Combat.Mk2HatchSequence",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperMk2HatchTest::RunTest(const FString& Parameters)
{
 auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
  .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,
  MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("HopperMk2HatchTest")),GetTransientPackage(),true,ERHIFeatureLevel::Num,&Values);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
 UClass* Class=LoadClass<ATunaSweeperHopperEnemyCharacter>(nullptr,
  TEXT("/Game/Characters/Hopper/BP_HopperEnemyMk2.BP_HopperEnemyMk2_C"));
 if (!TestNotNull(TEXT("Mk2 class loads"),Class)) return false;
 auto* Hopper=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Class,FVector::ZeroVector,FRotator::ZeroRotator);
 auto* P=Hopper->GetPresentation();
 auto* Pilot=P->GetPilotMesh();
 TArray<UStaticMeshComponent*> Meshes; Hopper->GetComponents(Meshes);
 auto Part=[&Meshes](const TCHAR* Prefix)->UStaticMeshComponent*
 {
  for (auto* Mesh:Meshes) if (Mesh->GetName().StartsWith(Prefix)) return Mesh;
  return nullptr;
 };
 auto* Brow=Part(TEXT("GuardBrow"));
 auto* Door=Part(TEXT("Hatch_Pivot"));
 auto* Step=Part(TEXT("RampExtension"));
 auto* Shield=Hopper->FindComponentByTag<UStaticMeshComponent>(TEXT("HopperCockpitShield"));
 if (!TestNotNull(TEXT("Moving guard"),Brow) || !TestNotNull(TEXT("Front hatch"),Door) ||
     !TestNotNull(TEXT("Telescoping step"),Step) || !TestNotNull(TEXT("Captain"),Pilot) ||
     !TestNotNull(TEXT("Mk2 cockpit shield"),Shield)) return false;
 if (!TestEqual(TEXT("One clock drives the field and spark materials"),Shield->GetCustomPrimitiveData().Data.Num(),4)) return false;
 auto Shader=[Shield](int32 Channel) { return Shield->GetCustomPrimitiveData().Data[Channel]; };
 TestTrue(TEXT("Shield and fragments are visual only"),Shield->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
 auto Tick=[P](float Dt) { P->TickComponent(Dt,LEVELTICK_All,nullptr); };
 P->StartBoarding(2.f);
 const FVector Ground=Pilot->GetComponentLocation();
 Tick(.24f);
 TestTrue(TEXT("Guard moves before front door"),!Brow->GetRelativeTransform().Equals(FTransform::Identity,.01f));
 TestTrue(TEXT("Door stays latched during first beat"),Door->GetRelativeTransform().Equals(FTransform::Identity,.01f));
 TestTrue(TEXT("Captain waits outside for the ramp"),Pilot->GetComponentLocation().Equals(Ground,.01f));
 Tick(.57f);
 const FTransform OpenGuard=Brow->GetRelativeTransform();
 TestTrue(TEXT("Hatch opens and step extends past its carrier"),
  !Door->GetRelativeTransform().Equals(FTransform::Identity,.01f) && !Step->GetRelativeTransform().Equals(Door->GetRelativeTransform(),.01f));
 Tick(1.19f);
 P->SeatPilot();
 Tick(.27f);
 TestTrue(TEXT("Guard stays back while the ramp begins folding"),Brow->GetRelativeTransform().Equals(OpenGuard,.01f));
 TestTrue(TEXT("Field stays off while the guard is unlatched"),Shield->bHiddenInGame);
 Tick(P->GetEjectionHatchDuration());
 TestTrue(TEXT("Guard returns to closed rest"),Brow->GetRelativeTransform().Equals(FTransform::Identity,.01f));
 TestTrue(TEXT("Door returns to closed rest"),Door->GetRelativeTransform().Equals(FTransform::Identity,.01f));
 const float IgnitionGain=Shader(1);
 TestTrue(TEXT("Closed guard ignites a bright expanding field"),!Shield->bHiddenInGame && Shader(0)>0.f && IgnitionGain>1.f);
 Tick(1.f);
 TestTrue(TEXT("The deployed shield settles to a faint steady glow"),Shader(0)==1.f && Shader(1)<IgnitionGain*.5f && Shader(1)>0.f);
 P->StartSeatEjection();
 const FVector Seated=Pilot->GetComponentLocation();
 const float HatchDuration=P->GetEjectionHatchDuration();
 const float Samples[]={.144f,.295f,.442f,.60f,.758f,.918f};
 float Elapsed=0.f;
 for (int32 Index=0;Index<UE_ARRAY_COUNT(Samples);++Index)
 {
  Tick(Samples[Index]-Elapsed); Elapsed=Samples[Index];
  TestTrue(TEXT("Three pre-escape flashes alternate with dark troughs"),Index%2==0 ? Shader(1)>1.f : Shader(1)<.25f);
  TestTrue(TEXT("Intact field holds the guard closed during the warning"),P->IsWaitingForShieldBreak() && P->GetHatchOpenAlpha()==0.f && Shader(3)==0.f);
 }
 TestTrue(TEXT("Cracks spread before the intact field cuts out"),Shader(2)>.9f);
 Tick(.05f); Elapsed+=.05f;
 TestTrue(TEXT("Shards and sparks start on the same update that removes field light"),
  !P->IsWaitingForShieldBreak() && Shader(1)==0.f && Shader(3)>=1.f && !Shield->bHiddenInGame);
 Tick(HatchDuration-Elapsed);
 TestTrue(TEXT("Guard is fully back before the seat moves"),Brow->GetRelativeTransform().Equals(OpenGuard,.01f));
 TestTrue(TEXT("Pilot is still seated at the end of opening"),Pilot->GetComponentLocation().Equals(Seated,.01f));
 Tick(P->EjectionSlideSeconds+.25f);
 TestNull(TEXT("Mk2 seat launches free of the mech"),Pilot->GetAttachParent()->GetAttachParent());
 TestTrue(TEXT("No lingering shield or burst on the wreck"),Shield->bHiddenInGame);
 auto* Unboarded=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Class,FVector(600,0,0),FRotator::ZeroRotator);
 auto* Early=Unboarded->GetPresentation(); Early->StartBoarding(2.f);
 Early->TickComponent(.3f,LEVELTICK_All,nullptr);
 Early->StartSeatEjection();
 TestFalse(TEXT("An interrupted boarding sequence cannot flash a shield that never deployed"),Early->IsWaitingForShieldBreak());
 return true;
}
#endif
