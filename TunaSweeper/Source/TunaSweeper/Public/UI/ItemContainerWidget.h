#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "ItemContainerWidget.generated.h"

class USizeBox;
class UTextBlock;
class UTileView;
class UButton;
class UDragDropOperation;
class UHorizontalBox;
class UImage;
class UTunaSweeperGameInstance;
class UTunaSweeperCurrencyDisplayWidget;
struct FTunaSweeperItemSlotReference;

UENUM(BlueprintType)
enum class ETunaSweeperStorageFilter : uint8
{
	All UMETA(DisplayName = "All"),
	Weapon UMETA(DisplayName = "Weapon"),
	Ammo UMETA(DisplayName = "Ammo"),
	Attachment UMETA(DisplayName = "Attachment"),
	Consumable UMETA(DisplayName = "Consumable"),
	Gear UMETA(DisplayName = "Gear"),
	Material UMETA(DisplayName = "Material"),
	Blueprint UMETA(DisplayName = "Blueprint"),
	Other UMETA(DisplayName = "Other")
};

UCLASS(Abstract, BlueprintType, Blueprintable)
class TUNASWEEPER_API UItemContainerWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, Category = "Item Container", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(BlueprintReadOnly, Category = "Item Container", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContainerTitleText;

	UPROPERTY(BlueprintReadOnly, Category = "Item Container", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ContainerOccupancyText;

	UPROPERTY(BlueprintReadOnly, Category = "Item Container", meta = (BindWidgetOptional))
	TObjectPtr<UTileView> ContainerTileView;

	void SetContainerInstanceInternal(const FTunaSweeperLootContainerInstance& InContainerInstance);
	void SetStorageViewInternal();
	void SetShopViewInternal(int32 ShopId);
	void SetWorkbenchViewInternal(
		int32 WorkbenchId,
		ETunaSweeperWorkbenchMode WorkbenchMode = ETunaSweeperWorkbenchMode::Craft);

	virtual void PopulateContainerItems();
	virtual void RefreshHeaderControls();
	virtual void RebuildCompactedSlotView(
		const TArray<FTunaSweeperInventorySlot>* Slots,
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem);
	virtual bool UsesCompactedSlotView() const;
	virtual int32 GetCompactedSlotViewCount() const;
	virtual int32 ResolveSourceSlotIndexFromDisplayIndex(int32 DisplaySlotIndex) const;
	virtual int32 ResolveCompactedOccupiedSlotCount(int32 DefaultOccupiedSlotCount) const;
	virtual int32 ResolveCompactedOccupancyDenominator(int32 Capacity, int32 TotalOccupiedSlotCount) const;
	virtual bool TryResolveCompactedDropSlotFromCursor(
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference);
	virtual void ApplyContainerPanelSize(int32 RowCount, float EntryHeight);

	bool TryResolveDropSlotFromCursor(
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference);

	UPROPERTY(Transient)
	FTunaSweeperLootContainerInstance ContainerInstance;

	UPROPERTY(Transient)
	ETunaSweeperItemSlotSource SlotSource = ETunaSweeperItemSlotSource::LootContainer;

	UPROPERTY(Transient)
	int32 ActiveShopId = INDEX_NONE;

	UPROPERTY(Transient)
	int32 ActiveWorkbenchId = INDEX_NONE;

	UPROPERTY(Transient)
	ETunaSweeperWorkbenchMode ActiveWorkbenchMode = ETunaSweeperWorkbenchMode::Craft;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> TileObjects;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ULootContainerWidget : public UItemContainerWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Loot Container")
	void SetContainerInstance(const FTunaSweeperLootContainerInstance& InContainerInstance);

	// Compatibility fallback for older HUD blueprints that still route every external container through this widget.
	void SetStorageView();
	void SetShopView(int32 ShopId);
	void SetWorkbenchView(
		int32 WorkbenchId,
		ETunaSweeperWorkbenchMode WorkbenchMode = ETunaSweeperWorkbenchMode::Craft);
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UStorageContainerWidget : public UItemContainerWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Storage")
	void SetStorageView();

protected:
	virtual void RefreshHeaderControls() override;
	virtual void RebuildCompactedSlotView(
		const TArray<FTunaSweeperInventorySlot>* Slots,
		UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem) override;
	virtual bool UsesCompactedSlotView() const override;
	virtual int32 GetCompactedSlotViewCount() const override;
	virtual int32 ResolveSourceSlotIndexFromDisplayIndex(int32 DisplaySlotIndex) const override;
	virtual int32 ResolveCompactedOccupiedSlotCount(int32 DefaultOccupiedSlotCount) const override;
	virtual int32 ResolveCompactedOccupancyDenominator(int32 Capacity, int32 TotalOccupiedSlotCount) const override;
	virtual bool TryResolveCompactedDropSlotFromCursor(
		const FVector2D& ScreenSpacePosition,
		FTunaSweeperItemSlotReference& OutSlotReference) override;
	virtual void ApplyContainerPanelSize(int32 RowCount, float EntryHeight) override;

private:
	void EnsureStorageSortButton();
	void RefreshStorageSortButton();
	void EnsureStorageFilterControls();
	void RefreshStorageFilterControls();
	void SetStorageFilter(ETunaSweeperStorageFilter NewFilter);
	void AddStorageFilterButton(ETunaSweeperStorageFilter Filter);
	FText ResolveStorageFilterText(ETunaSweeperStorageFilter Filter) const;

	UFUNCTION()
	void HandleStorageSortButtonClicked();

	UFUNCTION()
	void HandleStorageFilterAllClicked();

	UFUNCTION()
	void HandleStorageFilterWeaponClicked();

	UFUNCTION()
	void HandleStorageFilterAmmoClicked();

	UFUNCTION()
	void HandleStorageFilterAttachmentClicked();

	UFUNCTION()
	void HandleStorageFilterConsumableClicked();

	UFUNCTION()
	void HandleStorageFilterGearClicked();

	UFUNCTION()
	void HandleStorageFilterMaterialClicked();

	UFUNCTION()
	void HandleStorageFilterBlueprintClicked();

	UFUNCTION()
	void HandleStorageFilterOtherClicked();

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> StorageFilterTabsRow;

	UPROPERTY(Transient)
	TObjectPtr<UButton> StorageSortButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StorageSortButtonText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UButton>> StorageFilterButtons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> StorageFilterButtonImages;

	UPROPERTY(Transient)
	TArray<ETunaSweeperStorageFilter> StorageFilterButtonValues;

	UPROPERTY(Transient)
	TArray<int32> VisibleStorageSlotIndices;

	UPROPERTY(Transient)
	ETunaSweeperStorageFilter ActiveStorageFilter = ETunaSweeperStorageFilter::All;

	UPROPERTY(Transient)
	float StorageStretchedPanelMinHeight = 0.0f;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UShopContainerWidget : public UItemContainerWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Shop")
	void SetShopView(int32 ShopId);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void RefreshHeaderControls() override;
	virtual void ApplyContainerPanelSize(int32 RowCount, float EntryHeight) override;

private:
	UPROPERTY(BlueprintReadOnly, Category = "Shop", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UButton> ShopRefreshStockButton;

	UPROPERTY(BlueprintReadOnly, Category = "Shop", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> ShopRefreshStockButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Shop", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperCurrencyDisplayWidget> ShopCurrencyDisplayWidget;

	void EnsureShopCurrencyDisplayWidget();

	UFUNCTION()
	void HandleShopRefreshStockButtonClicked();
};
