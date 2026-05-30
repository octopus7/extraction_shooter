#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/TunaSweeperHudTypes.h"
#include "TunaSweeperHudTopReserveWidget.generated.h"

class UButton;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTunaSweeperHudModeSelectedSignature, ETunaSweeperHudMode, SelectedMode);

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHudTopReserveWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "TunaSweeper|HUD")
	FTunaSweeperHudModeSelectedSignature OnHudModeSelected;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetActiveMode(ETunaSweeperHudMode InActiveMode);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UButton> InventoryModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuestModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UButton> MapModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UButton> MemoModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> InventoryModeIcon;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> QuestModeIcon;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> MapModeIcon;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UImage> MemoModeIcon;

private:
	void RefreshTabVisuals();
	void CacheNamedWidgets();
	UImage* EnsureTabIcon(
		ETunaSweeperHudMode Mode,
		UButton* Button,
		TObjectPtr<UImage>& Icon,
		const TCHAR* IconWidgetName);
	void SetTabVisual(
		ETunaSweeperHudMode Mode,
		UButton* Button,
		TObjectPtr<UImage>& Icon,
		const TCHAR* IconWidgetName);

	UFUNCTION()
	void HandleInventoryModeClicked();

	UFUNCTION()
	void HandleQuestModeClicked();

	UFUNCTION()
	void HandleMapModeClicked();

	UFUNCTION()
	void HandleMemoModeClicked();

	UPROPERTY(Transient)
	ETunaSweeperHudMode ActiveMode = ETunaSweeperHudMode::None;
};
