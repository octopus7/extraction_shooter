#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "TunaSweeperItemStackSplitPopupWidget.generated.h"

class APlayerController;
class UBorder;
class UButton;
class UEditableTextBox;
class UTextBlock;
class UTunaSweeperGameInstance;

UCLASS()
class TUNASWEEPER_API UTunaSweeperItemStackSplitPopupWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	static bool TryOpenStackSplitPopup(
		APlayerController* OwningPlayer,
		UTunaSweeperGameInstance* TunaGameInstance,
		const FTunaSweeperItemSlotReference& SourceSlot,
		const FTunaSweeperItemSlotReference& TargetSlot,
		const FVector2D& ScreenSpacePosition);

	void ConfigureSplit(
		UTunaSweeperGameInstance* InTunaGameInstance,
		const FTunaSweeperItemSlotReference& InSourceSlot,
		const FTunaSweeperItemSlotReference& InTargetSlot,
		int32 InDefaultSplitQuantity,
		int32 InMaxSplitQuantity);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildSplitPopupWidget();
	void RefreshSplitText();
	void HandleLanguageChanged();
	void ConfirmSplit();
	int32 GetRequestedSplitQuantity() const;

	UFUNCTION()
	void HandleOkClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GuideText;

	UPROPERTY(Transient)
	TObjectPtr<UEditableTextBox> QuantityTextBox;

	UPROPERTY(Transient)
	TObjectPtr<UButton> OkButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> OkButtonText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CancelButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CancelButtonText;

	TWeakObjectPtr<UTunaSweeperGameInstance> TunaGameInstance;
	FTunaSweeperItemSlotReference SourceSlot;
	FTunaSweeperItemSlotReference TargetSlot;
	int32 DefaultSplitQuantity = 1;
	int32 MaxSplitQuantity = 1;
};
