#if WITH_DEV_AUTOMATION_TESTS
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
#include "AI/Hopper/HopperArmModule.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Component/TunaSweeperPlayerVisionComponent.h"
#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Camera/CameraActor.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Editor.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
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
 bool OpenNavigationFixture()
 {
  if (!FEditorFileUtils::LoadMap(FPaths::ProjectContentDir()/TEXT("Maps/HopperCombatTestMap.umap"),false,true)) return false;
  // PIE copies the editor's static tiles. Finish generation before duplicating a freshly opened level.
  auto* Navigation=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GEditor->GetEditorWorldContext().World());
  if (!Navigation) return false;
  Navigation->Build();
  return true;
 }
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
 auto* Viewer=World ? Cast<ATunaSweeperPlayerController>(World->GetFirstPlayerController()) : nullptr;
 if (Hopper && Viewer && Hopper->GetCombatPhase()==EHopperCombatPhase::WaitingForSight)
 {
  if(auto* Pawn=Cast<ATunaSweeperTopDownCharacter>(Viewer->GetPawn()))
  {
   const FVector Position=Hopper->GetActorLocation()+Hopper->GetActorRightVector()*300.f;
   Pawn->SetActorLocation(Position);
   Pawn->SetActorRotation((-Hopper->GetActorRightVector()).Rotation());
   Pawn->SetAimWorldPoint(Hopper->GetActorLocation());
   FVector2D Screen;
   if (Viewer->ProjectWorldLocationToScreen(Hopper->GetActorLocation(),Screen)) Viewer->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));
  }
 }
 UNavigationSystemV1* Navigation=World ? FNavigationSystem::GetCurrent<UNavigationSystemV1>(World) : nullptr;
 FNavLocation Start, Destination;
 const FVector Feet=Hopper ? Hopper->GetNavAgentLocation() : FVector::ZeroVector;
 const bool bReady=Hopper && Hopper->GetCombatPhase()==EHopperCombatPhase::Mech && (!Viewer || !Viewer->IsHopperShotActive()) && Navigation
  && !Navigation->IsNavigationBuildInProgress()
  && Navigation->ProjectPointToNavigation(Feet,Start,FVector(2,2,180))
  && Navigation->ProjectPointToNavigation(Feet+Hopper->GetActorForwardVector()*150.f,Destination,FVector(2,2,180));
 if(bReady) return true;
 if(FPlatformTime::Seconds()-StartTime < 30.f) return false;
 auto* Player=Viewer ? Cast<ATunaSweeperTopDownCharacter>(Viewer->GetPawn()) : nullptr;
 auto* Subject=Hopper ? Hopper->FindComponentByClass<UTunaSweeperVisionSubjectComponent>() : nullptr;
 const bool bSight=Player && Subject && Player->GetPlayerVisionComponent()->HasSightOf(*Subject);
 Test->AddError(FString::Printf(TEXT("Hopper PIE did not become ready: world=%s phase=%d navigation=%s building=%d feet=%s gameTime=%.2f viewer=%s player=%s at=%s sight=%d shot=%d"),
  *GetNameSafe(World),Hopper ? static_cast<int32>(Hopper->GetCombatPhase()) : -1,*GetNameSafe(Navigation),
  Navigation ? Navigation->IsNavigationBuildInProgress() : false,*Feet.ToCompactString(),World ? World->GetTimeSeconds() : 0.f,
  *GetNameSafe(Viewer),*GetNameSafe(Player),Player ? *Player->GetActorLocation().ToCompactString() : TEXT("none"),
  bSight,Viewer && Viewer->IsHopperShotActive()));
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
  Hopper->bEndCombatOnMechDestruction=false; // This legacy fixture explicitly covers the optional pilot phase.
  Test->TestEqual(TEXT("Boarding completes in the live world"),Hopper->GetCombatPhase(),EHopperCombatPhase::Mech);
  Test->TestNotNull(TEXT("Left weapon attached"),Hopper->GetArmModule(true));
  Test->TestNotNull(TEXT("Right weapon attached"),Hopper->GetArmModule(false));
  Test->TestNotNull(TEXT("Pilot mesh imported"),P->GetPilotMesh()->GetSkeletalMeshAsset());
  Test->TestEqual(TEXT("Combat test map places the Mk2 blueprint"),Hopper->GetClass()->GetPathName(),
   FString(TEXT("/Game/Characters/Hopper/BP_HopperEnemyMk2.BP_HopperEnemyMk2_C")));
  const UHopperVisualData* Visual=Hopper->VisualData.Get();
  if (!Test->TestNotNull(TEXT("Placed Mk2 visual data"),Visual)) return true;
  TMap<FSoftObjectPath,int32> ExpectedMeshes,PlacedMeshes;
  for (const FHopperMeshPart& Part:Visual->Parts) ++ExpectedMeshes.FindOrAdd(Part.Mesh.ToSoftObjectPath());
  ++ExpectedMeshes.FindOrAdd(Visual->PilotGun.ToSoftObjectPath());
  ++ExpectedMeshes.FindOrAdd(Visual->CockpitShieldMesh.ToSoftObjectPath());
  TArray<UStaticMeshComponent*> MeshParts; Hopper->GetComponents(MeshParts);
  for (const UStaticMeshComponent* Part:MeshParts)
   if (Part->ComponentHasTag(TEXT("HopperGenerated")) && Part->GetStaticMesh())
    ++PlacedMeshes.FindOrAdd(FSoftObjectPath(Part->GetStaticMesh()));
  Test->TestTrue(TEXT("PIE contains exactly one complete Mk2 assembly"),ExpectedMeshes.OrderIndependentCompareEqual(PlacedMeshes));
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
 if(!HopperPlayTest::OpenNavigationFixture()) return false;
 FRequestPlaySessionParams Play;
 Play.StartLocation=FVector(650,-320,110);
 Play.StartRotation=FRotator(0,90,0);
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
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FHopperEjectionStage,FAutomationTestBase*,Test,int32,Stage);
bool FHopperEjectionStage::Update()
{
 UWorld* World=GEditor->PlayWorld;
 auto* Hopper=HopperPlayTest::Find(World);
 if (!Test->TestNotNull(TEXT("Ejection fixture mech"),Hopper)) return true;
 auto* P=Hopper->GetPresentation();
 if (Stage==0)
 {
  const auto* Shield=Hopper->FindComponentByTag<UStaticMeshComponent>(TEXT("HopperCockpitShield"));
  if (!Test->TestNotNull(TEXT("Placed Mk2 cockpit shield"),Shield)) return true;
  const auto& Field=Shield->GetCustomPrimitiveData().Data;
  const bool bDeployed=!Shield->bHiddenInGame && Field.Num()>=4 && Field[0]>=1.f;
  if (!bDeployed && FPlatformTime::Seconds()-StartTime<8.f) return false;
  Test->TestTrue(TEXT("Boarding deploys the shield on the placed Mk2"),bDeployed);
  Test->TestTrue(TEXT("Placed Mk2 ejects its pilot by default"),Hopper->bEndCombatOnMechDestruction);
  Hopper->TakeDamage(10000.f,FDamageEvent(),nullptr,nullptr);
  Test->TestTrue(TEXT("Escape starts with a shield warning"),P->IsWaitingForShieldBreak());
  // Freeze only the presentation timeline to capture exact phases after rendering settles.
  P->SetComponentTickEnabled(false);
 }
 else
 {
  const float Launch=P->GetEjectionLaunchTime();
  const float Samples[]={0.f,FMath::Max(0.f,Launch-.18f),FMath::Max(0.f,Launch-.04f),Launch+.16f,Launch+.60f,Launch+10.f};
  P->TickComponent(Samples[Stage]-Samples[Stage-1],LEVELTICK_All,nullptr);
 }
 APlayerController* PC=World->GetFirstPlayerController();
 if (PC)
 {
  ACameraActor* Camera=Cast<ACameraActor>(PC->GetViewTarget());
  if (!Camera) Camera=World->SpawnActor<ACameraActor>();
  const FVector Look=Hopper->GetActorLocation()+FVector(0,0,Stage>=4?400:Stage==3?140:35);
  const float Distance=Stage>=4?1250:Stage==3?700:420;
  const FVector Location=Look+Hopper->GetActorForwardVector()*Distance-Hopper->GetActorRightVector()*Distance*.8f+FVector(0,0,Distance*.45f);
  Camera->SetActorLocation(Location); Camera->SetActorRotation((Look-Location).Rotation()); PC->SetViewTarget(Camera);
 }
 Test->TestTrue(TEXT("Death remains authoritative throughout escape"),Hopper->IsDead());
 return true;
}
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FHopperEjectionCapture,FAutomationTestBase*,Test,int32,Stage);
bool FHopperEjectionCapture::Update()
{
 HopperPlayTest::Capture(Test,FString::Printf(TEXT("Ejection_%02d.png"),Stage));
 return true;
}
DEFINE_LATENT_AUTOMATION_COMMAND_TWO_PARAMETER(FHopperEjectionRealtime,FAutomationTestBase*,Test,bool,bStart);
bool FHopperEjectionRealtime::Update()
{
 UWorld* World=GEditor->PlayWorld;
 if (bStart)
 {
  auto* Fixture=HopperPlayTest::Find(World);
  FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
  auto* H=World->SpawnActor<ATunaSweeperHopperEnemyCharacter>(Fixture->GetClass(),FVector(0,0,105),FRotator::ZeroRotator,Spawn);
  H->Tags.Add(TEXT("EjectionRealtimeTest"));
  H->TakeDamage(10000.f,FDamageEvent(),nullptr,nullptr);
 }
 else
 {
  for (TActorIterator<ATunaSweeperHopperEnemyCharacter> It(World);It;++It)
   if (It->ActorHasTag(TEXT("EjectionRealtimeTest")))
   {
    const float FlightSample=It->GetPresentation()->GetEjectionLaunchTime()+.60f;
    if (It->GetGameTimeSinceCreation()<FlightSample && FPlatformTime::Seconds()-StartTime<12.f) return false;
    Test->TestTrue(TEXT("Dead mech presentation keeps ticking in real gameplay"),It->GetPresentation()->GetPilotMesh()->GetComponentLocation().Z>400.f);
    Test->TestNull(TEXT("Real-time ejection detaches the complete seat"),It->GetPresentation()->GetPilotMesh()->GetAttachParent()->GetAttachParent());
    TInlineComponentArray<UStaticMeshComponent*> Meshes(*It);
    UStaticMeshComponent* const* Guard=Meshes.FindByPredicate([](const UStaticMeshComponent* Part)
    {
     return Part->GetName().StartsWith(TEXT("GuardBrow"));
    });
    const USceneComponent* Body=It->GetPresentation()->GetBodyRoot();
    const auto* FlyingFrame=Guard ? Cast<UPrimitiveComponent>((*Guard)->GetAttachmentRoot()) : nullptr;
    if (Test->TestNotNull(TEXT("The ejected frame has a physical root"),FlyingFrame))
    {
     const FVector Offset=(*Guard)->Bounds.Origin-Body->GetComponentLocation();
     Test->TestTrue(TEXT("Chaos carries the spinning frame upward and behind the wreck during the vertical launch"),
      FlyingFrame->IsSimulatingPhysics() && FlyingFrame->IsGravityEnabled() &&
      FVector::DotProduct(Offset,Body->GetForwardVector())<-90.f && Offset.Z>255.f &&
      FlyingFrame->GetPhysicsAngularVelocityInDegrees().Size()>80.f);
    }
    return true;
   }
  Test->AddError(TEXT("Real-time ejection fixture missing"));
 }
 return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHopperEjectionPresentationTest,"TunaSweeper.Hopper.Play.SeatEjection",
 EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHopperEjectionPresentationTest::RunTest(const FString& Parameters)
{
 if (!HopperPlayTest::OpenNavigationFixture()) return false;
 FRequestPlaySessionParams Play;
 Play.StartLocation=FVector(650,-320,110);
 Play.StartRotation=FRotator(0,90,0);
 Play.EditorPlaySettings=NewObject<ULevelEditorPlaySettings>(GetTransientPackage());
 Play.EditorPlaySettings->NewWindowWidth=1280; Play.EditorPlaySettings->NewWindowHeight=800;
 GEditor->RequestPlaySession(Play);
 ADD_LATENT_AUTOMATION_COMMAND(FHopperWaitForReady(this));
 for (int32 Stage=0;Stage<6;++Stage)
 {
  ADD_LATENT_AUTOMATION_COMMAND(FHopperEjectionStage(this,Stage));
  ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(.5f));
  ADD_LATENT_AUTOMATION_COMMAND(FHopperEjectionCapture(this,Stage));
 }
 ADD_LATENT_AUTOMATION_COMMAND(FHopperEjectionRealtime(this,true));
 ADD_LATENT_AUTOMATION_COMMAND(FHopperEjectionRealtime(this,false));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
 return true;
}
#endif
