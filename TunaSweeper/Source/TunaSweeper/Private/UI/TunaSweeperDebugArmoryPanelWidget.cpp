#include "UI/TunaSweeperDebugArmoryPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUIStyle.h"

namespace
{
	FText ArmoryText(const UTunaSweeperGameInstance* Game, FName Key)
	{
		return Game ? Game->ResolveLocalizedText(Key, FText::GetEmpty()) : FText::GetEmpty();
	}

	UTextBlock* ArmoryLabel(UWidgetTree* Tree, const TCHAR* Name, float Size)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TunaSweeperUIStyle::ApplyLabel(Label);
		TunaSweeperUIFont::ApplyFont(Label, Size);
		return Label;
	}

	int32 ItemCategory(const FTunaSweeperItemDefinition& Item)
	{
		if (Item.CategoryTag == TEXT("item.category.ammo")) return 1;
		if (Item.CategoryTag == TEXT("item.category.weapon.gun") ||
			Item.CategoryTag == TEXT("item.category.weapon.melee") ||
			Item.CategoryTag == TEXT("item.category.attachment")) return 0;
		return 2;
	}
}

void UTunaSweeperArmoryItemCardWidget::Configure(int32 InItemId, UTexture2D* Icon,
	const FText& Name, bool bSelected, FTunaSweeperArmoryItemSelected InSelected)
{
	ItemId = InItemId;
	IconTexture = Icon;
	ItemName = Name;
	bIsSelected = bSelected;
	SelectedDelegate = MoveTemp(InSelected);
	RefreshCard();
}

TSharedRef<SWidget> UTunaSweeperArmoryItemCardWidget::RebuildWidget()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	BuildCard();
	return Super::RebuildWidget();
}

void UTunaSweeperArmoryItemCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildCard();
	RefreshCard();
}

void UTunaSweeperArmoryItemCardWidget::BuildCard()
{
	if (!WidgetTree || WidgetTree->RootWidget) return;
	CardButton = WidgetTree->ConstructWidget<UButton>();
	WidgetTree->RootWidget = CardButton;
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
	CardButton->SetContent(Stack);
	USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
	Size->SetWidthOverride(190.0f);
	Size->SetHeightOverride(132.0f);
	UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
	Fit->SetStretch(EStretch::ScaleToFit);
	IconImage = WidgetTree->ConstructWidget<UImage>();
	IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	Fit->SetContent(IconImage);
	Size->SetContent(Fit);
	Stack->AddChildToVerticalBox(Size)->SetHorizontalAlignment(HAlign_Center);
	NameText = ArmoryLabel(WidgetTree, TEXT("ArmoryItemName"), 17.0f);
	NameText->SetAutoWrapText(true);
	NameText->SetJustification(ETextJustify::Center);
	Stack->AddChildToVerticalBox(NameText)->SetPadding(FMargin(4.0f, 8.0f));
	CardButton->OnClicked.AddDynamic(this, &ThisClass::HandleClicked);
}

void UTunaSweeperArmoryItemCardWidget::RefreshCard()
{
	if (!CardButton) return;
	TunaSweeperUIStyle::ApplyButton(CardButton, TunaSweeperUIStyle::EButtonRole::Secondary, bIsSelected);
	IconImage->SetBrushFromTexture(IconTexture, true);
	NameText->SetText(ItemName);
	NameText->SetColorAndOpacity(bIsSelected ? FLinearColor(0.38f, 0.94f, 0.8f) : FLinearColor::White);
	CardButton->SetToolTipText(ItemName);
}

void UTunaSweeperArmoryItemCardWidget::HandleClicked()
{
#if !UE_BUILD_SHIPPING
	SelectedDelegate.ExecuteIfBound(ItemId);
#endif
}

TSharedRef<SWidget> UTunaSweeperDebugArmoryPanelWidget::RebuildWidget()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	BuildPanel();
	return Super::RebuildWidget();
}

void UTunaSweeperDebugArmoryPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
#if !UE_BUILD_SHIPPING
	SetIsFocusable(true);
	BuildPanel();
	if (UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnLanguageChanged.RemoveAll(this);
		Game->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshArmory);
	}
	RefreshArmory();
#endif
}

void UTunaSweeperDebugArmoryPanelWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnLanguageChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UTunaSweeperDebugArmoryPanelWidget::BuildPanel()
{
#if !UE_BUILD_SHIPPING
	if (!WidgetTree || WidgetTree->RootWidget) return;
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DebugArmoryScreen"));
	Root->SetBrush(FSlateColorBrush(FLinearColor(0.015f, 0.029f, 0.04f, 0.94f)));
	Root->SetPadding(FMargin(32.0f));
	WidgetTree->RootWidget = Root;
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
	Root->SetContent(Stack);
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Stack->AddChildToVerticalBox(Header)->SetPadding(FMargin(0, 0, 0, 16));
	UVerticalBox* Heading = WidgetTree->ConstructWidget<UVerticalBox>();
	Header->AddChildToHorizontalBox(Heading)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleText = ArmoryLabel(WidgetTree, TEXT("DebugArmoryTitle"), 30);
	TunaSweeperUIFont::ApplyFont(TitleText, 30, ETunaSweeperUIFontWeight::Bold);
	Heading->AddChildToVerticalBox(TitleText);
	SubtitleText = ArmoryLabel(WidgetTree, TEXT("DebugArmorySubtitle"), 16);
	SubtitleText->SetColorAndOpacity(FLinearColor(0.59f, 0.72f, 0.75f));
	Heading->AddChildToVerticalBox(SubtitleText);
	UButton* CloseButton = WidgetTree->ConstructWidget<UButton>();
	CloseText = ArmoryLabel(WidgetTree, TEXT("DebugArmoryCloseText"), 18);
	CloseButton->SetContent(CloseText);
	TunaSweeperUIStyle::ApplyButton(CloseButton, TunaSweeperUIStyle::EButtonRole::Secondary);
	Header->AddChildToHorizontalBox(CloseButton)->SetVerticalAlignment(VAlign_Center);
	CloseButton->OnClicked.AddDynamic(this, &ThisClass::HandleCloseClicked);
	CategoryText = ArmoryLabel(WidgetTree, TEXT("DebugArmoryCategoryText"), 16);
	Stack->AddChildToVerticalBox(CategoryText);
	CategoryCombo = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("DebugArmoryCategory"));
	CategoryCombo->SetContentPadding(FMargin(12, 8));
	CategoryCombo->OnGenerateWidgetEvent.BindDynamic(this, &ThisClass::GenerateCategoryLabel);
	CategoryCombo->OnSelectionChanged.AddDynamic(this, &ThisClass::HandleCategoryChanged);
	Stack->AddChildToVerticalBox(CategoryCombo)->SetPadding(FMargin(0, 4, 0, 16));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
	Stack->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ItemGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("DebugArmoryItems"));
	ItemGrid->SetSlotPadding(FMargin(6));
	ItemGrid->SetMinDesiredSlotWidth(210);
	ItemGrid->SetMinDesiredSlotHeight(196);
	Scroll->AddChild(ItemGrid);
	SelectedNameText = ArmoryLabel(WidgetTree, TEXT("DebugArmorySelectedItem"), 22);
	Stack->AddChildToVerticalBox(SelectedNameText)->SetPadding(FMargin(0, 12, 0, 8));
	UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>();
	Stack->AddChildToVerticalBox(Footer);
	QuantityText = ArmoryLabel(WidgetTree, TEXT("DebugArmoryQuantityText"), 18);
	Footer->AddChildToHorizontalBox(QuantityText)->SetVerticalAlignment(VAlign_Center);
	QuantitySpinBox = WidgetTree->ConstructWidget<USpinBox>(USpinBox::StaticClass(), TEXT("DebugArmoryQuantity"));
	QuantitySpinBox->SetMinValue(1);
	QuantitySpinBox->SetMaxValue(999);
	QuantitySpinBox->SetMinSliderValue(1);
	QuantitySpinBox->SetMaxSliderValue(999);
	QuantitySpinBox->SetDelta(1);
	QuantitySpinBox->SetMinFractionalDigits(0);
	QuantitySpinBox->SetMaxFractionalDigits(0);
	QuantitySpinBox->SetFont(TunaSweeperUIFont::MakeFont(nullptr, 20));
	QuantitySpinBox->SetMinDesiredWidth(150);
	QuantitySpinBox->SetValue(1);
	Footer->AddChildToHorizontalBox(QuantitySpinBox)->SetPadding(FMargin(16, 0));
	SupplyButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DebugArmorySupplyButton"));
	SupplyText = ArmoryLabel(WidgetTree, TEXT("DebugArmorySupplyText"), 20);
	SupplyText->SetJustification(ETextJustify::Center);
	SupplyButton->SetContent(SupplyText);
	TunaSweeperUIStyle::ApplyButton(SupplyButton);
	Footer->AddChildToHorizontalBox(SupplyButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	SupplyButton->OnClicked.AddDynamic(this, &ThisClass::HandleSupplyClicked);
	StatusText = ArmoryLabel(WidgetTree, TEXT("DebugArmoryStatus"), 16);
	StatusText->SetAutoWrapText(true);
	Stack->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0, 10, 0, 0));
#endif
}

void UTunaSweeperDebugArmoryPanelWidget::OpenArmory()
{
#if !UE_BUILD_SHIPPING
	StatusKey = NAME_None;
	SelectedItemId = INDEX_NONE;
	CategoryIndex = 0;
	BuildPanel();
	if (QuantitySpinBox) QuantitySpinBox->SetValue(1);
	RefreshArmory();
#endif
}

void UTunaSweeperDebugArmoryPanelWidget::RefreshArmory()
{
#if !UE_BUILD_SHIPPING
	BuildPanel();
	if (!ItemGrid) return;
	UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>();
	TitleText->SetText(ArmoryText(Game, TEXT("ui.debug_armory.title")));
	SubtitleText->SetText(ArmoryText(Game, TEXT("ui.debug_armory.subtitle")));
	CategoryText->SetText(ArmoryText(Game, TEXT("ui.debug_armory.category")));
	QuantityText->SetText(ArmoryText(Game, TEXT("ui.debug_armory.quantity")));
	SupplyText->SetText(ArmoryText(Game, TEXT("ui.debug_armory.supply")));
	CloseText->SetText(ArmoryText(Game, TEXT("ui.debug_armory.close")));
	bRefreshingCategories = true;
	CategoryCombo->ClearOptions();
	CategoryCombo->AddOption(TEXT("ui.debug_armory.weapons"));
	CategoryCombo->AddOption(TEXT("ui.debug_armory.ammo"));
	CategoryCombo->AddOption(TEXT("ui.debug_armory.protection"));
	CategoryCombo->SetSelectedIndex(CategoryIndex);
	bRefreshingCategories = false;
	Catalog.Reset();
	if (ATunaSweeperPlayerController* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
	{
		Controller->GetDebugArmoryCatalog(Catalog);
	}
	RefreshItems();
#endif
}

UWidget* UTunaSweeperDebugArmoryPanelWidget::GenerateCategoryLabel(FString Option)
{
	UTextBlock* Label = ArmoryLabel(WidgetTree, TEXT(""), 18);
	Label->SetText(ArmoryText(GetGameInstance<UTunaSweeperGameInstance>(), FName(*Option)));
	return Label;
}

void UTunaSweeperDebugArmoryPanelWidget::HandleCategoryChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (bRefreshingCategories || !CategoryCombo) return;
	CategoryIndex = CategoryCombo->FindOptionIndex(SelectedItem);
	SelectedItemId = INDEX_NONE;
	StatusKey = NAME_None;
	RefreshItems();
}

void UTunaSweeperDebugArmoryPanelWidget::RefreshItems()
{
#if !UE_BUILD_SHIPPING
	if (!ItemGrid) return;
	UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* Items = Game ? Game->GetSubsystem<UTunaSweeperItemDataSubsystem>() : nullptr;
	ItemGrid->ClearChildren();
	ItemCards.Reset();
	FText SelectedName;
	bool bHasSelection = false;
	for (const FTunaSweeperItemDefinition& Item : Catalog)
	{
		if (ItemCategory(Item) != CategoryIndex || !Items) continue;
		if (SelectedItemId == INDEX_NONE) SelectedItemId = Item.Id;
		FText Name = ArmoryText(Game, TEXT("ui.debug_armory.unnamed_item"));
		Items->TryGetItemNameTextByKey(Item.NameStringKey, Game->GetCurrentTextLanguage(), Name);
		const bool bSelected = Item.Id == SelectedItemId;
		if (bSelected) { SelectedName = Name; bHasSelection = true; }
		UTunaSweeperArmoryItemCardWidget* Card = CreateWidget<UTunaSweeperArmoryItemCardWidget>(GetOwningPlayer());
		if (!Card) continue;
		const FString IconPath = Items->BuildItemIconObjectPath(Item);
		UTexture2D* Icon = IconPath.IsEmpty() ? nullptr : LoadObject<UTexture2D>(nullptr, *IconPath);
		Card->Configure(Item.Id, Icon, Name, bSelected, FTunaSweeperArmoryItemSelected::CreateUObject(this, &ThisClass::SelectItem));
		const int32 Index = ItemCards.Num();
		ItemCards.Add(Card);
		ItemGrid->AddChildToUniformGrid(Card, Index / 4, Index % 4);
	}
	SelectedNameText->SetText(SelectedName);
	SupplyButton->SetIsEnabled(bHasSelection);
	QuantitySpinBox->SetIsEnabled(bHasSelection);
	const FName MessageKey = !bHasSelection ? FName(TEXT("ui.debug_armory.empty")) : StatusKey;
	StatusText->SetText(MessageKey.IsNone() ? FText::GetEmpty() : ArmoryText(Game, MessageKey));
	StatusText->SetColorAndOpacity(StatusKey == TEXT("ui.debug_armory.failed")
		? FLinearColor(1.0f, 0.4f, 0.32f) : FLinearColor(0.59f, 0.82f, 0.8f));
#endif
}

void UTunaSweeperDebugArmoryPanelWidget::SelectItem(int32 ItemId)
{
	SelectedItemId = ItemId;
	StatusKey = NAME_None;
	RefreshItems();
}

void UTunaSweeperDebugArmoryPanelWidget::HandleSupplyClicked()
{
#if !UE_BUILD_SHIPPING
	ATunaSweeperPlayerController* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer());
	const int32 Quantity = QuantitySpinBox ? FMath::Clamp(FMath::RoundToInt(QuantitySpinBox->GetValue()), 1, 999) : 1;
	if (QuantitySpinBox) QuantitySpinBox->SetValue(Quantity);
	const bool bSucceeded = Controller && Controller->TrySupplyDebugArmoryItem(SelectedItemId, Quantity);
	StatusKey = bSucceeded ? TEXT("ui.debug_armory.success") : TEXT("ui.debug_armory.failed");
	RefreshItems();
#endif
}

void UTunaSweeperDebugArmoryPanelWidget::HandleCloseClicked()
{
	if (ATunaSweeperPlayerController* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
	{
		Controller->CloseDebugArmoryPanel();
	}
}
