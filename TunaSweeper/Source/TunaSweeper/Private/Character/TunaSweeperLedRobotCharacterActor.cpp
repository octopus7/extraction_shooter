#include "Character/TunaSweeperLedRobotCharacterActor.h"

#include "Component/TunaSweeperLedExpressionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperQuestNoticeWidget.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperLedRobotCharacterActor::ATunaSweeperLedRobotCharacterActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(SceneRoot);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);
	BodyMesh->SetCastShadow(true);

	BodyCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BodyCollision"));
	BodyCollision->SetupAttachment(SceneRoot);
	BodyCollision->SetMobility(EComponentMobility::Movable);
	BodyCollision->SetRelativeLocation(BodyRelativeLocation);
	BodyCollision->SetCapsuleSize(BodyCollisionRadius, BodyCollisionHalfHeight);
	BodyCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BodyCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BodyCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BodyCollision->SetGenerateOverlapEvents(false);
	BodyCollision->CanCharacterStepUpOn = ECB_No;
	BodyCollision->bEditableWhenInherited = true;

	ExpressionComponent = CreateDefaultSubobject<UTunaSweeperLedExpressionComponent>(TEXT("ExpressionComponent"));
	ExpressionComponent->SetupAttachment(SceneRoot);
	ExpressionComponent->SetMobility(EComponentMobility::Movable);
	ExpressionComponent->SetRelativeLocation(FVector(52.0f, 0.0f, 122.0f));
	ExpressionComponent->SetRelativeRotation(FRotator::ZeroRotator);
	ExpressionComponent->bEditableWhenInherited = true;

	InteractionMarkerWidgetClass = TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C")));
	QuestFallbackId = UTunaSweeperQuestSubsystem::GetFirstOutingQuestId();
	QuestProviderId = UTunaSweeperQuestSubsystem::GetCanBotProviderId();

	DialogueInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("DialogueInteractable"));
	DialogueInteractableComponent->SetupAttachment(SceneRoot);
	DialogueInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 155.0f));
	DialogueInteractableComponent->ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::CanBotDialogue,
		FText::FromString(TEXT("\uB300\uD654")),
		InteractionMarkerWidgetClass,
		FName(TEXT("ui.interaction.canbot_dialogue")));
	DialogueInteractableComponent->SetInteractionOrder(0);
	DialogueInteractableComponent->bEditableWhenInherited = true;

	QuestInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("QuestInteractable"));
	QuestInteractableComponent->SetupAttachment(SceneRoot);
	QuestInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 155.0f));
	QuestInteractableComponent->ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::Quest,
		FText::FromString(TEXT("\uD018\uC2A4\uD2B8")),
		InteractionMarkerWidgetClass,
		FName(TEXT("ui.quest.interaction_name")));
	QuestInteractableComponent->SetInteractionOrder(1);
	QuestInteractableComponent->bEditableWhenInherited = true;

	QuestNoticeWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("QuestNoticeWidget"));
	QuestNoticeWidgetComponent->SetupAttachment(SceneRoot);
	QuestNoticeWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 228.0f));
	QuestNoticeWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	QuestNoticeWidgetComponent->SetWidgetClass(UTunaSweeperQuestNoticeWidget::StaticClass());
	QuestNoticeWidgetComponent->SetDrawSize(FVector2D(56.0f, 56.0f));
	QuestNoticeWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	QuestNoticeWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	QuestNoticeWidgetComponent->SetVisibility(false);
	QuestNoticeWidgetComponent->SetHiddenInGame(false);
	QuestNoticeWidgetComponent->bEditableWhenInherited = true;

	BodyMeshOverride = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
	ExpressionPresetFilePath = TEXT("Data/LedExpressionPresets.txt");

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		BodyMesh->SetStaticMesh(CylinderMesh.Object);
	}
}

void ATunaSweeperLedRobotCharacterActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePlayerLookAt(DeltaSeconds);
}

void ATunaSweeperLedRobotCharacterActor::ConfigureRobotDefaults(
	FName InRobotId,
	const FString& InExpressionPresetFilePath,
	FName InInitialExpressionName,
	FLinearColor InLedColor,
	FLinearColor InOffColor,
	float InLedPitch,
	float InLedRadius,
	TSoftObjectPtr<UMaterialInterface> InBodyMaterial,
	bool bOverrideLedColor,
	bool bOverrideOffColor,
	bool bOverrideLedPitch,
	bool bOverrideLedRadius)
{
	RobotId = InRobotId.IsNone() ? RobotId : InRobotId;
	if (!InExpressionPresetFilePath.TrimStartAndEnd().IsEmpty())
	{
		ExpressionPresetFilePath = InExpressionPresetFilePath.TrimStartAndEnd();
	}
	if (!InInitialExpressionName.IsNone())
	{
		InitialExpressionName = InInitialExpressionName;
	}
	if (!InBodyMaterial.IsNull())
	{
		BodyMaterial = InBodyMaterial;
	}

	if (ExpressionComponent)
	{
		ExpressionComponent->ConfigureExpressionSource(ExpressionPresetFilePath, InitialExpressionName);
		if (bOverrideLedColor || bOverrideOffColor || bOverrideLedPitch || bOverrideLedRadius)
		{
			ExpressionComponent->ConfigureLedAppearance(
				bOverrideLedColor ? InLedColor : ExpressionComponent->GetLedColor(),
				bOverrideOffColor ? InOffColor : ExpressionComponent->GetOffColor(),
				bOverrideLedPitch ? InLedPitch : ExpressionComponent->GetLedPitch(),
				bOverrideLedRadius ? InLedRadius : ExpressionComponent->GetLedRadius());
		}
		ExpressionComponent->SetExpressionByName(InitialExpressionName);
	}

	RefreshRobotVisuals();
}

bool ATunaSweeperLedRobotCharacterActor::SetExpressionByName(FName ExpressionName)
{
	return ExpressionComponent ? ExpressionComponent->SetExpressionByName(ExpressionName) : false;
}

void ATunaSweeperLedRobotCharacterActor::ConfigureExpressionDemo(bool bEnabled, float InIntervalSeconds)
{
	bExpressionDemoMode = bEnabled;
	ExpressionDemoIntervalSeconds = FMath::Max(0.1f, InIntervalSeconds);
	if (ExpressionComponent)
	{
		ExpressionComponent->SetDemoExpressionIntervalSeconds(ExpressionDemoIntervalSeconds);
		ExpressionComponent->SetDemoModeEnabled(bExpressionDemoMode);
	}
}

void ATunaSweeperLedRobotCharacterActor::SetExpressionDemoModeEnabled(bool bEnabled)
{
	ConfigureExpressionDemo(bEnabled, ExpressionDemoIntervalSeconds);
}

bool ATunaSweeperLedRobotCharacterActor::IsExpressionDemoModeEnabled() const
{
	return bExpressionDemoMode || (ExpressionComponent && ExpressionComponent->IsDemoModeEnabled());
}

void ATunaSweeperLedRobotCharacterActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshRobotVisuals();
}

void ATunaSweeperLedRobotCharacterActor::BeginPlay()
{
	Super::BeginPlay();
	IdleActorRotation = GetActorRotation();
	PendingLookAtYaw = IdleActorRotation.Yaw;
	LookAtReactionDelay = FMath::FRandRange(LookAtMinReactionDelay, FMath::Max(LookAtMinReactionDelay, LookAtMaxReactionDelay));
	RefreshRobotVisuals();
	SetExpressionByName(InitialExpressionName);

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
				QuestSubsystem->OnQuestProgressChanged.AddUObject(
					this,
					&ATunaSweeperLedRobotCharacterActor::RefreshQuestNoticeVisibility);
			}
		}
	}

	RefreshQuestNoticeVisibility();
}

void ATunaSweeperLedRobotCharacterActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
			}
		}
	}

	Super::EndPlay(EndPlayReason);
}

FName ATunaSweeperLedRobotCharacterActor::ResolveQuestId() const
{
	const FName EffectiveProviderId = QuestProviderId.IsNone()
		? UTunaSweeperQuestSubsystem::GetCanBotProviderId()
		: QuestProviderId;

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (const UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				FName ResolvedQuestId = NAME_None;
				if (QuestSubsystem->TryResolveQuestForProvider(EffectiveProviderId, QuestFallbackId, ResolvedQuestId))
				{
					return ResolvedQuestId;
				}

				return NAME_None;
			}
		}
	}

	return QuestFallbackId;
}

void ATunaSweeperLedRobotCharacterActor::RefreshRobotVisuals()
{
	if (BodyMesh)
	{
		UStaticMesh* MeshToUse = BodyMeshOverride.IsNull()
			? LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"))
			: BodyMeshOverride.LoadSynchronous();
		if (MeshToUse)
		{
			BodyMesh->SetStaticMesh(MeshToUse);
		}

		if (!BodyMaterial.IsNull())
		{
			if (UMaterialInterface* LoadedBodyMaterial = BodyMaterial.LoadSynchronous())
			{
				BodyMesh->SetMaterial(0, LoadedBodyMaterial);
			}
		}

		BodyMesh->SetRelativeLocation(BodyRelativeLocation);
		BodyMesh->SetRelativeScale3D(BodyScale);
		BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BodyMesh->SetGenerateOverlapEvents(false);
	}

	if (ExpressionComponent)
	{
		ExpressionComponent->ConfigureExpressionSource(ExpressionPresetFilePath, InitialExpressionName);
		RefreshExpressionDemoSettings();
	}
}

void ATunaSweeperLedRobotCharacterActor::RefreshExpressionDemoSettings()
{
	if (!ExpressionComponent)
	{
		return;
	}

	const bool bComponentDemoModeEnabled = ExpressionComponent->IsDemoModeEnabled();
	const bool bResolvedDemoModeEnabled = bExpressionDemoMode || bComponentDemoModeEnabled;
	const float ResolvedIntervalSeconds = bExpressionDemoMode
		? ExpressionDemoIntervalSeconds
		: ExpressionComponent->GetDemoExpressionIntervalSeconds();

	ExpressionComponent->SetDemoExpressionIntervalSeconds(FMath::Max(0.1f, ResolvedIntervalSeconds));
	ExpressionComponent->SetDemoModeEnabled(bResolvedDemoModeEnabled);
}

void ATunaSweeperLedRobotCharacterActor::RefreshQuestNoticeVisibility()
{
	if (QuestNoticeWidgetComponent)
	{
		QuestNoticeWidgetComponent->SetVisibility(ShouldShowQuestNotice());
	}
}

bool ATunaSweeperLedRobotCharacterActor::ShouldShowQuestNotice() const
{
	const FName ResolvedQuestId = ResolveQuestId();
	if (ResolvedQuestId.IsNone())
	{
		return false;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return false;
	}

	const ETunaSweeperQuestState State = QuestSubsystem->GetQuestState(ResolvedQuestId);
	return (State == ETunaSweeperQuestState::Available && QuestSubsystem->CanAcceptQuest(ResolvedQuestId)) ||
		State == ETunaSweeperQuestState::RewardAvailable;
}

void ATunaSweeperLedRobotCharacterActor::UpdatePlayerLookAt(float DeltaSeconds)
{
	if (!bLookAtNearbyPlayer || DeltaSeconds <= 0.0f)
	{
		return;
	}

	float PlayerYaw = 0.0f;
	float PlayerDistance2D = 0.0f;
	const bool bHasPlayer = TryGetPlayerLookYaw(PlayerYaw, PlayerDistance2D);
	const float StartDistance = FMath::Max(0.0f, LookAtStartDistance);
	const float StopDistance = FMath::Max(StartDistance, LookAtStopDistance);

	if (!bIsLookingAtPlayer)
	{
		if (bHasPlayer && PlayerDistance2D <= StartDistance)
		{
			if (!bLookAtReactionPending)
			{
				bLookAtReactionPending = true;
				LookAtReactionElapsed = 0.0f;
				LookAtReactionDelay = FMath::FRandRange(
					FMath::Max(0.0f, LookAtMinReactionDelay),
					FMath::Max(LookAtMinReactionDelay, LookAtMaxReactionDelay));
			}

			LookAtReactionElapsed += DeltaSeconds;
			if (LookAtReactionElapsed >= LookAtReactionDelay)
			{
				bIsLookingAtPlayer = true;
				bLookAtReactionPending = false;
				LookAtRefreshElapsed = LookAtTargetRefreshInterval;
			}
		}
		else
		{
			bLookAtReactionPending = false;
			LookAtReactionElapsed = 0.0f;
		}
	}
	else if (!bHasPlayer || PlayerDistance2D >= StopDistance)
	{
		bIsLookingAtPlayer = false;
		bLookAtReactionPending = false;
		LookAtReactionElapsed = 0.0f;
	}

	float DesiredYaw = IdleActorRotation.Yaw;
	float InterpSpeed = LookAtReturnInterpolationSpeed;
	if (bIsLookingAtPlayer && bHasPlayer)
	{
		LookAtRefreshElapsed += DeltaSeconds;
		if (LookAtRefreshElapsed >= FMath::Max(0.01f, LookAtTargetRefreshInterval))
		{
			PendingLookAtYaw = PlayerYaw;
			LookAtRefreshElapsed = 0.0f;
		}

		DesiredYaw = PendingLookAtYaw + ResolveNonMechanicalYawOffset(DeltaSeconds);
		InterpSpeed = LookAtInterpolationSpeed;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation(CurrentRotation.Pitch, DesiredYaw, CurrentRotation.Roll);
	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaSeconds,
		FMath::Max(0.0f, InterpSpeed));
	SetActorRotation(NewRotation);
}

bool ATunaSweeperLedRobotCharacterActor::TryGetPlayerLookYaw(float& OutYaw, float& OutDistance2D) const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn)
	{
		return false;
	}

	const FVector ToPlayer = PlayerPawn->GetActorLocation() - GetActorLocation();
	const FVector ToPlayer2D(ToPlayer.X, ToPlayer.Y, 0.0f);
	OutDistance2D = ToPlayer2D.Size();
	if (OutDistance2D <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	OutYaw = ToPlayer2D.Rotation().Yaw;
	return true;
}

float ATunaSweeperLedRobotCharacterActor::ResolveNonMechanicalYawOffset(float DeltaSeconds)
{
	const float MaxOffset = FMath::Max(0.0f, LookAtYawOffsetDegrees);
	if (MaxOffset <= 0.0f)
	{
		LookAtYawOffset = 0.0f;
		LookAtYawOffsetTarget = 0.0f;
		return 0.0f;
	}

	LookAtYawOffsetRefreshElapsed += DeltaSeconds;
	if (LookAtYawOffsetRefreshElapsed >= 1.2f)
	{
		LookAtYawOffsetTarget = FMath::FRandRange(-MaxOffset, MaxOffset);
		LookAtYawOffsetRefreshElapsed = 0.0f;
	}

	LookAtYawOffset = FMath::FInterpTo(LookAtYawOffset, LookAtYawOffsetTarget, DeltaSeconds, 1.2f);
	return LookAtYawOffset;
}
