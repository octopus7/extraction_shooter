#include "UI/TunaSweeperResearchWidgets.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Subsystem/TunaSweeperResearchSubsystem.h"

void UTunaSweeperResearchNodeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	NodeButton->OnClicked.RemoveAll(this);
	NodeButton->OnClicked.AddDynamic(this, &UTunaSweeperResearchNodeWidget::HandleNodeClicked);
	RefreshFromSubsystem();
}

void UTunaSweeperResearchNodeWidget::RefreshFromSubsystem()
{
	const UGameInstance* GameInstance = GetGameInstance();
	const UTunaSweeperResearchSubsystem* Research = GameInstance ? GameInstance->GetSubsystem<UTunaSweeperResearchSubsystem>() : nullptr;
	FTunaSweeperResearchNodeView View;
	if (!Research || !Research->GetNodeView(NodeId, View)) return;
	NameText->SetText(View.DisplayName);
	RequirementText->SetText(FText::Format(NSLOCTEXT("TunaSweeperResearch", "Requirement", "Unlocked: {0}"), View.RequiredAppliedNodeCount));
	ResearchProgressBar->SetPercent(View.Progress);
	RemainingTimeText->SetText(View.State == ETunaSweeperResearchNodeState::Researching
		? FText::FromString(FString::Printf(TEXT("%02d:%02d"), View.RemainingSeconds / 60, View.RemainingSeconds % 60))
		: FText::GetEmpty());
	FText Action;
	switch (View.State)
	{
	case ETunaSweeperResearchNodeState::Locked: Action = NSLOCTEXT("TunaSweeperResearch", "Locked", "Locked"); break;
	case ETunaSweeperResearchNodeState::Available: Action = NSLOCTEXT("TunaSweeperResearch", "Start", "Start Research"); break;
	case ETunaSweeperResearchNodeState::Researching: Action = NSLOCTEXT("TunaSweeperResearch", "Researching", "Researching"); break;
	case ETunaSweeperResearchNodeState::ReadyToClaim: Action = NSLOCTEXT("TunaSweeperResearch", "Complete", "Complete"); break;
	case ETunaSweeperResearchNodeState::Applied: Action = NSLOCTEXT("TunaSweeperResearch", "Applied", "Applied"); break;
	}
	ActionText->SetText(Action);
	NodeButton->SetIsEnabled(View.State == ETunaSweeperResearchNodeState::Available || View.State == ETunaSweeperResearchNodeState::ReadyToClaim);
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
	RefreshAllNodes();
}

void UTunaSweeperResearchTreeWidget::NativeDestruct()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperResearchSubsystem* Research = GameInstance->GetSubsystem<UTunaSweeperResearchSubsystem>()) Research->OnResearchStateChanged.RemoveAll(this);
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
	ResearchStatusText->SetText(FText::Format(NSLOCTEXT("TunaSweeperResearch", "Status", "Applied {0} / {1}"), AppliedCount, TotalCount));
	for (const TWeakObjectPtr<UTunaSweeperResearchNodeWidget>& NodeWidget : NodeWidgets) if (NodeWidget.IsValid()) NodeWidget->RefreshFromSubsystem();
}
