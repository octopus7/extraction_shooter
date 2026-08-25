#include "UI/TunaSweeperDialogueWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SafeZone.h"
#include "Components/TextBlock.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputCoreTypes.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/SlateBrush.h"
#include "Engine/Texture2D.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"

namespace TunaSweeperDialogueWidget
{
	constexpr float PanelWidth = 1320.0f;
	constexpr float PanelHeight = 248.0f;
	constexpr float PanelBottomMargin = 52.0f;
	constexpr float PanelCornerRadius = 12.0f;
	constexpr float SpeakerTagWidth = 254.0f;
	constexpr float SpeakerTagHeight = 62.0f;
	constexpr float SpeakerTagLeftOffset = 26.0f;
	constexpr float SpeakerTagOverlap = 30.0f;
	constexpr float SpeakerFontSize = 27.0f;
	constexpr float BodyFontSize = 30.0f;
	constexpr float CompactBodyFontSize = 25.0f;
	constexpr float ContinueFontSize = 20.0f;
	constexpr float PanelHorizontalPadding = 64.0f;
	constexpr float PanelTopPadding = 50.0f;
	constexpr float BodyHeight = 154.0f;
	constexpr float ContinueReservedWidth = 166.0f;
	constexpr float SpeakerIconSize = 32.0f;
	constexpr TCHAR DialogueSurfaceTexturePath[] = TEXT("/Game/UI/Dialogue/T_UI_DialoguePanelSurface_C1.T_UI_DialoguePanelSurface_C1");
}

namespace
{
	using TunaSweeperUiText::ResolveUiText;

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
		return TunaSweeperUIFont::MakeFont(TextBlock, Size);
	}

	UTexture2D* LoadDialogueSurfaceTexture()
	{
		return LoadObject<UTexture2D>(nullptr, TunaSweeperDialogueWidget::DialogueSurfaceTexturePath);
	}

	float EstimateTextWidth(const FString& Text, float FontSize)
	{
		float Width = 0.0f;
		for (const TCHAR Character : Text)
		{
			if (FChar::IsWhitespace(Character))
			{
				Width += FontSize * 0.36f;
			}
			else if (Character < 0x80)
			{
				Width += FontSize * 0.54f;
			}
			else
			{
				Width += FontSize * 0.94f;
			}
		}

		return Width;
	}

	float MeasureDialogueTextWidth(const FString& Text, const FSlateFontInfo& FontInfo)
	{
		if (Text.IsEmpty())
		{
			return 0.0f;
		}

		if (FSlateApplication::IsInitialized())
		{
			if (FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer())
			{
				return Renderer->GetFontMeasureService()->Measure(Text, FontInfo).X;
			}
		}

		return EstimateTextWidth(Text, static_cast<float>(FontInfo.Size));
	}

	int32 FindLastWhitespaceIndex(const FString& Text)
	{
		for (int32 Index = Text.Len() - 1; Index >= 0; --Index)
		{
			if (FChar::IsWhitespace(Text[Index]))
			{
				return Index;
			}
		}

		return INDEX_NONE;
	}

	void AppendWrappedLine(FString& WrappedText, const FString& Line)
	{
		if (!WrappedText.IsEmpty())
		{
			WrappedText.AppendChar(TEXT('\n'));
		}

		WrappedText += Line.TrimEnd();
	}

	FString PreWrapDialogueText(const FString& SourceText, const FSlateFontInfo& FontInfo, float WrapWidth)
	{
		if (SourceText.IsEmpty() || WrapWidth <= 0.0f)
		{
			return SourceText;
		}

		FString WrappedText;
		FString CurrentLine;
		int32 LastWhitespaceIndex = INDEX_NONE;

		for (int32 SourceIndex = 0; SourceIndex < SourceText.Len(); ++SourceIndex)
		{
			const TCHAR Character = SourceText[SourceIndex];
			if (Character == TEXT('\r'))
			{
				continue;
			}

			if (Character == TEXT('\n'))
			{
				AppendWrappedLine(WrappedText, CurrentLine);
				CurrentLine.Reset();
				LastWhitespaceIndex = INDEX_NONE;
				continue;
			}

			if (FChar::IsWhitespace(Character) && CurrentLine.IsEmpty())
			{
				continue;
			}

			FString CandidateLine = CurrentLine;
			CandidateLine.AppendChar(Character);
			if (CurrentLine.IsEmpty() || MeasureDialogueTextWidth(CandidateLine, FontInfo) <= WrapWidth)
			{
				CurrentLine = MoveTemp(CandidateLine);
				if (FChar::IsWhitespace(Character))
				{
					LastWhitespaceIndex = CurrentLine.Len() - 1;
				}
				continue;
			}

			if (FChar::IsWhitespace(Character))
			{
				AppendWrappedLine(WrappedText, CurrentLine);
				CurrentLine.Reset();
				LastWhitespaceIndex = INDEX_NONE;
				continue;
			}

			if (LastWhitespaceIndex != INDEX_NONE)
			{
				AppendWrappedLine(WrappedText, CurrentLine.Left(LastWhitespaceIndex));
				CurrentLine = CurrentLine.Mid(LastWhitespaceIndex + 1).TrimStart();
				LastWhitespaceIndex = FindLastWhitespaceIndex(CurrentLine);
				--SourceIndex;
				continue;
			}

			AppendWrappedLine(WrappedText, CurrentLine);
			CurrentLine.Reset();
			LastWhitespaceIndex = INDEX_NONE;
			--SourceIndex;
		}

		AppendWrappedLine(WrappedText, CurrentLine);
		return WrappedText;
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
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
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

	bLastInputWasGamepad = InKeyEvent.GetKey().IsGamepadKey();
	UpdateContinueInputHint();
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

	USafeZone* SafeZone = WidgetTree->ConstructWidget<USafeZone>(
		USafeZone::StaticClass(),
		TEXT("DialogueSafeZone"));
	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("DialogueRoot"));
	DialogueShadow = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("DialogueShadow"));
	DialoguePanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("DialoguePanel"));
	DialogueSurfaceImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("DialogueSurfaceImage"));
	DialogueAccentLine = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("DialogueAccentLine"));
	SpeakerNamePanel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("SpeakerNamePanel"));
	SpeakerIconBackplate = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("SpeakerIconBackplate"));
	SpeakerIconImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("SpeakerIconImage"));
	SpeakerNameText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("SpeakerNameText"));
	DialogueBodyText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DialogueBodyText"));
	ContinuePromptText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ContinuePromptText"));
	ContinueKeycap = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("ContinueKeycap"));
	ContinueKeyText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ContinueKeyText"));

	if (!SafeZone ||
		!RootCanvas ||
		!DialogueShadow ||
		!DialoguePanel ||
		!DialogueSurfaceImage ||
		!DialogueAccentLine ||
		!SpeakerNamePanel ||
		!SpeakerIconBackplate ||
		!SpeakerIconImage ||
		!SpeakerNameText ||
		!DialogueBodyText ||
		!ContinuePromptText ||
		!ContinueKeycap ||
		!ContinueKeyText)
	{
		return;
	}

	WidgetTree->RootWidget = SafeZone;
	SafeZone->AddChild(RootCanvas);
	RootCanvas->SetVisibility(ESlateVisibility::Visible);

	DialogueShadow->SetBrush(MakeRoundedBoxBrush(
		FVector2D(TunaSweeperDialogueWidget::PanelWidth, TunaSweeperDialogueWidget::PanelHeight),
		FLinearColor(0.10f, 0.065f, 0.035f, 0.22f),
		TunaSweeperDialogueWidget::PanelCornerRadius + 2.0f,
		FLinearColor::Transparent,
		0.0f));
	DialogueShadow->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* ShadowSlot = RootCanvas->AddChildToCanvas(DialogueShadow))
	{
		ShadowSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		ShadowSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		ShadowSlot->SetPosition(FVector2D(6.0f, -TunaSweeperDialogueWidget::PanelBottomMargin + 7.0f));
		ShadowSlot->SetSize(FVector2D(TunaSweeperDialogueWidget::PanelWidth, TunaSweeperDialogueWidget::PanelHeight));
		ShadowSlot->SetZOrder(0);
	}

	DialoguePanel->SetBrush(MakeRoundedBoxBrush(
		FVector2D(TunaSweeperDialogueWidget::PanelWidth, TunaSweeperDialogueWidget::PanelHeight),
		FLinearColor(0.985f, 0.954f, 0.892f, 1.0f),
		TunaSweeperDialogueWidget::PanelCornerRadius,
		FLinearColor(0.33f, 0.27f, 0.20f, 0.40f),
		1.0f));
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

	if (UTexture2D* SurfaceTexture = LoadDialogueSurfaceTexture())
	{
		DialogueSurfaceImage->SetBrushFromTexture(SurfaceTexture, false);
	}
	DialogueSurfaceImage->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.62f));
	DialogueSurfaceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* SurfaceSlot = RootCanvas->AddChildToCanvas(DialogueSurfaceImage))
	{
		SurfaceSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		SurfaceSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		SurfaceSlot->SetPosition(FVector2D(0.0f, -TunaSweeperDialogueWidget::PanelBottomMargin - 2.0f));
		SurfaceSlot->SetSize(FVector2D(TunaSweeperDialogueWidget::PanelWidth - 4.0f, TunaSweeperDialogueWidget::PanelHeight - 4.0f));
		SurfaceSlot->SetZOrder(2);
	}

	DialogueAccentLine->SetBrush(MakeRoundedBoxBrush(
		FVector2D(TunaSweeperDialogueWidget::PanelWidth - 36.0f, 2.0f),
		FLinearColor(0.42f, 0.56f, 0.62f, 0.90f),
		1.0f,
		FLinearColor::Transparent,
		0.0f));
	DialogueAccentLine->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* AccentLineSlot = RootCanvas->AddChildToCanvas(DialogueAccentLine))
	{
		AccentLineSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		AccentLineSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		AccentLineSlot->SetPosition(FVector2D(0.0f, -TunaSweeperDialogueWidget::PanelBottomMargin - TunaSweeperDialogueWidget::PanelHeight + 4.0f));
		AccentLineSlot->SetSize(FVector2D(TunaSweeperDialogueWidget::PanelWidth - 36.0f, 2.0f));
		AccentLineSlot->SetZOrder(3);
	}

	const float PanelLeftX = -TunaSweeperDialogueWidget::PanelWidth * 0.5f;
	const float SpeakerTagX = PanelLeftX + TunaSweeperDialogueWidget::SpeakerTagLeftOffset;
	const float SpeakerTagBottomY =
		-TunaSweeperDialogueWidget::PanelBottomMargin -
		TunaSweeperDialogueWidget::PanelHeight +
		TunaSweeperDialogueWidget::SpeakerTagOverlap;

	SpeakerNamePanel->SetBrush(MakeRoundedBoxBrush(
		FVector2D(TunaSweeperDialogueWidget::SpeakerTagWidth, TunaSweeperDialogueWidget::SpeakerTagHeight),
		FLinearColor(0.35f, 0.46f, 0.54f, 0.98f),
		TunaSweeperDialogueWidget::PanelCornerRadius,
		FLinearColor(0.20f, 0.27f, 0.32f, 0.72f),
		1.0f));
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

	SpeakerIconBackplate->SetBrush(MakeRoundedBoxBrush(
		FVector2D(TunaSweeperDialogueWidget::SpeakerIconSize, TunaSweeperDialogueWidget::SpeakerIconSize),
		FLinearColor(0.88f, 0.93f, 0.94f, 0.94f),
		TunaSweeperDialogueWidget::SpeakerIconSize * 0.5f,
		FLinearColor(0.20f, 0.29f, 0.35f, 0.22f),
		1.0f));
	SpeakerIconBackplate->SetPadding(FMargin(0.0f));
	SpeakerIconBackplate->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* IconBackplateSlot = RootCanvas->AddChildToCanvas(SpeakerIconBackplate))
	{
		IconBackplateSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		IconBackplateSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		IconBackplateSlot->SetPosition(FVector2D(
			SpeakerTagX + 17.0f,
			SpeakerTagBottomY - TunaSweeperDialogueWidget::SpeakerTagHeight * 0.5f));
		IconBackplateSlot->SetSize(FVector2D(TunaSweeperDialogueWidget::SpeakerIconSize, TunaSweeperDialogueWidget::SpeakerIconSize));
		IconBackplateSlot->SetZOrder(5);
	}

	SpeakerIconImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* IconSlot = RootCanvas->AddChildToCanvas(SpeakerIconImage))
	{
		IconSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		IconSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		IconSlot->SetPosition(FVector2D(
			SpeakerTagX + 20.0f,
			SpeakerTagBottomY - TunaSweeperDialogueWidget::SpeakerTagHeight * 0.5f));
		IconSlot->SetSize(FVector2D(TunaSweeperDialogueWidget::SpeakerIconSize - 6.0f, TunaSweeperDialogueWidget::SpeakerIconSize - 6.0f));
		IconSlot->SetZOrder(6);
	}

	SpeakerNameText->SetFont(MakeDialogueFont(SpeakerNameText, TunaSweeperDialogueWidget::SpeakerFontSize));
	SpeakerNameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.99f, 1.0f, 1.0f)));
	SpeakerNameText->SetShadowOffset(FVector2D::ZeroVector);
	SpeakerNameText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	SpeakerNameText->SetJustification(ETextJustify::Center);
	SpeakerNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* SpeakerSlot = RootCanvas->AddChildToCanvas(SpeakerNameText))
	{
		SpeakerSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		SpeakerSlot->SetAlignment(FVector2D(0.0f, 1.0f));
		SpeakerSlot->SetPosition(FVector2D(SpeakerTagX + 42.0f, SpeakerTagBottomY - 9.0f));
		SpeakerSlot->SetSize(FVector2D(
			TunaSweeperDialogueWidget::SpeakerTagWidth - 48.0f,
			TunaSweeperDialogueWidget::SpeakerTagHeight - 12.0f));
		SpeakerSlot->SetZOrder(4);
	}

	DialogueBodyText->SetFont(MakeDialogueFont(DialogueBodyText, TunaSweeperDialogueWidget::BodyFontSize));
	DialogueBodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.235f, 0.14f, 0.075f, 0.98f)));
	DialogueBodyText->SetShadowOffset(FVector2D::ZeroVector);
	DialogueBodyText->SetShadowColorAndOpacity(FLinearColor::Transparent);
	DialogueBodyText->SetAutoWrapText(false);
	DialogueBodyText->SetWrapTextAt(0.0f);
	DialogueBodyText->SetJustification(ETextJustify::Left);
	DialogueBodyText->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* BodySlot = RootCanvas->AddChildToCanvas(DialogueBodyText))
	{
		BodySlot->SetAnchors(FAnchors(0.5f, 1.0f));
		BodySlot->SetAlignment(FVector2D::ZeroVector);
		BodySlot->SetPosition(FVector2D(
			PanelLeftX + TunaSweeperDialogueWidget::PanelHorizontalPadding,
			-TunaSweeperDialogueWidget::PanelBottomMargin -
			(TunaSweeperDialogueWidget::PanelHeight - TunaSweeperDialogueWidget::PanelTopPadding)));
		BodySlot->SetSize(FVector2D(
			TunaSweeperDialogueWidget::PanelWidth -
			TunaSweeperDialogueWidget::PanelHorizontalPadding * 2.0f -
			TunaSweeperDialogueWidget::ContinueReservedWidth,
			TunaSweeperDialogueWidget::BodyHeight));
		BodySlot->SetZOrder(4);
	}

	ContinuePromptText->SetText(ResolveUiText(
		GetGameInstance<UTunaSweeperGameInstance>(),
		TEXT("ui.dialogue.continue"),
		TEXT("\uACC4\uC18D")));
	ContinuePromptText->SetFont(MakeDialogueFont(ContinuePromptText, TunaSweeperDialogueWidget::ContinueFontSize));
	ContinuePromptText->SetColorAndOpacity(FSlateColor(FLinearColor(0.26f, 0.18f, 0.11f, 0.90f)));
	ContinuePromptText->SetJustification(ETextJustify::Right);
	ContinuePromptText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(ContinuePromptText))
	{
		PromptSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		PromptSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		PromptSlot->SetPosition(FVector2D(
			TunaSweeperDialogueWidget::PanelWidth * 0.5f - 92.0f,
			-TunaSweeperDialogueWidget::PanelBottomMargin - 28.0f));
		PromptSlot->SetSize(FVector2D(82.0f, 28.0f));
		PromptSlot->SetZOrder(4);
	}

	ContinueKeycap->SetBrush(MakeRoundedBoxBrush(
		FVector2D(58.0f, 34.0f),
		FLinearColor(0.90f, 0.94f, 0.95f, 0.96f),
		6.0f,
		FLinearColor(0.30f, 0.46f, 0.55f, 0.88f),
		1.5f));
	ContinueKeycap->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* KeycapSlot = RootCanvas->AddChildToCanvas(ContinueKeycap))
	{
		KeycapSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		KeycapSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		KeycapSlot->SetPosition(FVector2D(
			TunaSweeperDialogueWidget::PanelWidth * 0.5f - 28.0f,
			-TunaSweeperDialogueWidget::PanelBottomMargin - 22.0f));
		KeycapSlot->SetSize(FVector2D(58.0f, 34.0f));
		KeycapSlot->SetZOrder(4);
	}

	ContinueKeyText->SetFont(MakeDialogueFont(ContinueKeyText, 18));
	ContinueKeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.20f, 0.33f, 0.40f, 1.0f)));
	ContinueKeyText->SetJustification(ETextJustify::Center);
	ContinueKeyText->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* KeyTextSlot = RootCanvas->AddChildToCanvas(ContinueKeyText))
	{
		KeyTextSlot->SetAnchors(FAnchors(0.5f, 1.0f));
		KeyTextSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		KeyTextSlot->SetPosition(FVector2D(
			TunaSweeperDialogueWidget::PanelWidth * 0.5f - 28.0f,
			-TunaSweeperDialogueWidget::PanelBottomMargin - 22.0f));
		KeyTextSlot->SetSize(FVector2D(58.0f, 34.0f));
		KeyTextSlot->SetZOrder(5);
	}

	UpdateContinueInputHint();
}

void UTunaSweeperDialogueWidget::BeginCurrentLine()
{
	if (!DialogueLines.IsValidIndex(CurrentLineIndex))
	{
		FinishDialogue();
		return;
	}

	const FTunaSweeperDialogueLine& CurrentLine = DialogueLines[CurrentLineIndex];
	DialogueBodyText->SetFont(MakeDialogueFont(DialogueBodyText, TunaSweeperDialogueWidget::BodyFontSize));
	CurrentFullText = PreWrapDialogueText(
		CurrentLine.DialogueText.ToString(),
		DialogueBodyText ? DialogueBodyText->GetFont() : FSlateFontInfo(),
		TunaSweeperDialogueWidget::PanelWidth -
		TunaSweeperDialogueWidget::PanelHorizontalPadding * 2.0f -
		TunaSweeperDialogueWidget::ContinueReservedWidth);
	int32 LineBreakCount = 0;
	for (const TCHAR Character : CurrentFullText)
	{
		LineBreakCount += Character == TEXT('\n') ? 1 : 0;
	}
	if (LineBreakCount >= 3)
	{
		DialogueBodyText->SetFont(MakeDialogueFont(DialogueBodyText, TunaSweeperDialogueWidget::CompactBodyFontSize));
		CurrentFullText = PreWrapDialogueText(
			CurrentLine.DialogueText.ToString(),
			DialogueBodyText->GetFont(),
			TunaSweeperDialogueWidget::PanelWidth -
			TunaSweeperDialogueWidget::PanelHorizontalPadding * 2.0f -
			TunaSweeperDialogueWidget::ContinueReservedWidth);
	}
	TypewriterAccumulator = 0.0f;
	VisibleCharacterCount = 0;

	UpdateSpeakerPresentation(CurrentLine);
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

	if (ContinueKeycap)
	{
		ContinueKeycap->SetVisibility(IsCurrentLineFullyVisible()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (ContinueKeyText)
	{
		ContinueKeyText->SetVisibility(IsCurrentLineFullyVisible()
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

void UTunaSweeperDialogueWidget::UpdateSpeakerPresentation(const FTunaSweeperDialogueLine& CurrentLine)
{
	const bool bHasSpeakerName = !CurrentLine.SpeakerName.IsEmpty();
	const ESlateVisibility SpeakerVisibility = bHasSpeakerName
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed;

	if (SpeakerNamePanel)
	{
		SpeakerNamePanel->SetVisibility(SpeakerVisibility);
	}
	if (SpeakerIconBackplate)
	{
		SpeakerIconBackplate->SetVisibility(SpeakerVisibility);
	}
	if (SpeakerNameText)
	{
		SpeakerNameText->SetText(CurrentLine.SpeakerName);
		SpeakerNameText->SetVisibility(SpeakerVisibility);
	}

	if (SpeakerIconImage)
	{
		if (bHasSpeakerName && !CurrentLine.SpeakerIcon.IsNull())
		{
			if (UTexture2D* SpeakerIconTexture = CurrentLine.SpeakerIcon.LoadSynchronous())
			{
				SpeakerIconImage->SetBrushFromTexture(SpeakerIconTexture, true);
				SpeakerIconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
				return;
			}
		}

		SpeakerIconImage->SetBrush(FSlateBrush());
		SpeakerIconImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperDialogueWidget::UpdateContinueInputHint()
{
	if (ContinueKeyText)
	{
		ContinueKeyText->SetText(FText::FromString(bLastInputWasGamepad ? TEXT("A") : TEXT("Enter")));
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
