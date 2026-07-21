#include "Character/TunaSweeperTopDownCharacter.h"
#include "TunaSweeperTopDownCharacterShared.h"

#include "Component/TunaSweeperFactionComponent.h"

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
	DebuffComponent = CreateDefaultSubobject<UTunaSweeperDebuffComponent>(TEXT("DebuffComponent"));
	FactionComponent = CreateDefaultSubobject<UTunaSweeperFactionComponent>(TEXT("FactionComponent"));
	FactionComponent->SetFactionId(TunaSweeperFactionIds::Player);
	PlayerVisionComponent = CreateDefaultSubobject<UTunaSweeperPlayerVisionComponent>(TEXT("PlayerVisionComponent"));
	HeadphoneListenerComponent = CreateDefaultSubobject<UTunaSweeperHeadphoneListenerComponent>(TEXT("HeadphoneListenerComponent"));
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
	InteractionFocusAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/IA_InteractionFocus.IA_InteractionFocus")));
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
	CacheBaseSurvivalStats();
	ApplyExperienceLevelStatBonuses();
	ApplyBunkerPeaceZoneVitalsRules();
	CurrentStamina = FMath::Max(0.0f, MaxStamina);
	StaminaGaugeOpacity = 0.0f;

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &ATunaSweeperTopDownCharacter::HandleInventoryStateChanged);
		TunaGameInstance->OnExperienceChanged.RemoveAll(this);
		TunaGameInstance->OnExperienceChanged.AddUObject(this, &ATunaSweeperTopDownCharacter::ApplyExperienceLevelStatBonuses);
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
	RefreshCarryWeightConditionDebuffs();
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
	UpdateMeleeSwing(DeltaSeconds);
	UpdateSprintAndStamina(DeltaSeconds);
	UpdateMovementSpeed();
	UpdatePlayerFootsteps(DeltaSeconds);
	UpdateStaminaGauge(DeltaSeconds);
	UpdateAimingVisuals(DeltaSeconds);
}

void ATunaSweeperTopDownCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
		GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
		GetWorldTimerManager().ClearTimer(ItemUseTimerHandle);
		GetWorldTimerManager().ClearTimer(RespawnTransitionTimerHandle);
	}

	if (VitalsComponent)
	{
		VitalsComponent->OnVitalsChanged.RemoveDynamic(this, &ATunaSweeperTopDownCharacter::HandleVitalsChanged);
	}

	FinishRoll();
	CancelItemUse();
	CancelMeleeSwing();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnExperienceChanged.RemoveAll(this);
	}

	Super::EndPlay(EndPlayReason);
}

void ATunaSweeperTopDownCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();

	AddDefaultInputMapping();
}

