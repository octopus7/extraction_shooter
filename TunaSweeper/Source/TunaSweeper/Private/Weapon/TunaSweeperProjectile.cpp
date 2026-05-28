#include "Weapon/TunaSweeperProjectile.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Effect/TunaSweeperProjectileHitBurstActor.h"
#include "Effect/TunaSweeperProjectileHitEffectDataAsset.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "TunaSweeperCollisionChannels.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperProjectile::ATunaSweeperProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(12.0f);
	ApplyProjectileCollisionDefaults();
	CollisionComponent->SetNotifyRigidBodyCollision(true);
	CollisionComponent->OnComponentHit.AddDynamic(this, &ATunaSweeperProjectile::HandleHit);
	RootComponent = CollisionComponent;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(0.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMesh.Object);
	}

	TrailMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TrailMesh"));
	TrailMesh->SetupAttachment(RootComponent);
	TrailMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TrailMesh->SetGenerateOverlapEvents(false);
	TrailMesh->SetCastShadow(false);
	TrailMesh->SetHiddenInGame(true);
	TrailMesh->SetVisibility(false);

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 2200.0f;
	ProjectileMovement->MaxSpeed = 2200.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f;
}

void ATunaSweeperProjectile::BeginPlay()
{
	Super::BeginPlay();

	ApplyProjectileCollisionDefaults();
	IgnoreActor(GetOwner());
	IgnoreActor(GetInstigator());
	SetLifeSpan(LifeSeconds);
}

void ATunaSweeperProjectile::IgnoreActor(AActor* ActorToIgnore)
{
	if (!ActorToIgnore)
	{
		return;
	}

	if (CollisionComponent)
	{
		CollisionComponent->IgnoreActorWhenMoving(ActorToIgnore, true);
	}
}

void ATunaSweeperProjectile::ApplyVisualMaterial(
	UMaterialInterface* Material,
	const FLinearColor& BaseColor,
	float EmissiveStrength)
{
	if (!VisualMesh || !Material)
	{
		return;
	}

	DynamicVisualMaterial = VisualMesh->CreateDynamicMaterialInstance(0, Material);
	if (!DynamicVisualMaterial)
	{
		VisualMesh->SetMaterial(0, Material);
		return;
	}

	const float SafeEmissiveStrength = FMath::Max(0.0f, EmissiveStrength);
	const FLinearColor EmissiveColor = BaseColor * SafeEmissiveStrength;
	DynamicVisualMaterial->SetVectorParameterValue(TEXT("Color"), BaseColor);
	DynamicVisualMaterial->SetVectorParameterValue(TEXT("BaseColor"), BaseColor);
	DynamicVisualMaterial->SetVectorParameterValue(TEXT("Base Color"), BaseColor);
	DynamicVisualMaterial->SetVectorParameterValue(TEXT("LedColor"), BaseColor);
	DynamicVisualMaterial->SetVectorParameterValue(TEXT("OffColor"), BaseColor * 0.08f);
	DynamicVisualMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
	DynamicVisualMaterial->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveColor);
	DynamicVisualMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), SafeEmissiveStrength);
	DynamicVisualMaterial->SetScalarParameterValue(TEXT("Emissive Strength"), SafeEmissiveStrength);
	DynamicVisualMaterial->SetScalarParameterValue(TEXT("EmissiveIntensity"), SafeEmissiveStrength);
	DynamicVisualMaterial->SetScalarParameterValue(TEXT("Intensity"), SafeEmissiveStrength);
}

void ATunaSweeperProjectile::ApplyTrailVisual(
	UMaterialInterface* Material,
	const FLinearColor& TrailColor,
	float EmissiveStrength,
	float TrailLengthCm,
	float TrailRadiusCm,
	float Opacity,
	float EndFade)
{
	if (!TrailMesh || !Material)
	{
		return;
	}

	const float SafeTrailLength = FMath::Max(1.0f, TrailLengthCm);
	const float SafeTrailRadius = FMath::Max(0.1f, TrailRadiusCm);
	const float SafeOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	const float SafeEndFade = FMath::Clamp(EndFade, 0.0f, 1.0f);
	const float FrontX = -12.0f;
	const float TailX = FrontX - SafeTrailLength;
	const float TailRadius = SafeTrailRadius * 0.12f;
	const FLinearColor FrontColor(TrailColor.R, TrailColor.G, TrailColor.B, SafeOpacity);
	const FLinearColor TailColor(TrailColor.R, TrailColor.G, TrailColor.B, 0.0f);

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	Vertices.Reserve(8);
	Triangles.Reserve(12);
	Normals.Reserve(8);
	UVs.Reserve(8);
	VertexColors.Reserve(8);
	Tangents.Reserve(8);

	auto AddTrailQuad = [&](
		const FVector& TailA,
		const FVector& TailB,
		const FVector& FrontB,
		const FVector& FrontA,
		const FVector& Normal,
		const FProcMeshTangent& Tangent)
	{
		const int32 BaseIndex = Vertices.Num();
		Vertices.Add(TailA);
		Vertices.Add(TailB);
		Vertices.Add(FrontB);
		Vertices.Add(FrontA);
		Triangles.Add(BaseIndex);
		Triangles.Add(BaseIndex + 1);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex);
		Triangles.Add(BaseIndex + 2);
		Triangles.Add(BaseIndex + 3);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		Normals.Add(Normal);
		UVs.Add(FVector2D(0.0f, 0.0f));
		UVs.Add(FVector2D(0.0f, 1.0f));
		UVs.Add(FVector2D(1.0f, 1.0f));
		UVs.Add(FVector2D(1.0f, 0.0f));
		VertexColors.Add(TailColor);
		VertexColors.Add(TailColor);
		VertexColors.Add(FrontColor);
		VertexColors.Add(FrontColor);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
		Tangents.Add(Tangent);
	};

	AddTrailQuad(
		FVector(TailX, -TailRadius, 0.0f),
		FVector(TailX, TailRadius, 0.0f),
		FVector(FrontX, SafeTrailRadius, 0.0f),
		FVector(FrontX, -SafeTrailRadius, 0.0f),
		FVector::UpVector,
		FProcMeshTangent(FVector::ForwardVector, false));
	AddTrailQuad(
		FVector(TailX, 0.0f, -TailRadius),
		FVector(TailX, 0.0f, TailRadius),
		FVector(FrontX, 0.0f, SafeTrailRadius),
		FVector(FrontX, 0.0f, -SafeTrailRadius),
		FVector::RightVector,
		FProcMeshTangent(FVector::ForwardVector, false));

	TrailMesh->ClearAllMeshSections();
	TrailMesh->CreateMeshSection_LinearColor(
		0,
		Vertices,
		Triangles,
		Normals,
		UVs,
		VertexColors,
		Tangents,
		false);
	TrailMesh->SetHiddenInGame(false);
	TrailMesh->SetVisibility(true);

	DynamicTrailMaterial = TrailMesh->CreateDynamicMaterialInstance(0, Material);
	if (!DynamicTrailMaterial)
	{
		TrailMesh->SetMaterial(0, Material);
		return;
	}

	const float SafeEmissiveStrength = FMath::Max(0.0f, EmissiveStrength);
	const FLinearColor VisibleTrailColor(TrailColor.R, TrailColor.G, TrailColor.B, SafeOpacity);
	const FLinearColor EmissiveColor = VisibleTrailColor * SafeEmissiveStrength;
	DynamicTrailMaterial->SetVectorParameterValue(TEXT("Color"), VisibleTrailColor);
	DynamicTrailMaterial->SetVectorParameterValue(TEXT("BaseColor"), VisibleTrailColor);
	DynamicTrailMaterial->SetVectorParameterValue(TEXT("Base Color"), VisibleTrailColor);
	DynamicTrailMaterial->SetVectorParameterValue(TEXT("LedColor"), VisibleTrailColor);
	DynamicTrailMaterial->SetVectorParameterValue(TEXT("OffColor"), VisibleTrailColor * 0.05f);
	DynamicTrailMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), EmissiveColor);
	DynamicTrailMaterial->SetVectorParameterValue(TEXT("Emissive Color"), EmissiveColor);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("EmissiveStrength"), SafeEmissiveStrength);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("Emissive Strength"), SafeEmissiveStrength);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("EmissiveIntensity"), SafeEmissiveStrength);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("Intensity"), SafeEmissiveStrength);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("Opacity"), SafeOpacity);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("Alpha"), SafeOpacity);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("TrailOpacity"), SafeOpacity);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("EndFade"), SafeEndFade);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("TailFade"), SafeEndFade);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("Dissolve"), SafeEndFade);
	DynamicTrailMaterial->SetScalarParameterValue(TEXT("UVOffset"), 0.0f);
}

void ATunaSweeperProjectile::SetCameraHitReactionScale(float InCameraHitReactionScale)
{
	CameraHitReactionScale = FMath::Max(0.0f, InCameraHitReactionScale);
}

void ATunaSweeperProjectile::SetSpeedMultiplier(float InSpeedMultiplier)
{
	if (!ProjectileMovement)
	{
		return;
	}

	const float SafeSpeedMultiplier = FMath::Max(0.0f, InSpeedMultiplier);
	const float NewInitialSpeed = ProjectileMovement->InitialSpeed * SafeSpeedMultiplier;
	const float NewMaxSpeed = ProjectileMovement->MaxSpeed * SafeSpeedMultiplier;
	ProjectileMovement->InitialSpeed = NewInitialSpeed;
	ProjectileMovement->MaxSpeed = NewMaxSpeed;

	FVector MoveDirection = ProjectileMovement->Velocity.GetSafeNormal();
	if (MoveDirection.IsNearlyZero())
	{
		MoveDirection = GetActorForwardVector().GetSafeNormal();
	}
	if (!MoveDirection.IsNearlyZero())
	{
		ProjectileMovement->Velocity = MoveDirection * NewInitialSpeed;
	}
}

void ATunaSweeperProjectile::ApplyProjectileCollisionDefaults()
{
	if (!CollisionComponent)
	{
		return;
	}

	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(TunaSweeperCollisionChannels::Projectile);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Ignore);
}

void ATunaSweeperProjectile::SpawnHitEffect(
	const FHitResult& Hit,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp) const
{
	if (HitEffectId.IsNone())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FTunaSweeperProjectileHitEffectDefinition HitEffectDefinition;
	if (const UTunaSweeperGameInstance* TunaGameInstance = World->GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->TryGetProjectileHitEffectDefinition(HitEffectId, HitEffectDefinition);
	}

	TSubclassOf<ATunaSweeperProjectileHitBurstActor> HitEffectClass =
		HitEffectDefinition.EffectActorClass.LoadSynchronous();
	if (!HitEffectClass && HitEffectId == FName(TEXT("hit.red_burst")))
	{
		HitEffectClass = ATunaSweeperProjectileHitBurstActor::StaticClass();
		HitEffectDefinition.BurstColor = FLinearColor(1.0f, 0.03f, 0.0f, 1.0f);
		HitEffectDefinition.SpawnScale = FVector::OneVector;
		HitEffectDefinition.SurfaceOffsetCm = 1.0f;
	}

	if (!HitEffectClass)
	{
		return;
	}

	FVector EffectNormal = Hit.ImpactNormal.GetSafeNormal();
	if (EffectNormal.IsNearlyZero())
	{
		EffectNormal = -GetVelocity().GetSafeNormal();
	}
	if (EffectNormal.IsNearlyZero())
	{
		EffectNormal = -GetActorForwardVector();
	}

	const FVector EffectLocation =
		ResolveHitEffectLocation(Hit, OtherActor) +
		EffectNormal * FMath::Max(0.0f, HitEffectDefinition.SurfaceOffsetCm);
	const FRotator EffectRotation = EffectNormal.Rotation();
	const FVector EffectScale = HitEffectDefinition.SpawnScale.IsNearlyZero()
		? FVector::OneVector
		: HitEffectDefinition.SpawnScale;

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = GetOwner();
	SpawnParameters.Instigator = GetInstigator();
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ATunaSweeperProjectileHitBurstActor* HitEffectActor = World->SpawnActor<ATunaSweeperProjectileHitBurstActor>(
		HitEffectClass,
		EffectLocation,
		EffectRotation,
		SpawnParameters);
	if (HitEffectActor)
	{
		HitEffectActor->SetActorScale3D(EffectScale);
		HitEffectActor->SetBurstColor(HitEffectDefinition.BurstColor);
	}
}

FVector ATunaSweeperProjectile::ResolveHitEffectLocation(const FHitResult& Hit, AActor* OtherActor) const
{
	if (const ATunaSweeperEnemyCharacter* EnemyCharacter = Cast<ATunaSweeperEnemyCharacter>(OtherActor))
	{
		return EnemyCharacter->ResolveProjectileHitEffectLocation(Hit);
	}

	return Hit.ImpactPoint;
}

void ATunaSweeperProjectile::HandleHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}

	if (OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		IgnoreActor(OtherActor);
		return;
	}

	if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(OtherActor);
		TunaCharacter && TunaCharacter->IsDamageInvulnerable())
	{
		if (HitComponent)
		{
			HitComponent->IgnoreActorWhenMoving(OtherActor, true);
		}
		return;
	}

	float AppliedDamage = 0.0f;
	if (DamageAmount > 0.0f)
	{
		AppliedDamage = UGameplayStatics::ApplyDamage(
			OtherActor,
			DamageAmount,
			GetInstigatorController(),
			this,
			UDamageType::StaticClass());
	}

	if (AppliedDamage > 0.0f)
	{
		SpawnHitEffect(Hit, OtherActor, OtherComp);
	}

	Destroy();
}
