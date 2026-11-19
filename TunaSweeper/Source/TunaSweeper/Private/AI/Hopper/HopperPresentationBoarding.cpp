#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
 // Contacts and toe-offs are normalized to the authored ladder animation.
 constexpr float LowerToeOff=.08f, LowerContact=.30f;
 constexpr float MiddleToeOff=.40f, MiddleContact=.62f;
 constexpr float SeatToeOff=.72f, SeatContact=.91f;

 FVector JumpArc(const FVector& From,const FVector& To,float Alpha,float Start,float End,double Clearance)
 {
  const double T=FMath::Clamp((Alpha-Start)/(End-Start),0.f,1.f);
  const double Apex=FMath::Max(From.Z,To.Z)+Clearance;
  const double Curvature=FMath::Square(FMath::Sqrt(Apex-From.Z)+FMath::Sqrt(Apex-To.Z));
  FVector Position=FMath::Lerp(From,To,T);
  Position.Z+=Curvature*T*(1.-T);
  return Position;
 }

 float ClearanceArc(float Start,float End,float T)
 {
  return FMath::Sin(PI*FMath::Clamp((T-Start)/(End-Start),0.f,1.f));
 }

 FVector JumpIntoSeat(const FVector& Tread,const FVector& Pickup,float Alpha)
 {
  const float T=FMath::Clamp((Alpha-SeatToeOff)/(SeatContact-SeatToeOff),0.f,1.f);
  FVector Position=JumpArc(Tread,Pickup,Alpha,SeatToeOff,SeatContact,24.);
  // Push off the inclined rung away from the pedals. Move through the space
  // between the grips after gaining height, then lower the boots onto the footrest.
  Position.X=FMath::Lerp(Tread.X,Pickup.X,FMath::SmoothStep(.20f,.66f,T))
   +6.f*ClearanceArc(0.f,.70f,T)+3.f*ClearanceArc(.52f,1.f,T);
  Position.Z+=2.f*ClearanceArc(.64f,1.f,T);
  return Position;
 }
}

float UHopperPresentationComponent::GetBoardingClimbEndTime(float ClimbSeconds) const
{
 if (!LadderBoardingClip) return ClimbSeconds;
 const float Duration=FMath::Max(.05f,ClimbSeconds);
 const float Opening=bBoarding ? HatchLeadTime : FMath::Min(VisualData->HatchTravelSeconds*(1.f-HatchOpen),Duration*.4f);
 return Opening+GetSeatDockDuration()+Duration;
}

void UHopperPresentationComponent::TickLadderBoarding()
{
 const float DockDuration=GetSeatDockDuration();
 const float SinceOpen=TransferTime-HatchLeadTime;
 const float SinceFirstPose=SinceOpen-DockDuration;
 const float Alpha=FMath::Clamp(SinceFirstPose/TransferDuration,0.f,1.f);
 const float SinceClimbEnd=SinceFirstPose-TransferDuration;

 // The empty carriage first runs the entire mechanism backwards to meet the top tread.
 // It remains there through all three jumps and the seated settling pose.
 const float CarriageTime=SinceOpen<DockDuration ? DockDuration-FMath::Max(0.f,SinceOpen) : FMath::Max(0.f,SinceClimbEnd);
 Seat->SetRelativeLocation(VisualData->SeatLocation+GetSeatDockOffset(CarriageTime));
 if (Alpha<1.f)
 {
  Pilot->SetPosition(Alpha*LadderBoardingClip->GetPlayLength(),false);
  if (Alpha>=SeatContact)
  {
   if (Pilot->GetAttachParent()!=Seat)
    Pilot->AttachToComponent(Seat,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
   return;
  }
  const FVector Ground(VisualData->BoardingStartDistance,0,0);
  const FVector Pickup=VisualData->SeatLocation+GetSeatDockOffset(0.f);
  FVector Point;
  if (Alpha<MiddleToeOff)
   Point=JumpArc(Ground,VisualData->LowerTreadLanding,Alpha,LowerToeOff,LowerContact,14.);
  else if (Alpha<SeatToeOff)
   Point=JumpArc(VisualData->LowerTreadLanding,VisualData->MiddleTreadLanding,Alpha,MiddleToeOff,MiddleContact,13.);
  else
   Point=JumpIntoSeat(VisualData->MiddleTreadLanding,Pickup,Alpha);
  const float TurnIn=FMath::SmoothStep(0.f,.52f,TransferTime);
  const float TurnToSeat=FMath::SmoothStep(SeatToeOff,SeatToeOff+(SeatContact-SeatToeOff)*.64f,Alpha);
  const FQuat Facing=FQuat(FVector::UpVector,FMath::DegreesToRadians(180.f*TurnIn*(1.f-TurnToSeat)));
  const FTransform& Body=BodyRoot->GetComponentTransform();
  Pilot->SetWorldLocationAndRotation(Body.TransformPosition(Point),Body.GetRotation()*Facing);
  return;
 }
 if (!bSeated) AttachPilotToSeat();
 if (SinceClimbEnd>=DockDuration-UE_KINDA_SMALL_NUMBER)
 {
  SeatPilot();
  EmitRailSparks(false);
 }
}

void UHopperPresentationComponent::EmitRailSparks(bool bEjection)
{
 if (!VisualData || !BodyRoot || GetWorld()->GetNetMode()==NM_DedicatedServer) return;
 UNiagaraSystem* Effect=bEjection?LoadedEjectSparks.Get():LoadedLockSparks.Get();
 if (!Effect) return;
 const FTransform& Chassis=BodyRoot->GetComponentTransform();
 for (const float Side:{-1.f,1.f})
 {
  const bool bVerticalRelease=bEjection && !VisualData->EjectionGuardParts.IsEmpty();
  FVector Contact=bEjection && !bVerticalRelease ? VisualData->RailExitContact : VisualData->RailLockContact;
  Contact.Y*=Side;
  if (bVerticalRelease) Contact+=EjectionSeatStart-VisualData->SeatLocation;
  FVector StrikeDirection=Chassis.GetUnitAxis(EAxis::Y)*Side;
  if (!bEjection) StrikeDirection+=Chassis.GetUnitAxis(EAxis::X)*1.25f;
  const FRotator Direction=FRotationMatrix::MakeFromXZ(StrikeDirection,Chassis.GetUnitAxis(EAxis::Z)).Rotator();
  UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),Effect,Chassis.TransformPosition(Contact),Direction,FVector::OneVector,true,true,ENCPoolMethod::None,false);
 }
}
