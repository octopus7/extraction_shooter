#include "UI/TunaSweeperQuestWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Styling/SlateBrush.h"
#include "Subsystem/TunaSweeperHousingSubsystem.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperUIFont.h"

namespace
{
	FSlateBrush MakeQuestBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		float Radius,
		const FLinearColor& OutlineColor,
		float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FSlateChildSize MakeQuestSlateChildSize(ESlateSizeRule::Type SizeRule, float Value = 1.0f)
	{
		FSlateChildSize ChildSize;
		ChildSize.SizeRule = SizeRule;
		ChildSize.Value = Value;
		return ChildSize;
	}
}

UTunaSweeperQuestWidget::UTunaSweeperQuestWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShowAvailableTab = true;
	ActiveFilter = EQuestListFilter::Available;
}

UTunaSweeperMenuQuestWidget::UTunaSweeperMenuQuestWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShowAvailableTab = false;
}

UTunaSweeperInteractionQuestWidget::UTunaSweeperInteractionQuestWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShowAvailableTab = true;
}

void UTunaSweeperQuestListEntryWidget::InitializeQuestEntry(
	FName InQuestId,
	const FText& InLabel,
	const FText& InStateLabel,
	bool bInSelected,
	FTunaSweeperQuestEntryClickedDelegate InClickedDelegate)
{
	QuestId = InQuestId;
	Label = InLabel;
	StateLabel = InStateLabel;
	bSelected = bInSelected;
	ClickedDelegate = InClickedDelegate;
	RefreshEntryView();
}

TSharedRef<SWidget> UTunaSweeperQuestListEntryWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildEntryWidget();
	return Super::RebuildWidget();
}

void UTunaSweeperQuestListEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildEntryWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (EntryButton)
	{
		EntryButton->OnClicked.RemoveDynamic(this, &UTunaSweeperQuestListEntryWidget::HandleEntryClicked);
		EntryButton->OnClicked.AddDynamic(this, &UTunaSweeperQuestListEntryWidget::HandleEntryClicked);
	}

	RefreshEntryView();
}

void UTunaSweeperQuestListEntryWidget::BuildEntryWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestEntryButton"));
	UVerticalBox* EntryStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestEntryStack"));
	EntryLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestEntryLabelText"));
	EntryStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestEntryStateText"));
	if (!EntryButton || !EntryStack || !EntryLabelText || !EntryStateText)
	{
		return;
	}

	WidgetTree->RootWidget = EntryButton;
	EntryButton->SetContent(EntryStack);
	EntryButton->SetRenderOpacity(0.9f);

	EntryLabelText->SetJustification(ETextJustify::Left);
	EntryLabelText->SetAutoWrapText(true);
	EntryLabelText->SetWrapTextAt(0.0f);
	TunaSweeperUIFont::ApplyFont(EntryLabelText, 17, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* LabelSlot = EntryStack->AddChildToVerticalBox(EntryLabelText))
	{
		LabelSlot->SetPadding(FMargin(10.0f, 8.0f, 10.0f, 2.0f));
	}

	EntryStateText->SetJustification(ETextJustify::Left);
	EntryStateText->SetAutoWrapText(false);
	TunaSweeperUIFont::ApplyFont(EntryStateText, 13);
	if (UVerticalBoxSlot* StateSlot = EntryStack->AddChildToVerticalBox(EntryStateText))
	{
		StateSlot->SetPadding(FMargin(10.0f, 0.0f, 10.0f, 8.0f));
	}
}

void UTunaSweeperQuestListEntryWidget::RefreshEntryView()
{
	if (EntryButton)
	{
		EntryButton->SetRenderOpacity(bSelected ? 1.0f : 0.82f);
		EntryButton->SetBackgroundColor(
			bSelected
				? FLinearColor(0.18f, 0.30f, 0.25f, 0.92f)
				: FLinearColor(0.08f, 0.09f, 0.095f, 0.82f));
	}

	if (EntryLabelText)
	{
		EntryLabelText->SetText(Label);
		EntryLabelText->SetColorAndOpacity(FSlateColor(
			bSelected ? FLinearColor(0.86f, 0.98f, 0.90f, 1.0f) : FLinearColor(0.84f, 0.89f, 0.90f, 1.0f)));
	}

	if (EntryStateText)
	{
		EntryStateText->SetText(StateLabel);
		EntryStateText->SetColorAndOpacity(FSlateColor(
			bSelected ? FLinearColor(0.72f, 0.88f, 0.76f, 1.0f) : FLinearColor(0.58f, 0.66f, 0.68f, 1.0f)));
	}
}

void UTunaSweeperQuestListEntryWidget::HandleEntryClicked()
{
	if (!QuestId.IsNone() && ClickedDelegate.IsBound())
	{
		ClickedDelegate.Execute(QuestId);
	}
}

void UTunaSweeperQuestWidget::InitializeQuest(FName InQuestId)
{
	ActiveFilter = GetDefaultFilter();
	QuestId = InQuestId;

	if (!QuestId.IsNone())
	{
		if (const UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
		{
			switch (QuestSubsystem->GetQuestState(QuestId))
			{
			case ETunaSweeperQuestState::Accepted:
			case ETunaSweeperQuestState::RewardAvailable:
				ActiveFilter = EQuestListFilter::InProgress;
				break;
			case ETunaSweeperQuestState::RewardCompleted:
				ActiveFilter = EQuestListFilter::RewardCompleted;
				break;
			case ETunaSweeperQuestState::Available:
			default:
				ActiveFilter = bShowAvailableTab ? EQuestListFilter::Available : GetDefaultFilter();
				break;
			}
		}
	}

	NormalizeActiveFilter();
	if (!QuestId.IsNone())
	{
		SetSavedSelectedQuestId(ActiveFilter, QuestId);
	}
	else
	{
		QuestId = GetSavedSelectedQuestId(ActiveFilter);
	}
	RefreshQuestView();
}

void UTunaSweeperQuestWidget::RefreshQuestView()
{
	BuildQuestWidget();
	NormalizeActiveFilter();
	UpdateTabButtonStates();

	UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return;
	}

	TArray<FTunaSweeperQuestDefinition> FilteredDefinitions;
	BuildFilteredQuestDefinitions(*QuestSubsystem, FilteredDefinitions);
	ApplySelectedQuest(FilteredDefinitions);
	RebuildQuestList(FilteredDefinitions);
	UpdateDetailView();
}

void UTunaSweeperQuestWidget::ResetQuestSelection()
{
	QuestId = NAME_None;
	AvailableSelectedQuestId = NAME_None;
	InProgressSelectedQuestId = NAME_None;
	CompletedSelectedQuestId = NAME_None;
	ActiveFilter = GetDefaultFilter();
}

UTunaSweeperQuestWidget::EQuestListFilter UTunaSweeperQuestWidget::GetDefaultFilter() const
{
	return bShowAvailableTab ? EQuestListFilter::Available : EQuestListFilter::InProgress;
}

void UTunaSweeperQuestWidget::NormalizeActiveFilter()
{
	if (!bShowAvailableTab && ActiveFilter == EQuestListFilter::Available)
	{
		ActiveFilter = EQuestListFilter::InProgress;
	}
}

TSharedRef<SWidget> UTunaSweeperQuestWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildQuestWidget();
	return Super::RebuildWidget();
}

void UTunaSweeperQuestWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildQuestWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
		QuestSubsystem->OnQuestProgressChanged.AddUObject(this, &UTunaSweeperQuestWidget::HandleQuestProgressChanged);
	}

	if (AvailableTabButton)
	{
		AvailableTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperQuestWidget::HandleAvailableTabClicked);
		AvailableTabButton->OnClicked.AddDynamic(this, &UTunaSweeperQuestWidget::HandleAvailableTabClicked);
	}

	if (InProgressTabButton)
	{
		InProgressTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperQuestWidget::HandleInProgressTabClicked);
		InProgressTabButton->OnClicked.AddDynamic(this, &UTunaSweeperQuestWidget::HandleInProgressTabClicked);
	}

	if (CompletedTabButton)
	{
		CompletedTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperQuestWidget::HandleCompletedTabClicked);
		CompletedTabButton->OnClicked.AddDynamic(this, &UTunaSweeperQuestWidget::HandleCompletedTabClicked);
	}

	if (PrimaryButton)
	{
		PrimaryButton->OnClicked.RemoveDynamic(this, &UTunaSweeperQuestWidget::HandlePrimaryButtonClicked);
		PrimaryButton->OnClicked.AddDynamic(this, &UTunaSweeperQuestWidget::HandlePrimaryButtonClicked);
	}

	RefreshQuestView();
}

void UTunaSweeperQuestWidget::NativeDestruct()
{
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
	}

	ResetQuestSelection();
	Super::NativeDestruct();
}

void UTunaSweeperQuestWidget::HandleAvailableTabClicked()
{
	if (!bShowAvailableTab)
	{
		return;
	}

	SetActiveFilter(EQuestListFilter::Available);
}

void UTunaSweeperQuestWidget::HandleInProgressTabClicked()
{
	SetActiveFilter(EQuestListFilter::InProgress);
}

void UTunaSweeperQuestWidget::HandleCompletedTabClicked()
{
	SetActiveFilter(EQuestListFilter::RewardCompleted);
}

void UTunaSweeperQuestWidget::HandlePrimaryButtonClicked()
{
	UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	const FName ActingQuestId = QuestId;
	if (!QuestSubsystem || ActingQuestId.IsNone())
	{
		return;
	}

	ATunaSweeperPlayerController* TunaPlayerController = GetOwningPlayer<ATunaSweeperPlayerController>();
	const ETunaSweeperQuestState State = QuestSubsystem->GetQuestState(ActingQuestId);
	if (State == ETunaSweeperQuestState::Available)
	{
		if (QuestSubsystem->AcceptQuest(ActingQuestId))
		{
			QuestId = ActingQuestId;
			ActiveFilter = EQuestListFilter::InProgress;
			SetSavedSelectedQuestId(ActiveFilter, QuestId);
			if (TunaPlayerController)
			{
				TunaPlayerController->PlayQuestPresentation(ActingQuestId, ETunaSweeperQuestPresentationTrigger::OnAccept);
			}
		}
	}
	else if (State == ETunaSweeperQuestState::RewardAvailable)
	{
		if (QuestSubsystem->ClaimQuestReward(ActingQuestId))
		{
			QuestId = ActingQuestId;
			ActiveFilter = EQuestListFilter::RewardCompleted;
			SetSavedSelectedQuestId(ActiveFilter, QuestId);
			if (TunaPlayerController)
			{
				TunaPlayerController->PlayQuestPresentation(ActingQuestId, ETunaSweeperQuestPresentationTrigger::OnRewardClaim);
			}
		}
	}

	RefreshQuestView();
}

bool UTunaSweeperQuestWidget::CacheBuiltQuestWidgets()
{
	if (!WidgetTree)
	{
		return false;
	}

	RootPanel = Cast<UBorder>(WidgetTree->FindWidget(TEXT("QuestRootPanel")));
	RootColumns = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("QuestRootColumns")));
	QuestListScrollBox = Cast<UScrollBox>(WidgetTree->FindWidget(TEXT("QuestListScrollBox")));
	DetailStack = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("QuestDetailStack")));
	AvailableTabButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("QuestAvailableTabButton")));
	AvailableTabText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestAvailableTabText")));
	InProgressTabButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("QuestInProgressTabButton")));
	InProgressTabText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestInProgressTabText")));
	CompletedTabButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("QuestCompletedTabButton")));
	CompletedTabText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestCompletedTabText")));
	DetailTitleText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestDetailTitleText")));
	DetailDescriptionText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestDetailDescriptionText")));
	DetailStateText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestDetailStateText")));
	DetailObjectiveHeaderText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestDetailObjectiveHeaderText")));
	DetailObjectiveText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestDetailObjectiveText")));
	DetailRewardHeaderText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestDetailRewardHeaderText")));
	DetailRewardText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestDetailRewardText")));
	DetailEmptyText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestDetailEmptyText")));
	PrimaryButton = Cast<UButton>(WidgetTree->FindWidget(TEXT("QuestPrimaryButton")));
	PrimaryButtonText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QuestPrimaryButtonText")));

	return RootPanel &&
		RootColumns &&
		QuestListScrollBox &&
		DetailStack &&
		InProgressTabButton &&
		InProgressTabText &&
		CompletedTabButton &&
		CompletedTabText &&
		DetailTitleText &&
		DetailDescriptionText &&
		DetailStateText &&
		DetailObjectiveHeaderText &&
		DetailObjectiveText &&
		DetailRewardHeaderText &&
		DetailRewardText &&
		DetailEmptyText &&
		PrimaryButton &&
		PrimaryButtonText &&
		(!bShowAvailableTab || (AvailableTabButton && AvailableTabText));
}

void UTunaSweeperQuestWidget::BuildQuestWidget()
{
	if (!WidgetTree)
	{
		return;
	}

	if (WidgetTree->RootWidget)
	{
		if (CacheBuiltQuestWidgets())
		{
			return;
		}

		WidgetTree->RemoveWidget(WidgetTree->RootWidget);
		WidgetTree->RootWidget = nullptr;
	}

	RootPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestRootPanel"));
	RootColumns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QuestRootColumns"));
	UBorder* DetailPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestDetailPanel"));
	UOverlay* DetailOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("QuestDetailOverlay"));
	DetailStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestDetailStack"));
	UHorizontalBox* DetailHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QuestDetailHeaderRow"));
	DetailTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDetailTitleText"));
	DetailStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDetailStateText"));
	DetailDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDetailDescriptionText"));
	UScrollBox* DetailScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("QuestDetailScrollBox"));
	UVerticalBox* DetailBodyStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestDetailBodyStack"));
	DetailObjectiveHeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDetailObjectiveHeaderText"));
	DetailObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDetailObjectiveText"));
	DetailRewardHeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDetailRewardHeaderText"));
	DetailRewardText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDetailRewardText"));
	DetailEmptyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDetailEmptyText"));
	PrimaryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestPrimaryButton"));
	PrimaryButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestPrimaryButtonText"));
	USizeBox* ListSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuestListSizeBox"));
	UBorder* ListPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestListPanel"));
	UVerticalBox* ListStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestListStack"));
	UTextBlock* ListTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestListTitleText"));
	UHorizontalBox* TabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("QuestTabRow"));
	AvailableTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestAvailableTabButton"));
	AvailableTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestAvailableTabText"));
	InProgressTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestInProgressTabButton"));
	InProgressTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestInProgressTabText"));
	CompletedTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestCompletedTabButton"));
	CompletedTabText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestCompletedTabText"));
	QuestListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("QuestListScrollBox"));

	if (!RootPanel ||
		!RootColumns ||
		!DetailPanel ||
		!DetailOverlay ||
		!DetailStack ||
		!DetailHeaderRow ||
		!DetailTitleText ||
		!DetailStateText ||
		!DetailDescriptionText ||
		!DetailScrollBox ||
		!DetailBodyStack ||
		!DetailObjectiveHeaderText ||
		!DetailObjectiveText ||
		!DetailRewardHeaderText ||
		!DetailRewardText ||
		!DetailEmptyText ||
		!PrimaryButton ||
		!PrimaryButtonText ||
		!ListSizeBox ||
		!ListPanel ||
		!ListStack ||
		!ListTitleText ||
		!TabRow ||
		!AvailableTabButton ||
		!AvailableTabText ||
		!InProgressTabButton ||
		!InProgressTabText ||
		!CompletedTabButton ||
		!CompletedTabText ||
		!QuestListScrollBox)
	{
		return;
	}

	WidgetTree->RootWidget = RootPanel;
	RootPanel->SetBrush(MakeQuestBoxBrush(
		FVector2D(1220.0f, 672.0f),
		FLinearColor(0.035f, 0.04f, 0.045f, 0.94f),
		6.0f,
		FLinearColor(0.42f, 0.48f, 0.50f, 0.56f),
		1.0f));
	RootPanel->SetPadding(FMargin(16.0f));
	RootPanel->SetContent(RootColumns);

	DetailPanel->SetBrush(MakeQuestBoxBrush(
		FVector2D(792.0f, 640.0f),
		FLinearColor(0.063f, 0.067f, 0.072f, 0.96f),
		4.0f,
		FLinearColor(0.27f, 0.31f, 0.32f, 0.62f),
		1.0f));
	DetailPanel->SetPadding(FMargin(22.0f, 18.0f));
	DetailPanel->SetContent(DetailOverlay);
	if (UOverlaySlot* DetailStackSlot = DetailOverlay->AddChildToOverlay(DetailStack))
	{
		DetailStackSlot->SetHorizontalAlignment(HAlign_Fill);
		DetailStackSlot->SetVerticalAlignment(VAlign_Fill);
	}

	DetailTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.98f, 0.98f, 1.0f)));
	DetailTitleText->SetAutoWrapText(true);
	DetailTitleText->SetWrapTextAt(0.0f);
	TunaSweeperUIFont::ApplyFont(DetailTitleText, 26, ETunaSweeperUIFontWeight::Bold);
	if (UHorizontalBoxSlot* TitleSlot = DetailHeaderRow->AddChildToHorizontalBox(DetailTitleText))
	{
		TitleSlot->SetSize(MakeQuestSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}

	DetailStateText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.88f, 0.76f, 1.0f)));
	DetailStateText->SetJustification(ETextJustify::Right);
	TunaSweeperUIFont::ApplyFont(DetailStateText, 16, ETunaSweeperUIFontWeight::Bold);
	if (UHorizontalBoxSlot* StateSlot = DetailHeaderRow->AddChildToHorizontalBox(DetailStateText))
	{
		StateSlot->SetSize(MakeQuestSlateChildSize(ESlateSizeRule::Automatic));
		StateSlot->SetVerticalAlignment(VAlign_Center);
		StateSlot->SetPadding(FMargin(16.0f, 0.0f, 0.0f, 0.0f));
	}

	if (UVerticalBoxSlot* HeaderSlot = DetailStack->AddChildToVerticalBox(DetailHeaderRow))
	{
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	DetailDescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.80f, 0.86f, 0.85f, 1.0f)));
	DetailDescriptionText->SetAutoWrapText(true);
	DetailDescriptionText->SetWrapTextAt(0.0f);
	DetailDescriptionText->SetLineHeightPercentage(1.12f);
	TunaSweeperUIFont::ApplyFont(DetailDescriptionText, 18);
	if (UVerticalBoxSlot* DescriptionSlot = DetailStack->AddChildToVerticalBox(DetailDescriptionText))
	{
		DescriptionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
	}

	DetailObjectiveHeaderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.96f, 0.94f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(DetailObjectiveHeaderText, 19, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* ObjectiveHeaderSlot = DetailBodyStack->AddChildToVerticalBox(DetailObjectiveHeaderText))
	{
		ObjectiveHeaderSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 8.0f));
	}

	DetailObjectiveText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.88f, 0.86f, 1.0f)));
	DetailObjectiveText->SetAutoWrapText(true);
	DetailObjectiveText->SetWrapTextAt(0.0f);
	DetailObjectiveText->SetLineHeightPercentage(1.12f);
	TunaSweeperUIFont::ApplyFont(DetailObjectiveText, 17);
	if (UVerticalBoxSlot* ObjectiveSlot = DetailBodyStack->AddChildToVerticalBox(DetailObjectiveText))
	{
		ObjectiveSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
	}

	DetailRewardHeaderText->SetColorAndOpacity(FSlateColor(FLinearColor(0.93f, 0.96f, 0.94f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(DetailRewardHeaderText, 19, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* RewardHeaderSlot = DetailBodyStack->AddChildToVerticalBox(DetailRewardHeaderText))
	{
		RewardHeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	DetailRewardText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.88f, 0.86f, 1.0f)));
	DetailRewardText->SetAutoWrapText(true);
	DetailRewardText->SetWrapTextAt(0.0f);
	TunaSweeperUIFont::ApplyFont(DetailRewardText, 17);
	DetailBodyStack->AddChildToVerticalBox(DetailRewardText);

	DetailScrollBox->AddChild(DetailBodyStack);
	if (UVerticalBoxSlot* ScrollSlot = DetailStack->AddChildToVerticalBox(DetailScrollBox))
	{
		ScrollSlot->SetSize(MakeQuestSlateChildSize(ESlateSizeRule::Fill));
	}

	PrimaryButton->SetContent(PrimaryButtonText);
	PrimaryButton->SetBackgroundColor(FLinearColor(0.55f, 0.82f, 0.98f, 1.0f));
	PrimaryButtonText->SetJustification(ETextJustify::Center);
	PrimaryButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.03f, 0.05f, 0.06f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(PrimaryButtonText, 18, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* PrimaryButtonSlot = DetailStack->AddChildToVerticalBox(PrimaryButton))
	{
		PrimaryButtonSlot->SetHorizontalAlignment(HAlign_Right);
		PrimaryButtonSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
	}

	DetailEmptyText->SetJustification(ETextJustify::Center);
	DetailEmptyText->SetAutoWrapText(true);
	DetailEmptyText->SetWrapTextAt(0.0f);
	DetailEmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.74f, 0.74f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(DetailEmptyText, 22, ETunaSweeperUIFontWeight::Bold);
	DetailEmptyText->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* EmptySlot = DetailOverlay->AddChildToOverlay(DetailEmptyText))
	{
		EmptySlot->SetHorizontalAlignment(HAlign_Center);
		EmptySlot->SetVerticalAlignment(VAlign_Center);
		EmptySlot->SetPadding(FMargin(24.0f));
	}

	ListSizeBox->SetWidthOverride(380.0f);
	ListPanel->SetBrush(MakeQuestBoxBrush(
		FVector2D(380.0f, 640.0f),
		FLinearColor(0.07f, 0.078f, 0.085f, 0.96f),
		4.0f,
		FLinearColor(0.24f, 0.30f, 0.33f, 0.65f),
		1.0f));
	ListPanel->SetPadding(FMargin(12.0f));
	ListPanel->SetContent(ListStack);
	ListSizeBox->SetContent(ListPanel);
	if (UHorizontalBoxSlot* ListColumnSlot = RootColumns->AddChildToHorizontalBox(ListSizeBox))
	{
		ListColumnSlot->SetSize(MakeQuestSlateChildSize(ESlateSizeRule::Automatic));
		ListColumnSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
	}

	if (UHorizontalBoxSlot* DetailColumnSlot = RootColumns->AddChildToHorizontalBox(DetailPanel))
	{
		DetailColumnSlot->SetSize(MakeQuestSlateChildSize(ESlateSizeRule::Fill));
	}

	ListTitleText->SetText(GetQuestText(FName(TEXT("quest.ui.list_title"))));
	ListTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.96f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(ListTitleText, 20, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* ListTitleSlot = ListStack->AddChildToVerticalBox(ListTitleText))
	{
		ListTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	auto ConfigureTab = [this, TabRow](UButton* Button, UTextBlock* Text)
	{
		Button->SetContent(Text);
		Text->SetJustification(ETextJustify::Center);
		Text->SetAutoWrapText(false);
		TunaSweeperUIFont::ApplyFont(Text, 13, ETunaSweeperUIFontWeight::Bold);
		if (UHorizontalBoxSlot* TabSlot = TabRow->AddChildToHorizontalBox(Button))
		{
			TabSlot->SetSize(MakeQuestSlateChildSize(ESlateSizeRule::Fill));
			TabSlot->SetPadding(FMargin(0.0f, 0.0f, 6.0f, 0.0f));
		}
	};

	if (bShowAvailableTab)
	{
		ConfigureTab(AvailableTabButton, AvailableTabText);
	}
	else
	{
		AvailableTabButton->SetVisibility(ESlateVisibility::Collapsed);
	}
	ConfigureTab(InProgressTabButton, InProgressTabText);
	ConfigureTab(CompletedTabButton, CompletedTabText);
	if (UVerticalBoxSlot* TabRowSlot = ListStack->AddChildToVerticalBox(TabRow))
	{
		TabRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	if (UVerticalBoxSlot* ScrollSlot = ListStack->AddChildToVerticalBox(QuestListScrollBox))
	{
		ScrollSlot->SetSize(MakeQuestSlateChildSize(ESlateSizeRule::Fill));
	}
}

void UTunaSweeperQuestWidget::RebuildQuestList(const TArray<FTunaSweeperQuestDefinition>& QuestDefinitions)
{
	if (!QuestListScrollBox)
	{
		return;
	}

	QuestListScrollBox->ClearChildren();
	if (QuestDefinitions.Num() <= 0)
	{
		UTextBlock* EmptyText = WidgetTree
			? WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestListEmptyText"))
			: nullptr;
		if (EmptyText)
		{
			EmptyText->SetText(GetEmptyListText());
			EmptyText->SetAutoWrapText(true);
			EmptyText->SetWrapTextAt(0.0f);
			EmptyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.68f, 0.68f, 1.0f)));
			TunaSweeperUIFont::ApplyFont(EmptyText, 16);
			QuestListScrollBox->AddChild(EmptyText);
		}
		return;
	}

	for (const FTunaSweeperQuestDefinition& QuestDefinition : QuestDefinitions)
	{
		UTunaSweeperQuestListEntryWidget* EntryWidget = CreateWidget<UTunaSweeperQuestListEntryWidget>(
			GetOwningPlayer(),
			UTunaSweeperQuestListEntryWidget::StaticClass());
		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->InitializeQuestEntry(
			QuestDefinition.QuestId,
			QuestDefinition.Title,
			GetStateText(QuestDefinition.QuestId),
			QuestDefinition.QuestId == QuestId,
			FTunaSweeperQuestEntryClickedDelegate::CreateUObject(this, &UTunaSweeperQuestWidget::SetSelectedQuestId));
		QuestListScrollBox->AddChild(EntryWidget);
	}
}

void UTunaSweeperQuestWidget::ApplySelectedQuest(const TArray<FTunaSweeperQuestDefinition>& QuestDefinitions)
{
	const FName SavedQuestId = GetSavedSelectedQuestId(ActiveFilter);
	for (const FTunaSweeperQuestDefinition& QuestDefinition : QuestDefinitions)
	{
		if (QuestDefinition.QuestId == SavedQuestId)
		{
			QuestId = SavedQuestId;
			return;
		}
	}

	QuestId = QuestDefinitions.Num() > 0 ? QuestDefinitions[0].QuestId : NAME_None;
	SetSavedSelectedQuestId(ActiveFilter, QuestId);
}

void UTunaSweeperQuestWidget::SetSelectedQuestId(FName InQuestId)
{
	QuestId = InQuestId;
	SetSavedSelectedQuestId(ActiveFilter, QuestId);
	RefreshQuestView();
}

void UTunaSweeperQuestWidget::SetActiveFilter(EQuestListFilter InFilter)
{
	SetSavedSelectedQuestId(ActiveFilter, QuestId);
	ActiveFilter = InFilter;
	NormalizeActiveFilter();
	QuestId = GetSavedSelectedQuestId(ActiveFilter);
	RefreshQuestView();
}

FName UTunaSweeperQuestWidget::GetSavedSelectedQuestId(EQuestListFilter Filter) const
{
	switch (Filter)
	{
	case EQuestListFilter::Available:
		return AvailableSelectedQuestId;
	case EQuestListFilter::InProgress:
		return InProgressSelectedQuestId;
	case EQuestListFilter::RewardCompleted:
		return CompletedSelectedQuestId;
	default:
		return NAME_None;
	}
}

void UTunaSweeperQuestWidget::SetSavedSelectedQuestId(EQuestListFilter Filter, FName InQuestId)
{
	switch (Filter)
	{
	case EQuestListFilter::Available:
		AvailableSelectedQuestId = InQuestId;
		break;
	case EQuestListFilter::InProgress:
		InProgressSelectedQuestId = InQuestId;
		break;
	case EQuestListFilter::RewardCompleted:
		CompletedSelectedQuestId = InQuestId;
		break;
	default:
		break;
	}
}

void UTunaSweeperQuestWidget::HandleQuestProgressChanged()
{
	RefreshQuestView();
}

void UTunaSweeperQuestWidget::UpdateTabButtonStates()
{
	struct FTabInfo
	{
		UButton* Button = nullptr;
		UTextBlock* Text = nullptr;
		EQuestListFilter Filter = EQuestListFilter::Available;
		FName TextKey = NAME_None;
	};

	const FTabInfo Tabs[] = {
		{ AvailableTabButton, AvailableTabText, EQuestListFilter::Available, FName(TEXT("quest.ui.tab.available")) },
		{ InProgressTabButton, InProgressTabText, EQuestListFilter::InProgress, FName(TEXT("quest.ui.tab.in_progress")) },
		{ CompletedTabButton, CompletedTabText, EQuestListFilter::RewardCompleted, FName(TEXT("quest.ui.tab.completed")) }
	};

	for (const FTabInfo& Tab : Tabs)
	{
		if (Tab.Filter == EQuestListFilter::Available && !bShowAvailableTab)
		{
			if (Tab.Button)
			{
				Tab.Button->SetVisibility(ESlateVisibility::Collapsed);
			}
			continue;
		}

		const bool bActive = ActiveFilter == Tab.Filter;
		if (Tab.Button)
		{
			Tab.Button->SetVisibility(ESlateVisibility::Visible);
			Tab.Button->SetBackgroundColor(
				bActive
					? FLinearColor(0.44f, 0.76f, 0.88f, 1.0f)
					: FLinearColor(0.13f, 0.15f, 0.16f, 0.92f));
		}
		if (Tab.Text)
		{
			Tab.Text->SetText(GetQuestText(Tab.TextKey));
			Tab.Text->SetColorAndOpacity(FSlateColor(
				bActive
					? FLinearColor(0.03f, 0.05f, 0.06f, 1.0f)
					: FLinearColor(0.76f, 0.82f, 0.84f, 1.0f)));
		}
	}
}

void UTunaSweeperQuestWidget::UpdateDetailView()
{
	UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;

	FTunaSweeperQuestDefinition QuestDefinition;
	const bool bHasQuest =
		QuestSubsystem &&
		!QuestId.IsNone() &&
		QuestSubsystem->TryGetQuestDefinition(QuestId, QuestDefinition);

	if (DetailStack)
	{
		DetailStack->SetIsEnabled(bHasQuest);
		DetailStack->SetVisibility(bHasQuest ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DetailEmptyText)
	{
		DetailEmptyText->SetText(GetQuestText(
			FName(TEXT("quest.ui.empty.detail")),
			FText::FromString(TEXT("퀘스트가 없습니다"))));
		DetailEmptyText->SetVisibility(bHasQuest ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (DetailTitleText)
	{
		DetailTitleText->SetText(bHasQuest ? QuestDefinition.Title : FText::GetEmpty());
	}
	if (DetailDescriptionText)
	{
		DetailDescriptionText->SetText(bHasQuest ? QuestDefinition.Description : FText::GetEmpty());
	}
	if (DetailStateText)
	{
		DetailStateText->SetText(bHasQuest ? GetStateText(QuestId) : FText::GetEmpty());
	}
	if (DetailObjectiveHeaderText)
	{
		DetailObjectiveHeaderText->SetText(bHasQuest ? GetQuestText(FName(TEXT("quest.ui.objectives"))) : FText::GetEmpty());
	}
	if (DetailObjectiveText)
	{
		DetailObjectiveText->SetText(bHasQuest && QuestSubsystem ? BuildObjectiveText(*QuestSubsystem, QuestId) : FText::GetEmpty());
	}
	if (DetailRewardHeaderText)
	{
		DetailRewardHeaderText->SetText(bHasQuest ? GetQuestText(FName(TEXT("quest.ui.rewards"))) : FText::GetEmpty());
	}
	if (DetailRewardText)
	{
		DetailRewardText->SetText(bHasQuest && QuestSubsystem ? BuildRewardText(*QuestSubsystem, QuestId) : FText::GetEmpty());
	}
	if (PrimaryButtonText)
	{
		PrimaryButtonText->SetText(bHasQuest ? GetPrimaryButtonText(QuestId) : FText::GetEmpty());
	}
	if (PrimaryButton)
	{
		PrimaryButton->SetIsEnabled(bHasQuest && IsPrimaryButtonEnabled(QuestId));
		PrimaryButton->SetVisibility(bHasQuest ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		PrimaryButton->SetRenderOpacity(1.0f);
	}
}

void UTunaSweeperQuestWidget::BuildFilteredQuestDefinitions(
	const UTunaSweeperQuestSubsystem& QuestSubsystem,
	TArray<FTunaSweeperQuestDefinition>& OutQuestDefinitions) const
{
	OutQuestDefinitions.Reset();

	TArray<FTunaSweeperQuestDefinition> AllDefinitions;
	if (!QuestSubsystem.GetAllQuestDefinitions(AllDefinitions))
	{
		return;
	}

	for (const FTunaSweeperQuestDefinition& QuestDefinition : AllDefinitions)
	{
		if (IsQuestVisibleInActiveFilter(QuestSubsystem, QuestDefinition))
		{
			OutQuestDefinitions.Add(QuestDefinition);
		}
	}
}

bool UTunaSweeperQuestWidget::IsQuestVisibleInActiveFilter(
	const UTunaSweeperQuestSubsystem& QuestSubsystem,
	const FTunaSweeperQuestDefinition& QuestDefinition) const
{
	const ETunaSweeperQuestState State = QuestSubsystem.GetQuestState(QuestDefinition.QuestId);
	switch (ActiveFilter)
	{
	case EQuestListFilter::Available:
		return bShowAvailableTab &&
			State == ETunaSweeperQuestState::Available &&
			QuestSubsystem.CanAcceptQuest(QuestDefinition.QuestId);
	case EQuestListFilter::InProgress:
		return State == ETunaSweeperQuestState::Accepted || State == ETunaSweeperQuestState::RewardAvailable;
	case EQuestListFilter::RewardCompleted:
		return State == ETunaSweeperQuestState::RewardCompleted;
	default:
		return false;
	}
}

FText UTunaSweeperQuestWidget::GetQuestText(FName StringKey, const FText& FallbackText) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (const UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
	{
		FText Text;
		if (QuestSubsystem->TryGetQuestTextByKey(StringKey, TunaGameInstance->GetCurrentTextLanguage(), Text))
		{
			return Text;
		}
	}

	return FallbackText.IsEmpty() ? FText::FromString(StringKey.ToString()) : FallbackText;
}

FText UTunaSweeperQuestWidget::GetStateText(FName InQuestId) const
{
	const UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return FText::GetEmpty();
	}

	switch (QuestSubsystem->GetQuestState(InQuestId))
	{
	case ETunaSweeperQuestState::Available:
		return GetQuestText(QuestSubsystem->CanAcceptQuest(InQuestId)
			? FName(TEXT("quest.ui.state.available"))
			: FName(TEXT("quest.ui.state.locked")));
	case ETunaSweeperQuestState::Accepted:
		return GetQuestText(FName(TEXT("quest.ui.state.accepted")));
	case ETunaSweeperQuestState::RewardAvailable:
		return GetQuestText(FName(TEXT("quest.ui.state.reward_available")));
	case ETunaSweeperQuestState::RewardCompleted:
		return GetQuestText(FName(TEXT("quest.ui.state.reward_completed")));
	default:
		return FText::GetEmpty();
	}
}

FText UTunaSweeperQuestWidget::GetPrimaryButtonText(FName InQuestId) const
{
	const UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return FText::GetEmpty();
	}

	switch (QuestSubsystem->GetQuestState(InQuestId))
	{
	case ETunaSweeperQuestState::Available:
		return GetQuestText(QuestSubsystem->CanAcceptQuest(InQuestId)
			? FName(TEXT("quest.ui.button.accept"))
			: FName(TEXT("quest.ui.button.locked")));
	case ETunaSweeperQuestState::Accepted:
		return GetQuestText(FName(TEXT("quest.ui.button.in_progress")));
	case ETunaSweeperQuestState::RewardAvailable:
		return GetQuestText(FName(TEXT("quest.ui.button.claim_reward")));
	case ETunaSweeperQuestState::RewardCompleted:
		return GetQuestText(FName(TEXT("quest.ui.button.completed")));
	default:
		return FText::GetEmpty();
	}
}

FText UTunaSweeperQuestWidget::GetEmptyListText() const
{
	switch (ActiveFilter)
	{
	case EQuestListFilter::Available:
		return GetQuestText(FName(TEXT("quest.ui.empty.available")));
	case EQuestListFilter::InProgress:
		return GetQuestText(FName(TEXT("quest.ui.empty.in_progress")));
	case EQuestListFilter::RewardCompleted:
		return GetQuestText(FName(TEXT("quest.ui.empty.completed")));
	default:
		return FText::GetEmpty();
	}
}

FText UTunaSweeperQuestWidget::BuildObjectiveText(
	const UTunaSweeperQuestSubsystem& QuestSubsystem,
	FName InQuestId) const
{
	TArray<FTunaSweeperObjectiveProgressView> ObjectiveProgress;
	if (!QuestSubsystem.GetQuestObjectiveProgress(InQuestId, ObjectiveProgress) || ObjectiveProgress.Num() <= 0)
	{
		return GetQuestText(FName(TEXT("quest.ui.no_reward")));
	}

	TArray<FString> ObjectiveLines;
	for (const FTunaSweeperObjectiveProgressView& Progress : ObjectiveProgress)
	{
		ObjectiveLines.Add(FString::Printf(
			TEXT("- %s (%d/%d)"),
			*Progress.Text.ToString(),
			FMath::Clamp(Progress.CurrentCount, 0, FMath::Max(1, Progress.RequiredCount)),
			FMath::Max(1, Progress.RequiredCount)));
	}

	return FText::FromString(FString::Join(ObjectiveLines, LINE_TERMINATOR));
}

FText UTunaSweeperQuestWidget::BuildRewardText(
	const UTunaSweeperQuestSubsystem& QuestSubsystem,
	FName InQuestId) const
{
	FTunaSweeperQuestDefinition QuestDefinition;
	if (!QuestSubsystem.TryGetQuestDefinition(InQuestId, QuestDefinition))
	{
		return FText::GetEmpty();
	}

	TArray<FString> RewardParts;
	if (QuestDefinition.Rewards.Coins > 0)
	{
		RewardParts.Add(FString::Printf(
			TEXT("%s %d"),
			*GetQuestText(FName(TEXT("quest.ui.coins"))).ToString(),
			QuestDefinition.Rewards.Coins));
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;
	for (const FTunaSweeperItemStack& ItemReward : QuestDefinition.Rewards.Items)
	{
		if (ItemReward.ItemId == INDEX_NONE || ItemReward.Quantity <= 0)
		{
			continue;
		}

		FText ItemName;
		if (!ItemDataSubsystem ||
			!ItemDataSubsystem->TryGetItemNameText(ItemReward.ItemId, Language, ItemName))
		{
			ItemName = FText::FromString(FString::Printf(
				TEXT("%s %d"),
				*GetQuestText(FName(TEXT("quest.ui.item_fallback"))).ToString(),
				ItemReward.ItemId));
		}

		RewardParts.Add(FString::Printf(TEXT("%s x%d"), *ItemName.ToString(), FMath::Max(1, ItemReward.Quantity)));
	}

	UTunaSweeperHousingSubsystem* HousingSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	for (const FName& FacilityId : QuestDefinition.Rewards.HousingFacilityUnlocks)
	{
		if (FacilityId.IsNone())
		{
			continue;
		}

		FText FacilityName = FText::FromName(FacilityId);
		FTunaSweeperHousingFacilityDefinition FacilityDefinition;
		if (HousingSubsystem && HousingSubsystem->TryGetFacilityDefinition(FacilityId, FacilityDefinition))
		{
			FacilityName = TunaGameInstance && !FacilityDefinition.DisplayNameStringKey.IsNone()
				? TunaGameInstance->ResolveLocalizedText(FacilityDefinition.DisplayNameStringKey, FacilityDefinition.FallbackDisplayName)
				: FacilityDefinition.FallbackDisplayName;
		}

		RewardParts.Add(FString::Printf(
			TEXT("%s: %s"),
			*GetQuestText(FName(TEXT("quest.ui.facility_unlock")), FText::FromString(TEXT("Facility Unlock"))).ToString(),
			*FacilityName.ToString()));
	}

	return RewardParts.Num() > 0
		? FText::FromString(FString::Join(RewardParts, TEXT(" / ")))
		: GetQuestText(FName(TEXT("quest.ui.no_reward")));
}

bool UTunaSweeperQuestWidget::IsPrimaryButtonEnabled(FName InQuestId) const
{
	const UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem)
	{
		return false;
	}

	return QuestSubsystem->CanAcceptQuest(InQuestId) ||
		QuestSubsystem->GetQuestState(InQuestId) == ETunaSweeperQuestState::RewardAvailable;
}
