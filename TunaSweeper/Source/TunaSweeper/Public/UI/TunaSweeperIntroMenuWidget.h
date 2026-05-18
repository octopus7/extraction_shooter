#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperIntroMenuWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperIntroMenuWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	FName StartTargetLevelName = FName(TEXT("BunkerMap"));

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MainMenuPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SaveSlotPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SaveSlotActionRow;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StartButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentSaveSlotText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotSelectButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CreditsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveSlot1Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SaveSlot1Text;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveSlot2Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SaveSlot2Text;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveSlot3Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SaveSlot3Text;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> PrimarySaveSlotButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PrimarySaveSlotButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeleteSaveSlotButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeleteSaveSlotButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UImage> DeleteHoldGaugeFill;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackToMainMenuButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DeleteConfirmPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmDeleteButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelDeleteButton;

private:
	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleSlotSelectClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleCreditsClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleSaveSlot1Focused();

	UFUNCTION()
	void HandleSaveSlot2Focused();

	UFUNCTION()
	void HandleSaveSlot3Focused();

	UFUNCTION()
	void HandlePrimarySaveSlotClicked();

	UFUNCTION()
	void HandleDeleteSaveSlotClicked();

	UFUNCTION()
	void HandleDeleteSaveSlotPressed();

	UFUNCTION()
	void HandleDeleteSaveSlotReleased();

	UFUNCTION()
	void HandleBackToMainMenuClicked();

	UFUNCTION()
	void HandleConfirmDeleteClicked();

	UFUNCTION()
	void HandleCancelDeleteClicked();

	void ShowMainMenu();
	void ShowSaveSlotSelection();
	void SelectSaveSlot(int32 SaveSlotIndex);
	void RefreshMainMenu();
	void RefreshSaveSlotMenu();
	void RefreshSaveSlotButton(int32 SaveSlotIndex, UButton* SlotButton, UTextBlock* SlotText);
	FText BuildCurrentSaveSlotText(int32 SaveSlotIndex) const;
	FText BuildSaveSlotButtonText(int32 SaveSlotIndex) const;
	FString FormatPlayTime(float TotalSeconds) const;
	FString FormatSaveTime(int64 LastSavedAtTicks) const;
	bool IsSaveSlotSelectionVisible() const;
	bool CanDeleteSelectedSaveSlot() const;
	void ResetDeleteHoldProgress();
	void SetDeleteHoldProgress(float Progress);
	void ShowDeleteConfirmDialog();
	void HideDeleteConfirmDialog();

	int32 SelectedSaveSlotIndex = INDEX_NONE;
	float DeleteHoldElapsedSeconds = 0.0f;
	bool bDeleteHoldActive = false;
	bool bDeleteConfirmVisible = false;

	static constexpr float DeleteHoldDurationSeconds = 3.0f;
};
