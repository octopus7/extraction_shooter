#include "Character/TunaSweeperMoleCompanionActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/BlendSpace.h"
#include "Component/TunaSweeperQuestMarkerComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperMoleCompanionActor::ATunaSweeperMoleCompanionActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(SceneRoot);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetGenerateOverlapEvents(false);
	SkeletalMesh->SetCastShadow(true);
	SkeletalMesh->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	SkeletalMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	SkeletalMesh->bEditableWhenInherited = true;
	SkeletalMesh->AddTickPrerequisiteActor(this);

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

	QuestMarkerComponent = CreateDefaultSubobject<UTunaSweeperQuestMarkerComponent>(TEXT("QuestMarker"));
	QuestMarkerComponent->SetupAttachment(SceneRoot);
	QuestMarkerComponent->SetMarkerHeight(190.0f);
	QuestMarkerComponent->SetVisibility(false, true);
	QuestMarkerComponent->bEditableWhenInherited = true;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MoleMesh(
		TEXT("/Game/Characters/NPC/Mole/SKM_MoleDummy.SKM_MoleDummy"));
	if (MoleMesh.Succeeded())
	{
		SkeletalMesh->SetSkeletalMesh(MoleMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UBlendSpace> MoleIdleTurn(
		TEXT("/Game/Characters/NPC/Mole/BS_Mole_IdleTurn.BS_Mole_IdleTurn"));
	if (MoleIdleTurn.Succeeded())
	{
		IdleTurnBlendSpace = MoleIdleTurn.Object;
		SkeletalMesh->OverrideAnimationData(IdleTurnBlendSpace, true, true);
	}
}

void ATunaSweeperMoleCompanionActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	const float PreviousYaw = GetActorRotation().Yaw;
	UpdatePlayerLookAt(DeltaSeconds);
	UpdateCompanionAnimation(PreviousYaw, DeltaSeconds);
}

void ATunaSweeperMoleCompanionActor::ConfigureCompanionDefaults(
	FName InCompanionId,
	TSoftObjectPtr<USkeletalMesh> InSkeletalMesh,
	TSoftObjectPtr<UMaterialInterface> InVisualMaterial)
{
	CompanionId = InCompanionId.IsNone() ? CompanionId : InCompanionId;
	if (!InSkeletalMesh.IsNull())
	{
		SkeletalMeshOverride = InSkeletalMesh;
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
	// Rest facing opposite the placed actor's heading, both initially and after looking away.
	IdleActorRotation.Yaw = FRotator::NormalizeAxis(IdleActorRotation.Yaw + 180.0f);
	SetActorRotation(IdleActorRotation);
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
	if (SkeletalMesh)
	{
		USkeletalMesh* MeshToUse = SkeletalMeshOverride.IsNull()
			? LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/NPC/Mole/SKM_MoleDummy.SKM_MoleDummy"))
			: SkeletalMeshOverride.LoadSynchronous();
		if (MeshToUse && SkeletalMesh->GetSkeletalMeshAsset() != MeshToUse)
		{
			SkeletalMesh->SetSkeletalMesh(MeshToUse);
		}
		if (!VisualMaterial.IsNull())
		{
			if (UMaterialInterface* LoadedMaterial = VisualMaterial.LoadSynchronous())
			{
				SkeletalMesh->SetMaterial(0, LoadedMaterial);
			}
		}
		SkeletalMesh->SetVisibility(true, true);
		SkeletalMesh->SetHiddenInGame(false, true);
		if (IdleTurnBlendSpace && SkeletalMesh->AnimationData.AnimToPlay != IdleTurnBlendSpace)
		{
			SkeletalMesh->OverrideAnimationData(IdleTurnBlendSpace, true, true);
		}
	}

	if (BodyCollision)
	{
		BodyCollision->SetRelativeLocation(BodyCollisionRelativeLocation);
		BodyCollision->SetCapsuleSize(BodyCollisionRadius, BodyCollisionHalfHeight);
	}
}

float ATunaSweeperMoleCompanionActor::ResolveTurnAnimationAmount(float PreviousYaw, float CurrentYaw, float DeltaSeconds)
{
	if (DeltaSeconds <= SMALL_NUMBER)
	{
		return 0.0f;
	}
	const float SignedTurnSpeed = FMath::FindDeltaAngleDegrees(PreviousYaw, CurrentYaw) / DeltaSeconds;
	// Ignore tiny gaze drift and settle into breathing as the look-at rotation finishes.
	return FMath::Sign(SignedTurnSpeed) * FMath::GetMappedRangeValueClamped(
		FVector2D(2.0f, 25.0f), FVector2D(0.0f, 1.0f), FMath::Abs(SignedTurnSpeed));
}

void ATunaSweeperMoleCompanionActor::UpdateCompanionAnimation(float PreviousYaw, float DeltaSeconds)
{
	if (UAnimSingleNodeInstance* Animation = SkeletalMesh ? SkeletalMesh->GetSingleNodeInstance() : nullptr)
	{
		// Negative yaw selects left footwork, positive yaw selects right footwork.
		// The clips have fixed roots; only the actor owns the heading.
		const float TurnAmount = ResolveTurnAnimationAmount(PreviousYaw, GetActorRotation().Yaw, DeltaSeconds);
		Animation->SetBlendSpacePosition(FVector(TurnAmount, 0.0f, 0.0f));
		// Source turns cover 90 degrees in 64 frames at 30 fps. Match their cadence
		// to the actual rotation while preserving normal-speed breathing at rest.
		const float TurnSpeed = DeltaSeconds > SMALL_NUMBER
			? FMath::Abs(FMath::FindDeltaAngleDegrees(PreviousYaw, GetActorRotation().Yaw)) / DeltaSeconds : 0.0f;
		const float StepRate = FMath::Clamp(TurnSpeed / (90.0f / (64.0f / 30.0f)), 0.5f, 2.2f);
		Animation->SetPlayRate(FMath::Lerp(1.0f, StepRate, FMath::Abs(TurnAmount)));
	}
}

void ATunaSweeperMoleCompanionActor::RefreshQuestNoticeVisibility()
{
	if (QuestMarkerComponent)
	{
		QuestMarkerComponent->SetVisibility(ShouldShowQuestNotice(), true);
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
				LookAtReturnDelayRemaining = 0.0f;
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
	else if (!bHasPlayer || PlayerDistance2D > StopDistance)
	{
		bIsLookingAtPlayer = false;
		bLookAtReactionPending = false;
		LookAtReactionElapsed = 0.0f;
		LookAtReturnDelayRemaining = FMath::Max(0.0f, LookAtReturnDelay);
	}

	if (!bIsLookingAtPlayer && LookAtReturnDelayRemaining > 0.0f)
	{
		LookAtReturnDelayRemaining = FMath::Max(0.0f, LookAtReturnDelayRemaining - DeltaSeconds);
		// Hold the last heading so the animation settles into idle before returning.
		return;
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
	const FRotator SmoothedRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, FMath::Max(0.0f, InterpSpeed));
	const float MaxYawStep = FMath::Max(0.0f, LookAtMaxTurnSpeed) * DeltaSeconds;
	const float YawStep = FMath::Clamp(FMath::FindDeltaAngleDegrees(CurrentRotation.Yaw, SmoothedRotation.Yaw), -MaxYawStep, MaxYawStep);
	SetActorRotation(FRotator(CurrentRotation.Pitch, CurrentRotation.Yaw + YawStep, CurrentRotation.Roll));
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
