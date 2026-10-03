#include "AI/Hopper/HopperArmModule.h"
#include "AI/Hopper/HopperEnemyCharacter.h"
#include "AI/Hopper/HopperVisualData.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "UObject/ConstructorHelpers.h"

AHopperArmModule::AHopperArmModule()
{
    PrimaryActorTick.bCanEverTick = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("ArmRoot")));
    Muzzle = CreateDefaultSubobject<USceneComponent>(TEXT("Muzzle"));
    Muzzle->SetupAttachment(RootComponent);
    Muzzle->SetRelativeLocation(FVector(0,0,-91));
    CannonNozzle = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CannonNozzle"));
    CannonNozzle->SetupAttachment(RootComponent);
    CannonNozzle->SetRelativeLocation(FVector(0,0,-80));
    CannonNozzle->SetRelativeScale3D(FVector(.11f,.11f,.22f));
    CannonNozzle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CannonNozzle->SetCanEverAffectNavigation(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cylinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (Cylinder.Succeeded()) CannonNozzle->SetStaticMesh(Cylinder.Object);
    ProjectileClass = ATunaSweeperProjectile::StaticClass();
}

void AHopperArmModule::ConfigureVisual(UHopperVisualData* Data, bool bLeft)
{
    for (UStaticMeshComponent* Part : VisualParts) if (Part) Part->DestroyComponent();
    VisualParts.Reset();
    VisualData = Data;
    CannonNozzle->SetVisibility(bShowCannonNozzle);
    if (!Data || !bUseHopperArmVisuals) return;
    const FString Prefix = bLeft ? TEXT("Arm_L_") : TEXT("Arm_R_");
    const FVector Mount = bLeft ? Data->LeftArmMount : Data->RightArmMount;
    for (const FHopperMeshPart& Part : Data->Parts)
    {
        if (!Part.PartName.ToString().StartsWith(Prefix)) continue;
        UStaticMesh* Mesh = Part.Mesh.LoadSynchronous();
        if (!Mesh) continue;
        UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(this);
        // SetStaticMesh can register navigation data even before RegisterComponent.
        Component->SetCanEverAffectNavigation(false);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetupAttachment(RootComponent);
        Component->SetStaticMesh(Mesh);
        Component->SetRelativeLocation(-Mount);
        Component->RegisterComponent();
        VisualParts.Add(Component);
    }
}

bool AHopperArmModule::IsReady() const
{
    return !IsActorBeingDestroyed() && GetWorld() && GetWorld()->GetTimeSeconds() >= NextFireTime;
}

void AHopperArmModule::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GetWorld()->GetTimeSeconds() >= RestorePoseTime)
        RootComponent->SetRelativeRotation(FMath::QInterpTo(RootComponent->GetRelativeRotation().Quaternion(), FQuat::Identity, DeltaSeconds, 5.f));
}

bool AHopperArmModule::TryFireAt(AActor* TargetActor)
{
    ATunaSweeperHopperEnemyCharacter* Hopper = Cast<ATunaSweeperHopperEnemyCharacter>(GetOwner());
    if (!IsReady() || !IsValid(TargetActor) || !Hopper || Hopper->GetCombatPhase() != EHopperCombatPhase::Mech
        || !Hopper->CanAttackHopperTarget(TargetActor)) return false;
    const FVector AimPoint = TargetActor->GetActorLocation();
    const USceneComponent* Parent = RootComponent->GetAttachParent();
    const FVector Direction = (AimPoint - GetActorLocation()).GetSafeNormal();
    const FVector LocalDirection = Parent ? Parent->GetComponentTransform().InverseTransformVectorNoScale(Direction) : Direction;
    RootComponent->SetRelativeRotation(FQuat::FindBetweenNormals(-FVector::UpVector, LocalDirection));
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HopperArmFire), true, Hopper);
    Query.AddIgnoredActor(this);
    if (GetWorld()->LineTraceSingleByChannel(Hit, Muzzle->GetComponentLocation(), AimPoint, ECC_Visibility, Query)
        && Hit.GetActor() != TargetActor) return false;
    if (!FireWeapon(TargetActor, AimPoint)) return false;
    NextFireTime = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, CooldownSeconds);
    RestorePoseTime = GetWorld()->GetTimeSeconds() + .4;
    return true;
}

bool AHopperArmModule::FireWeapon_Implementation(AActor* TargetActor, const FVector& AimPoint)
{
    if (!ProjectileClass) return false;
    const FVector Location = Muzzle->GetComponentLocation();
    const FTransform Transform((AimPoint - Location).Rotation(), Location);
    ATunaSweeperProjectile* Projectile = GetWorld()->SpawnActorDeferred<ATunaSweeperProjectile>(
        ProjectileClass, Transform, GetOwner(), Cast<APawn>(GetOwner()), ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Projectile) return false;
    Projectile->SetDamageAmount(Damage);
    Projectile->SetHitEffectId(TEXT("hit.red_burst"));
    Projectile->IgnoreActor(GetOwner());
    Projectile->IgnoreActor(this);
    Projectile->SetAimIntent(TargetActor, nullptr, AimPoint, true);
    Projectile->FinishSpawning(Transform);
    return true;
}
