#include "UI/TunaSweeperScreenSpaceSpeechBubbleWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperScreenSpaceSpeechBubble
{
	constexpr float MinimumBodyWidth = 160.0f;
	constexpr float MinimumBodyHeight = 56.0f;
	constexpr float MaximumBodyWidth = 420.0f;
	constexpr float TextWrapWidth = 376.0f;
	constexpr float TailSize = 34.0f;
	constexpr float TailReserve = 24.0f;
	constexpr float TailTipRadius = 16.0f;

	FVector2D DirectionVector(const ETunaSweeperSpeechBubbleTailDirection Direction)
	{
		switch (Direction)
		{
		case ETunaSweeperSpeechBubbleTailDirection::Up: return FVector2D(0.0f, -1.0f);
		case ETunaSweeperSpeechBubbleTailDirection::UpRight: return FVector2D(1.0f, -1.0f).GetSafeNormal();
		case ETunaSweeperSpeechBubbleTailDirection::Right: return FVector2D(1.0f, 0.0f);
		case ETunaSweeperSpeechBubbleTailDirection::DownRight: return FVector2D(1.0f, 1.0f).GetSafeNormal();
		case ETunaSweeperSpeechBubbleTailDirection::Down: return FVector2D(0.0f, 1.0f);
		case ETunaSweeperSpeechBubbleTailDirection::DownLeft: return FVector2D(-1.0f, 1.0f).GetSafeNormal();
		case ETunaSweeperSpeechBubbleTailDirection::Left: return FVector2D(-1.0f, 0.0f);
		case ETunaSweeperSpeechBubbleTailDirection::UpLeft: return FVector2D(-1.0f, -1.0f).GetSafeNormal();
		default: return FVector2D::ZeroVector;
		}
	}

	FMargin BodyPadding(const ETunaSweeperSpeechBubbleTailDirection Direction)
	{
		const FVector2D Vector = DirectionVector(Direction);
		return FMargin(
			Vector.X < 0.0f ? TailReserve : 0.0f,
			Vector.Y < 0.0f ? TailReserve : 0.0f,
			Vector.X > 0.0f ? TailReserve : 0.0f,
			Vector.Y > 0.0f ? TailReserve : 0.0f);
	}

	float TailAngle(const ETunaSweeperSpeechBubbleTailDirection Direction)
	{
		switch (Direction)
		{
		case ETunaSweeperSpeechBubbleTailDirection::Down: return 0.0f;
		case ETunaSweeperSpeechBubbleTailDirection::DownRight: return -45.0f;
		case ETunaSweeperSpeechBubbleTailDirection::Right: return -90.0f;
		case ETunaSweeperSpeechBubbleTailDirection::UpRight: return -135.0f;
		case ETunaSweeperSpeechBubbleTailDirection::Up: return 180.0f;
		case ETunaSweeperSpeechBubbleTailDirection::UpLeft: return 135.0f;
		case ETunaSweeperSpeechBubbleTailDirection::Left: return 90.0f;
		case ETunaSweeperSpeechBubbleTailDirection::DownLeft: return 45.0f;
		default: return 0.0f;
		}
	}

	EHorizontalAlignment HorizontalAlignment(const FVector2D& Direction)
	{
		return Direction.X < 0.0f ? HAlign_Left : (Direction.X > 0.0f ? HAlign_Right : HAlign_Center);
	}

	EVerticalAlignment VerticalAlignment(const FVector2D& Direction)
	{
		return Direction.Y < 0.0f ? VAlign_Top : (Direction.Y > 0.0f ? VAlign_Bottom : VAlign_Center);
	}
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::Configure(
	const FText& InText,
	const ETunaSweeperSpeechBubbleTailDirection InTailDirection)
{
	PendingText = InText;
	TailDirection = InTailDirection;
	EnsureWidgetTree();
	ApplyPresentation();
}

TSharedRef<SWidget> UTunaSweeperScreenSpaceSpeechBubbleWidget::RebuildWidget()
{
	EnsureWidgetTree();
	TSharedRef<SWidget> RebuiltWidget = Super::RebuildWidget();
	ApplyPresentation();
	return RebuiltWidget;
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::EnsureWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (!WidgetTree->RootWidget || !RootOverlay || !BodyBox || !BodyImage || !BubbleText || !TailBox || !TailImage)
	{
		BuildNativeWidgetTree();
	}
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::BuildNativeWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	RootOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SpeechBubbleRoot"));
	TailBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("TailBox"));
	TailImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TailImage"));
	BodyBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BodyBox"));
	UOverlay* BodyOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BodyOverlay"));
	BodyImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BodyImage"));
	BubbleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BubbleText"));
	if (!RootOverlay || !TailBox || !TailImage || !BodyBox || !BodyOverlay || !BodyImage || !BubbleText)
	{
		return;
	}

	WidgetTree->RootWidget = RootOverlay;
	RootOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);

	TailBox->SetWidthOverride(TunaSweeperScreenSpaceSpeechBubble::TailSize);
	TailBox->SetHeightOverride(TunaSweeperScreenSpaceSpeechBubble::TailSize);
	TailBox->SetContent(TailImage);
	RootOverlay->AddChildToOverlay(TailBox);

	BodyBox->SetMinDesiredWidth(TunaSweeperScreenSpaceSpeechBubble::MinimumBodyWidth);
	BodyBox->SetMinDesiredHeight(TunaSweeperScreenSpaceSpeechBubble::MinimumBodyHeight);
	BodyBox->SetMaxDesiredWidth(TunaSweeperScreenSpaceSpeechBubble::MaximumBodyWidth);
	BodyBox->SetContent(BodyOverlay);
	RootOverlay->AddChildToOverlay(BodyBox);

	BodyOverlay->AddChildToOverlay(BodyImage);
	if (UOverlaySlot* TextSlot = BodyOverlay->AddChildToOverlay(BubbleText))
	{
		TextSlot->SetPadding(FMargin(22.0f, 12.0f));
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
	}

	BubbleText->SetAutoWrapText(true);
	BubbleText->SetWrapTextAt(TunaSweeperScreenSpaceSpeechBubble::TextWrapWidth);
	BubbleText->SetJustification(ETextJustify::Center);
	BubbleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.055f, 0.065f, 0.075f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(BubbleText, 20.0f);
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::ApplyPresentation()
{
	if (!BubbleText || !BodyImage || !TailImage)
	{
		return;
	}

	BubbleText->SetText(PendingText);
	TunaSweeperUIFont::ApplyFont(BubbleText, 20.0f);

	if (!BodyTexture)
	{
		BodyTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/SpeechBubble/T_SpeechBubble_Body.T_SpeechBubble_Body"));
	}
	if (!TailTexture)
	{
		TailTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/SpeechBubble/T_SpeechBubble_Tail.T_SpeechBubble_Tail"));
	}

	if (BodyTexture)
	{
		BodyImage->SetBrushFromTexture(BodyTexture, false);
		FSlateBrush BodyBrush = BodyImage->GetBrush();
		BodyBrush.DrawAs = ESlateBrushDrawType::Box;
		BodyBrush.Margin = FMargin(0.125f, 0.25f);
		BodyImage->SetBrush(BodyBrush);
	}
	if (TailTexture)
	{
		TailImage->SetBrushFromTexture(TailTexture, false);
	}

	ApplyTailLayout();
	InvalidateLayoutAndVolatility();
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::ApplyTailLayout()
{
	if (!RootOverlay || !BodyBox || !TailBox)
	{
		return;
	}

	const bool bHasTail = TailDirection != ETunaSweeperSpeechBubbleTailDirection::None;
	TailBox->SetVisibility(bHasTail ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (UOverlaySlot* BodySlot = Cast<UOverlaySlot>(BodyBox->Slot))
	{
		BodySlot->SetPadding(TunaSweeperScreenSpaceSpeechBubble::BodyPadding(TailDirection));
		BodySlot->SetHorizontalAlignment(HAlign_Fill);
		BodySlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (UOverlaySlot* TailSlot = Cast<UOverlaySlot>(TailBox->Slot))
	{
		const FVector2D Direction = TunaSweeperScreenSpaceSpeechBubble::DirectionVector(TailDirection);
		TailSlot->SetHorizontalAlignment(TunaSweeperScreenSpaceSpeechBubble::HorizontalAlignment(Direction));
		TailSlot->SetVerticalAlignment(TunaSweeperScreenSpaceSpeechBubble::VerticalAlignment(Direction));
	}

	TailBox->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	TailBox->SetRenderTransformAngle(TunaSweeperScreenSpaceSpeechBubble::TailAngle(TailDirection));
}

FVector2D UTunaSweeperScreenSpaceSpeechBubbleWidget::GetLocalAnchorPoint() const
{
	FVector2D BodySize(TunaSweeperScreenSpaceSpeechBubble::MinimumBodyWidth, TunaSweeperScreenSpaceSpeechBubble::MinimumBodyHeight);
	if (BodyBox)
	{
		const FVector2D DesiredSize = BodyBox->GetDesiredSize();
		BodySize.X = FMath::Max(BodySize.X, DesiredSize.X);
		BodySize.Y = FMath::Max(BodySize.Y, DesiredSize.Y);
	}
	return CalculateLocalAnchorPoint(TailDirection, BodySize);
}

FVector2D UTunaSweeperScreenSpaceSpeechBubbleWidget::CalculateLocalAnchorPoint(
	const ETunaSweeperSpeechBubbleTailDirection InTailDirection,
	const FVector2D& BodySize)
{
	if (InTailDirection == ETunaSweeperSpeechBubbleTailDirection::None)
	{
		return BodySize * 0.5f;
	}

	const FVector2D Direction = TunaSweeperScreenSpaceSpeechBubble::DirectionVector(InTailDirection);
	const FMargin Padding = TunaSweeperScreenSpaceSpeechBubble::BodyPadding(InTailDirection);
	const FVector2D RootSize(BodySize.X + Padding.Left + Padding.Right, BodySize.Y + Padding.Top + Padding.Bottom);
	FVector2D TailCenter = RootSize * 0.5f;
	if (Direction.X < 0.0f)
	{
		TailCenter.X = TunaSweeperScreenSpaceSpeechBubble::TailSize * 0.5f;
	}
	else if (Direction.X > 0.0f)
	{
		TailCenter.X = RootSize.X - TunaSweeperScreenSpaceSpeechBubble::TailSize * 0.5f;
	}
	if (Direction.Y < 0.0f)
	{
		TailCenter.Y = TunaSweeperScreenSpaceSpeechBubble::TailSize * 0.5f;
	}
	else if (Direction.Y > 0.0f)
	{
		TailCenter.Y = RootSize.Y - TunaSweeperScreenSpaceSpeechBubble::TailSize * 0.5f;
	}

	return TailCenter + Direction * TunaSweeperScreenSpaceSpeechBubble::TailTipRadius;
}
