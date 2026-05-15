#include "UI/TunaSweeperQuestWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

void UTunaSweeperQuestWidget::InitializeQuest(FName InQuestId)
{
	QuestId = InQuestId;
	RefreshQuestView();
}

void UTunaSweeperQuestWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (QuestPrimaryButton)
	{
		QuestPrimaryButton->OnClicked.RemoveDynamic(this, &UTunaSweeperQuestWidget::HandlePrimaryButtonClicked);
		QuestPrimaryButton->OnClicked.AddDynamic(this, &UTunaSweeperQuestWidget::HandlePrimaryButtonClicked);
	}

	if (QuestCloseButton)
	{
		QuestCloseButton->OnClicked.RemoveDynamic(this, &UTunaSweeperQuestWidget::HandleCloseButtonClicked);
		QuestCloseButton->OnClicked.AddDynamic(this, &UTunaSweeperQuestWidget::HandleCloseButtonClicked);
	}

	if (QuestId.IsNone())
	{
		QuestId = UTunaSweeperQuestSubsystem::GetFirstOutingQuestId();
	}

	RefreshQuestView();
}

void UTunaSweeperQuestWidget::HandlePrimaryButtonClicked()
{
	UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return;
	}

	switch (QuestSubsystem->GetQuestState(QuestId))
	{
	case ETunaSweeperQuestState::Available:
		QuestSubsystem->AcceptQuest(QuestId);
		break;
	case ETunaSweeperQuestState::RewardAvailable:
		QuestSubsystem->ClaimQuestReward(QuestId);
		break;
	default:
		break;
	}

	RefreshQuestView();
}

void UTunaSweeperQuestWidget::HandleCloseButtonClicked()
{
	RemoveFromParent();

	if (ATunaSweeperPlayerController* TunaPlayerController = GetOwningPlayer<ATunaSweeperPlayerController>())
	{
		TunaPlayerController->ApplyDefaultGameInputMode();
	}
}

void UTunaSweeperQuestWidget::RefreshQuestView()
{
	UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return;
	}

	FTunaSweeperQuestDefinition QuestDefinition;
	if (!QuestSubsystem->TryGetQuestDefinition(QuestId, QuestDefinition))
	{
		return;
	}

	if (QuestTitleText)
	{
		QuestTitleText->SetText(QuestDefinition.Title);
	}

	if (QuestDescriptionText)
	{
		QuestDescriptionText->SetText(QuestDefinition.Description);
	}

	if (QuestObjectiveText)
	{
		QuestObjectiveText->SetText(FText::Format(
			FText::FromString(TEXT("\uBAA9\uD45C: {0}")),
			QuestDefinition.ObjectiveText));
	}

	if (QuestRewardText)
	{
		QuestRewardText->SetText(FText::Format(
			FText::FromString(TEXT("\uBCF4\uC0C1: \uCF54\uC778 {0}")),
			FText::AsNumber(QuestDefinition.CoinReward)));
	}

	if (QuestStateText)
	{
		QuestStateText->SetText(GetStateText());
	}

	if (QuestPrimaryButtonText)
	{
		QuestPrimaryButtonText->SetText(GetPrimaryButtonText());
	}

	if (QuestPrimaryButton)
	{
		QuestPrimaryButton->SetIsEnabled(IsPrimaryButtonEnabled());
	}
}

FText UTunaSweeperQuestWidget::GetStateText() const
{
	const UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return FText::GetEmpty();
	}

	switch (QuestSubsystem->GetQuestState(QuestId))
	{
	case ETunaSweeperQuestState::Available:
		return FText::FromString(TEXT("\uC0C1\uD0DC: \uBC1B\uAE30 \uAC00\uB2A5"));
	case ETunaSweeperQuestState::Accepted:
		return FText::FromString(TEXT("\uC0C1\uD0DC: \uC218\uB77D\uB428"));
	case ETunaSweeperQuestState::RewardAvailable:
		return FText::FromString(TEXT("\uC0C1\uD0DC: \uBCF4\uC0C1 \uAC00\uB2A5"));
	case ETunaSweeperQuestState::RewardCompleted:
		return FText::FromString(TEXT("\uC0C1\uD0DC: \uBCF4\uC0C1 \uC644\uB8CC"));
	default:
		return FText::GetEmpty();
	}
}

FText UTunaSweeperQuestWidget::GetPrimaryButtonText() const
{
	const UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return FText::GetEmpty();
	}

	switch (QuestSubsystem->GetQuestState(QuestId))
	{
	case ETunaSweeperQuestState::Available:
		return FText::FromString(TEXT("\uC218\uB77D"));
	case ETunaSweeperQuestState::Accepted:
		return FText::FromString(TEXT("\uC9C4\uD589 \uC911"));
	case ETunaSweeperQuestState::RewardAvailable:
		return FText::FromString(TEXT("\uBCF4\uC0C1 \uBC1B\uAE30"));
	case ETunaSweeperQuestState::RewardCompleted:
		return FText::FromString(TEXT("\uC644\uB8CC"));
	default:
		return FText::GetEmpty();
	}
}

bool UTunaSweeperQuestWidget::IsPrimaryButtonEnabled() const
{
	const UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return false;
	}

	const ETunaSweeperQuestState State = QuestSubsystem->GetQuestState(QuestId);
	return State == ETunaSweeperQuestState::Available || State == ETunaSweeperQuestState::RewardAvailable;
}
