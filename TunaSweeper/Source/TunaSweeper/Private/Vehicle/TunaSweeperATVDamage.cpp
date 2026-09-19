#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

void ATunaSweeperATVActor::CreateDamageComponents()
{
	LightDamageSmoke = CreateDefaultSubobject<UNiagaraComponent>(TEXT("LightDamageSmoke"));
	HeavyDamageSmoke = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HeavyDamageSmoke"));
	for (auto* Smoke : {LightDamageSmoke.Get(), HeavyDamageSmoke.Get()})
	{
		Smoke->SetupAttachment(VehicleMesh);
		Smoke->SetRelativeLocation(FVector(-15, 0, 65));
		// Only the source follows the vehicle. Already emitted particles remain in world space.
		Smoke->SetAbsolute(false, true, true);
		Smoke->SetRelativeScale3D(FVector(0.55f));
		Smoke->SetAutoActivate(false);
		Smoke->SetCanEverAffectNavigation(false);
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Light(TEXT("/Game/Effects/ATV/NS_ATV_DamageSmokeLight.NS_ATV_DamageSmokeLight"));
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Heavy(TEXT("/Game/Effects/ATV/NS_ATV_DamageSmokeHeavy.NS_ATV_DamageSmokeHeavy"));
	LightDamageSmoke->SetAsset(Light.Object);
	HeavyDamageSmoke->SetAsset(Heavy.Object);
	DestructionExplosion = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DestructionExplosion"));
	DestructionExplosion->SetupAttachment(VehicleMesh);
	DestructionExplosion->SetRelativeLocation(FVector(-15, 0, 65));
	DestructionExplosion->SetAbsolute(false, true, true);
	DestructionExplosion->SetAutoActivate(false);
	DestructionExplosion->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> Explosion(TEXT("/Game/Effects/ExplosionTuna/NS_Explosion_Tuna.NS_Explosion_Tuna"));
	DestructionExplosion->SetAsset(Explosion.Object);
	DestructionExplosionSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Audio/Imported/SW_barrel_explosion.SW_barrel_explosion")));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Front(TEXT("/Game/Meshes/Props/ATV/Debris/SM_ATV_Debris_wheel_FL.SM_ATV_Debris_wheel_FL"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Rear(TEXT("/Game/Meshes/Props/ATV/Debris/SM_ATV_Debris_wheel_RR.SM_ATV_Debris_wheel_RR"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Bar(TEXT("/Game/Meshes/Props/ATV/Debris/SM_ATV_Debris_handlebar.SM_ATV_Debris_handlebar"));
	DetachedPartMeshes = {Front.Object, Rear.Object, Bar.Object};
}

float ATunaSweeperATVActor::GetDurabilityRatio() const
{
	return FMath::Clamp(CurrentDurability / FMath::Max(1.0f, MaxDurability), 0.0f, 1.0f);
}

ETunaSweeperATVDamageState ATunaSweeperATVActor::GetDamageState() const
{
	if (bVehicleDestroyed) return ETunaSweeperATVDamageState::Destroyed;
	if (GetDurabilityRatio() <= HeavySmokeDurabilityRatio) return ETunaSweeperATVDamageState::Critical;
	if (GetDurabilityRatio() <= SmokeDurabilityRatio) return ETunaSweeperATVDamageState::Damaged;
	return ETunaSweeperATVDamageState::Healthy;
}

float ATunaSweeperATVActor::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	if (bVehicleDestroyed || bProcessingDamage || !CanBeDamaged() || !FMath::IsFinite(DamageAmount) || DamageAmount <= 0) return 0;
	TGuardValue<bool> Guard(bProcessingDamage, true);
	const float Applied = FMath::Min(DamageAmount, CurrentDurability);
	CurrentDurability = FMath::Max(0.0f, CurrentDurability - Applied);
	HitSmokeRemaining = FMath::Max(0.0f, HitSmokeDuration);
	if (CurrentDurability <= 0) DestroyVehicle();
	UpdateDamageSmoke(0);
	// Retain point/radial damage delegates and the existing small physical hit impulse.
	Super::TakeDamage(Applied, DamageEvent, EventInstigator, DamageCauser);
	return Applied;
}

void ATunaSweeperATVActor::UpdateDamageSmoke(float DeltaSeconds)
{
	HitSmokeRemaining = FMath::Max(0.0f, HitSmokeRemaining - DeltaSeconds);
	if (bVehicleDestroyed) WreckSmokeElapsed += DeltaSeconds;
	const auto State = GetDamageState();
	const bool bWreckSmoking = bVehicleDestroyed && WreckSmokeElapsed < WreckSmokeDuration;
	const bool bHeavy = State == ETunaSweeperATVDamageState::Critical || bWreckSmoking;
	const bool bLight = !bVehicleDestroyed && !bHeavy && (State == ETunaSweeperATVDamageState::Damaged || HitSmokeRemaining > 0);
	const float Strength = bVehicleDestroyed ? 1.0f - FMath::Clamp(WreckSmokeElapsed / FMath::Max(0.01f, WreckSmokeDuration), 0.0f, 1.0f) : 1.0f;
	for (auto* Smoke : {LightDamageSmoke.Get(), HeavyDamageSmoke.Get()})
	{
		if (!Smoke) continue;
		const bool bEmit = Smoke == LightDamageSmoke ? bLight : bHeavy;
		bool& bWasEmitting = Smoke == LightDamageSmoke ? bLightSmokeEmitting : bHeavySmokeEmitting;
		Smoke->SetFloatParameter(TEXT("User.Fade"), Strength);
		Smoke->SetFloatParameter(TEXT("User.StageSmokeIntensity"), Strength);
		// Niagara stays IsActive while deactivated particles drain. Track emission
		// separately so a fresh hit can restart spawning during that interval.
		if (bEmit && !bWasEmitting) Smoke->Activate(false);
		else if (!bEmit && bWasEmitting) Smoke->Deactivate();
		bWasEmitting = bEmit;
	}
}

void ATunaSweeperATVActor::DestroyVehicle()
{
	if (bVehicleDestroyed) return;
	bVehicleDestroyed = true;
	ClearDriveInput();
	MountComponent->ReleaseRiderForVehicleDestruction();
	// Remove the Chaos vehicle from its manager. The chassis remains a normal rigid
	// body and settles on the ground instead of being held up by invisible wheels.
	VehicleMovement->Deactivate();
	VehicleMovement->SetComponentTickEnabled(false);
	VehicleMovement->DestroyPhysicsState();
	VehicleMesh->SetPhysicsLinearVelocity(VehicleMesh->GetPhysicsLinearVelocity().GetClampedToMaxSize(250));
	VehicleMesh->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	VehicleMesh->SetLinearDamping(1.5f);
	VehicleMesh->SetAngularDamping(3.0f);
	const FName Bones[] = {TEXT("wheel_FL"), TEXT("wheel_RR"), TEXT("handlebar")};
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Bones); ++Index)
	{
		if (!DetachedPartMeshes.IsValidIndex(Index) || !DetachedPartMeshes[Index]) continue;
		const FTransform Transform = VehicleMesh->GetSocketTransform(Bones[Index]);
		FActorSpawnParameters Spawn;
		Spawn.Owner = this;
		Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto* Part = GetWorld()->SpawnActor<AActor>(AActor::StaticClass(), Transform, Spawn);
		if (!Part) continue;
		auto* Mesh = NewObject<UStaticMeshComponent>(Part);
		Part->SetRootComponent(Mesh);
		Mesh->SetStaticMesh(DetachedPartMeshes[Index]);
		Mesh->SetCollisionObjectType(ECC_PhysicsBody);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		Mesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		Mesh->SetMassOverrideInKg(NAME_None, Index == 2 ? 4.0f : 12.0f);
		Mesh->SetLinearDamping(1.0f);
		Mesh->SetAngularDamping(2.0f);
		Mesh->SetCanEverAffectNavigation(false);
		Mesh->RegisterComponent();
		Mesh->SetWorldTransform(Transform);
		Mesh->SetSimulatePhysics(true);
		const FVector Outward = (Transform.GetLocation() - GetActorLocation()).GetSafeNormal2D();
		Mesh->SetPhysicsLinearVelocity((GetVelocity() + Outward * 110 + FVector(0, 0, 110)).GetClampedToMaxSize(350));
		Mesh->SetPhysicsAngularVelocityInDegrees(FVector(45, 80, Index % 2 ? -70 : 70));
		Part->Tags.Add(TEXT("ATVDebris"));
		Part->SetLifeSpan(FMath::Max(1.0f, DebrisLifetime));
		DetachedParts.Add(Part);
		VehicleMesh->HideBoneByName(Bones[Index], PBO_None);
	}
	const float Delay = FMath::IsFinite(DestructionExplosionDelay) ? FMath::Max(0.0f, DestructionExplosionDelay) : 0.0f;
	if (Delay > 0)
	{
		GetWorldTimerManager().SetTimer(DestructionExplosionTimer, this, &ATunaSweeperATVActor::PlayDestructionExplosion, Delay, false);
	}
	else PlayDestructionExplosion();
}

void ATunaSweeperATVActor::PlayDestructionExplosion()
{
	if (!bVehicleDestroyed || bDestructionExplosionTriggered || IsActorBeingDestroyed()) return;
	bDestructionExplosionTriggered = true;
	const FVector ExplosionLocation = DestructionExplosion ? DestructionExplosion->GetComponentLocation() : GetActorLocation();
	if (DestructionExplosion)
	{
		// Burst at the wreck's current position, then keep it fixed in world space.
		DestructionExplosion->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		DestructionExplosion->Activate(true);
	}
	if (USoundBase* Sound = DestructionExplosionSound.LoadSynchronous())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Sound, ExplosionLocation);
	}
}

void ATunaSweeperATVActor::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearTimer(DestructionExplosionTimer);
	if (DestructionExplosion) DestructionExplosion->DeactivateImmediate();
	for (const auto& Part : DetachedParts) if (Part.IsValid()) Part->Destroy();
	DetachedParts.Empty();
	Super::EndPlay(Reason);
}
