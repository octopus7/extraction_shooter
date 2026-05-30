#include "UI/TunaSweeperInteractionMarkerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UI/TunaSweeperUIFont.h"

namespace
{
	const TCHAR* OpenedCheckTexturePath = TEXT("/Game/UI/Interaction/T_InteractionMarkerOpenedCheck.T_InteractionMarkerOpenedCheck");

	void ApplyPaintOpacity(UWidget* Widget, float Opacity)
	{
		if (!Widget)
		{
			return;
		}

		const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);

		if (UImage* Image = Cast<UImage>(Widget))
		{
			Image->SetRenderOpacity(1.0f);
			Image->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, ClampedOpacity));
			Image->SetBrushTintColor(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, ClampedOpacity)));
			Image->SetOpacity(ClampedOpacity);
			return;
		}

		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			FLinearColor TextColor = TextBlock->GetColorAndOpacity().GetSpecifiedColor();
			TextColor.A = ClampedOpacity;
			TextBlock->SetColorAndOpacity(FSlateColor(TextColor));

			FLinearColor ShadowColor = TextBlock->GetShadowColorAndOpacity();
			ShadowColor.A = ClampedOpacity;
			TextBlock->SetShadowColorAndOpacity(ShadowColor);
			TextBlock->SetRenderOpacity(1.0f);
			return;
		}

		if (UBorder* Border = Cast<UBorder>(Widget))
		{
			FLinearColor BrushColor = Border->GetBrushColor();
			BrushColor.A = ClampedOpacity;
			Border->SetBrushColor(BrushColor);
			Border->SetRenderOpacity(1.0f);
			return;
		}

		Widget->SetRenderOpacity(ClampedOpacity);
	}

	FSlateBrush BuildBrushWithPaintOpacity(const FSlateBrush& SourceBrush, float Opacity)
	{
		FSlateBrush Brush = SourceBrush;
		const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);

		FLinearColor TintColor = Brush.TintColor.GetSpecifiedColor();
		TintColor.A *= ClampedOpacity;
		Brush.TintColor = FSlateColor(TintColor);

		FLinearColor OutlineColor = Brush.OutlineSettings.Color.GetSpecifiedColor();
		OutlineColor.A *= ClampedOpacity;
		Brush.OutlineSettings.Color = FSlateColor(OutlineColor);

		return Brush;
	}

	FSlateBrush BuildRoundedBrush(const FVector2D& ImageSize, const FLinearColor& FillColor, float Radius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(FLinearColor::Transparent), 0.0f);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FSlateBrush BuildRoundedOutlineBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float Radius,
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

	FSlateBrush BuildCircularBrush(const FVector2D& ImageSize, const FLinearColor& FillColor)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		Brush.OutlineSettings.Color = FSlateColor(FLinearColor::Transparent);
		Brush.OutlineSettings.Width = 0.0f;
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	bool TextArraysEqual(const TArray<FText>& LeftTexts, const TArray<FText>& RightTexts)
	{
		if (LeftTexts.Num() != RightTexts.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < LeftTexts.Num(); ++Index)
		{
			if (!LeftTexts[Index].EqualTo(RightTexts[Index]))
			{
				return false;
			}
		}

		return true;
	}
}

void UTunaSweeperInteractionMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheNamedWidgets();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	CacheNamedWidgets();

	if (IsDesignTime())
	{
		CachedAlpha = 1.0f;
		CachedRingScale = 1.0f;
		CachedLabelAlpha = 1.0f;
	}

	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetMarkerText(const FText& InText)
{
	CachedDisplayText = InText;
	if (CachedOptionTexts.Num() > 0 || CachedFocusedOptionIndex != INDEX_NONE)
	{
		CachedOptionTexts.Reset();
		CachedFocusedOptionIndex = INDEX_NONE;
		bMultiOptionListDirty = true;
	}
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetInteractionOptions(const TArray<FText>& InOptions, int32 InFocusedOptionIndex)
{
	const int32 NewFocusedOptionIndex = InOptions.IsValidIndex(InFocusedOptionIndex)
		? InFocusedOptionIndex
		: 0;
	const bool bOptionsChanged = !TextArraysEqual(CachedOptionTexts, InOptions);
	const bool bFocusChanged = CachedFocusedOptionIndex != NewFocusedOptionIndex;

	CachedOptionTexts = InOptions;
	CachedFocusedOptionIndex = NewFocusedOptionIndex;
	if (CachedOptionTexts.IsValidIndex(CachedFocusedOptionIndex))
	{
		CachedDisplayText = CachedOptionTexts[CachedFocusedOptionIndex];
	}
	bMultiOptionListDirty = bMultiOptionListDirty || bOptionsChanged || bFocusChanged;
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetRequirementPreview(
	UTexture2D* InIconTexture,
	int32 InRequiredQuantity,
	bool bInShowRequirement)
{
	CachedRequirementIconTexture = InIconTexture;
	CachedRequiredQuantity = FMath::Max(0, InRequiredQuantity);
	bCachedShowRequirement = bInShowRequirement;
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetMarkerPresentation(float InAlpha, float InRingScale, float InLabelAlpha)
{
	CachedAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);
	CachedRingScale = FMath::Max(InRingScale, 0.01f);
	CachedLabelAlpha = FMath::Clamp(InLabelAlpha, 0.0f, 1.0f);
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetMarkerOpened(bool bInOpened)
{
	bCachedOpened = bInOpened;
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!MarkerRoot)
	{
		MarkerRoot = WidgetTree->FindWidget(TEXT("MarkerRoot"));
	}

	if (!RingImage)
	{
		RingImage = WidgetTree->FindWidget(TEXT("RingImage"));
		if (!RingImage)
		{
			RingImage = WidgetTree->FindWidget(TEXT("RingText"));
		}
	}
	if (!RingBrushImage)
	{
		RingBrushImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("RingBrushImage")));
		if (!RingBrushImage)
		{
			RingBrushImage = Cast<UImage>(RingImage);
		}
	}
	if (!bHasCachedRingBrush)
	{
		if (RingBrushImage)
		{
			CachedRingBrush = RingBrushImage->GetBrush();
			bHasCachedRingBrush = true;
		}
	}

	if (!FilledImage)
	{
		FilledImage = WidgetTree->FindWidget(TEXT("FilledImage"));
		if (!FilledImage)
		{
			FilledImage = WidgetTree->FindWidget(TEXT("FilledText"));
		}
	}
	if (!FilledBrushImage)
	{
		FilledBrushImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("FilledBrushImage")));
		if (!FilledBrushImage)
		{
			FilledBrushImage = Cast<UImage>(FilledImage);
		}
	}
	if (!bHasCachedFilledBrush)
	{
		if (FilledBrushImage)
		{
			CachedFilledBrush = FilledBrushImage->GetBrush();
			bHasCachedFilledBrush = true;
		}
	}

	if (!LabelBackground)
	{
		LabelBackground = Cast<UBorder>(WidgetTree->FindWidget(TEXT("LabelBackground")));
	}

	if (!DisplayNameText)
	{
		DisplayNameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DisplayNameText")));
	}

	if (!RequirementRoot)
	{
		RequirementRoot = WidgetTree->FindWidget(TEXT("RequirementRoot"));
	}

	if (!RequirementIconImage)
	{
		RequirementIconImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("RequirementIconImage")));
	}

	if (!RequirementQuantityText)
	{
		RequirementQuantityText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("RequirementQuantityText")));
	}

	EnsureRequirementWidgets();
}

UTexture2D* UTunaSweeperInteractionMarkerWidget::ResolveOpenedCheckTexture()
{
	if (!CachedOpenedCheckTexture)
	{
		CachedOpenedCheckTexture = LoadObject<UTexture2D>(nullptr, OpenedCheckTexturePath);
	}

	return CachedOpenedCheckTexture;
}

void UTunaSweeperInteractionMarkerWidget::EnsureRequirementWidgets()
{
	if (!WidgetTree || !LabelBackground || (RequirementRoot && RequirementIconImage && RequirementQuantityText))
	{
		return;
	}

	if (!LabelContentRow)
	{
		LabelContentRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("LabelContentRow")));
	}
	if (!LabelContentRow)
	{
		LabelContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LabelContentRow_Runtime"));
		if (!LabelContentRow)
		{
			return;
		}

		UWidget* ExistingContent = LabelBackground->GetContent();
		LabelBackground->SetContent(LabelContentRow);
		if (DisplayNameText && ExistingContent == DisplayNameText)
		{
			if (UHorizontalBoxSlot* DisplayNameSlot = LabelContentRow->AddChildToHorizontalBox(DisplayNameText))
			{
				DisplayNameSlot->SetHorizontalAlignment(HAlign_Left);
				DisplayNameSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	UHorizontalBox* RequirementHorizontalBox = Cast<UHorizontalBox>(RequirementRoot.Get());
	if (!RequirementHorizontalBox)
	{
		RequirementHorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			TEXT("RequirementRoot_Runtime"));
		RequirementRoot = RequirementHorizontalBox;
	}

	if (!RequirementHorizontalBox)
	{
		return;
	}

	if (!RequirementIconImage)
	{
		USizeBox* RequirementIconBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			TEXT("RequirementIconBox_Runtime"));
		RequirementIconImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			TEXT("RequirementIconImage_Runtime"));
		if (RequirementIconBox && RequirementIconImage)
		{
			RequirementIconBox->SetWidthOverride(22.0f);
			RequirementIconBox->SetHeightOverride(22.0f);
			RequirementIconBox->SetContent(RequirementIconImage);
			if (UHorizontalBoxSlot* IconSlot = RequirementHorizontalBox->AddChildToHorizontalBox(RequirementIconBox))
			{
				IconSlot->SetHorizontalAlignment(HAlign_Center);
				IconSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	if (!RequirementQuantityText)
	{
		RequirementQuantityText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("RequirementQuantityText_Runtime"));
		if (RequirementQuantityText)
		{
			if (DisplayNameText)
			{
				FSlateFontInfo QuantityFont = DisplayNameText->GetFont();
				QuantityFont.Size = FMath::Max(1, QuantityFont.Size - 1);
				RequirementQuantityText->SetFont(QuantityFont);
				RequirementQuantityText->SetColorAndOpacity(DisplayNameText->GetColorAndOpacity());
			}
			RequirementQuantityText->SetJustification(ETextJustify::Left);

			if (UHorizontalBoxSlot* QuantitySlot = RequirementHorizontalBox->AddChildToHorizontalBox(RequirementQuantityText))
			{
				QuantitySlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
				QuantitySlot->SetHorizontalAlignment(HAlign_Left);
				QuantitySlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	if (RequirementRoot && RequirementRoot->GetParent() != LabelContentRow)
	{
		if (UHorizontalBoxSlot* RequirementSlot = LabelContentRow->AddChildToHorizontalBox(RequirementRoot))
		{
			RequirementSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
			RequirementSlot->SetHorizontalAlignment(HAlign_Left);
			RequirementSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void UTunaSweeperInteractionMarkerWidget::EnsureSingleLabelContent()
{
	if (!WidgetTree || !LabelBackground)
	{
		return;
	}

	if (!LabelContentRow)
	{
		LabelContentRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("LabelContentRow")));
	}

	if (LabelContentRow && LabelBackground->GetContent() != LabelContentRow)
	{
		LabelBackground->SetContent(LabelContentRow);
	}

	if (DisplayNameText)
	{
		DisplayNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (MultiOptionListRoot)
	{
		MultiOptionListRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperInteractionMarkerWidget::EnsureMultiOptionList()
{
	if (!WidgetTree || !LabelBackground)
	{
		return;
	}

	if (!MultiOptionListRoot)
	{
		MultiOptionListRoot = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("MultiOptionListRoot")));
	}

	if (!MultiOptionListRoot)
	{
		MultiOptionListRoot = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			TEXT("MultiOptionListRoot_Runtime"));
		bMultiOptionListDirty = true;
	}

	if (MultiOptionListRoot && LabelBackground->GetContent() != MultiOptionListRoot)
	{
		LabelBackground->SetContent(MultiOptionListRoot);
	}

	if (DisplayNameText)
	{
		DisplayNameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (MultiOptionListRoot)
	{
		MultiOptionListRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UTunaSweeperInteractionMarkerWidget::RebuildMultiOptionList()
{
	if (!WidgetTree || !MultiOptionListRoot)
	{
		return;
	}

	MultiOptionListRoot->ClearChildren();

	const FSlateBrush DotBrush = BuildCircularBrush(FVector2D(8.0f, 8.0f), FLinearColor::White);
	const FSlateBrush FocusBackgroundBrush = BuildRoundedBrush(
		FVector2D(128.0f, 24.0f),
		FLinearColor(1.0f, 1.0f, 1.0f, 0.95f),
		2.0f);
	const FSlateBrush TransparentBrush = BuildRoundedBrush(
		FVector2D(128.0f, 24.0f),
		FLinearColor::Transparent,
		2.0f);

	for (int32 OptionIndex = 0; OptionIndex < CachedOptionTexts.Num(); ++OptionIndex)
	{
		const bool bFocused = OptionIndex == CachedFocusedOptionIndex;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionRow_%d"), OptionIndex));
		USizeBox* IndicatorBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionIndicatorBox_%d"), OptionIndex));
		UOverlay* IndicatorOverlay = WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionIndicator_%d"), OptionIndex));
		UImage* DotImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionDot_%d"), OptionIndex));
		UTextBlock* UpIndicatorText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionUp_%d"), OptionIndex));
		UTextBlock* DownIndicatorText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionDown_%d"), OptionIndex));
		UBorder* OptionBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionBackground_%d"), OptionIndex));
		UHorizontalBox* OptionContent = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionContent_%d"), OptionIndex));
		UTextBlock* OptionText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionText_%d"), OptionIndex));
		UBorder* KeyPromptBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionKeyBackground_%d"), OptionIndex));
		UTextBlock* KeyPromptText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionKey_%d"), OptionIndex));

		if (!Row || !IndicatorBox || !IndicatorOverlay || !DotImage || !UpIndicatorText ||
			!DownIndicatorText || !OptionBackground || !OptionContent || !OptionText ||
			!KeyPromptBackground || !KeyPromptText)
		{
			continue;
		}

		IndicatorBox->SetWidthOverride(20.0f);
		IndicatorBox->SetHeightOverride(24.0f);
		IndicatorBox->SetContent(IndicatorOverlay);

		DotImage->SetBrush(DotBrush);
		DotImage->SetColorAndOpacity(FLinearColor::White);
		if (UOverlaySlot* DotSlot = IndicatorOverlay->AddChildToOverlay(DotImage))
		{
			DotSlot->SetHorizontalAlignment(HAlign_Center);
			DotSlot->SetVerticalAlignment(VAlign_Center);
		}

		const FLinearColor IndicatorColor = FLinearColor::White;
		UpIndicatorText->SetText(FText::FromString(TEXT("^")));
		UpIndicatorText->SetColorAndOpacity(FSlateColor(IndicatorColor));
		UpIndicatorText->SetJustification(ETextJustify::Center);
		UpIndicatorText->SetVisibility(bFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (DisplayNameText)
		{
			FSlateFontInfo IndicatorFont = DisplayNameText->GetFont();
			IndicatorFont.Size = 9;
			UpIndicatorText->SetFont(IndicatorFont);
		}
		if (UOverlaySlot* UpSlot = IndicatorOverlay->AddChildToOverlay(UpIndicatorText))
		{
			UpSlot->SetHorizontalAlignment(HAlign_Center);
			UpSlot->SetVerticalAlignment(VAlign_Top);
		}

		DownIndicatorText->SetText(FText::FromString(TEXT("v")));
		DownIndicatorText->SetColorAndOpacity(FSlateColor(IndicatorColor));
		DownIndicatorText->SetJustification(ETextJustify::Center);
		DownIndicatorText->SetVisibility(bFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (DisplayNameText)
		{
			FSlateFontInfo IndicatorFont = DisplayNameText->GetFont();
			IndicatorFont.Size = 9;
			DownIndicatorText->SetFont(IndicatorFont);
		}
		if (UOverlaySlot* DownSlot = IndicatorOverlay->AddChildToOverlay(DownIndicatorText))
		{
			DownSlot->SetHorizontalAlignment(HAlign_Center);
			DownSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		if (UHorizontalBoxSlot* IndicatorSlot = Row->AddChildToHorizontalBox(IndicatorBox))
		{
			IndicatorSlot->SetHorizontalAlignment(HAlign_Center);
			IndicatorSlot->SetVerticalAlignment(VAlign_Center);
		}

		OptionBackground->SetBrush(bFocused ? FocusBackgroundBrush : TransparentBrush);
		OptionBackground->SetPadding(bFocused ? FMargin(7.0f, 1.0f, 6.0f, 1.0f) : FMargin(7.0f, 1.0f, 6.0f, 1.0f));
		OptionBackground->SetContent(OptionContent);

		OptionText->SetText(CachedOptionTexts[OptionIndex]);
		OptionText->SetJustification(ETextJustify::Left);
		if (DisplayNameText)
		{
			OptionText->SetFont(DisplayNameText->GetFont());
		}
		OptionText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		if (UHorizontalBoxSlot* TextSlot = OptionContent->AddChildToHorizontalBox(OptionText))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Left);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}

		KeyPromptBackground->SetVisibility(bFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		KeyPromptBackground->SetBrush(BuildRoundedOutlineBrush(
			FVector2D(22.0f, 20.0f),
			FLinearColor::White,
			FLinearColor::Black,
			5.0f,
			1.0f));
		KeyPromptBackground->SetPadding(FMargin(5.0f, 0.0f, 5.0f, 0.0f));
		KeyPromptText->SetText(FText::FromString(TEXT("F")));
		KeyPromptText->SetJustification(ETextJustify::Center);
		if (DisplayNameText)
		{
			FSlateFontInfo KeyFont = DisplayNameText->GetFont();
			KeyFont.Size = FMath::Max(1, KeyFont.Size - 2);
			KeyPromptText->SetFont(KeyFont);
		}
		KeyPromptText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		KeyPromptBackground->SetContent(KeyPromptText);
		if (UHorizontalBoxSlot* KeySlot = OptionContent->AddChildToHorizontalBox(KeyPromptBackground))
		{
			KeySlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			KeySlot->SetHorizontalAlignment(HAlign_Left);
			KeySlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UHorizontalBoxSlot* BackgroundSlot = Row->AddChildToHorizontalBox(OptionBackground))
		{
			BackgroundSlot->SetHorizontalAlignment(HAlign_Left);
			BackgroundSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UVerticalBoxSlot* RowSlot = MultiOptionListRoot->AddChildToVerticalBox(Row))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Left);
			RowSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	bMultiOptionListDirty = false;
}

void UTunaSweeperInteractionMarkerWidget::ApplyState()
{
	SetRenderOpacity(1.0f);
	const bool bUseMultiOptionList = CachedOptionTexts.Num() > 1;

	if (MarkerRoot)
	{
		MarkerRoot->SetRenderOpacity(1.0f);
	}

	if (RingImage)
	{
		RingImage->SetRenderScale(FVector2D(CachedRingScale));
		RingImage->SetRenderOpacity(1.0f);
	}
	if (RingBrushImage)
	{
		if (!bHasCachedRingBrush)
		{
			CachedRingBrush = RingBrushImage->GetBrush();
			bHasCachedRingBrush = true;
		}
		RingBrushImage->SetBrush(BuildBrushWithPaintOpacity(CachedRingBrush, CachedAlpha));
		RingBrushImage->SetRenderOpacity(1.0f);
		RingBrushImage->SetColorAndOpacity(FLinearColor::White);
	}
	else if (RingImage)
	{
		ApplyPaintOpacity(RingImage, CachedAlpha);
	}

	if (FilledImage)
	{
		FilledImage->SetRenderOpacity(1.0f);
	}
	if (FilledBrushImage)
	{
		if (!bHasCachedFilledBrush)
		{
			CachedFilledBrush = FilledBrushImage->GetBrush();
			bHasCachedFilledBrush = true;
		}

		if (bCachedOpened)
		{
			if (UTexture2D* OpenedCheckTexture = ResolveOpenedCheckTexture())
			{
				FilledBrushImage->SetBrushFromTexture(OpenedCheckTexture, false);
			}
		}
		else if (bHasCachedFilledBrush)
		{
			FilledBrushImage->SetBrush(BuildBrushWithPaintOpacity(CachedFilledBrush, CachedAlpha));
			FilledBrushImage->SetRenderOpacity(1.0f);
			FilledBrushImage->SetColorAndOpacity(FLinearColor::White);
		}
		if (bCachedOpened)
		{
			ApplyPaintOpacity(FilledBrushImage, CachedAlpha);
		}
	}
	else if (FilledImage)
	{
		ApplyPaintOpacity(FilledImage, CachedAlpha);
	}

	if (bUseMultiOptionList)
	{
		EnsureMultiOptionList();
		if (bMultiOptionListDirty)
		{
			RebuildMultiOptionList();
		}

		if (DisplayNameText)
		{
			DisplayNameText->SetText(CachedDisplayText);
			DisplayNameText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (MultiOptionListRoot)
		{
			MultiOptionListRoot->SetRenderOpacity(CachedLabelAlpha);
		}
		if (LabelBackground)
		{
			LabelBackground->SetBrushColor(FLinearColor::Transparent);
			LabelBackground->SetRenderOpacity(1.0f);
		}
	}
	else
	{
		EnsureSingleLabelContent();

		if (DisplayNameText)
		{
			DisplayNameText->SetText(CachedDisplayText);
			ApplyPaintOpacity(DisplayNameText, CachedLabelAlpha);
		}

		if (LabelBackground)
		{
			LabelBackground->SetBrushColor(FLinearColor::White);
			ApplyPaintOpacity(LabelBackground, CachedLabelAlpha);
		}
	}

	const bool bShowRequirement = bCachedShowRequirement && CachedRequiredQuantity > 0;
	if (RequirementRoot)
	{
		RequirementRoot->SetVisibility(bShowRequirement ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		ApplyPaintOpacity(RequirementRoot, CachedLabelAlpha);
	}

	if (RequirementIconImage)
	{
		if (bShowRequirement && CachedRequirementIconTexture)
		{
			RequirementIconImage->SetBrushFromTexture(CachedRequirementIconTexture, true);
			RequirementIconImage->SetOpacity(1.0f);
		}
		else
		{
			RequirementIconImage->SetBrushFromTexture(nullptr, false);
			RequirementIconImage->SetOpacity(0.0f);
		}
		ApplyPaintOpacity(RequirementIconImage, CachedLabelAlpha);
	}

	if (RequirementQuantityText)
	{
		RequirementQuantityText->SetText(
			bShowRequirement
				? FText::Format(FText::FromString(TEXT("x{0}")), FText::AsNumber(CachedRequiredQuantity))
				: FText::GetEmpty());
		ApplyPaintOpacity(RequirementQuantityText, CachedLabelAlpha);
	}
}
