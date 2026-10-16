#include "UI/TunaSweeperScreenSpaceSpeechBubbleWidget.h"

#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/GameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperScreenSpaceSpeechBubble
{
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

	FMargin BodyPadding(const ETunaSweeperSpeechBubbleTailDirection Direction, const float TailReserve)
	{
		const FVector2D Vector = DirectionVector(Direction);
		return FMargin(
			Vector.X < 0.0f ? TailReserve : 0.0f,
			Vector.Y < 0.0f ? TailReserve : 0.0f,
			Vector.X > 0.0f ? TailReserve : 0.0f,
			Vector.Y > 0.0f ? TailReserve : 0.0f);
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
	bHasPendingText = true;
	TailDirection = InTailDirection;
	ApplyPresentation();
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime())
	{
		TailDirection = PreviewTailDirection;
		TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
		TStrongObjectPtr<UTunaSweeperTextSubsystem> Strings(NewObject<UTunaSweeperTextSubsystem>(Owner.Get()));
		FText PreviewText;
		Strings->TryGetTextByKey(PreviewTextStringKey, ETunaSweeperItemTextLanguage::Korean, PreviewText);
		if (BubbleText) BubbleText->SetText(PreviewText);
	}
	ApplyPresentation();
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::ApplyPresentation()
{
	// Fonts, brushes, colors and sizing belong to the serialized WBP hierarchy.
	if (BubbleText && bHasPendingText) BubbleText->SetText(PendingText);
	ApplyTailLayout();
	// The Designer does not route NativeTick. Measure the rebuilt children before
	// initializing the material, rather than leaving its dimensions at zero.
	if (BodyBox) BodyBox->ForceLayoutPrepass();
	if (TailBox) TailBox->ForceLayoutPrepass();
	UpdateShapeMaterial();
	InvalidateLayoutAndVolatility();
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::ApplyTailLayout()
{
	if (!BodyBox || !TailBox)
	{
		return;
	}

	const bool bHasTail = TailDirection != ETunaSweeperSpeechBubbleTailDirection::None;
	TailBox->SetVisibility(bHasTail ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if (UOverlaySlot* BodySlot = Cast<UOverlaySlot>(BodyBox->Slot))
	{
		BodySlot->SetPadding(TunaSweeperScreenSpaceSpeechBubble::BodyPadding(TailDirection, FMath::Max(0.0f, TailReserve)));
		BodySlot->SetHorizontalAlignment(HAlign_Fill);
		BodySlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (UOverlaySlot* TailSlot = Cast<UOverlaySlot>(TailBox->Slot))
	{
		const FVector2D Direction = TunaSweeperScreenSpaceSpeechBubble::DirectionVector(TailDirection);
		TailSlot->SetHorizontalAlignment(TunaSweeperScreenSpaceSpeechBubble::HorizontalAlignment(Direction));
		TailSlot->SetVerticalAlignment(TunaSweeperScreenSpaceSpeechBubble::VerticalAlignment(Direction));
	}

	// TailBox is a designer sizing guide. The background draws the union of the
	// body and tail in one pass, so there is never an internal outline at the join.
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UpdateShapeMaterial();
}

void UTunaSweeperScreenSpaceSpeechBubbleWidget::UpdateShapeMaterial() const
{
	if (!BodyImage || !BodyBox || !TailBox) return;
	auto* Material = BodyImage->GetDynamicMaterial();
	if (!Material) return;
	const FVector2D BodySize = BodyBox->GetDesiredSize();
	const FVector2D TailSize = TailBox->GetDesiredSize();
	const bool bHasTail = TailDirection != ETunaSweeperSpeechBubbleTailDirection::None;
	const float Reserve = FMath::Max(0.0f, TailReserve);
	const FMargin TailPadding = TunaSweeperScreenSpaceSpeechBubble::BodyPadding(TailDirection, Reserve);
	const FVector2D Size(
		FMath::Max(BodySize.X + TailPadding.Left + TailPadding.Right, bHasTail ? TailSize.X : 0.0),
		FMath::Max(BodySize.Y + TailPadding.Top + TailPadding.Bottom, bHasTail ? TailSize.Y : 0.0));
	const FVector2D Direction = TunaSweeperScreenSpaceSpeechBubble::DirectionVector(TailDirection);
	const FVector2D Tip = CalculateLocalAnchorPoint(TailDirection, BodySize, TailSize, Reserve, FMath::Max(0.0f, TailTipInset));
	Material->SetVectorParameterValue(TEXT("WidgetSize"), FLinearColor(Size.X, Size.Y, 0, 0));
	Material->SetVectorParameterValue(TEXT("BodyPadding"), FLinearColor(TailPadding.Left, TailPadding.Top, TailPadding.Right, TailPadding.Bottom));
	Material->SetVectorParameterValue(TEXT("TailDirection"), FLinearColor(Direction.X, Direction.Y, bHasTail ? 1.0f : 0.0f, 0));
	Material->SetVectorParameterValue(TEXT("TailGeometry"), FLinearColor(Tip.X, Tip.Y, TailSize.X, TailSize.Y));
}

FVector2D UTunaSweeperScreenSpaceSpeechBubbleWidget::GetLocalAnchorPoint() const
{
	UpdateShapeMaterial();
	const FVector2D BodySize = BodyBox ? BodyBox->GetDesiredSize() : FVector2D::ZeroVector;
	const FVector2D TailSize = TailBox ? TailBox->GetDesiredSize() : FVector2D::ZeroVector;
	return CalculateLocalAnchorPoint(TailDirection, BodySize, TailSize,
		FMath::Max(0.0f, TailReserve), FMath::Max(0.0f, TailTipInset));
}

FVector2D UTunaSweeperScreenSpaceSpeechBubbleWidget::CalculateLocalAnchorPoint(
	const ETunaSweeperSpeechBubbleTailDirection InTailDirection,
	const FVector2D& BodySize,
	const FVector2D& TailSize,
	const float InTailReserve,
	const float InTailTipInset)
{
	if (InTailDirection == ETunaSweeperSpeechBubbleTailDirection::None)
	{
		return BodySize * 0.5f;
	}

	const FVector2D Direction = TunaSweeperScreenSpaceSpeechBubble::DirectionVector(InTailDirection);
	const FMargin TailPadding = TunaSweeperScreenSpaceSpeechBubble::BodyPadding(InTailDirection, InTailReserve);
	const FVector2D RootSize(
		FMath::Max(BodySize.X + TailPadding.Left + TailPadding.Right, TailSize.X),
		FMath::Max(BodySize.Y + TailPadding.Top + TailPadding.Bottom, TailSize.Y));
	FVector2D TailCenter = RootSize * 0.5f;
	if (Direction.X < 0.0f)
	{
		TailCenter.X = TailSize.X * 0.5f;
	}
	else if (Direction.X > 0.0f)
	{
		TailCenter.X = RootSize.X - TailSize.X * 0.5f;
	}
	if (Direction.Y < 0.0f)
	{
		TailCenter.Y = TailSize.Y * 0.5f;
	}
	else if (Direction.Y > 0.0f)
	{
		TailCenter.Y = RootSize.Y - TailSize.Y * 0.5f;
	}

	const FVector2D TipRadius(
		FMath::Max(0.0, TailSize.X * 0.5 - InTailTipInset),
		FMath::Max(0.0, TailSize.Y * 0.5 - InTailTipInset));
	return TailCenter + Direction * TipRadius;
}
