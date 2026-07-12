#include "Interaction/TunaSweeperCookableChickenActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Interaction/TunaSweeperExplosiveBarrelActor.h"
#include "TunaSweeperCollisionChannels.h"

ATunaSweeperCookableChickenActor::ATunaSweeperCookableChickenActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(true);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(RootComponent);
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
	BlockingCollision->SetCanEverAffectNavigation(true);

	ChickenMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ChickenMesh"));
	ChickenMeshComponent->SetupAttachment(RootComponent);
	ChickenMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChickenMeshComponent->SetGenerateOverlapEvents(false);
	ChickenMeshComponent->SetCanEverAffectNavigation(false);
	ChickenMeshComponent->SetCastShadow(true);
}

void ATunaSweeperCookableChickenActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyActorDefaults();
}

void ATunaSweeperCookableChickenActor::BeginPlay()
{
	Super::BeginPlay();
	bCooked = false;
	SetCanBeDamaged(true);
	ApplyActorDefaults();
}

float ATunaSweeperCookableChickenActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bCooked || DamageAmount <= 0.0f || !Cast<ATunaSweeperExplosiveBarrelActor>(DamageCauser))
	{
		return 0.0f;
	}

	CookChicken();
	return DamageAmount;
}

void ATunaSweeperCookableChickenActor::CookChicken()
{
	if (bCooked)
	{
		return;
	}

	bCooked = true;
	ApplyCurrentMesh();
}

void ATunaSweeperCookableChickenActor::ResetToRawChicken()
{
	bCooked = false;
	ApplyCurrentMesh();
}

void ATunaSweeperCookableChickenActor::ApplyActorDefaults()
{
	CollisionExtent.X = FMath::Max(1.0f, CollisionExtent.X);
	CollisionExtent.Y = FMath::Max(1.0f, CollisionExtent.Y);
	CollisionExtent.Z = FMath::Max(1.0f, CollisionExtent.Z);

	if (BlockingCollision)
	{
		BlockingCollision->SetRelativeLocation(CollisionCenterOffset);
		BlockingCollision->SetBoxExtent(CollisionExtent);
		BlockingCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		BlockingCollision->SetCollisionObjectType(ECC_WorldDynamic);
		BlockingCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
		BlockingCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
		BlockingCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		BlockingCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
		BlockingCollision->SetGenerateOverlapEvents(false);
		BlockingCollision->CanCharacterStepUpOn = ECB_No;
		BlockingCollision->SetCanEverAffectNavigation(true);
	}

	ApplyCurrentMesh();
}

void ATunaSweeperCookableChickenActor::ApplyCurrentMesh()
{
	if (!ChickenMeshComponent)
	{
		return;
	}

	const TSoftObjectPtr<UStaticMesh>& MeshToDisplay = bCooked ? CookedChickenMesh : RawChickenMesh;
	ChickenMeshComponent->SetStaticMesh(MeshToDisplay.LoadSynchronous());
	ChickenMeshComponent->SetRelativeLocation(FVector::ZeroVector);
	ChickenMeshComponent->SetRelativeRotation(FRotator::ZeroRotator);
	ChickenMeshComponent->SetRelativeScale3D(FVector::OneVector);
}
