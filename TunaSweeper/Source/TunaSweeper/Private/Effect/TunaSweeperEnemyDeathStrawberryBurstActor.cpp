#include "Effect/TunaSweeperEnemyDeathStrawberryBurstActor.h"

#include "Components/BillboardComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"

namespace
{
	constexpr float GroundTraceUpCm = 420.0f;
	constexpr float GroundTraceDownCm = 1200.0f;
	constexpr float MinGroundNormalZ = 0.72f;
	constexpr float ParticleGroundOffsetCm = 12.0f;
	constexpr float MinimumBounceVelocityCmPerSecond = 85.0f;
	constexpr float BounceDamping = 0.28f;
	constexpr float GroundFriction = 0.58f;
}

ATunaSweeperEnemyDeathStrawberryBurstActor::ATunaSweeperEnemyDeathStrawberryBurstActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SetActorEnableCollision(false);
	StrawberryTexture = TSoftObjectPtr<UTexture2D>(
		FSoftObjectPath(TEXT("/Game/Effects/T_EnemyDeathStrawberry.T_EnemyDeathStrawberry")));
}

void ATunaSweeperEnemyDeathStrawberryBurstActor::BeginPlay()
{
	Super::BeginPlay();

	bHasGround = TryResolveGroundHeight(GroundHeight);
	CreateBurstParticles();
	SetLifeSpan(FMath::Max(ParticleLifetimeSeconds + 0.15f, 0.25f));
}

void ATunaSweeperEnemyDeathStrawberryBurstActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const float SafeDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	for (int32 ParticleIndex = 0; ParticleIndex < ParticleSprites.Num(); ++ParticleIndex)
	{
		UpdateParticle(ParticleIndex, SafeDeltaSeconds);
	}
}

void ATunaSweeperEnemyDeathStrawberryBurstActor::CreateBurstParticles()
{
	UTexture2D* LoadedTexture = StrawberryTexture.LoadSynchronous();
	if (!LoadedTexture)
	{
		return;
	}

	const int32 SafeParticleCount = FMath::Clamp(ParticleCount, 1, 32);
	ParticleSprites.Reserve(SafeParticleCount);
	ParticleLocations.Reserve(SafeParticleCount);
	ParticleVelocities.Reserve(SafeParticleCount);
	ParticleRotationDegrees.Reserve(SafeParticleCount);
	ParticleRotationSpeeds.Reserve(SafeParticleCount);
	ParticleSettled.Reserve(SafeParticleCount);

	for (int32 ParticleIndex = 0; ParticleIndex < SafeParticleCount; ++ParticleIndex)
	{
		const float BurstAngleRadians = FMath::FRandRange(0.0f, 2.0f * PI);
		const float HorizontalSpeed = FMath::FRandRange(175.0f, 350.0f);
		const FVector HorizontalDirection(FMath::Cos(BurstAngleRadians), FMath::Sin(BurstAngleRadians), 0.0f);
		const FVector SpawnLocation = GetActorLocation() + FVector(0.0f, 0.0f, FMath::FRandRange(-4.0f, 22.0f));

		UBillboardComponent* Sprite = NewObject<UBillboardComponent>(this);
		if (!Sprite)
		{
			continue;
		}

		Sprite->SetSprite(LoadedTexture);
		Sprite->SetHiddenInGame(false);
		Sprite->SetVisibility(true, true);
		Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Sprite->SetGenerateOverlapEvents(false);
		Sprite->SetCastShadow(false);
		Sprite->SetReceivesDecals(false);
		Sprite->SetMobility(EComponentMobility::Movable);
		Sprite->SetWorldLocation(SpawnLocation);
		Sprite->SetWorldScale3D(FVector(FMath::FRandRange(0.11f, 0.16f)));
		AddInstanceComponent(Sprite);
		Sprite->RegisterComponent();

		ParticleSprites.Add(Sprite);
		ParticleLocations.Add(SpawnLocation);
		ParticleVelocities.Add(
			HorizontalDirection * HorizontalSpeed + FVector(0.0f, 0.0f, FMath::FRandRange(260.0f, 430.0f)));
		ParticleRotationDegrees.Add(FMath::FRandRange(0.0f, 360.0f));
		ParticleRotationSpeeds.Add(FMath::FRandRange(-420.0f, 420.0f));
		ParticleSettled.Add(false);
	}
}

bool ATunaSweeperEnemyDeathStrawberryBurstActor::TryResolveGroundHeight(float& OutGroundHeight) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Origin = GetActorLocation();
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperEnemyDeathStrawberryGroundTrace), false, this);
	FHitResult GroundHit;
	if (World->LineTraceSingleByChannel(
		GroundHit,
		Origin + FVector(0.0f, 0.0f, GroundTraceUpCm),
		Origin - FVector(0.0f, 0.0f, GroundTraceDownCm),
		ECC_Visibility,
		QueryParams) &&
		GroundHit.bBlockingHit &&
		GroundHit.ImpactNormal.Z >= MinGroundNormalZ)
	{
		OutGroundHeight = GroundHit.ImpactPoint.Z + ParticleGroundOffsetCm;
		return true;
	}

	return false;
}

void ATunaSweeperEnemyDeathStrawberryBurstActor::UpdateParticle(int32 ParticleIndex, float DeltaSeconds)
{
	if (!ParticleSprites.IsValidIndex(ParticleIndex) ||
		!ParticleLocations.IsValidIndex(ParticleIndex) ||
		!ParticleVelocities.IsValidIndex(ParticleIndex) ||
		!ParticleSettled.IsValidIndex(ParticleIndex))
	{
		return;
	}

	UBillboardComponent* Sprite = ParticleSprites[ParticleIndex];
	if (!Sprite)
	{
		return;
	}

	FVector& Location = ParticleLocations[ParticleIndex];
	FVector& Velocity = ParticleVelocities[ParticleIndex];
	if (!ParticleSettled[ParticleIndex])
	{
		Velocity.Z -= FMath::Max(0.0f, GravityCmPerSecondSquared) * DeltaSeconds;
		Location += Velocity * DeltaSeconds;

		if (bHasGround && Location.Z <= GroundHeight)
		{
			Location.Z = GroundHeight;
			if (Velocity.Z < -MinimumBounceVelocityCmPerSecond)
			{
				Velocity.Z *= -BounceDamping;
				Velocity.X *= GroundFriction;
				Velocity.Y *= GroundFriction;
			}
			else
			{
				Velocity = FVector::ZeroVector;
				ParticleSettled[ParticleIndex] = true;
			}
		}
	}

	ParticleRotationDegrees[ParticleIndex] += ParticleRotationSpeeds[ParticleIndex] * DeltaSeconds;
	Sprite->SetWorldLocation(Location);
	Sprite->SetWorldRotation(FRotator(0.0f, 0.0f, ParticleRotationDegrees[ParticleIndex]));
}
