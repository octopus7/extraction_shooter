#include "UI/TunaSweeperInteractionMarkerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "UI/TunaSweeperUIFont.h"

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

	if (!FilledImage)
	{
		FilledImage = WidgetTree->FindWidget(TEXT("FilledImage"));
		if (!FilledImage)
		{
			FilledImage = WidgetTree->FindWidget(TEXT("FilledText"));
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

void UTunaSweeperInteractionMarkerWidget::EnsureRequirementWidgets()
{
	if (!WidgetTree || !LabelBackground || (RequirementRoot && RequirementIconImage && RequirementQuantityText))
	{
		return;
	}

	UHorizontalBox* LabelContentRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("LabelContentRow")));
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

void UTunaSweeperInteractionMarkerWidget::ApplyState()
{
	SetRenderOpacity(CachedAlpha);

	if (MarkerRoot)
	{
		MarkerRoot->SetRenderOpacity(CachedAlpha);
	}

	if (RingImage)
	{
		RingImage->SetRenderScale(FVector2D(CachedRingScale));
		RingImage->SetRenderOpacity(CachedAlpha);
	}

	if (FilledImage)
	{
		FilledImage->SetRenderOpacity(CachedAlpha);
	}

	if (DisplayNameText)
	{
		DisplayNameText->SetText(CachedDisplayText);
		DisplayNameText->SetRenderOpacity(CachedLabelAlpha);
	}

	if (LabelBackground)
	{
		LabelBackground->SetRenderOpacity(CachedLabelAlpha);
	}

	const bool bShowRequirement = bCachedShowRequirement && CachedRequiredQuantity > 0;
	if (RequirementRoot)
	{
		RequirementRoot->SetVisibility(bShowRequirement ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		RequirementRoot->SetRenderOpacity(CachedLabelAlpha);
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
		RequirementIconImage->SetRenderOpacity(CachedLabelAlpha);
	}

	if (RequirementQuantityText)
	{
		RequirementQuantityText->SetText(
			bShowRequirement
				? FText::Format(FText::FromString(TEXT("x{0}")), FText::AsNumber(CachedRequiredQuantity))
				: FText::GetEmpty());
		RequirementQuantityText->SetRenderOpacity(CachedLabelAlpha);
	}
}
