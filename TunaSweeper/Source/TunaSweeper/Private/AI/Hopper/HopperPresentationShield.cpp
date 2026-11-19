#include "AI/Hopper/HopperPresentationComponent.h"
#include "AI/Hopper/HopperVisualData.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

namespace
{
 constexpr float SweepSeconds=.34f;
 constexpr float SettleSeconds=1.f;
 constexpr float StableGain=.38f;
 constexpr float WarningSeconds=.96f;
 constexpr float FragmentSeconds=.78f;

 float Flash(float Time,float Center,float HalfWidth)
 {
  return 1.f-FMath::SmoothStep(0.f,HalfWidth,FMath::Abs(Time-Center));
 }
}

void UHopperPresentationComponent::InitializeCockpitShield()
{
 CockpitShield=nullptr;
 PendingDestructionEffect=nullptr;
 ShieldStage=EShieldStage::Off;
 ShieldAge=ShieldCoverage=EjectionShieldDelay=0.f;
 if (VisualData->CockpitShieldMesh.IsNull()) return;
 UStaticMesh* Mesh=VisualData->CockpitShieldMesh.LoadSynchronous();
 if (!Mesh) return;
 CockpitShield=NewObject<UStaticMeshComponent>(GetOwner(),MakeUniqueObjectName(GetOwner(),
  UStaticMeshComponent::StaticClass(),TEXT("CockpitShield")),RF_Transient);
 CockpitShield->CreationMethod=HasBegunPlay()?EComponentCreationMethod::Instance:EComponentCreationMethod::UserConstructionScript;
 CockpitShield->ComponentTags.Add(TEXT("HopperGenerated"));
 CockpitShield->ComponentTags.Add(TEXT("HopperCockpitShield"));
 CockpitShield->SetCanEverAffectNavigation(false);
 CockpitShield->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 CockpitShield->SetGenerateOverlapEvents(false);
 CockpitShield->SetCastShadow(false);
 CockpitShield->SetReceivesDecals(false);
 CockpitShield->SetupAttachment(BodyRoot);
 CockpitShield->SetStaticMesh(Mesh);
 CockpitShield->SetHiddenInGame(true);
 CockpitShield->SetCustomPrimitiveDataVector4(0,FVector4(0,0,0,0));
 CockpitShield->RegisterComponent();
}

bool UHopperPresentationComponent::IsWaitingForShieldBreak() const
{
 return ShieldStage==EShieldStage::Failing;
}

void UHopperPresentationComponent::EmitDestructionEffect(UNiagaraSystem* Effect)
{
 if (VisualData && VisualData->WreckEffects.IsConfigured())
  EmitWreckEffects();
 else if (Effect)
  UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),Effect,GetOwner()->GetActorLocation());
}

float UHopperPresentationComponent::GetShieldWarningDuration() const
{
 if (IsWaitingForShieldBreak()) return FMath::Max(0.f,WarningSeconds-ShieldAge);
 return (ShieldStage==EShieldStage::Deploying || ShieldStage==EShieldStage::Stable) && ShieldCoverage>0.f ? WarningSeconds : 0.f;
}

float UHopperPresentationComponent::StartDestructionEffects(UNiagaraSystem* MechEffect,const FVector& ImpactTravel)
{
 if (!ImpactTravel.IsNearlyZero() && !bWreckBurstPlayed) WreckSprayDirection=ImpactTravel.GetSafeNormal2D();
 if (EjectionTime>=0.f) return FMath::Max(0.f,EjectionShieldDelay-EjectionTime);
 const float Warning=GetShieldWarningDuration();
 if (IsWaitingForShieldBreak()) return Warning;
 if (Warning>0.f)
 {
  ShieldStage=EShieldStage::Failing;
  ShieldAge=0.f;
  PendingDestructionEffect=MechEffect;
  return WarningSeconds;
 }
 EmitDestructionEffect(MechEffect);
 return 0.f;
}

void UHopperPresentationComponent::TickCockpitShield(float Dt)
{
 // The pull, shield break and seat detach use the same clock, including frames that cross the trip point.
 const bool bLeverTimed=bAwaitingEjectionPull;
 if (bLeverTimed && EjectionTime>=EjectionShieldDelay)
 {
  bAwaitingEjectionPull=false;
  if (!IsWaitingForShieldBreak())
  {
   // Boarding can be interrupted before a shield has deployed.
   EmitDestructionEffect(PendingDestructionEffect);
   PendingDestructionEffect=nullptr;
  }
 }
 if (!CockpitShield) return;
 // Both material slots consume one update: swept coverage, field gain, crack spread, burst clock.
 // The burst uses 1+age so zero denotes an intact field. Shards and sparks share the same onset.
 if (ShieldStage==EShieldStage::Failing)
 {
  const float Duration=bLeverTimed ? EjectionShieldDelay : WarningSeconds;
  ShieldAge=bLeverTimed ? EjectionTime : ShieldAge+Dt;
  if (ShieldAge<Duration)
  {
   const float T=ShieldAge/Duration;
   const float Flicker=FMath::Max3(Flash(T,.15f,.12f),Flash(T,.46f,.115f),Flash(T,.79f,.14f));
   const float Cracks=bLeverTimed ? FMath::Lerp(.22f,1.f,FMath::SmoothStep(0.f,.96f,T)) : FMath::SmoothStep(.16f,.93f,T);
   const float Gain=bLeverTimed ? .30f+1.05f*Flicker : .10f+1.30f*Flicker;
   CockpitShield->SetCustomPrimitiveDataVector4(0,FVector4(ShieldCoverage,Gain,Cracks,0.f));
   return;
  }
  ShieldAge-=Duration;
  ShieldStage=EShieldStage::Fragments;
  CockpitShield->SetBoundsScale(3.f);
  EmitDestructionEffect(PendingDestructionEffect);
  PendingDestructionEffect=nullptr;
 }
 else if (ShieldStage==EShieldStage::Fragments) ShieldAge+=Dt;

 if (ShieldStage==EShieldStage::Fragments)
 {
  CockpitShield->SetCustomPrimitiveDataVector4(0,FVector4(ShieldCoverage,0.f,0.f,1.f+ShieldAge));
  if (ShieldAge<FragmentSeconds) return;
  CockpitShield->SetHiddenInGame(true);
  CockpitShield->SetBoundsScale(1.f);
  ShieldStage=EShieldStage::Off;
  ShieldCoverage=0.f;
  return;
 }

 const bool bClosed=bMechActive && bSeated && !bWreck && !bTransferring && HatchOpen<=0.f;
 if (!bClosed)
 {
  if (ShieldStage!=EShieldStage::Off)
  {
   CockpitShield->SetHiddenInGame(true);
   CockpitShield->SetCustomPrimitiveDataVector4(0,FVector4(0,0,0,0));
   ShieldCoverage=0.f;
   ShieldStage=EShieldStage::Off;
  }
  return;
 }
 if (ShieldStage==EShieldStage::Stable) return;
 if (ShieldStage==EShieldStage::Off)
 {
  ShieldStage=EShieldStage::Deploying;
  ShieldAge=0.f;
  CockpitShield->SetHiddenInGame(false);
 }
 ShieldAge+=Dt;
 ShieldCoverage=FMath::SmoothStep(0.f,SweepSeconds,ShieldAge);
 const float Gain=FMath::Lerp(1.48f,StableGain,FMath::SmoothStep(.25f,SettleSeconds,ShieldAge));
 CockpitShield->SetCustomPrimitiveDataVector4(0,FVector4(ShieldCoverage,Gain,0.f,0.f));
 if (ShieldAge>=SettleSeconds) ShieldStage=EShieldStage::Stable;
}
