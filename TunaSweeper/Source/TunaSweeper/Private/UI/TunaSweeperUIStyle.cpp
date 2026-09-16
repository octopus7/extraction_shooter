#include "UI/TunaSweeperUIStyle.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
#include "Components/TextBlock.h"
#include "UI/TunaSweeperCheckIndicatorWidget.h"
#include "UI/TunaSweeperUIFont.h"

void TunaSweeperUIStyle::ApplyLabel(UTextBlock* Label, int32 FontSize)
{
	if (!Label) return;
	if (FontSize > 0) TunaSweeperUIFont::ApplyFont(Label, FontSize);
	Label->SetColorAndOpacity(FLinearColor(0.97f, 1.0f, 1.0f));
	Label->SetShadowOffset(FVector2D(1.0f, 1.0f));
	Label->SetShadowColorAndOpacity(FLinearColor(0.005f, 0.035f, 0.05f, 0.55f));
}

void TunaSweeperUIStyle::ApplyButton(UButton* Button, EButtonRole Role, bool bSelected)
{
	if (!Button) return;
	FLinearColor Normal(0.012f, 0.35f, 0.43f);
	FLinearColor Hover(0.025f, 0.49f, 0.58f);
	FLinearColor Pressed(0.008f, 0.23f, 0.29f);
	FMargin Padding(14.0f, 7.0f);
	float Radius = 12.0f;
	if (Role == EButtonRole::Secondary)
	{
		Normal = bSelected ? FLinearColor(0.035f, 0.25f, 0.29f, 0.92f) : FLinearColor(0.035f, 0.18f, 0.22f, 0.88f);
		Hover = bSelected ? FLinearColor(0.055f, 0.34f, 0.39f) : FLinearColor(0.055f, 0.30f, 0.35f);
	}
	else if (Role == EButtonRole::Danger)
	{
		Normal = FLinearColor(0.42f, 0.085f, 0.075f);
		Hover = FLinearColor(0.58f, 0.14f, 0.11f);
		Pressed = FLinearColor(0.28f, 0.045f, 0.04f);
	}
	else if (Role == EButtonRole::Icon || Role == EButtonRole::Tab)
	{
		Normal = bSelected ? Normal : FLinearColor(0.035f, 0.10f, 0.12f, 0.45f);
		Hover = FLinearColor(0.035f, 0.36f, 0.43f, 0.92f);
		Padding = Role == EButtonRole::Icon ? FMargin(4.0f) : FMargin(12.0f, 7.0f);
		Radius = 8.0f;
	}
	FButtonStyle Style = Button->GetStyle();
	Style.SetNormal(FSlateRoundedBoxBrush(Normal, Radius));
	Style.SetHovered(FSlateRoundedBoxBrush(Hover, Radius));
	Style.SetPressed(FSlateRoundedBoxBrush(Pressed, Radius));
	Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor(0.075f, 0.13f, 0.15f, 0.65f), Radius));
	Style.SetNormalPadding(Padding);
	Style.SetPressedPadding(Padding + FMargin(0.0f, 1.0f, 0.0f, -1.0f));
	Style.SetNormalForeground(FLinearColor::White);
	Style.SetHoveredForeground(FLinearColor::White);
	Style.SetPressedForeground(FLinearColor::White);
	Style.SetDisabledForeground(FLinearColor(0.55f, 0.66f, 0.68f));
	Button->SetStyle(Style);
	Button->SetBackgroundColor(FLinearColor::White);
	Button->SetColorAndOpacity(FLinearColor::White);
	ApplyLabel(Cast<UTextBlock>(Button->GetContent()));
}

void TunaSweeperUIStyle::SetCheckButton(UWidgetTree* Tree, UButton* Button, UTextBlock* Label, bool bChecked)
{
	if (!Tree || !Button || !Label) return;
	// Title controls can live in a nested authored WBP, not in the caller's root tree.
	if (UWidgetTree* OwnerTree = Button->GetTypedOuter<UWidgetTree>()) Tree = OwnerTree;
	const FName IndicatorName(*(Button->GetName() + TEXT("_CheckIndicator")));
	UHorizontalBox* ExistingContent = Cast<UHorizontalBox>(Button->GetContent());
	UTunaSweeperCheckIndicatorWidget* Indicator = ExistingContent && ExistingContent->GetChildrenCount() > 0
		? Cast<UTunaSweeperCheckIndicatorWidget>(ExistingContent->GetChildAt(0)) : nullptr;
	if (!Indicator)
	{
		Indicator = Tree->ConstructWidget<UTunaSweeperCheckIndicatorWidget>(UTunaSweeperCheckIndicatorWidget::StaticClass(), IndicatorName);
		Indicator->SetVisibility(ESlateVisibility::HitTestInvisible);
		UHorizontalBox* Content = Tree->ConstructWidget<UHorizontalBox>();
		Label->RemoveFromParent();
		Content->AddChildToHorizontalBox(Indicator)->SetVerticalAlignment(VAlign_Center);
		UScaleBox* LabelScale = Tree->ConstructWidget<UScaleBox>();
		LabelScale->SetStretch(EStretch::ScaleToFit);
		LabelScale->SetStretchDirection(EStretchDirection::DownOnly);
		LabelScale->SetContent(Label);
		if (UScaleBoxSlot* TextSlot = Cast<UScaleBoxSlot>(Label->Slot))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Left);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(LabelScale);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		LabelSlot->SetPadding(FMargin(12.0f, 0.0f, 2.0f, 0.0f));
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		Button->SetContent(Content);
	}
	Indicator->SetChecked(bChecked);
	if (UButtonSlot* ContentSlot = Cast<UButtonSlot>(Button->GetContent()->Slot))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Fill);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
	}
	FButtonStyle Style = Button->GetStyle();
	Style.SetNormal(FSlateRoundedBoxBrush(FLinearColor::Transparent, 8.0f));
	Style.SetHovered(FSlateRoundedBoxBrush(FLinearColor(0.10f, 0.31f, 0.34f, 0.22f), 8.0f));
	Style.SetPressed(FSlateRoundedBoxBrush(FLinearColor(0.04f, 0.20f, 0.23f, 0.36f), 8.0f));
	Style.SetDisabled(FSlateRoundedBoxBrush(FLinearColor::Transparent, 8.0f));
	Style.SetNormalPadding(FMargin(6.0f));
	Style.SetPressedPadding(FMargin(6.0f, 7.0f, 6.0f, 5.0f));
	Button->SetStyle(Style);
	Button->SetBackgroundColor(FLinearColor::White);
	Button->SetColorAndOpacity(FLinearColor::White);
	ApplyLabel(Label);
	Label->SetJustification(ETextJustify::Left);
}
