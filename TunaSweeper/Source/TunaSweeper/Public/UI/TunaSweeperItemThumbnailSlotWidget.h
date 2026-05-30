#pragma once

#include "Blueprint/IUserObjectListEntry.h"
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "TunaSweeperItemThumbnailSlotWidget.generated.h"

class UBorder;
class UImage;
class USizeBox;
class UTextBlock;
class UDragDropOperation;
class UTunaSweeperItemHoverBorderEffectWidget;
class UTunaSweeperItemHoverPromptWidget;
class UTunaSweeperItemRaritySlotAccentWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperItemThumbnailSlotWidget : public UUserWidget, public IUserObjectListEntry
{
	GENERATED_BODY()

public:
	void SetTileData(const FTunaSweeperItemStackTileData& InTileData);

protected:
	virtual void NativeOnListItemObjectSet(UObject* ListItemObject) override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnDragDetected(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent,
		UDragDropOperation*& OutOperation) override;
	virtual void NativeOnDragEnter(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual void NativeOnDragLeave(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual bool NativeOnDragOver(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;
	virtual bool NativeOnDrop(
		const FGeometry& InGeometry,
		const FDragDropEvent& InDragDropEvent,
		UDragDropOperation* InOperation) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SlotBackground;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> SlotSizeBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> IconBox;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EquipmentSlotNameText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIconImage;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ItemQuantityPlate;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemQuantityText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AttachmentSlotIndicatorText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ItemNamePlate;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ItemPricePlate;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemPriceText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Item Tile", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemPriceCoinImage;

private:
	void CacheLayoutWidgets();
	void ApplySlotMetrics(bool bIsEquipmentSlot);
	void ApplyTileData();
	void ApplyDropHighlight(bool bCanAcceptDrop);
	void EnsureRaritySlotAccentWidget();
	void ApplyRaritySlotAccent();
	void EnsureHoverBorderEffectWidget();
	void EnsureAttachmentSlotIndicatorWidget();
	void EnsureItemPriceCoinWidget();
	FText BuildAttachmentSlotIndicatorText() const;
	bool CanAcceptDragOperation(UDragDropOperation* InOperation) const;
	void UpdateHoveredDropSlot(UDragDropOperation* InOperation, bool bCanAcceptDrop) const;
	FTunaSweeperItemSlotReference GetCachedSlotReference() const;
	bool CanShowHoverBorderEffect() const;
	void SetHoverBorderEffectActive(bool bInActive);
	void DrawHoverBorderEffect(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId) const;
	bool CanShowHoverPrompt() const;
	void ShowHoverPrompt(const FPointerEvent& InMouseEvent);
	void HideHoverPrompt();
	void UpdateHoverPromptPosition(const FPointerEvent& InMouseEvent) const;
	void SetHoveredItemSlot() const;
	void ClearHoveredItemSlot() const;

	UPROPERTY(Transient)
	FTunaSweeperItemStackTileData CachedTileData;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperItemHoverPromptWidget> ActiveHoverPrompt;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperItemRaritySlotAccentWidget> RaritySlotAccentWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperItemHoverBorderEffectWidget> HoverBorderEffectWidget;

	bool bSuppressNextMouseButtonUpSelection = false;
	bool bHoverBorderEffectActive = false;
	float HoverBorderAnimationSeconds = 0.0f;
	float HoverBorderEffectOpacity = 0.0f;
};
