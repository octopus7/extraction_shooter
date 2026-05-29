#include "UI/TunaSweeperShopSellPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Styling/SlateBrush.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"
#include "Widgets/SWidget.h"

namespace TunaSweeperShopSellPanel
{
	constexpr float PanelWidth = 292.0f;
	constexpr float PanelHeight = 150.0f;

	using TunaSweeperUiText::ResolveUiText;

	FSlateBrush MakeRoundedBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(8.0f, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}
}

void UTunaSweeperShopSellPanelWidget::RefreshSelectedItem()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	FTunaSweeperItemInstance SelectedItemInstance;
	FTunaSweeperItemDefinition SelectedItemDefinition;
	int32 SalePrice = 0;
	if (!TunaGameInstance ||
		!ItemDataSubsystem ||
		!TunaGameInstance->TryGetSelectedItemInstance(SelectedItemInstance) ||
		!ItemDataSubsystem->TryGetItemDefinition(SelectedItemInstance.ItemId, SelectedItemDefinition) ||
		!TunaGameInstance->TryGetSlotSellPrice(TunaGameInstance->GetSelectedItemSlotReference(), SalePrice))
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FText DisplayName;
	if (!ItemDataSubsystem->TryGetItemNameTextByKey(
		SelectedItemDefinition.NameStringKey,
		TunaGameInstance->GetCurrentTextLanguage(),
		DisplayName))
	{
		DisplayName = FText::Format(
			TunaSweeperShopSellPanel::ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
			FText::AsNumber(SelectedItemInstance.ItemId));
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(DisplayName);
	}
	if (SalePriceText)
	{
		SalePriceText->SetText(FText::Format(
			TunaSweeperShopSellPanel::ResolveUiText(TunaGameInstance, TEXT("ui.shop.sell_price_pattern"), TEXT("\ud310\ub9e4\uac00 ${0}")),
			FText::AsNumber(SalePrice)));
	}
	if (SellButtonText)
	{
		SellButtonText->SetText(TunaSweeperShopSellPanel::ResolveUiText(
			TunaGameInstance,
			TEXT("ui.shop.sell_button"),
			TEXT("F\ud0a4 \ud310\ub9e4")));
	}
}

bool UTunaSweeperShopSellPanelWidget::TrySellSelectedHoveredItem()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance || !TunaGameInstance->HasSelectedInventoryItem() || !TunaGameInstance->HasHoveredItemSlot())
	{
		return false;
	}

	const FTunaSweeperItemSlotReference SelectedSlot = TunaGameInstance->GetSelectedItemSlotReference();
	const FTunaSweeperItemSlotReference HoveredSlot = TunaGameInstance->GetHoveredItemSlotReference();
	if (!SelectedSlot.IsValid() ||
		SelectedSlot.Source != HoveredSlot.Source ||
		SelectedSlot.SlotIndex != HoveredSlot.SlotIndex)
	{
		return false;
	}

	int32 SalePrice = 0;
	return TunaGameInstance->TrySellItemInSlot(SelectedSlot, SalePrice);
}

TSharedRef<SWidget> UTunaSweeperShopSellPanelWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildNativeWidgetTree();
	}

	TSharedRef<SWidget> RebuiltWidget = Super::RebuildWidget();
	CacheNamedWidgets();
	RefreshSelectedItem();
	return RebuiltWidget;
}

void UTunaSweeperShopSellPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnSelectedInventoryItemChanged.AddUObject(this, &UTunaSweeperShopSellPanelWidget::RefreshSelectedItem);
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperShopSellPanelWidget::RefreshSelectedItem);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperShopSellPanelWidget::RefreshSelectedItem);
	}

	if (SellButton)
	{
		SellButton->OnClicked.RemoveDynamic(this, &UTunaSweeperShopSellPanelWidget::HandleSellButtonClicked);
		SellButton->OnClicked.AddDynamic(this, &UTunaSweeperShopSellPanelWidget::HandleSellButtonClicked);
	}

	RefreshSelectedItem();
}

void UTunaSweeperShopSellPanelWidget::NativeDestruct()
{
	if (SellButton)
	{
		SellButton->OnClicked.RemoveDynamic(this, &UTunaSweeperShopSellPanelWidget::HandleSellButtonClicked);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTunaSweeperShopSellPanelWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	CacheNamedWidgets();
	RefreshSelectedItem();
}

void UTunaSweeperShopSellPanelWidget::BuildNativeWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
	UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
	ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));
	SalePriceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SalePriceText"));
	SellButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SellButton"));
	SellButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SellButtonText"));
	if (!PanelBackground || !PanelStack || !ItemNameText || !SalePriceText || !SellButton || !SellButtonText)
	{
		return;
	}

	WidgetTree->RootWidget = PanelBackground;
	PanelBackground->SetPadding(FMargin(18.0f, 14.0f));
	PanelBackground->SetBrush(TunaSweeperShopSellPanel::MakeRoundedBoxBrush(
		FVector2D(TunaSweeperShopSellPanel::PanelWidth, TunaSweeperShopSellPanel::PanelHeight),
		FLinearColor(0.02f, 0.024f, 0.028f, 0.88f),
		FLinearColor(0.62f, 0.70f, 0.66f, 0.50f),
		1.0f));
	PanelBackground->SetContent(PanelStack);

	ItemNameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	ItemNameText->SetAutoWrapText(false);
	TunaSweeperUIFont::ApplyFont(ItemNameText, 20, ETunaSweeperUIFontWeight::Bold);
	UVerticalBoxSlot* NameSlot = PanelStack->AddChildToVerticalBox(ItemNameText);
	if (NameSlot)
	{
		NameSlot->SetHorizontalAlignment(HAlign_Fill);
		NameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	SalePriceText->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.96f, 0.92f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(SalePriceText, 17);
	UVerticalBoxSlot* PriceSlot = PanelStack->AddChildToVerticalBox(SalePriceText);
	if (PriceSlot)
	{
		PriceSlot->SetHorizontalAlignment(HAlign_Fill);
		PriceSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
	}

	SellButtonText->SetJustification(ETextJustify::Center);
	SellButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.024f, 0.028f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(SellButtonText, 18, ETunaSweeperUIFontWeight::Bold);
	SellButton->SetContent(SellButtonText);
	UVerticalBoxSlot* ButtonSlot = PanelStack->AddChildToVerticalBox(SellButton);
	if (ButtonSlot)
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
	}
}

void UTunaSweeperShopSellPanelWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!PanelBackground)
	{
		PanelBackground = Cast<UBorder>(WidgetTree->FindWidget(FName(TEXT("PanelBackground"))));
	}
	if (!ItemNameText)
	{
		ItemNameText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("ItemNameText"))));
	}
	if (!SalePriceText)
	{
		SalePriceText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("SalePriceText"))));
	}
	if (!SellButton)
	{
		SellButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("SellButton"))));
	}
	if (!SellButtonText)
	{
		SellButtonText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("SellButtonText"))));
	}
}

void UTunaSweeperShopSellPanelWidget::HandleSellButtonClicked()
{
	TrySellSelectedHoveredItem();
}
