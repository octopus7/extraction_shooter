#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
#include "Animation/AnimSequence.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "ProceduralMeshComponent.h"

namespace
{
 constexpr float SmokeInterval=.025f;
 constexpr float SmokeLifetime=1.6f;
 constexpr int32 SmokePoolSize=72;
 constexpr float GuardClearanceSeconds=.10f;
 float Smooth(float Alpha) { Alpha=FMath::Clamp(Alpha,0.f,1.f); return Alpha*Alpha*(3.f-2.f*Alpha); }
}

void UHopperPresentationComponent::InitializeEjectionHandle()
{
 EjectionHandle=nullptr;
 EjectionPullClip=nullptr;
 bEjectionPullFinished=false;
 bAwaitingEjectionPull=false;
 const FHopperEjectionPull& Pull=VisualData->EjectionPull;
 if (Pull.HandlePoses.Num()<2 || Pull.HandleMesh.IsNull() || Pull.PilotAnimation.IsNull()) return;
 UStaticMesh* Mesh=Pull.HandleMesh.LoadSynchronous();
 EjectionPullClip=Pull.PilotAnimation.LoadSynchronous();
 if (!Mesh || !EjectionPullClip || EjectionPullClip->GetPlayLength()<=UE_SMALL_NUMBER) return;

 AActor* Owner=GetOwner();
 const FName Name=MakeUniqueObjectName(Owner,UStaticMeshComponent::StaticClass(),TEXT("EjectionHandle"));
 EjectionHandle=NewObject<UStaticMeshComponent>(Owner,Name,RF_Transient);
 EjectionHandle->CreationMethod=HasBegunPlay()?EComponentCreationMethod::Instance:EComponentCreationMethod::UserConstructionScript;
 EjectionHandle->ComponentTags.Append({TEXT("HopperGenerated"),TEXT("HopperEjectionHandle")});
 EjectionHandle->SetMobility(EComponentMobility::Movable);
 EjectionHandle->SetCanEverAffectNavigation(false);
 EjectionHandle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 EjectionHandle->SetGenerateOverlapEvents(false);
 EjectionHandle->SetStaticMesh(Mesh);
 EjectionHandle->SetupAttachment(Seat);
 EjectionHandle->SetRelativeTransform(Pull.HandlePoses[0]);
 EjectionHandle->RegisterComponent();
 // The glove must be evaluated after its matching grip sample on this same frame.
 Pilot->AddTickPrerequisiteComponent(this);
}

float UHopperPresentationComponent::GetEjectionPullStartTime() const
{
 // Let the player's view reach the cockpit before the first head turn.
 return EjectionHandle ? .22f : 0.f;
}

float UHopperPresentationComponent::GetEjectionPullTriggerTime() const
{
 return EjectionHandle ? GetEjectionPullStartTime()+EjectionPullClip->GetPlayLength()*FMath::Clamp(VisualData->EjectionPull.TripPhase,0.f,1.f) : 0.f;
}

void UHopperPresentationComponent::TickEjectionPull()
{
 if (!EjectionHandle || bEjectionPullFinished) return;
 const float ClipLength=EjectionPullClip->GetPlayLength();
 const float Phase=FMath::Clamp((EjectionTime-GetEjectionPullStartTime())/ClipLength,0.f,1.f);
 if (CurrentClip!=EjectionPullClip)
 {
  PlayClip(EjectionPullClip,false,true);
  Pilot->SetPlayRate(0.f);
 }
 Pilot->SetPosition(Phase*ClipLength,false);
 const TArray<FTransform>& Poses=VisualData->EjectionPull.HandlePoses;
 const float Frame=Phase*(Poses.Num()-1);
 const int32 A=FMath::FloorToInt(Frame), B=FMath::Min(A+1,Poses.Num()-1);
 FTransform Pose;
 Pose.Blend(Poses[A],Poses[B],Frame-A);
 EjectionHandle->SetRelativeTransform(Pose);
 bEjectionPullFinished=Phase>=1.f;
}

void UHopperPresentationComponent::StartSeatEjection(UNiagaraSystem* MechEffect,const FVector& ImpactTravel)
{
 if (EjectionTime>=0.f || !Seat || !Pilot) return;
 const float TripTime=GetEjectionPullTriggerTime();
 bAwaitingEjectionPull=TripTime>0.f;
 if (bAwaitingEjectionPull)
 {
  WreckSprayDirection=ImpactTravel.GetSafeNormal2D();
  PendingDestructionEffect=MechEffect;
  EjectionShieldDelay=TripTime;
  if (GetShieldWarningDuration()>0.f)
  {
   ShieldStage=EShieldStage::Failing;
   ShieldAge=0.f;
  }
 }
 else EjectionShieldDelay=StartDestructionEffects(MechEffect,ImpactTravel);
 // Defeat can interrupt boarding. Re-seat the pilot before moving the entire cabin assembly.
 SeatPilot();
 bWreck=true; bMechActive=false; bTransferring=false; bBoarding=false;
 SetGunVisible(false);
 EjectionTime=0.f; EjectionInitialHatch=HatchOpen; bSeatLaunched=false; bRailReleaseFlashed=false;
 if (bAwaitingEjectionPull) TickCockpitShield(0.f);
 EjectionSeatStart=Seat->GetRelativeLocation();
 if (auto* Cabin=Parts.Find(TEXT("Cabin_Interior")))
  (*Cabin)->AttachToComponent(Seat,FAttachmentTransformRules::KeepWorldTransform);
 if (EjectionShieldDelay<=0.f && !VisualData->EjectionGuardParts.IsEmpty()) ReleaseEjectionGuard();
 SetComponentTickEnabled(true);
}

void UHopperPresentationComponent::ReleaseEjectionGuard()
{
 GuardReleaseTime=EjectionTime;
 if (GetNetMode()==NM_DedicatedServer) return;
 // Defeating the enemy disables collision actor-wide. The discarded frame needs its own physics owner.
 FActorSpawnParameters Spawn;
 Spawn.Owner=GetOwner();
 Spawn.ObjectFlags=RF_Transient;
 Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
 AActor* Debris=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),BodyRoot->GetComponentTransform(),Spawn);
 if (!Debris) return;
 ReleasedGuard=Debris;
 UStaticMeshComponent* RigidBody=nullptr;
 for (FName Name:VisualData->EjectionGuardParts)
 {
  UStaticMeshComponent* Panel=Parts.FindRef(Name);
  if (!Panel || !Panel->GetStaticMesh()) continue;
  // Use each panel's cooked convex hull. Keeping the original render components preserves their live soot and visibility.
  auto* Shape=NewObject<UStaticMeshComponent>(Debris,Name,RF_Transient);
  Shape->SetMobility(EComponentMobility::Movable);
  Shape->SetCanEverAffectNavigation(false);
  Shape->SetVisibility(false);
  Shape->SetGenerateOverlapEvents(false);
  Shape->SetCollisionObjectType(ECC_PhysicsBody);
  Shape->SetCollisionResponseToAllChannels(ECR_Ignore);
  Shape->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
  Shape->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
  Shape->SetStaticMesh(Panel->GetStaticMesh());
  Shape->SetWorldTransform(Panel->GetComponentTransform());
  if (!RigidBody) Debris->SetRootComponent(Shape);
  Shape->RegisterComponent();
  if (RigidBody)
   Shape->AttachToComponent(RigidBody,FAttachmentTransformRules(EAttachmentRule::KeepWorld,true));
  else
  {
   RigidBody=Shape;
   RigidBody->SetUseCCD(true);
   RigidBody->SetSimulatePhysics(true);
  }
  Panel->AttachToComponent(RigidBody,FAttachmentTransformRules::KeepWorldTransform);
  ScorchedHullMaterials.RemoveSingleSwap(Cast<UMaterialInstanceDynamic>(Panel->GetOverlayMaterial()));
 }
 if (!RigidBody) { Debris->Destroy(); return; }
 const FVector Forward=BodyRoot->GetForwardVector().GetSafeNormal2D();
 const FVector Right=FVector::CrossProduct(FVector::UpVector,Forward);
 RigidBody->AddImpulse(FVector::UpVector*570.f-Forward*690.f,NAME_None,true);
 RigidBody->AddAngularImpulseInDegrees(-Right*235.f+Forward*45.f,NAME_None,true);
}

FVector UHopperPresentationComponent::EjectionPosition(float FlightTime) const
{
 FVector Position=EjectionLaunchTransform.GetLocation()+FVector::UpVector*(EjectionUpSpeed*FlightTime+.5f*EjectionUpAcceleration*FlightTime*FlightTime);
 if (VisualData->EjectionGuardParts.IsEmpty()) Position+=EjectionLaunchTransform.GetUnitAxis(EAxis::X)*EjectionForwardSpeed*FlightTime;
 return Position;
}

float UHopperPresentationComponent::GetEjectionHatchDuration() const
{
 const float FullStroke=FMath::Max3(.05f,EjectionHatchSeconds,VisualData ? VisualData->HatchTravelSeconds : 0.f);
 const float RemainingStroke=GetSeatDockDuration()>0.f ? 1.f-EjectionInitialHatch : 1.f;
 return EjectionShieldDelay+FullStroke*RemainingStroke;
}
float UHopperPresentationComponent::GetEjectionLaunchTime() const
{
 if (VisualData && !VisualData->EjectionGuardParts.IsEmpty())
 {
  if (EjectionHandle) return EjectionShieldDelay;
  return (GuardReleaseTime>=0.f ? GuardReleaseTime : EjectionShieldDelay)+GuardClearanceSeconds;
 }
 float LiftDuration=0.f;
 if (VisualData && VisualData->SeatLockDrop>0.f)
 {
  const float RemainingLift=VisualData->SeatLocation.Z+VisualData->SeatLockDrop-EjectionSeatStart.Z;
  const bool bAtPickup=LadderBoardingClip && VisualData->SeatPickupLift>0.f &&
   FMath::IsNearlyEqual(EjectionSeatStart.X,VisualData->SeatLocation.X+VisualData->SeatBoardingReach,.1);
  LiftDuration=bAtPickup ? PickupLiftSeconds*FMath::Clamp(RemainingLift/VisualData->SeatPickupLift,0.f,1.f) :
   SeatLiftSeconds*FMath::Clamp(RemainingLift/VisualData->SeatLockDrop,0.f,1.f);
 }
 return GetEjectionHatchDuration()+LiftDuration+FMath::Max(.05f,EjectionSlideSeconds);
}

void UHopperPresentationComponent::UpdateEjectionHatch()
{
 if (bAwaitingEjectionPull) return;
 if (GuardReleaseTime<0.f && !VisualData->EjectionGuardParts.IsEmpty() && !IsWaitingForShieldBreak()) ReleaseEjectionGuard();
 if (HatchOpen>=1.f) return;
 const float OpeningDuration=GetEjectionHatchDuration()-EjectionShieldDelay;
 const float OpeningAlpha=OpeningDuration>UE_SMALL_NUMBER ? (EjectionTime-EjectionShieldDelay)/OpeningDuration : 1.f;
 SetHatchOpen(FMath::Lerp(EjectionInitialHatch,1.f,Smooth(OpeningAlpha)));
}

void UHopperPresentationComponent::TickSeatEjection(float LaunchTime)
{
 if (!Seat || !Pilot) return;
 const float FlightDuration=FMath::Max(.1f,EjectionFlightSeconds);
 const bool bVertical=!VisualData->EjectionGuardParts.IsEmpty();
 if (!bSeatLaunched && !bVertical)
 {
  const float HatchDuration=GetEjectionHatchDuration();
  if (EjectionTime<HatchDuration) return;
  const float SlideDuration=FMath::Max(.05f,EjectionSlideSeconds);
  const float SlideStart=LaunchTime-SlideDuration;
  FVector Position=EjectionSeatStart;
  const float LiftDuration=SlideStart-HatchDuration;
  if (LiftDuration>UE_SMALL_NUMBER)
   Position.Z=FMath::Lerp(EjectionSeatStart.Z,VisualData->SeatLocation.Z+VisualData->SeatLockDrop,
    Smooth((EjectionTime-HatchDuration)/LiftDuration));
  // If defeat interrupts docking, continue from the carriage's live position to the same rail exit.
  const float RailOrigin=GetSeatDockDuration()>0.f ? VisualData->SeatLocation.X : EjectionSeatStart.X;
  Position.X=FMath::Lerp(EjectionSeatStart.X,RailOrigin+EjectionSlideDistance,Smooth((EjectionTime-SlideStart)/SlideDuration));
  Seat->SetRelativeLocation(Position);
  // The rear roller clears the end keeper before the rocket takes over.
  const double ExitStation=RailOrigin+VisualData->RailReleaseTravel;
  if (LoadedEjectSparks && !bRailReleaseFlashed && EjectionTime>=SlideStart &&
      (Position.X>=ExitStation || EjectionTime>=LaunchTime))
  {
   EmitRailSparks(true);
   bRailReleaseFlashed=true;
  }
 }
 if (!bSeatLaunched)
 {
  if (EjectionTime<LaunchTime || (bVertical && GuardReleaseTime<0.f)) return;
  if (bVertical) EmitRailSparks(true);
  EjectionLaunchTransform=Seat->GetComponentTransform();
  Seat->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
  bSeatLaunched=true;
  if (EjectionTime<LaunchTime+FlightDuration) CreateEjectionExhaust();
 }
 const float FlightTime=EjectionTime-LaunchTime;
 Seat->SetWorldLocation(EjectionPosition(FMath::Min(FlightTime,FlightDuration)));
 const bool bFlying=FlightTime<FlightDuration;
 if (LoadedSeatExhaust)
 {
  if (!bFlying)
  {
   for (UNiagaraComponent* Jet:SeatJets)
   {
    if (!IsValid(Jet)) continue;
    Jet->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    Jet->Deactivate(); // Stop emitting. Existing embers and smoke finish in world space.
   }
   Seat->SetHiddenInGame(true,true);
   bSeatFlightFinished=true;
  }
  return;
 }
 if (EjectionFlame)
 {
  EjectionFlame->SetVisibility(bFlying);
  EjectionFlame->SetRelativeScale3D(FVector(1,1,1.f+.16f*FMath::Sin(FlightTime*53.f)));
 }
 // Sample the analytic path, not the current seat position: low frame rates still leave a continuous trail.
 const float LastEmission=FMath::Min(FlightTime,FlightDuration);
 NextSmokeTime=FMath::Max(NextSmokeTime,FlightTime-SmokeLifetime);
 while (!EjectionSmoke.IsEmpty() && NextSmokeTime<LastEmission)
 {
  const int32 Index=NextSmokeIndex++%EjectionSmoke.Num();
  SmokeBirthTimes[Index]=NextSmokeTime;
  SmokeOrigins[Index]=EjectionPosition(NextSmokeTime)-FVector(0,0,32)+
   EjectionLaunchTransform.GetUnitAxis(EAxis::Y)*(FMath::Sin(NextSmokeTime*47.f)*10.f);
  NextSmokeTime+=SmokeInterval;
 }
 FVector CameraLocation=Seat->GetComponentLocation()+FVector(500,-500,500);
 FRotator CameraRotation;
 if (APlayerController* PC=GetWorld()->GetFirstPlayerController()) PC->GetPlayerViewPoint(CameraLocation,CameraRotation);
 for (int32 Index=0;Index<EjectionSmoke.Num();++Index)
 {
  const float Age=FlightTime-SmokeBirthTimes[Index];
  const bool bVisible=Age>=0 && Age<SmokeLifetime;
  UStaticMeshComponent* Sprite=EjectionSmoke[Index];
  Sprite->SetVisibility(bVisible);
  if (!bVisible) continue;
  const float Alpha=Age/SmokeLifetime;
  const FVector Location=SmokeOrigins[Index]+FVector(FMath::Sin(Index*2.4f)*Age*8.f,0,Age*18.f);
  Sprite->SetWorldLocation(Location);
  Sprite->SetWorldRotation(FRotationMatrix::MakeFromZ(CameraLocation-Location).ToQuat());
  Sprite->SetWorldScale3D(FVector(FMath::Lerp(.55f,1.4f,Smooth(Alpha))));
  UMaterialInstanceDynamic* MID=EjectionSmokeMIDs[Index];
  if (MID)
  {
   MID->SetScalarParameterValue(TEXT("Opacity"),.88f*(1.f-Smooth(Alpha))*FMath::Min(1.f,Age/.08f));
  }
 }
 if (!bFlying) Seat->SetHiddenInGame(true,true);
 if (FlightTime>=FlightDuration+SmokeLifetime)
 {
  ClearEjectionExhaust();
  bSeatFlightFinished=true;
 }
}

void UHopperPresentationComponent::CreateEjectionExhaust()
{
 if (GetWorld()->GetNetMode()==NM_DedicatedServer) return;
 if (LoadedSeatExhaust)
 {
  for (const float Side : {-1.f,1.f})
  {
   FVector Nozzle=VisualData->SeatExhaustNozzle;
   Nozzle.Y*=Side;
   if (UNiagaraComponent* Jet=UNiagaraFunctionLibrary::SpawnSystemAttached(LoadedSeatExhaust,Seat,NAME_None,
      Nozzle,FRotator::ZeroRotator,EAttachLocation::KeepRelativeOffset,true,false,ENCPoolMethod::None,false))
   {
    Jet->AddTickPrerequisiteComponent(this);
    Jet->Activate();
    SeatJets.Add(Jet);
   }
  }
  return;
 }
 NextSmokeTime=0.f; NextSmokeIndex=0;
 UMaterialInterface* Glow=EjectionFlameMaterial.LoadSynchronous();
 if (Glow)
 {
  EjectionFlame=NewObject<UProceduralMeshComponent>(GetOwner(),NAME_None,RF_Transient);
  EjectionFlame->CreationMethod=EComponentCreationMethod::Instance;
  EjectionFlame->SetCanEverAffectNavigation(false); EjectionFlame->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  EjectionFlame->SetCastShadow(false); EjectionFlame->SetReceivesDecals(false);
  EjectionFlame->SetupAttachment(Seat); EjectionFlame->SetRelativeLocation(FVector(0,0,-18));
  EjectionFlame->RegisterComponent();
  auto* GlowMID=UMaterialInstanceDynamic::Create(Glow,this);
  GlowMID->SetScalarParameterValue(TEXT("Intensity"),5.f);
  EjectionFlame->SetMaterial(0,GlowMID);
  TArray<FVector> Vertices,Normals;
  TArray<int32> Triangles;
  TArray<FVector2D> UVs;
  TArray<FLinearColor> Colors;
  TArray<FProcMeshTangent> Tangents;
  for (int32 Jet=0;Jet<2;++Jet) for (int32 Layer=0;Layer<2;++Layer) for (int32 Side=0;Side<12;++Side)
  {
   const float Radius=Layer==0?8.f:4.5f, Length=Layer==0?85.f:48.f;
   const FVector Origin(0,Jet==0?-17:17,0);
   const float A=Side*2.f*PI/12, B=(Side+1)*2.f*PI/12;
   const int32 First=Vertices.Num();
   Vertices.Append({Origin+FVector(Radius*FMath::Cos(A),Radius*FMath::Sin(A),0),
    Origin+FVector(Radius*FMath::Cos(B),Radius*FMath::Sin(B),0),Origin-FVector(0,0,Length)});
   Triangles.Append({First,First+1,First+2});
   const FLinearColor Color=Layer==0?FLinearColor(1.f,.16f,.015f,.85f):FLinearColor(1.f,.85f,.4f,1.f);
   for (int32 V=0;V<3;++V)
   {
    Normals.Add(FVector::ForwardVector); UVs.Add(FVector2D::ZeroVector); Tangents.Add(FProcMeshTangent(0,1,0));
    Colors.Add(V==2?FLinearColor(Color.R,Color.G,Color.B,0.f):Color);
   }
  }
  EjectionFlame->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UVs,Colors,Tangents,false);
 }
 UMaterialInterface* SmokeMaterial=EjectionSmokeMaterial.LoadSynchronous();
 UStaticMesh* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
 if (!SmokeMaterial || !Plane) return;
 for (int32 Index=0;Index<SmokePoolSize;++Index)
 {
  auto* Sprite=NewObject<UStaticMeshComponent>(GetOwner(),NAME_None,RF_Transient);
  Sprite->CreationMethod=EComponentCreationMethod::Instance;
  Sprite->SetCanEverAffectNavigation(false); Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Sprite->SetCastShadow(false); Sprite->SetReceivesDecals(false);
  Sprite->SetStaticMesh(Plane); Sprite->SetVisibility(false); Sprite->RegisterComponent();
  auto* MID=Sprite->CreateDynamicMaterialInstance(0,SmokeMaterial);
  MID->SetVectorParameterValue(TEXT("TintColor"),FLinearColor(.96f,.98f,1.f));
  MID->SetScalarParameterValue(TEXT("EmissiveStrength"),1.2f);
  EjectionSmoke.Add(Sprite); EjectionSmokeMIDs.Add(MID);
  SmokeBirthTimes.Add(-100.f); SmokeOrigins.Add(FVector::ZeroVector);
 }
}

void UHopperPresentationComponent::ClearEjectionExhaust()
{
 for (UNiagaraComponent* Jet:SeatJets) if (IsValid(Jet)) Jet->DestroyComponent();
 SeatJets.Reset();
 if (IsValid(EjectionFlame)) EjectionFlame->DestroyComponent();
 EjectionFlame=nullptr;
 for (UStaticMeshComponent* Sprite:EjectionSmoke) if (IsValid(Sprite)) Sprite->DestroyComponent();
 EjectionSmoke.Reset(); EjectionSmokeMIDs.Reset(); SmokeBirthTimes.Reset(); SmokeOrigins.Reset();
}
