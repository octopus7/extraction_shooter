#include "Character/TunaSweeperTopDownCharacter.h"

#include "Camera/CameraComponent.h"
#include "Component/TunaSweeperPlayerVisionComponent.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "MediaSource.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "TimerManager.h"
#include "TunaSweeperCollisionChannels.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UI/TunaSweeperStaminaGaugeWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperWeapon.h"

namespace TunaSweeperEquippedWeaponVisual
{
	const FName GunCategoryTag(TEXT("item.category.weapon.gun"));
	const FSoftObjectPath AssaultRifleClassPath(TEXT("/Game/Weapons/BP_AssaultRifle.BP_AssaultRifle_C"));
}

namespace TunaSweeperStaminaGauge
{
	constexpr float DrawSize = 70.0f;
	const FVector RelativeLocation(0.0f, 0.0f, -92.0f);
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
	RespawnMediaSource = TSoftObjectPtr<UMediaSource>(FSoftObjectPath(TEXT("/Game/Movies/MS_Respawn.MS_Respawn")));
	RespawnTransitionWidgetClass = TSoftClassPtr<UTunaSweeperLevelTransitionWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_LevelTransitionVideo.WBP_LevelTransitionVideo_C")));
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

	if (!SelectWeaponSlot(1))
	{
		SelectWeaponSlot(2);
	}

	UpdateMovementSpeed();
	UpdateStaminaGauge(0.0f);
}

void ATunaSweeperTopDownCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDead)
	{
		return;
	}

	UpdateRoll(DeltaSeconds);
	UpdateSprintAndStamina(DeltaSeconds);
	UpdateMovementSpeed();
	UpdateStaminaGauge(DeltaSeconds);
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

	const FVector ToAimPoint = FVector(WorldPoint.X - GetActorLocation().X, WorldPoint.Y - GetActorLocation().Y, 0.0f);
	const FVector NewAimDirection = ToAimPoint.GetSafeNormal();
	if (!NewAimDirection.IsNearlyZero())
	{
		AimDirection = NewAimDirection;
	}
}

float ATunaSweeperTopDownCharacter::GetStaminaPercent() const
{
	return MaxStamina > 0.0f
		? FMath::Clamp(CurrentStamina / MaxStamina, 0.0f, 1.0f)
		: 0.0f;
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
	}
}

TSubclassOf<ATunaSweeperWeapon> ATunaSweeperTopDownCharacter::ResolveEquippedWeaponClass() const
{
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
	if (!TunaGameInstance || !TunaGameInstance->TryConsumeLoadedAmmoForWeaponSlot(SelectedWeaponSlotNumber))
	{
		return;
	}

	EquippedWeapon->Fire(AimDirection, this);
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

	if (SelectedWeaponSlotNumber != SlotNumber)
	{
		CancelReload();
		CloseAmmoSelection();
	}

	SelectedWeaponSlotNumber = SlotNumber;
	EnsureEquippedWeaponActor();
	return true;
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

	CancelReload();
	CloseAmmoSelection();
	SelectedWeaponSlotNumber = 0;
	ClearEquippedWeaponActor();
}

void ATunaSweeperTopDownCharacter::RefreshCharacterVisualVisibility()
{
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	if (CharacterMesh)
	{
		CharacterMesh->SetHiddenInGame(false);
		CharacterMesh->SetVisibility(true, true);
	}

	const bool bHasCharacterMesh = CharacterMesh && CharacterMesh->GetSkeletalMeshAsset();
	if (VisualMesh)
	{
		VisualMesh->SetHiddenInGame(bHasCharacterMesh);
		VisualMesh->SetVisibility(!bHasCharacterMesh, true);
	}
}

bool ATunaSweeperTopDownCharacter::IsGameplayActionInputLocked() const
{
	const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController());
	return TunaPlayerController &&
		(TunaPlayerController->IsInventoryUiOpen() || TunaPlayerController->IsDialogueSequenceActive());
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
	GetWorldTimerManager().ClearTimer(FireTimerHandle);

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
		const float TargetFOV = bIsAiming ? CameraModeSettings.AimFOV : CameraModeSettings.DefaultFOV;
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

		const FVector AimTargetOffset = bIsAiming ? AimDirection * AimCameraLeadDistance : FVector::ZeroVector;
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
	(void)DamageCauser;

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
