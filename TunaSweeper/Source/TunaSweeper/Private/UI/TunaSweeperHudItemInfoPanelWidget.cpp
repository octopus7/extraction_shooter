#include "UI/TunaSweeperHudItemInfoPanelWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/PanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperItemDragDropOperation.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"
#include "Weapon/TunaSweeperProjectile.h"

namespace TunaSweeperItemInfoPanel
{
	constexpr int32 AttachmentSlotColumnCount = 2;
	constexpr float AttachmentSlotTileWidth = 96.0f;
	constexpr float AttachmentSlotTileHeight = 96.0f;
	constexpr float SelectedItemIconSize = 132.0f;
	constexpr float DefaultProjectileDamageAmount = 10.0f;

	using TunaSweeperUiText::ResolveUiText;

	float GetDefaultProjectileDamageAmount()
	{
		const ATunaSweeperProjectile* DefaultProjectile =
			ATunaSweeperProjectile::StaticClass()->GetDefaultObject<ATunaSweeperProjectile>();
		return DefaultProjectile ? FMath::Max(0.0f, DefaultProjectile->GetDamageAmount()) : DefaultProjectileDamageAmount;
	}

	FText BuildItemFormulaInfo(const FTunaSweeperItemDefinition& ItemDefinition, FLinearColor& OutTextColor)
	{
		OutTextColor = FLinearColor(0.72f, 0.84f, 0.88f, 1.0f);
		if (!ItemDefinition.AmmoTypeTag.IsNone())
		{
			const int32 BaseDamage = FMath::Max(0, FMath::RoundToInt(GetDefaultProjectileDamageAmount()));
			const float DamageMultiplier = FMath::Max(
				0.0f,
				TunaSweeperDataValues::ToRatioFloat(ItemDefinition.ProjectileDamageMultiplier));
			const int32 DamageBonus = ItemDefinition.ProjectileDamageBonus;
			const int32 ResultDamage = FMath::Max(0, FMath::RoundToInt(BaseDamage * DamageMultiplier) + DamageBonus);

			if (ResultDamage > BaseDamage)
			{
				OutTextColor = FLinearColor(0.66f, 0.95f, 0.70f, 1.0f);
			}
			else if (ResultDamage < BaseDamage)
			{
				OutTextColor = FLinearColor(0.98f, 0.72f, 0.56f, 1.0f);
			}

			const FString FormulaString = FMath::IsNearlyEqual(DamageMultiplier, 1.0f)
				? FString::Printf(
					TEXT("\uD0C4\uC57D \uC218\uC2DD\n")
					TEXT("\uD53C\uD574\uB7C9 = \uAE30\uBCF8 %d + \uD0C4\uC57D %+d = %d\n")
					TEXT("\uD0C4\uC885 = %s"),
					BaseDamage,
					DamageBonus,
					ResultDamage,
					*ItemDefinition.AmmoTypeTag.ToString())
				: FString::Printf(
					TEXT("\uD0C4\uC57D \uC218\uC2DD\n")
					TEXT("\uD53C\uD574\uB7C9 = round(\uAE30\uBCF8 %d x %.2f) + \uD0C4\uC57D %+d = %d\n")
					TEXT("\uD0C4\uC885 = %s"),
					BaseDamage,
					DamageMultiplier,
					DamageBonus,
					ResultDamage,
					*ItemDefinition.AmmoTypeTag.ToString());

			return FText::FromString(FormulaString);
		}

		if (ItemDefinition.DefenseValue > 0)
		{
			const int32 DefenseValue = FMath::Max(0, ItemDefinition.DefenseValue);
			OutTextColor = FLinearColor(0.68f, 0.88f, 1.0f, 1.0f);
			const FString FormulaString = FString::Printf(
				TEXT("\uBC29\uC5B4 \uC218\uC2DD\n")
				TEXT("\uBC1B\uB294 \uD53C\uD574 = max(0, \uB4E4\uC5B4\uC628 \uD53C\uD574 - \uBC29\uC5B4 %d)\n")
				TEXT("\uBC29\uC5B4 = %d"),
				DefenseValue,
				DefenseValue);

			return FText::FromString(FormulaString);
		}

		return FText::GetEmpty();
	}

	FText GetAttachmentSlotDisplayName(FName AttachmentSlotTag, const UTunaSweeperGameInstance* TunaGameInstance)
	{
		if (AttachmentSlotTag == TEXT("attachment.slot.magazine"))
		{
			return ResolveUiText(TunaGameInstance, TEXT("ui.item_info.attachment_magazine"), TEXT("\uD0C4\uCC3D"));
		}
		if (AttachmentSlotTag == TEXT("attachment.slot.optic"))
		{
			return ResolveUiText(TunaGameInstance, TEXT("ui.item_info.attachment_optic"), TEXT("\uAD11\uD559"));
		}
		if (AttachmentSlotTag == TEXT("attachment.slot.tactical"))
		{
			return ResolveUiText(TunaGameInstance, TEXT("ui.item_info.attachment_tactical"), TEXT("\uC804\uC220 \uC7A5\uBE44"));
		}

		return FText::FromName(AttachmentSlotTag);
	}

	bool TryResolveSlotFromTileView(
		const UTileView* TileView,
		int32 SlotCount,
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference)
	{
		if (!TileView || SlotCount <= 0)
		{
			return false;
		}

		const FGeometry& TileViewGeometry = TileView->GetCachedGeometry();
		const FVector2D LocalPosition = TileViewGeometry.AbsoluteToLocal(ScreenSpacePosition);
		const FVector2D LocalSize = TileViewGeometry.GetLocalSize();
		if (LocalPosition.X < 0.0f || LocalPosition.Y < 0.0f ||
			LocalPosition.X >= LocalSize.X || LocalPosition.Y >= LocalSize.Y)
		{
			return false;
		}

		const float EntryWidth = FMath::Max(1.0f, TileView->GetEntryWidth());
		const float EntryHeight = FMath::Max(1.0f, TileView->GetEntryHeight());
		const int32 ColumnIndex = FMath::FloorToInt(LocalPosition.X / EntryWidth);
		const int32 RowIndex = FMath::FloorToInt(LocalPosition.Y / EntryHeight);
		if (ColumnIndex < 0 || ColumnIndex >= AttachmentSlotColumnCount || RowIndex < 0)
		{
			return false;
		}

		const int32 FirstVisibleItemIndex = FMath::Max(0, FMath::FloorToInt(TileView->GetScrollOffset()));
		const int32 SlotIndex = FirstVisibleItemIndex + RowIndex * AttachmentSlotColumnCount + ColumnIndex;
		if (SlotIndex < 0 || SlotIndex >= SlotCount)
		{
			return false;
		}

		OutSlotReference.Source = ETunaSweeperItemSlotSource::SelectedWeaponAttachment;
		OutSlotReference.SlotIndex = SlotIndex;
		return true;
	}

	bool TryMoveFromDropSlot(
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDragDropOperation* ItemDragOperation,
		const FTunaSweeperItemSlotReference& TargetSlot)
	{
		if (!TunaGameInstance || !ItemDragOperation || ItemDragOperation->TileData.bIsEmpty || !TargetSlot.IsValid())
		{
			return false;
		}

		FTunaSweeperItemSlotReference SourceSlot = ItemDragOperation->TileData.SlotReference;
		if (!SourceSlot.IsValid())
		{
			SourceSlot.Source = ItemDragOperation->TileData.Source;
			SourceSlot.SlotIndex = ItemDragOperation->TileData.SourceIndex;
		}

		const bool bMoved = TunaGameInstance->MoveItemBetweenSlots(SourceSlot, TargetSlot);
		ItemDragOperation->bHasHoveredSlotReference = false;
		ItemDragOperation->HoveredSlotReference = FTunaSweeperItemSlotReference();
		return bMoved;
	}

	bool TryMoveFromHoveredDropSlot(
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDragDropOperation* ItemDragOperation)
	{
		if (!TunaGameInstance || !ItemDragOperation || ItemDragOperation->TileData.bIsEmpty ||
			!ItemDragOperation->bHasHoveredSlotReference || !ItemDragOperation->HoveredSlotReference.IsValid())
		{
			return false;
		}

		const bool bMoved = TryMoveFromDropSlot(TunaGameInstance, ItemDragOperation, ItemDragOperation->HoveredSlotReference);
		ItemDragOperation->bHasHoveredSlotReference = false;
		ItemDragOperation->HoveredSlotReference = FTunaSweeperItemSlotReference();
		return bMoved;
	}
}

void UTunaSweeperHudItemInfoPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	CacheNamedWidgets();
	EnsureThumbnailWidgets();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnSelectedInventoryItemChanged.AddUObject(this, &UTunaSweeperHudItemInfoPanelWidget::RefreshSelectedItemInfo);
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperHudItemInfoPanelWidget::RefreshSelectedItemInfo);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperHudItemInfoPanelWidget::RefreshSelectedItemInfo);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudItemInfoPanelWidget::HandleCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UTunaSweeperHudItemInfoPanelWidget::HandleCloseButtonClicked);
	}

	RefreshSelectedItemInfo();
}

void UTunaSweeperHudItemInfoPanelWidget::NativeDestruct()
{
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudItemInfoPanelWidget::HandleCloseButtonClicked);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

bool UTunaSweeperHudItemInfoPanelWidget::NativeOnDrop(
	const FGeometry& InGeometry,
	const FDragDropEvent& InDragDropEvent,
	UDragDropOperation* InOperation)
{
	UTunaSweeperItemDragDropOperation* ItemDragOperation = Cast<UTunaSweeperItemDragDropOperation>(InOperation);
	if (!ItemDragOperation || ItemDragOperation->TileData.bIsEmpty)
	{
		return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperItemSlotReference CursorSlotReference;
	if (TryResolveAttachmentDropSlotFromCursor(InDragDropEvent.GetScreenSpacePosition(), CursorSlotReference) &&
		TunaSweeperItemInfoPanel::TryMoveFromDropSlot(TunaGameInstance, ItemDragOperation, CursorSlotReference))
	{
		return true;
	}

	if (TunaSweeperItemInfoPanel::TryMoveFromHoveredDropSlot(TunaGameInstance, ItemDragOperation))
	{
		return true;
	}

	return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
}

void UTunaSweeperHudItemInfoPanelWidget::RefreshSelectedItemInfo()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;

	FTunaSweeperItemInstance SelectedItemInstance;
	FTunaSweeperItemDefinition SelectedItemDefinition;
	if (!TunaGameInstance || !ItemDataSubsystem ||
		!TunaGameInstance->TryGetSelectedItemInstance(SelectedItemInstance) ||
		!ItemDataSubsystem->TryGetItemDefinition(SelectedItemInstance.ItemId, SelectedItemDefinition))
	{
		ClearSelectedItemInfo();
		return;
	}

	const ETunaSweeperItemTextLanguage Language = TunaGameInstance->GetCurrentTextLanguage();
	FText DisplayName;
	if (!ItemDataSubsystem->TryGetItemNameTextByKey(SelectedItemDefinition.NameStringKey, Language, DisplayName))
	{
		DisplayName = FText::FromString(FString::Printf(TEXT("Item %d"), SelectedItemInstance.ItemId));
	}

	FText Description;
	ItemDataSubsystem->TryGetItemDescriptionText(SelectedItemInstance.ItemId, Language, Description);

	UTexture2D* IconTexture = nullptr;
	const FString SelectedIconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(SelectedItemDefinition);
	if (!SelectedIconObjectPath.IsEmpty())
	{
		IconTexture = LoadObject<UTexture2D>(nullptr, *SelectedIconObjectPath);
	}
	SetSelectedItemThumbnail(IconTexture);

	const TArray<FTunaSweeperInventorySlot>& AttachmentSlots = TunaGameInstance->GetSelectedWeaponAttachmentSlots();
	const TArray<FName>& AttachmentSlotTags = TunaGameInstance->GetSelectedWeaponAttachmentSlotTags();
	SetSelectedItemInfo(DisplayName, Description, AttachmentSlots.Num() > 0);
	FLinearColor FormulaTextColor;
	const FText FormulaText = TunaSweeperItemInfoPanel::BuildItemFormulaInfo(
		SelectedItemDefinition,
		FormulaTextColor);
	SetSelectedItemFormulaInfo(FormulaText, FormulaTextColor, !FormulaText.IsEmpty());

	AttachmentTileObjects.Reset();
	if (AttachmentSlotTileView)
	{
		AttachmentSlotTileView->ClearListItems();
		AttachmentSlotTileView->SetEntryWidth(TunaSweeperItemInfoPanel::AttachmentSlotTileWidth);
		AttachmentSlotTileView->SetEntryHeight(TunaSweeperItemInfoPanel::AttachmentSlotTileHeight);

		for (int32 SlotIndex = 0; SlotIndex < AttachmentSlots.Num(); ++SlotIndex)
		{
			FTunaSweeperItemStackTileData TileData;
			TileData.Source = ETunaSweeperItemSlotSource::SelectedWeaponAttachment;
			TileData.SourceIndex = SlotIndex;
			TileData.SlotReference.Source = ETunaSweeperItemSlotSource::SelectedWeaponAttachment;
			TileData.SlotReference.SlotIndex = SlotIndex;
			TileData.bIsEmpty = true;
			TileData.bShowEmptySlotLabel = true;
			TileData.DisplayName = AttachmentSlotTags.IsValidIndex(SlotIndex)
				? TunaSweeperItemInfoPanel::GetAttachmentSlotDisplayName(AttachmentSlotTags[SlotIndex], TunaGameInstance)
				: TunaSweeperItemInfoPanel::ResolveUiText(TunaGameInstance, TEXT("ui.item_info.mod"), TEXT("Mod"));

			if (AttachmentSlots.IsValidIndex(SlotIndex) && AttachmentSlots[SlotIndex].ItemUid.IsValid() &&
				TunaGameInstance->TryGetItemInstance(AttachmentSlots[SlotIndex].ItemUid, TileData.ItemInstance))
			{
				TileData.bIsEmpty = false;
				TileData.ItemStack.ItemId = TileData.ItemInstance.ItemId;
				TileData.ItemStack.Quantity = FMath::Max(1, TileData.ItemInstance.Quantity);

				FTunaSweeperItemDefinition AttachmentDefinition;
				if (ItemDataSubsystem->TryGetItemDefinition(TileData.ItemInstance.ItemId, AttachmentDefinition))
				{
					ItemDataSubsystem->TryGetItemNameTextByKey(AttachmentDefinition.NameStringKey, Language, TileData.DisplayName);
					const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(AttachmentDefinition);
					if (!IconObjectPath.IsEmpty())
					{
						TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
					}
				}
			}

			UTunaSweeperItemStackTileItemObject* TileObject = NewObject<UTunaSweeperItemStackTileItemObject>(this);
			if (TileObject)
			{
				TileObject->Initialize(TileData);
				AttachmentTileObjects.Add(TileObject);
				AttachmentSlotTileView->AddItem(TileObject);
			}
		}
	}

	if (ModdingText)
	{
		const bool bHasRifleModding = SelectedItemDefinition.WeaponTypeTag == TEXT("weapon.type.rifle") && AttachmentSlots.Num() > 0;
		ModdingText->SetText(bHasRifleModding
			? TunaSweeperItemInfoPanel::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.item_info.rifle_modding"),
				TEXT("\uC18C\uCD1D \uBAA8\uB529: \uB300\uC6A9\uB7C9 \uD0C4\uCC3D / \uAD11\uD559 \uC7A5\uBE44"))
			: TunaSweeperItemInfoPanel::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.item_info.attachments"),
				TEXT("\uBD80\uCC29\uBB3C")));
	}
}

void UTunaSweeperHudItemInfoPanelWidget::SetSelectedItemInfo(const FText& ItemName, const FText& ItemDescription, bool bShowModdingPanel)
{
	if (SelectedItemNameText)
	{
		SelectedItemNameText->SetText(ItemName);
	}

	if (SelectedItemDescriptionText)
	{
		SelectedItemDescriptionText->SetText(ItemDescription);
	}

	SetModdingPanelVisible(bShowModdingPanel);
}

void UTunaSweeperHudItemInfoPanelWidget::ClearSelectedItemInfo()
{
	ClearSelectedItemThumbnail();
	AttachmentTileObjects.Reset();
	if (AttachmentSlotTileView)
	{
		AttachmentSlotTileView->ClearListItems();
	}

	if (SelectedItemNameText)
	{
		const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
		SelectedItemNameText->SetText(TunaSweeperItemInfoPanel::ResolveUiText(
			TunaGameInstance,
			TEXT("ui.common.no_item"),
			TEXT("No Item")));
	}

	if (SelectedItemDescriptionText)
	{
		SelectedItemDescriptionText->SetText(FText::GetEmpty());
	}

	SetSelectedItemFormulaInfo(FText::GetEmpty(), FLinearColor::White, false);
	SetModdingPanelVisible(false);
}

void UTunaSweeperHudItemInfoPanelWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!PanelStack)
	{
		PanelStack = Cast<UVerticalBox>(WidgetTree->FindWidget(FName(TEXT("PanelStack"))));
	}
	if (!HeaderRow)
	{
		HeaderRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("HeaderRow"))));
	}
	if (!SelectedItemDetailRow)
	{
		SelectedItemDetailRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("SelectedItemDetailRow"))));
	}
	if (!SelectedItemIconContainer)
	{
		SelectedItemIconContainer = WidgetTree->FindWidget(FName(TEXT("SelectedItemIconContainer")));
	}
	if (!SelectedItemIconImage)
	{
		SelectedItemIconImage = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("SelectedItemIconImage"))));
	}
	if (!SelectedItemDescriptionText)
	{
		SelectedItemDescriptionText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("SelectedItemDescriptionText"))));
	}
	if (!SelectedItemFormulaText)
	{
		SelectedItemFormulaText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("SelectedItemFormulaText"))));
	}
}

void UTunaSweeperHudItemInfoPanelWidget::EnsureThumbnailWidgets()
{
	CacheNamedWidgets();
	if (!WidgetTree || !PanelStack)
	{
		return;
	}

	USizeBox* IconSizeBox = Cast<USizeBox>(SelectedItemIconContainer);
	if (!IconSizeBox)
	{
		if (SelectedItemIconContainer)
		{
			SelectedItemIconContainer->RemoveFromParent();
		}
		IconSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SelectedItemIconContainer"));
		SelectedItemIconContainer = IconSizeBox;
	}

	if (!SelectedItemIconImage)
	{
		SelectedItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedItemIconImage"));
		if (SelectedItemIconImage)
		{
			SelectedItemIconImage->SetOpacity(0.0f);
		}
	}
	if (!IconSizeBox || !SelectedItemIconImage)
	{
		return;
	}

	IconSizeBox->SetWidthOverride(TunaSweeperItemInfoPanel::SelectedItemIconSize);
	IconSizeBox->SetHeightOverride(TunaSweeperItemInfoPanel::SelectedItemIconSize);
	if (SelectedItemIconImage->GetParent() != IconSizeBox)
	{
		SelectedItemIconImage->RemoveFromParent();
		IconSizeBox->SetContent(SelectedItemIconImage);
	}

	IconSizeBox->RemoveFromParent();
	const int32 HeaderIndex = HeaderRow ? PanelStack->GetChildIndex(HeaderRow) : INDEX_NONE;
	UPanelSlot* IconPanelSlot = HeaderIndex != INDEX_NONE
		? PanelStack->InsertChildAt(HeaderIndex + 1, IconSizeBox)
		: PanelStack->AddChild(IconSizeBox);
	if (UVerticalBoxSlot* IconSlot = Cast<UVerticalBoxSlot>(IconPanelSlot))
	{
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Top);
		IconSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 8.0f));
	}

	if (SelectedItemDescriptionText)
	{
		SelectedItemDescriptionText->RemoveFromParent();
		const int32 IconIndex = PanelStack->GetChildIndex(IconSizeBox);
		UPanelSlot* DescriptionPanelSlot = IconIndex != INDEX_NONE
			? PanelStack->InsertChildAt(IconIndex + 1, SelectedItemDescriptionText)
			: PanelStack->AddChild(SelectedItemDescriptionText);
		if (UVerticalBoxSlot* DescriptionSlot = Cast<UVerticalBoxSlot>(DescriptionPanelSlot))
		{
			DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
			DescriptionSlot->SetVerticalAlignment(VAlign_Top);
			DescriptionSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}
	}

	if (!SelectedItemFormulaText)
	{
		SelectedItemFormulaText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("SelectedItemFormulaText"));
		if (SelectedItemFormulaText)
		{
			SelectedItemFormulaText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	if (SelectedItemFormulaText)
	{
		SelectedItemFormulaText->RemoveFromParent();
		const int32 DescriptionIndex = SelectedItemDescriptionText
			? PanelStack->GetChildIndex(SelectedItemDescriptionText)
			: INDEX_NONE;
		const int32 IconIndex = PanelStack->GetChildIndex(IconSizeBox);
		UPanelSlot* FormulaPanelSlot = DescriptionIndex != INDEX_NONE
			? PanelStack->InsertChildAt(DescriptionIndex + 1, SelectedItemFormulaText)
			: (IconIndex != INDEX_NONE
				? PanelStack->InsertChildAt(IconIndex + 1, SelectedItemFormulaText)
				: PanelStack->AddChild(SelectedItemFormulaText));
		if (UVerticalBoxSlot* FormulaSlot = Cast<UVerticalBoxSlot>(FormulaPanelSlot))
		{
			FormulaSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			FormulaSlot->SetHorizontalAlignment(HAlign_Fill);
			FormulaSlot->SetVerticalAlignment(VAlign_Top);
			FormulaSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 6.0f));
		}

		SelectedItemFormulaText->SetAutoWrapText(true);
		SelectedItemFormulaText->SetJustification(ETextJustify::Left);
		TunaSweeperUIFont::ApplyFont(SelectedItemFormulaText, 14.0f, ETunaSweeperUIFontWeight::Bold);
	}

	if (SelectedItemDetailRow)
	{
		SelectedItemDetailRow->RemoveFromParent();
	}

	SelectedItemIconContainer = IconSizeBox;
}

void UTunaSweeperHudItemInfoPanelWidget::SetSelectedItemThumbnail(UTexture2D* IconTexture)
{
	EnsureThumbnailWidgets();
	if (!SelectedItemIconImage)
	{
		return;
	}

	if (IconTexture)
	{
		SelectedItemIconImage->SetBrushFromTexture(IconTexture, true);
		SelectedItemIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		SelectedItemIconImage->SetOpacity(1.0f);
	}
	else
	{
		ClearSelectedItemThumbnail();
	}
}

void UTunaSweeperHudItemInfoPanelWidget::ClearSelectedItemThumbnail()
{
	if (SelectedItemIconImage)
	{
		SelectedItemIconImage->SetBrushFromTexture(nullptr, false);
		SelectedItemIconImage->SetOpacity(0.0f);
	}
}

void UTunaSweeperHudItemInfoPanelWidget::SetSelectedItemFormulaInfo(
	const FText& FormulaText,
	const FLinearColor& TextColor,
	bool bVisible)
{
	EnsureThumbnailWidgets();
	if (!SelectedItemFormulaText)
	{
		return;
	}

	SelectedItemFormulaText->SetText(FormulaText);
	SelectedItemFormulaText->SetColorAndOpacity(FSlateColor(TextColor));
	SelectedItemFormulaText->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UTunaSweeperHudItemInfoPanelWidget::SetModdingPanelVisible(bool bVisible)
{
	if (ModdingPanel)
	{
		ModdingPanel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

bool UTunaSweeperHudItemInfoPanelWidget::TryResolveAttachmentDropSlotFromCursor(
	const FVector2D& ScreenSpacePosition,
	FTunaSweeperItemSlotReference& OutSlotReference) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const int32 SlotCount = TunaGameInstance ? TunaGameInstance->GetSelectedWeaponAttachmentSlotTags().Num() : 0;
	return TunaSweeperItemInfoPanel::TryResolveSlotFromTileView(
		AttachmentSlotTileView,
		SlotCount,
		ScreenSpacePosition,
		OutSlotReference);
}

void UTunaSweeperHudItemInfoPanelWidget::HandleCloseButtonClicked()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->ClearSelectedItemSelection();
	}
}
