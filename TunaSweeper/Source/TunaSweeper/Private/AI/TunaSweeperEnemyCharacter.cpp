#include "AI/TunaSweeperEnemyCharacter.h"

#include "AI/TunaSweeperEnemyAIController.h"
#include "Component/TunaSweeperDebuffComponent.h"
#include "Component/TunaSweeperEnemySensorDebugComponent.h"
#include "Component/TunaSweeperVisionSubjectComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "Effect/TunaSweeperMeleeImpactBurstActor.h"
#include "Effect/TunaSweeperMeleeSwingTrailActor.h"
#include "Engine/StaticMesh.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Debuff/TunaSweeperDebuffTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "TimerManager.h"
#include "UI/TunaSweeperReloadRingWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "Weapon/TunaSweeperWeapon.h"
#include "Weapon/TunaSweeperWeaponSpreadRecoilDataAsset.h"

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
	constexpr float LumberjackMeleeTrackingRange = 2300.0f;
	constexpr float LumberjackMeleeAttackCooldownSeconds = 1.25f;
	constexpr float LumberjackMeleeKnockbackVelocity = 680.0f;
	constexpr float LumberjackMeleeImpactHeight = 55.0f;
	constexpr float LumberjackMeleeImpactLifetimeSeconds = 0.55f;
	constexpr int32 DefaultEnemyWeaponItemId = 1002;
	constexpr int32 DefaultEnemyAmmoItemId = 2002;
	constexpr int32 DefaultEnemyReserveMagazineCount = 2;
	constexpr float DefaultEnemyReloadSeconds = 1.8f;
	constexpr float DefaultProjectileDamageAmount = 10.0f;
	const FLinearColor LumberjackMeleeImpactColor(0.0f, 0.92f, 1.0f, 1.0f);
	const FName PistolWeaponTypeTag(TEXT("weapon.type.pistol"));
	const FName RifleWeaponTypeTag(TEXT("weapon.type.rifle"));
	const FName ShotgunWeaponTypeTag(TEXT("weapon.type.shotgun"));
	const FName SmgWeaponTypeTag(TEXT("weapon.type.smg"));
	const TCHAR* EnemyVoxelBodyMeshPath = TEXT("/Game/Characters/Enemy/SM_Enemy_VoxelBody.SM_Enemy_VoxelBody");
	const TCHAR* EnemyVoxelForwardMarkerMeshPath =
		TEXT("/Game/Characters/Enemy/SM_Enemy_VoxelForwardMarker.SM_Enemy_VoxelForwardMarker");
	const TCHAR* EnemyVoxelMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");

	float GetRandomizedEnemyValue(float BaseValue, const FVector2D& OffsetRange, float MinValue)
	{
		const float MinOffset = FMath::Min(OffsetRange.X, OffsetRange.Y);
		const float MaxOffset = FMath::Max(OffsetRange.X, OffsetRange.Y);
		return FMath::Max(MinValue, BaseValue + FMath::FRandRange(MinOffset, MaxOffset));
	}

	int32 GetDefaultAmmoItemIdForWeaponType(FName WeaponTypeTag)
	{
		if (WeaponTypeTag == PistolWeaponTypeTag)
		{
			return 2001;
		}
		if (WeaponTypeTag == ShotgunWeaponTypeTag)
		{
			return 2003;
		}
		if (WeaponTypeTag == SmgWeaponTypeTag)
		{
			return 2001;
		}

		return DefaultEnemyAmmoItemId;
	}

	bool IsAmmoCompatibleWithWeapon(
		const FTunaSweeperItemDefinition& WeaponDefinition,
		const FTunaSweeperItemDefinition& AmmoDefinition)
	{
		return !AmmoDefinition.AmmoTypeTag.IsNone() &&
			WeaponDefinition.CompatibleAmmoTypeTags.Contains(AmmoDefinition.AmmoTypeTag);
	}
}

ATunaSweeperEnemyCharacter::ATunaSweeperEnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

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

	EnemyWeaponAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("EnemyWeaponAttachPoint"));
	EnemyWeaponAttachPoint->SetupAttachment(RootComponent);
	EnemyWeaponAttachPoint->SetRelativeLocation(FVector(48.0f, 0.0f, 55.0f));

	EnemyReloadWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("EnemyReloadWidgetComponent"));
	EnemyReloadWidgetComponent->SetupAttachment(RootComponent);
	EnemyReloadWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	EnemyReloadWidgetComponent->SetWidgetClass(UTunaSweeperReloadRingWidget::StaticClass());
	EnemyReloadWidgetComponent->SetDrawSize(FVector2D(58.0f, 58.0f));
	EnemyReloadWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 138.0f));
	EnemyReloadWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	EnemyReloadWidgetComponent->SetVisibility(false);

	VisionSubjectComponent = CreateDefaultSubobject<UTunaSweeperVisionSubjectComponent>(TEXT("VisionSubject"));
	SensorDebugComponent = CreateDefaultSubobject<UTunaSweeperEnemySensorDebugComponent>(TEXT("SensorDebug"));

	ApplyVoxelVisualMeshes();

	EnemyWeaponClass = ATunaSweeperWeapon::StaticClass();
	ProjectileClass = TSoftClassPtr<ATunaSweeperProjectile>(
		FSoftObjectPath(TEXT("/Game/Weapons/BP_TunaSweeperProjectile.BP_TunaSweeperProjectile_C")));
	BodyMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(EnemyVoxelMaterialPath));
	ForwardMarkerMaterial = TSoftObjectPtr<UMaterialInterface>(
		FSoftObjectPath(EnemyVoxelMaterialPath));
	LootContainerClass = TSoftClassPtr<ATunaSweeperLootContainerActor>(
		FSoftObjectPath(TEXT("/Game/Interaction/BP_LootContainer.BP_LootContainer_C")));
	MeleeImpactBurstActorClass = TSoftClassPtr<ATunaSweeperMeleeImpactBurstActor>(
		FSoftObjectPath(TEXT("/Script/TunaSweeper.TunaSweeperMeleeImpactBurstActor")));
	MeleeSwingTrailActorClass = TSoftClassPtr<ATunaSweeperMeleeSwingTrailActor>(
		FSoftObjectPath(TEXT("/Script/TunaSweeper.TunaSweeperMeleeSwingTrailActor")));
}

void ATunaSweeperEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	GetCharacterMovement()->MaxWalkSpeed = GetRandomizedEnemyValue(MovementSpeed, MovementSpeedRandomOffset, 0.0f);
	ApplyVoxelVisualMeshes();
	ApplyVisualMaterials();
	if (!UsesMeleeAttack())
	{
		InitializeEnemyWeaponRuntime();
	}
	UpdateEnemyReloadWidget();
}

void ATunaSweeperEnemyCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDead)
	{
		return;
	}

	TickFootstepNoise(DeltaSeconds);
	CompleteEnemyReloadIfReady();
	UpdateEnemyReloadWidget();
}

void ATunaSweeperEnemyCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EnemyWeapon)
	{
		EnemyWeapon->Destroy();
		EnemyWeapon = nullptr;
	}

	Super::EndPlay(EndPlayReason);
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
		HandleDeath(EventInstigator, DamageCauser);
	}

	return AppliedDamage;
}

FVector ATunaSweeperEnemyCharacter::ResolveProjectileHitEffectLocation(const FHitResult& Hit) const
{
	return Hit.ImpactPoint;
}

void ATunaSweeperEnemyCharacter::ConfigureSpawnData(
	const TSoftObjectPtr<UMaterialInterface>& InBodyMaterial,
	FName InEnemyId,
	int32 InDropContainerDefinitionId,
	int32 InDropContentsId,
	float InMaxHealth,
	int32 InExperienceValue,
	int32 InBleedingChanceBonus,
	float InBleedingDurationBonusSeconds,
	int32 InWeaponItemId,
	int32 InAmmoItemId,
	int32 InReserveAmmoCount,
	float InLootLoadedAmmoDeductionRatio,
	int32 InLootLoadedAmmoFlatDeduction)
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

	EnemyId = InEnemyId;
	DropContainerDefinitionId = InDropContainerDefinitionId;
	DropContentsId = InDropContentsId;
	ExperienceValue = FMath::Max(0, InExperienceValue);
	BleedingChanceBonus = TunaSweeperDataValues::ClampProbabilityValue(InBleedingChanceBonus);
	BleedingDurationBonusSeconds = FMath::Max(0.0f, InBleedingDurationBonusSeconds);
	EnemyWeaponItemId = InWeaponItemId;
	EnemyAmmoItemId = InAmmoItemId;
	EnemyReserveAmmoCount = InReserveAmmoCount;
	LootLoadedAmmoDeductionRatio = FMath::Clamp(InLootLoadedAmmoDeductionRatio, 0.0f, 1.0f);
	LootLoadedAmmoFlatDeduction = FMath::Max(0, InLootLoadedAmmoFlatDeduction);
	bEnemyWeaponRuntimeInitialized = false;
	ApplyVisualMaterials();
}

void ATunaSweeperEnemyCharacter::ApplyVoxelVisualMeshes()
{
	if (VisualMesh)
	{
		if (UStaticMesh* VoxelBodyMesh = LoadObject<UStaticMesh>(nullptr, EnemyVoxelBodyMeshPath))
		{
			VisualMesh->SetStaticMesh(VoxelBodyMesh);
			VisualMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, 1.6f));
		}
	}

	if (ForwardMarkerMesh)
	{
		if (UStaticMesh* VoxelForwardMarkerMesh = LoadObject<UStaticMesh>(nullptr, EnemyVoxelForwardMarkerMeshPath))
		{
			ForwardMarkerMesh->SetStaticMesh(VoxelForwardMarkerMesh);
			ForwardMarkerMesh->SetRelativeLocation(FVector(62.0f, 0.0f, 54.0f));
			ForwardMarkerMesh->SetRelativeRotation(FRotator::ZeroRotator);
			ForwardMarkerMesh->SetRelativeScale3D(FVector::OneVector);
		}
	}
}

void ATunaSweeperEnemyCharacter::InitializeEnemyWeaponRuntime()
{
	if (bEnemyWeaponRuntimeInitialized || UsesMeleeAttack())
	{
		return;
	}

	bEnemyWeaponRuntimeInitialized = true;
	EnemyWeaponTypeTag = NAME_None;
	EnemyImpactProfileId = NAME_None;
	EnemyProjectileHitEffectId = ProjectileHitEffectId;
	EnemyProjectileDamageMultiplier = 1.0f;
	EnemyProjectileDamageBonus = 0;
	EnemyMagazineCapacity = 0;
	EnemyLoadedAmmoCount = 0;
	PendingEnemyReloadAmmoCount = 0;
	EnemyReloadSeconds = DefaultEnemyReloadSeconds;

	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = nullptr;
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (TunaGameInstance)
	{
		ItemDataSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	}
	if (!ItemDataSubsystem)
	{
		return;
	}

	FTunaSweeperItemDefinition WeaponDefinition;
	const int32 RequestedWeaponItemId = EnemyWeaponItemId != INDEX_NONE
		? EnemyWeaponItemId
		: DefaultEnemyWeaponItemId;
	if (!ItemDataSubsystem->TryGetItemDefinition(RequestedWeaponItemId, WeaponDefinition) ||
		WeaponDefinition.WeaponTypeTag.IsNone())
	{
		if (!ItemDataSubsystem->TryGetItemDefinition(DefaultEnemyWeaponItemId, WeaponDefinition) ||
			WeaponDefinition.WeaponTypeTag.IsNone())
		{
			return;
		}
	}

	FTunaSweeperItemDefinition AmmoDefinition;
	const int32 RequestedAmmoItemId = EnemyAmmoItemId != INDEX_NONE
		? EnemyAmmoItemId
		: GetDefaultAmmoItemIdForWeaponType(WeaponDefinition.WeaponTypeTag);
	if (!ItemDataSubsystem->TryGetItemDefinition(RequestedAmmoItemId, AmmoDefinition) ||
		!IsAmmoCompatibleWithWeapon(WeaponDefinition, AmmoDefinition))
	{
		const int32 DefaultAmmoItemId = GetDefaultAmmoItemIdForWeaponType(WeaponDefinition.WeaponTypeTag);
		if (!ItemDataSubsystem->TryGetItemDefinition(DefaultAmmoItemId, AmmoDefinition) ||
			!IsAmmoCompatibleWithWeapon(WeaponDefinition, AmmoDefinition))
		{
			return;
		}
	}

	EnemyWeaponItemId = WeaponDefinition.Id;
	EnemyAmmoItemId = AmmoDefinition.Id;
	EnemyWeaponTypeTag = WeaponDefinition.WeaponTypeTag;
	EnemyMagazineCapacity = FMath::Max(1, WeaponDefinition.MagazineCapacity);
	EnemyLoadedAmmoCount = EnemyMagazineCapacity;
	EnemyReloadSeconds = WeaponDefinition.ReloadSeconds > 0.0f
		? WeaponDefinition.ReloadSeconds
		: DefaultEnemyReloadSeconds;
	EnemyImpactProfileId = AmmoDefinition.ImpactProfileId;
	EnemyProjectileHitEffectId = AmmoDefinition.ProjectileHitEffectId;
	EnemyProjectileDamageMultiplier = TunaSweeperDataValues::ToRatioFloat(AmmoDefinition.ProjectileDamageMultiplier);
	EnemyProjectileDamageBonus = AmmoDefinition.ProjectileDamageBonus;

	if (EnemyReserveAmmoCount == INDEX_NONE)
	{
		EnemyReserveAmmoCount = EnemyMagazineCapacity * DefaultEnemyReserveMagazineCount;
	}
	else
	{
		EnemyReserveAmmoCount = FMath::Max(0, EnemyReserveAmmoCount);
	}

	if (EnsureEnemyWeaponActor() && TunaGameInstance)
	{
		FTunaSweeperWeaponSpreadRecoilDefinition RecoilDefinition;
		if (TunaGameInstance->TryGetWeaponSpreadRecoilDefinition(EnemyWeaponTypeTag, RecoilDefinition))
		{
			EnemyWeapon->ConfigureRuntimeSpreadRecoil(EnemyWeaponTypeTag, RecoilDefinition);
		}
	}
}

bool ATunaSweeperEnemyCharacter::EnsureEnemyWeaponActor()
{
	if (EnemyWeapon)
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	TSubclassOf<ATunaSweeperWeapon> LoadedWeaponClass = EnemyWeaponClass;
	if (!LoadedWeaponClass)
	{
		LoadedWeaponClass = ATunaSweeperWeapon::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	EnemyWeapon = World->SpawnActor<ATunaSweeperWeapon>(
		LoadedWeaponClass,
		GetActorLocation(),
		GetActorRotation(),
		SpawnParameters);
	if (!EnemyWeapon)
	{
		return false;
	}

	EnemyWeapon->SetActorEnableCollision(false);
	EnemyWeapon->SetActorHiddenInGame(true);
	EnemyWeapon->ConfigureGunVisual();
	USceneComponent* AttachParent = EnemyWeaponAttachPoint ? EnemyWeaponAttachPoint.Get() : GetRootComponent();
	if (AttachParent)
	{
		EnemyWeapon->AttachToComponent(AttachParent, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	return true;
}

bool ATunaSweeperEnemyCharacter::StartEnemyReload()
{
	if (!EnemyWeapon || EnemyWeapon->IsReloadRuntimeActive() || EnemyMagazineCapacity <= 0)
	{
		return false;
	}

	const int32 MissingAmmoCount = EnemyMagazineCapacity - EnemyLoadedAmmoCount;
	if (MissingAmmoCount <= 0 || EnemyReserveAmmoCount <= 0)
	{
		return false;
	}

	PendingEnemyReloadAmmoCount = FMath::Min(MissingAmmoCount, EnemyReserveAmmoCount);
	if (PendingEnemyReloadAmmoCount <= 0)
	{
		return false;
	}

	return EnemyWeapon->StartReloadRuntime(EnemyReloadSeconds);
}

void ATunaSweeperEnemyCharacter::CompleteEnemyReloadIfReady()
{
	if (!EnemyWeapon || !EnemyWeapon->HasReloadRuntimeFinished())
	{
		return;
	}

	const int32 ReloadedAmmoCount = FMath::Clamp(
		PendingEnemyReloadAmmoCount,
		0,
		FMath::Min(EnemyMagazineCapacity - EnemyLoadedAmmoCount, EnemyReserveAmmoCount));
	EnemyLoadedAmmoCount = FMath::Clamp(EnemyLoadedAmmoCount + ReloadedAmmoCount, 0, EnemyMagazineCapacity);
	EnemyReserveAmmoCount = FMath::Max(0, EnemyReserveAmmoCount - ReloadedAmmoCount);
	PendingEnemyReloadAmmoCount = 0;
	EnemyWeapon->FinishReloadRuntime();
}

void ATunaSweeperEnemyCharacter::UpdateEnemyReloadWidget()
{
	if (!EnemyReloadWidgetComponent)
	{
		return;
	}

	const bool bShowReloadWidget = !bIsDead && EnemyWeapon && EnemyWeapon->IsReloadRuntimeActive();
	EnemyReloadWidgetComponent->SetVisibility(bShowReloadWidget);
	EnemyReloadWidgetComponent->SetHiddenInGame(!bShowReloadWidget);
	if (!bShowReloadWidget)
	{
		return;
	}

	EnemyReloadWidgetComponent->InitWidget();
	if (UTunaSweeperReloadRingWidget* ReloadWidget =
		Cast<UTunaSweeperReloadRingWidget>(EnemyReloadWidgetComponent->GetUserWidgetObject()))
	{
		ReloadWidget->SetReloadProgress(EnemyWeapon->GetReloadRuntimeProgress(), true);
	}
}

void ATunaSweeperEnemyCharacter::TickFootstepNoise(float DeltaSeconds)
{
	if (FootstepNoiseLoudness <= 0.0f || FootstepNoiseMaxRange <= 0.0f || FootstepNoiseIntervalSeconds <= 0.0f)
	{
		FootstepNoiseElapsedSeconds = 0.0f;
		return;
	}

	FVector HorizontalVelocity = GetVelocity();
	HorizontalVelocity.Z = 0.0f;
	if (HorizontalVelocity.Size() < FootstepNoiseMinSpeed)
	{
		FootstepNoiseElapsedSeconds = 0.0f;
		return;
	}

	const float SafeIntervalSeconds = FMath::Max(0.05f, FootstepNoiseIntervalSeconds);
	FootstepNoiseElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	if (FootstepNoiseElapsedSeconds < SafeIntervalSeconds)
	{
		return;
	}

	FootstepNoiseElapsedSeconds = FMath::Fmod(FootstepNoiseElapsedSeconds, SafeIntervalSeconds);
	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
		{
			NoiseSubsystem->ReportNoiseAtLocation(
				GetActorTransform().TransformPosition(FootstepNoiseSourceOffset),
				FootstepNoiseLoudness,
				FootstepNoiseMaxRange,
				FootstepNoiseTag,
				this,
				this);
		}
	}
}

int32 ATunaSweeperEnemyCharacter::ResolveLootLoadedAmmoCount(
	int32& OutSourceLoadedAmmoCount,
	int32& OutDeductedLoadedAmmoCount) const
{
	OutSourceLoadedAmmoCount = FMath::Clamp(EnemyLoadedAmmoCount, 0, FMath::Max(0, EnemyMagazineCapacity));
	const float SafeDeductionRatio = FMath::Clamp(LootLoadedAmmoDeductionRatio, 0.0f, 1.0f);
	const int32 RatioDeduction = FMath::FloorToInt(static_cast<float>(OutSourceLoadedAmmoCount) * SafeDeductionRatio);
	const int32 FlatDeduction = FMath::Max(0, LootLoadedAmmoFlatDeduction);
	OutDeductedLoadedAmmoCount = FMath::Clamp(RatioDeduction + FlatDeduction, 0, OutSourceLoadedAmmoCount);

	return FMath::Max(0, OutSourceLoadedAmmoCount - OutDeductedLoadedAmmoCount);
}

bool ATunaSweeperEnemyCharacter::TryCreateEnemyWeaponLootInstance(
	UTunaSweeperGameInstance* TunaGameInstance,
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
	FGuid& OutWeaponUid) const
{
	OutWeaponUid.Invalidate();
	if (!TunaGameInstance || !ItemDataSubsystem || EnemyWeaponItemId == INDEX_NONE)
	{
		return false;
	}

	FTunaSweeperItemDefinition WeaponDefinition;
	if (!ItemDataSubsystem->TryGetItemDefinition(EnemyWeaponItemId, WeaponDefinition))
	{
		return false;
	}

	FTunaSweeperItemInstance WeaponInstance;
	WeaponInstance.ItemId = EnemyWeaponItemId;
	WeaponInstance.Quantity = 1;
	WeaponInstance.LoadedAmmoItemId = EnemyAmmoItemId;
	WeaponInstance.SelectedAmmoItemId = EnemyAmmoItemId;

	int32 SourceLoadedAmmoCount = 0;
	int32 DeductedLoadedAmmoCount = 0;
	WeaponInstance.LoadedAmmoCount = ResolveLootLoadedAmmoCount(SourceLoadedAmmoCount, DeductedLoadedAmmoCount);
	WeaponInstance.LootLoadedAmmoSourceCount = SourceLoadedAmmoCount;
	WeaponInstance.LootLoadedAmmoDeductedCount = DeductedLoadedAmmoCount;
	WeaponInstance.LootLoadedAmmoDeductionRatio = FMath::Clamp(LootLoadedAmmoDeductionRatio, 0.0f, 1.0f);
	WeaponInstance.LootLoadedAmmoFlatDeduction = FMath::Max(0, LootLoadedAmmoFlatDeduction);

	OutWeaponUid = TunaGameInstance->CreateItemInstanceFromTemplate(WeaponInstance);
	return OutWeaponUid.IsValid();
}

bool ATunaSweeperEnemyCharacter::TryBuildDeathLootRuntimeItemUids(
	FTunaSweeperLootContainerInstance& OutContainerInstance,
	TArray<FGuid>& OutRuntimeItemUids) const
{
	OutContainerInstance = FTunaSweeperLootContainerInstance();
	OutRuntimeItemUids.Reset();

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	if (!TunaGameInstance || !ItemDataSubsystem)
	{
		return false;
	}

	const ETunaSweeperItemTextLanguage Language = TunaGameInstance->GetCurrentTextLanguage();
	if (!ItemDataSubsystem->TryBuildLootContainerInstance(
		DropContainerDefinitionId,
		DropContentsId,
		Language,
		OutContainerInstance))
	{
		return false;
	}

	bool bCreatedEnemyWeapon = false;
	for (const FTunaSweeperItemStack& ItemStack : OutContainerInstance.Items)
	{
		if (OutRuntimeItemUids.Num() >= OutContainerInstance.Capacity)
		{
			break;
		}

		if (!bCreatedEnemyWeapon && ItemStack.ItemId == EnemyWeaponItemId)
		{
			FGuid WeaponUid;
			if (TryCreateEnemyWeaponLootInstance(TunaGameInstance, ItemDataSubsystem, WeaponUid))
			{
				OutRuntimeItemUids.Add(WeaponUid);
				bCreatedEnemyWeapon = true;
				continue;
			}
		}

		FTunaSweeperItemInstance ItemInstance;
		ItemInstance.ItemId = ItemStack.ItemId;
		ItemInstance.Quantity = ItemStack.Quantity;
		const FGuid ItemUid = TunaGameInstance->CreateItemInstanceFromTemplate(ItemInstance);
		if (ItemUid.IsValid())
		{
			OutRuntimeItemUids.Add(ItemUid);
		}
	}

	return OutRuntimeItemUids.Num() > 0;
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

bool ATunaSweeperEnemyCharacter::TryApplyBleedTo(AActor* TargetActor) const
{
	if (!TargetActor || TargetActor == this || bIsDead)
	{
		return false;
	}

	UTunaSweeperDebuffComponent* DebuffComponent = TargetActor->FindComponentByClass<UTunaSweeperDebuffComponent>();
	return DebuffComponent
		? DebuffComponent->TryApplyDebuff(
			TunaSweeperDebuff::BleedingDebuffId(),
			BleedingChanceBonus,
			BleedingDurationBonusSeconds,
			const_cast<ATunaSweeperEnemyCharacter*>(this))
		: false;
}

bool ATunaSweeperEnemyCharacter::FireProjectileAt(AActor* TargetActor)
{
	UWorld* World = GetWorld();
	if (!World || !TargetActor || bIsDead || UsesMeleeAttack())
	{
		return false;
	}

	InitializeEnemyWeaponRuntime();
	if (!EnemyWeapon || EnemyWeaponTypeTag.IsNone() || EnemyMagazineCapacity <= 0)
	{
		return false;
	}
	if (EnemyWeapon->IsReloadRuntimeActive())
	{
		return false;
	}
	if (EnemyLoadedAmmoCount <= 0)
	{
		StartEnemyReload();
		return false;
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

	const float ProjectileDamageScale = ProjectileDamage > 0.0f
		? ProjectileDamage / DefaultProjectileDamageAmount
		: 0.0f;
	const bool bFired = EnemyWeapon->FireWithAimIntent(
		FireDirection,
		this,
		EnemyImpactProfileId,
		EnemyProjectileHitEffectId,
		EnemyWeaponTypeTag,
		ProjectileDamageScale * EnemyProjectileDamageMultiplier,
		EnemyProjectileDamageBonus,
		EnemyWeapon->GetRuntimeSpreadHalfAngleDegrees(),
		TargetLocation,
		true,
		TargetActor,
		nullptr,
		TargetLocation,
		true);
	if (!bFired)
	{
		return false;
	}

	EnemyLoadedAmmoCount = FMath::Max(0, EnemyLoadedAmmoCount - 1);
	EnemyWeapon->AddRuntimeSpreadRecoilShot();
	if (EnemyLoadedAmmoCount <= 0)
	{
		StartEnemyReload();
	}

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
	const FVector ResolvedAttackDirection = AttackDirection.IsNearlyZero() ? GetActorForwardVector() : AttackDirection;
	if (!ResolvedAttackDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, ResolvedAttackDirection.Rotation().Yaw, 0.0f));
	}

	SpawnMeleeSwingEffect(ResolvedAttackDirection);

	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		TargetActor,
		LumberjackMeleeDamage,
		GetController(),
		this,
		UDamageType::StaticClass());
	if (AppliedDamage <= 0.0f)
	{
		return false;
	}

	TryApplyBleedTo(TargetActor);
	ApplyMeleeKnockbackTo(TargetActor, ResolvedAttackDirection);

	const FVector HitLocation = TargetLocation + FVector(0.0f, 0.0f, LumberjackMeleeImpactHeight);
	SpawnMeleeImpactBurst(HitLocation, -ResolvedAttackDirection);
	return true;
}

void ATunaSweeperEnemyCharacter::ApplyMeleeKnockbackTo(
	AActor* TargetActor,
	const FVector& AttackDirection) const
{
	if (!TargetActor || AttackDirection.IsNearlyZero())
	{
		return;
	}

	const FVector KnockbackVelocity = AttackDirection.GetSafeNormal2D() * LumberjackMeleeKnockbackVelocity;
	if (ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
	{
		if (UCharacterMovementComponent* TargetMovement = TargetCharacter->GetCharacterMovement())
		{
			TargetMovement->AddImpulse(KnockbackVelocity, true);
			return;
		}
	}

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(TargetActor->GetRootComponent()))
	{
		RootPrimitive->AddImpulse(KnockbackVelocity, NAME_None, true);
	}
}

void ATunaSweeperEnemyCharacter::SpawnMeleeSwingEffect(const FVector& AttackDirection)
{
	UWorld* World = GetWorld();
	if (!World || AttackDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator EffectRotation(0.0f, AttackDirection.Rotation().Yaw, 0.0f);
	TSubclassOf<ATunaSweeperMeleeSwingTrailActor> LoadedTrailClass = MeleeSwingTrailActorClass.LoadSynchronous();
	if (!LoadedTrailClass)
	{
		LoadedTrailClass = ATunaSweeperMeleeSwingTrailActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<ATunaSweeperMeleeSwingTrailActor>(
		LoadedTrailClass,
		GetActorLocation(),
		EffectRotation,
		SpawnParameters);
}

void ATunaSweeperEnemyCharacter::SpawnMeleeImpactBurst(
	const FVector& HitLocation,
	const FVector& BurstDirection)
{
	UWorld* World = GetWorld();
	const FVector SafeBurstDirection = BurstDirection.GetSafeNormal2D();
	if (!World || SafeBurstDirection.IsNearlyZero())
	{
		return;
	}

	const FRotator BurstRotation(0.0f, SafeBurstDirection.Rotation().Yaw, 0.0f);
	if (UNiagaraSystem* LoadedImpactEffect = MeleeSwingEffect.LoadSynchronous())
	{
		UNiagaraComponent* ImpactComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			World,
			LoadedImpactEffect,
			HitLocation,
			BurstRotation,
			FVector(1.15f),
			true,
			true);
		if (ImpactComponent)
		{
			ImpactComponent->SetVariableLinearColor(TEXT("User.Color"), LumberjackMeleeImpactColor);
			ImpactComponent->SetVariableLinearColor(TEXT("User.Tint"), LumberjackMeleeImpactColor);

			const TWeakObjectPtr<UNiagaraComponent> WeakImpactComponent = ImpactComponent;
			FTimerHandle DestroyTimerHandle;
			World->GetTimerManager().SetTimer(
				DestroyTimerHandle,
				FTimerDelegate::CreateLambda([WeakImpactComponent]()
				{
					if (UNiagaraComponent* Component = WeakImpactComponent.Get())
					{
						Component->DeactivateImmediate();
						Component->DestroyComponent();
					}
				}),
				LumberjackMeleeImpactLifetimeSeconds,
				false);
		}
	}

	TSubclassOf<ATunaSweeperMeleeImpactBurstActor> LoadedBurstClass =
		MeleeImpactBurstActorClass.LoadSynchronous();
	if (!LoadedBurstClass)
	{
		LoadedBurstClass = ATunaSweeperMeleeImpactBurstActor::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	World->SpawnActor<ATunaSweeperMeleeImpactBurstActor>(
		LoadedBurstClass,
		HitLocation,
		BurstRotation,
		SpawnParameters);
}

void ATunaSweeperEnemyCharacter::HandleDeath(AController* KillerController, AActor* DamageCauser)
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
	if (KillerController && KillerController->IsPlayerController())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				QuestSubsystem->NotifyEnemyKilled(EnemyId);
			}
			if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GameInstance))
			{
				TunaGameInstance->AddRaidExperience(ExperienceValue);
			}
		}
	}
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
	if (!UsesMeleeAttack())
	{
		FTunaSweeperLootContainerInstance RuntimeContainerInstance;
		TArray<FGuid> RuntimeItemUids;
		if (TryBuildDeathLootRuntimeItemUids(RuntimeContainerInstance, RuntimeItemUids))
		{
			SpawnedContainer->SetRuntimeContainerItemUids(RuntimeContainerInstance, RuntimeItemUids);
		}
	}
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
