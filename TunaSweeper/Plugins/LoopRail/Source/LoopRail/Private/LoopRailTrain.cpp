#include "LoopRailTrain.h"
#include "LoopRailTrack.h"
#include "LoopRailStation.h"
#include "LoopRailMotion.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameStateBase.h"
#include "Net/UnrealNetwork.h"

ALoopRailTrain::ALoopRailTrain()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PrePhysics;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    RootComponent->SetMobility(EComponentMobility::Movable);
    bReplicates = true;
    bAlwaysRelevant = true;
    SetReplicateMovement(false);
    SetNetUpdateFrequency(20);
    LocomotiveMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/LoopRail/Meshes/SM_Locomotive.SM_Locomotive")));
    CarriageMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/LoopRail/Meshes/SM_Carriage.SM_Carriage")));
    CarriageRoofMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/LoopRail/Meshes/SM_CarriageRoof.SM_CarriageRoof")));
}
void ALoopRailTrain::OnConstruction(const FTransform& Transform)
{ Super::OnConstruction(Transform); RebuildTrain(); HeadDistance = StartDistance; UpdateVehicles(); }
void ALoopRailTrain::BeginPlay()
{ Super::BeginPlay(); RebuildTrain(); if (HasAuthority()) ResetTrain(); else OnRep_State(); }
void ALoopRailTrain::RebuildTrain()
{
    // Arrays are transient; serialized construction components can still exist after map load.
    TInlineComponentArray<USceneComponent*> Existing(this);
    for (USceneComponent* C : Existing) if (C->ComponentHasTag(TEXT("LoopRailVehicle"))) C->DestroyComponent();
    Visuals.Reset(); Floors.Reset();
    VehicleCount = FMath::Clamp(VehicleCount, 1, 16);
    VehicleGap = FMath::Max(0.f, VehicleGap);
    UStaticMesh* EngineMesh = LocomotiveMesh.LoadSynchronous();
    UStaticMesh* CoachMesh = CarriageMesh.LoadSynchronous();
    UStaticMesh* RoofMesh = bShowCarriageRoofs ? CarriageRoofMesh.LoadSynchronous() : nullptr;
    for (int32 I = 0; I < VehicleCount; ++I)
    {
        UBoxComponent* Floor = NewObject<UBoxComponent>(this, *FString::Printf(TEXT("VehicleFloor_%02d"), I));
        Floor->CreationMethod = EComponentCreationMethod::UserConstructionScript;
        Floor->ComponentTags.Add(TEXT("LoopRailVehicle"));
        Floor->SetNetAddressable();
        Floor->SetupAttachment(RootComponent);
        Floor->SetMobility(EComponentMobility::Movable);
        Floor->SetBoxExtent(FVector(VehicleLength*.5, VehicleWidth*.5, 10));
        Floor->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        Floor->SetCanEverAffectNavigation(false);
        Floor->CanCharacterStepUpOn = ECB_Yes;
        Floor->RegisterComponent(); Floors.Add(Floor);
        UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("VehicleMesh_%02d"), I));
        Mesh->CreationMethod = EComponentCreationMethod::UserConstructionScript;
        Mesh->ComponentTags.Add(TEXT("LoopRailVehicle"));
        Mesh->SetNetAddressable();
        Mesh->SetupAttachment(Floor);
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(I == 0 ? EngineMesh : CoachMesh);
        Mesh->SetRelativeLocation(FVector(0,0,-50));
        Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->CanCharacterStepUpOn = ECB_Yes;
        Mesh->RegisterComponent(); Visuals.Add(Mesh);
        if (I > 0 && RoofMesh)
        {
            auto* Roof = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("VehicleRoof_%02d"), I));
            Roof->CreationMethod = EComponentCreationMethod::UserConstructionScript;
            Roof->ComponentTags.Add(TEXT("LoopRailVehicle"));
            Roof->SetupAttachment(Floor); Roof->SetMobility(EComponentMobility::Movable);
            Roof->SetStaticMesh(RoofMesh); Roof->SetRelativeLocation(FVector(0,0,-50));
            Roof->SetCollisionEnabled(ECollisionEnabled::NoCollision); Roof->SetCanEverAffectNavigation(false);
            Roof->RegisterComponent(); Visuals.Add(Roof);
        }
    }
    UpdateVehicles();
}
void ALoopRailTrain::ResetTrain()
{
    HeadDistance = IsValid(Track) ? LoopRailMotion::Wrap(StartDistance, Track->GetLength()) : StartDistance;
    CurrentSpeed = 0; DwellRemaining = 0; SinceDeparture = 100; CurrentStation = nullptr;
    bRunning = bAutoRun;
    UpdateVehicles();
    if (HasAuthority()) PublishState();
}
void ALoopRailTrain::SetRunning(bool bEnabled)
{
    if (!HasAuthority()) return;
    bRunning = bEnabled;
    if (!bEnabled) CurrentSpeed = 0;
    PublishState(); ForceNetUpdate();
}
double ALoopRailTrain::GetConsistLength() const
{ const int32 Count = FMath::Clamp(VehicleCount, 1, 16); return VehicleLength*Count + FMath::Max(0.f, VehicleGap)*(Count-1); }
bool ALoopRailTrain::HasUsableTrack() const
{ return IsValid(Track) && Track->IsUsableLoop() && Track->GetLength() > GetConsistLength() + 100; }
UBoxComponent* ALoopRailTrain::GetVehicleFloor(int32 Index) const { return Floors.IsValidIndex(Index) ? Floors[Index].Get() : nullptr; }
void ALoopRailTrain::UpdateVehicles()
{
    for (int32 I = 0; I < Floors.Num(); ++I)
    {
        FTransform Pose;
        const double D = HeadDistance - I * (VehicleLength + FMath::Max(0.f, VehicleGap));
        if (IsValid(Track) && Track->GetLength() > 1)
        {
            Pose = Track->Sample(D);
            // Both bogie positions determine the car body orientation on bends.
            const FVector Front = Track->Sample(D+300).GetLocation(), Back = Track->Sample(D-300).GetLocation();
            if (!(Front-Back).IsNearlyZero()) Pose.SetRotation(FRotationMatrix::MakeFromXZ(Front-Back, Pose.GetUnitAxis(EAxis::Z)).ToQuat());
            Pose.SetLocation((Front+Back)*.5);
        }
        else { Pose = GetActorTransform(); Pose.AddToTranslation(Pose.TransformVectorNoScale(FVector(-I*(VehicleLength+VehicleGap),0,0))); }
        Pose.AddToTranslation(Pose.GetRotation().RotateVector(FVector(0,0,50)));
        Floors[I]->SetWorldTransform(Pose, false, nullptr, ETeleportType::None);
    }
}
ALoopRailStation* ALoopRailTrain::FindNextStation(double& Distance) const
{
    Distance = TNumericLimits<double>::Max();
    ALoopRailStation* Best = nullptr;
    for (TActorIterator<ALoopRailStation> It(GetWorld()); It; ++It)
    {
        if (!It->bEnabled || It->Track != Track || (CurrentStation == *It && SinceDeparture < 5)) continue;
        const double D = LoopRailMotion::Forward(HeadDistance, It->GetStopDistance(), Track->GetLength());
        if (D < Distance) { Distance = D; Best = *It; }
    }
    return Best;
}
void ALoopRailTrain::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasUsableTrack()) { CurrentSpeed = 0; if (HasAuthority()) PublishState(); return; }
    if (!HasAuthority())
    {
        if (const AGameStateBase* GS = GetWorld()->GetGameState())
        {
            const double Age = FMath::Clamp(GS->GetServerWorldTimeSeconds() - RepState.ServerTime, 0.0, .25);
            HeadDistance = LoopRailMotion::Wrap(RepState.Distance + (RepState.bMoving ? RepState.Speed*Age : 0), Track->GetLength());
        }
        UpdateVehicles(); return;
    }
    // Bounded substeps avoid station skips after a frame hitch. Excess time is deliberately discarded.
    double RemainingTime = FMath::Clamp(static_cast<double>(DeltaSeconds), 0.0, 1.0);
    while (RemainingTime > UE_SMALL_NUMBER && bRunning)
    {
        const double Dt = FMath::Min(RemainingTime, .02); RemainingTime -= Dt;
        if (DwellRemaining > 0)
        {
            DwellRemaining = FMath::Max(0.f, DwellRemaining-static_cast<float>(Dt));
            if (DwellRemaining == 0) { SinceDeparture = 0; OnStationDeparted.Broadcast(CurrentStation); }
            continue;
        }
        double DistanceToStop;
        ALoopRailStation* Next = FindNextStation(DistanceToStop);
        const auto Step = LoopRailMotion::Advance(CurrentSpeed, CruiseSpeed, Acceleration, BrakingDeceleration, DistanceToStop, Dt);
        HeadDistance = LoopRailMotion::Wrap(HeadDistance+Step.Distance, Track->GetLength());
        SinceDeparture += Step.Distance;
        CurrentSpeed = Step.Speed;
        if (Next && Step.Arrived)
        {
            CurrentStation = Next; DwellRemaining = FMath::Max(0.f, Next->DwellSeconds); SinceDeparture = 0;
            OnStationArrived.Broadcast(Next);
            if (DwellRemaining == 0) OnStationDeparted.Broadcast(Next);
        }
    }
    UpdateVehicles(); PublishState();
}
void ALoopRailTrain::PublishState()
{
    RepState.Distance = HeadDistance; RepState.Speed = CurrentSpeed;
    RepState.ServerTime = GetWorld()->GetTimeSeconds(); RepState.bMoving = bRunning && DwellRemaining <= 0;
    RepState.DwellSeconds = DwellRemaining;
}
void ALoopRailTrain::OnRep_State() { HeadDistance = RepState.Distance; CurrentSpeed = RepState.Speed; DwellRemaining = RepState.DwellSeconds; UpdateVehicles(); }
void ALoopRailTrain::OnRep_Configuration() { RebuildTrain(); OnRep_State(); }
void ALoopRailTrain::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ALoopRailTrain, Track); DOREPLIFETIME(ALoopRailTrain, VehicleCount);
    DOREPLIFETIME(ALoopRailTrain, VehicleGap); DOREPLIFETIME(ALoopRailTrain, RepState);
}
