#if WITH_DEV_AUTOMATION_TESTS
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
#include "AI/Hopper/HopperArmModule.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Editor.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Tests/AutomationEditorCommon.h"
#include "Tests/AutomationCommon.h"
#include "UnrealClient.h"

namespace HopperPlayTest
{
 FVector WalkStart;
 ATunaSweeperHopperEnemyCharacter* Find(UWorld* W)
 {
  if (W) for(TActorIterator<ATunaSweeperHopperEnemyCharacter> I(W);I;++I) return *I;
  return nullptr;
 }
 void Capture(FAutomationTestBase* Test,const FString& Name)
 {
  FViewport* V=GEditor->GetPIEViewport(); if (!V) return;
  TArray<FColor> Pixels;
  if (!Test->TestTrue(TEXT("Hopper viewport pixels"),V->ReadPixels(Pixels))) return;
  const FIntPoint Size=V->GetSizeXY(); TArray64<uint8> Png;
  FImageUtils::PNGCompressImageArray(Size.X,Size.Y,TArrayView64<const FColor>(Pixels.GetData(),Pixels.Num()),Png);
  const FString Dir=FPaths::ProjectSavedDir()/TEXT("Screenshots/Hopper");
  IFileManager::Get().MakeDirectory(*Dir,true);
  Test->TestTrue(TEXT("Hopper capture saved"),FFileHelper::SaveArrayToFile(Png,*(Dir/Name)));
 }
 void Camera(UWorld* World,AActor* Target,float Distance)
 {
  APlayerController* PC=World->GetFirstPlayerController(); if(!PC) return;
  ACameraActor* C=World->SpawnActor<ACameraActor>();
  const FVector Look=Target->GetActorLocation()+FVector(0,0,20);
  const FVector Pos=Look+FVector(-Distance,-Distance*.8,Distance*.5);
  C->SetActorLocation(Pos); C->SetActorRotation((Look-Pos).Rotation()); PC->SetViewTarget(C);
 }
}
// PIE loading and navigation generation can outlast a wall-clock wait during a test suite.
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FHopperWaitForReady, FAutomationTestBase*, Test);
bool FHopperWaitForReady::Update()
{
 UWorld* World=GEditor->PlayWorld;
 ATunaSweeperHopperEnemyCharacter* Hopper=HopperPlayTest::Find(World);
 UNavigationSystemV1* Navigation=World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
 FNavLocation Start, Destination;
 const FVector Feet=Hopper ? Hopper->GetNavAgentLocation() : FVector::ZeroVector;
 const bool bReady=Hopper && Hopper->GetCombatPhase()==EHopperCombatPhase::Mech && Navigation
  && !Navigation->IsNavigationBuildInProgress()
  && Navigation->ProjectPointToNavigation(Feet,Start,FVector(2,2,180))
  && Navigation->ProjectPointToNavigation(Feet+Hopper->GetActorForwardVector()*150.f,Destination,FVector(2,2,180));
 if(bReady) return true;
 if(FPlatformTime::Seconds()-StartTime < 30.f) return false;
 Test->AddError(FString::Printf(TEXT("Hopper PIE did not become ready: world=%s phase=%d navigation=%s building=%d feet=%s gameTime=%.2f"),
  *GetNameSafe(World),Hopper ? static_cast<int32>(Hopper->GetCombatPhase()) : -1,*GetNameSafe(Navigation),
  Navigation ? Navigation->IsNavigationBuildInProgress() : false,*Feet.ToCompactString(),World ? World->GetTimeSeconds() : 0.f));
 return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FHopperWaitForWalk, FAutomationTestBase*, Test);
bool FHopperWaitForWalk::Update()
{
 UWorld* World=GEditor->PlayWorld;
 ATunaSweeperHopperEnemyCharacter* Hopper=HopperPlayTest::Find(World);
 if(Hopper && FVector::Dist2D(HopperPlayTest::WalkStart,Hopper->GetActorLocation())>25.f) return true;
 if(FPlatformTime::Seconds()-StartTime < 15.f) return false;
 Test->AddError(TEXT("Hopper did not move 25 cm along its navigation request within 15 seconds"));
 return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FHopperWaitForDismount, FAutomationTestBase*, Test);
bool FHopperWaitForDismount::Update()
{
 ATunaSweeperHopperEnemyCharacter* Hopper=HopperPlayTest::Find(GEditor->PlayWorld);
 if(Hopper && Hopper->GetCombatPhase()==EHopperCombatPhase::PilotRanged) return true;
 if(FPlatformTime::Seconds()-StartTime < 15.f) return false;
 Test->AddError(TEXT("Hopper did not finish a safe dismount within 15 seconds"));
 return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FHopperPlayStep,FAutomationTestBase*,Test,int32,Step);
bool FHopperPlayStep::Update()
{
 UWorld* W=GEditor->PlayWorld;
 ATunaSweeperHopperEnemyCharacter* Hopper=HopperPlayTest::Find(W);
 if(!Test->TestNotNull(TEXT("Live Hopper exists"),Hopper)) return true;
 auto* P=Hopper->GetPresentation();
 if(Step==0)
 {
  Test->TestEqual(TEXT("Boarding completes in the live world"),Hopper->GetCombatPhase(),EHopperCombatPhase::Mech);
  Test->TestNotNull(TEXT("Left weapon attached"),Hopper->GetArmModule(true));
  Test->TestNotNull(TEXT("Right weapon attached"),Hopper->GetArmModule(false));
  Test->TestNotNull(TEXT("Pilot mesh imported"),P->GetPilotMesh()->GetSkeletalMeshAsset());
  TArray<UStaticMeshComponent*> MeshParts; Hopper->GetComponents(MeshParts);
  int32 HopperParts=0;
  for(UStaticMeshComponent* Part:MeshParts)
   if(Part->GetStaticMesh() && Part->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Game/Characters/Hopper/"))) ++HopperParts;
  Test->TestEqual(TEXT("Exactly one generated assembly after map load and PIE duplication"),HopperParts,62);
  TArray<USkeletalMeshComponent*> Pilots; Hopper->GetComponents(Pilots);
  int32 PilotCount=0;
  for(USkeletalMeshComponent* Pilot:Pilots) if(Pilot->GetSkeletalMeshAsset()==P->GetPilotMesh()->GetSkeletalMeshAsset()) ++PilotCount;
  Test->TestEqual(TEXT("Exactly one rabbit after map load and PIE duplication"),PilotCount,1);
  Test->TestTrue(TEXT("Pilot follows cockpit"),P->GetPilotMesh()->GetAttachParent()!=Hopper->GetRootComponent());
  // Close-up capture uses an external camera; visibility linkage is tested separately.
  Hopper->FindComponentByClass<UTunaSweeperVisionSubjectComponent>()->SetVisionVisibilityEnabled(false);
  HopperPlayTest::Camera(W,Hopper,310);
  USceneComponent* BodyBefore=P->GetBodyRoot();
  Hopper->RerunConstructionScripts();
  Test->TestEqual(TEXT("Live property reconstruction preserves current presentation"),P->GetBodyRoot(),BodyBefore);
  Test->TestTrue(TEXT("Live reconstruction preserves registered body"),IsValid(BodyBefore) && BodyBefore->IsRegistered());
 }
 else if(Step==6)
 {
  HopperPlayTest::WalkStart=Hopper->GetActorLocation();
  if(AAIController* AI=Cast<AAIController>(Hopper->GetController()))
  {
   AI->SetActorTickEnabled(false);
   const auto Move=AI->MoveToLocation(HopperPlayTest::WalkStart+Hopper->GetActorForwardVector()*150.f,5.f,false);
   Test->TestTrue(TEXT("Live mech accepts navigation movement"),Move!=EPathFollowingRequestResult::Failed);
  }
 }
 else if(Step==1)
 {
  Test->TestTrue(TEXT("Heavy mech walks in the live navigation world"),FVector::Dist2D(HopperPlayTest::WalkStart,Hopper->GetActorLocation())>25.f);
  if(Hopper->GetController()) Hopper->GetController()->SetActorTickEnabled(true);
  HopperPlayTest::Capture(Test,TEXT("01_Mounted.png"));
  W->GetFirstPlayerController()->ConsoleCommand(TEXT("viewmode unlit"));
 }
 else if(Step==5)
 {
  HopperPlayTest::Capture(Test,TEXT("01_Mounted_Unlit.png"));
  W->GetFirstPlayerController()->ConsoleCommand(TEXT("viewmode lit"));
  UGameplayStatics::ApplyDamage(Hopper,10000,nullptr,nullptr,nullptr);
  Test->TestFalse(TEXT("Destroyed mech does not kill pilot"),Hopper->IsDead());
 }
 else if(Step==2)
 {
  Test->TestEqual(TEXT("Navigation permits live safe dismount"),Hopper->GetCombatPhase(),EHopperCombatPhase::PilotRanged);
  Test->TestEqual(TEXT("Pilot capsule owns on-foot presentation"),P->GetPilotMesh()->GetAttachParent(),Hopper->GetRootComponent());
  HopperPlayTest::Camera(W,Hopper,140);
 }
 else if(Step==3)
 {
  HopperPlayTest::Capture(Test,TEXT("02_PilotRanged.png"));
  if(Hopper->GetController()) { Hopper->GetController()->UnPossess(); }
  Hopper->GetCharacterMovement()->StopMovementImmediately();
  FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  ATunaSweeperEnemyCharacter* Target=W->SpawnActor<ATunaSweeperEnemyCharacter>(Hopper->GetActorLocation()+FVector(400,0,0),FRotator::ZeroRotator,Params);
  if(!Test->TestNotNull(TEXT("Ammo test target"),Target)) return true;
  if(Target->GetController()) Target->GetController()->UnPossess();
  Target->GetFactionComponent()->SetFactionId(TunaSweeperFactionIds::Player);
  Target->GetCharacterMovement()->DisableMovement();
  Hopper->SetActorRotation(FRotator::ZeroRotator);
  Hopper->PilotShotCooldown=0;
  const int32 Count=Hopper->GetPilotAmmo();
  for(int32 I=0;I<Count;++I)
   Test->TestEqual(TEXT("Each finite round creates a real shot"),Hopper->TryFireProjectileAt(Target),ETunaSweeperEnemyFireResult::Fired);
  Test->TestEqual(TEXT("Actual shots exhaust pilot ammo"),Hopper->GetPilotAmmo(),0);
  Test->TestEqual(TEXT("Last shot enters melee phase"),Hopper->GetCombatPhase(),EHopperCombatPhase::PilotMelee);
  Test->TestTrue(TEXT("Existing AI sees melee mode"),static_cast<ATunaSweeperEnemyCharacter*>(Hopper)->UsesMeleeAttack());
  Target->Destroy();
  P->PlayPilotMelee();
 }
 else if(Step==4)
 {
  HopperPlayTest::Capture(Test,TEXT("03_PilotMelee.png"));
 }
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperPlayablePresentationTest,"TunaSweeper.Hopper.Play.PresentationAndPhases",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHopperPlayablePresentationTest::RunTest(const FString& Parameters)
{
 if(!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir()/TEXT("Maps/HopperCombatTestMap.umap"),false,true))return false;
 FRequestPlaySessionParams Play;
 Play.EditorPlaySettings=NewObject<ULevelEditorPlaySettings>(GetTransientPackage());
 Play.EditorPlaySettings->NewWindowWidth=1280;
 Play.EditorPlaySettings->NewWindowHeight=800;
 GEditor->RequestPlaySession(Play);
 ADD_LATENT_AUTOMATION_COMMAND(FHopperWaitForReady(this));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperPlayStep(this,0));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperPlayStep(this,6));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperWaitForWalk(this));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperPlayStep(this,1));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.3f));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperPlayStep(this,5));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperWaitForDismount(this));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperPlayStep(this,2));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.4f));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperPlayStep(this,3));
 ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.3f));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperPlayStep(this,4));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
 return true;
}
#endif
