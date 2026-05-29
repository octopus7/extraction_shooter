#include "Character/TunaSweeperTopDownCharacter.h"

#include "Camera/CameraComponent.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Component/TunaSweeperPlayerVisionComponent.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Effect/TunaSweeperMeleeImpactBurstActor.h"
#include "Effect/TunaSweeperMeleeSwingTrailActor.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/OverlapResult.h"
#include "Engine/StaticMesh.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "MediaSource.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "TimerManager.h"
#include "TunaSweeperCollisionChannels.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UI/TunaSweeperStaminaGaugeWidget.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperWeapon.h"

namespace TunaSweeperEquippedWeaponVisual
{
	const FName GunCategoryTag(TEXT("item.category.weapon.gun"));
	const FSoftObjectPath AssaultRifleClassPath(TEXT("/Game/Weapons/BP_AssaultRifle.BP_AssaultRifle_C"));
	constexpr int32 BaseballBatItemId = 1005;
	const FSoftObjectPath BaseballBatMeshPath(TEXT("/Game/Weapons/SM_BaseballBat.SM_BaseballBat"));
	const FSoftObjectPath BaseballBatMaterialPath(TEXT("/Game/Weapons/M_BaseballBat_Wood.M_BaseballBat_Wood"));
}

namespace TunaSweeperStaminaGauge
{
	constexpr float DrawSize = 70.0f;
	const FVector RelativeLocation(0.0f, 0.0f, -92.0f);
}

namespace TunaSweeperSandbagCoverOutline
{
	const TCHAR* PostProcessMaterialPath = TEXT("/Game/Interaction/M_SandbagCover_Outline.M_SandbagCover_Outline");
}

ATunaSweeperTopDownCharacter::ATunaSweeperTopDownCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

	GetMesh()->SetHiddenInGame(false);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.75f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CylinderMesh.Object);
	}

	WeaponAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponAttachPoint"));
	WeaponAttachPoint->SetupAttachment(RootComponent);
	WeaponAttachPoint->SetRelativeLocation(FVector(35.0f, 0.0f, 38.0f));

	RollWeaponHandAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RollWeaponHandAttachPoint"));
	RollWeaponHandAttachPoint->SetupAttachment(GetMesh());
	RollWeaponHandAttachPoint->SetRelativeLocation(FVector(35.0f, 0.0f, 38.0f));

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1200.0f;
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;

	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
	TopDownCamera->FieldOfView = DefaultCameraFOV;

	TopDownCameraModeSettings.TargetArmLength = 1500.0f;
	TopDownCameraModeSettings.BoomRotation = FRotator(-88.0f, 0.0f, 0.0f);
	TopDownCameraModeSettings.TargetOffset = FVector::ZeroVector;
	TopDownCameraModeSettings.DefaultFOV = 70.0f;
	TopDownCameraModeSettings.AimFOV = 60.0f;

	LowFrontCameraModeSettings.TargetArmLength = 650.0f;
	LowFrontCameraModeSettings.BoomRotation = FRotator(-18.0f, 0.0f, 0.0f);
	LowFrontCameraModeSettings.TargetOffset = FVector(0.0f, 0.0f, 88.0f);
	LowFrontCameraModeSettings.DefaultFOV = 65.0f;
	LowFrontCameraModeSettings.AimFOV = 60.0f;

	VitalsComponent = CreateDefaultSubobject<UTunaSweeperVitalsComponent>(TEXT("VitalsComponent"));
	PlayerVisionComponent = CreateDefaultSubobject<UTunaSweeperPlayerVisionComponent>(TEXT("PlayerVisionComponent"));
	StaminaGaugeWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("StaminaGaugeWidget"));
	StaminaGaugeWidgetComponent->SetupAttachment(RootComponent);
	StaminaGaugeWidgetComponent->SetRelativeLocation(TunaSweeperStaminaGauge::RelativeLocation);
	StaminaGaugeWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	StaminaGaugeWidgetComponent->SetWidgetClass(UTunaSweeperStaminaGaugeWidget::StaticClass());
	StaminaGaugeWidgetComponent->SetDrawSize(FVector2D(TunaSweeperStaminaGauge::DrawSize, TunaSweeperStaminaGauge::DrawSize));
	StaminaGaugeWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	StaminaGaugeWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaminaGaugeWidgetComponent->SetWindowFocusable(false);
	StaminaGaugeWidgetComponent->SetHiddenInGame(false);
	StaminaGaugeWidgetComponent->SetVisibility(false);

	DefaultMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_Player.IMC_Player")));
	MoveAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Move.IA_Move")));
	FireAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Fire.IA_Fire")));
	AimAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Aim.IA_Aim")));
	InteractAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Interact.IA_Interact")));
	InventoryAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Inventory.IA_Inventory")));
	MapAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Map.IA_Map")));
	ReloadAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Reload.IA_Reload")));
	AmmoSelectAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_AmmoSelect.IA_AmmoSelect")));
	AmmoFocusAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_AmmoFocus.IA_AmmoFocus")));
	CameraModeAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_CameraMode.IA_CameraMode")));
	SprintAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Sprint.IA_Sprint")));
	RollAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Roll.IA_Roll")));
	DefaultWeaponClass = TSoftClassPtr<ATunaSweeperWeapon>(TunaSweeperEquippedWeaponVisual::AssaultRifleClassPath);
	MeleeSwingTrailActorClass = TSoftClassPtr<ATunaSweeperMeleeSwingTrailActor>(
		FSoftObjectPath(TEXT("/Script/TunaSweeper.TunaSweeperMeleeSwingTrailActor")));
	MeleeImpactBurstActorClass = TSoftClassPtr<ATunaSweeperMeleeImpactBurstActor>(
		FSoftObjectPath(TEXT("/Script/TunaSweeper.TunaSweeperMeleeImpactBurstActor")));
	RespawnMediaSource = TSoftObjectPtr<UMediaSource>(FSoftObjectPath(TEXT("/Game/Movies/MS_Respawn.MS_Respawn")));
	RespawnTransitionWidgetClass = TSoftClassPtr<UTunaSweeperLevelTransitionWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_LevelTransitionVideo.WBP_LevelTransitionVideo_C")));

	FTunaSweeperCameraHitReactionSettings ProjectileCameraHitReaction;
	ProjectileCameraHitReaction.Duration = 0.18f;
	ProjectileCameraHitReaction.LocationAmplitude = 10.0f;
	ProjectileCameraHitReaction.RollAmplitudeDegrees = 0.08f;
	ProjectileCameraHitReaction.FOVAmplitudeDegrees = 0.0f;
	ProjectileCameraHitReaction.Frequency = 8.0f;
	ProjectileCameraHitReaction.DamageScaleReference = 5.0f;
	ProjectileCameraHitReaction.MinDamageScale = 0.0f;
	ProjectileCameraHitReaction.MaxDamageScale = 1.0f;
	CameraHitReactionOverrides.Add(ETunaSweeperHitReactionType::Projectile, ProjectileCameraHitReaction);
}

void ATunaSweeperTopDownCharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	RefreshCharacterVisualVisibility();
}

void ATunaSweeperTopDownCharacter::BeginPlay()
{
	Super::BeginPlay();

	RefreshCharacterVisualVisibility();
	ApplySandbagCoverOutlinePostProcess();

	DefaultCameraFOV = TopDownCamera ? TopDownCamera->FieldOfView : DefaultCameraFOV;
	CurrentCameraBaseFOV = DefaultCameraFOV;
	DefaultCameraRelativeRotation = TopDownCamera ? TopDownCamera->GetRelativeRotation() : DefaultCameraRelativeRotation;
	DefaultCameraArmLength = CameraBoom ? CameraBoom->TargetArmLength : DefaultCameraArmLength;
	CurrentCameraArmLength = DefaultCameraArmLength;
	DefaultCameraBoomRotation = CameraBoom ? CameraBoom->GetRelativeRotation() : DefaultCameraBoomRotation;
	CurrentCameraBoomRotation = DefaultCameraBoomRotation;
	DefaultCameraTargetOffset = CameraBoom ? CameraBoom->TargetOffset : DefaultCameraTargetOffset;
	CurrentCameraModeOffset = DefaultCameraTargetOffset;
	CurrentCameraAimOffset = FVector::ZeroVector;
	DefaultSkeletalMeshRelativeRotation = GetMesh() ? GetMesh()->GetRelativeRotation() : FRotator::ZeroRotator;
	DefaultVisualMeshRelativeRotation = VisualMesh ? VisualMesh->GetRelativeRotation() : FRotator::ZeroRotator;
	CurrentStamina = FMath::Max(0.0f, MaxStamina);
	StaminaGaugeOpacity = 0.0f;

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &ATunaSweeperTopDownCharacter::RefreshSelectedWeaponAfterInventoryChanged);
	}

	if (VitalsComponent)
	{
		VitalsComponent->OnVitalsChanged.AddDynamic(this, &ATunaSweeperTopDownCharacter::HandleVitalsChanged);
		HandleVitalsChanged(VitalsComponent->GetVitalsState());
	}

	if (!RestoreRuntimeSelectedWeaponSelection() && !SelectWeaponSlot(1) && !SelectWeaponSlot(2))
	{
		SelectMeleeWeapon();
	}

	UpdateMovementSpeed();
	UpdateStaminaGauge(0.0f);
}

void ATunaSweeperTopDownCharacter::ApplySandbagCoverOutlinePostProcess()
{
	if (!TopDownCamera)
	{
		return;
	}

	UMaterialInterface* OutlineMaterial = LoadObject<UMaterialInterface>(
		nullptr,
		TunaSweeperSandbagCoverOutline::PostProcessMaterialPath);
	if (OutlineMaterial)
	{
		TopDownCamera->AddOrUpdateBlendable(OutlineMaterial, 1.0f);
	}
}

void ATunaSweeperTopDownCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDead)
	{
		return;
	}

	UpdateRoll(DeltaSeconds);
	UpdateMeleeSwing(DeltaSeconds);
	UpdateSprintAndStamina(DeltaSeconds);
	UpdateMovementSpeed();
	UpdateStaminaGauge(DeltaSeconds);
	UpdateWeaponSpreadRecoil(DeltaSeconds);
	UpdateAimingVisuals(DeltaSeconds);
}

void ATunaSweeperTopDownCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
		GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
		GetWorldTimerManager().ClearTimer(RespawnTransitionTimerHandle);
	}

	if (VitalsComponent)
	{
		VitalsComponent->OnVitalsChanged.RemoveDynamic(this, &ATunaSweeperTopDownCharacter::HandleVitalsChanged);
	}

	FinishRoll();
	CancelMeleeSwing();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperTopDownCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	AddDefaultInputMapping();
}

float ATunaSweeperTopDownCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bIsDead || DamageAmount <= 0.0f || !VitalsComponent)
	{
		return 0.0f;
	}

	if (IsDamageInvulnerable())
	{
		return 0.0f;
	}

	LastDamageImpulseDirection = ResolveDamageCameraReactionDirection(DamageCauser);

	FTunaSweeperVitalsDelta DamageDelta;
	DamageDelta.Health = -DamageAmount;
	VitalsComponent->ApplyVitalsDelta(DamageDelta);
	TriggerDamageCameraReaction(DamageAmount, DamageEvent, DamageCauser);
	return DamageAmount;
}

void ATunaSweeperTopDownCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (UInputAction* LoadedMoveAction = MoveAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedMoveAction, ETriggerEvent::Triggered, this, &ATunaSweeperTopDownCharacter::HandleMove);
		EnhancedInputComponent->BindAction(LoadedMoveAction, ETriggerEvent::Completed, this, &ATunaSweeperTopDownCharacter::HandleMoveStopped);
		EnhancedInputComponent->BindAction(LoadedMoveAction, ETriggerEvent::Canceled, this, &ATunaSweeperTopDownCharacter::HandleMoveStopped);
	}

	if (UInputAction* LoadedFireAction = FireAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedFireAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::BeginFire);
		EnhancedInputComponent->BindAction(LoadedFireAction, ETriggerEvent::Completed, this, &ATunaSweeperTopDownCharacter::EndFire);
		EnhancedInputComponent->BindAction(LoadedFireAction, ETriggerEvent::Canceled, this, &ATunaSweeperTopDownCharacter::EndFire);
	}

	if (UInputAction* LoadedAimAction = AimAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedAimAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::BeginAim);
		EnhancedInputComponent->BindAction(LoadedAimAction, ETriggerEvent::Completed, this, &ATunaSweeperTopDownCharacter::EndAim);
		EnhancedInputComponent->BindAction(LoadedAimAction, ETriggerEvent::Canceled, this, &ATunaSweeperTopDownCharacter::EndAim);
	}

	if (UInputAction* LoadedInteractAction = InteractAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedInteractAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleInteract);
	}

	if (UInputAction* LoadedInventoryAction = InventoryAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedInventoryAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleInventory);
	}

	if (UInputAction* LoadedMapAction = MapAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedMapAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleMap);
	}

	if (UInputAction* LoadedReloadAction = ReloadAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedReloadAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleReload);
	}

	if (UInputAction* LoadedAmmoSelectAction = AmmoSelectAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedAmmoSelectAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleAmmoSelect);
	}

	if (UInputAction* LoadedAmmoFocusAction = AmmoFocusAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedAmmoFocusAction, ETriggerEvent::Triggered, this, &ATunaSweeperTopDownCharacter::HandleAmmoFocus);
	}

	if (UInputAction* LoadedCameraModeAction = CameraModeAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedCameraModeAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::HandleCameraMode);
	}

	if (UInputAction* LoadedSprintAction = SprintAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedSprintAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::BeginSprint);
		EnhancedInputComponent->BindAction(LoadedSprintAction, ETriggerEvent::Completed, this, &ATunaSweeperTopDownCharacter::EndSprint);
		EnhancedInputComponent->BindAction(LoadedSprintAction, ETriggerEvent::Canceled, this, &ATunaSweeperTopDownCharacter::EndSprint);
	}

	if (UInputAction* LoadedRollAction = RollAction.LoadSynchronous())
	{
		EnhancedInputComponent->BindAction(LoadedRollAction, ETriggerEvent::Started, this, &ATunaSweeperTopDownCharacter::BeginRoll);
	}
}

void ATunaSweeperTopDownCharacter::SetAimWorldPoint(const FVector& WorldPoint)
{
	AimWorldPoint = WorldPoint;
	bHasAimWorldPoint = true;
	AimIntentActor.Reset();
	AimIntentComponent.Reset();
	AimIntentWorldPoint = WorldPoint;
	bHasAimIntent = false;

	const FVector ToAimPoint = FVector(WorldPoint.X - GetActorLocation().X, WorldPoint.Y - GetActorLocation().Y, 0.0f);
	const FVector NewAimDirection = ToAimPoint.GetSafeNormal();
	if (!NewAimDirection.IsNearlyZero())
	{
		AimDirection = NewAimDirection;
	}
}

void ATunaSweeperTopDownCharacter::SetAimWorldHit(const FVector& WorldPoint, const FHitResult& AimHit)
{
	SetAimWorldPoint(WorldPoint);

	AActor* HitActor = AimHit.GetActor();
	UPrimitiveComponent* HitComponent = AimHit.GetComponent();
	if (!HitActor || !HitComponent)
	{
		return;
	}

	AimIntentActor = HitActor;
	AimIntentComponent = HitComponent;
	AimIntentWorldPoint = AimHit.ImpactPoint;
	bHasAimIntent = true;
}

FVector2D ATunaSweeperTopDownCharacter::GetWeaponRecoilCrosshairScreenOffset() const
{
	return WeaponRecoilOffsetDegrees * FMath::Max(0.0f, WeaponRecoilScreenPixelsPerDegree);
}

float ATunaSweeperTopDownCharacter::GetStaminaPercent() const
{
	return MaxStamina > 0.0f
		? FMath::Clamp(CurrentStamina / MaxStamina, 0.0f, 1.0f)
		: 0.0f;
}

void ATunaSweeperTopDownCharacter::SetHousingModeVisualHidden(bool bShouldHide)
{
	if (bHousingModeVisualHidden == bShouldHide)
	{
		RefreshCharacterVisualVisibility();
		return;
	}

	bHousingModeVisualHidden = bShouldHide;
	RefreshCharacterVisualVisibility();
}

void ATunaSweeperTopDownCharacter::AddDefaultInputMapping() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	UInputMappingContext* LoadedMappingContext = DefaultMappingContext.LoadSynchronous();
	if (InputSubsystem && LoadedMappingContext)
	{
		InputSubsystem->RemoveMappingContext(LoadedMappingContext);
		InputSubsystem->AddMappingContext(LoadedMappingContext, 0);
	}
}

void ATunaSweeperTopDownCharacter::EnsureEquippedWeaponActor()
{
	if (EquippedWeapon || !GetWorld())
	{
		return;
	}

	TSubclassOf<ATunaSweeperWeapon> LoadedWeaponClass = ResolveEquippedWeaponClass();
	if (!LoadedWeaponClass)
	{
		LoadedWeaponClass = ATunaSweeperWeapon::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EquippedWeapon = GetWorld()->SpawnActor<ATunaSweeperWeapon>(LoadedWeaponClass, GetActorTransform(), SpawnParameters);
	if (EquippedWeapon && WeaponAttachPoint)
	{
		EquippedWeapon->AttachToComponent(WeaponAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		if (bMeleeWeaponSelected)
		{
			EquippedWeapon->ConfigureMeleeVisual();
			ApplyEquippedMeleeWeaponVisual();
		}
		else
		{
			if (EquippedWeapon->GetClass() == ATunaSweeperWeapon::StaticClass())
			{
				EquippedWeapon->ConfigureGunVisual();
			}
		}
		EquippedWeapon->SetActorHiddenInGame(bHousingModeVisualHidden);
	}
}

TSubclassOf<ATunaSweeperWeapon> ATunaSweeperTopDownCharacter::ResolveEquippedWeaponClass() const
{
	if (bMeleeWeaponSelected)
	{
		return ATunaSweeperWeapon::StaticClass();
	}

	TSoftClassPtr<ATunaSweeperWeapon> WeaponClassToLoad = DefaultWeaponClass;

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
		TunaGameInstance &&
		SelectedWeaponSlotNumber > 0 &&
		TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition) &&
		WeaponDefinition.CategoryTag == TunaSweeperEquippedWeaponVisual::GunCategoryTag)
	{
		WeaponClassToLoad = TSoftClassPtr<ATunaSweeperWeapon>(TunaSweeperEquippedWeaponVisual::AssaultRifleClassPath);
	}

	if (TSubclassOf<ATunaSweeperWeapon> LoadedWeaponClass = WeaponClassToLoad.LoadSynchronous())
	{
		return LoadedWeaponClass;
	}

	return DefaultWeaponClass.LoadSynchronous();
}

void ATunaSweeperTopDownCharacter::ClearEquippedWeaponActor()
{
	CancelMeleeSwing();

	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	bWeaponAttachedForRoll = false;
	SavedWeaponAttachParent.Reset();
	SavedWeaponAttachSocketName = NAME_None;
	SavedWeaponRelativeTransform = FTransform::Identity;
}

void ATunaSweeperTopDownCharacter::HandleMove(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		CurrentMoveInput = FVector2D::ZeroVector;
		return;
	}

	const FVector2D MoveVector = Value.Get<FVector2D>();
	CurrentMoveInput = MoveVector.GetClampedToMaxSize(1.0f);
	if (bIsRolling)
	{
		return;
	}

	if (!FMath::IsNearlyZero(MoveVector.Y))
	{
		AddMovementInput(FVector::ForwardVector, MoveVector.Y);
	}

	if (!FMath::IsNearlyZero(MoveVector.X))
	{
		AddMovementInput(FVector::RightVector, MoveVector.X);
	}
}

void ATunaSweeperTopDownCharacter::HandleMoveStopped(const FInputActionValue& Value)
{
	(void)Value;
	CurrentMoveInput = FVector2D::ZeroVector;
}

void ATunaSweeperTopDownCharacter::BeginFire(const FInputActionValue& Value)
{
	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController()))
	{
		if (TunaPlayerController->IsHousingPlacementActive())
		{
			TunaPlayerController->TryCommitHousingPlacement();
			return;
		}
	}

	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	bFireHeld = true;
	FireWeapon();

	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(FireTimerHandle, this, &ATunaSweeperTopDownCharacter::FireWeapon, FireInterval, true, FireInterval);
	}
}

void ATunaSweeperTopDownCharacter::EndFire(const FInputActionValue& Value)
{
	bFireHeld = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
}

void ATunaSweeperTopDownCharacter::BeginAim(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	bIsAiming = true;
}

void ATunaSweeperTopDownCharacter::EndAim(const FInputActionValue& Value)
{
	bIsAiming = false;
}

void ATunaSweeperTopDownCharacter::HandleInteract(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController()))
	{
		if (TunaPlayerController->TryHandleHoveredItemInteract())
		{
			return;
		}

		if (TunaPlayerController->IsHousingModeOpen())
		{
			return;
		}

		if (TunaPlayerController->IsInventoryUiOpen())
		{
			return;
		}
	}

	if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
	{
		InteractionSubsystem->TryInteract(this);
	}
}

void ATunaSweeperTopDownCharacter::HandleInventory(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController()))
	{
		if (TunaPlayerController->IsDialogueSequenceActive())
		{
			return;
		}

		TunaPlayerController->ToggleInventoryOnlyPanel();
	}
}

void ATunaSweeperTopDownCharacter::HandleMap(const FInputActionValue& Value)
{
	if (bIsDead)
	{
		return;
	}

	if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController()))
	{
		if (TunaPlayerController->IsDialogueSequenceActive())
		{
			return;
		}

		TunaPlayerController->ToggleMapPanel();
	}
}

void ATunaSweeperTopDownCharacter::HandleReload(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	StartReload();
}

void ATunaSweeperTopDownCharacter::HandleAmmoSelect(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	if (bAmmoSelectionOpen)
	{
		ConfirmAmmoSelection();
	}
	else
	{
		OpenAmmoSelection();
	}
}

void ATunaSweeperTopDownCharacter::HandleAmmoFocus(const FInputActionValue& Value)
{
	if (!bAmmoSelectionOpen || AmmoSelectionItemIds.Num() <= 0)
	{
		return;
	}

	const float AxisValue = Value.Get<float>();
	if (FMath::Abs(AxisValue) <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	MoveAmmoSelectionFocus(AxisValue > 0.0f ? -1 : 1);
}

void ATunaSweeperTopDownCharacter::HandleCameraMode(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	CyclePlayerCameraMode();
}

void ATunaSweeperTopDownCharacter::BeginSprint(const FInputActionValue& Value)
{
	(void)Value;
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	bSprintInputHeld = true;
}

void ATunaSweeperTopDownCharacter::EndSprint(const FInputActionValue& Value)
{
	(void)Value;
	bSprintInputHeld = false;
	bIsSprinting = false;
	bSprintLockedUntilReleased = false;
}

void ATunaSweeperTopDownCharacter::BeginRoll(const FInputActionValue& Value)
{
	(void)Value;
	if (bIsDead || bIsRolling || IsGameplayActionInputLocked())
	{
		return;
	}

	RollDirection = ResolveRollDirection();
	if (RollDirection.IsNearlyZero())
	{
		return;
	}

	const float EffectiveRollStaminaCost = FMath::Max(0.0f, RollStaminaCost);
	CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, FMath::Max(0.0f, MaxStamina));
	if (CurrentStamina < EffectiveRollStaminaCost)
	{
		return;
	}

	bIsRolling = true;
	bIsSprinting = false;
	bSprintInputHeld = false;
	bSprintLockedUntilReleased = false;
	bFireHeld = false;
	bIsAiming = false;
	RollElapsedSeconds = 0.0f;
	CurrentStamina = FMath::Max(0.0f, CurrentStamina - EffectiveRollStaminaCost);
	DefaultSkeletalMeshRelativeRotation = GetMesh() ? GetMesh()->GetRelativeRotation() : DefaultSkeletalMeshRelativeRotation;
	DefaultVisualMeshRelativeRotation = VisualMesh ? VisualMesh->GetRelativeRotation() : DefaultVisualMeshRelativeRotation;

	CancelReload();
	CloseAmmoSelection();
	CancelMeleeSwing();
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}
	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
	ConsumeMovementInputVector();

	SetActorRotation(FRotator(0.0f, RollDirection.Rotation().Yaw, 0.0f));
	SetRollProjectileCollisionPassthrough(true);
	AttachWeaponForRoll();
	UpdateMovementSpeed();
	ApplyTemporaryRollVisualRotation(0.0f);
}

void ATunaSweeperTopDownCharacter::CyclePlayerCameraMode()
{
	switch (CurrentCameraMode)
	{
	case ETunaSweeperPlayerCameraMode::Default:
		CurrentCameraMode = ETunaSweeperPlayerCameraMode::TopDown;
		break;
	case ETunaSweeperPlayerCameraMode::TopDown:
		CurrentCameraMode = ETunaSweeperPlayerCameraMode::LowFront;
		break;
	case ETunaSweeperPlayerCameraMode::LowFront:
	default:
		CurrentCameraMode = ETunaSweeperPlayerCameraMode::Default;
		break;
	}
}

void ATunaSweeperTopDownCharacter::FireWeapon()
{
	if (IsGameplayActionInputLocked())
	{
		CancelActiveGameplayActions();
		return;
	}

	if (bIsDead || !CanUseSelectedWeaponSlot())
	{
		if (bMeleeWeaponSelected)
		{
			StartMeleeAttack();
		}
		return;
	}

	if (bIsReloading)
	{
		CancelReload();
	}

	EnsureEquippedWeaponActor();
	if (!EquippedWeapon)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	FName ProjectileHitEffectId = NAME_None;
	FName WeaponTypeTag = NAME_None;
	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		WeaponTypeTag = WeaponDefinition.WeaponTypeTag;
		const int32 LoadedAmmoItemId = WeaponInstance.LoadedAmmoItemId != INDEX_NONE
			? WeaponInstance.LoadedAmmoItemId
			: WeaponInstance.SelectedAmmoItemId;
		if (LoadedAmmoItemId != INDEX_NONE)
		{
			if (UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>())
			{
				FTunaSweeperItemDefinition AmmoDefinition;
				if (ItemDataSubsystem->TryGetItemDefinition(LoadedAmmoItemId, AmmoDefinition))
				{
					ProjectileHitEffectId = AmmoDefinition.ProjectileHitEffectId;
				}
			}
		}
	}

	const UWorld* World = GetWorld();
	const bool bIsBunkerMap = World &&
		World->GetMapName().EndsWith(TEXT("BunkerMap"));
	if (bIsBunkerMap)
	{
		if (TunaGameInstance->GetWeaponLoadedAmmoCount(SelectedWeaponSlotNumber) <= 0)
		{
			return;
		}
	}
	else if (!TunaGameInstance->TryConsumeLoadedAmmoForWeaponSlot(SelectedWeaponSlotNumber))
	{
		return;
	}

	const float SpreadHalfAngleDegrees = ResolveWeaponSpreadHalfAngleDegrees(WeaponTypeTag);
	EquippedWeapon->FireWithAimIntent(
		AimDirection,
		this,
		ProjectileHitEffectId,
		WeaponTypeTag,
		SpreadHalfAngleDegrees,
		bHasAimIntent ? AimIntentActor.Get() : nullptr,
		bHasAimIntent ? AimIntentComponent.Get() : nullptr,
		AimIntentWorldPoint,
		bHasAimIntent);
	AddWeaponSpreadRecoilShot(WeaponTypeTag);
}

bool ATunaSweeperTopDownCharacter::CanUseSelectedWeaponSlot()
{
	if (SelectedWeaponSlotNumber <= 0)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	return TunaGameInstance && TunaGameInstance->IsEquipmentWeaponSlotOccupied(SelectedWeaponSlotNumber);
}

bool ATunaSweeperTopDownCharacter::CanUseSelectedMeleeWeapon()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	return TunaGameInstance && TunaGameInstance->IsEquipmentMeleeSlotOccupied();
}

bool ATunaSweeperTopDownCharacter::SelectWeaponSlot(int32 SlotNumber)
{
	if (bIsDead || SlotNumber < 1 || SlotNumber > 2)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance || !TunaGameInstance->IsEquipmentWeaponSlotOccupied(SlotNumber))
	{
		return false;
	}

	if (SelectedWeaponSlotNumber != SlotNumber || bMeleeWeaponSelected)
	{
		CancelReload();
		CloseAmmoSelection();
		CancelMeleeSwing();
		ClearEquippedWeaponActor();
		ResetWeaponSpreadRecoil();
	}

	SelectedWeaponSlotNumber = SlotNumber;
	bMeleeWeaponSelected = false;
	TunaGameInstance->SetRuntimeSelectedWeaponSlotNumber(SlotNumber);
	EnsureEquippedWeaponActor();
	return true;
}

bool ATunaSweeperTopDownCharacter::SelectMeleeWeapon()
{
	if (bIsDead || !CanUseSelectedMeleeWeapon())
	{
		return false;
	}

	if (!bMeleeWeaponSelected || SelectedWeaponSlotNumber != 0)
	{
		CancelReload();
		CloseAmmoSelection();
		CancelMeleeSwing();
		ClearEquippedWeaponActor();
		ResetWeaponSpreadRecoil();
	}

	SelectedWeaponSlotNumber = 0;
	bMeleeWeaponSelected = true;
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->SetRuntimeSelectedMeleeWeapon();
	}
	EnsureEquippedWeaponActor();
	return true;
}

bool ATunaSweeperTopDownCharacter::RestoreRuntimeSelectedWeaponSelection()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return false;
	}

	bool bRestoreMeleeWeapon = false;
	int32 RestoreWeaponSlotNumber = 1;
	if (!TunaGameInstance->TryGetRuntimeSelectedWeaponSelection(bRestoreMeleeWeapon, RestoreWeaponSlotNumber))
	{
		return false;
	}

	return bRestoreMeleeWeapon
		? SelectMeleeWeapon()
		: SelectWeaponSlot(RestoreWeaponSlotNumber);
}

void ATunaSweeperTopDownCharacter::StartMeleeAttack()
{
	if (bIsDead || bIsRolling || !bMeleeWeaponSelected || !CanUseSelectedMeleeWeapon())
	{
		return;
	}

	UWorld* World = GetWorld();
	const float CurrentTimeSeconds = World ? World->GetTimeSeconds() : 0.0f;
	if (CurrentTimeSeconds - LastMeleeAttackWorldSeconds < FMath::Max(0.01f, MeleeAttackCooldownSeconds))
	{
		return;
	}

	CancelReload();
	CloseAmmoSelection();
	EnsureEquippedWeaponActor();
	if (!EquippedWeapon)
	{
		return;
	}

	FVector AttackDirection = AimDirection.GetSafeNormal2D();
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = FVector::ForwardVector;
	}

	SetActorRotation(FRotator(0.0f, AttackDirection.Rotation().Yaw, 0.0f));
	SpawnMeleeSwingEffect(AttackDirection);

	LastMeleeAttackWorldSeconds = CurrentTimeSeconds;
	MeleeSwingElapsedSeconds = 0.0f;
	bMeleeSwingActive = true;
	bMeleeJudgementApplied = false;
	ResetEquippedWeaponRelativeTransform();
}

void ATunaSweeperTopDownCharacter::UpdateMeleeSwing(float DeltaSeconds)
{
	if (!bMeleeSwingActive)
	{
		return;
	}

	const float EffectiveDuration = FMath::Max(0.01f, MeleeSwingDurationSeconds);
	MeleeSwingElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);
	const float JudgementTime = FMath::Clamp(MeleeJudgementTimeSeconds, 0.0f, EffectiveDuration);
	if (!bMeleeJudgementApplied && MeleeSwingElapsedSeconds >= JudgementTime)
	{
		ApplyMeleeAttackJudgement();
	}

	const float Alpha = FMath::Clamp(MeleeSwingElapsedSeconds / EffectiveDuration, 0.0f, 1.0f);
	const float SmoothAlpha = Alpha * Alpha * (3.0f - 2.0f * Alpha);
	if (EquippedWeapon)
	{
		if (USceneComponent* WeaponRoot = EquippedWeapon->GetRootComponent())
		{
			const float SideOffset = FMath::Lerp(-24.0f, 24.0f, SmoothAlpha);
			const float LiftOffset = FMath::Sin(Alpha * PI) * 10.0f;
			const float SwingYaw = FMath::Lerp(74.0f, -66.0f, SmoothAlpha);
			const float SwingPitch = FMath::Sin(Alpha * PI) * -18.0f;
			const float SwingRoll = FMath::Lerp(-18.0f, 16.0f, SmoothAlpha);
			WeaponRoot->SetRelativeLocation(FVector(0.0f, SideOffset, LiftOffset));
			WeaponRoot->SetRelativeRotation(FRotator(SwingPitch, SwingYaw, SwingRoll));
		}
	}

	if (MeleeSwingElapsedSeconds >= EffectiveDuration)
	{
		if (!bMeleeJudgementApplied)
		{
			ApplyMeleeAttackJudgement();
		}
		bMeleeSwingActive = false;
		MeleeSwingElapsedSeconds = 0.0f;
		ResetEquippedWeaponRelativeTransform();
	}
}

void ATunaSweeperTopDownCharacter::CancelMeleeSwing()
{
	bMeleeSwingActive = false;
	bMeleeJudgementApplied = false;
	MeleeSwingElapsedSeconds = 0.0f;
	ResetEquippedWeaponRelativeTransform();
}

void ATunaSweeperTopDownCharacter::ResetEquippedWeaponRelativeTransform()
{
	if (EquippedWeapon)
	{
		if (USceneComponent* WeaponRoot = EquippedWeapon->GetRootComponent())
		{
			WeaponRoot->SetRelativeLocation(FVector::ZeroVector);
			WeaponRoot->SetRelativeRotation(FRotator::ZeroRotator);
			WeaponRoot->SetRelativeScale3D(FVector::OneVector);
		}
	}
}

void ATunaSweeperTopDownCharacter::ApplyEquippedMeleeWeaponVisual()
{
	if (!EquippedWeapon || !bMeleeWeaponSelected)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	FTunaSweeperItemInstance MeleeInstance;
	FTunaSweeperItemDefinition MeleeDefinition;
	if (!TunaGameInstance->TryGetEquipmentMeleeSlotItem(MeleeInstance, MeleeDefinition) ||
		MeleeDefinition.Id != TunaSweeperEquippedWeaponVisual::BaseballBatItemId)
	{
		return;
	}

	UStaticMesh* BaseballBatMesh = Cast<UStaticMesh>(TunaSweeperEquippedWeaponVisual::BaseballBatMeshPath.TryLoad());
	UMaterialInterface* BaseballBatMaterial =
		Cast<UMaterialInterface>(TunaSweeperEquippedWeaponVisual::BaseballBatMaterialPath.TryLoad());
	EquippedWeapon->SetWeaponMeshOverride(
		BaseballBatMesh,
		BaseballBatMaterial,
		FVector(26.0f, 0.0f, 0.0f),
		FRotator::ZeroRotator,
		FVector(0.54f, 1.0f, 1.0f));
}

void ATunaSweeperTopDownCharacter::ApplyMeleeAttackJudgement()
{
	bMeleeJudgementApplied = true;

	UWorld* World = GetWorld();
	if (!World || MeleeAttackDamage <= 0.0f || MeleeAttackRange <= 0.0f)
	{
		return;
	}

	FVector AttackDirection = AimDirection.GetSafeNormal2D();
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	if (AttackDirection.IsNearlyZero())
	{
		AttackDirection = FVector::ForwardVector;
	}

	const FVector Origin = GetActorLocation();
	const float RangeSquared = FMath::Square(MeleeAttackRange);
	const float CosHalfAngle = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(MeleeAttackHalfAngleDegrees, 0.0f, 180.0f)));

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperPlayerMeleeCone), false, this);
	if (EquippedWeapon)
	{
		QueryParams.AddIgnoredActor(EquippedWeapon);
	}

	TArray<FOverlapResult> Overlaps;
	if (!World->OverlapMultiByObjectType(
		Overlaps,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(MeleeAttackRange),
		QueryParams))
	{
		return;
	}

	TSet<AActor*> HitActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* TargetActor = Overlap.GetActor();
		if (!IsValid(TargetActor) || TargetActor == this || HitActors.Contains(TargetActor))
		{
			continue;
		}

		FVector ToTarget = TargetActor->GetActorLocation() - Origin;
		ToTarget.Z = 0.0f;
		const float DistanceSquared = ToTarget.SizeSquared();
		if (DistanceSquared > RangeSquared)
		{
			continue;
		}

		FVector DirectionToTarget = ToTarget.GetSafeNormal();
		if (DirectionToTarget.IsNearlyZero())
		{
			DirectionToTarget = AttackDirection;
		}

		if (FVector::DotProduct(AttackDirection, DirectionToTarget) < CosHalfAngle)
		{
			continue;
		}

		HitActors.Add(TargetActor);
		const float AppliedDamage = UGameplayStatics::ApplyDamage(
			TargetActor,
			MeleeAttackDamage,
			GetController(),
			this,
			UDamageType::StaticClass());
		if (AppliedDamage > 0.0f)
		{
			const FVector HitLocation = TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, MeleeImpactHeight);
			SpawnMeleeImpactBurst(HitLocation, -AttackDirection);
		}
	}
}

void ATunaSweeperTopDownCharacter::SpawnMeleeSwingEffect(const FVector& AttackDirection)
{
	UWorld* World = GetWorld();
	const FVector SafeAttackDirection = AttackDirection.GetSafeNormal2D();
	if (!World || SafeAttackDirection.IsNearlyZero())
	{
		return;
	}

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
		FRotator(0.0f, SafeAttackDirection.Rotation().Yaw, 0.0f),
		SpawnParameters);
}

void ATunaSweeperTopDownCharacter::SpawnMeleeImpactBurst(
	const FVector& HitLocation,
	const FVector& BurstDirection)
{
	UWorld* World = GetWorld();
	const FVector SafeBurstDirection = BurstDirection.GetSafeNormal2D();
	if (!World || SafeBurstDirection.IsNearlyZero())
	{
		return;
	}

	TSubclassOf<ATunaSweeperMeleeImpactBurstActor> LoadedBurstClass = MeleeImpactBurstActorClass.LoadSynchronous();
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
		FRotator(0.0f, SafeBurstDirection.Rotation().Yaw, 0.0f),
		SpawnParameters);
}

float ATunaSweeperTopDownCharacter::GetReloadProgress() const
{
	if (!bIsReloading || ReloadDurationSeconds <= 0.0f)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : ReloadStartWorldSeconds;
	return FMath::Clamp((CurrentTime - ReloadStartWorldSeconds) / ReloadDurationSeconds, 0.0f, 1.0f);
}

void ATunaSweeperTopDownCharacter::GetAmmoSelectionItemIds(TArray<int32>& OutAmmoItemIds) const
{
	OutAmmoItemIds = AmmoSelectionItemIds;
}

void ATunaSweeperTopDownCharacter::StartReload()
{
	if (bIsReloading || !CanUseSelectedWeaponSlot())
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	const int32 MagazineCapacity = TunaGameInstance->GetWeaponMagazineCapacity(SelectedWeaponSlotNumber);
	const int32 LoadedAmmoCount = TunaGameInstance->GetWeaponLoadedAmmoCount(SelectedWeaponSlotNumber);
	if (MagazineCapacity <= 0 || LoadedAmmoCount >= MagazineCapacity)
	{
		return;
	}

	int32 ReloadAmmoItemId = TunaGameInstance->GetWeaponSelectedAmmoItemId(SelectedWeaponSlotNumber);
	if (LoadedAmmoCount > 0)
	{
		FTunaSweeperItemInstance WeaponInstance;
		FTunaSweeperItemDefinition WeaponDefinition;
		if (TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition) &&
			WeaponInstance.LoadedAmmoItemId != INDEX_NONE)
		{
			ReloadAmmoItemId = WeaponInstance.LoadedAmmoItemId;
		}
	}

	if (ReloadAmmoItemId == INDEX_NONE || TunaGameInstance->GetWeaponInventoryAmmoCount(SelectedWeaponSlotNumber) <= 0)
	{
		return;
	}

	PendingReloadAmmoItemId = ReloadAmmoItemId;
	ReloadDurationSeconds = FMath::Max(0.01f, TunaGameInstance->GetWeaponReloadSeconds(SelectedWeaponSlotNumber));
	ReloadStartWorldSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bIsReloading = true;
	CloseAmmoSelection();

	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&ATunaSweeperTopDownCharacter::CompleteReload,
			ReloadDurationSeconds,
			false);
	}
}

void ATunaSweeperTopDownCharacter::CompleteReload()
{
	const int32 ReloadSlotNumber = SelectedWeaponSlotNumber;
	const int32 ReloadAmmoItemId = PendingReloadAmmoItemId;
	CancelReload();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		int32 LoadedAmmoCount = 0;
		TunaGameInstance->TryReloadWeaponSlot(ReloadSlotNumber, ReloadAmmoItemId, LoadedAmmoCount);
	}
}

void ATunaSweeperTopDownCharacter::CancelReload()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	}

	bIsReloading = false;
	PendingReloadAmmoItemId = INDEX_NONE;
	ReloadStartWorldSeconds = 0.0f;
	ReloadDurationSeconds = 0.0f;
}

void ATunaSweeperTopDownCharacter::OpenAmmoSelection()
{
	if (!CanUseSelectedWeaponSlot())
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	TunaGameInstance->GetCompatibleAmmoItemIdsForWeaponSlot(
		SelectedWeaponSlotNumber,
		AmmoSelectionItemIds,
		false);
	if (AmmoSelectionItemIds.Num() <= 0)
	{
		CloseAmmoSelection();
		return;
	}

	const int32 CurrentAmmoItemId = TunaGameInstance->GetWeaponSelectedAmmoItemId(SelectedWeaponSlotNumber);
	AmmoSelectionFocusIndex = AmmoSelectionItemIds.IndexOfByKey(CurrentAmmoItemId);
	if (!AmmoSelectionItemIds.IsValidIndex(AmmoSelectionFocusIndex))
	{
		AmmoSelectionFocusIndex = 0;
	}

	bAmmoSelectionOpen = true;
}

void ATunaSweeperTopDownCharacter::ConfirmAmmoSelection()
{
	if (!bAmmoSelectionOpen || !AmmoSelectionItemIds.IsValidIndex(AmmoSelectionFocusIndex))
	{
		CloseAmmoSelection();
		return;
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		const bool bWasAmmoUnspecified = TunaGameInstance->GetWeaponSelectedAmmoItemId(SelectedWeaponSlotNumber) == INDEX_NONE;
		const bool bAmmoSelected = TunaGameInstance->SetSelectedAmmoItemForWeaponSlot(
			SelectedWeaponSlotNumber,
			AmmoSelectionItemIds[AmmoSelectionFocusIndex]);
		const bool bShouldAutoReload =
			bWasAmmoUnspecified &&
			bAmmoSelected &&
			TunaGameInstance->GetWeaponInventoryAmmoCount(SelectedWeaponSlotNumber) > 0;

		CloseAmmoSelection();
		if (bShouldAutoReload)
		{
			StartReload();
		}
		return;
	}

	CloseAmmoSelection();
}

void ATunaSweeperTopDownCharacter::CloseAmmoSelection()
{
	bAmmoSelectionOpen = false;
	AmmoSelectionItemIds.Reset();
	AmmoSelectionFocusIndex = INDEX_NONE;
}

void ATunaSweeperTopDownCharacter::MoveAmmoSelectionFocus(int32 FocusDelta)
{
	if (!bAmmoSelectionOpen || AmmoSelectionItemIds.Num() <= 0 || FocusDelta == 0)
	{
		return;
	}

	if (!AmmoSelectionItemIds.IsValidIndex(AmmoSelectionFocusIndex))
	{
		AmmoSelectionFocusIndex = 0;
		return;
	}

	const int32 OptionCount = AmmoSelectionItemIds.Num();
	AmmoSelectionFocusIndex = (AmmoSelectionFocusIndex + FocusDelta) % OptionCount;
	if (AmmoSelectionFocusIndex < 0)
	{
		AmmoSelectionFocusIndex += OptionCount;
	}
}

void ATunaSweeperTopDownCharacter::RefreshSelectedWeaponAfterInventoryChanged()
{
	if (SelectedWeaponSlotNumber > 0 && CanUseSelectedWeaponSlot())
	{
		return;
	}
	if (bMeleeWeaponSelected && CanUseSelectedMeleeWeapon())
	{
		return;
	}

	CancelReload();
	CloseAmmoSelection();
	CancelMeleeSwing();
	SelectedWeaponSlotNumber = 0;
	bMeleeWeaponSelected = false;
	ClearEquippedWeaponActor();
	ResetWeaponSpreadRecoil();
}

void ATunaSweeperTopDownCharacter::RefreshCharacterVisualVisibility()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (CharacterMesh)
	{
		CharacterMesh->SetHiddenInGame(bHousingModeVisualHidden);
		CharacterMesh->SetVisibility(!bHousingModeVisualHidden, true);
	}

	const bool bHasCharacterMesh = CharacterMesh && CharacterMesh->GetSkeletalMeshAsset();
	if (VisualMesh)
	{
		VisualMesh->SetHiddenInGame(bHousingModeVisualHidden || bHasCharacterMesh);
		VisualMesh->SetVisibility(!bHousingModeVisualHidden && !bHasCharacterMesh, true);
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->SetActorHiddenInGame(bHousingModeVisualHidden);
	}

	if (StaminaGaugeWidgetComponent)
	{
		StaminaGaugeWidgetComponent->SetHiddenInGame(bHousingModeVisualHidden);
		if (bHousingModeVisualHidden)
		{
			StaminaGaugeWidgetComponent->SetVisibility(false);
		}
	}
}

bool ATunaSweeperTopDownCharacter::IsGameplayActionInputLocked() const
{
	const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController());
	return TunaPlayerController &&
		(TunaPlayerController->IsInventoryUiOpen() ||
			TunaPlayerController->IsDialogueSequenceActive() ||
			TunaPlayerController->IsHousingModeOpen());
}

void ATunaSweeperTopDownCharacter::CancelActiveGameplayActions()
{
	FinishRoll();
	bFireHeld = false;
	bIsAiming = false;
	bSprintInputHeld = false;
	bIsSprinting = false;
	bSprintLockedUntilReleased = false;
	CurrentMoveInput = FVector2D::ZeroVector;
	CancelReload();
	CloseAmmoSelection();
	CancelMeleeSwing();

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
}

void ATunaSweeperTopDownCharacter::HandleVitalsChanged(const FTunaSweeperVitalsState& VitalsState)
{
	if (!bIsDead && VitalsState.Health <= 0.0f)
	{
		HandleDeath();
	}
}

void ATunaSweeperTopDownCharacter::HandleDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	FinishRoll();
	bFireHeld = false;
	bIsAiming = false;
	bSprintInputHeld = false;
	bIsSprinting = false;
	bSprintLockedUntilReleased = false;
	CurrentMoveInput = FVector2D::ZeroVector;
	CancelReload();
	CloseAmmoSelection();
	ClearEquippedWeaponActor();
	GetWorldTimerManager().ClearTimer(FireTimerHandle);
	if (StaminaGaugeWidgetComponent)
	{
		StaminaGaugeWidgetComponent->SetVisibility(false);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		const FName SourceLevelName = GetWorld() ? FName(*GetWorld()->GetMapName()) : NAME_None;
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyBunkerRescueReturn(SourceLevelName, RespawnTargetLevelName);
		}

		TunaGameInstance->ClearInventoryAndSave();
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
		MovementComponent->DisableMovement();
	}

	ApplyDeathRagdoll();

	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->SetIgnoreMoveInput(true);
		PlayerController->SetIgnoreLookInput(true);
		DisableInput(PlayerController);
	}

	GetWorldTimerManager().SetTimer(
		RespawnTransitionTimerHandle,
		this,
		&ATunaSweeperTopDownCharacter::StartRespawnTransition,
		FMath::Max(0.0f, RespawnDelaySeconds),
		false);
}

void ATunaSweeperTopDownCharacter::ApplyDeathRagdoll()
{
	if (!bEnableDeathRagdoll)
	{
		return;
	}

	const FVector RagdollImpulse = ResolveDeathRagdollImpulse();

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Capsule->SetGenerateOverlapEvents(false);
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh();
		CharacterMesh && CharacterMesh->GetSkeletalMeshAsset())
	{
		CharacterMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		if (!DeathRagdollCollisionProfileName.IsNone())
		{
			CharacterMesh->SetCollisionProfileName(DeathRagdollCollisionProfileName);
		}
		CharacterMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CharacterMesh->SetEnableGravity(true);
		CharacterMesh->SetAllBodiesSimulatePhysics(true);
		CharacterMesh->SetSimulatePhysics(true);
		CharacterMesh->WakeAllRigidBodies();
		CharacterMesh->bBlendPhysics = true;
		if (!RagdollImpulse.IsNearlyZero())
		{
			CharacterMesh->AddImpulse(RagdollImpulse);
		}
		return;
	}

	if (VisualMesh)
	{
		VisualMesh->SetHiddenInGame(false);
		VisualMesh->SetVisibility(true, true);
		VisualMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		if (!DeathRagdollCollisionProfileName.IsNone())
		{
			VisualMesh->SetCollisionProfileName(DeathRagdollCollisionProfileName);
		}
		VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		VisualMesh->SetEnableGravity(true);
		VisualMesh->SetSimulatePhysics(true);
		VisualMesh->WakeRigidBody();
		if (!RagdollImpulse.IsNearlyZero())
		{
			VisualMesh->AddImpulse(RagdollImpulse);
		}
	}
}

FVector ATunaSweeperTopDownCharacter::ResolveDeathRagdollImpulse() const
{
	FVector HorizontalDirection = LastDamageImpulseDirection;
	HorizontalDirection.Z = 0.0f;
	if (!HorizontalDirection.Normalize())
	{
		HorizontalDirection = -GetActorForwardVector();
		HorizontalDirection.Z = 0.0f;
		HorizontalDirection.Normalize();
	}

	if (HorizontalDirection.IsNearlyZero())
	{
		HorizontalDirection = -FVector::ForwardVector;
	}

	return HorizontalDirection * FMath::Max(0.0f, DeathRagdollHorizontalImpulse) +
		FVector::UpVector * FMath::Max(0.0f, DeathRagdollUpwardImpulse);
}

void ATunaSweeperTopDownCharacter::StartRespawnTransition()
{
	if (RespawnTargetLevelName.IsNone())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GameInstance);
		if (UTunaSweeperLevelTransitionSubsystem* TransitionSubsystem = GameInstance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>())
		{
			if (TransitionSubsystem->StartTransition(
				this,
				RespawnTargetLevelName,
				RespawnMediaSource,
				RespawnTransitionWidgetClass,
				RespawnFadeToBlackDuration,
				RespawnFadeFromBlackDuration,
				TunaGameInstance
					? TunaGameInstance->ResolveLocalizedText(
						FName(TEXT("ui.notification.rescue_cart")),
						FText::FromString(TEXT("\uAD6C\uAE09 \uCE74\uD2B8 \uD6C4\uC1A1 \uC911")))
					: FText::FromString(TEXT("\uAD6C\uAE09 \uCE74\uD2B8 \uD6C4\uC1A1 \uC911"))))
			{
				return;
			}
		}
	}

	UGameplayStatics::OpenLevel(this, RespawnTargetLevelName);
}

float ATunaSweeperTopDownCharacter::ResolveCameraCursorLeadRatio() const
{
	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return 0.0f;
	}

	int32 ViewportSizeX = 0;
	int32 ViewportSizeY = 0;
	PlayerController->GetViewportSize(ViewportSizeX, ViewportSizeY);
	if (ViewportSizeX <= 0 || ViewportSizeY <= 0)
	{
		return 0.0f;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PlayerController->GetMousePosition(MouseX, MouseY))
	{
		return 0.0f;
	}

	const FVector2D ClampedMousePosition(
		FMath::Clamp(MouseX, 0.0f, static_cast<float>(ViewportSizeX)),
		FMath::Clamp(MouseY, 0.0f, static_cast<float>(ViewportSizeY)));
	FVector2D CharacterScreenPosition(
		static_cast<float>(ViewportSizeX) * 0.5f,
		static_cast<float>(ViewportSizeY) * 0.5f);
	PlayerController->ProjectWorldLocationToScreen(GetActorLocation(), CharacterScreenPosition, true);

	const FVector2D CursorDelta = ClampedMousePosition - CharacterScreenPosition;
	const float CursorDistance = CursorDelta.Size();
	if (CursorDistance <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	const FVector2D CursorDirection = CursorDelta / CursorDistance;
	float DistanceToViewportEdge = TNumericLimits<float>::Max();
	auto ConsiderEdgeDistance = [&DistanceToViewportEdge](float CandidateDistance)
	{
		if (CandidateDistance > KINDA_SMALL_NUMBER)
		{
			DistanceToViewportEdge = FMath::Min(DistanceToViewportEdge, CandidateDistance);
		}
	};

	if (CursorDirection.X > KINDA_SMALL_NUMBER)
	{
		ConsiderEdgeDistance((static_cast<float>(ViewportSizeX) - CharacterScreenPosition.X) / CursorDirection.X);
	}
	else if (CursorDirection.X < -KINDA_SMALL_NUMBER)
	{
		ConsiderEdgeDistance((0.0f - CharacterScreenPosition.X) / CursorDirection.X);
	}

	if (CursorDirection.Y > KINDA_SMALL_NUMBER)
	{
		ConsiderEdgeDistance((static_cast<float>(ViewportSizeY) - CharacterScreenPosition.Y) / CursorDirection.Y);
	}
	else if (CursorDirection.Y < -KINDA_SMALL_NUMBER)
	{
		ConsiderEdgeDistance((0.0f - CharacterScreenPosition.Y) / CursorDirection.Y);
	}

	if (DistanceToViewportEdge == TNumericLimits<float>::Max() || DistanceToViewportEdge <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	return FMath::Clamp(CursorDistance / DistanceToViewportEdge, 0.0f, 1.0f);
}

void ATunaSweeperTopDownCharacter::UpdateAimingVisuals(float DeltaSeconds)
{
	float HitReactionRollDegrees = 0.0f;
	float HitReactionFOVDegrees = 0.0f;
	const FVector HitReactionOffset = UpdateDamageCameraReaction(DeltaSeconds, HitReactionRollDegrees, HitReactionFOVDegrees);
	const FTunaSweeperPlayerCameraModeSettings CameraModeSettings = ResolveCurrentCameraModeSettings();

	if (!bIsRolling && !AimDirection.IsNearlyZero())
	{
		const FRotator CurrentRotation = GetActorRotation();
		const FRotator TargetRotation(0.0f, AimDirection.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 18.0f));
	}

	if (TopDownCamera)
	{
		const float TargetFOV = CameraModeSettings.DefaultFOV;
		CurrentCameraBaseFOV = FMath::FInterpTo(CurrentCameraBaseFOV, TargetFOV, DeltaSeconds, CameraInterpSpeed);
		TopDownCamera->SetFieldOfView(CurrentCameraBaseFOV + HitReactionFOVDegrees);

		const FRotator TargetCameraRotation = DefaultCameraRelativeRotation + FRotator(0.0f, 0.0f, HitReactionRollDegrees);
		TopDownCamera->SetRelativeRotation(TargetCameraRotation);
	}

	if (CameraBoom)
	{
		CurrentCameraArmLength = FMath::FInterpTo(
			CurrentCameraArmLength,
			CameraModeSettings.TargetArmLength,
			DeltaSeconds,
			CameraInterpSpeed);
		CameraBoom->TargetArmLength = CurrentCameraArmLength;

		CurrentCameraBoomRotation = FMath::RInterpTo(
			CurrentCameraBoomRotation,
			CameraModeSettings.BoomRotation,
			DeltaSeconds,
			CameraInterpSpeed);
		CameraBoom->SetRelativeRotation(CurrentCameraBoomRotation);

		CurrentCameraModeOffset = FMath::VInterpTo(
			CurrentCameraModeOffset,
			CameraModeSettings.TargetOffset,
			DeltaSeconds,
			CameraInterpSpeed);

		FVector AimTargetOffset = FVector::ZeroVector;
		if (bHasAimWorldPoint)
		{
			const FVector AimLeadDirection = AimDirection.GetSafeNormal2D();
			if (!AimLeadDirection.IsNearlyZero())
			{
				AimTargetOffset =
					AimLeadDirection *
					FMath::Max(0.0f, AimCameraLeadDistance) *
					ResolveCameraCursorLeadRatio();
			}
		}
		CurrentCameraAimOffset = FMath::VInterpTo(CurrentCameraAimOffset, AimTargetOffset, DeltaSeconds, CameraInterpSpeed);
		CameraBoom->TargetOffset = CurrentCameraModeOffset + CurrentCameraAimOffset + HitReactionOffset;
	}
}

FTunaSweeperPlayerCameraModeSettings ATunaSweeperTopDownCharacter::ResolveCurrentCameraModeSettings() const
{
	switch (CurrentCameraMode)
	{
	case ETunaSweeperPlayerCameraMode::TopDown:
		return TopDownCameraModeSettings;
	case ETunaSweeperPlayerCameraMode::LowFront:
		return LowFrontCameraModeSettings;
	case ETunaSweeperPlayerCameraMode::Default:
	default:
		break;
	}

	FTunaSweeperPlayerCameraModeSettings DefaultSettings;
	DefaultSettings.TargetArmLength = DefaultCameraArmLength;
	DefaultSettings.BoomRotation = DefaultCameraBoomRotation;
	DefaultSettings.TargetOffset = DefaultCameraTargetOffset;
	DefaultSettings.DefaultFOV = DefaultCameraFOV;
	DefaultSettings.AimFOV = AimCameraFOV;
	return DefaultSettings;
}

void ATunaSweeperTopDownCharacter::TriggerDamageCameraReaction(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AActor* DamageCauser)
{
	if (DamageAmount <= 0.0f || (!CameraBoom && !TopDownCamera))
	{
		return;
	}

	ActiveCameraHitReaction = ResolveDamageCameraReactionSettings(DamageEvent, DamageCauser);
	if (const ATunaSweeperProjectile* ProjectileCauser = Cast<ATunaSweeperProjectile>(DamageCauser))
	{
		const float ProjectileReactionScale = ProjectileCauser->GetCameraHitReactionScale();
		ActiveCameraHitReaction.LocationAmplitude *= ProjectileReactionScale;
		ActiveCameraHitReaction.RollAmplitudeDegrees *= ProjectileReactionScale;
		ActiveCameraHitReaction.FOVAmplitudeDegrees *= ProjectileReactionScale;
	}
	if (ActiveCameraHitReaction.Duration <= 0.0f ||
		(ActiveCameraHitReaction.LocationAmplitude <= 0.0f && ActiveCameraHitReaction.RollAmplitudeDegrees <= 0.0f))
	{
		return;
	}

	const float DamageReference = FMath::Max(0.01f, ActiveCameraHitReaction.DamageScaleReference);
	const float MinDamageScale = FMath::Min(ActiveCameraHitReaction.MinDamageScale, ActiveCameraHitReaction.MaxDamageScale);
	const float MaxDamageScale = FMath::Max(ActiveCameraHitReaction.MinDamageScale, ActiveCameraHitReaction.MaxDamageScale);
	CameraHitReactionScale = FMath::Clamp(
		DamageAmount / DamageReference,
		MinDamageScale,
		MaxDamageScale);
	CameraHitReactionDirection = ResolveDamageCameraReactionDirection(DamageCauser);
	CameraHitReactionElapsed = 0.0f;
	CameraHitReactionPhase = 0.0f;
	bCameraHitReactionActive = true;
}

ETunaSweeperHitReactionType ATunaSweeperTopDownCharacter::ResolveDamageCameraReactionType(
	FDamageEvent const& DamageEvent,
	AActor* DamageCauser) const
{
	(void)DamageEvent;

	if (Cast<ATunaSweeperProjectile>(DamageCauser))
	{
		return ETunaSweeperHitReactionType::Projectile;
	}

	return ETunaSweeperHitReactionType::Default;
}

FTunaSweeperCameraHitReactionSettings ATunaSweeperTopDownCharacter::ResolveDamageCameraReactionSettings(
	FDamageEvent const& DamageEvent,
	AActor* DamageCauser) const
{
	const ETunaSweeperHitReactionType ReactionType = ResolveDamageCameraReactionType(DamageEvent, DamageCauser);
	if (const FTunaSweeperCameraHitReactionSettings* OverrideSettings = CameraHitReactionOverrides.Find(ReactionType))
	{
		return *OverrideSettings;
	}

	return DefaultCameraHitReaction;
}

FVector ATunaSweeperTopDownCharacter::ResolveDamageCameraReactionDirection(AActor* DamageCauser) const
{
	FVector ReactionDirection = DamageCauser
		? GetActorLocation() - DamageCauser->GetActorLocation()
		: -GetActorForwardVector();
	ReactionDirection.Z = 0.0f;

	if (!ReactionDirection.Normalize())
	{
		ReactionDirection = -GetActorForwardVector();
		ReactionDirection.Z = 0.0f;
		ReactionDirection.Normalize();
	}

	return ReactionDirection.IsNearlyZero() ? FVector::ForwardVector : ReactionDirection;
}

FVector ATunaSweeperTopDownCharacter::UpdateDamageCameraReaction(
	float DeltaSeconds,
	float& OutRollDegrees,
	float& OutFOVDegrees)
{
	OutRollDegrees = 0.0f;
	OutFOVDegrees = 0.0f;
	if (!bCameraHitReactionActive)
	{
		return FVector::ZeroVector;
	}

	const float Duration = FMath::Max(0.01f, ActiveCameraHitReaction.Duration);
	CameraHitReactionElapsed += FMath::Max(0.0f, DeltaSeconds);

	const float NormalizedTime = FMath::Clamp(CameraHitReactionElapsed / Duration, 0.0f, 1.0f);
	if (NormalizedTime >= 1.0f)
	{
		bCameraHitReactionActive = false;
		return FVector::ZeroVector;
	}

	const float Decay = FMath::Square(1.0f - NormalizedTime);
	const float BaseRadians = (CameraHitReactionElapsed * ActiveCameraHitReaction.Frequency * 2.0f * PI) + CameraHitReactionPhase;
	const float ForwardOscillation = FMath::Sin(BaseRadians);
	const float SideOscillation = FMath::Cos(BaseRadians * 1.37f);
	const FVector PlanarDirection = CameraHitReactionDirection.GetSafeNormal2D();
	FVector SideDirection = FVector::CrossProduct(FVector::UpVector, PlanarDirection).GetSafeNormal();
	if (SideDirection.IsNearlyZero())
	{
		SideDirection = FVector::RightVector;
	}

	const float ScaledDecay = CameraHitReactionScale * Decay;
	OutRollDegrees = ActiveCameraHitReaction.RollAmplitudeDegrees * ScaledDecay * ForwardOscillation;
	OutFOVDegrees = ActiveCameraHitReaction.FOVAmplitudeDegrees * ScaledDecay;
	return (PlanarDirection * ForwardOscillation + SideDirection * 0.45f * SideOscillation) *
		ActiveCameraHitReaction.LocationAmplitude *
		ScaledDecay;
}

void ATunaSweeperTopDownCharacter::UpdateRoll(float DeltaSeconds)
{
	if (!bIsRolling)
	{
		return;
	}

	const float EffectiveRollDuration = FMath::Max(0.01f, RollDurationSeconds);
	RollElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);

	AddMovementInput(RollDirection, 1.0f);
	ApplyTemporaryRollVisualRotation(FMath::Clamp(RollElapsedSeconds / EffectiveRollDuration, 0.0f, 1.0f));

	if (RollElapsedSeconds >= EffectiveRollDuration)
	{
		FinishRoll();
	}
}

void ATunaSweeperTopDownCharacter::UpdateSprintAndStamina(float DeltaSeconds)
{
	const float ClampedDeltaSeconds = FMath::Max(0.0f, DeltaSeconds);
	const float EffectiveMaxStamina = FMath::Max(0.0f, MaxStamina);
	if (EffectiveMaxStamina <= 0.0f)
	{
		CurrentStamina = 0.0f;
		bIsSprinting = false;
		return;
	}

	CurrentStamina = FMath::Clamp(CurrentStamina, 0.0f, EffectiveMaxStamina);
	if (bIsRolling)
	{
		bIsSprinting = false;
		return;
	}

	const bool bCanSprint =
		bSprintInputHeld &&
		!bSprintLockedUntilReleased &&
		HasActiveMoveInput() &&
		!IsGameplayActionInputLocked();
	bIsSprinting = bCanSprint && CurrentStamina > 0.0f;

	if (bIsSprinting)
	{
		CurrentStamina = FMath::Max(0.0f, CurrentStamina - FMath::Max(0.0f, SprintStaminaDrainPerSecond) * ClampedDeltaSeconds);
		if (CurrentStamina <= KINDA_SMALL_NUMBER)
		{
			CurrentStamina = 0.0f;
			bIsSprinting = false;
			bSprintLockedUntilReleased = true;
		}
	}
	else
	{
		CurrentStamina = FMath::Min(EffectiveMaxStamina, CurrentStamina + FMath::Max(0.0f, StaminaRegenPerSecond) * ClampedDeltaSeconds);
	}
}

void ATunaSweeperTopDownCharacter::UpdateMovementSpeed()
{
	UCharacterMovementComponent* MovementComponent = GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const float CarryWeightSpeedMultiplier = TunaGameInstance
		? TunaGameInstance->GetCarryWeightMovementSpeedMultiplier()
		: 1.0f;
	const float ActionSpeedMultiplier = bIsRolling
		? FMath::Max(0.0f, RollDistance) / FMath::Max(0.01f, RollDurationSeconds) / FMath::Max(1.0f, BaseWalkSpeed)
		: (bIsSprinting ? FMath::Max(1.0f, SprintSpeedMultiplier) : 1.0f);

	MovementComponent->MaxWalkSpeed =
		BaseWalkSpeed *
		FMath::Clamp(CarryWeightSpeedMultiplier, 0.0f, 1.0f) *
		ActionSpeedMultiplier;
}

void ATunaSweeperTopDownCharacter::UpdateStaminaGauge(float DeltaSeconds)
{
	if (!StaminaGaugeWidgetComponent)
	{
		return;
	}

	if (bHousingModeVisualHidden)
	{
		StaminaGaugeWidgetComponent->SetHiddenInGame(true);
		StaminaGaugeWidgetComponent->SetVisibility(false);
		return;
	}

	StaminaGaugeWidgetComponent->SetHiddenInGame(false);
	const float TargetOpacity = GetStaminaPercent() < 0.999f ? 1.0f : 0.0f;
	if (DeltaSeconds <= 0.0f)
	{
		StaminaGaugeOpacity = TargetOpacity;
	}
	else
	{
		StaminaGaugeOpacity = FMath::FInterpTo(
			StaminaGaugeOpacity,
			TargetOpacity,
			DeltaSeconds,
			FMath::Max(0.0f, StaminaGaugeFadeInterpSpeed));
	}

	if (FMath::IsNearlyEqual(StaminaGaugeOpacity, TargetOpacity, 0.01f))
	{
		StaminaGaugeOpacity = TargetOpacity;
	}

	const bool bVisible = StaminaGaugeOpacity > 0.01f;
	StaminaGaugeWidgetComponent->SetVisibility(bVisible);
	if (!bVisible)
	{
		return;
	}

	StaminaGaugeWidgetComponent->InitWidget();
	if (UTunaSweeperStaminaGaugeWidget* StaminaGaugeWidget = Cast<UTunaSweeperStaminaGaugeWidget>(StaminaGaugeWidgetComponent->GetUserWidgetObject()))
	{
		StaminaGaugeWidget->SetStaminaGauge(GetStaminaPercent(), StaminaGaugeOpacity);
	}
}

void ATunaSweeperTopDownCharacter::UpdateWeaponSpreadRecoil(float DeltaSeconds)
{
	FName SelectedWeaponTypeTag = NAME_None;
	if (!TryGetSelectedWeaponTypeTag(SelectedWeaponTypeTag))
	{
		ResetWeaponSpreadRecoil();
		return;
	}

	if (WeaponRecoilTypeTag != SelectedWeaponTypeTag)
	{
		WeaponRecoilTypeTag = SelectedWeaponTypeTag;
		WeaponRecoilOffsetDegrees = FVector2D::ZeroVector;
		return;
	}

	const float CurrentMagnitude = WeaponRecoilOffsetDegrees.Size();
	if (CurrentMagnitude <= KINDA_SMALL_NUMBER)
	{
		WeaponRecoilOffsetDegrees = FVector2D::ZeroVector;
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperWeaponSpreadRecoilDefinition RecoilDefinition;
	if (!TunaGameInstance || !TunaGameInstance->TryGetWeaponSpreadRecoilDefinition(SelectedWeaponTypeTag, RecoilDefinition))
	{
		ResetWeaponSpreadRecoil();
		return;
	}

	const float NewMagnitude = FMath::Max(
		0.0f,
		CurrentMagnitude - RecoilDefinition.DecreasePerSecond * FMath::Max(0.0f, DeltaSeconds));
	if (NewMagnitude <= KINDA_SMALL_NUMBER)
	{
		WeaponRecoilOffsetDegrees = FVector2D::ZeroVector;
		return;
	}

	WeaponRecoilOffsetDegrees *= NewMagnitude / CurrentMagnitude;
}

void ATunaSweeperTopDownCharacter::ResetWeaponSpreadRecoil()
{
	WeaponRecoilOffsetDegrees = FVector2D::ZeroVector;
	WeaponRecoilTypeTag = NAME_None;
}

bool ATunaSweeperTopDownCharacter::TryGetSelectedWeaponTypeTag(FName& OutWeaponTypeTag) const
{
	OutWeaponTypeTag = NAME_None;
	if (bMeleeWeaponSelected || SelectedWeaponSlotNumber <= 0)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return false;
	}

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition) ||
		WeaponDefinition.WeaponTypeTag.IsNone())
	{
		return false;
	}

	OutWeaponTypeTag = WeaponDefinition.WeaponTypeTag;
	return true;
}

float ATunaSweeperTopDownCharacter::ResolveWeaponSpreadHalfAngleDegrees(FName WeaponTypeTag) const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperWeaponSpreadRecoilDefinition RecoilDefinition;
	if (!TunaGameInstance || !TunaGameInstance->TryGetWeaponSpreadRecoilDefinition(WeaponTypeTag, RecoilDefinition))
	{
		return 0.0f;
	}

	return FMath::Clamp(
		FMath::Max(RecoilDefinition.MinimumSpreadHalfAngleDegrees, WeaponRecoilOffsetDegrees.Size()),
		RecoilDefinition.MinimumSpreadHalfAngleDegrees,
		RecoilDefinition.MaximumSpreadHalfAngleDegrees);
}

void ATunaSweeperTopDownCharacter::AddWeaponSpreadRecoilShot(FName WeaponTypeTag)
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperWeaponSpreadRecoilDefinition RecoilDefinition;
	if (!TunaGameInstance || !TunaGameInstance->TryGetWeaponSpreadRecoilDefinition(WeaponTypeTag, RecoilDefinition))
	{
		return;
	}

	if (WeaponRecoilTypeTag != WeaponTypeTag)
	{
		WeaponRecoilTypeTag = WeaponTypeTag;
		WeaponRecoilOffsetDegrees = FVector2D::ZeroVector;
	}

	const float KickMagnitude = FMath::Max(0.0f, RecoilDefinition.IncreasePerShot);
	if (KickMagnitude <= 0.0f)
	{
		return;
	}

	const float KickAngleRadians = FMath::FRandRange(0.0f, 2.0f * PI);
	WeaponRecoilOffsetDegrees += FVector2D(FMath::Cos(KickAngleRadians), FMath::Sin(KickAngleRadians)) * KickMagnitude;

	const float MaxMagnitude = FMath::Max(0.0f, RecoilDefinition.MaximumSpreadHalfAngleDegrees);
	const float CurrentMagnitude = WeaponRecoilOffsetDegrees.Size();
	if (MaxMagnitude > 0.0f && CurrentMagnitude > MaxMagnitude)
	{
		WeaponRecoilOffsetDegrees *= MaxMagnitude / CurrentMagnitude;
	}
}

bool ATunaSweeperTopDownCharacter::HasActiveMoveInput() const
{
	return CurrentMoveInput.SizeSquared() > KINDA_SMALL_NUMBER;
}

void ATunaSweeperTopDownCharacter::FinishRoll()
{
	if (!bIsRolling && !bHasSavedProjectileCollisionResponse && !bWeaponAttachedForRoll)
	{
		return;
	}

	bIsRolling = false;
	RollElapsedSeconds = 0.0f;
	SetRollProjectileCollisionPassthrough(false);
	RestoreTemporaryRollVisualRotation();
	RestoreWeaponAfterRoll();
	UpdateMovementSpeed();
}

void ATunaSweeperTopDownCharacter::SetRollProjectileCollisionPassthrough(bool bEnabled)
{
	UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule)
	{
		return;
	}

	if (bEnabled)
	{
		if (!bHasSavedProjectileCollisionResponse)
		{
			SavedProjectileCollisionResponse = Capsule->GetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile);
			bHasSavedProjectileCollisionResponse = true;
		}

		Capsule->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Ignore);
		return;
	}

	if (bHasSavedProjectileCollisionResponse)
	{
		Capsule->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, SavedProjectileCollisionResponse);
		bHasSavedProjectileCollisionResponse = false;
	}
}

void ATunaSweeperTopDownCharacter::AttachWeaponForRoll()
{
	if (!EquippedWeapon || bWeaponAttachedForRoll)
	{
		return;
	}

	USceneComponent* WeaponRoot = EquippedWeapon->GetRootComponent();
	if (!WeaponRoot)
	{
		return;
	}

	FName RollSocketName = NAME_None;
	USceneComponent* RollAttachParent = ResolveRollWeaponAttachParent(RollSocketName);
	if (!RollAttachParent)
	{
		return;
	}

	SavedWeaponAttachParent = WeaponRoot->GetAttachParent();
	SavedWeaponAttachSocketName = WeaponRoot->GetAttachSocketName();
	SavedWeaponRelativeTransform = WeaponRoot->GetRelativeTransform();
	bWeaponAttachedForRoll = true;

	EquippedWeapon->AttachToComponent(
		RollAttachParent,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		RollSocketName);
}

void ATunaSweeperTopDownCharacter::RestoreWeaponAfterRoll()
{
	if (!bWeaponAttachedForRoll)
	{
		return;
	}

	USceneComponent* WeaponRoot = EquippedWeapon ? EquippedWeapon->GetRootComponent() : nullptr;
	USceneComponent* RestoreParent = SavedWeaponAttachParent.Get();
	if (WeaponRoot)
	{
		if (!RestoreParent)
		{
			RestoreParent = WeaponAttachPoint;
		}

		if (RestoreParent)
		{
			EquippedWeapon->AttachToComponent(
				RestoreParent,
				FAttachmentTransformRules::KeepRelativeTransform,
				SavedWeaponAttachSocketName);
			WeaponRoot->SetRelativeTransform(SavedWeaponRelativeTransform);
		}
		else
		{
			EquippedWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		}
	}

	bWeaponAttachedForRoll = false;
	SavedWeaponAttachParent.Reset();
	SavedWeaponAttachSocketName = NAME_None;
	SavedWeaponRelativeTransform = FTransform::Identity;
}

USceneComponent* ATunaSweeperTopDownCharacter::ResolveRollWeaponAttachParent(FName& OutSocketName) const
{
	OutSocketName = NAME_None;

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		if (!RollWeaponHandSocketName.IsNone() && CharacterMesh->DoesSocketExist(RollWeaponHandSocketName))
		{
			OutSocketName = RollWeaponHandSocketName;
			return CharacterMesh;
		}
	}

	if (RollWeaponHandAttachPoint)
	{
		return RollWeaponHandAttachPoint;
	}

	return GetMesh();
}

void ATunaSweeperTopDownCharacter::ApplyTemporaryRollVisualRotation(float NormalizedRollTime)
{
	if (!bUseTemporaryRollVisualRotation)
	{
		return;
	}

	bRollVisualRotationApplied = true;
	const float RollAngleRadians = FMath::DegreesToRadians(
		TemporaryRollVisualRightAxisDegrees * FMath::Clamp(NormalizedRollTime, 0.0f, 1.0f));
	const FQuat RightAxisRoll(FVector::RightVector, RollAngleRadians);
	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetRelativeRotation((RightAxisRoll * DefaultSkeletalMeshRelativeRotation.Quaternion()).Rotator());
	}

	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation((RightAxisRoll * DefaultVisualMeshRelativeRotation.Quaternion()).Rotator());
	}
}

void ATunaSweeperTopDownCharacter::RestoreTemporaryRollVisualRotation()
{
	if (!bRollVisualRotationApplied)
	{
		return;
	}

	if (USkeletalMeshComponent* CharacterMesh = GetMesh())
	{
		CharacterMesh->SetRelativeRotation(DefaultSkeletalMeshRelativeRotation);
	}

	if (VisualMesh)
	{
		VisualMesh->SetRelativeRotation(DefaultVisualMeshRelativeRotation);
	}

	bRollVisualRotationApplied = false;
}

FVector ATunaSweeperTopDownCharacter::ResolveRollDirection() const
{
	FVector ResolvedDirection(CurrentMoveInput.Y, CurrentMoveInput.X, 0.0f);
	if (!ResolvedDirection.Normalize())
	{
		ResolvedDirection = AimDirection;
		ResolvedDirection.Z = 0.0f;
		ResolvedDirection.Normalize();
	}

	if (ResolvedDirection.IsNearlyZero())
	{
		ResolvedDirection = GetActorForwardVector();
		ResolvedDirection.Z = 0.0f;
		ResolvedDirection.Normalize();
	}

	return ResolvedDirection.IsNearlyZero() ? FVector::ForwardVector : ResolvedDirection;
}
