#include "Interaction/TunaSweeperExplosiveBarrelActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Effect/TunaSweeperLocalExplosionEffectActor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	const TCHAR* DefaultIntactBarrelMeshPath = TEXT("/Game/Interaction/SM_ExplosiveBarrel_Intact.SM_ExplosiveBarrel_Intact");
	const TCHAR* DefaultDestroyedBarrelMeshPath = TEXT("/Game/Interaction/SM_ExplosiveBarrel_DestroyedBase.SM_ExplosiveBarrel_DestroyedBase");
	const TCHAR* DefaultIntactBarrelMaterialPath = TEXT("/Game/Interaction/M_ExplosiveBarrel_Gray.M_ExplosiveBarrel_Gray");
	const TCHAR* DefaultDestroyedBarrelMaterialPath = TEXT("/Game/Interaction/M_ExplosiveBarrel_CharredGray.M_ExplosiveBarrel_CharredGray");
	const TCHAR* DefaultExplosionEffectClassPath = TEXT("/Script/TunaSweeper.TunaSweeperLocalExplosionEffectActor");

	FVector MakeSafeExtent(const FVector& Extent)
	{
		return FVector(
			FMath::Max(1.0f, Extent.X),
			FMath::Max(1.0f, Extent.Y),
			FMath::Max(1.0f, Extent.Z));
	}
}

ATunaSweeperExplosiveBarrelActor::ATunaSweeperExplosiveBarrelActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(RootComponent);
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
	BlockingCollision->SetCanEverAffectNavigation(true);

	BarrelMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BarrelMesh"));
	BarrelMeshComponent->SetupAttachment(RootComponent);
	BarrelMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BarrelMeshComponent->SetGenerateOverlapEvents(false);

	DestroyedLoopEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DestroyedLoopEffect"));
	DestroyedLoopEffectComponent->SetupAttachment(RootComponent);
	DestroyedLoopEffectComponent->SetAutoActivate(false);
	DestroyedLoopEffectComponent->SetRelativeLocation(DestroyedLoopEffectRelativeLocation);

	IntactBarrelMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultIntactBarrelMeshPath));
	DestroyedBarrelMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultDestroyedBarrelMeshPath));
	IntactMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultIntactBarrelMaterialPath));
	DestroyedMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultDestroyedBarrelMaterialPath));
	ExplosionEffectActorClass = TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor>(FSoftObjectPath(DefaultExplosionEffectClassPath));

	ApplyCollisionDefaults();
}

void ATunaSweeperExplosiveBarrelActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	IntactCollisionExtent = MakeSafeExtent(IntactCollisionExtent);
	DestroyedCollisionExtent = MakeSafeExtent(DestroyedCollisionExtent);
	ExplosionVisualRadiusCm = FMath::Max(1.0f, ExplosionVisualRadiusCm);
	ExplosionDurationSeconds = FMath::Max(0.05f, ExplosionDurationSeconds);

	ApplyCollisionDefaults();
	ApplyVisualState();
	RefreshDestroyedLoopEffect();
}

void ATunaSweeperExplosiveBarrelActor::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	bBarrelDestroyed = false;

	ApplyCollisionDefaults();
	ApplyVisualState();
	RefreshDestroyedLoopEffect();
}

float ATunaSweeperExplosiveBarrelActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bBarrelDestroyed || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(CurrentHealth, DamageAmount);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	if (CurrentHealth <= 0.0f)
	{
		DestroyBarrel();
	}

	return AppliedDamage;
}

void ATunaSweeperExplosiveBarrelActor::ConfigureExplosiveBarrelDefaults(
	FName InBarrelId,
	float InMaxHealth,
	const TSoftObjectPtr<UStaticMesh>& InIntactMesh,
	const TSoftObjectPtr<UStaticMesh>& InDestroyedMesh,
	const TSoftObjectPtr<UNiagaraSystem>& InDestroyedLoopEffect,
	const TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor>& InExplosionEffectActorClass,
	float InExplosionVisualRadiusCm,
	float InExplosionDurationSeconds)
{
	BarrelId = InBarrelId;
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = MaxHealth;
	if (!InIntactMesh.IsNull())
	{
		IntactBarrelMesh = InIntactMesh;
	}
	if (!InDestroyedMesh.IsNull())
	{
		DestroyedBarrelMesh = InDestroyedMesh;
	}
	DestroyedLoopEffect = InDestroyedLoopEffect;
	if (!InExplosionEffectActorClass.IsNull())
	{
		ExplosionEffectActorClass = InExplosionEffectActorClass;
	}
	ExplosionVisualRadiusCm = FMath::Max(1.0f, InExplosionVisualRadiusCm);
	ExplosionDurationSeconds = FMath::Max(0.05f, InExplosionDurationSeconds);

	ApplyCollisionDefaults();
	ApplyVisualState();
	RefreshDestroyedLoopEffect();
}

void ATunaSweeperExplosiveBarrelActor::ApplyCollisionDefaults()
{
	if (!BlockingCollision)
	{
		return;
	}

	const FVector ActiveExtent = MakeSafeExtent(bBarrelDestroyed ? DestroyedCollisionExtent : IntactCollisionExtent);
	BlockingCollision->SetRelativeLocation(FVector(0.0f, 0.0f, ActiveExtent.Z));
	BlockingCollision->SetBoxExtent(ActiveExtent);
	BlockingCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BlockingCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Block);
	BlockingCollision->SetGenerateOverlapEvents(false);
	BlockingCollision->CanCharacterStepUpOn = ECB_No;
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
}

void ATunaSweeperExplosiveBarrelActor::ApplyVisualState()
{
	if (!BarrelMeshComponent)
	{
		return;
	}

	UStaticMesh* ActiveMesh = bBarrelDestroyed
		? DestroyedBarrelMesh.LoadSynchronous()
		: IntactBarrelMesh.LoadSynchronous();
	BarrelMeshComponent->SetStaticMesh(ActiveMesh);

	UMaterialInterface* ActiveMaterial = bBarrelDestroyed
		? DestroyedMaterial.LoadSynchronous()
		: IntactMaterial.LoadSynchronous();
	if (ActiveMaterial)
	{
		BarrelMeshComponent->SetMaterial(0, ActiveMaterial);
	}

	BarrelMeshComponent->SetHiddenInGame(false);
	BarrelMeshComponent->SetVisibility(true, true);
}

void ATunaSweeperExplosiveBarrelActor::RefreshDestroyedLoopEffect()
{
	if (!DestroyedLoopEffectComponent)
	{
		return;
	}

	DestroyedLoopEffectComponent->SetRelativeLocation(DestroyedLoopEffectRelativeLocation);
	if (UNiagaraSystem* LoadedLoopEffect = DestroyedLoopEffect.LoadSynchronous())
	{
		DestroyedLoopEffectComponent->SetAsset(LoadedLoopEffect);
		if (bBarrelDestroyed)
		{
			DestroyedLoopEffectComponent->Activate(true);
		}
		else
		{
			DestroyedLoopEffectComponent->Deactivate();
		}
	}
	else
	{
		DestroyedLoopEffectComponent->Deactivate();
	}
}

void ATunaSweeperExplosiveBarrelActor::DestroyBarrel()
{
	if (bBarrelDestroyed)
	{
		return;
	}

	bBarrelDestroyed = true;
	SpawnExplosionEffect();
	ApplyCollisionDefaults();
	ApplyVisualState();
	RefreshDestroyedLoopEffect();
}

void ATunaSweeperExplosiveBarrelActor::SpawnExplosionEffect()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	TSubclassOf<ATunaSweeperLocalExplosionEffectActor> LoadedExplosionClass =
		ExplosionEffectActorClass.LoadSynchronous();
	if (!LoadedExplosionClass)
	{
		LoadedExplosionClass = ATunaSweeperLocalExplosionEffectActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATunaSweeperLocalExplosionEffectActor* ExplosionActor =
		World->SpawnActor<ATunaSweeperLocalExplosionEffectActor>(
			LoadedExplosionClass,
			GetActorLocation() + GetActorTransform().TransformVectorNoScale(ExplosionEffectOffset),
			GetActorRotation(),
			SpawnParameters);
	if (ExplosionActor)
	{
		ExplosionActor->ConfigureExplosion(ExplosionVisualRadiusCm, ExplosionDurationSeconds);
	}
}
