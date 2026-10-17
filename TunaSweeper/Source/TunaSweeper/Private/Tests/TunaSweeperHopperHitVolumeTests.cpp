#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperArmModule.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Weapon/TunaSweeperWeapon.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "TunaSweeperCollisionChannels.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperHitVolumeTest, "TunaSweeper.Hopper.Combat.HitVolumesAndScopedAim",
 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHopperHitVolumeTest::RunTest(const FString& Parameters)
{
 auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
  .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false).EnableTraceCollision(true);
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,GetTransientPackage(),true,ERHIFeatureLevel::Num,&Values);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
 ON_SCOPE_EXIT { W->DestroyWorld(false); GEngine->DestroyWorldContext(W); W->RemoveFromRoot(); };
 FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 auto* H=W->SpawnActor<ATunaSweeperHopperEnemyCharacter>(FVector(600,0,105),FRotator::ZeroRotator,Spawn);
 H->SetArmModule(true,AHopperArmModule::StaticClass()); H->SetArmModule(false,AHopperArmModule::StaticClass());
 TArray<UBoxComponent*> Boxes; H->GetComponents(Boxes);
 Boxes.RemoveAll([](const UBoxComponent* B){return !B->ComponentHasTag(TEXT("HopperMechHitBox"));});
 TestTrue(TEXT("Mech has separate part hit volumes"),Boxes.Num()>20);
 TestEqual(TEXT("Movement capsule radius remains unchanged"),H->GetCapsuleComponent()->GetUnscaledCapsuleRadius(),65.f);
 auto* Source=W->SpawnActor<APawn>();
 auto* Faction=NewObject<UTunaSweeperFactionComponent>(Source); Source->AddInstanceComponent(Faction);
 Faction->SetFactionId(TunaSweeperFactionIds::Player); Faction->RegisterComponent();
 UBoxComponent* ArmBox=nullptr;
 for(auto* Box:Boxes)
 {
  TestEqual(TEXT("Hit volume does not obstruct player movement"),Box->GetCollisionResponseToChannel(ECC_Pawn),ECR_Ignore);
  TestEqual(TEXT("Part accepts cursor trace"),Box->GetCollisionResponseToChannel(ECC_Visibility),ECR_Block);
  TestEqual(TEXT("Part accepts projectiles"),Box->GetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile),ECR_Block);
  if(Box->GetAttachParent() && Cast<AHopperArmModule>(Box->GetAttachParent()->GetOwner())) ArmBox=Box;
 }
 TestNotNull(TEXT("Arm hit volume belongs to enemy and follows runtime arm"),ArmBox);
 auto Fire=[&](AActor* Target,UPrimitiveComponent* Part,const FVector& Point)
 {
  auto* Gun=W->SpawnActor<ATunaSweeperWeapon>();
  TestTrue(TEXT("Weapon fires"),Gun->FireWithAimIntent(FVector::ForwardVector,Source,NAME_None,NAME_None,NAME_None,
   1.f,0,0.f,FVector(900,0,40),true,Target,Part,Point,true,0.f,true));
  for(TActorIterator<ATunaSweeperProjectile> It(W);It;++It) if(It->GetOwner()==Gun)
  {
   // This isolated world never begins play; perform the engine's normal local-to-world velocity initialization.
   auto* Movement=It->FindComponentByClass<UProjectileMovementComponent>();
   if(!Movement->HasBeenInitialized()) Movement->InitializeComponent();
   return *It;
  }
  return static_cast<ATunaSweeperProjectile*>(nullptr);
 };
 const FVector TargetPoint=ArmBox?ArmBox->GetComponentLocation():FVector(600,80,160);
 auto* Shot=Fire(H,ArmBox,TargetPoint);
 if(TestNotNull(TEXT("Mech-directed projectile spawned"),Shot))
 {
  const FVector Expected=(TargetPoint-Shot->GetActorLocation()).GetSafeNormal();
  TestTrue(TEXT("Only mech target uses the cursor hit height and XY"),Shot->FindComponentByClass<UProjectileMovementComponent>()->Velocity.GetSafeNormal().Equals(Expected,.001f));
  if(ArmBox)
  {
   FHitResult Hit;
   const FVector End=Shot->GetActorLocation()+Expected*2000.f;
   TestTrue(TEXT("Actual shot ray intersects mech"),W->LineTraceSingleByChannel(Hit,Shot->GetActorLocation(),End,TunaSweeperCollisionChannels::Projectile));
   TestEqual(TEXT("Physics resolves damage to the mech owner"),Hit.GetActor(),static_cast<AActor*>(H));
   auto* Sphere=Cast<USphereComponent>(Shot->GetRootComponent());
   const float Before=H->GetMechHealth();
   FEditorScriptExecutionGuard Guard;
   Sphere->OnComponentHit.Broadcast(Sphere,H,ArmBox,FVector::ZeroVector,FHitResult(H,ArmBox,TargetPoint,FVector::UpVector));
   TestTrue(TEXT("Arm hit damages mech durability"),H->GetMechHealth()<Before);
  }
 }
 auto* Normal=W->SpawnActor<ATunaSweeperEnemyCharacter>(FVector(600,400,88),FRotator::ZeroRotator,Spawn);
 auto* NormalShot=Fire(Normal,Normal->GetCapsuleComponent(),FVector(600,400,160));
 if(TestNotNull(TEXT("Ordinary projectile spawned"),NormalShot))
  TestTrue(TEXT("Ordinary targets retain horizontal shots"),FMath::IsNearlyZero(NormalShot->FindComponentByClass<UProjectileMovementComponent>()->Velocity.Z));
 H->TakeDamage(10000,FDamageEvent(),nullptr,nullptr);
 for(auto* Box:Boxes) TestEqual(TEXT("Wreck hit volumes are disabled"),Box->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
 return true;
}
#endif
