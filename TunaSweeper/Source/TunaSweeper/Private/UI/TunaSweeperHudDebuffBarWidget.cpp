#include "UI/TunaSweeperHudDebuffBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Styling/SlateBrush.h"
#include "Subsystem/TunaSweeperDebuffDataSubsystem.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperHudDebuffBar
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
}

void UTunaSweeperHudDebuffBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureDebuffRow();
	RefreshEntryTexts();
}

void UTunaSweeperHudDebuffBarWidget::SetActiveDebuffs(
	const TArray<FTunaSweeperActiveDebuffState>& InActiveDebuffs)
{
	ActiveDebuffs = InActiveDebuffs;
	ActiveDebuffs.Sort(
		[](const FTunaSweeperActiveDebuffState& Left, const FTunaSweeperActiveDebuffState& Right)
		{
			return Left.AppliedOrder < Right.AppliedOrder;
		});

	EnsureDebuffRow();
	if (!DoesEntryLayoutMatch())
	{
		RebuildEntries();
	}
	else
	{
		RefreshEntryTexts();
	}
}

void UTunaSweeperHudDebuffBarWidget::EnsureDebuffRow()
{
	if (DebuffRow)
	{
		return;
	}

	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}
	if (!WidgetTree)
	{
		return;
	}

	DebuffRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("DebuffRow"))));
	if (DebuffRow)
	{
		return;
	}

	DebuffRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("DebuffRowRoot"))));
	if (DebuffRow)
	{
		return;
	}

	DebuffRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DebuffRow"));
	if (DebuffRow)
	{
		WidgetTree->RootWidget = DebuffRow;
	}
}

void UTunaSweeperHudDebuffBarWidget::RebuildEntries()
{
	EnsureDebuffRow();
	if (!DebuffRow || !WidgetTree)
	{
		return;
	}

	DebuffRow->ClearChildren();
	DebuffNameTexts.Reset();
	DebuffTimeTexts.Reset();
	EntryDebuffIds.Reset();

	for (int32 DebuffIndex = 0; DebuffIndex < ActiveDebuffs.Num(); ++DebuffIndex)
	{
		const FTunaSweeperActiveDebuffState& DebuffState = ActiveDebuffs[DebuffIndex];
		UBorder* EntryBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("DebuffEntry_%d"), DebuffIndex));
		UHorizontalBox* EntryRow = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			*FString::Printf(TEXT("DebuffEntryRow_%d"), DebuffIndex));
		USizeBox* IconSizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("DebuffIconSize_%d"), DebuffIndex));
		UImage* IconImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			*FString::Printf(TEXT("DebuffIcon_%d"), DebuffIndex));
		UTextBlock* NameText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("DebuffName_%d"), DebuffIndex));
		UTextBlock* TimeText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("DebuffTime_%d"), DebuffIndex));

		if (!EntryBackground || !EntryRow || !IconSizeBox || !IconImage || !NameText || !TimeText)
		{
			continue;
		}

		EntryBackground->SetPadding(FMargin(8.0f, 5.0f, 10.0f, 5.0f));
		EntryBackground->SetBrush(TunaSweeperHudDebuffBar::MakeRoundedBoxBrush(
			FVector2D(124.0f, 34.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.70f),
			8.0f,
			FLinearColor(1.0f, 1.0f, 1.0f, 0.06f),
			1.0f));
		EntryBackground->SetContent(EntryRow);

		IconSizeBox->SetWidthOverride(22.0f);
		IconSizeBox->SetHeightOverride(22.0f);
		IconSizeBox->SetContent(IconImage);
		ApplyDebuffIcon(IconImage, DebuffState);

		NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		NameText->SetJustification(ETextJustify::Left);
		NameText->SetMinDesiredWidth(42.0f);
		TunaSweeperUIFont::ApplyFont(NameText, 15.0f, ETunaSweeperUIFontWeight::Bold);

		TimeText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.82f, 0.78f, 1.0f)));
		TimeText->SetJustification(ETextJustify::Right);
		TimeText->SetMinDesiredWidth(24.0f);
		TimeText->SetVisibility(DebuffState.bHasDuration ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		TunaSweeperUIFont::ApplyFont(TimeText, 14.0f, ETunaSweeperUIFontWeight::Bold);

		if (UHorizontalBoxSlot* IconSlot = EntryRow->AddChildToHorizontalBox(IconSizeBox))
		{
			IconSlot->SetPadding(FMargin(0.0f, 0.0f, 7.0f, 0.0f));
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}
		if (UHorizontalBoxSlot* NameSlot = EntryRow->AddChildToHorizontalBox(NameText))
		{
			NameSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			NameSlot->SetVerticalAlignment(VAlign_Center);
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		if (UHorizontalBoxSlot* TimeSlot = EntryRow->AddChildToHorizontalBox(TimeText))
		{
			TimeSlot->SetVerticalAlignment(VAlign_Center);
			TimeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		if (UHorizontalBoxSlot* EntrySlot = DebuffRow->AddChildToHorizontalBox(EntryBackground))
		{
			EntrySlot->SetPadding(FMargin(DebuffIndex == 0 ? 0.0f : 8.0f, 0.0f, 0.0f, 0.0f));
			EntrySlot->SetVerticalAlignment(VAlign_Center);
			EntrySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		DebuffNameTexts.Add(NameText);
		DebuffTimeTexts.Add(TimeText);
		EntryDebuffIds.Add(DebuffState.DebuffId);
	}

	RefreshEntryTexts();
}

void UTunaSweeperHudDebuffBarWidget::RefreshEntryTexts()
{
	for (int32 DebuffIndex = 0; DebuffIndex < ActiveDebuffs.Num(); ++DebuffIndex)
	{
		if (DebuffNameTexts.IsValidIndex(DebuffIndex) && DebuffNameTexts[DebuffIndex])
		{
			DebuffNameTexts[DebuffIndex]->SetText(ResolveDebuffNameText(ActiveDebuffs[DebuffIndex]));
		}
		if (DebuffTimeTexts.IsValidIndex(DebuffIndex) && DebuffTimeTexts[DebuffIndex])
		{
			DebuffTimeTexts[DebuffIndex]->SetText(ResolveDebuffTimeText(ActiveDebuffs[DebuffIndex]));
			DebuffTimeTexts[DebuffIndex]->SetVisibility(
				ActiveDebuffs[DebuffIndex].bHasDuration
					? ESlateVisibility::HitTestInvisible
					: ESlateVisibility::Collapsed);
		}
	}
}

bool UTunaSweeperHudDebuffBarWidget::DoesEntryLayoutMatch() const
{
	if (EntryDebuffIds.Num() != ActiveDebuffs.Num() ||
		DebuffNameTexts.Num() != ActiveDebuffs.Num() ||
		DebuffTimeTexts.Num() != ActiveDebuffs.Num())
	{
		return false;
	}

	for (int32 DebuffIndex = 0; DebuffIndex < ActiveDebuffs.Num(); ++DebuffIndex)
	{
		if (EntryDebuffIds[DebuffIndex] != ActiveDebuffs[DebuffIndex].DebuffId)
		{
			return false;
		}
	}

	return true;
}

FText UTunaSweeperHudDebuffBarWidget::ResolveDebuffNameText(
	const FTunaSweeperActiveDebuffState& DebuffState) const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperDebuffDataSubsystem* DebuffDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperDebuffDataSubsystem>()
		: nullptr;

	FTunaSweeperDebuffDefinition Definition;
	if (DebuffDataSubsystem &&
		DebuffDataSubsystem->TryGetDebuffDefinition(DebuffState.DebuffId, Definition) &&
		!Definition.NameStringKey.IsNone())
	{
		return TunaGameInstance
			? TunaGameInstance->ResolveLocalizedText(
				Definition.NameStringKey,
				FText::FromString(DebuffState.DebuffId.ToString()))
			: FText::FromString(DebuffState.DebuffId.ToString());
	}

	return FText::FromString(DebuffState.DebuffId.ToString());
}

FText UTunaSweeperHudDebuffBarWidget::ResolveDebuffTimeText(
	const FTunaSweeperActiveDebuffState& DebuffState) const
{
	if (!DebuffState.bHasDuration)
	{
		return FText::GetEmpty();
	}

	const int32 RemainingSeconds = FMath::Max(0, FMath::CeilToInt(DebuffState.RemainingSeconds));
	return FText::FromString(FString::Printf(TEXT("%ds"), RemainingSeconds));
}

void UTunaSweeperHudDebuffBarWidget::ApplyDebuffIcon(
	UImage* IconImage,
	const FTunaSweeperActiveDebuffState& DebuffState) const
{
	if (!IconImage)
	{
		return;
	}

	FSlateBrush IconBrush;
	IconBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	IconBrush.TintColor = FSlateColor(FLinearColor(0.86f, 0.08f, 0.05f, 1.0f));
	IconBrush.SetImageSize(FVector2D(22.0f, 22.0f));
	IconBrush.OutlineSettings = FSlateBrushOutlineSettings(4.0f, FSlateColor(FLinearColor::Transparent), 0.0f);
	IconBrush.OutlineSettings.bUseBrushTransparency = false;

	UTunaSweeperDebuffDataSubsystem* DebuffDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperDebuffDataSubsystem>()
		: nullptr;

	FTunaSweeperDebuffDefinition Definition;
	if (DebuffDataSubsystem &&
		DebuffDataSubsystem->TryGetDebuffDefinition(DebuffState.DebuffId, Definition))
	{
		const FString IconObjectPath = DebuffDataSubsystem->BuildDebuffIconObjectPath(Definition);
		if (!IconObjectPath.IsEmpty())
		{
			if (UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, *IconObjectPath))
			{
				IconBrush.DrawAs = ESlateBrushDrawType::Image;
				IconBrush.SetResourceObject(IconTexture);
				IconBrush.TintColor = FSlateColor(FLinearColor::White);
			}
		}
	}

	IconImage->SetBrush(IconBrush);
}
