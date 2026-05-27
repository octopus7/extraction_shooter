#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "UI/TunaSweeperHudTypes.h"
#include "TunaSweeperHudTopReserveWidget.generated.h"

class UButton;
class UTextBlock;

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
	TObjectPtr<UTextBlock> InventoryModeText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestModeText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MapModeText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MemoModeText;

private:
	void RefreshTabVisuals();
	void SetTabVisual(ETunaSweeperHudMode Mode, UButton* Button, UTextBlock* TextBlock);

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
