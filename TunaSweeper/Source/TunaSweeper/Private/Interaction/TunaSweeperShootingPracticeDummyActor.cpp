#include "Interaction/TunaSweeperShootingPracticeDummyActor.h"

#include "Components/SceneComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TunaSweeperCollisionChannels.h"
#include "UI/TunaSweeperPracticeDummyHealthBarWidget.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	void SetMaterialColor(UStaticMeshComponent* MeshComponent, const FLinearColor& Color)
	{
		if (!MeshComponent)
		{
			return;
		}

		UMaterialInstanceDynamic* DynamicMaterial = MeshComponent->CreateAndSetMaterialInstanceDynamic(0);
		if (!DynamicMaterial)
		{
			return;
		}

		DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
		DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
		DynamicMaterial->SetVectorParameterValue(TEXT("Base Color"), Color);
		DynamicMaterial->SetVectorParameterValue(TEXT("Tint"), Color);
		DynamicMaterial->SetScalarParameterValue(TEXT("Roughness"), 0.82f);
	}
}

ATunaSweeperShootingPracticeDummyActor::ATunaSweeperShootingPracticeDummyActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 92.0f));
	BodyMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 1.34f));

	CriticalPlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CriticalPlateMesh"));
	CriticalPlateMesh->SetupAttachment(RootComponent);
	CriticalPlateMesh->SetRelativeLocation(FVector(37.0f, 0.0f, 44.0f));
	CriticalPlateMesh->SetRelativeScale3D(FVector(0.08f, 0.42f, 0.24f));

	HeadshotPlateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadshotPlateMesh"));
	HeadshotPlateMesh->SetupAttachment(RootComponent);
	HeadshotPlateMesh->SetRelativeLocation(FVector(46.0f, 0.0f, 44.0f));
	HeadshotPlateMesh->SetRelativeScale3D(FVector(0.055f, 0.16f, 0.16f));

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(RootComponent);
	HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 190.0f));
	HeadMesh->SetRelativeScale3D(FVector(0.42f, 0.42f, 0.42f));

	HealthBarWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("HealthBarWidgetComponent"));
	HealthBarWidgetComponent->SetupAttachment(RootComponent);
	HealthBarWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 244.0f));
	HealthBarWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	HealthBarWidgetComponent->SetDrawSize(FVector2D(138.0f, 13.0f));
	HealthBarWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	HealthBarWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBarWidgetComponent->SetWidgetClass(UTunaSweeperPracticeDummyHealthBarWidget::StaticClass());

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (CubeMesh.Succeeded())
	{
		CriticalPlateMesh->SetStaticMesh(CubeMesh.Object);
		HeadshotPlateMesh->SetStaticMesh(CubeMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		HeadMesh->SetStaticMesh(SphereMesh.Object);
	}

	ConfigureHitComponent(BodyMesh);
	ConfigureHitComponent(CriticalPlateMesh);
	ConfigureHitComponent(HeadshotPlateMesh);
	ConfigureHitComponent(HeadMesh);
}

void ATunaSweeperShootingPracticeDummyActor::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	MinimumHealth = FMath::Clamp(MinimumHealth, 0.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	ApplyHitZoneColors();
	RefreshHealthBar();
}

void ATunaSweeperShootingPracticeDummyActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (CurrentHealth >= MaxHealth)
	{
		return;
	}

	const float RecoveryPerSecond = MaxHealth / FMath::Max(0.05f, HealthRecoverySeconds);
	CurrentHealth = FMath::Min(MaxHealth, CurrentHealth + RecoveryPerSecond * FMath::Max(0.0f, DeltaSeconds));
	RefreshHealthBar();
}

float ATunaSweeperShootingPracticeDummyActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AdjustedDamage = DamageAmount * ResolveDamageMultiplierForNextHit();
	ApplyDummyDamage(AdjustedDamage);
	return AdjustedDamage;
}

void ATunaSweeperShootingPracticeDummyActor::ConfigurePracticeDummyDefaults(
	float InMaxHealth,
	float InCriticalDamageMultiplier,
	float InHeadshotDamageMultiplier,
	float InHealthRecoverySeconds)
{
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	MinimumHealth = FMath::Clamp(MinimumHealth, 0.0f, MaxHealth);
	CurrentHealth = FMath::Clamp(CurrentHealth, MinimumHealth, MaxHealth);
	CriticalDamageMultiplier = FMath::Max(1.0f, InCriticalDamageMultiplier);
	HeadshotDamageMultiplier = FMath::Max(CriticalDamageMultiplier, InHeadshotDamageMultiplier);
	HealthRecoverySeconds = FMath::Max(0.05f, InHealthRecoverySeconds);
	RefreshHealthBar();
}

float ATunaSweeperShootingPracticeDummyActor::GetHealthFraction() const
{
	return MaxHealth > 0.0f ? FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f) : 0.0f;
}

void ATunaSweeperShootingPracticeDummyActor::ConfigureHitComponent(UStaticMeshComponent* MeshComponent) const
{
	if (!MeshComponent)
	{
		return;
	}

	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Ignore);
	MeshComponent->SetGenerateOverlapEvents(false);
	MeshComponent->SetNotifyRigidBodyCollision(true);
	MeshComponent->SetCanEverAffectNavigation(false);
}

void ATunaSweeperShootingPracticeDummyActor::ApplyHitZoneColors()
{
	SetMaterialColor(BodyMesh, FLinearColor(0.36f, 0.42f, 0.48f, 1.0f));
	SetMaterialColor(CriticalPlateMesh, FLinearColor(1.0f, 0.55f, 0.08f, 1.0f));
	SetMaterialColor(HeadshotPlateMesh, FLinearColor(1.0f, 0.04f, 0.02f, 1.0f));
	SetMaterialColor(HeadMesh, FLinearColor(0.95f, 0.16f, 0.10f, 1.0f));
}

float ATunaSweeperShootingPracticeDummyActor::ResolveDamageMultiplierForNextHit()
{
	++ReceivedHitCount;

	if (ReceivedHitCount % 6 == 0)
	{
		return HeadshotDamageMultiplier;
	}
	if (ReceivedHitCount % 3 == 0)
	{
		return CriticalDamageMultiplier;
	}

	return 1.0f;
}

void ATunaSweeperShootingPracticeDummyActor::ApplyDummyDamage(float DamageAmount)
{
	if (DamageAmount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - DamageAmount, MinimumHealth, MaxHealth);
	RefreshHealthBar();
}

void ATunaSweeperShootingPracticeDummyActor::RefreshHealthBar()
{
	if (!HealthBarWidgetComponent)
	{
		return;
	}

	HealthBarWidgetComponent->InitWidget();
	if (UTunaSweeperPracticeDummyHealthBarWidget* HealthBarWidget =
		Cast<UTunaSweeperPracticeDummyHealthBarWidget>(HealthBarWidgetComponent->GetUserWidgetObject()))
	{
		HealthBarWidget->SetHealthFraction(GetHealthFraction());
	}
}
