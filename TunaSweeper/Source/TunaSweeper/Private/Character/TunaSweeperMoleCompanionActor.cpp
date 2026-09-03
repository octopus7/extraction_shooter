#include "Character/TunaSweeperMoleCompanionActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperQuestNoticeWidget.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperMoleCompanionActor::ATunaSweeperMoleCompanionActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DummyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DummyMesh"));
	DummyMesh->SetupAttachment(SceneRoot);
	DummyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DummyMesh->SetGenerateOverlapEvents(false);
	DummyMesh->SetCastShadow(true);
	DummyMesh->bEditableWhenInherited = true;

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(SceneRoot);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetGenerateOverlapEvents(false);
	SkeletalMesh->SetCastShadow(true);
	SkeletalMesh->SetVisibility(false, true);
	SkeletalMesh->SetHiddenInGame(true, true);
	SkeletalMesh->bEditableWhenInherited = true;

	BodyCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BodyCollision"));
	BodyCollision->SetupAttachment(SceneRoot);
	BodyCollision->SetMobility(EComponentMobility::Movable);
	BodyCollision->SetRelativeLocation(BodyCollisionRelativeLocation);
	BodyCollision->SetCapsuleSize(BodyCollisionRadius, BodyCollisionHalfHeight);
	BodyCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BodyCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BodyCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BodyCollision->SetGenerateOverlapEvents(false);
	BodyCollision->CanCharacterStepUpOn = ECB_No;
	BodyCollision->bEditableWhenInherited = true;

	InteractionMarkerWidgetClass = TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C")));
	QuestFallbackId = UTunaSweeperQuestSubsystem::GetFirstOutingQuestId();
	QuestProviderId = UTunaSweeperQuestSubsystem::GetMoleProviderId();

	DialogueInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("DialogueInteractable"));
	DialogueInteractableComponent->SetupAttachment(SceneRoot);
	DialogueInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
	DialogueInteractableComponent->ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::MoleDialogue,
		FText::FromString(TEXT("\uB300\uD654")),
		InteractionMarkerWidgetClass,
		FName(TEXT("ui.interaction.mole_dialogue")));
	DialogueInteractableComponent->SetInteractionOrder(0);
	DialogueInteractableComponent->bEditableWhenInherited = true;

	QuestInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("QuestInteractable"));
	QuestInteractableComponent->SetupAttachment(SceneRoot);
	QuestInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
	QuestInteractableComponent->ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::Quest,
		FText::FromString(TEXT("\uD018\uC2A4\uD2B8")),
		InteractionMarkerWidgetClass,
		FName(TEXT("ui.quest.interaction_name")));
	QuestInteractableComponent->SetInteractionOrder(1);
	QuestInteractableComponent->bEditableWhenInherited = true;

	QuestNoticeWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("QuestNoticeWidget"));
	QuestNoticeWidgetComponent->SetupAttachment(SceneRoot);
	QuestNoticeWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 190.0f));
	QuestNoticeWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	QuestNoticeWidgetComponent->SetWidgetClass(UTunaSweeperQuestNoticeWidget::StaticClass());
	QuestNoticeWidgetComponent->SetDrawSize(FVector2D(56.0f, 56.0f));
	QuestNoticeWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	QuestNoticeWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	QuestNoticeWidgetComponent->SetVisibility(false);
	QuestNoticeWidgetComponent->SetHiddenInGame(false);
	QuestNoticeWidgetComponent->bEditableWhenInherited = true;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> MoleDummyMesh(
		TEXT("/Game/Characters/NPC/Mole/SM_MoleDummy.SM_MoleDummy"));
	if (MoleDummyMesh.Succeeded())
	{
		DummyMesh->SetStaticMesh(MoleDummyMesh.Object);
	}
}

void ATunaSweeperMoleCompanionActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdatePlayerLookAt(DeltaSeconds);
}

void ATunaSweeperMoleCompanionActor::ConfigureCompanionDefaults(
	FName InCompanionId,
	TSoftObjectPtr<UStaticMesh> InDummyMesh,
	TSoftObjectPtr<UMaterialInterface> InVisualMaterial)
{
	CompanionId = InCompanionId.IsNone() ? CompanionId : InCompanionId;
	if (!InDummyMesh.IsNull())
	{
		DummyMeshOverride = InDummyMesh;
	}
	if (!InVisualMaterial.IsNull())
	{
		VisualMaterial = InVisualMaterial;
	}

	RefreshCompanionVisuals();
}

void ATunaSweeperMoleCompanionActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshCompanionVisuals();
}

void ATunaSweeperMoleCompanionActor::BeginPlay()
{
	Super::BeginPlay();
	IdleActorRotation = GetActorRotation();
	PendingLookAtYaw = IdleActorRotation.Yaw;
	LookAtReactionDelay = FMath::FRandRange(LookAtMinReactionDelay, FMath::Max(LookAtMinReactionDelay, LookAtMaxReactionDelay));
	RefreshCompanionVisuals();

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
				QuestSubsystem->OnQuestProgressChanged.AddUObject(
					this,
					&ATunaSweeperMoleCompanionActor::RefreshQuestNoticeVisibility);
			}
		}
	}

	RefreshQuestNoticeVisibility();
}

void ATunaSweeperMoleCompanionActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

FName ATunaSweeperMoleCompanionActor::ResolveQuestId() const
{
	const FName EffectiveProviderId = QuestProviderId.IsNone()
		? UTunaSweeperQuestSubsystem::GetMoleProviderId()
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

void ATunaSweeperMoleCompanionActor::RefreshCompanionVisuals()
{
	if (DummyMesh)
	{
		UStaticMesh* MeshToUse = DummyMeshOverride.IsNull()
			? LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Characters/NPC/Mole/SM_MoleDummy.SM_MoleDummy"))
			: DummyMeshOverride.LoadSynchronous();
		if (MeshToUse)
		{
			DummyMesh->SetStaticMesh(MeshToUse);
		}
		if (!VisualMaterial.IsNull())
		{
			if (UMaterialInterface* LoadedMaterial = VisualMaterial.LoadSynchronous())
			{
				DummyMesh->SetMaterial(0, LoadedMaterial);
			}
		}
		DummyMesh->SetRelativeLocation(DummyMeshRelativeLocation);
		DummyMesh->SetRelativeScale3D(DummyMeshScale);
	}

	const bool bUseSkeletalMesh = SkeletalMesh && SkeletalMesh->GetSkeletalMeshAsset() != nullptr;
	if (DummyMesh)
	{
		DummyMesh->SetVisibility(!bUseSkeletalMesh, true);
		DummyMesh->SetHiddenInGame(bUseSkeletalMesh, true);
	}
	if (SkeletalMesh)
	{
		SkeletalMesh->SetVisibility(bUseSkeletalMesh, true);
		SkeletalMesh->SetHiddenInGame(!bUseSkeletalMesh, true);
	}

	if (BodyCollision)
	{
		BodyCollision->SetRelativeLocation(BodyCollisionRelativeLocation);
		BodyCollision->SetCapsuleSize(BodyCollisionRadius, BodyCollisionHalfHeight);
	}
}

void ATunaSweeperMoleCompanionActor::RefreshQuestNoticeVisibility()
{
	if (QuestNoticeWidgetComponent)
	{
		QuestNoticeWidgetComponent->SetVisibility(ShouldShowQuestNotice());
	}
}

bool ATunaSweeperMoleCompanionActor::ShouldShowQuestNotice() const
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

void ATunaSweeperMoleCompanionActor::UpdatePlayerLookAt(float DeltaSeconds)
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
		DesiredYaw = PendingLookAtYaw + ResolveOrganicYawOffset(DeltaSeconds);
		InterpSpeed = LookAtInterpolationSpeed;
	}

	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation(CurrentRotation.Pitch, DesiredYaw, CurrentRotation.Roll);
	SetActorRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, FMath::Max(0.0f, InterpSpeed)));
}

bool ATunaSweeperMoleCompanionActor::TryGetPlayerLookYaw(float& OutYaw, float& OutDistance2D) const
{
	const UWorld* World = GetWorld();
	const APawn* PlayerPawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
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

float ATunaSweeperMoleCompanionActor::ResolveOrganicYawOffset(float DeltaSeconds)
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
