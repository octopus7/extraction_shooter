#include "Character/TunaSweeperTopDownCharacter.h"

#include "Camera/CameraComponent.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
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
#include "TimerManager.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "Weapon/TunaSweeperWeapon.h"

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

	GetMesh()->SetHiddenInGame(true);
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
	WeaponAttachPoint->SetRelativeLocation(FVector(60.0f, 0.0f, 35.0f));

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

	VitalsComponent = CreateDefaultSubobject<UTunaSweeperVitalsComponent>(TEXT("VitalsComponent"));

	DefaultMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_Player.IMC_Player")));
	MoveAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Move.IA_Move")));
	FireAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Fire.IA_Fire")));
	AimAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Aim.IA_Aim")));
	InteractAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Interact.IA_Interact")));
	InventoryAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_Inventory.IA_Inventory")));
	DefaultWeaponClass = TSoftClassPtr<ATunaSweeperWeapon>(FSoftObjectPath(TEXT("/Game/Weapons/BP_TunaSweeperWeapon.BP_TunaSweeperWeapon_C")));
	RespawnMediaSource = TSoftObjectPtr<UMediaSource>(FSoftObjectPath(TEXT("/Game/Movies/MS_Respawn.MS_Respawn")));
	RespawnTransitionWidgetClass = TSoftClassPtr<UTunaSweeperLevelTransitionWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_LevelTransitionVideo.WBP_LevelTransitionVideo_C")));
}

void ATunaSweeperTopDownCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultCameraFOV = TopDownCamera ? TopDownCamera->FieldOfView : DefaultCameraFOV;
	SpawnDefaultWeapon();

	if (VitalsComponent)
	{
		VitalsComponent->OnVitalsChanged.AddDynamic(this, &ATunaSweeperTopDownCharacter::HandleVitalsChanged);
		HandleVitalsChanged(VitalsComponent->GetVitalsState());
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
		GetWorldTimerManager().ClearTimer(RespawnTransitionTimerHandle);
	}

	if (VitalsComponent)
	{
		VitalsComponent->OnVitalsChanged.RemoveDynamic(this, &ATunaSweeperTopDownCharacter::HandleVitalsChanged);
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

void ATunaSweeperTopDownCharacter::SpawnDefaultWeapon()
{
	if (EquippedWeapon || !GetWorld())
	{
		return;
	}

	TSubclassOf<ATunaSweeperWeapon> LoadedWeaponClass = DefaultWeaponClass.LoadSynchronous();
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

void ATunaSweeperTopDownCharacter::HandleMove(const FInputActionValue& Value)
{
	if (bIsDead)
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
	if (bIsDead)
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
	if (bIsDead)
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
		TunaPlayerController->ToggleInventoryOnlyPanel();
	}
}

void ATunaSweeperTopDownCharacter::FireWeapon()
{
	if (!bIsDead && EquippedWeapon)
	{
		EquippedWeapon->Fire(AimDirection, this);
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
	GetWorldTimerManager().ClearTimer(FireTimerHandle);

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
				FText::FromString(TEXT("Respawning"))))
			{
				return;
			}
		}
	}

	UGameplayStatics::OpenLevel(this, RespawnTargetLevelName);
}

void ATunaSweeperTopDownCharacter::UpdateAimingVisuals(float DeltaSeconds)
{
	if (!AimDirection.IsNearlyZero())
	{
		const FRotator CurrentRotation = GetActorRotation();
		const FRotator TargetRotation(0.0f, AimDirection.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 18.0f));
	}

	if (TopDownCamera)
	{
		const float TargetFOV = bIsAiming ? AimCameraFOV : DefaultCameraFOV;
		TopDownCamera->SetFieldOfView(FMath::FInterpTo(TopDownCamera->FieldOfView, TargetFOV, DeltaSeconds, CameraInterpSpeed));
	}

	if (CameraBoom)
	{
		const FVector TargetOffset = bIsAiming ? AimDirection * AimCameraLeadDistance : FVector::ZeroVector;
		CameraBoom->TargetOffset = FMath::VInterpTo(CameraBoom->TargetOffset, TargetOffset, DeltaSeconds, CameraInterpSpeed);
	}
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
