#include "AI/TunaSweeperEnemyCharacter.h"

#include "AI/TunaSweeperEnemyAIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"

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
}

void ATunaSweeperEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
	ApplyVisualMaterials();
}

bool ATunaSweeperEnemyCharacter::FireProjectileAt(AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World || !TargetActor)
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
