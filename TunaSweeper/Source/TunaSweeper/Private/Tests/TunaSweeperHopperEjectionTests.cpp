#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/WorldSettings.h"
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
#include "Animation/AnimSequence.h"
#if WITH_EDITOR
#include "Engine/SkeletalMesh.h"
#include "SkinnedAssetCompiler.h"
#endif
#include "Animation/AnimSingleNodeInstance.h"
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
 const FTransform OpenDoor=Door->GetRelativeTransform(), OpenStep=Step->GetRelativeTransform();
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
 constexpr float Pull=1.f;
 const float InitialCracks=Shader(2);
 TestTrue(TEXT("Defeat immediately cracks the deployed field as the approach begins"),
  P->IsWaitingForShieldBreak() && !Shield->bHiddenInGame && Shader(0)==1.f && InitialCracks>0.f && Shader(3)==0.f);
 float Elapsed=0.f;
 auto EjectTo=[&](float Time) { Tick(Time-Elapsed); Elapsed=Time; };
 bool bBright=false, bIntact=true;
 int32 Peaks=0, Troughs=0;
 float FirstPeak=-1.f, LastPeak=-1.f;
 float MinGain=Shader(1), MaxGain=Shader(1);
 for (int32 Frame=1;Frame<50;++Frame)
 {
  EjectTo(Frame/50.f);
  const float Gain=Shader(1);
  MinGain=FMath::Min(MinGain,Gain);
  MaxGain=FMath::Max(MaxGain,Gain);
  if (!bBright && Gain>1.f)
  {
   if (Peaks++==0) FirstPeak=Elapsed;
   LastPeak=Elapsed;
   bBright=true;
  }
  else if (bBright && Gain<.45f) { ++Troughs; bBright=false; }
  bIntact &= P->IsWaitingForShieldBreak() && !Shield->bHiddenInGame && Shader(2)>0.f && Shader(3)==0.f &&
   P->GetHatchOpenAlpha()==0.f && Brow->GetAttachParent()==P->GetBodyRoot() && Seat->GetAttachParent()==P->GetBodyRoot();
 }
 TestTrue(FString::Printf(TEXT("The approach flashes repeatedly (peaks %d, troughs %d, first %.2f, last %.2f, gain %.2f to %.2f)"),
  Peaks,Troughs,FirstPeak,LastPeak,MinGain,MaxGain),Peaks>=2 && Troughs>=2 && FirstPeak<.35f && LastPeak>.65f && MaxGain-MinGain>.8f);
 TestTrue(TEXT("The cracked field, occupied seat and guard stay in place throughout the warning"),bIntact);
 TestTrue(TEXT("Cracking spreads across the intact field by the last pull frames"),Shader(2)>.9f && Shader(2)>InitialCracks);
 TestNull(TEXT("Guard paint stays clean throughout the shield warning"),Brow->GetOverlayMaterial());
 auto* Flash=Hopper->FindComponentByTag<UMaterialBillboardComponent>(TEXT("HopperBlastFlash"));
 auto* FlashMaterial=Flash ? Cast<UMaterialInstanceDynamic>(Flash->GetMaterial(0)) : nullptr;
 TestTrue(TEXT("The queued blast core is dark throughout the shield warning"),
  FlashMaterial && FlashMaterial->K2_GetScalarParameterValue(TEXT("FlashGain"))==0.f);
 EjectTo(Pull-.001f);
 auto SeatJets=[](const USceneComponent* Carriage)
 {
  TArray<UNiagaraComponent*> Jets;
  Carriage->GetOwner()->GetComponents(Jets);
  Jets.RemoveAll([Carriage](const UNiagaraComponent* Jet){ return Jet->GetAttachParent()!=Carriage; });
  return Jets;
 };
 TestTrue(TEXT("Immediately before full pull the shield is intact, the guard is bolted and the seat has no exhaust"),
  P->IsWaitingForShieldBreak() && Shader(3)==0.f && Brow->GetAttachParent()==P->GetBodyRoot() &&
  Seat->GetAttachParent()==P->GetBodyRoot() && Seat->GetRelativeLocation().Equals(Locked,.01) && SeatJets(Seat).IsEmpty() &&
  BurstCount(TEXT("NS_HopperMk2SeatEjectSparks"))==0 && BurstCount(TEXT("NS_HopperMk2RadialBlast"))==0);
 EjectTo(Pull);
 TestTrue(TEXT("The full-pull update extinguishes the field and begins its fragments"),
  !P->IsWaitingForShieldBreak() && Shader(1)==0.f && Shader(3)>=1.f && !Shield->bHiddenInGame);
 auto* Scorch=Cast<UMaterialInstanceDynamic>(Door->GetOverlayMaterial());
 auto* GuardSoot=Cast<UMaterialInstanceDynamic>(Brow->GetOverlayMaterial());
 TestNotNull(TEXT("Dry blast soot appears on the shatter frame"),Scorch);
 TestNotNull(TEXT("The breakaway frame receives the same initial blast soot"),GuardSoot);
 auto* LeftArm=Part(TEXT("GuardArm_L"));
 auto* RightArm=Part(TEXT("GuardArm_R"));
 auto* Mount=Part(TEXT("GuardMount_L"));
 auto* ReleasedBody=Cast<UPrimitiveComponent>(Brow->GetAttachmentRoot());
 TestTrue(TEXT("The brow and both side arms leave as one rigid frame on shield breakup"),
  ReleasedBody && ReleasedBody->GetOwner()!=Hopper && !Brow->IsAttachedTo(P->GetBodyRoot())
  && LeftArm && LeftArm->GetAttachmentRoot()==ReleasedBody && RightArm && RightArm->GetAttachmentRoot()==ReleasedBody
  && Mount && Mount->GetAttachParent()==P->GetBodyRoot());
 TestTrue(TEXT("The discarded frame collides with the environment independently of the defeated enemy"),
  ReleasedBody && ReleasedBody->GetCollisionEnabled()==ECollisionEnabled::PhysicsOnly &&
  ReleasedBody->GetCollisionResponseToChannel(ECC_WorldStatic)==ECR_Block && ReleasedBody->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Ignore);
 TestTrue(TEXT("The occupied seat detaches at its socket on the same update as the guard and shards"),
  !Seat->GetAttachParent() && Pilot->GetAttachParent()==Seat && Pilot->GetComponentLocation().Equals(Seated,.05));
 const TArray<UNiagaraComponent*> Jets=SeatJets(Seat);
 TestEqual(TEXT("Both nozzle-mounted exhausts ignite on the full-pull update"),Jets.Num(),2);
 TestEqual(TEXT("Both seat locks flash on the full-pull update"),BurstCount(TEXT("NS_HopperMk2SeatEjectSparks")),2);
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
 for (const TCHAR* Effect:{TEXT("NS_HopperMk2RadialBlast"),TEXT("NS_HopperMk2MetalDebris"),TEXT("NS_HopperMk2OilSpray")})
  TestEqual(FString::Printf(TEXT("%s starts once on the shield break"),Effect),BurstCount(Effect),1);
 TestEqual(TEXT("Sustained fire waits for the pilot to escape"),AttachedFires()+BurstCount(TEXT("NS_HopperMk2HullFire")),0);
 TestEqual(TEXT("The Mk2 replaces the common rubble explosion"),BurstCount(TEXT("NS_Explosion_SmokeRobot")),0);
 P->StartDestructionEffects(nullptr);
 EjectTo(Pull+.04f);
 const FVector Rising=Pilot->GetComponentLocation()-Seated;
 TestTrue(TEXT("The seat is already rising straight up and the front hatch is opening 40 ms after the blast"),
  !Seat->GetAttachParent() && Rising.Size2D()<.1 && Rising.Z>2 && P->GetHatchOpenAlpha()>0 && P->GetHatchOpenAlpha()<1);
 TestEqual(TEXT("A repeated destruction signal cannot replay the blast"),BurstCount(TEXT("NS_HopperMk2RadialBlast")),1);
 TestTrue(TEXT("The occupied cabin ejects with the pilot"),Cabin->GetAttachParent()==Seat && Pilot->GetAttachParent()==Seat);
 if (Scorch) TestEqual(TEXT("The escape starts with first-stage soot"),Scorch->K2_GetScalarParameterValue(TEXT("PaintFade")),0.f);
 TestEqual(TEXT("The seat visibly clears the wreck before it ignites"),AttachedFires()+BurstCount(TEXT("NS_HopperMk2HullFire")),0);
 EjectTo(Pull+.34f);
 TestEqual(TEXT("Only the empty chassis catches fire"),AttachedFires(),1);
 EjectTo(HatchDuration+.002f);
 TestTrue(TEXT("The front door and telescoping ladder finish their authored opening stroke independently"),
  Door->GetRelativeTransform().Equals(OpenDoor,.05) && Step->GetRelativeTransform().Equals(OpenStep,.05));
 TestTrue(TEXT("The pilot continues ascending on the original world-vertical line"),
  FVector::Dist2D(Seated,Pilot->GetComponentLocation())<.1 && Pilot->GetComponentLocation().Z>Seated.Z+100);
 EjectTo(FMath::Max(Elapsed,Pull+.8f));
 TestNull(TEXT("The blast core has expired"),Hopper->FindComponentByTag<UMaterialBillboardComponent>(TEXT("HopperBlastFlash")));
 TestTrue(TEXT("No lingering shield or burst on the wreck"),Shield->bHiddenInGame);
 if (Jets.Num()==2)
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
   TestTrue(TEXT("Fully bleached paint remains on the cold front hatch"),Door->GetOverlayMaterial()==Scorch &&
    Scorch->K2_GetScalarParameterValue(TEXT("PaintFade"))==1.f);
   if (GuardSoot) TestEqual(TEXT("The discarded frame keeps its blast soot outside the hull fire"),
    GuardSoot->K2_GetScalarParameterValue(TEXT("PaintFade")),0.f);
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
 const auto* InactiveField=Interrupted->GetOwner()->FindComponentByTag<UStaticMeshComponent>(TEXT("HopperCockpitShield"));
 if (!TestNotNull(TEXT("Interrupted Mk2 has the unpowered field mesh"),InactiveField)) return false;
 auto FieldIsOff=[InactiveField]()
 {
  const auto& Channels=InactiveField->GetCustomPrimitiveData().Data;
  return InactiveField->bHiddenInGame && Channels.Num()>=4 && Channels[2]==0.f && Channels[3]==0.f;
 };
 Interrupted->StartBoarding(2.f);
 Interrupted->TickComponent(SeatedTime+.58f+.26f,LEVELTICK_All,nullptr);
 const FVector MidRail=InterruptedSeat->GetRelativeLocation();
 Interrupted->StartSeatEjection();
 TestTrue(TEXT("Interrupting docking preserves the occupied carriage position"),InterruptedSeat->GetRelativeLocation().Equals(MidRail,.05));
 Interrupted->TickComponent(Pull-.002f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Interrupted boarding holds the carriage for the entire emergency pull"),
  InterruptedSeat->GetAttachParent()==Interrupted->GetBodyRoot() && InterruptedSeat->GetRelativeLocation().Equals(MidRail,.05) &&
  !Interrupted->IsWaitingForShieldBreak() && FieldIsOff());
 Interrupted->TickComponent(.002f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Interrupted boarding launches with exhaust at full pull without illuminating or fragmenting the unpowered shield"),
  !InterruptedSeat->GetAttachParent() && SeatJets(InterruptedSeat).Num()==2 && !Interrupted->IsWaitingForShieldBreak() && FieldIsOff());
 Interrupted->TickComponent(.23f,LEVELTICK_All,nullptr);
 const FVector Launch=Interrupted->GetBodyRoot()->GetComponentTransform().InverseTransformPosition(InterruptedSeat->GetComponentLocation());
 TestTrue(TEXT("The seat launches from its actual partway-in position"),FVector::Dist2D(Launch,MidRail)<.1 && Launch.Z>MidRail.Z+15);
 auto* Low=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Class,FVector(0,600,0),FRotator(0,70,0))->GetPresentation();
 auto* LowSeat=Low->GetPilotMesh()->GetAttachParent();
 Low->StartBoarding(2.f);
 Low->TickComponent(ClimbStart+1.9f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Rotated mech receives the pilot in its lowered chair"),Low->GetPilotMesh()->GetAttachParent()==LowSeat && LowSeat->GetRelativeLocation().Equals(Pickup,.05));
 Low->GetBodyRoot()->SetWorldRotation(FRotator(12,70,9));
 const FVector InclinedPickup=LowSeat->GetComponentLocation();
 Low->StartSeatEjection();
 Low->TickComponent(Pull+.001f,LEVELTICK_All,nullptr);
 Low->TickComponent(.24f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("An interrupted low pickup on an inclined mech ejects along world up"),
  !LowSeat->GetAttachParent() && FVector::Dist2D(LowSeat->GetComponentLocation(),InclinedPickup)<.1
  && LowSeat->GetComponentLocation().Z>InclinedPickup.Z+20);
 auto* SlowFrame=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Class,FVector(0,-600,0),FRotator::ZeroRotator)->GetPresentation();
 SlowFrame->SeatPilot(); SlowFrame->SetMechActive(true);
 SlowFrame->TickComponent(1.f,LEVELTICK_All,nullptr);
 SlowFrame->StartSeatEjection();
 auto* SlowSeat=SlowFrame->GetPilotMesh()->GetAttachParent();
 const FVector SlowStart=SlowSeat->GetComponentLocation();
 const int32 BlastsBefore=BurstCount(TEXT("NS_HopperMk2RadialBlast")), StrikesBefore=BurstCount(TEXT("NS_HopperMk2SeatEjectSparks"));
 constexpr float Overshoot=.24f;
 SlowFrame->TickComponent(Pull+Overshoot,LEVELTICK_All,nullptr);
 const double FlightHeight=SlowFrame->EjectionUpSpeed*Overshoot+.5*SlowFrame->EjectionUpAcceleration*Overshoot*Overshoot;
 TestTrue(TEXT("A long update crossing full pull places the detached seat at its scheduled ballistic height"),
  !SlowFrame->IsWaitingForShieldBreak() && !SlowSeat->GetAttachParent() && FMath::IsNearlyEqual(SlowFrame->GetEjectionLaunchTime(),Pull,.001f) &&
  SlowSeat->GetComponentLocation().Equals(SlowStart+FVector::UpVector*FlightHeight,.1));
 TestTrue(TEXT("The crossing update emits one blast, one strike per latch, and both exhausts"),
  BurstCount(TEXT("NS_HopperMk2RadialBlast"))==BlastsBefore+1 && BurstCount(TEXT("NS_HopperMk2SeatEjectSparks"))==StrikesBefore+2 &&
  SeatJets(SlowSeat).Num()==2);
 SlowFrame->TickComponent(.14f,LEVELTICK_All,nullptr);
 TestTrue(TEXT("The following flight update cannot repeat the blast, latch strikes, or exhaust ignition"),
  BurstCount(TEXT("NS_HopperMk2RadialBlast"))==BlastsBefore+1 && BurstCount(TEXT("NS_HopperMk2SeatEjectSparks"))==StrikesBefore+2 &&
  SeatJets(SlowSeat).Num()==2);

 auto* SixtyHzMech=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Class,FVector(-600,-600,0),FRotator::ZeroRotator);
 auto* SixtyHz=SixtyHzMech->GetPresentation();
 auto* TestSeat=SixtyHz->GetPilotMesh()->GetAttachParent();
 const auto* Grip=SixtyHzMech->FindComponentByTag<UStaticMeshComponent>(TEXT("HopperEjectionHandle"));
 const auto* Field=SixtyHzMech->FindComponentByTag<UStaticMeshComponent>(TEXT("HopperCockpitShield"));
 TInlineComponentArray<UStaticMeshComponent*> Panels(SixtyHzMech);
 auto* Guard=Panels.FindByPredicate([](const UStaticMeshComponent* Panel){ return Panel->GetName().StartsWith(TEXT("GuardBrow")); });
 if (!TestTrue(TEXT("The 60 Hz fixture has its grip, shield and guard"),Grip && Field && Guard && Field->GetCustomPrimitiveData().Data.Num()>=4)) return false;
 SixtyHz->SeatPilot(); SixtyHz->SetMechActive(true);
 SixtyHz->TickComponent(1.f,LEVELTICK_All,nullptr);
 SixtyHz->StartSeatEjection();
 const FVector DownStop(7.04,-14.40,23.17);
 const int32 DebrisBefore=BurstCount(TEXT("NS_HopperMk2MetalDebris"));
 // Repeated float deltas reproduce the game clock rounding below 1.0 on the sixtieth update.
 constexpr float FrameTime=1.f/60.f;
 for (int32 Frame=1;Frame<=59;++Frame) SixtyHz->TickComponent(FrameTime,LEVELTICK_All,nullptr);
 const double RemainingPull=FVector::Dist(Grip->GetRelativeLocation(),DownStop);
 TestTrue(FString::Printf(TEXT("60 Hz frame 59 still has a small pull left (%.3f cm) and all parts attached"),RemainingPull),
  RemainingPull>.1 && RemainingPull<.8 && SixtyHz->IsWaitingForShieldBreak() && Field->GetCustomPrimitiveData().Data[3]==0.f &&
  TestSeat->GetAttachParent()==SixtyHz->GetBodyRoot() && (*Guard)->GetAttachParent()==SixtyHz->GetBodyRoot() &&
  BurstCount(TEXT("NS_HopperMk2MetalDebris"))==DebrisBefore);
 SixtyHz->TickComponent(FrameTime,LEVELTICK_All,nullptr);
 TestTrue(TEXT("60 Hz frame 60 bottoms the grip, shatters the shield, emits debris and releases the guard and occupied rocket seat"),
  Grip->GetRelativeLocation().Equals(DownStop,.02) && !SixtyHz->IsWaitingForShieldBreak() && Field->GetCustomPrimitiveData().Data[3]>=1.f &&
  (*Guard)->GetAttachParent() && (*Guard)->GetAttachParent()->GetOwner()!=SixtyHzMech && !TestSeat->GetAttachParent() &&
  SixtyHz->GetPilotMesh()->GetAttachParent()==TestSeat && SeatJets(TestSeat).Num()==2 && BurstCount(TEXT("NS_HopperMk2MetalDebris"))==DebrisBefore+1);

 auto* Fallback=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Class,FVector(600,600,0),FRotator::ZeroRotator)->GetPresentation();
 auto* GuardWithoutHandle=DuplicateObject<UHopperVisualData>(Hopper->VisualData.LoadSynchronous(),GetTransientPackage());
 GuardWithoutHandle->EjectionPull=FHopperEjectionPull();
 Fallback->Initialize(GuardWithoutHandle);
 auto* FallbackSeat=Fallback->GetPilotMesh()->GetAttachParent();
 Fallback->StartSeatEjection();
 TestTrue(TEXT("A guard-equipped seat without a handle retains its original 100 ms clearance"),
  Fallback->GetEjectionPullTriggerTime()==0.f && FMath::IsNearlyEqual(Fallback->GetEjectionLaunchTime(),.10f,.001f));
 Fallback->TickComponent(.09f,LEVELTICK_All,nullptr);
 TestEqual(TEXT("The fallback carriage remains docked during its guard clearance"),FallbackSeat->GetAttachParent(),Fallback->GetBodyRoot());
 Fallback->TickComponent(.02f,LEVELTICK_All,nullptr);
 TestNull(TEXT("The fallback carriage then detaches"),FallbackSeat->GetAttachParent());
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperEmergencyHandleTest,"TunaSweeper.Hopper.Combat.EmergencyHandle",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperEmergencyHandleTest::RunTest(const FString& Parameters)
{
 auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
  .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,
  MakeUniqueObjectName(GetTransientPackage(),UWorld::StaticClass(),TEXT("HopperHandleTest")),GetTransientPackage(),true,ERHIFeatureLevel::Num,&Values);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
 UClass* Mk2=LoadClass<ATunaSweeperHopperEnemyCharacter>(nullptr,TEXT("/Game/Characters/Hopper/BP_HopperEnemyMk2.BP_HopperEnemyMk2_C"));
 if (!TestNotNull(TEXT("Mk2 class loads"),Mk2)) return false;
#if WITH_EDITOR
 // This synchronous fixture evaluates fifty gameplay samples in one engine frame, including on a cold import.
 auto* Visual=Mk2->GetDefaultObject<ATunaSweeperHopperEnemyCharacter>()->VisualData.LoadSynchronous();
 if (!TestNotNull(TEXT("Mk2 presentation data loads"),Visual)) return false;
 auto* Captain=Visual->PilotMesh.LoadSynchronous();
 if (!TestNotNull(TEXT("The seated captain's mesh loads"),Captain)) return false;
 FSkinnedAssetCompilingManager::Get().FinishCompilation({Captain});
 for (auto* Clip:{Visual->PilotSeated.LoadSynchronous(),Visual->EjectionPull.PilotAnimation.LoadSynchronous()})
 {
  if (!TestNotNull(TEXT("The seated and ejection sequences load"),Clip)) return false;
  Clip->WaitOnExistingCompression();
 }
#endif
 auto* Original=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>();
 TestNull(TEXT("Original Hopper has its original seat"),Original->FindComponentByTag<UStaticMeshComponent>(TEXT("HopperEjectionHandle")));
 TestEqual(TEXT("Original Hopper adds no handle delay"),Original->GetPresentation()->GetEjectionPullTriggerTime(),0.f);
 TestEqual(TEXT("Original Hopper has no reaction-camera lead"),Original->GetPresentation()->GetEjectionPullStartTime(),0.f);
 for (bool bShielded:{true,false})
 {
  auto* Mech=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Mk2,FVector(bShielded ? 500 : -500,0,0),FRotator(0,76,0));
  auto* Stage=Mech->GetPresentation();
  auto* Pilot=Stage->GetPilotMesh();
  auto* Seat=Pilot->GetAttachParent();
  auto* Handle=Mech->FindComponentByTag<UStaticMeshComponent>(TEXT("HopperEjectionHandle"));
  if (!TestNotNull(TEXT("The Mk2 seat carries a movable emergency handle"),Handle)) return false;
  TestTrue(TEXT("The stowed grip belongs to the seat"),Handle->GetAttachParent()==Seat && Handle->GetStaticMesh());
  const FVector Ready(-2.14,-14.40,33.74), Pulled(7.04,-14.40,23.17);
  const FTransform Parked=Handle->GetRelativeTransform();
  TestTrue(TEXT("The handle is already parked beside the right glove in normal play"),Parked.GetLocation().Equals(Ready,.05));
  const float Lead=Stage->GetEjectionPullStartTime();
  TestTrue(TEXT("The medium shot gets 0.22 seconds before the clip starts"),FMath::IsNearlyEqual(Lead,.22f,.001f));
  TestTrue(TEXT("The loaded Mk2 trips the ejection at one second, on animation frame 39"),
   FMath::IsNearlyEqual(Stage->GetEjectionPullTriggerTime(),1.f,.001f));
  if (bShielded)
  {
   Stage->SeatPilot(); Stage->SetMechActive(true);
   Stage->TickComponent(1.2f,LEVELTICK_All,nullptr);
  }
  else Stage->StartBoarding(2.f);
  Stage->StartSeatEjection();
  TestTrue(TEXT("The launch is scheduled for full pull with either shield state"),
   FMath::IsNearlyEqual(Stage->GetEjectionLaunchTime(),1.f,.001f));
  TestEqual(TEXT("Only an already deployed shield begins warning on defeat"),Stage->IsWaitingForShieldBreak(),bShielded);
  // Surface point on the seated captain's right glove, expressed in the imported hand bone's frame.
  auto Palm=[Pilot,Seat]() { return Seat->GetComponentTransform().InverseTransformPosition(
   Pilot->GetSocketTransform(TEXT("hand_r")).TransformPosition(FVector(-2.0542,.8093,3.3000))); };
  Stage->TickComponent(.02f,LEVELTICK_All,nullptr);
  Pilot->TickAnimation(0.f,false); Pilot->RefreshBoneTransforms();
  const FQuat NeutralHead=Pilot->GetSocketQuaternion(TEXT("head"));
  const FVector RestPalm=Palm();
  Stage->TickComponent(Lead-.02f,LEVELTICK_All,nullptr);
  Pilot->TickAnimation(0.f,false); Pilot->RefreshBoneTransforms();
  TestTrue(TEXT("The ready pose and parked lever hold while the gameplay view blends to the cockpit"),
   FMath::IsNearlyZero(Pilot->GetPosition(),.001f) && NeutralHead.Equals(Pilot->GetSocketQuaternion(TEXT("head")),.002) &&
   RestPalm.Equals(Palm(),.1) && Handle->GetRelativeTransform().Equals(Parked,.04) && Stage->IsWaitingForShieldBreak()==bShielded);
  const FVector Forward=Seat->GetForwardVector(), Right=Seat->GetRightVector();
  double MinYaw=0., MaxYaw=0., MaxRestDrift=0., MaxContactGap=-1.;
  int32 MaxGapFrame=0;
  FVector WorstPalm=FVector::ZeroVector, WorstLever=FVector::ZeroVector, WorstComponentPalm=FVector::ZeroVector;
  FString WorstClip;
  float WorstClipTime=0.f;
  bool bLeverParked=true, bLeverLatched=true, bPawWaited=true, bCorrectWarning=true, bDockedUntilPull=true;
  FTransform Down;
  float PreviousTime=Lead;
  for (int32 Frame=1;Frame<=50;++Frame)
  {
   const float Now=Lead+Frame/50.f;
   Stage->TickComponent(Now-PreviousTime,LEVELTICK_All,nullptr);
   PreviousTime=Now;
   Pilot->TickAnimation(0.f,false); Pilot->RefreshBoneTransforms();
   const FTransform Lever=Handle->GetRelativeTransform();
   const FVector SeatPalm=Palm();
   const double Gap=FVector::Distance(SeatPalm,Lever.GetLocation());
   if (Frame==1)
   {
    auto* Instance=Pilot->GetSingleNodeInstance();
    auto* Clip=Instance ? Cast<UAnimSequence>(Instance->GetCurrentAsset()) : nullptr;
    if (!TestNotNull(TEXT("Defeat immediately starts the authored pull"),Clip)) return false;
    TestEqual(TEXT("The ejection uses the captain's Mk2 pull"),Clip->GetFName(),FName(TEXT("A_Captain_Mk2EjectionPull")));
    TestTrue(TEXT("The clip spans one second at 50 fps"),FMath::IsNearlyEqual(Clip->GetPlayLength(),1.f,.001f) &&
     FMath::IsNearlyEqual(Clip->GetSamplingFrameRate().AsDecimal(),50.,.001));
   }
   if (Frame<=19)
   {
    const FVector Look=(Pilot->GetSocketQuaternion(TEXT("head"))*NeutralHead.Inverse()).RotateVector(Forward);
    const double Yaw=FMath::RadiansToDegrees(FMath::Atan2(FVector::DotProduct(Look,Right),FVector::DotProduct(Look,Forward)));
    MinYaw=FMath::Min(MinYaw,Yaw); MaxYaw=FMath::Max(MaxYaw,Yaw);
    MaxRestDrift=FMath::Max(MaxRestDrift,FVector::Dist(SeatPalm,RestPalm));
    bPawWaited &= SeatPalm.Equals(RestPalm,.4);
   }
   if (Frame<=30) bLeverParked &= Lever.Equals(Parked,.04);
   bCorrectWarning &= Stage->IsWaitingForShieldBreak()==(bShielded && Frame<39);
   if (Frame<39) bDockedUntilPull &= Seat->GetAttachParent()==Stage->GetBodyRoot();
   if (Frame==27) TestTrue(TEXT("The reaching glove has not made contact one frame early"),Gap>.45);
   if (Frame>=28 && Frame<=42 && Gap>MaxContactGap)
   {
    MaxContactGap=Gap; MaxGapFrame=Frame;
    WorstPalm=SeatPalm; WorstLever=Lever.GetLocation();
    WorstComponentPalm=Pilot->GetSocketTransform(TEXT("hand_r"),RTS_Component).TransformPosition(FVector(-2.0542,.8093,3.3000));
    auto* Instance=Pilot->GetSingleNodeInstance();
    WorstClip=Instance ? GetNameSafe(Instance->GetCurrentAsset()) : TEXT("None");
    WorstClipTime=Pilot->GetPosition();
   }
   if (Frame==31) TestTrue(TEXT("The lever leaves the ready stop on the first downstroke frame"),
    FVector::Distance(Lever.GetLocation(),Ready)>.25);
   if (Frame==35) TestTrue(TEXT("The handle moves progressively after contact"),
    Lever.GetLocation().Z<Ready.Z-2 && Lever.GetLocation().Z>Pulled.Z+2);
   if (Frame==38) TestTrue(TEXT("The handle has not hit the down stop one frame early"),
    Lever.GetLocation().Z>Pulled.Z+.25);
   if (Frame==39)
   {
    Down=Lever;
    TestTrue(TEXT("The lever reaches its down stop on the trigger frame"),Lever.GetLocation().Equals(Pulled,.08));
    TestNull(TEXT("The rising assembly detaches on the same animation frame that the lever bottoms out"),Seat->GetAttachParent());
   }
   if (Frame>39) bLeverLatched &= Lever.Equals(Down,.04);
  }
  AddInfo(FString::Printf(TEXT("Emergency pull shielded=%d yaw=[%.3f,%.3f] restPalm=%s earlyDrift=%.4f cm maxGap=%.4f cm frame=%d clip=%s t=%.4f"),
   bShielded,MinYaw,MaxYaw,*RestPalm.ToString(),MaxRestDrift,MaxContactGap,MaxGapFrame,*WorstClip,WorstClipTime));
  if (MaxContactGap>=.35 || !bPawWaited || MinYaw>=-12. || MaxYaw<=12.)
   AddInfo(FString::Printf(TEXT("Contact pose seatPalm=%s seatGrip=%s componentPalm=%s pilotParent=%s testSeat=%s pilotRelative=%s postProcess=%s"),
    *WorstPalm.ToString(),*WorstLever.ToString(),*WorstComponentPalm.ToString(),*GetNameSafe(Pilot->GetAttachParent()),
    *GetNameSafe(Seat),*Pilot->GetRelativeTransform().ToString(),*GetNameSafe(Pilot->GetPostProcessInstance())));
  TestTrue(FString::Printf(TEXT("The captain looks both ways before reaching (shielded=%d, yaw %.2f to %.2f)"),bShielded,MinYaw,MaxYaw),
   MinYaw<-12. && MaxYaw>12.);
  TestTrue(FString::Printf(TEXT("The right paw waits for the head turns (shielded=%d, maximum drift %.3f cm)"),bShielded,MaxRestDrift),bPawWaited);
  TestTrue(TEXT("The lever remains parked through frame 30"),bLeverParked);
  TestTrue(FString::Printf(TEXT("Glove contact from frame 28 through 42 (shielded=%d, maximum %.4f cm at frame %d)"),
   bShielded,MaxContactGap,MaxGapFrame),MaxContactGap<.35);
  TestTrue(TEXT("The handle stays latched down as the glove returns to its brace"),bLeverLatched && FVector::Dist(Palm(),Pulled)>4.);
  TestTrue(TEXT("The seat remains docked through every animation frame before full pull"),bDockedUntilPull);
  TestTrue(TEXT("Only the deployed shield warns, from defeat through the frame before full pull"),bCorrectWarning);
  TestTrue(TEXT("The grip leaves with the occupied seat on schedule"),!Seat->GetAttachParent() &&
   Pilot->GetAttachParent()==Seat && Handle->GetAttachParent()==Seat);
 }
 return true;
}
#endif
