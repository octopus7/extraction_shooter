#include "Weapon/TunaSweeperThrowableGrenade.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "TimerManager.h"
#include "Weapon/TunaSweeperThrowableDamageType.h"

ATunaSweeperThrowableGrenade::ATunaSweeperThrowableGrenade()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(15.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->SetSimulatePhysics(false);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ATunaSweeperThrowableGrenade::HandleHit);
	RootComponent = CollisionComponent;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(CollisionComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->SetUpdatedComponent(CollisionComponent);
	ProjectileMovement->InitialSpeed = ThrowSpeed;
	ProjectileMovement->MaxSpeed = ThrowSpeed * 2.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;
	ProjectileMovement->Bounciness = 0.3f;
	ProjectileMovement->Friction = 0.3f;
	ProjectileMovement->ProjectileGravityScale = 1.0f;
}

void ATunaSweeperThrowableGrenade::BeginPlay()
{
	Super::BeginPlay();

	FuseStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	GetWorldTimerManager().SetTimer(
		FuseTimerHandle,
		this,
		&ATunaSweeperThrowableGrenade::Explode,
		FMath::Max(0.0f, FuseTime),
		false);
}

void ATunaSweeperThrowableGrenade::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bHasExploded && bShowDebugExplosion)
	{
		DrawDebugCircle(
			GetWorld(),
			GetActorLocation(),
			ExplosionRadius,
			48,
			FColor::Orange,
			false,
			0.0f,
			0,
			2.0f,
			FVector(1.0f, 0.0f, 0.0f),
			FVector(0.0f, 1.0f, 0.0f),
			false);

		const float ElapsedTime = GetWorld() ? GetWorld()->GetTimeSeconds() - FuseStartTime : FuseTime;
		const float Progress = FuseTime > KINDA_SMALL_NUMBER ? FMath::Clamp(ElapsedTime / FuseTime, 0.0f, 1.0f) : 1.0f;
		const float ExpandingRadius = ExplosionRadius * Progress;
		if (ExpandingRadius > 1.0f)
		{
			DrawDebugCircle(
				GetWorld(),
				GetActorLocation(),
				ExpandingRadius,
				48,
				FColor::Red,
				false,
				0.0f,
				0,
				3.0f,
				FVector(1.0f, 0.0f, 0.0f),
				FVector(0.0f, 1.0f, 0.0f),
				false);
		}
	}
}

void ATunaSweeperThrowableGrenade::LaunchGrenade(const FVector& Direction, float Speed)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Direction.GetSafeNormal() * FMath::Max(0.0f, Speed);
	}
}

void ATunaSweeperThrowableGrenade::HandleHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	bHasLanded = true;
}

void ATunaSweeperThrowableGrenade::Explode()
{
	if (bHasExploded)
	{
		return;
	}

	bHasExploded = true;
	const FVector ExplosionLocation = GetActorLocation();

	TArray<AActor*> IgnoreActors;
	if (const UWorld* World = GetWorld())
	{
		if (const UTunaSweeperFactionSubsystem* FactionSubsystem =
			World->GetSubsystem<UTunaSweeperFactionSubsystem>())
		{
			FactionSubsystem->GetActorsWithAttitude(
				this,
				ETunaSweeperFactionAttitude::Friendly,
				IgnoreActors);
		}
	}
	UGameplayStatics::ApplyRadialDamage(
		this,
		Damage,
		ExplosionLocation,
		ExplosionRadius,
		UTunaSweeperGrenadeDamageType::StaticClass(),
		IgnoreActors,
		this,
		GetInstigatorController(),
		true,
		ECC_Visibility);

	if (ExplosionNiagara)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ExplosionNiagara,
			ExplosionLocation,
			GetActorRotation());
	}
	else if (ExplosionParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			ExplosionParticle,
			ExplosionLocation,
			GetActorRotation());
	}

	if (ExplosionSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, ExplosionLocation);
	}

	if (ExplosionCameraShake)
	{
		UGameplayStatics::PlayWorldCameraShake(
			GetWorld(),
			ExplosionCameraShake,
			ExplosionLocation,
			0.0f,
			CameraShakeRadius);
	}

	if (bShowDebugExplosion)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(
				-1,
				2.0f,
				FColor::Orange,
				FString::Printf(TEXT("Grenade exploded. Damage: %.0f, Radius: %.0f"), Damage, ExplosionRadius));
		}

		DrawDebugSphere(GetWorld(), ExplosionLocation, ExplosionRadius, 24, FColor::Orange, false, 1.0f);
	}

	Destroy();
}

void ATunaSweeperThrowableGrenade::SetExplosionEffects(
	UParticleSystem* InParticle,
	UNiagaraSystem* InNiagara,
	USoundBase* InSound,
	TSubclassOf<UCameraShakeBase> InCameraShake,
	float InCameraShakeRadius)
{
	ExplosionParticle = InParticle;
	ExplosionNiagara = InNiagara;
	ExplosionSound = InSound;
	ExplosionCameraShake = InCameraShake;
	CameraShakeRadius = FMath::Max(0.0f, InCameraShakeRadius);
}
