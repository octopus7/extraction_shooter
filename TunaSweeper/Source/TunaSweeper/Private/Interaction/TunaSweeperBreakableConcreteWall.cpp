#include "Interaction/TunaSweeperBreakableConcreteWall.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/TunaSweeperExplosiveBarrelActor.h"
#include "TunaSweeperCollisionChannels.h"

ATunaSweeperBreakableConcreteWall::ATunaSweeperBreakableConcreteWall()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));

	WallCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("WallCollision"));
	WallCollision->SetupAttachment(RootComponent);
	WallCollision->SetBoxExtent(FVector(200.0, 22.5, 125.0));
	WallCollision->SetRelativeLocation(FVector(0.0, 0.0, 125.0));
	WallCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	WallCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
	WallCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Block);
	WallCollision->SetGenerateOverlapEvents(false);

	WallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetupAttachment(RootComponent);
	WallMesh->SetRelativeLocation(FVector(0.0, 0.0, 125.0));
	WallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

float ATunaSweeperBreakableConcreteWall::TakeDamage(float DamageAmount, const FDamageEvent& DamageEvent,
	AController* EventInstigator, AActor* DamageCauser)
{
	const auto* Barrel = Cast<ATunaSweeperExplosiveBarrelActor>(DamageCauser);
	if (!(DamageAmount > 0.0f) || !Barrel || !Barrel->IsBarrelDestroyed() || IsActorBeingDestroyed())
	{
		return 0.0f;
	}

	return Destroy() ? DamageAmount : 0.0f;
}
