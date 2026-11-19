#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/StaticMesh.h"
#include "TunaSweeperCollisionChannels.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"

UHopperPresentationComponent::UHopperPresentationComponent()
{
 PrimaryComponentTick.bCanEverTick=true;
 PrimaryComponentTick.TickGroup=TG_PostPhysics;
 EjectionSmokeMaterial=FSoftObjectPath(TEXT("/Game/Characters/Hopper/Materials/M_HopperEscapeSmoke.M_HopperEscapeSmoke"));
 EjectionFlameMaterial=FSoftObjectPath(TEXT("/Game/Characters/CombatPatterns/Materials/M_CP_Glow.M_CP_Glow"));
}
float UHopperPresentationComponent::CapsuleHalfHeight() const
{
 const ACharacter* C=Cast<ACharacter>(GetOwner());
 return C ? C->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() : 105.f;
}
void UHopperPresentationComponent::Initialize(UHopperVisualData* Data)
{
 if (!Data || (VisualData==Data && IsValid(BodyRoot))) return;
 if (ReleasedGuard.IsValid()) ReleasedGuard->Destroy();
 ReleasedGuard.Reset(); GuardReleaseTime=-1.f;
 ClearEjectionExhaust(); EjectionTime=-1.f; EjectionInitialHatch=0.f; bSeatLaunched=false;
 bWreckBurstPlayed=false; bWreckFireStarted=false; bSeatFlightFinished=false;
 WreckSprayDirection=FVector::ZeroVector; ScorchedHullMaterials.Reset(); OilReceivers.Reset();
 // This component owns every generated component. Reconstruct only when its data changes.
 TArray<USceneComponent*> Old;
 TArray<USceneComponent*> Owned;
 GetOwner()->GetComponents(Owned);
 // Construction components can survive map/PIE duplication without the transient lookup map.
 for (USceneComponent* Component:Owned)
  if (Component->ComponentHasTag(TEXT("HopperGenerated"))) Old.AddUnique(Component);
 for (const auto& Pair:Parts) Old.Add(Pair.Value);
 Old.Append({Gun.Get(),Pilot.Get(),Seat.Get(),LeftSocket.Get(),RightSocket.Get(),BodyRoot.Get()});
 for (USceneComponent* C:Old) if (IsValid(C)) C->DestroyComponent();
 Parts.Empty(); VisualData=Data; CurrentClip=nullptr; BodyCrouch=0; Gait.Reset();
 LoadedSeatExhaust=Data->SeatExhaustSystem.LoadSynchronous();
 LadderBoardingClip=Data->PilotLadderBoarding.LoadSynchronous();
 LoadedLockSparks=Data->SeatLockSparks.LoadSynchronous();
 LoadedEjectSparks=Data->SeatEjectSparks.LoadSynchronous();
 AActor* Owner=GetOwner();
 const auto Creation=HasBegunPlay()?EComponentCreationMethod::Instance:EComponentCreationMethod::UserConstructionScript;
 auto Scene=[Owner,Creation](FName Name,USceneComponent* Parent)
 {
  USceneComponent* C=NewObject<USceneComponent>(Owner,MakeUniqueObjectName(Owner,USceneComponent::StaticClass(),Name),RF_Transient);
  C->CreationMethod=Creation; C->ComponentTags.Add(TEXT("HopperGenerated")); C->SetupAttachment(Parent); C->RegisterComponent(); return C;
 };
 BodyRoot=Scene(TEXT("HopperBody"),Owner->GetRootComponent());
 BodyRoot->SetRelativeLocation(FVector(0,0,-CapsuleHalfHeight()));
 Seat=Scene(TEXT("PilotSeat"),BodyRoot); Seat->SetRelativeLocation(Data->SeatLocation);
 EjectionSeatStart=Data->SeatLocation;
 LeftSocket=Scene(TEXT("LeftArmSocket"),BodyRoot); LeftSocket->SetRelativeLocation(Data->LeftArmMount);
 RightSocket=Scene(TEXT("RightArmSocket"),BodyRoot); RightSocket->SetRelativeLocation(Data->RightArmMount);
 for (const FHopperMeshPart& Part:Data->Parts)
 {
  // Arms are also shown as editor previews; runtime arm actors replace them in BeginPlay.
  UStaticMesh* Mesh=Part.Mesh.LoadSynchronous(); if (!Mesh) continue;
  UStaticMeshComponent* C=NewObject<UStaticMeshComponent>(Owner,MakeUniqueObjectName(Owner,UStaticMeshComponent::StaticClass(),Part.PartName),RF_Transient);
  // Mesh assignment can queue navigation data before component registration.
  C->SetCanEverAffectNavigation(false); C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  C->CreationMethod=Creation; C->ComponentTags.Add(TEXT("HopperGenerated")); C->SetupAttachment(BodyRoot); C->SetStaticMesh(Mesh);
  C->SetMobility(EComponentMobility::Movable); C->RegisterComponent(); C->SetHiddenInGame(HasBegunPlay() && Part.PartName.ToString().StartsWith(TEXT("Arm_"))); Parts.Add(Part.PartName,C);
  if (Part.PartName==TEXT("Cabin_Interior") && GetSeatDockDuration()>0.f)
   C->AttachToComponent(Seat,FAttachmentTransformRules::KeepWorldTransform);
  if (!Part.PartName.ToString().StartsWith(TEXT("Arm_"))) CreateMechHitBox(C);
 }
 Pilot=NewObject<USkeletalMeshComponent>(Owner,MakeUniqueObjectName(Owner,USkeletalMeshComponent::StaticClass(),TEXT("HopperPilot")),RF_Transient);
 Pilot->SetCanEverAffectNavigation(false); Pilot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Pilot->CreationMethod=Creation; Pilot->ComponentTags.Add(TEXT("HopperGenerated")); Pilot->SetupAttachment(Seat); Pilot->SetSkeletalMeshAsset(Data->PilotMesh.LoadSynchronous());
 Pilot->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
 Pilot->RegisterComponent();
 Gun=NewObject<UStaticMeshComponent>(Owner,MakeUniqueObjectName(Owner,UStaticMeshComponent::StaticClass(),TEXT("PilotGun")),RF_Transient);
 Gun->SetCanEverAffectNavigation(false); Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Gun->CreationMethod=Creation; Gun->ComponentTags.Add(TEXT("HopperGenerated")); Gun->SetupAttachment(Pilot,TEXT("hand_r"));
 Gun->SetStaticMesh(Data->PilotGun.LoadSynchronous()); Gun->SetRelativeTransform(Data->GunTransform);
 Gun->RegisterComponent(); Gun->SetVisibility(false);
 PlayClip(Data->PilotSeated.LoadSynchronous(),true);
 HatchOpen=0; bWreck=false; bBoarding=false; bTransferring=false; bSeated=true; bMechActive=false;
 InitializeCockpitShield();
 InitializeBlastFlash();
 if (HasBegunPlay()) PreloadWreckEffects();
}
USceneComponent* UHopperPresentationComponent::GetArmSocket(bool bLeft) const { return bLeft?LeftSocket:RightSocket; }
void UHopperPresentationComponent::PlayClip(UAnimSequence* Clip,bool bLoop,bool bOverrideAction)
{
 if (!Pilot || !Clip || (!bOverrideAction && ActionRemaining>0)) return;
 if (Clip!=CurrentClip || !bLoop)
 {
  Pilot->PlayAnimation(Clip,bLoop); Pilot->SetPlayRate(1.f); CurrentClip=Clip;
 }
 if (bOverrideAction) ActionRemaining=Clip->GetPlayLength();
}
void UHopperPresentationComponent::WaitBesideMech()
{
 if (!Pilot || !Seat) return;
 TransferStart=BodyRoot->GetComponentTransform().TransformPosition(FVector(VisualData->BoardingStartDistance,0,0));
 if (GetSeatDockDuration()>0.f)
 {
  Seat->SetRelativeLocation(VisualData->SeatLocation);
  TransferEnd=BodyRoot->GetComponentTransform().TransformPosition(VisualData->SeatLocation+GetSeatDockOffset(0.f));
 }
 else TransferEnd=Seat->GetComponentLocation();
 Pilot->AttachToComponent(BodyRoot,FAttachmentTransformRules::KeepWorldTransform);
 Pilot->SetWorldLocationAndRotation(TransferStart,Seat->GetComponentQuat());
 bSeated=false; bMechActive=false;
 SetPilotLocomotion(false);
}
void UHopperPresentationComponent::StartBoarding(float Duration)
{
 if (!Pilot || !Seat) return;
 WaitBesideMech();
 Pilot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
 BeginTransfer(Duration,true);
}
void UHopperPresentationComponent::SeatPilot()
{
 if (!Pilot || !Seat) return;
 if (!bSeated || Pilot->GetAttachParent()!=Seat) AttachPilotToSeat();
 bTransferring=false; bMechActive=true;
}
void UHopperPresentationComponent::AttachPilotToSeat()
{
 bSeated=true; ActionRemaining=0;
 Pilot->AttachToComponent(Seat,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
 Pilot->SetRelativeRotation(FRotator::ZeroRotator);
 PlayClip(VisualData->PilotSeated.LoadSynchronous(),true);
}
float UHopperPresentationComponent::GetSeatDockDuration() const
{
 return VisualData && (VisualData->SeatBoardingReach>0.f || VisualData->SeatLockDrop>0.f)
  ? RailTravelSeconds+RailStopSeconds+SeatLiftSeconds+(LadderBoardingClip?PickupLiftSeconds+PickupStopSeconds:0.f) : 0.f;
}
FVector UHopperPresentationComponent::GetSeatDockOffset(float DockTime) const
{
 const float RailStart=LadderBoardingClip ? PickupLiftSeconds+PickupStopSeconds : 0.f;
 const float LockStart=RailStart+RailTravelSeconds+RailStopSeconds;
 const float RailHeight=VisualData->SeatLockDrop*(1.f-FMath::SmoothStep(LockStart,LockStart+SeatLiftSeconds,DockTime));
 const float BelowRail=LadderBoardingClip ? VisualData->SeatPickupLift*(1.f-FMath::SmoothStep(0.f,PickupLiftSeconds,DockTime)) : 0.f;
 return FVector(VisualData->SeatBoardingReach*(1.f-FMath::SmoothStep(RailStart,RailStart+RailTravelSeconds,DockTime)),0.f,RailHeight-BelowRail);
}
void UHopperPresentationComponent::StartDisembark(const FVector& GroundDestination,float Duration)
{
 if (!Pilot || !BodyRoot) return;
 bMechActive=false; bWreck=true; bSeated=false;
 BodyRoot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
 TransferStart=Pilot->GetComponentLocation(); TransferEnd=GroundDestination;
 Pilot->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
 BeginTransfer(Duration,false);
}
void UHopperPresentationComponent::BeginTransfer(float Duration, bool bIsBoarding)
{
 bBoarding=bIsBoarding; bTransferring=true; bSeated=false;
 TransferTime=0; TransferDuration=FMath::Max(.05f,Duration);
 TransferInitialHatch=HatchOpen;
 // Leave time to climb even when a placement shortens the overall boarding animation.
 HatchLeadTime=FMath::Clamp(VisualData->HatchTravelSeconds*(1.f-HatchOpen),0.f,TransferDuration*.4f);
 if (HatchLeadTime<=0.f) SetHatchOpen(1.f);
 if (bIsBoarding && LadderBoardingClip)
 {
  PlayClip(LadderBoardingClip,false,true);
  Pilot->SetPlayRate(0.f);
  return;
 }
 if (UAnimSequence* Clip=(bIsBoarding?VisualData->PilotBoarding:VisualData->PilotDisembark).LoadSynchronous())
 {
  PlayClip(Clip,false,true);
  Pilot->SetPlayRate(HatchLeadTime>0.f ? 0.f : Clip->GetPlayLength()/TransferDuration);
 }
}
void UHopperPresentationComponent::FinishDisembark()
{
 if (!Pilot) return;
 bTransferring=false; bSeated=false; ActionRemaining=0;
 Pilot->AttachToComponent(GetOwner()->GetRootComponent(),FAttachmentTransformRules::SnapToTargetNotIncludingScale);
 Pilot->SetRelativeLocation(FVector(0,0,-CapsuleHalfHeight()));
 Pilot->SetRelativeRotation(FRotator::ZeroRotator);
 PlayClip(VisualData->PilotIdle.LoadSynchronous(),true);
}
float UHopperPresentationComponent::GetMaxStableWalkSpeed() const
{
 if (!VisualData) return 35.f;
 const float Length=FVector::Distance(VisualData->LeftHip,VisualData->LeftKnee)+FVector::Distance(VisualData->LeftKnee,VisualData->LeftAnkle);
 const float Height=VisualData->LeftHip.Z-10.f-VisualData->LeftAnkle.Z+2.f;
 const float Horizontal=FMath::Sqrt(FMath::Max(0.f,Length*Length-Height*Height));
 // One full alternating cycle plus support time must fit the planted leg reach.
 return FMath::Max(0.f,Horizontal-8.f)/FMath::Max(.1f,2.f*StepDuration+.08f-.22f);
}
UBoxComponent* UHopperPresentationComponent::CreateMechHitBox(UStaticMeshComponent* Part)
{
 if (!Part || !Part->GetStaticMesh()) return nullptr;
 // All damage volumes belong to the enemy, even when attached to a replaceable arm actor.
 UBoxComponent* Box=NewObject<UBoxComponent>(GetOwner(),NAME_None,RF_Transient);
 Box->CreationMethod=EComponentCreationMethod::Instance;
 Box->ComponentTags.Add(TEXT("HopperGenerated")); Box->ComponentTags.Add(TEXT("HopperMechHitBox"));
 Box->SetCanEverAffectNavigation(false); Box->SetGenerateOverlapEvents(false);
 Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 Box->SetCollisionObjectType(ECC_WorldDynamic); Box->SetCollisionResponseToAllChannels(ECR_Ignore);
 Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
 Box->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile,ECR_Block);
 Box->SetupAttachment(Part);
 const FBox Bounds=Part->GetStaticMesh()->GetBoundingBox();
 Box->SetRelativeLocation(Bounds.GetCenter());
 Box->SetBoxExtent(Bounds.GetExtent().ComponentMax(FVector(1.f)));
 Box->RegisterComponent();
 return Box;
}
void UHopperPresentationComponent::DisableMechHitBoxes()
{
 TArray<UBoxComponent*> Boxes; GetOwner()->GetComponents(Boxes);
 for (UBoxComponent* Box:Boxes)
  if (Box->ComponentHasTag(TEXT("HopperMechHitBox"))) Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void UHopperPresentationComponent::SetMechActive(bool bActive) { bMechActive=bActive && !bWreck; }
void UHopperPresentationComponent::SetGunVisible(bool bVisible) { if (Gun) Gun->SetVisibility(bVisible); }
FVector UHopperPresentationComponent::GetPilotMuzzleLocation() const
{
 return Gun && Gun->GetStaticMesh() ? Gun->GetComponentTransform().TransformPosition(FVector(24,0,5)) :
 Pilot ? Pilot->GetComponentLocation()+GetOwner()->GetActorForwardVector()*28+FVector(0,0,38) : GetOwner()->GetActorLocation();
}
void UHopperPresentationComponent::SetPilotLocomotion(bool bWalking)
{
 if (VisualData && !bSeated && !bTransferring) PlayClip((bWalking?VisualData->PilotWalk:VisualData->PilotIdle).LoadSynchronous(),true);
}
void UHopperPresentationComponent::PlayPilotFire()
{
 if (VisualData) PlayClip(VisualData->PilotFire.LoadSynchronous(),false,true);
}
void UHopperPresentationComponent::PlayPilotMelee()
{
 if (VisualData) PlayClip(VisualData->PilotMelee.LoadSynchronous(),false,true);
}
void UHopperPresentationComponent::SetHatchOpen(float Alpha)
{
 HatchOpen=FMath::Clamp(Alpha,0.f,1.f);
 if (VisualData && !VisualData->HatchMotion.IsEmpty())
 {
  for (const FHopperPartMotion& Track:VisualData->HatchMotion)
  {
   const auto* Part=Parts.Find(Track.PartName); if (!Part || Track.Frames.IsEmpty()) continue;
   // The released hoop follows Chaos; the disconnected latches and rams stay on the chassis.
   if (GuardReleaseTime>=0.f && ((*Part)->GetAttachParent()!=BodyRoot || Track.PartName.ToString().StartsWith(TEXT("Guard")))) continue;
   const float Frame=HatchOpen*(Track.Frames.Num()-1);
   const int32 A=FMath::FloorToInt(Frame), B=FMath::Min(A+1,Track.Frames.Num()-1);
   FTransform Pose; Pose.Blend(Track.Frames[A],Track.Frames[B],Frame-A);
   (*Part)->SetRelativeTransform(Pose);
  }
  return;
 }
 // Original cockpit rotates forward around its lower hinge.
 if (const TObjectPtr<UStaticMeshComponent>* Hatch=Parts.Find(TEXT("Hatch_Pivot")))
 {
  const FVector Pivot(27.55135,0,108.85042);
  const FQuat Rotation(FVector::YAxisVector,FMath::DegreesToRadians(96.f*HatchOpen));
  (*Hatch)->SetRelativeTransform(FTransform(Rotation,Pivot-Rotation.RotateVector(Pivot)));
 }
}
float UHopperPresentationComponent::GetPilotTravelAlpha(float Elapsed) const
{
 return FMath::Clamp((Elapsed-HatchLeadTime)/(TransferDuration-HatchLeadTime),0.f,1.f);
}
FVector UHopperPresentationComponent::GetTransferPosition(const FVector& Start, const FVector& End, float Alpha, bool bIsBoarding) const
{
 const float T=FMath::Clamp(Alpha,0.f,1.f);
 const float Arc=FMath::Sin(PI*T)*(bIsBoarding?22.f:30.f);
 if (VisualData && VisualData->HatchTravelSeconds>0.f)
 {
  // Clear the extended step vertically before crossing its outer edge. Reverse the path on exit.
  const float Climb=bIsBoarding?T:1.f-T;
  const FVector& Ground=bIsBoarding?Start:End;
  const FVector& Cockpit=bIsBoarding?End:Start;
  FVector P=FMath::Lerp(Ground,Cockpit,FMath::SmoothStep(.22f,1.f,Climb));
  P.Z=FMath::Lerp(Ground.Z,Cockpit.Z,FMath::SmoothStep(0.f,.68f,Climb))+Arc;
  return P;
 }
 return FMath::Lerp(Start,End,T*T*(3.f-2.f*T))+FVector(0,0,Arc);
}
void UHopperPresentationComponent::TickTransfer(float Dt)
{
 const float PreviousTime=TransferTime;
 TransferTime+=Dt;
 if (PreviousTime<HatchLeadTime)
 {
  const float OpenFraction=FMath::Min(TransferTime/HatchLeadTime,1.f);
  SetHatchOpen(FMath::Lerp(TransferInitialHatch,1.f,OpenFraction));
  if (TransferTime>=HatchLeadTime && CurrentClip && !(bBoarding && LadderBoardingClip))
  {
   Pilot->SetPlayRate(CurrentClip->GetPlayLength()/(TransferDuration-HatchLeadTime));
   Pilot->SetPosition(CurrentClip->GetPlayLength()*GetPilotTravelAlpha(TransferTime),false);
  }
 }
 if (bBoarding && LadderBoardingClip)
 {
  TickLadderBoarding();
  return;
 }
 const float PilotAlpha=GetPilotTravelAlpha(TransferTime);
 const float DockDuration=GetSeatDockDuration();
 if (bBoarding && DockDuration>0.f)
 {
  // With the hatch clear, run the empty carriage out in reverse while the captain climbs.
  // Hold it at the boarding stop before contact. Both directions use the same two physical axes.
  if (TransferTime<TransferDuration-UE_KINDA_SMALL_NUMBER)
  {
   const float Extension=FMath::Min(PilotAlpha/.72f,1.f);
   Seat->SetRelativeLocation(VisualData->SeatLocation+GetSeatDockOffset(DockDuration*(1.f-Extension)));
  }
  else
  {
   const float DockTime=FMath::Max(0.f,TransferTime-TransferDuration);
   Seat->SetRelativeLocation(VisualData->SeatLocation+GetSeatDockOffset(DockTime));
   if (DockTime>=DockDuration-UE_KINDA_SMALL_NUMBER) SeatPilot();
   else if (!bSeated) AttachPilotToSeat();
   return;
  }
 }
 Pilot->SetWorldLocation(GetTransferPosition(TransferStart,TransferEnd,PilotAlpha,bBoarding));
 if (bBoarding && Seat) Pilot->SetWorldRotation(Seat->GetComponentQuat());
}
void UHopperPresentationComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
 Super::TickComponent(Dt,TickType,TickFunction);
 if (!VisualData || !BodyRoot || !FMath::IsFinite(Dt) || Dt<=0) return;
 if (BlastFlashRemaining>0.f) FadeBlastFlash(Dt);
 ActionRemaining=FMath::Max(0.f,ActionRemaining-Dt);
 if (EjectionTime>=0.f)
 {
  TickCockpitShield(Dt);
  EjectionTime+=Dt;
  UpdateEjectionHatch();
  const float LaunchTime=GetEjectionLaunchTime();
  if (!bSeatFlightFinished) TickSeatEjection(LaunchTime);
  if (bSeatLaunched && bWreckBurstPlayed) UpdateWreckBurn(EjectionTime-LaunchTime);
  if (bSeatFlightFinished && HatchOpen>=1.f && ScorchedHullMaterials.IsEmpty() && (!bWreckBurstPlayed || bWreckFireStarted))
   SetComponentTickEnabled(false);
  return;
 }
 if (bTransferring)
 {
  TickTransfer(Dt);
  if (!bTransferring && bBoarding)
   Dt=FMath::Clamp(TransferTime-GetBoardingClimbEndTime(TransferDuration)-GetSeatDockDuration(),0.f,Dt);
 }
 if (bMechActive && Dt>0.f) TickMech(Dt);
 float ShieldDt=Dt;
 if (bSeated && !bWreck && !bTransferring && HatchOpen>0)
 {
  const float CloseRate=VisualData->HatchTravelSeconds>0.f ? 1.f/VisualData->HatchTravelSeconds : 1.8f;
  ShieldDt=FMath::Max(0.f,Dt-HatchOpen/CloseRate);
  SetHatchOpen(FMath::Max(0.f,HatchOpen-Dt*CloseRate));
 }
 TickCockpitShield(ShieldDt);
}
void UHopperPresentationComponent::TickMech(float Dt)
{
 const ACharacter* Character=Cast<ACharacter>(GetOwner());
 if (!Character) return;
 BodyCrouch=FMath::FInterpTo(BodyCrouch,10.f,Dt,4.f);
 const FTransform ActorWorld=GetOwner()->GetActorTransform();
 const FVector GroundOrigin=ActorWorld.TransformPosition(FVector(0,0,-CapsuleHalfHeight()));
 const FTransform GroundWorld(ActorWorld.GetRotation(),GroundOrigin);
 const FVector HipCenter=(VisualData->LeftHip+VisualData->RightHip)*.5;
 const FTransform HipWorld(ActorWorld.GetRotation(),GroundWorld.TransformPosition(HipCenter-FVector(0,0,BodyCrouch)));
 const FVector Hips[2]={VisualData->LeftHip,VisualData->RightHip};
 const FVector Knees[2]={VisualData->LeftKnee,VisualData->RightKnee};
 const FVector Ankles[2]={VisualData->LeftAnkle,VisualData->RightAnkle};
 FVector Targets[2],Normals[2]; bool ValidGround=true;
 FCollisionQueryParams Params(SCENE_QUERY_STAT(HopperFeet),false,GetOwner());
 for (int32 I=0;I<2;++I)
 {
  const FVector Prediction=FVector(Character->GetVelocity().X,Character->GetVelocity().Y,0)*.22f;
  const FVector Neutral=GroundWorld.TransformPosition(FVector(Ankles[I].X,Ankles[I].Y,0))+Prediction.GetClampedToMaxSize(24.f);
  FHitResult Hit;
  const bool Found=GetWorld()->LineTraceSingleByChannel(Hit,Neutral+FVector(0,0,100),Neutral-FVector(0,0,140),ECC_Visibility,Params) &&
   Hit.ImpactNormal.Z>=Character->GetCharacterMovement()->GetWalkableFloorZ();
  Normals[I]=Found?Hit.ImpactNormal:FVector::UpVector;
  Targets[I]=(Found?Hit.ImpactPoint:Neutral)+Normals[I]*Ankles[I].Z;
  ValidGround &= Found;
  Gait.Settings.HipOffsets[I]=Hips[I]-HipCenter;
 }
 Gait.Settings.MaxLegReach=FVector::Distance(Hips[0],Knees[0])+FVector::Distance(Knees[0],Ankles[0])-.2f;
 Gait.Settings.PredictionTime=0; // Ground was sampled at the predicted landing point, including steps and slopes.
 Gait.Settings.StepDuration=StepDuration; Gait.Settings.StepHeight=StepHeight; Gait.Settings.StrideTrigger=StrideTrigger;
 Gait.Update(Dt,HipWorld,Character->GetVelocity(),Targets[0],Normals[0],Targets[1],Normals[1],
  Character->GetCharacterMovement()->IsMovingOnGround() && ValidGround);
 BodyRoot->SetRelativeLocation(FVector(0,0,-CapsuleHalfHeight()-BodyCrouch)+Gait.BodyOffset);
 BodyRoot->SetRelativeRotation(FRotator(Gait.PitchLeanDegrees,0,Gait.RollLeanDegrees));
 const FTransform BodyWorld=BodyRoot->GetComponentTransform();
 ApplyLeg(0,BodyWorld); ApplyLeg(1,BodyWorld);
 for (const TunaSweeperHopper::FGaitFoot& Foot:Gait.Feet)
 {
  if (!Foot.bTouchdown) continue;
  if (FootfallSound) UGameplayStatics::PlaySoundAtLocation(this,FootfallSound,Foot.Position,FootfallVolume,.8f);
  if (UTunaSweeperNoiseSubsystem* Noise=GetWorld()->GetSubsystem<UTunaSweeperNoiseSubsystem>())
   Noise->ReportNoiseAtLocation(Foot.Position,.8f,2400.f,TEXT("noise.enemy_footstep"),GetOwner(),GetOwner());
 }
}
void UHopperPresentationComponent::ApplyLeg(int32 Side,const FTransform& BodyWorld)
{
 const FVector Hip=Side?VisualData->RightHip:VisualData->LeftHip;
 const FVector Knee=Side?VisualData->RightKnee:VisualData->LeftKnee;
 const FVector Ankle=Side?VisualData->RightAnkle:VisualData->LeftAnkle;
 const auto Solved=TunaSweeperHopper::SolveTwoBone(BodyWorld.TransformPosition(Hip),Gait.Feet[Side].Position,
  BodyWorld.TransformVectorNoScale(FVector(-1,0,0)),FVector::Distance(Hip,Knee),FVector::Distance(Knee,Ankle));
 const FVector NewKnee=BodyWorld.InverseTransformPosition(Solved.Knee);
 const FVector NewAnkle=BodyWorld.InverseTransformPosition(Solved.Foot);
 const FQuat Upper=FQuat::FindBetweenNormals((Knee-Hip).GetSafeNormal(),(NewKnee-Hip).GetSafeNormal());
 const FQuat Lower=FQuat::FindBetweenNormals((Ankle-Knee).GetSafeNormal(),(NewAnkle-NewKnee).GetSafeNormal());
 const FQuat Foot=BodyWorld.GetRotation().Inverse()*Gait.Feet[Side].Rotation;
 const FString Prefix=Side?TEXT("Leg_R_"):TEXT("Leg_L_");
 for (const auto& Pair:Parts)
 {
  const FString Name=Pair.Key.ToString(); if (!Name.StartsWith(Prefix)) continue;
  const int32 Index=FCString::Atoi(*Name.Mid(6,2));
  FQuat Rotation; FVector OldPivot,NewPivot;
  if (Index<=3) { Rotation=Upper;OldPivot=Hip;NewPivot=Hip; }
  else if (Index<=6) { Rotation=Lower;OldPivot=Knee;NewPivot=NewKnee; }
  else { Rotation=Foot;OldPivot=Ankle;NewPivot=NewAnkle; }
  Pair.Value->SetRelativeTransform(FTransform(Rotation,NewPivot-Rotation.RotateVector(OldPivot)));
 }
}
void UHopperPresentationComponent::BeginPlay()
{
 Super::BeginPlay();
 PreloadWreckEffects();
 // Runtime components must survive editor property reconstruction during PIE.
 TArray<USceneComponent*> Owned; GetOwner()->GetComponents(Owned);
 for (USceneComponent* Component:Owned)
  if (Component->ComponentHasTag(TEXT("HopperGenerated"))) Component->CreationMethod=EComponentCreationMethod::Instance;
 for (const auto& Pair:Parts)
  if (Pair.Key.ToString().StartsWith(TEXT("Arm_"))) Pair.Value->SetHiddenInGame(true);
}
void UHopperPresentationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
 if (ReleasedGuard.IsValid()) ReleasedGuard->Destroy();
 WreckAssetHandle.Reset();
 ClearEjectionExhaust();
 // Detached components remain owned by the enemy, so actor destruction cleans the wreck too.
 Super::EndPlay(Reason);
}
