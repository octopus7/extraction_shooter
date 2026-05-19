#include "UI/TunaSweeperDialogueWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "InputCoreTypes.h"
#include "Styling/SlateBrush.h"

namespace TunaSweeperDialogueWidget
{
	constexpr float PanelWidth = 1120.0f;
	constexpr float PanelHeight = 214.0f;
	constexpr float PanelBottomMargin = 58.0f;
	constexpr float PanelCornerRadius = 8.0f;
	constexpr float SpeakerTagWidth = 224.0f;
	constexpr float SpeakerTagHeight = 52.0f;
	constexpr float SpeakerTagLeftOffset = 20.0f;
	constexpr float SpeakerTagBottomGap = 6.0f;
	constexpr float SpeakerFontSize = 26.0f;
	constexpr float BodyFontSize = 27.0f;
	constexpr float ContinueFontSize = 21.0f;
	constexpr float ContinueMarkerFontSize = 24.0f;
	constexpr float PanelHorizontalPadding = 58.0f;
}

namespace
{
	FSlateBrush MakeRoundedBoxBrush(
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
	SpeakerNamePanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("SpeakerNamePanel"));
	SpeakerAccentBar = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("SpeakerAccentBar"));
	SpeakerNameText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("SpeakerNameText"));
	DialogueBodyText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DialogueBodyText"));
	ContinuePromptText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ContinuePromptText"));
	ContinueMarkerText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ContinueMarkerText"));

	if (!RootCanvas ||
		!DialoguePanel ||
		!SpeakerNamePanel ||
		!SpeakerAccentBar ||
		!SpeakerNameText ||
		!DialogueBodyText ||
		!ContinuePromptText ||
		!ContinueMarkerText)
	{
		return;
	}

	WidgetTree->RootWidget = RootCanvas;
	RootCanvas->SetVisibility(ESlateVisibility::Visible);

	DialoguePanel->SetBrush(MakeRoundedBoxBrush(
		FVector2D(TunaSweeperDialogueWidget::PanelWidth, TunaSweeperDialogueWidget::PanelHeight),
		FLinearColor(0.095f, 0.105f, 0.118f, 0.94f),
		TunaSweeperDialogueWidget::PanelCornerRadius,
		FLinearColor(0.48f, 0.51f, 0.54f, 0.58f),
		1.25f));
	DialoguePanel->SetPadding(FMargin(0.0f));
	DialoguePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(DialoguePanel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		PanelSlot->SetPosition(FVector2D(0.0f, -TunaSweeperDialogueWidget::PanelBottomMargin));
		PanelSlot->SetSize(FVector2D(TunaSweeperDialogueWidget::PanelWidth, TunaSweeperDialogueWidget::PanelHeight));
		PanelSlot->SetZOrder(1);
	}

	const float PanelLeftX = -TunaSweeperDialogueWidget::PanelWidth * 0.5f;
	const float SpeakerTagX = PanelLeftX + TunaSweeperDialogueWidget::SpeakerTagLeftOffset;
	const float SpeakerTagBottomY =
		-TunaSweeperDialogueWidget::PanelBottomMargin -
		TunaSweeperDialogueWidget::PanelHeight -
		TunaSweeperDialogueWidget::SpeakerTagBottomGap;

	SpeakerNamePanel->SetBrush(MakeRoundedBoxBrush(
		FVector2D(TunaSweeperDialogueWidget::SpeakerTagWidth, TunaSweeperDialogueWidget::SpeakerTagHeight),
		FLinearColor(0.105f, 0.115f, 0.128f, 0.96f),
		TunaSweeperDialogueWidget::PanelCornerRadius,
		FLinearColor(0.48f, 0.51f, 0.54f, 0.60f),
		1.25f));
	SpeakerNamePanel->SetPadding(FMargin(0.0f));
	SpeakerNamePanel->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* SpeakerPanelSlot = RootCanvas->AddChildToCanvas(SpeakerNamePanel))
	{
		SpeakerPanelSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		SpeakerPanelSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		SpeakerPanelSlot->SetPosition(FVector2D(SpeakerTagX, SpeakerTagBottomY));
		SpeakerPanelSlot->SetSize(FVector2D(
			TunaSweeperDialogueWidget::SpeakerTagWidth,
			TunaSweeperDialogueWidget::SpeakerTagHeight));
		SpeakerPanelSlot->SetZOrder(2);
	}

	SpeakerAccentBar->SetBrush(MakeRoundedBoxBrush(
		FVector2D(5.0f, 36.0f),
		FLinearColor(0.45f, 0.90f, 0.92f, 1.0f),
		2.0f,
		FLinearColor::Transparent,
		0.0f));
	SpeakerAccentBar->SetPadding(FMargin(0.0f));
	SpeakerAccentBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* AccentSlot = RootCanvas->AddChildToCanvas(SpeakerAccentBar))
	{
		AccentSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		AccentSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		AccentSlot->SetPosition(FVector2D(
			SpeakerTagX + 18.0f,
			SpeakerTagBottomY - TunaSweeperDialogueWidget::SpeakerTagHeight * 0.5f));
		AccentSlot->SetSize(FVector2D(5.0f, 36.0f));
		AccentSlot->SetZOrder(3);
	}

	SpeakerNameText->SetFont(MakeDialogueFont(SpeakerNameText, TunaSweeperDialogueWidget::SpeakerFontSize));
	SpeakerNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.98f, 1.0f, 0.98f)));
	SpeakerNameText->SetShadowOffset(FVector2D::ZeroVector);
	SpeakerNameText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	SpeakerNameText->SetJustification(ETextJustify::Center);
	SpeakerNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* SpeakerSlot = RootCanvas->AddChildToCanvas(SpeakerNameText))
	{
		SpeakerSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		SpeakerSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		SpeakerSlot->SetPosition(FVector2D(SpeakerTagX, SpeakerTagBottomY - 9.0f));
		SpeakerSlot->SetSize(FVector2D(
			TunaSweeperDialogueWidget::SpeakerTagWidth,
			TunaSweeperDialogueWidget::SpeakerTagHeight - 12.0f));
		SpeakerSlot->SetZOrder(4);
	}

	DialogueBodyText->SetFont(MakeDialogueFont(DialogueBodyText, TunaSweeperDialogueWidget::BodyFontSize));
	DialogueBodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.98f, 1.0f, 0.98f)));
	DialogueBodyText->SetShadowOffset(FVector2D::ZeroVector);
	DialogueBodyText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	DialogueBodyText->SetAutoWrapText(true);
	DialogueBodyText->SetWrapTextAt(
		TunaSweeperDialogueWidget::PanelWidth -
		TunaSweeperDialogueWidget::PanelHorizontalPadding * 2.0f -
		120.0f);
	DialogueBodyText->SetJustification(ETextJustify::Left);
	DialogueBodyText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* BodySlot = RootCanvas->AddChildToCanvas(DialogueBodyText))
	{
		BodySlot->SetAnchors(FAnchors(0.5f, 1.0f));
		BodySlot->SetAlignment(FVector2D(0.0f, 1.0f));
		BodySlot->SetPosition(FVector2D(
			PanelLeftX + TunaSweeperDialogueWidget::PanelHorizontalPadding,
			-TunaSweeperDialogueWidget::PanelBottomMargin - 68.0f));
		BodySlot->SetSize(FVector2D(
			TunaSweeperDialogueWidget::PanelWidth -
			TunaSweeperDialogueWidget::PanelHorizontalPadding * 2.0f -
			120.0f,
			116.0f));
		BodySlot->SetZOrder(2);
	}

	ContinuePromptText->SetText(FText::FromString(TEXT("\uACC4\uC18D")));
	ContinuePromptText->SetFont(MakeDialogueFont(ContinuePromptText, TunaSweeperDialogueWidget::ContinueFontSize));
	ContinuePromptText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.98f, 1.0f, 0.92f)));
	ContinuePromptText->SetJustification(ETextJustify::Right);
	ContinuePromptText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(ContinuePromptText))
	{
		PromptSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		PromptSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		PromptSlot->SetPosition(FVector2D(
			TunaSweeperDialogueWidget::PanelWidth * 0.5f - 78.0f,
			-TunaSweeperDialogueWidget::PanelBottomMargin - 38.0f));
		PromptSlot->SetSize(FVector2D(56.0f, 30.0f));
		PromptSlot->SetZOrder(2);
	}

	ContinueMarkerText->SetText(FText::FromString(TEXT("\u25BE")));
	ContinueMarkerText->SetFont(MakeDialogueFont(ContinueMarkerText, TunaSweeperDialogueWidget::ContinueMarkerFontSize));
	ContinueMarkerText->SetColorAndOpacity(FSlateColor(FLinearColor(0.45f, 0.90f, 0.92f, 0.95f)));
	ContinueMarkerText->SetJustification(ETextJustify::Right);
	ContinueMarkerText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* MarkerSlot = RootCanvas->AddChildToCanvas(ContinueMarkerText))
	{
		MarkerSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		MarkerSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		MarkerSlot->SetPosition(FVector2D(
			TunaSweeperDialogueWidget::PanelWidth * 0.5f - 34.0f,
			-TunaSweeperDialogueWidget::PanelBottomMargin - 35.0f));
		MarkerSlot->SetSize(FVector2D(30.0f, 30.0f));
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

	if (ContinuePromptText)
	{
		ContinuePromptText->SetVisibility(IsCurrentLineFullyVisible()
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
