#include "Character/TunaSweeperFacilityNpcActor.h"

#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Component/TunaSweeperQuestMarkerComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperFacilityNpcActor::ATunaSweeperFacilityNpcActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(SceneRoot);
	BodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 62.0f));
	BodyMesh->SetRelativeScale3D(FVector(0.46f, 0.46f, 0.88f));
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BodyMesh->SetGenerateOverlapEvents(false);

	HeadMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadMesh"));
	HeadMesh->SetupAttachment(SceneRoot);
	HeadMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 128.0f));
	HeadMesh->SetRelativeScale3D(FVector(0.62f, 0.48f, 0.32f));
	HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeadMesh->SetGenerateOverlapEvents(false);

	BodyCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("BodyCollision"));
	BodyCollision->SetupAttachment(SceneRoot);
	BodyCollision->SetRelativeLocation(FVector(0.0f, 0.0f, 82.0f));
	BodyCollision->SetCapsuleSize(42.0f, 82.0f);
	BodyCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BodyCollision->SetCollisionObjectType(ECC_WorldDynamic);
	BodyCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BodyCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	BodyCollision->SetGenerateOverlapEvents(false);
	BodyCollision->CanCharacterStepUpOn = ECB_No;

	InteractionMarkerWidgetClass = TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C")));

	QuestInteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("QuestInteractable"));
	QuestInteractableComponent->SetupAttachment(SceneRoot);
	QuestInteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 156.0f));
	QuestInteractableComponent->SetInteractionOrder(0);
	QuestInteractableComponent->ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::Quest,
		FText::FromString(TEXT("Quest")),
		InteractionMarkerWidgetClass,
		FName(TEXT("ui.quest.interaction_name")));

	QuestMarkerComponent = CreateDefaultSubobject<UTunaSweeperQuestMarkerComponent>(TEXT("QuestMarker"));
	QuestMarkerComponent->SetupAttachment(SceneRoot);
	QuestMarkerComponent->SetMarkerHeight(214.0f);
	QuestMarkerComponent->SetVisibility(false, true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMeshFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMeshFinder.Succeeded())
	{
		DefaultBodyMesh = CylinderMeshFinder.Object;
		BodyMesh->SetStaticMesh(DefaultBodyMesh);
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshFinder(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshFinder.Succeeded())
	{
		DefaultHeadMesh = CubeMeshFinder.Object;
		HeadMesh->SetStaticMesh(DefaultHeadMesh);
	}
}

void ATunaSweeperFacilityNpcActor::BeginPlay()
{
	Super::BeginPlay();

	if (QuestInteractableComponent)
	{
		QuestInteractableComponent->SetObjectiveEventId(QuestInteractionEventId);
	}

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
				QuestSubsystem->OnQuestProgressChanged.AddUObject(
					this,
					&ATunaSweeperFacilityNpcActor::RefreshQuestNoticeVisibility);
			}
		}
	}

	RefreshQuestNoticeVisibility();
}

void ATunaSweeperFacilityNpcActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

void ATunaSweeperFacilityNpcActor::ConfigureFacilityNpcDefaults(
	FName InNpcId,
	FName InQuestProviderId,
	FName InQuestFallbackId,
	FName InQuestInteractionEventId,
	FVector InBodyScale,
	FVector InHeadScale,
	FVector InHeadRelativeLocation)
{
	NpcId = InNpcId.IsNone() ? NpcId : InNpcId;
	QuestProviderId = InQuestProviderId;
	QuestFallbackId = InQuestFallbackId;
	QuestInteractionEventId = InQuestInteractionEventId;

	if (QuestInteractableComponent)
	{
		QuestInteractableComponent->SetObjectiveEventId(QuestInteractionEventId);
	}

	if (BodyMesh)
	{
		BodyMesh->SetRelativeScale3D(InBodyScale);
	}
	if (HeadMesh)
	{
		HeadMesh->SetRelativeLocation(InHeadRelativeLocation);
		HeadMesh->SetRelativeScale3D(InHeadScale);
	}
}

FName ATunaSweeperFacilityNpcActor::ResolveQuestId() const
{
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (const UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				FName ResolvedQuestId = NAME_None;
				if (QuestSubsystem->TryResolveQuestForProvider(QuestProviderId, QuestFallbackId, ResolvedQuestId))
				{
					return ResolvedQuestId;
				}
			}
		}
	}

	return QuestFallbackId;
}

void ATunaSweeperFacilityNpcActor::RefreshQuestNoticeVisibility()
{
	if (QuestMarkerComponent)
	{
		QuestMarkerComponent->SetVisibility(ShouldShowQuestNotice(), true);
	}
}

bool ATunaSweeperFacilityNpcActor::ShouldShowQuestNotice() const
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

ATunaSweeperSignalBotActor::ATunaSweeperSignalBotActor()
{
	ConfigureFacilityNpcDefaults(
		FName(TEXT("npc.signalbot")),
		FName(TEXT("provider.signalbot")),
		FName(TEXT("quest_signalbot_map_check")),
		FName(TEXT("interaction.signalbot.quest")),
		FVector(0.36f, 0.36f, 0.92f),
		FVector(0.72f, 0.36f, 0.28f),
		FVector(0.0f, 0.0f, 132.0f));
}

ATunaSweeperRicePotBotActor::ATunaSweeperRicePotBotActor()
{
	ConfigureFacilityNpcDefaults(
		FName(TEXT("npc.ricepotbot")),
		FName(TEXT("provider.ricepotbot")),
		FName(TEXT("quest_ricepotbot_supply_check")),
		FName(TEXT("interaction.ricepotbot.quest")),
		FVector(0.58f, 0.58f, 0.52f),
		FVector(0.62f, 0.52f, 0.34f),
		FVector(0.0f, 0.0f, 106.0f));
}
