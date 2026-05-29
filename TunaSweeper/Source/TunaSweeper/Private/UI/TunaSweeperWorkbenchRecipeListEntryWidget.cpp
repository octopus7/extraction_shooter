#include "UI/TunaSweeperWorkbenchRecipeListEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperWorkbenchRecipeEntry
{
	FSlateBrush MakeRowBrush(const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(FVector2D(260.0f, 52.0f));
		Brush.OutlineSettings = FSlateBrushOutlineSettings(4.0f, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}
}

void UTunaSweeperWorkbenchRecipeListEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureWidgetTree();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	ApplyTileData();
	ApplyRowState();
}

void UTunaSweeperWorkbenchRecipeListEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UTunaSweeperItemStackTileItemObject* TileItemObject = Cast<UTunaSweeperItemStackTileItemObject>(ListItemObject);
	CachedTileData = TileItemObject ? TileItemObject->GetTileData() : FTunaSweeperItemStackTileData();
	EnsureWidgetTree();
	ApplyTileData();
	ApplyRowState();
}

void UTunaSweeperWorkbenchRecipeListEntryWidget::NativeOnItemSelectionChanged(bool bIsSelected)
{
	IUserListEntry::NativeOnItemSelectionChanged(bIsSelected);

	bSelected = bIsSelected;
	ApplyRowState();
}

void UTunaSweeperWorkbenchRecipeListEntryWidget::NativeOnMouseEnter(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bHovered = true;
	ApplyRowState();
}

void UTunaSweeperWorkbenchRecipeListEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	bHovered = false;
	ApplyRowState();
	Super::NativeOnMouseLeave(InMouseEvent);
}

void UTunaSweeperWorkbenchRecipeListEntryWidget::EnsureWidgetTree()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RowBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RecipeRowBackground"));
	UHorizontalBox* RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RecipeRowBox"));
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RecipeIconBox"));
	RecipeIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RecipeIconImage"));
	RecipeNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RecipeNameText"));
	if (!RowBackground || !RowBox || !IconBox || !RecipeIconImage || !RecipeNameText)
	{
		return;
	}

	WidgetTree->RootWidget = RowBackground;
	RowBackground->SetPadding(FMargin(8.0f, 5.0f));
	RowBackground->SetContent(RowBox);

	IconBox->SetWidthOverride(34.0f);
	IconBox->SetHeightOverride(34.0f);
	IconBox->SetContent(RecipeIconImage);

	UHorizontalBoxSlot* IconSlot = RowBox->AddChildToHorizontalBox(IconBox);
	if (IconSlot)
	{
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		IconSlot->SetHorizontalAlignment(HAlign_Left);
		IconSlot->SetVerticalAlignment(VAlign_Center);
	}

	TunaSweeperUIFont::ApplyFont(RecipeNameText, 16.0f);
	RecipeNameText->SetAutoWrapText(false);
	RecipeNameText->SetJustification(ETextJustify::Left);

	UHorizontalBoxSlot* NameSlot = RowBox->AddChildToHorizontalBox(RecipeNameText);
	if (NameSlot)
	{
		NameSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UTunaSweeperWorkbenchRecipeListEntryWidget::ApplyTileData()
{
	if (RecipeIconImage)
	{
		UTexture2D* IconTexture = CachedTileData.IconTexture.LoadSynchronous();
		if (IconTexture && !CachedTileData.bIsEmpty)
		{
			RecipeIconImage->SetBrushFromTexture(IconTexture, true);
			RecipeIconImage->SetOpacity(1.0f);
		}
		else
		{
			RecipeIconImage->SetBrushFromTexture(nullptr, false);
			RecipeIconImage->SetOpacity(0.0f);
		}
	}

	if (RecipeNameText)
	{
		RecipeNameText->SetText(CachedTileData.bIsEmpty ? FText::GetEmpty() : CachedTileData.DisplayName);
		RecipeNameText->SetColorAndOpacity(FSlateColor(CachedTileData.bCanCraftWorkbenchRecipe
			? FLinearColor(0.92f, 0.96f, 1.0f, 1.0f)
			: FLinearColor(0.74f, 0.78f, 0.82f, 1.0f)));
	}
}

void UTunaSweeperWorkbenchRecipeListEntryWidget::ApplyRowState()
{
	if (!RowBackground)
	{
		return;
	}

	if (bSelected)
	{
		RowBackground->SetBrush(TunaSweeperWorkbenchRecipeEntry::MakeRowBrush(
			FLinearColor(0.055f, 0.135f, 0.215f, 0.96f),
			FLinearColor(0.36f, 0.58f, 0.86f, 1.0f),
			1.5f));
		return;
	}

	if (bHovered)
	{
		RowBackground->SetBrush(TunaSweeperWorkbenchRecipeEntry::MakeRowBrush(
			FLinearColor(0.040f, 0.056f, 0.070f, 0.94f),
			FLinearColor(0.28f, 0.34f, 0.40f, 0.95f),
			1.0f));
		return;
	}

	RowBackground->SetBrush(TunaSweeperWorkbenchRecipeEntry::MakeRowBrush(
		FLinearColor(0.025f, 0.030f, 0.034f, 0.72f),
		FLinearColor(0.14f, 0.17f, 0.20f, 0.80f),
		1.0f));
}
