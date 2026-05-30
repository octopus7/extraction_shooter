#include "Character/TunaSweeperQuestNpcActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperQuestNoticeWidget.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperQuestNpcActor::ATunaSweeperQuestNpcActor()
{
	QuestId = UTunaSweeperQuestSubsystem::GetFirstOutingQuestId();
	ProviderId = UTunaSweeperQuestSubsystem::GetInstructorProviderId();
	NpcDisplayName = FText::FromString(TEXT("\uAD50\uAD00"));

	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(FVector(0.7f, 0.7f, 1.8f));

		static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
		if (CylinderMesh.Succeeded())
		{
			VisualMesh->SetStaticMesh(CylinderMesh.Object);
		}
	}

	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionTypeAndDisplayName(
			ETunaSweeperInteractionType::Quest,
			FText::FromString(TEXT("\uD018\uC2A4\uD2B8")));
	}

	QuestNoticeWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("QuestNoticeWidget"));
	if (QuestNoticeWidgetComponent)
	{
		QuestNoticeWidgetComponent->SetupAttachment(RootComponent);
		QuestNoticeWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 235.0f));
		QuestNoticeWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
		QuestNoticeWidgetComponent->SetWidgetClass(UTunaSweeperQuestNoticeWidget::StaticClass());
		QuestNoticeWidgetComponent->SetDrawSize(FVector2D(56.0f, 56.0f));
		QuestNoticeWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
		QuestNoticeWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		QuestNoticeWidgetComponent->SetVisibility(false);
		QuestNoticeWidgetComponent->SetHiddenInGame(false);
	}
}

void ATunaSweeperQuestNpcActor::BeginPlay()
{
	Super::BeginPlay();

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
				QuestSubsystem->OnQuestProgressChanged.AddUObject(this, &ATunaSweeperQuestNpcActor::RefreshQuestNoticeVisibility);
			}
		}
	}

	RefreshQuestNoticeVisibility();
}

void ATunaSweeperQuestNpcActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

FName ATunaSweeperQuestNpcActor::ResolveQuestId() const
{
	const FName EffectiveProviderId = ProviderId.IsNone()
		? UTunaSweeperQuestSubsystem::GetInstructorProviderId()
		: ProviderId;

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (const UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
			{
				FName ResolvedQuestId = NAME_None;
				if (QuestSubsystem->TryResolveQuestForProvider(EffectiveProviderId, QuestId, ResolvedQuestId))
				{
					return ResolvedQuestId;
				}

				return NAME_None;
			}
		}
	}

	return QuestId;
}

void ATunaSweeperQuestNpcActor::ConfigureQuestNpcDefaults(
	FName InQuestId,
	const FText& InNpcDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	FName InProviderId)
{
	Modify();
	QuestId = InQuestId;
	ProviderId = InProviderId.IsNone() ? UTunaSweeperQuestSubsystem::GetInstructorProviderId() : InProviderId;
	NpcDisplayName = InNpcDisplayName;
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::Quest,
		FText::FromString(TEXT("\uD018\uC2A4\uD2B8")),
		InMarkerWidgetClass);
}

void ATunaSweeperQuestNpcActor::RefreshQuestNoticeVisibility()
{
	if (QuestNoticeWidgetComponent)
	{
		QuestNoticeWidgetComponent->SetVisibility(ShouldShowQuestNotice());
	}
}

bool ATunaSweeperQuestNpcActor::ShouldShowQuestNotice() const
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
