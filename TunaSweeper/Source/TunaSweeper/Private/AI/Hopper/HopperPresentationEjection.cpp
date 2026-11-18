#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
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
 float Smooth(float Alpha) { Alpha=FMath::Clamp(Alpha,0.f,1.f); return Alpha*Alpha*(3.f-2.f*Alpha); }
}

void UHopperPresentationComponent::StartSeatEjection()
{
 if (EjectionTime>=0.f || !Seat || !Pilot) return;
 EjectionShieldDelay=StartDestructionEffects(nullptr);
 // Defeat can interrupt boarding. Re-seat the pilot before moving the entire cabin assembly.
 SeatPilot();
 bWreck=true; bMechActive=false; bTransferring=false; bBoarding=false;
 SetGunVisible(false);
 EjectionTime=0.f; EjectionInitialHatch=HatchOpen; bSeatLaunched=false; bRailReleaseFlashed=false;
 EjectionSeatStart=Seat->GetRelativeLocation();
 if (auto* Cabin=Parts.Find(TEXT("Cabin_Interior")))
  (*Cabin)->AttachToComponent(Seat,FAttachmentTransformRules::KeepWorldTransform);
 SetComponentTickEnabled(true);
}

FVector UHopperPresentationComponent::EjectionPosition(float FlightTime) const
{
 // World-up ascent keeps the escape clear even when the wreck is leaning on uneven ground.
 return EjectionLaunchTransform.GetLocation()+EjectionLaunchTransform.GetUnitAxis(EAxis::X)*EjectionForwardSpeed*FlightTime+
  FVector::UpVector*(EjectionUpSpeed*FlightTime+.5f*EjectionUpAcceleration*FlightTime*FlightTime);
}

float UHopperPresentationComponent::GetEjectionHatchDuration() const
{
 const float FullStroke=FMath::Max3(.05f,EjectionHatchSeconds,VisualData ? VisualData->HatchTravelSeconds : 0.f);
 const float RemainingStroke=GetSeatDockDuration()>0.f ? 1.f-EjectionInitialHatch : 1.f;
 return EjectionShieldDelay+FullStroke*RemainingStroke;
}
float UHopperPresentationComponent::GetEjectionLaunchTime() const
{
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

void UHopperPresentationComponent::TickSeatEjection(float Dt)
{
 if (!Seat || !Pilot) return;
 EjectionTime+=Dt;
 const float HatchDuration=GetEjectionHatchDuration();
 const float SlideDuration=FMath::Max(.05f,EjectionSlideSeconds);
 const float LaunchTime=GetEjectionLaunchTime();
 const float SlideStart=LaunchTime-SlideDuration;
 const float FlightDuration=FMath::Max(.1f,EjectionFlightSeconds);
 const float OpeningDuration=HatchDuration-EjectionShieldDelay;
 const float OpeningAlpha=OpeningDuration>UE_SMALL_NUMBER ? (EjectionTime-EjectionShieldDelay)/OpeningDuration : 1.f;
 SetHatchOpen(FMath::Lerp(EjectionInitialHatch,1.f,Smooth(OpeningAlpha)));
 if (EjectionTime<HatchDuration) return;
 if (!bSeatLaunched)
 {
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
  if (EjectionTime<LaunchTime) return;
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
   SetComponentTickEnabled(false);
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
  SetComponentTickEnabled(false);
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
