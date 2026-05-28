#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperHudQuickSlotBarWidget.generated.h"

class UImage;
class UProgressBar;
class UTextBlock;
class UTexture2D;
class UWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudQuickSlotBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetQuickSlotIcon(int32 SlotNumber, UTexture2D* IconTexture);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ClearQuickSlotIcon(int32 SlotNumber);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetMeleeQuickSlotIcon(UTexture2D* IconTexture);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ClearMeleeQuickSlotIcon();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetSelectedQuickSlot(int32 SlotNumber);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetSelectedMeleeQuickSlot();

	void SetWeaponAmmoTypeText(int32 SlotNumber, const FText& AmmoTypeText, bool bVisible);
	void SetWeaponAmmoText(int32 SlotNumber, int32 LoadedAmmoCount, int32 InventoryAmmoCount, bool bVisible);
	void SetReloadProgress(float Progress, bool bVisible);
	void SetAmmoSelectorOptions(const TArray<FText>& OptionTexts, int32 FocusedOptionIndex, int32 WeaponSlotNumber, bool bVisible);
	void SetAmmoSelectorPrompt(int32 WeaponSlotNumber, const FText& PromptText, bool bVisible);

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;

private:
	void CacheNamedWidgets();
	int32 GetSlotIndex(int32 SlotNumber) const;
	float GetWeaponSlotCenterOffsetX(int32 WeaponSlotNumber) const;
	void SetAmmoSelectorPanelPosition(int32 WeaponSlotNumber);
	void SetAmmoSelectorPromptVisible(const FText& PromptText, bool bVisible);
	void SetAmmoSelectorKeyHintVisible(bool bVisible);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> SlotIconImages;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> SlotSelectionFrames;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotAmmoTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> SlotAmmoTextContainers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotAmmoTypeTexts;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> SlotAmmoTypeContainers;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> SlotAmmoKeyBackgrounds;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> SlotAmmoKeyTexts;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UImage> QuickSlotMeleeIcon;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> QuickSlotMeleeSelectionFrame;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> AmmoSelectorOptionBackgrounds;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTextBlock>> AmmoSelectorOptionTexts;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> AmmoSelectorPromptBackground;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> AmmoSelectorPromptText;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> AmmoSelectorKeyBackground;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> AmmoSelectorKeyText;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> AmmoSelectorPanel;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> ReloadProgressPanel;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UProgressBar> ReloadProgressBar;
};
