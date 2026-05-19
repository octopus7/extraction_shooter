#include "UI/TunaSweeperDialogueWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"

namespace TunaSweeperDialogueWidget
{
	constexpr float PanelWidth = 1180.0f;
	constexpr float PanelHeight = 188.0f;
	constexpr float PanelBottomMargin = 56.0f;
	constexpr float SpeakerFontSize = 22.0f;
	constexpr float BodyFontSize = 25.0f;
	constexpr float ContinueFontSize = 28.0f;
}

namespace
{
	FSlateFontInfo MakeDialogueFont(UTextBlock* TextBlock, int32 Size)
	{
		FSlateFontInfo FontInfo = TextBlock ? TextBlock->GetFont() : FSlateFontInfo();
		FontInfo.Size = Size;
		return FontInfo;
	}
}

TSharedRef<SWidget> UTunaSweeperDialogueWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildDialogueWidget();
	}

	return Super::RebuildWidget();
}

void UTunaSweeperDialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	BuildDialogueWidget();
}

void UTunaSweeperDialogueWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bDialogueRunning || DialogueLines.IsEmpty() || !DialogueLines.IsValidIndex(CurrentLineIndex) || IsCurrentLineFullyVisible())
	{
		return;
	}

	TypewriterAccumulator += FMath::Max(0.0f, InDeltaTime) * FMath::Max(0.1f, CharactersPerSecond);
	const int32 TargetVisibleCharacters = FMath::Clamp(
		FMath::FloorToInt(TypewriterAccumulator),
		0,
		CurrentFullText.Len());

	if (TargetVisibleCharacters != VisibleCharacterCount)
	{
		VisibleCharacterCount = TargetVisibleCharacters;
		UpdateVisibleDialogueText();
	}
}

FReply UTunaSweeperDialogueWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}

	AdvanceOrFillLine();
	return FReply::Handled();
}

FReply UTunaSweeperDialogueWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	AdvanceOrFillLine();
	return FReply::Handled();
}

void UTunaSweeperDialogueWidget::StartDialogue(
	const TArray<FTunaSweeperDialogueLine>& InDialogueLines,
	float InCharactersPerSecond)
{
	BuildDialogueWidget();

	DialogueLines = InDialogueLines;
	CharactersPerSecond = FMath::Max(0.1f, InCharactersPerSecond);
	CurrentLineIndex = DialogueLines.IsEmpty() ? INDEX_NONE : 0;
	bDialogueRunning = !DialogueLines.IsEmpty();

	if (bDialogueRunning)
	{
		BeginCurrentLine();
	}
	else
	{
		FinishDialogue();
	}
}

void UTunaSweeperDialogueWidget::SetLineActivatedDelegate(FTunaSweeperDialogueLineActivatedDelegate InDelegate)
{
	LineActivatedDelegate = InDelegate;
}

void UTunaSweeperDialogueWidget::SetFinishedDelegate(FTunaSweeperDialogueFinishedDelegate InDelegate)
{
	FinishedDelegate = InDelegate;
}

void UTunaSweeperDialogueWidget::BuildDialogueWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("DialogueRoot"));
	DialoguePanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("DialoguePanel"));
	SpeakerNameText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("SpeakerNameText"));
	DialogueBodyText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DialogueBodyText"));
	ContinueMarkerText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ContinueMarkerText"));

	if (!RootCanvas || !DialoguePanel || !SpeakerNameText || !DialogueBodyText || !ContinueMarkerText)
	{
		return;
	}

	WidgetTree->RootWidget = RootCanvas;
	RootCanvas->SetVisibility(ESlateVisibility::Visible);

	DialoguePanel->SetBrushColor(FLinearColor(0.015f, 0.018f, 0.022f, 0.86f));
	DialoguePanel->SetPadding(FMargin(26.0f, 20.0f, 26.0f, 20.0f));
	DialoguePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(DialoguePanel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		PanelSlot->SetPosition(FVector2D(0.0f, -TunaSweeperDialogueWidget::PanelBottomMargin));
		PanelSlot->SetSize(FVector2D(TunaSweeperDialogueWidget::PanelWidth, TunaSweeperDialogueWidget::PanelHeight));
		PanelSlot->SetZOrder(1);
	}

	SpeakerNameText->SetFont(MakeDialogueFont(SpeakerNameText, TunaSweeperDialogueWidget::SpeakerFontSize));
	SpeakerNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.92f, 1.0f, 0.98f)));
	SpeakerNameText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	SpeakerNameText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.75f));
	SpeakerNameText->SetJustification(ETextJustify::Left);
	SpeakerNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* SpeakerSlot = RootCanvas->AddChildToCanvas(SpeakerNameText))
	{
		SpeakerSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		SpeakerSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		SpeakerSlot->SetPosition(FVector2D(
			-TunaSweeperDialogueWidget::PanelWidth * 0.5f + 34.0f,
			-TunaSweeperDialogueWidget::PanelBottomMargin - TunaSweeperDialogueWidget::PanelHeight + 24.0f));
		SpeakerSlot->SetSize(FVector2D(360.0f, 34.0f));
		SpeakerSlot->SetZOrder(2);
	}

	DialogueBodyText->SetFont(MakeDialogueFont(DialogueBodyText, TunaSweeperDialogueWidget::BodyFontSize));
	DialogueBodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.97f, 1.0f, 0.98f)));
	DialogueBodyText->SetShadowOffset(FVector2D(1.2f, 1.2f));
	DialogueBodyText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
	DialogueBodyText->SetAutoWrapText(true);
	DialogueBodyText->SetWrapTextAt(TunaSweeperDialogueWidget::PanelWidth - 96.0f);
	DialogueBodyText->SetJustification(ETextJustify::Left);
	DialogueBodyText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* BodySlot = RootCanvas->AddChildToCanvas(DialogueBodyText))
	{
		BodySlot->SetAnchors(FAnchors(0.5f, 1.0f));
		BodySlot->SetAlignment(FVector2D(0.0f, 1.0f));
		BodySlot->SetPosition(FVector2D(
			-TunaSweeperDialogueWidget::PanelWidth * 0.5f + 34.0f,
			-TunaSweeperDialogueWidget::PanelBottomMargin - 36.0f));
		BodySlot->SetSize(FVector2D(TunaSweeperDialogueWidget::PanelWidth - 92.0f, 116.0f));
		BodySlot->SetZOrder(2);
	}

	ContinueMarkerText->SetText(FText::FromString(TEXT(">")));
	ContinueMarkerText->SetFont(MakeDialogueFont(ContinueMarkerText, TunaSweeperDialogueWidget::ContinueFontSize));
	ContinueMarkerText->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.92f, 1.0f, 0.85f)));
	ContinueMarkerText->SetJustification(ETextJustify::Right);
	ContinueMarkerText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* MarkerSlot = RootCanvas->AddChildToCanvas(ContinueMarkerText))
	{
		MarkerSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		MarkerSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		MarkerSlot->SetPosition(FVector2D(
			TunaSweeperDialogueWidget::PanelWidth * 0.5f - 32.0f,
			-TunaSweeperDialogueWidget::PanelBottomMargin - 18.0f));
		MarkerSlot->SetSize(FVector2D(40.0f, 36.0f));
		MarkerSlot->SetZOrder(2);
	}
}

void UTunaSweeperDialogueWidget::BeginCurrentLine()
{
	if (!DialogueLines.IsValidIndex(CurrentLineIndex))
	{
		FinishDialogue();
		return;
	}

	const FTunaSweeperDialogueLine& CurrentLine = DialogueLines[CurrentLineIndex];
	CurrentFullText = CurrentLine.DialogueText.ToString();
	TypewriterAccumulator = 0.0f;
	VisibleCharacterCount = 0;

	if (SpeakerNameText)
	{
		SpeakerNameText->SetText(CurrentLine.SpeakerName);
	}
	UpdateVisibleDialogueText();

	if (LineActivatedDelegate.IsBound())
	{
		LineActivatedDelegate.Execute(CurrentLine);
	}
}

void UTunaSweeperDialogueWidget::UpdateVisibleDialogueText()
{
	if (DialogueBodyText)
	{
		DialogueBodyText->SetText(FText::FromString(CurrentFullText.Left(VisibleCharacterCount)));
	}

	if (ContinueMarkerText)
	{
		ContinueMarkerText->SetVisibility(IsCurrentLineFullyVisible()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperDialogueWidget::AdvanceOrFillLine()
{
	if (!bDialogueRunning)
	{
		return;
	}

	if (!IsCurrentLineFullyVisible())
	{
		VisibleCharacterCount = CurrentFullText.Len();
		TypewriterAccumulator = static_cast<float>(VisibleCharacterCount);
		UpdateVisibleDialogueText();
		return;
	}

	if (CurrentLineIndex + 1 < DialogueLines.Num())
	{
		++CurrentLineIndex;
		BeginCurrentLine();
		return;
	}

	FinishDialogue();
}

void UTunaSweeperDialogueWidget::FinishDialogue()
{
	if (!bDialogueRunning && DialogueLines.Num() > 0)
	{
		return;
	}

	bDialogueRunning = false;
	DialogueLines.Reset();
	CurrentFullText.Reset();
	CurrentLineIndex = INDEX_NONE;
	VisibleCharacterCount = 0;
	TypewriterAccumulator = 0.0f;

	if (FinishedDelegate.IsBound())
	{
		FinishedDelegate.Execute();
	}
}

bool UTunaSweeperDialogueWidget::IsCurrentLineFullyVisible() const
{
	return VisibleCharacterCount >= CurrentFullText.Len();
}
