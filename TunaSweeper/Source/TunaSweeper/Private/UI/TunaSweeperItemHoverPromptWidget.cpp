#include "UI/TunaSweeperItemHoverPromptWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateBrush.h"
#include "Widgets/SWidget.h"

namespace TunaSweeperItemHoverPrompt
{
	constexpr float PromptOffsetX = 16.0f;
	constexpr float PromptOffsetY = 16.0f;
	constexpr float InfoBoxWidth = 300.0f;
	constexpr float ActionBoxWidth = 154.0f;
	constexpr float PromptHeight = 124.0f;

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
		Brush.OutlineSettings = FSlateBrushOutlineSettings(6.0f, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	void ConfigureTextBlock(UTextBlock* TextBlock, const FText& Text, const FLinearColor& Color, int32 FontSize)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = FontSize;
		TextBlock->SetFont(FontInfo);
		TextBlock->SetText(Text);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(ETextJustify::Left);
	}

	void AddTextLine(UVerticalBox* Stack, UTextBlock* TextBlock, float BottomPadding)
	{
		if (!Stack || !TextBlock)
		{
			return;
		}

		UVerticalBoxSlot* LineSlot = Stack->AddChildToVerticalBox(TextBlock);
		if (LineSlot)
		{
			LineSlot->SetHorizontalAlignment(HAlign_Fill);
			LineSlot->SetVerticalAlignment(VAlign_Top);
			LineSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, BottomPadding));
		}
	}

	USizeBox* BuildKeyBox(UWidgetTree* WidgetTree, const TCHAR* Prefix, UTextBlock*& OutKeyText)
	{
		USizeBox* KeySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(FString::Printf(TEXT("%sKeySizeBox"), Prefix)));
		UBorder* KeyOuterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(FString::Printf(TEXT("%sKeyOuterBorder"), Prefix)));
		UBorder* KeyInnerBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), FName(FString::Printf(TEXT("%sKeyInnerBorder"), Prefix)));
		OutKeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(FString::Printf(TEXT("%sKeyText"), Prefix)));

		if (!KeySizeBox || !KeyOuterBorder || !KeyInnerBorder || !OutKeyText)
		{
			return nullptr;
		}

		KeySizeBox->SetWidthOverride(46.0f);
		KeySizeBox->SetHeightOverride(28.0f);
		KeyOuterBorder->SetPadding(FMargin(3.0f, 2.0f));
		KeyOuterBorder->SetBrush(MakeRoundedBoxBrush(
			FVector2D(46.0f, 28.0f),
			FLinearColor(0.36f, 0.39f, 0.42f, 0.96f),
			FLinearColor::Transparent,
			0.0f));
		KeyInnerBorder->SetPadding(FMargin(8.0f, 1.0f));
		KeyInnerBorder->SetBrush(MakeRoundedBoxBrush(
			FVector2D(38.0f, 22.0f),
			FLinearColor(0.94f, 0.95f, 0.96f, 1.0f),
			FLinearColor::Transparent,
			0.0f));
		ConfigureTextBlock(OutKeyText, FText::GetEmpty(), FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 16);
		OutKeyText->SetJustification(ETextJustify::Center);
		KeyInnerBorder->SetContent(OutKeyText);
		KeyOuterBorder->SetContent(KeyInnerBorder);
		KeySizeBox->SetContent(KeyOuterBorder);
		return KeySizeBox;
	}

	void AddActionRow(
		UWidgetTree* WidgetTree,
		UVerticalBox* Stack,
		const TCHAR* Prefix,
		const FText& KeyText,
		const FText& ActionText,
		UTextBlock*& OutKeyText,
		UTextBlock*& OutActionText)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), FName(FString::Printf(TEXT("%sActionRow"), Prefix)));
		USizeBox* KeyBox = BuildKeyBox(WidgetTree, Prefix, OutKeyText);
		OutActionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(FString::Printf(TEXT("%sActionText"), Prefix)));
		if (!Row || !KeyBox || !OutActionText)
		{
			return;
		}

		ConfigureTextBlock(OutKeyText, KeyText, FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 16);
		ConfigureTextBlock(OutActionText, ActionText, FLinearColor(0.94f, 0.96f, 0.98f, 1.0f), 14);

		UHorizontalBoxSlot* KeySlot = Row->AddChildToHorizontalBox(KeyBox);
		if (KeySlot)
		{
			KeySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			KeySlot->SetVerticalAlignment(VAlign_Center);
			KeySlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}

		UHorizontalBoxSlot* ActionSlot = Row->AddChildToHorizontalBox(OutActionText);
		if (ActionSlot)
		{
			ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ActionSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* RowSlot = Stack->AddChildToVerticalBox(Row);
		if (RowSlot)
		{
			RowSlot->SetHorizontalAlignment(HAlign_Fill);
			RowSlot->SetVerticalAlignment(VAlign_Top);
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
	}
}

void UTunaSweeperItemHoverPromptWidget::SetItemTileData(const FTunaSweeperItemStackTileData& InTileData)
{
	CachedTileData = InTileData;
	ApplyTileData();
}

void UTunaSweeperItemHoverPromptWidget::SetPromptViewportPosition(const FVector2D& ViewportPosition)
{
	SetPositionInViewport(
		ViewportPosition + FVector2D(TunaSweeperItemHoverPrompt::PromptOffsetX, TunaSweeperItemHoverPrompt::PromptOffsetY),
		false);
}

TSharedRef<SWidget> UTunaSweeperItemHoverPromptWidget::RebuildWidget()
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
	ApplyTileData();
	return RebuiltWidget;
}

void UTunaSweeperItemHoverPromptWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	CacheNamedWidgets();
	ApplyTileData();
}

void UTunaSweeperItemHoverPromptWidget::BuildNativeWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	UHorizontalBox* PromptRootRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("PromptRootRow"));
	RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
	USizeBox* InfoSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InfoSizeBox"));
	ItemInfoBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ItemInfoBackground"));
	UVerticalBox* ItemInfoStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ItemInfoStack"));
	USizeBox* ActionSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ActionSizeBox"));
	ActionHintsBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ActionHintsBackground"));
	UVerticalBox* ActionHintsStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ActionHintsStack"));
	ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));
	ItemWeightText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemWeightText"));
	ItemDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemDescriptionText"));
	ItemPriceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemPriceText"));

	if (!RootSizeBox || !PromptRootRow || !InfoSizeBox || !ItemInfoBackground || !ItemInfoStack ||
		!ActionSizeBox || !ActionHintsBackground || !ActionHintsStack ||
		!ItemNameText || !ItemWeightText || !ItemDescriptionText || !ItemPriceText)
	{
		return;
	}

	WidgetTree->RootWidget = RootSizeBox;
	RootSizeBox->SetWidthOverride(TunaSweeperItemHoverPrompt::InfoBoxWidth + TunaSweeperItemHoverPrompt::ActionBoxWidth + 6.0f);
	RootSizeBox->SetHeightOverride(TunaSweeperItemHoverPrompt::PromptHeight);
	RootSizeBox->SetContent(PromptRootRow);

	InfoSizeBox->SetWidthOverride(TunaSweeperItemHoverPrompt::InfoBoxWidth);
	InfoSizeBox->SetHeightOverride(TunaSweeperItemHoverPrompt::PromptHeight);
	InfoSizeBox->SetContent(ItemInfoBackground);
	ItemInfoBackground->SetPadding(FMargin(16.0f, 10.0f));
	ItemInfoBackground->SetBrush(TunaSweeperItemHoverPrompt::MakeRoundedBoxBrush(
		FVector2D(TunaSweeperItemHoverPrompt::InfoBoxWidth, TunaSweeperItemHoverPrompt::PromptHeight),
		FLinearColor(0.0f, 0.0f, 0.0f, 0.82f),
		FLinearColor(0.10f, 0.12f, 0.14f, 0.72f),
		1.0f));
	ItemInfoBackground->SetContent(ItemInfoStack);

	TunaSweeperItemHoverPrompt::ConfigureTextBlock(ItemNameText, FText::GetEmpty(), FLinearColor::White, 18);
	TunaSweeperItemHoverPrompt::ConfigureTextBlock(ItemWeightText, FText::GetEmpty(), FLinearColor(0.92f, 0.94f, 0.96f, 1.0f), 16);
	TunaSweeperItemHoverPrompt::ConfigureTextBlock(ItemDescriptionText, FText::GetEmpty(), FLinearColor(0.88f, 0.90f, 0.92f, 1.0f), 15);
	TunaSweeperItemHoverPrompt::ConfigureTextBlock(ItemPriceText, FText::GetEmpty(), FLinearColor(0.98f, 0.98f, 0.98f, 1.0f), 17);
	ItemNameText->SetAutoWrapText(false);
	ItemWeightText->SetAutoWrapText(false);
	ItemDescriptionText->SetAutoWrapText(true);
	ItemPriceText->SetAutoWrapText(false);
	TunaSweeperItemHoverPrompt::AddTextLine(ItemInfoStack, ItemNameText, 8.0f);
	TunaSweeperItemHoverPrompt::AddTextLine(ItemInfoStack, ItemWeightText, 8.0f);
	TunaSweeperItemHoverPrompt::AddTextLine(ItemInfoStack, ItemDescriptionText, 8.0f);
	TunaSweeperItemHoverPrompt::AddTextLine(ItemInfoStack, ItemPriceText, 0.0f);

	ActionSizeBox->SetWidthOverride(TunaSweeperItemHoverPrompt::ActionBoxWidth);
	ActionSizeBox->SetHeightOverride(TunaSweeperItemHoverPrompt::PromptHeight);
	ActionSizeBox->SetContent(ActionHintsBackground);
	ActionHintsBackground->SetPadding(FMargin(10.0f, 12.0f));
	ActionHintsBackground->SetBrush(TunaSweeperItemHoverPrompt::MakeRoundedBoxBrush(
		FVector2D(TunaSweeperItemHoverPrompt::ActionBoxWidth, TunaSweeperItemHoverPrompt::PromptHeight),
		FLinearColor(0.0f, 0.0f, 0.0f, 0.82f),
		FLinearColor(0.10f, 0.12f, 0.14f, 0.72f),
		1.0f));
	ActionHintsBackground->SetContent(ActionHintsStack);

	UTextBlock* RawTakeKeyText = nullptr;
	UTextBlock* RawTakeActionText = nullptr;
	TunaSweeperItemHoverPrompt::AddActionRow(
		WidgetTree,
		ActionHintsStack,
		TEXT("Take"),
		FText::FromString(TEXT("F")),
		FText::FromString(TEXT("\uC90D\uAE30/\uBC30\uCE58")),
		RawTakeKeyText,
		RawTakeActionText);
	TakeKeyText = RawTakeKeyText;
	TakeActionText = RawTakeActionText;

	UTextBlock* RawDropKeyText = nullptr;
	UTextBlock* RawDropActionText = nullptr;
	TunaSweeperItemHoverPrompt::AddActionRow(
		WidgetTree,
		ActionHintsStack,
		TEXT("Drop"),
		FText::FromString(TEXT("X")),
		FText::FromString(TEXT("\uBC84\uB9AC\uAE30")),
		RawDropKeyText,
		RawDropActionText);
	DropKeyText = RawDropKeyText;
	DropActionText = RawDropActionText;

	UHorizontalBoxSlot* InfoSlot = PromptRootRow->AddChildToHorizontalBox(InfoSizeBox);
	if (InfoSlot)
	{
		InfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		InfoSlot->SetVerticalAlignment(VAlign_Top);
	}

	UHorizontalBoxSlot* ActionSlot = PromptRootRow->AddChildToHorizontalBox(ActionSizeBox);
	if (ActionSlot)
	{
		ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		ActionSlot->SetVerticalAlignment(VAlign_Top);
		ActionSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
	}
}

void UTunaSweeperItemHoverPromptWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootSizeBox)
	{
		RootSizeBox = Cast<USizeBox>(WidgetTree->FindWidget(FName(TEXT("RootSizeBox"))));
	}
	if (!ItemInfoBackground)
	{
		ItemInfoBackground = Cast<UBorder>(WidgetTree->FindWidget(FName(TEXT("ItemInfoBackground"))));
	}
	if (!ActionHintsBackground)
	{
		ActionHintsBackground = Cast<UBorder>(WidgetTree->FindWidget(FName(TEXT("ActionHintsBackground"))));
	}
	if (!ItemNameText)
	{
		ItemNameText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("ItemNameText"))));
	}
	if (!ItemWeightText)
	{
		ItemWeightText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("ItemWeightText"))));
	}
	if (!ItemDescriptionText)
	{
		ItemDescriptionText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("ItemDescriptionText"))));
	}
	if (!ItemPriceText)
	{
		ItemPriceText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("ItemPriceText"))));
	}
	if (!TakeKeyText)
	{
		TakeKeyText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("TakeKeyText"))));
	}
	if (!TakeActionText)
	{
		TakeActionText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("TakeActionText"))));
	}
	if (!DropKeyText)
	{
		DropKeyText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("DropKeyText"))));
	}
	if (!DropActionText)
	{
		DropActionText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("DropActionText"))));
	}
}

void UTunaSweeperItemHoverPromptWidget::ApplyTileData()
{
	if (CachedTileData.bIsEmpty)
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	if (ItemNameText)
	{
		ItemNameText->SetText(BuildNameText());
	}
	if (ItemWeightText)
	{
		ItemWeightText->SetText(BuildWeightText());
	}
	if (ItemDescriptionText)
	{
		ItemDescriptionText->SetText(CachedTileData.DescriptionText);
	}
	if (ItemPriceText)
	{
		ItemPriceText->SetText(BuildPriceText());
	}
	if (TakeKeyText)
	{
		TakeKeyText->SetText(FText::FromString(TEXT("F")));
	}
	if (TakeActionText)
	{
		TakeActionText->SetText(FText::FromString(TEXT("\uC90D\uAE30/\uBC30\uCE58")));
	}
	if (DropKeyText)
	{
		DropKeyText->SetText(FText::FromString(TEXT("X")));
	}
	if (DropActionText)
	{
		DropActionText->SetText(FText::FromString(TEXT("\uBC84\uB9AC\uAE30")));
	}
}

FText UTunaSweeperItemHoverPromptWidget::BuildNameText() const
{
	const int32 ItemId = CachedTileData.bHasItemDefinition
		? CachedTileData.ItemDefinition.Id
		: CachedTileData.ItemInstance.ItemId;
	const FString DisplayName = CachedTileData.DisplayName.IsEmpty()
		? FString::Printf(TEXT("Item %d"), ItemId)
		: CachedTileData.DisplayName.ToString();

	return FText::FromString(FString::Printf(TEXT("%s #%d"), *DisplayName, ItemId));
}

FText UTunaSweeperItemHoverPromptWidget::BuildWeightText() const
{
	const float WeightKg = CachedTileData.bHasItemDefinition
		? FMath::Max(0.0f, CachedTileData.ItemDefinition.WeightKg)
		: 0.0f;

	if (FMath::IsNearlyEqual(WeightKg, FMath::RoundToFloat(WeightKg), 0.01f))
	{
		return FText::FromString(FString::Printf(TEXT("%.0f kg"), WeightKg));
	}

	return FText::FromString(FString::Printf(TEXT("%.1f kg"), WeightKg));
}

FText UTunaSweeperItemHoverPromptWidget::BuildPriceText() const
{
	const int32 Price = CachedTileData.bHasItemDefinition
		? FMath::Max(0, CachedTileData.ItemDefinition.ShopSellPrice)
		: 0;
	return FText::FromString(FString::Printf(TEXT("$%d"), Price));
}
