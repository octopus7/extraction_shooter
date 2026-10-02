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
    ConnectionMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/LoopRail/Meshes/SM_CarriageConnection.SM_CarriageConnection")));
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
    ConnectionFloors.Reset(); ConnectionGuards.Reset(); ConnectionVisuals.Reset(); ConnectionGuardVisuals.Reset();
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
    UStaticMesh* BridgeMesh = VehicleCount > 1 ? ConnectionMesh.LoadSynchronous() : nullptr;
    bUsesFallbackConnectionMesh = BridgeMesh == nullptr;
    if (VehicleCount > 1 && !BridgeMesh)
        BridgeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (int32 I = 0; I + 1 < VehicleCount; ++I)
    {
        auto* Floor = NewObject<UBoxComponent>(this, *FString::Printf(TEXT("ConnectionFloor_%02d"), I));
        Floor->CreationMethod = EComponentCreationMethod::UserConstructionScript;
        Floor->ComponentTags.Add(TEXT("LoopRailVehicle"));
        Floor->ComponentTags.Add(TEXT("LoopRailConnectionFloor"));
        Floor->SetNetAddressable();
        Floor->SetupAttachment(RootComponent);
        Floor->SetMobility(EComponentMobility::Movable);
        Floor->SetBoxExtent(FVector(20, 50, 10));
        Floor->SetCollisionProfileName(TEXT("BlockAllDynamic"));
        Floor->SetCanEverAffectNavigation(false);
        Floor->CanCharacterStepUpOn = ECB_Yes;
        Floor->RegisterComponent(); ConnectionFloors.Add(Floor);

        auto* Mesh = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("ConnectionMesh_%02d"), I));
        Mesh->CreationMethod = EComponentCreationMethod::UserConstructionScript;
        Mesh->ComponentTags.Add(TEXT("LoopRailVehicle"));
        Mesh->SetupAttachment(Floor);
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(BridgeMesh);
        // Authored connection: 100cm long, deck top at local Z=0. Floor collision top is Z=10.
        Mesh->SetRelativeLocation(FVector(0, 0, bUsesFallbackConnectionMesh ? 5 : 10));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->RegisterComponent(); ConnectionVisuals.Add(Mesh);

        for (int32 Side = 0; Side < 2; ++Side)
        {
            auto* Guard = NewObject<UBoxComponent>(this, *FString::Printf(TEXT("ConnectionGuard_%02d_%d"), I, Side));
            Guard->CreationMethod = EComponentCreationMethod::UserConstructionScript;
            Guard->ComponentTags.Add(TEXT("LoopRailVehicle"));
            Guard->ComponentTags.Add(TEXT("LoopRailConnectionGuard"));
            Guard->SetNetAddressable();
            Guard->SetupAttachment(Floor);
            Guard->SetMobility(EComponentMobility::Movable);
            Guard->SetBoxExtent(FVector(20, 2, 45));
            Guard->SetRelativeLocation(FVector(0, Side == 0 ? -48 : 48, 55));
            Guard->SetCollisionProfileName(TEXT("BlockAllDynamic"));
            Guard->SetCanEverAffectNavigation(false);
            Guard->CanCharacterStepUpOn = ECB_No;
            Guard->RegisterComponent(); ConnectionGuards.Add(Guard);
            if (bUsesFallbackConnectionMesh)
            {
                auto* Rail = NewObject<UStaticMeshComponent>(this, *FString::Printf(TEXT("ConnectionRail_%02d_%d"), I, Side));
                Rail->CreationMethod = EComponentCreationMethod::UserConstructionScript;
                Rail->ComponentTags.Add(TEXT("LoopRailVehicle"));
                Rail->SetupAttachment(Guard);
                Rail->SetMobility(EComponentMobility::Movable);
                Rail->SetStaticMesh(BridgeMesh);
                Rail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                Rail->SetCanEverAffectNavigation(false);
                Rail->RegisterComponent(); ConnectionGuardVisuals.Add(Rail);
            }
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
UBoxComponent* ALoopRailTrain::GetConnectionFloor(int32 Index) const { return ConnectionFloors.IsValidIndex(Index) ? ConnectionFloors[Index].Get() : nullptr; }
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
    UpdateConnections();
}
void ALoopRailTrain::UpdateConnections()
{
    for (int32 I = 0; I < ConnectionFloors.Num() && I + 1 < Floors.Num(); ++I)
    {
        const FTransform Leading = Floors[I]->GetComponentTransform();
        const FTransform Following = Floors[I + 1]->GetComponentTransform();
        const FVector RearEnd = Leading.TransformPosition(FVector(-VehicleLength * .5, 0, 0));
        const FVector FrontEnd = Following.TransformPosition(FVector(VehicleLength * .5, 0, 0));
        const FVector Span = RearEnd - FrontEnd;
        FVector Forward = Span.GetSafeNormal();
        if (Forward.IsNearlyZero()) Forward = Leading.GetUnitAxis(EAxis::X);
        const FVector Up = (Leading.GetUnitAxis(EAxis::Z) + Following.GetUnitAxis(EAxis::Z)).GetSafeNormal();
        const FQuat Rotation = FRotationMatrix::MakeFromXZ(Forward, Up).ToQuat();
        // Overlap both decks by 20cm so changes in yaw/pitch do not expose a walkable seam.
        const double HalfLength = Span.Size() * .5 + 20;
        ConnectionFloors[I]->SetBoxExtent(FVector(HalfLength, 50, 10), false);
        ConnectionFloors[I]->SetWorldTransform(FTransform(Rotation, (RearEnd + FrontEnd) * .5), false, nullptr, ETeleportType::None);
        ConnectionVisuals[I]->SetRelativeScale3D(FVector(HalfLength * 2 / 100, 1, bUsesFallbackConnectionMesh ? .1 : 1));
        for (int32 Side = 0; Side < 2; ++Side)
        {
            const int32 GuardIndex = I * 2 + Side;
            ConnectionGuards[GuardIndex]->SetBoxExtent(FVector(HalfLength, 2, 45), false);
            if (ConnectionGuardVisuals.IsValidIndex(GuardIndex))
                ConnectionGuardVisuals[GuardIndex]->SetRelativeScale3D(FVector(HalfLength * 2 / 100, .04, .9));
        }
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
