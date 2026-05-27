#include "UI/TunaSweeperGameHudWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "UI/TunaSweeperHudExternalPanelWidget.h"
#include "UI/TunaSweeperHudInventoryAreaWidget.h"
#include "UI/TunaSweeperHudItemInfoPanelWidget.h"
#include "UI/TunaSweeperHudQuickSlotBarWidget.h"
#include "UI/TunaSweeperHudTopReserveWidget.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "UI/TunaSweeperItemThumbnailSlotWidget.h"
#include "UI/TunaSweeperMapWidget.h"
#include "UI/TunaSweeperMemoWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "Styling/SlateBrush.h"

namespace
{
	constexpr int32 InventoryQuickSlotFirstNumber = 3;
	constexpr int32 InventoryQuickSlotLastNumber = 8;
	constexpr float InventoryQuickSlotPanelWidth = 760.0f;
	constexpr float InventoryQuickSlotPanelHeight = 168.0f;
	constexpr float InventoryQuickSlotTileSize = 112.0f;
	constexpr float InventoryQuickSlotTileScale = 1.12f;

	FSlateBrush MakeHudRoundedBoxBrush(
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

	FTunaSweeperItemStackTileData BuildQuickSlotTileData(
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const FTunaSweeperInventorySlot& Slot,
		int32 SlotIndex)
	{
		FTunaSweeperItemStackTileData TileData;
		TileData.Source = ETunaSweeperItemSlotSource::UsableQuickSlot;
		TileData.SourceIndex = SlotIndex;
		TileData.SlotReference.Source = ETunaSweeperItemSlotSource::UsableQuickSlot;
		TileData.SlotReference.SlotIndex = SlotIndex;
		TileData.bIsEmpty = true;

		FTunaSweeperItemInstance ItemInstance;
		if (!TunaGameInstance || !Slot.ItemUid.IsValid() || !TunaGameInstance->TryGetItemInstance(Slot.ItemUid, ItemInstance))
		{
			return TileData;
		}

		TileData.ItemInstance = ItemInstance;
		TileData.ItemStack.ItemId = ItemInstance.ItemId;
		TileData.ItemStack.Quantity = FMath::Max(1, ItemInstance.Quantity);
		TileData.bIsEmpty = false;

		if (!ItemDataSubsystem)
		{
			TileData.DisplayName = FText::FromString(FString::Printf(TEXT("Item %d"), ItemInstance.ItemId));
			return TileData;
		}

		FTunaSweeperItemDefinition ItemDefinition;
		if (ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
		{
			TileData.ItemDefinition = ItemDefinition;
			TileData.bHasItemDefinition = true;

			FText DisplayName;
			if (ItemDataSubsystem->TryGetItemNameTextByKey(ItemDefinition.NameStringKey, ETunaSweeperItemTextLanguage::Korean, DisplayName))
			{
				TileData.DisplayName = DisplayName;
			}
			else
			{
				TileData.DisplayName = FText::FromString(FString::Printf(TEXT("Item %d"), ItemInstance.ItemId));
			}

			const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
			if (!IconObjectPath.IsEmpty())
			{
				TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
			}

			FText DescriptionText;
			if (ItemDataSubsystem->TryGetItemTextByKey(ItemDefinition.DescriptionStringKey, ETunaSweeperItemTextLanguage::Korean, DescriptionText))
			{
				TileData.DescriptionText = DescriptionText;
			}
		}

		return TileData;
	}
}

void UTunaSweeperGameHudWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnSelectedInventoryItemChanged.AddUObject(this, &UTunaSweeperGameHudWidget::HandleSelectedInventoryItemChanged);
	}
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
		QuestSubsystem->OnQuestProgressChanged.AddUObject(this, &UTunaSweeperGameHudWidget::HandleQuestProgressChanged);
	}
	if (TopStatusReserveWidget)
	{
		TopStatusReserveWidget->OnHudModeSelected.RemoveDynamic(this, &UTunaSweeperGameHudWidget::HandleHudModeTabSelected);
		TopStatusReserveWidget->OnHudModeSelected.AddDynamic(this, &UTunaSweeperGameHudWidget::HandleHudModeTabSelected);
	}

	EnsureQuestTrackerWidgets();
	EnsureInventoryQuickSlotPanelWidget();
	EnsureMapPanelWidget();
	EnsureMemoPanelWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	CacheAmmoReloadWidgets();
	SetHudMode(ETunaSweeperHudMode::None);
	SetItemInfoPanelVisible(false);
	RefreshBottomStatusFromGameInstance();
	RefreshQuestTrackerFromQuestSubsystem();
	RefreshQuickSlotsFromGameState();
	RefreshInventoryQuickSlotPanel();
	RefreshReloadWidgets();
	RefreshDialogueHudVisibility();
}

void UTunaSweeperGameHudWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
	}
	if (UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr)
	{
		QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
	}
	if (TopStatusReserveWidget)
	{
		TopStatusReserveWidget->OnHudModeSelected.RemoveDynamic(this, &UTunaSweeperGameHudWidget::HandleHudModeTabSelected);
	}

	Super::NativeDestruct();
}

void UTunaSweeperGameHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshBottomStatusFromGameInstance();
	RefreshQuestTrackerFromQuestSubsystem();
	RefreshQuickSlotsFromGameState();
	RefreshInventoryQuickSlotPanel();
	RefreshReloadWidgets();
	RefreshDialogueHudVisibility();
}

void UTunaSweeperGameHudWidget::SetCenterPanelsVisible(bool bVisible)
{
	if (!bVisible)
	{
		CloseLootContainerPanelIfOpen();
	}

	ActiveHudMode = bVisible && ActiveHudMode == ETunaSweeperHudMode::None
		? ETunaSweeperHudMode::Inventory
		: (bVisible ? ActiveHudMode : ETunaSweeperHudMode::None);
	ApplyHudModeVisibility();
}

void UTunaSweeperGameHudWidget::SetInventoryAreaVisible(bool bVisible)
{
	if (bVisible)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
		ApplyHudModeVisibility();
	}

	if (InventoryAreaWidget)
	{
		InventoryAreaWidget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		InventoryAreaWidget->SetInventoryVisible(bVisible);
	}
}

void UTunaSweeperGameHudWidget::SetItemInfoPanelVisible(bool bVisible)
{
	if (bVisible)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
		ApplyHudModeVisibility();
	}

	if (ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->SetVisibility(
			bVisible && ActiveHudMode == ETunaSweeperHudMode::Inventory
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperGameHudWidget::ShowExternalPanel(ETunaSweeperHudExternalPanelMode PanelMode)
{
	if (PanelMode != ETunaSweeperHudExternalPanelMode::LootingBox)
	{
		CloseLootContainerPanelIfOpen();
	}

	if (PanelMode != ETunaSweeperHudExternalPanelMode::None)
	{
		ActiveHudMode = ETunaSweeperHudMode::Inventory;
	}

	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetExternalPanelMode(PanelMode);
	}

	ApplyHudModeVisibility();
}

void UTunaSweeperGameHudWidget::ShowInventoryOnlyPanel()
{
	SetHudMode(ETunaSweeperHudMode::Inventory);
	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::None);
	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ToggleInventoryOnlyPanel()
{
	if (ActiveHudMode == ETunaSweeperHudMode::None)
	{
		ShowInventoryOnlyPanel();
	}
	else
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->ClearSelectedItemSelection();
		}

		CloseLootContainerPanelIfOpen();
		SetHudMode(ETunaSweeperHudMode::None);
	}
}

void UTunaSweeperGameHudWidget::ShowLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance)
{
	ActiveHudMode = ETunaSweeperHudMode::Inventory;

	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetLootContainerInstance(ContainerInstance);
	}

	ApplyHudModeVisibility();
	HandleSelectedInventoryItemChanged();
}

void UTunaSweeperGameHudWidget::ShowMemoPanel(int32 MemoId)
{
	SetHudMode(ETunaSweeperHudMode::Memo);
	EnsureMemoPanelWidget();
	if (MemoPanelWidget)
	{
		MemoPanelWidget->OpenMemo(MemoId);
	}
}

void UTunaSweeperGameHudWidget::SetHudMode(ETunaSweeperHudMode InHudMode)
{
	if (InHudMode != ETunaSweeperHudMode::Inventory)
	{
		CloseLootContainerPanelIfOpen();
	}

	ActiveHudMode = InHudMode;
	ApplyHudModeVisibility();
	HandleSelectedInventoryItemChanged();
}

bool UTunaSweeperGameHudWidget::IsInventoryUiOpen() const
{
	if (ActiveHudMode != ETunaSweeperHudMode::None)
	{
		return true;
	}

	auto IsWidgetVisible = [](const UWidget* Widget)
	{
		if (!Widget)
		{
			return false;
		}

		const ESlateVisibility Visibility = Widget->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	};

	if (CenterContentPanel)
	{
		return IsWidgetVisible(CenterContentPanel);
	}

	return IsWidgetVisible(InventoryAreaWidget) || IsWidgetVisible(ExternalPanelWidget);
}

void UTunaSweeperGameHudWidget::ApplyHudModeVisibility()
{
	const bool bUtilityModeOpen = ActiveHudMode != ETunaSweeperHudMode::None;
	const bool bInventoryMode = ActiveHudMode == ETunaSweeperHudMode::Inventory;
	const bool bMapMode = ActiveHudMode == ETunaSweeperHudMode::Map;
	const bool bMemoMode = ActiveHudMode == ETunaSweeperHudMode::Memo;

	if (TopStatusReserveWidget)
	{
		TopStatusReserveWidget->SetVisibility(bUtilityModeOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		TopStatusReserveWidget->SetActiveMode(ActiveHudMode);
	}

	if (CenterContentPanel)
	{
		CenterContentPanel->SetVisibility(bUtilityModeOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (InventoryAreaWidget)
	{
		InventoryAreaWidget->SetVisibility(bUtilityModeOpen && bInventoryMode ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		InventoryAreaWidget->SetInventoryVisible(bUtilityModeOpen && bInventoryMode);
	}

	EnsureInventoryQuickSlotPanelWidget();
	if (InventoryQuickSlotPanel)
	{
		InventoryQuickSlotPanel->SetVisibility(
			bUtilityModeOpen && bInventoryMode
				? ESlateVisibility::SelfHitTestInvisible
				: ESlateVisibility::Collapsed);
		if (bUtilityModeOpen && bInventoryMode)
		{
			RefreshInventoryQuickSlotPanel();
		}
	}

	if (ItemInfoPanelWidget && !bInventoryMode)
	{
		ItemInfoPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	}

	EnsureMapPanelWidget();
	if (MapPanelWidget)
	{
		MapPanelWidget->SetVisibility(bUtilityModeOpen && bMapMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (bUtilityModeOpen && bMapMode)
		{
			MapPanelWidget->RefreshMapView();
		}
	}

	EnsureMemoPanelWidget();
	if (MemoPanelWidget)
	{
		MemoPanelWidget->SetVisibility(bUtilityModeOpen && bMemoMode ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		if (bUtilityModeOpen && bMemoMode)
		{
			MemoPanelWidget->RefreshMemoView();
		}
	}

	if (ExternalPanelWidget)
	{
		const bool bShowExternalPanel =
			bUtilityModeOpen &&
			bInventoryMode &&
			ExternalPanelWidget->GetExternalPanelMode() != ETunaSweeperHudExternalPanelMode::None;
		ExternalPanelWidget->SetVisibility(bShowExternalPanel ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (UnsupportedModePanel)
	{
		UnsupportedModePanel->SetVisibility(
			bUtilityModeOpen && !bInventoryMode && !bMapMode && !bMemoMode
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	if (UnsupportedModeText)
	{
		UnsupportedModeText->SetText(FText::FromString(TEXT("\uBBF8\uAD6C\uD604")));
	}
}

void UTunaSweeperGameHudWidget::CloseLootContainerPanelIfOpen()
{
	if (!ExternalPanelWidget ||
		ExternalPanelWidget->GetExternalPanelMode() != ETunaSweeperHudExternalPanelMode::LootingBox)
	{
		return;
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->NotifyActiveLootContainerUiClosed();
	}

	ExternalPanelWidget->SetExternalPanelMode(ETunaSweeperHudExternalPanelMode::None);
}

void UTunaSweeperGameHudWidget::EnsureInventoryQuickSlotPanelWidget()
{
	if (InventoryQuickSlotPanel || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	TSubclassOf<UTunaSweeperItemThumbnailSlotWidget> EntryWidgetClass =
		LoadClass<UTunaSweeperItemThumbnailSlotWidget>(
			nullptr,
			TEXT("/Game/UI/WBP_ItemThumbnailSlot.WBP_ItemThumbnailSlot_C"));
	if (!EntryWidgetClass)
	{
		return;
	}

	InventoryQuickSlotPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryQuickSlotPanel"));
	UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryQuickSlotStack"));
	UTextBlock* GuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryQuickSlotGuideText"));
	InventoryQuickSlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryQuickSlotRow"));
	if (!InventoryQuickSlotPanel || !PanelStack || !GuideText || !InventoryQuickSlotRow)
	{
		return;
	}

	InventoryQuickSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	InventoryQuickSlotPanel->SetPadding(FMargin(18.0f, 12.0f, 18.0f, 12.0f));
	InventoryQuickSlotPanel->SetBrush(MakeHudRoundedBoxBrush(
		FVector2D(InventoryQuickSlotPanelWidth, InventoryQuickSlotPanelHeight),
		FLinearColor(0.015f, 0.018f, 0.018f, 0.68f),
		8.0f,
		FLinearColor(0.48f, 0.54f, 0.52f, 0.48f),
		1.0f));
	InventoryQuickSlotPanel->SetContent(PanelStack);

	GuideText->SetText(FText::FromString(TEXT("\uC544\uC774\uD15C\uC744 \uC2AC\uB86F\uC73C\uB85C \uB4DC\uB798\uADF8\uD558\uC5EC \uD035\uC2AC\uB86F\uC744 \uC124\uC815\uD558\uC138\uC694")));
	GuideText->SetColorAndOpacity(FSlateColor(FLinearColor(0.94f, 0.94f, 0.90f, 1.0f)));
	GuideText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(GuideText, 24, ETunaSweeperUIFontWeight::Bold);
	UVerticalBoxSlot* GuideSlot = PanelStack->AddChildToVerticalBox(GuideText);
	if (GuideSlot)
	{
		GuideSlot->SetHorizontalAlignment(HAlign_Fill);
		GuideSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	}

	UVerticalBoxSlot* RowStackSlot = PanelStack->AddChildToVerticalBox(InventoryQuickSlotRow);
	if (RowStackSlot)
	{
		RowStackSlot->SetHorizontalAlignment(HAlign_Center);
	}

	InventoryQuickSlotWidgets.Reset();
	for (int32 SlotNumber = InventoryQuickSlotFirstNumber; SlotNumber <= InventoryQuickSlotLastNumber; ++SlotNumber)
	{
		UVerticalBox* SlotStack = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dStack"), SlotNumber)));
		USizeBox* SlotSizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dSizeBox"), SlotNumber)));
		UTunaSweeperItemThumbnailSlotWidget* SlotWidget = WidgetTree->ConstructWidget<UTunaSweeperItemThumbnailSlotWidget>(
			EntryWidgetClass,
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dWidget"), SlotNumber)));
		USizeBox* KeySizeBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dKeySizeBox"), SlotNumber)));
		UBorder* KeyBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dKeyBackground"), SlotNumber)));
		UTextBlock* KeyText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("InventoryQuickSlot%dKeyText"), SlotNumber)));
		if (!SlotStack || !SlotSizeBox || !SlotWidget || !KeySizeBox || !KeyBackground || !KeyText)
		{
			continue;
		}

		SlotSizeBox->SetWidthOverride(InventoryQuickSlotTileSize);
		SlotSizeBox->SetHeightOverride(InventoryQuickSlotTileSize);
		SlotWidget->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		SlotWidget->SetRenderScale(FVector2D(InventoryQuickSlotTileScale, InventoryQuickSlotTileScale));
		SlotSizeBox->SetContent(SlotWidget);
		UVerticalBoxSlot* SlotWidgetStackSlot = SlotStack->AddChildToVerticalBox(SlotSizeBox);
		if (SlotWidgetStackSlot)
		{
			SlotWidgetStackSlot->SetHorizontalAlignment(HAlign_Center);
		}

		KeySizeBox->SetWidthOverride(34.0f);
		KeySizeBox->SetHeightOverride(28.0f);
		KeyBackground->SetPadding(FMargin(8.0f, 1.0f));
		KeyBackground->SetBrush(MakeHudRoundedBoxBrush(
			FVector2D(34.0f, 28.0f),
			FLinearColor(0.96f, 0.96f, 0.96f, 0.98f),
			5.0f,
			FLinearColor(1.0f, 1.0f, 1.0f, 0.98f),
			0.0f));
		KeyText->SetText(FText::AsNumber(SlotNumber));
		KeyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.02f, 0.024f, 0.028f, 1.0f)));
		KeyText->SetJustification(ETextJustify::Center);
		TunaSweeperUIFont::ApplyFont(KeyText, 17, ETunaSweeperUIFontWeight::Bold);
		KeyBackground->SetContent(KeyText);
		KeySizeBox->SetContent(KeyBackground);
		UVerticalBoxSlot* KeyStackSlot = SlotStack->AddChildToVerticalBox(KeySizeBox);
		if (KeyStackSlot)
		{
			KeyStackSlot->SetHorizontalAlignment(HAlign_Center);
			KeyStackSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
		}

		UHorizontalBoxSlot* RowSlot = InventoryQuickSlotRow->AddChildToHorizontalBox(SlotStack);
		if (RowSlot)
		{
			RowSlot->SetPadding(FMargin(SlotNumber == InventoryQuickSlotFirstNumber ? 0.0f : 12.0f, 0.0f, 0.0f, 0.0f));
			RowSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		InventoryQuickSlotWidgets.Add(SlotWidget);
	}

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(InventoryQuickSlotPanel);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
		CanvasSlot->SetPosition(FVector2D(0.0f, -34.0f));
		CanvasSlot->SetSize(FVector2D(InventoryQuickSlotPanelWidth, InventoryQuickSlotPanelHeight));
		CanvasSlot->SetZOrder(35);
	}
}

void UTunaSweeperGameHudWidget::EnsureMapPanelWidget()
{
	if (MapPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	MapPanelWidget = CreateWidget<UTunaSweeperMapWidget>(
		GetOwningPlayer(),
		UTunaSweeperMapWidget::StaticClass());
	if (!MapPanelWidget)
	{
		return;
	}

	MapPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(MapPanelWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetZOrder(-5);
	}
}

void UTunaSweeperGameHudWidget::EnsureMemoPanelWidget()
{
	if (MemoPanelWidget || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	MemoPanelWidget = CreateWidget<UTunaSweeperMemoWidget>(
		GetOwningPlayer(),
		UTunaSweeperMemoWidget::StaticClass());
	if (!MemoPanelWidget)
	{
		return;
	}

	MemoPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(MemoPanelWidget);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(FVector2D(0.0f, 34.0f));
		CanvasSlot->SetSize(FVector2D(1180.0f, 640.0f));
		CanvasSlot->SetZOrder(20);
	}
}

void UTunaSweeperGameHudWidget::RefreshBottomStatusFromGameInstance()
{
	if (!BottomStatusWidget && !InventoryAreaWidget)
	{
		return;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperPlayerHudState HudState = TunaGameInstance ? TunaGameInstance->PlayerHudState : FTunaSweeperPlayerHudState();

	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		const APawn* Pawn = PlayerController->GetPawn();
		const UTunaSweeperVitalsComponent* VitalsComponent = nullptr;
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(Pawn))
		{
			VitalsComponent = TunaCharacter->GetVitalsComponent();
		}
		else if (Pawn)
		{
			VitalsComponent = Pawn->FindComponentByClass<UTunaSweeperVitalsComponent>();
		}

		if (VitalsComponent)
		{
			const FTunaSweeperVitalsState& VitalsState = VitalsComponent->GetVitalsState();
			HudState.Health = VitalsState.Health;
			HudState.MaxHealth = VitalsState.MaxHealth;
			HudState.Food = VitalsState.Food;
			HudState.MaxFood = VitalsState.MaxFood;
			HudState.Hydration = VitalsState.Hydration;
			HudState.MaxHydration = VitalsState.MaxHydration;
		}
	}

	if (BottomStatusWidget)
	{
		BottomStatusWidget->SetHudState(HudState);
	}

	if (InventoryAreaWidget)
	{
		InventoryAreaWidget->SetHudState(HudState);
	}
}

void UTunaSweeperGameHudWidget::RefreshQuickSlotsFromGameState()
{
	if (!QuickSlotBarWidget)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	int32 SelectedSlotNumber = 0;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn()))
		{
			SelectedSlotNumber = TunaCharacter->GetSelectedWeaponSlotNumber();
		}
	}
	QuickSlotBarWidget->SetSelectedQuickSlot(SelectedSlotNumber);

	for (int32 SlotNumber = 1; SlotNumber <= 2; ++SlotNumber)
	{
		FTunaSweeperItemInstance WeaponInstance;
		FTunaSweeperItemDefinition WeaponDefinition;
		if (!TunaGameInstance ||
			!TunaGameInstance->TryGetEquipmentWeaponSlotItem(SlotNumber, WeaponInstance, WeaponDefinition))
		{
			QuickSlotBarWidget->ClearQuickSlotIcon(SlotNumber);
			QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, FText::GetEmpty(), false);
			QuickSlotBarWidget->SetWeaponAmmoText(SlotNumber, 0, 0, false);
			continue;
		}

		UTexture2D* IconTexture = nullptr;
		if (ItemDataSubsystem)
		{
			const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(WeaponDefinition);
			if (!IconObjectPath.IsEmpty())
			{
				IconTexture = LoadObject<UTexture2D>(nullptr, *IconObjectPath);
			}
		}
		QuickSlotBarWidget->SetQuickSlotIcon(SlotNumber, IconTexture);

		FText AmmoTypeText = FText::FromString(TEXT("\uD0C4\uC57D \uBBF8\uC9C0\uC815"));
		const int32 AmmoItemId = TunaGameInstance->GetWeaponSelectedAmmoItemId(SlotNumber);
		if (AmmoItemId != INDEX_NONE)
		{
			AmmoTypeText = FText::FromString(FString::Printf(TEXT("Ammo %d"), AmmoItemId));
			if (ItemDataSubsystem)
			{
				ItemDataSubsystem->TryGetItemNameText(AmmoItemId, ETunaSweeperItemTextLanguage::Korean, AmmoTypeText);
			}
		}
		QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, AmmoTypeText, true);
		QuickSlotBarWidget->SetWeaponAmmoText(
			SlotNumber,
			TunaGameInstance->GetWeaponLoadedAmmoCount(SlotNumber),
			TunaGameInstance->GetWeaponInventoryAmmoCount(SlotNumber),
			true);
	}

	static const TArray<FTunaSweeperInventorySlot> EmptyQuickSlots;
	const TArray<FTunaSweeperInventorySlot>& UsableQuickSlots = TunaGameInstance
		? TunaGameInstance->GetUsableQuickSlots()
		: EmptyQuickSlots;
	for (int32 SlotNumber = InventoryQuickSlotFirstNumber; SlotNumber <= InventoryQuickSlotLastNumber; ++SlotNumber)
	{
		const int32 SlotIndex = SlotNumber - InventoryQuickSlotFirstNumber;
		FTunaSweeperItemInstance ItemInstance;
		FTunaSweeperItemDefinition ItemDefinition;
		if (!UsableQuickSlots.IsValidIndex(SlotIndex) ||
			!TunaGameInstance ||
			!TunaGameInstance->TryGetItemInstance(UsableQuickSlots[SlotIndex].ItemUid, ItemInstance) ||
			!ItemDataSubsystem ||
			!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
		{
			QuickSlotBarWidget->ClearQuickSlotIcon(SlotNumber);
			QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, FText::GetEmpty(), false);
			QuickSlotBarWidget->SetWeaponAmmoText(SlotNumber, 0, 0, false);
			continue;
		}

		UTexture2D* IconTexture = nullptr;
		const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
		if (!IconObjectPath.IsEmpty())
		{
			IconTexture = LoadObject<UTexture2D>(nullptr, *IconObjectPath);
		}

		QuickSlotBarWidget->SetQuickSlotIcon(SlotNumber, IconTexture);
		QuickSlotBarWidget->SetWeaponAmmoTypeText(SlotNumber, FText::GetEmpty(), false);
		QuickSlotBarWidget->SetWeaponAmmoText(SlotNumber, 0, 0, false);
	}
}

void UTunaSweeperGameHudWidget::RefreshInventoryQuickSlotPanel()
{
	if (InventoryQuickSlotWidgets.Num() <= 0)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	static const TArray<FTunaSweeperInventorySlot> EmptyQuickSlots;
	const TArray<FTunaSweeperInventorySlot>& UsableQuickSlots = TunaGameInstance
		? TunaGameInstance->GetUsableQuickSlots()
		: EmptyQuickSlots;

	for (int32 SlotIndex = 0; SlotIndex < InventoryQuickSlotWidgets.Num(); ++SlotIndex)
	{
		if (!InventoryQuickSlotWidgets[SlotIndex])
		{
			continue;
		}

		const FTunaSweeperInventorySlot& QuickSlot = UsableQuickSlots.IsValidIndex(SlotIndex)
			? UsableQuickSlots[SlotIndex]
			: FTunaSweeperInventorySlot();
		InventoryQuickSlotWidgets[SlotIndex]->SetTileData(BuildQuickSlotTileData(
			TunaGameInstance,
			ItemDataSubsystem,
			QuickSlot,
			SlotIndex));
	}
}

void UTunaSweeperGameHudWidget::RefreshReloadWidgets()
{
	CacheAmmoReloadWidgets();

	const bool bDialogueActive = IsDialogueSequenceActive();
	ATunaSweeperTopDownCharacter* TunaCharacter = nullptr;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn());
	}

	const bool bShowReload = !bDialogueActive && TunaCharacter && TunaCharacter->IsWeaponReloading();
	const float ReloadProgress = bShowReload ? TunaCharacter->GetReloadProgress() : 0.0f;
	bool bShowReloadPrompt = false;
	if (!bDialogueActive && !bShowReload && TunaCharacter && !TunaCharacter->IsAmmoSelectionOpen() && !IsInventoryUiOpen())
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			const int32 SelectedWeaponSlotNumber = TunaCharacter->GetSelectedWeaponSlotNumber();
			bShowReloadPrompt =
				SelectedWeaponSlotNumber > 0 &&
				TunaGameInstance->IsEquipmentWeaponSlotOccupied(SelectedWeaponSlotNumber) &&
				TunaGameInstance->GetWeaponMagazineCapacity(SelectedWeaponSlotNumber) > 0 &&
				TunaGameInstance->GetWeaponLoadedAmmoCount(SelectedWeaponSlotNumber) <= 0 &&
				TunaGameInstance->GetWeaponSelectedAmmoItemId(SelectedWeaponSlotNumber) != INDEX_NONE &&
				TunaGameInstance->GetWeaponInventoryAmmoCount(SelectedWeaponSlotNumber) > 0;
		}
	}

	if (QuickSlotBarWidget)
	{
		QuickSlotBarWidget->SetReloadProgress(ReloadProgress, bShowReload);
	}

	if (CenterReloadGaugeRoot)
	{
		CenterReloadGaugeRoot->SetVisibility(bShowReload ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CenterReloadPromptRoot)
	{
		CenterReloadPromptRoot->SetVisibility(bShowReloadPrompt ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CenterReloadPercentText)
	{
		CenterReloadPercentText->SetText(
			bShowReload
				? FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(ReloadProgress * 100.0f)))
				: FText::GetEmpty());
	}

	const int32 FilledSegmentCount = FMath::CeilToInt(ReloadProgress * CenterReloadSegments.Num());
	for (int32 SegmentIndex = 0; SegmentIndex < CenterReloadSegments.Num(); ++SegmentIndex)
	{
		if (CenterReloadSegments[SegmentIndex])
		{
			CenterReloadSegments[SegmentIndex]->SetRenderOpacity(
				bShowReload && SegmentIndex < FilledSegmentCount ? 1.0f : 0.18f);
		}
	}

	TArray<FText> AmmoOptionTexts;
	int32 FocusedOptionIndex = INDEX_NONE;
	BuildAmmoSelectorOptionTexts(AmmoOptionTexts, FocusedOptionIndex);
	if (QuickSlotBarWidget)
	{
		const int32 SelectedWeaponSlotNumber = TunaCharacter ? TunaCharacter->GetSelectedWeaponSlotNumber() : 0;
		const bool bAmmoSelectionOpen = TunaCharacter && TunaCharacter->IsAmmoSelectionOpen();
		if (bAmmoSelectionOpen)
		{
			QuickSlotBarWidget->SetAmmoSelectorOptions(
				AmmoOptionTexts,
				FocusedOptionIndex,
				SelectedWeaponSlotNumber,
				true);
		}
		else
		{
			QuickSlotBarWidget->SetAmmoSelectorPrompt(SelectedWeaponSlotNumber, FText::GetEmpty(), false);
		}
	}
}

void UTunaSweeperGameHudWidget::CacheAmmoReloadWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	CenterReloadGaugeRoot = WidgetTree->FindWidget(FName(TEXT("CenterReloadGaugeRoot")));
	CenterReloadPromptRoot = WidgetTree->FindWidget(FName(TEXT("CenterReloadPromptRoot")));
	CenterReloadPercentText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("CenterReloadPercentText"))));
	CenterReloadSegments.SetNum(12);
	for (int32 SegmentNumber = 1; SegmentNumber <= CenterReloadSegments.Num(); ++SegmentNumber)
	{
		CenterReloadSegments[SegmentNumber - 1] = Cast<UBorder>(WidgetTree->FindWidget(
			FName(*FString::Printf(TEXT("CenterReloadSegment%02d"), SegmentNumber))));
	}
}

void UTunaSweeperGameHudWidget::RefreshDialogueHudVisibility()
{
	const bool bDialogueActive = IsDialogueSequenceActive();
	const bool bInventoryUiOpen = IsInventoryUiOpen();
	const ESlateVisibility BottomStatusVisibility = bDialogueActive || bInventoryUiOpen
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;
	const ESlateVisibility QuickSlotVisibility = bDialogueActive || bInventoryUiOpen
		? ESlateVisibility::Collapsed
		: ESlateVisibility::HitTestInvisible;

	if (BottomStatusWidget)
	{
		BottomStatusWidget->SetVisibility(BottomStatusVisibility);
	}

	if (QuickSlotBarWidget)
	{
		QuickSlotBarWidget->SetVisibility(QuickSlotVisibility);
	}
}

void UTunaSweeperGameHudWidget::EnsureQuestTrackerWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!QuestTrackerRoot)
	{
		QuestTrackerRoot = Cast<UBorder>(WidgetTree->FindWidget(FName(TEXT("QuestTrackerRoot"))));
		QuestTrackerTitleText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("QuestTrackerTitleText"))));
		QuestTrackerObjectiveText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("QuestTrackerObjectiveText"))));
	}

	if (QuestTrackerRoot)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	QuestTrackerRoot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("QuestTrackerRoot"));
	UVerticalBox* TrackerStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("QuestTrackerStack"));
	QuestTrackerTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestTrackerTitleText"));
	QuestTrackerObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestTrackerObjectiveText"));
	if (!QuestTrackerRoot || !TrackerStack || !QuestTrackerTitleText || !QuestTrackerObjectiveText)
	{
		return;
	}

	QuestTrackerRoot->SetPadding(FMargin(14.0f, 10.0f));
	QuestTrackerRoot->SetBrushColor(FLinearColor(0.02f, 0.025f, 0.03f, 0.78f));
	QuestTrackerRoot->SetContent(TrackerStack);

	QuestTrackerTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.98f, 1.0f, 1.0f)));
	QuestTrackerTitleText->SetAutoWrapText(false);
	TunaSweeperUIFont::ApplyFont(QuestTrackerTitleText, 18, ETunaSweeperUIFontWeight::Bold);

	QuestTrackerObjectiveText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 0.86f, 0.90f, 1.0f)));
	QuestTrackerObjectiveText->SetAutoWrapText(true);
	QuestTrackerObjectiveText->SetWrapTextAt(320.0f);
	TunaSweeperUIFont::ApplyFont(QuestTrackerObjectiveText, 15);

	UVerticalBoxSlot* TitleSlot = TrackerStack->AddChildToVerticalBox(QuestTrackerTitleText);
	if (TitleSlot)
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}
	TrackerStack->AddChildToVerticalBox(QuestTrackerObjectiveText);

	UCanvasPanelSlot* CanvasSlot = RootCanvas->AddChildToCanvas(QuestTrackerRoot);
	if (CanvasSlot)
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		CanvasSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		CanvasSlot->SetPosition(FVector2D(24.0f, 104.0f));
		CanvasSlot->SetSize(FVector2D(360.0f, 132.0f));
		CanvasSlot->SetZOrder(5);
	}
}

void UTunaSweeperGameHudWidget::RefreshQuestTrackerFromQuestSubsystem()
{
	EnsureQuestTrackerWidgets();
	if (!QuestTrackerRoot)
	{
		return;
	}

	UTunaSweeperQuestSubsystem* QuestSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	if (!QuestSubsystem || QuestSubsystem->GetTrackedQuestId().IsNone())
	{
		QuestTrackerRoot->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	FTunaSweeperQuestDefinition QuestDefinition;
	const FName TrackedQuestId = QuestSubsystem->GetTrackedQuestId();
	if (!QuestSubsystem->TryGetQuestDefinition(TrackedQuestId, QuestDefinition))
	{
		QuestTrackerRoot->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TArray<FTunaSweeperObjectiveProgressView> ObjectiveProgress;
	if (!QuestSubsystem->GetQuestObjectiveProgress(TrackedQuestId, ObjectiveProgress))
	{
		QuestTrackerRoot->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (QuestTrackerTitleText)
	{
		QuestTrackerTitleText->SetText(QuestDefinition.Title);
	}

	if (QuestTrackerObjectiveText)
	{
		TArray<FString> ObjectiveLines;
		for (const FTunaSweeperObjectiveProgressView& Progress : ObjectiveProgress)
		{
			ObjectiveLines.Add(FString::Printf(
				TEXT("%s (%d/%d)"),
				*Progress.Text.ToString(),
				FMath::Clamp(Progress.CurrentCount, 0, FMath::Max(1, Progress.RequiredCount)),
				FMath::Max(1, Progress.RequiredCount)));
		}

		if (QuestSubsystem->GetQuestState(TrackedQuestId) == ETunaSweeperQuestState::RewardAvailable)
		{
			ObjectiveLines.Add(TEXT("\uBCF4\uC0C1 \uC218\uB839 \uAC00\uB2A5"));
		}

		QuestTrackerObjectiveText->SetText(FText::FromString(FString::Join(ObjectiveLines, LINE_TERMINATOR)));
	}

	QuestTrackerRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTunaSweeperGameHudWidget::BuildAmmoSelectorOptionTexts(TArray<FText>& OutOptionTexts, int32& OutFocusedIndex) const
{
	OutOptionTexts.Reset();
	OutFocusedIndex = INDEX_NONE;

	const APlayerController* PlayerController = GetOwningPlayer();
	const ATunaSweeperTopDownCharacter* TunaCharacter = PlayerController
		? Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn())
		: nullptr;
	if (!TunaCharacter || !TunaCharacter->IsAmmoSelectionOpen())
	{
		return;
	}

	TArray<int32> AmmoItemIds;
	TunaCharacter->GetAmmoSelectionItemIds(AmmoItemIds);
	if (AmmoItemIds.Num() <= 0)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	for (int32 AmmoItemId : AmmoItemIds)
	{
		FText AmmoName = FText::FromString(FString::Printf(TEXT("Ammo %d"), AmmoItemId));
		if (ItemDataSubsystem)
		{
			ItemDataSubsystem->TryGetItemNameText(AmmoItemId, ETunaSweeperItemTextLanguage::Korean, AmmoName);
		}
		OutOptionTexts.Add(AmmoName);
	}

	OutFocusedIndex = TunaCharacter->GetAmmoSelectionFocusIndex();
}

bool UTunaSweeperGameHudWidget::IsDialogueSequenceActive() const
{
	const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer());
	return TunaPlayerController && TunaPlayerController->IsDialogueSequenceActive();
}

void UTunaSweeperGameHudWidget::HandleSelectedInventoryItemChanged()
{
	const bool bCenterVisible =
		ActiveHudMode == ETunaSweeperHudMode::Inventory &&
		CenterContentPanel &&
		CenterContentPanel->GetVisibility() != ESlateVisibility::Collapsed;
	const bool bHasSelection = bCenterVisible &&
		GetGameInstance<UTunaSweeperGameInstance>() &&
		GetGameInstance<UTunaSweeperGameInstance>()->HasSelectedInventoryItem();

	SetItemInfoPanelVisible(bHasSelection);
	if (bHasSelection && ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->RefreshSelectedItemInfo();
	}
}

void UTunaSweeperGameHudWidget::HandleQuestProgressChanged()
{
	RefreshQuestTrackerFromQuestSubsystem();
}

void UTunaSweeperGameHudWidget::HandleHudModeTabSelected(ETunaSweeperHudMode SelectedMode)
{
	SetHudMode(SelectedMode);
}
