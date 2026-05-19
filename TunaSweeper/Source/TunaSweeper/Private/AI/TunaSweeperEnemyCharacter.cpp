#include "AI/TunaSweeperEnemyCharacter.h"

#include "AI/TunaSweeperEnemyAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace
{
	constexpr float LootDropGroundTraceUp = 500.0f;
	constexpr float LootDropGroundTraceDown = 900.0f;
	constexpr float LootContainerRootHeight = 40.0f;
	constexpr float MinLootDropGroundNormalZ = 0.72f;
	constexpr int32 LumberjackDropContainerDefinitionId = 7007;
	constexpr int32 LumberjackDropContentsId = 8006;
	constexpr float LumberjackMeleeDamage = 1.0f;
	constexpr float LumberjackMeleeAttackRange = 150.0f;
	constexpr float LumberjackMeleeApproachStartRange = 130.0f;
	constexpr float LumberjackMeleeApproachStopRange = 95.0f;
	constexpr float LumberjackMeleeTrackingRange = 1800.0f;
	constexpr float LumberjackMeleeAttackCooldownSeconds = 1.25f;

	float GetRandomizedEnemyValue(float BaseValue, const FVector2D& OffsetRange, float MinValue)
	{
		const float MinOffset = FMath::Min(OffsetRange.X, OffsetRange.Y);
		const float MaxOffset = FMath::Max(OffsetRange.X, OffsetRange.Y);
		return FMath::Max(MinValue, BaseValue + FMath::FRandRange(MinOffset, MaxOffset));
	}
}

ATunaSweeperEnemyCharacter::ATunaSweeperEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = ATunaSweeperEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);
	GetCharacterMovement()->MaxWalkSpeed = 260.0f;
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 480.0f, 0.0f);

	GetMesh()->SetHiddenInGame(true);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.6f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMesh.Object);
	}

	ForwardMarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ForwardMarkerMesh"));
	ForwardMarkerMesh->SetupAttachment(RootComponent);
	ForwardMarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ForwardMarkerMesh->SetRelativeLocation(FVector(60.0f, 0.0f, 50.0f));
	ForwardMarkerMesh->SetRelativeRotation(FRotator(90.0f, 0.0f, 0.0f));
	ForwardMarkerMesh->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.8f));

	if (CylinderMesh.Succeeded())
	{
		ForwardMarkerMesh->SetStaticMesh(CylinderMesh.Object);
	}

	ProjectileClass = TSoftClassPtr<ATunaSweeperProjectile>(
		FSoftObjectPath(TEXT("/Game/Weapons/BP_TunaSweeperProjectile.BP_TunaSweeperProjectile_C")));
	BodyMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Characters/Enemy/M_Enemy_Red.M_Enemy_Red")));
	ForwardMarkerMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(TEXT("/Game/Characters/Enemy/M_Enemy_Sightline.M_Enemy_Sightline")));
	LootContainerClass = TSoftClassPtr<ATunaSweeperLootContainerActor>(
		FSoftObjectPath(TEXT("/Game/Interaction/BP_LootContainer.BP_LootContainer_C")));
}

void ATunaSweeperEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = GetRandomizedEnemyValue(MovementSpeed, MovementSpeedRandomOffset, 0.0f);
	ApplyVisualMaterials();
}

float ATunaSweeperEnemyCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(CurrentHealth, DamageAmount);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	if (CurrentHealth <= 0.0f)
	{
		HandleDeath(DamageCauser);
	}

	return AppliedDamage;
}

void ATunaSweeperEnemyCharacter::ConfigureSpawnData(
	const TSoftObjectPtr<UMaterialInterface>& InBodyMaterial,
	int32 InDropContainerDefinitionId,
	int32 InDropContentsId,
	float InMaxHealth)
{
	if (!InBodyMaterial.IsNull())
	{
		BodyMaterial = InBodyMaterial;
	}

	if (InMaxHealth > 0.0f)
	{
		MaxHealth = InMaxHealth;
		CurrentHealth = MaxHealth;
	}

	DropContainerDefinitionId = InDropContainerDefinitionId;
	DropContentsId = InDropContentsId;
	ApplyVisualMaterials();
}

bool ATunaSweeperEnemyCharacter::AttackTarget(AActor* TargetActor)
{
	if (UsesMeleeAttack())
	{
		return ApplyMeleeDamageTo(TargetActor);
	}

	return FireProjectileAt(TargetActor);
}

bool ATunaSweeperEnemyCharacter::UsesMeleeAttack() const
{
	return DropContainerDefinitionId == LumberjackDropContainerDefinitionId ||
		DropContentsId == LumberjackDropContentsId;
}

float ATunaSweeperEnemyCharacter::GetMeleeAttackRange() const
{
	return LumberjackMeleeAttackRange;
}

float ATunaSweeperEnemyCharacter::GetMeleeApproachStartRange() const
{
	return LumberjackMeleeApproachStartRange;
}

float ATunaSweeperEnemyCharacter::GetMeleeApproachStopRange() const
{
	return LumberjackMeleeApproachStopRange;
}

float ATunaSweeperEnemyCharacter::GetMeleeTrackingRange() const
{
	return LumberjackMeleeTrackingRange;
}

float ATunaSweeperEnemyCharacter::GetMeleeAttackCooldownSeconds() const
{
	return LumberjackMeleeAttackCooldownSeconds;
}

bool ATunaSweeperEnemyCharacter::FireProjectileAt(AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World || !TargetActor || bIsDead || UsesMeleeAttack())
	{
		return false;
	}

	TSubclassOf<ATunaSweeperProjectile> LoadedProjectileClass = ProjectileClass.LoadSynchronous();
	if (!LoadedProjectileClass)
	{
		LoadedProjectileClass = ATunaSweeperProjectile::StaticClass();
	}

	const FVector ActorLocation = GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 45.0f);
	const FVector ToTarget = TargetLocation - (ActorLocation + FVector(0.0f, 0.0f, ProjectileSpawnOffset.Z));
	const FVector FireDirection = ToTarget.GetSafeNormal();
	if (FireDirection.IsNearlyZero())
	{
		return false;
	}

	const FRotator FireRotation = FireDirection.Rotation();
	SetActorRotation(FRotator(0.0f, FireRotation.Yaw, 0.0f));

	const FVector SpawnLocation = ActorLocation + GetActorRotation().RotateVector(ProjectileSpawnOffset);

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATunaSweeperProjectile* SpawnedProjectile = World->SpawnActor<ATunaSweeperProjectile>(
		LoadedProjectileClass,
		SpawnLocation,
		FireRotation,
		SpawnParameters);
	if (!SpawnedProjectile)
	{
		return false;
	}

	SpawnedProjectile->SetDamageAmount(ProjectileDamage);
	return true;
}

bool ATunaSweeperEnemyCharacter::ApplyMeleeDamageTo(AActor* TargetActor)
{
	if (!TargetActor || TargetActor == this || bIsDead || LumberjackMeleeDamage <= 0.0f)
	{
		return false;
	}

	const FVector ActorLocation = GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	const float AttackRange = FMath::Max(1.0f, GetMeleeAttackRange());
	if (FVector::DistSquared2D(ActorLocation, TargetLocation) > FMath::Square(AttackRange))
	{
		return false;
	}

	const FVector ToTarget = FVector(TargetLocation.X - ActorLocation.X, TargetLocation.Y - ActorLocation.Y, 0.0f);
	const FVector AttackDirection = ToTarget.GetSafeNormal();
	if (!AttackDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, AttackDirection.Rotation().Yaw, 0.0f));
	}

	UGameplayStatics::ApplyDamage(TargetActor, LumberjackMeleeDamage, GetController(), this, UDamageType::StaticClass());
	return true;
}

void ATunaSweeperEnemyCharacter::HandleDeath(AActor* DamageCauser)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	CurrentHealth = 0.0f;

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	SetActorEnableCollision(false);
	DetachFromControllerPendingDestroy();
	SpawnDeathLootContainer(DamageCauser);
	Destroy();
}

bool ATunaSweeperEnemyCharacter::SpawnDeathLootContainer(AActor* DamageCauser)
{
	if (DropContainerDefinitionId == INDEX_NONE || DropContentsId == INDEX_NONE)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	TSubclassOf<ATunaSweeperLootContainerActor> LoadedLootContainerClass = LootContainerClass.LoadSynchronous();
	if (!LoadedLootContainerClass)
	{
		LoadedLootContainerClass = ATunaSweeperLootContainerActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATunaSweeperLootContainerActor* SpawnedContainer = World->SpawnActor<ATunaSweeperLootContainerActor>(
		LoadedLootContainerClass,
		ResolveLootDropLocation(DamageCauser),
		FRotator::ZeroRotator,
		SpawnParameters);
	if (!SpawnedContainer)
	{
		return false;
	}

	SpawnedContainer->SetContainerDataIds(DropContainerDefinitionId, DropContentsId);
	return true;
}

FVector ATunaSweeperEnemyCharacter::ResolveLootDropLocation(AActor* IgnoredActor) const
{
	UWorld* World = GetWorld();
	const FVector ActorLocation = GetActorLocation();
	if (!World)
	{
		return ActorLocation;
	}

	const FVector TraceStart = ActorLocation + FVector(0.0f, 0.0f, LootDropGroundTraceUp);
	const FVector TraceEnd = ActorLocation - FVector(0.0f, 0.0f, LootDropGroundTraceDown);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperEnemyLootDropGroundTrace), false, this);
	if (IgnoredActor)
	{
		QueryParams.AddIgnoredActor(IgnoredActor);
	}

	FHitResult GroundHit;
	if (World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) &&
		GroundHit.bBlockingHit &&
		GroundHit.ImpactNormal.Z >= MinLootDropGroundNormalZ)
	{
		return GroundHit.ImpactPoint + FVector(0.0f, 0.0f, LootContainerRootHeight);
	}

	return ActorLocation;
}

void ATunaSweeperEnemyCharacter::ApplyVisualMaterials()
{
	UMaterialInterface* LoadedBodyMaterial = BodyMaterial.LoadSynchronous();
	if (VisualMesh && LoadedBodyMaterial)
	{
		VisualMesh->SetMaterial(0, LoadedBodyMaterial);
	}
	else if (VisualMesh)
	{
		UMaterialInstanceDynamic* DynamicMaterial = VisualMesh->CreateAndSetMaterialInstanceDynamic(0);
		if (DynamicMaterial)
		{
			const FLinearColor FallbackTint(0.85f, 0.04f, 0.03f, 1.0f);
			DynamicMaterial->SetVectorParameterValue(TEXT("Color"), FallbackTint);
			DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), FallbackTint);
		}
	}

	UMaterialInterface* LoadedForwardMarkerMaterial = ForwardMarkerMaterial.LoadSynchronous();
	if (ForwardMarkerMesh && LoadedForwardMarkerMaterial)
	{
		ForwardMarkerMesh->SetMaterial(0, LoadedForwardMarkerMaterial);
	}
	else if (ForwardMarkerMesh && LoadedBodyMaterial)
	{
		ForwardMarkerMesh->SetMaterial(0, LoadedBodyMaterial);
	}
}
