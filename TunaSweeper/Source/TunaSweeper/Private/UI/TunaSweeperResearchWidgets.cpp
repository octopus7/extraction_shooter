#include "UI/TunaSweeperResearchWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperResearchSubsystem.h"
#include "UI/TunaSweeperUIStyle.h"

namespace TunaSweeperResearchUi
{
	FText Resolve(const UTunaSweeperGameInstance* GameInstance, const TCHAR* Key)
	{
		return GameInstance ? GameInstance->ResolveLocalizedText(FName(Key), FText::GetEmpty()) : FText::GetEmpty();
	}
}

void UTunaSweeperResearchNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIStyle::ApplyButton(NodeButton, TunaSweeperUIStyle::EButtonRole::Secondary);
	TunaSweeperUIStyle::ApplyLabel(ActionText);
	NodeButton->OnClicked.RemoveAll(this);
	NodeButton->OnClicked.AddDynamic(this, &UTunaSweeperResearchNodeWidget::HandleNodeClicked);
	RefreshFromSubsystem();
}

void UTunaSweeperResearchNodeWidget::RefreshFromSubsystem()
{
	const UTunaSweeperGameInstance* GameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const UTunaSweeperResearchSubsystem* Research = GameInstance ? GameInstance->GetSubsystem<UTunaSweeperResearchSubsystem>() : nullptr;
	FTunaSweeperResearchNodeView View;
	if (!Research || !Research->GetNodeView(NodeId, View)) return;
	NameText->SetText(View.DisplayName);
	SetToolTipText(View.Description);
	RequirementText->SetText(FText::Format(
		TunaSweeperResearchUi::Resolve(GameInstance, TEXT("ui.research.requirement")),
		View.RequiredAppliedNodeCount));
	ResearchProgressBar->SetPercent(View.Progress);
	RemainingTimeText->SetText(View.State == ETunaSweeperResearchNodeState::Researching
		? FText::Format(
			TunaSweeperResearchUi::Resolve(GameInstance, TEXT("ui.research.remaining_time")),
			FText::FromString(FString::Printf(TEXT("%02d"), View.RemainingSeconds / 60)),
			FText::FromString(FString::Printf(TEXT("%02d"), View.RemainingSeconds % 60)))
		: FText::GetEmpty());
	FText Action;
	switch (View.State)
	{
	case ETunaSweeperResearchNodeState::Locked: Action = TunaSweeperResearchUi::Resolve(GameInstance, TEXT("ui.research.action.locked")); break;
	case ETunaSweeperResearchNodeState::Available: Action = TunaSweeperResearchUi::Resolve(GameInstance, TEXT("ui.research.action.start")); break;
	case ETunaSweeperResearchNodeState::Researching: Action = TunaSweeperResearchUi::Resolve(GameInstance, TEXT("ui.research.action.researching")); break;
	case ETunaSweeperResearchNodeState::ReadyToClaim: Action = TunaSweeperResearchUi::Resolve(GameInstance, TEXT("ui.research.action.complete")); break;
	case ETunaSweeperResearchNodeState::Applied: Action = TunaSweeperResearchUi::Resolve(GameInstance, TEXT("ui.research.action.applied")); break;
	}
	ActionText->SetText(Action);
	const bool bActionable =
		View.State == ETunaSweeperResearchNodeState::Available ||
		View.State == ETunaSweeperResearchNodeState::ReadyToClaim;
	NodeButton->SetIsEnabled(bActionable);
	TunaSweeperUIStyle::ApplyButton(
		NodeButton,
		TunaSweeperUIStyle::EButtonRole::Secondary,
		View.State == ETunaSweeperResearchNodeState::Researching ||
			View.State == ETunaSweeperResearchNodeState::ReadyToClaim);
	TunaSweeperUIStyle::ApplyLabel(ActionText);
	ResearchProgressBar->SetVisibility(View.State == ETunaSweeperResearchNodeState::Researching ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UTunaSweeperResearchNodeWidget::HandleNodeClicked()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperResearchSubsystem* Research = GameInstance->GetSubsystem<UTunaSweeperResearchSubsystem>())
		{
			FTunaSweeperResearchNodeView View;
			if (Research->GetNodeView(NodeId, View))
			{
				if (View.State == ETunaSweeperResearchNodeState::Available) Research->TryStartResearch(NodeId);
				else if (View.State == ETunaSweeperResearchNodeState::ReadyToClaim) Research->TryClaimResearch(NodeId);
			}
		}
	}
	RefreshFromSubsystem();
}

void UTunaSweeperResearchTreeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	NodeWidgets.Reset();
	TArray<UWidget*> Widgets;
	WidgetTree->GetAllWidgets(Widgets);
	for (UWidget* Widget : Widgets)
	{
		if (UTunaSweeperResearchNodeWidget* NodeWidget = Cast<UTunaSweeperResearchNodeWidget>(Widget)) NodeWidgets.Add(NodeWidget);
	}
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperResearchSubsystem* Research = GameInstance->GetSubsystem<UTunaSweeperResearchSubsystem>())
		{
			Research->OnResearchStateChanged.RemoveAll(this);
			Research->OnResearchStateChanged.AddUObject(this, &UTunaSweeperResearchTreeWidget::RefreshAllNodes);
		}
	}
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperResearchTreeWidget::RefreshAllNodes);
	}
	RefreshAllNodes();
}

void UTunaSweeperResearchTreeWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperResearchSubsystem* Research = GameInstance->GetSubsystem<UTunaSweeperResearchSubsystem>()) Research->OnResearchStateChanged.RemoveAll(this);
	}
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UTunaSweeperResearchTreeWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshAccumulator += InDeltaTime;
	if (RefreshAccumulator >= 0.25f) { RefreshAccumulator = 0.0f; RefreshAllNodes(); }
}

void UTunaSweeperResearchTreeWidget::RefreshAllNodes()
{
	int32 AppliedCount = 0;
	int32 TotalCount = NodeWidgets.Num();
	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (const UTunaSweeperResearchSubsystem* Research = GameInstance->GetSubsystem<UTunaSweeperResearchSubsystem>()) AppliedCount = Research->GetAppliedNodeCount();
	}
	ResearchStatusText->SetText(FText::Format(
		TunaSweeperResearchUi::Resolve(GetGameInstance<UTunaSweeperGameInstance>(), TEXT("ui.research.status")),
		AppliedCount, TotalCount));
	for (const TWeakObjectPtr<UTunaSweeperResearchNodeWidget>& NodeWidget : NodeWidgets) if (NodeWidget.IsValid()) NodeWidget->RefreshFromSubsystem();
}
