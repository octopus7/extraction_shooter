#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/WorldSettings.h"
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"

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
 auto* Cabin=Part(TEXT("Cabin_Interior"));
 auto* Shield=Hopper->FindComponentByTag<UStaticMeshComponent>(TEXT("HopperCockpitShield"));
 if (!TestNotNull(TEXT("Moving guard"),Brow) || !TestNotNull(TEXT("Front hatch"),Door) ||
     !TestNotNull(TEXT("Telescoping step"),Step) || !TestNotNull(TEXT("Captain"),Pilot) || !TestNotNull(TEXT("Seat carriage"),Cabin) ||
     !TestNotNull(TEXT("Mk2 cockpit shield"),Shield)) return false;
 if (!TestEqual(TEXT("One clock drives the field and spark materials"),Shield->GetCustomPrimitiveData().Data.Num(),4)) return false;
 auto Shader=[Shield](int32 Channel) { return Shield->GetCustomPrimitiveData().Data[Channel]; };
 TestTrue(TEXT("Shield and fragments are visual only"),Shield->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
 auto Tick=[P](float Dt) { P->TickComponent(Dt,LEVELTICK_All,nullptr); };
 USceneComponent* Seat=Pilot->GetAttachParent();
 const FVector Locked=Seat->GetRelativeLocation();
 P->StartBoarding(2.f);
 const FVector Ground=Pilot->GetComponentLocation();
 Tick(.24f);
 TestTrue(TEXT("Guard moves before front door"),!Brow->GetRelativeTransform().Equals(FTransform::Identity,.01f));
 TestTrue(TEXT("Door stays latched during first beat"),Door->GetRelativeTransform().Equals(FTransform::Identity,.01f));
 TestTrue(TEXT("Captain waits outside for the ramp"),Pilot->GetComponentLocation().Equals(Ground,.01f));
 Tick(.57f);
 const FTransform OpenGuard=Brow->GetRelativeTransform();
 TestTrue(TEXT("Rabbit turns to face the machine before pushing off"),FVector::DotProduct(Pilot->GetForwardVector(),P->GetBodyRoot()->GetForwardVector())<-.95f);
 TestTrue(TEXT("Hatch opens and step extends past its carrier"),
  !Door->GetRelativeTransform().Equals(FTransform::Identity,.01f) && !Step->GetRelativeTransform().Equals(Door->GetRelativeTransform(),.01f));
 auto BurstCount=[World](const TCHAR* Name)
 {
  TArray<UNiagaraComponent*> Fx; World->GetWorldSettings()->GetComponents(Fx);
  return Fx.FilterByPredicate([Name](const UNiagaraComponent* C){return C->GetAsset() && C->GetAsset()->GetFName()==Name;}).Num();
 };
 const float SeatedTime=P->GetBoardingClimbEndTime(2.f);
 const float ClimbStart=SeatedTime-2.f;
 float BoardTime=.81f;
 auto BoardTo=[&](float T) { Tick(T-BoardTime); BoardTime=T; };
 auto PilotPosition=[&]() {return P->GetBodyRoot()->GetComponentTransform().InverseTransformPosition(Pilot->GetComponentLocation());};
 BoardTo(ClimbStart);
 const FVector Pickup=Seat->GetRelativeLocation();
 TestTrue(TEXT("The empty carriage waits below the locked seat at the ladder head"),Pickup.X>Locked.X+30 && Pickup.Z<Locked.Z-18);
 BoardTo(ClimbStart+2.f*.23f); const FVector FirstApex=PilotPosition();
 BoardTo(ClimbStart+2.f*.33f); const FVector Lower=PilotPosition();
 BoardTo(ClimbStart+2.f*.39f);
 TestTrue(TEXT("First hop lands and loads both boots on the lower tread"),FirstApex.Z>Lower.Z+8 && Lower.Z>45 && Lower.Z<65 && PilotPosition().Equals(Lower,.05));
 BoardTo(ClimbStart+2.f*.55f); const FVector SecondApex=PilotPosition();
 BoardTo(ClimbStart+2.f*.65f); const FVector Middle=PilotPosition();
 BoardTo(ClimbStart+2.f*.71f);
 TestTrue(TEXT("Second hop lands and compresses on a higher tread"),SecondApex.Z>Middle.Z+7 && Middle.Z>Lower.Z+12 && Middle.X<Lower.X-6 && PilotPosition().Equals(Middle,.05));
 TestTrue(TEXT("The rabbit faces into the ladder on both footfalls"),FVector::DotProduct(Pilot->GetForwardVector(),P->GetBodyRoot()->GetForwardVector())<-.99f);
 bool bTurnedInFlight=false;
 double Peak=Middle.Z;
 for (int32 Sample=73;Sample<91;++Sample)
 {
  BoardTo(ClimbStart+2.f*Sample/100.f);
  Peak=FMath::Max(Peak,PilotPosition().Z);
  bTurnedInFlight|=PilotPosition().Z>Middle.Z+5 && FMath::Abs(FVector::DotProduct(Pilot->GetForwardVector(),P->GetBodyRoot()->GetForwardVector()))<.9f;
 }
 TestTrue(TEXT("The rabbit turns during the final leap and clears the loading seat"),bTurnedInFlight && Peak>Pickup.Z+10);
 BoardTo(ClimbStart+2.f*.94f);
 TestTrue(TEXT("Third hop lands facing mech forward on the stationary low seat"),Pilot->GetAttachParent()==Seat && Seat->GetRelativeLocation().Equals(Pickup,.05) &&
  FVector::DotProduct(Pilot->GetForwardVector(),P->GetBodyRoot()->GetForwardVector())>.99f);
 BoardTo(SeatedTime);
 Tick(.24f);
 const FVector LoadedLift=Seat->GetRelativeLocation();
 TestTrue(TEXT("Occupied chair first climbs the vertical guide"),FMath::IsNearlyEqual(LoadedLift.X,Pickup.X,.05) && LoadedLift.Z>Pickup.Z+10 && LoadedLift.Z<Locked.Z+4);
 Tick(.34f);
 const FVector Outer=Seat->GetRelativeLocation();
 TestTrue(TEXT("Occupied carriage reaches the horizontal rail"),FMath::IsNearlyEqual(Outer.X,Pickup.X,.05) && Outer.Z>Locked.Z+4);
 TestTrue(TEXT("Pilot and control consoles share the carriage when seated"),Pilot->GetAttachParent()==Seat && Cabin->GetAttachParent()==Seat);
 Tick(.26f);
 const FVector Inward=Seat->GetRelativeLocation();
 TestTrue(TEXT("Occupied seat travels inward at constant height"),Inward.X<Outer.X-2 && Inward.X>Locked.X+2 && FMath::IsNearlyEqual(Inward.Z,Outer.Z,.05));
 Tick(.32f);
 TestTrue(TEXT("Horizontal rails reach their stop before the vertical stroke"),Seat->GetRelativeLocation().Equals(FVector(Locked.X,Locked.Y,Outer.Z),.05));
 Tick(.19f);
 const FVector Lowering=Seat->GetRelativeLocation();
 TestTrue(TEXT("Seat drops vertically only after docking"),FMath::IsNearlyEqual(Lowering.X,Locked.X,.05) && Lowering.Z<Outer.Z-1 && Lowering.Z>Locked.Z+1);
 TestTrue(TEXT("Front hatch stays fully open until the seat locks"),P->GetHatchOpenAlpha()==1.f && Shield->bHiddenInGame);
 TestEqual(TEXT("Lock sparks wait for metal contact"),BurstCount(TEXT("NS_HopperMk2SeatLockSparks")),0);
 Tick(.15f);
 TestTrue(TEXT("Carriage reaches its locking position"),Seat->GetRelativeLocation().Equals(Locked,.05));
 TestEqual(TEXT("One fine lock strike on each keeper"),BurstCount(TEXT("NS_HopperMk2SeatLockSparks")),2);
 Tick(.27f);
 TestEqual(TEXT("Holding the latch never replays the strike"),BurstCount(TEXT("NS_HopperMk2SeatLockSparks")),2);
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
 TestNull(TEXT("Guard paint stays clean throughout the shield warning"),Brow->GetOverlayMaterial());
 auto* Flash=Hopper->FindComponentByTag<UMaterialBillboardComponent>(TEXT("HopperBlastFlash"));
 auto* FlashMaterial=Flash ? Cast<UMaterialInstanceDynamic>(Flash->GetMaterial(0)) : nullptr;
 TestTrue(TEXT("The queued blast core is dark throughout the shield warning"),
  FlashMaterial && FlashMaterial->K2_GetScalarParameterValue(TEXT("FlashGain"))==0.f);
 Tick(.05f); Elapsed+=.05f;
 TestTrue(TEXT("Shards and sparks start on the same update that removes field light"),
  !P->IsWaitingForShieldBreak() && Shader(1)==0.f && Shader(3)>=1.f && !Shield->bHiddenInGame);
 auto* Scorch=Cast<UMaterialInstanceDynamic>(Brow->GetOverlayMaterial());
 TestNotNull(TEXT("Dry blast soot appears on the shatter frame"),Scorch);
 TestTrue(TEXT("The blast core lights up on the same update as the soot and shards"),
  FlashMaterial && FlashMaterial->K2_GetScalarParameterValue(TEXT("FlashGain"))==1.f);
 auto AttachedFires=[Hopper,P]()
 {
  TArray<UNiagaraComponent*> Systems;
  Hopper->GetComponents(Systems);
  return Systems.FilterByPredicate([P](const UNiagaraComponent* System)
  {
   return System->GetAttachParent()==P->GetBodyRoot() && System->GetAsset() && System->GetAsset()->GetFName()==TEXT("NS_HopperMk2HullFire");
  }).Num();
 };
 float PaintFade=-1.f;
 if (Scorch)
 {
  TestTrue(TEXT("Blast deposits soot with the original pigments still visible"),
   Scorch->GetScalarParameterValue(FMaterialParameterInfo(TEXT("PaintFade")),PaintFade) && PaintFade==0.f);
 }
 TestNull(TEXT("The escaping seat interior is clean"),Cabin->GetOverlayMaterial());
 P->StartDestructionEffects(nullptr);
 for (const TCHAR* Effect:{TEXT("NS_HopperMk2RadialBlast"),TEXT("NS_HopperMk2MetalDebris"),TEXT("NS_HopperMk2OilSpray")})
  TestEqual(FString::Printf(TEXT("%s starts once on the shield break"),Effect),BurstCount(Effect),1);
 TestEqual(TEXT("Sustained fire waits for the pilot to escape"),AttachedFires()+BurstCount(TEXT("NS_HopperMk2HullFire")),0);
 TestEqual(TEXT("The Mk2 replaces the common rubble explosion"),BurstCount(TEXT("NS_Explosion_SmokeRobot")),0);
 Tick(HatchDuration-Elapsed);
 TestNull(TEXT("The impulse is over before the seat starts to move"),
  Hopper->FindComponentByTag<UMaterialBillboardComponent>(TEXT("HopperBlastFlash")));
 TestTrue(TEXT("Guard is fully back before the seat moves"),Brow->GetRelativeTransform().Equals(OpenGuard,.01f));
 TestTrue(TEXT("Pilot is still seated at the end of opening"),Pilot->GetComponentLocation().Equals(Seated,.01f));
 Tick(.15f);
 const FVector Rising=Seat->GetRelativeLocation();
 TestTrue(TEXT("Emergency release lifts vertically with the door clear"),FMath::IsNearlyEqual(Rising.X,Locked.X,.05) && Rising.Z>Locked.Z+1 && Rising.Z<Outer.Z-1);
 Tick(.15f+P->EjectionSlideSeconds*.5f);
 const FVector Outward=Seat->GetRelativeLocation();
 TestTrue(TEXT("Ejection rail advances at the raised height"),Outward.X>Locked.X+10 && FMath::IsNearlyEqual(Outward.Z,Outer.Z,.05));
 if (Scorch) TestEqual(TEXT("Paint stays at first-stage soot during the rail slide"),Scorch->K2_GetScalarParameterValue(TEXT("PaintFade")),0.f);
 Tick(P->EjectionSlideSeconds*.5f+.055f);
 TestEqual(TEXT("Both rail mouths flash with the stronger ejection effect"),BurstCount(TEXT("NS_HopperMk2SeatEjectSparks")),2);
 TestNull(TEXT("Mk2 seat launches free of the mech"),Pilot->GetAttachParent()->GetAttachParent());
 TestEqual(TEXT("The seat visibly clears the wreck before it ignites"),AttachedFires()+BurstCount(TEXT("NS_HopperMk2HullFire")),0);
 Tick(.195f);
 TestEqual(TEXT("Only the empty chassis catches fire"),AttachedFires(),1);
 TestTrue(TEXT("No lingering shield or burst on the wreck"),Shield->bHiddenInGame);
 TArray<UNiagaraComponent*> Jets;
 Hopper->GetComponents(Jets);
 Jets.RemoveAll([Seat](UNiagaraComponent* Jet){ return Jet->GetAttachParent()!=Seat; });
 if (TestEqual(TEXT("Two nozzle-mounted Niagara systems ignite at launch"),Jets.Num(),2))
 {
  TestTrue(TEXT("Both nozzles use the layered Mk2 effect"),Jets[0]->GetAsset() && Jets[0]->GetAsset()==Jets[1]->GetAsset() && Jets[0]->GetAsset()->GetNumEmitters()>=4);
  const FVector Left=Jets[0]->GetRelativeLocation(), Right=Jets[1]->GetRelativeLocation();
  TestTrue(TEXT("Nozzles sit below the carriage on opposite sides"),Left.Z<0 && FMath::IsNearlyEqual(Left.Z,Right.Z,.05) && Left.Y*Right.Y<0);
  Tick(P->EjectionFlightSeconds);
  TestTrue(TEXT("The seat disappears at burnout"),Seat->bHiddenInGame);
  if (Scorch)
  {
   const float DuringFire=Scorch->K2_GetScalarParameterValue(TEXT("PaintFade"));
   TestTrue(TEXT("The paint keeps bleaching after the seat exhaust ends"),DuringFire>0.f && DuringFire<1.f && P->IsComponentTickEnabled());
   Tick(6.f);
   TestTrue(TEXT("Fully bleached paint remains on the cold guard"),Brow->GetOverlayMaterial()==Scorch &&
    Scorch->K2_GetScalarParameterValue(TEXT("PaintFade"))==1.f);
   TestFalse(TEXT("The settled wreck needs no presentation tick"),P->IsComponentTickEnabled());
  }
  for (const auto* Jet:Jets)
   TestTrue(TEXT("The detached exhaust can dissipate after the seat disappears"),!IsValid(Jet) || (!Jet->GetAttachParent() && !Jet->bHiddenInGame));
 }
 auto* Unboarded=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Class,FVector(600,0,0),FRotator::ZeroRotator);
 auto* Early=Unboarded->GetPresentation(); Early->StartBoarding(2.f);
 Early->TickComponent(.3f,LEVELTICK_All,nullptr);
 Early->StartSeatEjection();
 TestFalse(TEXT("An interrupted boarding sequence cannot flash a shield that never deployed"),Early->IsWaitingForShieldBreak());
 auto* Interrupted=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Class,FVector(-600,0,0),FRotator::ZeroRotator)->GetPresentation();
 auto* InterruptedSeat=Interrupted->GetPilotMesh()->GetAttachParent();
 Interrupted->StartBoarding(2.f);
 Interrupted->TickComponent(SeatedTime+.58f+.26f,LEVELTICK_All,nullptr);
 const FVector MidRail=InterruptedSeat->GetRelativeLocation();
 Interrupted->StartSeatEjection();
 TestTrue(TEXT("Interrupting docking preserves the occupied carriage position"),InterruptedSeat->GetRelativeLocation().Equals(MidRail,.05));
 Interrupted->TickComponent(.1f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("An already raised carriage reverses outward without replaying the hatch or lift"),
  InterruptedSeat->GetRelativeLocation().X>MidRail.X && FMath::IsNearlyEqual(InterruptedSeat->GetRelativeLocation().Z,MidRail.Z,.05));
 Interrupted->TickComponent(Interrupted->GetEjectionLaunchTime()-.1f,LEVELTICK_All,nullptr);
 TestNull(TEXT("Interrupted boarding launches from the same rail end"),InterruptedSeat->GetAttachParent());
 const FVector Launch=Interrupted->GetBodyRoot()->GetComponentTransform().InverseTransformPosition(InterruptedSeat->GetComponentLocation());
 TestTrue(TEXT("Interrupted ejection never adds the remaining boarding offset to the rail length"),
  Launch.Equals(FVector(Locked.X+Interrupted->EjectionSlideDistance,Locked.Y,Outer.Z),.05));
 auto* Low=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Class,FVector(0,600,0),FRotator(0,70,0))->GetPresentation();
 auto* LowSeat=Low->GetPilotMesh()->GetAttachParent();
 Low->StartBoarding(2.f);
 Low->TickComponent(ClimbStart+1.9f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Rotated mech receives the pilot in its lowered chair"),Low->GetPilotMesh()->GetAttachParent()==LowSeat && LowSeat->GetRelativeLocation().Equals(Pickup,.05));
 Low->StartSeatEjection();
 Low->TickComponent(.24f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Interrupting pickup lifts vertically from its actual height before release"),
  FMath::IsNearlyEqual(LowSeat->GetRelativeLocation().X,Pickup.X,.05) && LowSeat->GetRelativeLocation().Z>Pickup.Z+10 && LowSeat->GetRelativeLocation().Z<Outer.Z-10);
 return true;
}
#endif
