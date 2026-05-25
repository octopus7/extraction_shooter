#include "Character/TunaSweeperTopDownCharacter.h"

#include "Camera/CameraComponent.h"
#include "Component/TunaSweeperPlayerVisionComponent.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
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
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperWeapon.h"

namespace TunaSweeperEquippedWeaponVisual
{
	const FName GunCategoryTag(TEXT("item.category.weapon.gun"));
	const FSoftObjectPath AssaultRifleClassPath(TEXT("/Game/Weapons/BP_AssaultRifle.BP_AssaultRifle_C"));
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

	DefaultMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_Player.IMC_Player")));
	MoveAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Move.IA_Move")));
	FireAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Fire.IA_Fire")));
	AimAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Aim.IA_Aim")));
	InteractAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Interact.IA_Interact")));
	InventoryAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Inventory.IA_Inventory")));
	ReloadAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Reload.IA_Reload")));
	AmmoSelectAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_AmmoSelect.IA_AmmoSelect")));
	AmmoFocusAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_AmmoFocus.IA_AmmoFocus")));
	CameraModeAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_CameraMode.IA_CameraMode")));
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
}

void ATunaSweeperTopDownCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsDead)
	{
		return;
	}

	UpdateCarryWeightMovementSpeed();
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
}

void ATunaSweeperTopDownCharacter::HandleMove(const FInputActionValue& Value)
{
	if (bIsDead || IsGameplayActionInputLocked())
	{
		return;
	}

	const FVector2D MoveVector = Value.Get<FVector2D>();
	if (!FMath::IsNearlyZero(MoveVector.Y))
	{
		AddMovementInput(FVector::ForwardVector, MoveVector.Y);
	}

	if (!FMath::IsNearlyZero(MoveVector.X))
	{
		AddMovementInput(FVector::RightVector, MoveVector.X);
	}
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
	bFireHeld = false;
	bIsAiming = false;
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
	bFireHeld = false;
	bIsAiming = false;
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
		if (UTunaSweeperLevelTransitionSubsystem* TransitionSubsystem = GameInstance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>())
		{
			if (TransitionSubsystem->StartTransition(
				this,
				RespawnTargetLevelName,
				RespawnMediaSource,
				RespawnTransitionWidgetClass,
				RespawnFadeToBlackDuration,
				RespawnFadeFromBlackDuration,
				FText::FromString(TEXT("구급 카트 후송 중"))))
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

	if (!AimDirection.IsNearlyZero())
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

void ATunaSweeperTopDownCharacter::UpdateCarryWeightMovementSpeed()
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

	MovementComponent->MaxWalkSpeed = BaseWalkSpeed * FMath::Clamp(CarryWeightSpeedMultiplier, 0.0f, 1.0f);
}
