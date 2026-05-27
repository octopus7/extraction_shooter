#include "UI/TunaSweeperMemoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Styling/SlateBrush.h"
#include "Subsystem/TunaSweeperMemoSubsystem.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"

namespace
{
	using TunaSweeperUiText::ResolveUiText;

	FSlateBrush MakeMemoBoxBrush(
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

	FSlateChildSize MakeSlateChildSize(ESlateSizeRule::Type SizeRule, float Value = 1.0f)
	{
		FSlateChildSize ChildSize;
		ChildSize.SizeRule = SizeRule;
		ChildSize.Value = Value;
		return ChildSize;
	}
}

void UTunaSweeperMemoListEntryWidget::InitializeMemoEntry(
	int32 InMemoId,
	const FText& InLabel,
	bool bInAcquired,
	bool bInSelected,
	FTunaSweeperMemoEntryClickedDelegate InClickedDelegate)
{
	MemoId = InMemoId;
	Label = InLabel;
	bAcquired = bInAcquired;
	bSelected = bInSelected;
	ClickedDelegate = InClickedDelegate;
	RefreshEntryView();
}

TSharedRef<SWidget> UTunaSweeperMemoListEntryWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildEntryWidget();
	return Super::RebuildWidget();
}

void UTunaSweeperMemoListEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildEntryWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (EntryButton)
	{
		EntryButton->OnClicked.RemoveDynamic(this, &UTunaSweeperMemoListEntryWidget::HandleEntryClicked);
		EntryButton->OnClicked.AddDynamic(this, &UTunaSweeperMemoListEntryWidget::HandleEntryClicked);
	}

	RefreshEntryView();
}

void UTunaSweeperMemoListEntryWidget::BuildEntryWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MemoEntryButton"));
	EntryLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MemoEntryLabelText"));
	if (!EntryButton || !EntryLabelText)
	{
		return;
	}

	WidgetTree->RootWidget = EntryButton;
	EntryButton->SetContent(EntryLabelText);
	EntryButton->SetRenderOpacity(0.9f);
	EntryLabelText->SetJustification(ETextJustify::Left);
	EntryLabelText->SetAutoWrapText(false);
	TunaSweeperUIFont::ApplyFont(EntryLabelText, 17);
}

void UTunaSweeperMemoListEntryWidget::RefreshEntryView()
{
	if (EntryButton)
	{
		EntryButton->SetIsEnabled(bAcquired);
		EntryButton->SetRenderOpacity(bAcquired ? (bSelected ? 1.0f : 0.82f) : 0.38f);
	}

	if (EntryLabelText)
	{
		EntryLabelText->SetText(Label);
		EntryLabelText->SetColorAndOpacity(FSlateColor(
			bAcquired
				? (bSelected ? FLinearColor(0.84f, 0.98f, 0.90f, 1.0f) : FLinearColor(0.82f, 0.88f, 0.90f, 1.0f))
				: FLinearColor(0.46f, 0.50f, 0.52f, 1.0f)));
	}
}

void UTunaSweeperMemoListEntryWidget::HandleEntryClicked()
{
	if (bAcquired && ClickedDelegate.IsBound())
	{
		ClickedDelegate.Execute(MemoId);
	}
}

void UTunaSweeperMemoWidget::OpenMemo(int32 MemoId)
{
	SelectedMemoId = MemoId;
	RefreshMemoView();
}

void UTunaSweeperMemoWidget::RefreshMemoView()
{
	BuildMemoWidget();
	if (MemoListTitleText)
	{
		MemoListTitleText->SetText(ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.memo.list_title"),
			TEXT("\uBA54\uBAA8 \uBAA9\uB85D")));
	}

	UTunaSweeperMemoSubsystem* MemoSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperMemoSubsystem>()
		: nullptr;
	if (!MemoSubsystem)
	{
		if (DetailTitleText)
		{
			DetailTitleText->SetText(ResolveUiText(
				GetGameInstance<UTunaSweeperGameInstance>(),
				TEXT("ui.memo.title"),
				TEXT("\uBA54\uBAA8")));
		}
		if (DetailBodyText)
		{
			DetailBodyText->SetText(FText::GetEmpty());
		}
		return;
	}

	TArray<FTunaSweeperMemoListEntry> Entries;
	MemoSubsystem->GetMemoListEntries(Entries);
	ApplySelectedMemo(Entries);
	RebuildMemoList(Entries);
}

TSharedRef<SWidget> UTunaSweeperMemoWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildMemoWidget();
	return Super::RebuildWidget();
}

void UTunaSweeperMemoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildMemoWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnMemoStateChanged.RemoveAll(this);
		TunaGameInstance->OnMemoStateChanged.AddUObject(this, &UTunaSweeperMemoWidget::HandleMemoStateChanged);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperMemoWidget::RefreshMemoView);
	}

	RefreshMemoView();
}

void UTunaSweeperMemoWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnMemoStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTunaSweeperMemoWidget::BuildMemoWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MemoRootPanel"));
	RootColumns = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MemoRootColumns"));
	USizeBox* ListSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MemoListSizeBox"));
	UBorder* ListPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MemoListPanel"));
	UVerticalBox* ListStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MemoListStack"));
	UTextBlock* ListTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MemoListTitleText"));
	MemoListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("MemoListScrollBox"));
	UBorder* DetailPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MemoDetailPanel"));
	UVerticalBox* DetailStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MemoDetailStack"));
	DetailTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MemoDetailTitleText"));
	DetailBodyScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("MemoDetailBodyScrollBox"));
	DetailBodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MemoDetailBodyText"));

	if (!RootPanel ||
		!RootColumns ||
		!ListSizeBox ||
		!ListPanel ||
		!ListStack ||
		!ListTitleText ||
		!MemoListScrollBox ||
		!DetailPanel ||
		!DetailStack ||
		!DetailTitleText ||
		!DetailBodyScrollBox ||
		!DetailBodyText)
	{
		return;
	}

	WidgetTree->RootWidget = RootPanel;
	RootPanel->SetBrush(MakeMemoBoxBrush(
		FVector2D(1180.0f, 640.0f),
		FLinearColor(0.035f, 0.04f, 0.045f, 0.94f),
		6.0f,
		FLinearColor(0.42f, 0.48f, 0.50f, 0.56f),
		1.0f));
	RootPanel->SetPadding(FMargin(16.0f));
	RootPanel->SetContent(RootColumns);

	ListSizeBox->SetWidthOverride(332.0f);
	ListPanel->SetBrush(MakeMemoBoxBrush(
		FVector2D(332.0f, 608.0f),
		FLinearColor(0.07f, 0.078f, 0.085f, 0.96f),
		4.0f,
		FLinearColor(0.24f, 0.30f, 0.33f, 0.65f),
		1.0f));
	ListPanel->SetPadding(FMargin(12.0f));
	ListPanel->SetContent(ListStack);
	ListSizeBox->SetContent(ListPanel);

	if (UHorizontalBoxSlot* ListColumnSlot = RootColumns->AddChildToHorizontalBox(ListSizeBox))
	{
		ListColumnSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
		ListColumnSlot->SetSize(MakeSlateChildSize(ESlateSizeRule::Automatic));
	}

	MemoListTitleText = ListTitleText;
	ListTitleText->SetText(ResolveUiText(
		GetGameInstance<UTunaSweeperGameInstance>(),
		TEXT("ui.memo.list_title"),
		TEXT("\uBA54\uBAA8 \uBAA9\uB85D")));
	ListTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.96f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(ListTitleText, 20, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* ListTitleSlot = ListStack->AddChildToVerticalBox(ListTitleText))
	{
		ListTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	if (UVerticalBoxSlot* ScrollSlot = ListStack->AddChildToVerticalBox(MemoListScrollBox))
	{
		ScrollSlot->SetSize(MakeSlateChildSize(ESlateSizeRule::Fill));
	}

	DetailPanel->SetBrush(MakeMemoBoxBrush(
		FVector2D(818.0f, 608.0f),
		FLinearColor(0.065f, 0.067f, 0.072f, 0.96f),
		4.0f,
		FLinearColor(0.27f, 0.31f, 0.32f, 0.62f),
		1.0f));
	DetailPanel->SetPadding(FMargin(20.0f, 18.0f));
	DetailPanel->SetContent(DetailStack);

	if (UHorizontalBoxSlot* DetailColumnSlot = RootColumns->AddChildToHorizontalBox(DetailPanel))
	{
		DetailColumnSlot->SetSize(MakeSlateChildSize(ESlateSizeRule::Fill));
	}

	DetailTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.98f, 0.98f, 1.0f)));
	DetailTitleText->SetAutoWrapText(true);
	TunaSweeperUIFont::ApplyFont(DetailTitleText, 24, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* DetailTitleSlot = DetailStack->AddChildToVerticalBox(DetailTitleText))
	{
		DetailTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	DetailBodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.82f, 0.88f, 0.86f, 1.0f)));
	DetailBodyText->SetAutoWrapText(true);
	DetailBodyText->SetWrapTextAt(760.0f);
	DetailBodyText->SetLineHeightPercentage(1.15f);
	TunaSweeperUIFont::ApplyFont(DetailBodyText, 18);
	DetailBodyScrollBox->AddChild(DetailBodyText);
	if (UVerticalBoxSlot* DetailBodySlot = DetailStack->AddChildToVerticalBox(DetailBodyScrollBox))
	{
		DetailBodySlot->SetSize(MakeSlateChildSize(ESlateSizeRule::Fill));
	}
}

void UTunaSweeperMemoWidget::RebuildMemoList(const TArray<FTunaSweeperMemoListEntry>& Entries)
{
	if (!MemoListScrollBox)
	{
		return;
	}

	MemoListScrollBox->ClearChildren();
	for (const FTunaSweeperMemoListEntry& Entry : Entries)
	{
		UTunaSweeperMemoListEntryWidget* EntryWidget = CreateWidget<UTunaSweeperMemoListEntryWidget>(
			GetOwningPlayer(),
			UTunaSweeperMemoListEntryWidget::StaticClass());
		if (!EntryWidget)
		{
			continue;
		}

		const FString LabelText = FString::Printf(
			TEXT("#%02d  %s"),
			Entry.MemoId,
			Entry.bAcquired ? *Entry.Title.ToString() : TEXT("???"));
		EntryWidget->InitializeMemoEntry(
			Entry.MemoId,
			FText::FromString(LabelText),
			Entry.bAcquired,
			Entry.MemoId == SelectedMemoId,
			FTunaSweeperMemoEntryClickedDelegate::CreateUObject(this, &UTunaSweeperMemoWidget::SetSelectedMemoId));
		MemoListScrollBox->AddChild(EntryWidget);
	}
}

void UTunaSweeperMemoWidget::ApplySelectedMemo(const TArray<FTunaSweeperMemoListEntry>& Entries)
{
	const FTunaSweeperMemoListEntry* SelectedEntry = nullptr;
	for (const FTunaSweeperMemoListEntry& Entry : Entries)
	{
		if (Entry.bAcquired && Entry.MemoId == SelectedMemoId)
		{
			SelectedEntry = &Entry;
			break;
		}
	}

	if (!SelectedEntry)
	{
		for (const FTunaSweeperMemoListEntry& Entry : Entries)
		{
			if (Entry.bAcquired)
			{
				SelectedEntry = &Entry;
				SelectedMemoId = Entry.MemoId;
				break;
			}
		}
	}

	if (!SelectedEntry)
	{
		SelectedMemoId = INDEX_NONE;
		if (DetailTitleText)
		{
			DetailTitleText->SetText(ResolveUiText(
				GetGameInstance<UTunaSweeperGameInstance>(),
				TEXT("ui.memo.title"),
				TEXT("\uBA54\uBAA8")));
		}
		if (DetailBodyText)
		{
			DetailBodyText->SetText(ResolveUiText(
				GetGameInstance<UTunaSweeperGameInstance>(),
				TEXT("ui.memo.none"),
				TEXT("\uD68D\uB4DD\uD55C \uBA54\uBAA8\uAC00 \uC5C6\uC2B5\uB2C8\uB2E4.")));
		}
		return;
	}

	if (DetailTitleText)
	{
		DetailTitleText->SetText(FText::FromString(FString::Printf(
			TEXT("#%02d  %s"),
			SelectedEntry->MemoId,
			*SelectedEntry->Title.ToString())));
	}
	if (DetailBodyText)
	{
		DetailBodyText->SetText(SelectedEntry->Body);
	}
}

void UTunaSweeperMemoWidget::SetSelectedMemoId(int32 MemoId)
{
	SelectedMemoId = MemoId;
	RefreshMemoView();
}

void UTunaSweeperMemoWidget::HandleMemoStateChanged()
{
	RefreshMemoView();
}
