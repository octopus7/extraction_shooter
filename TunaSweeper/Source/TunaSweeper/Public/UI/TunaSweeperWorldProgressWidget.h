#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperWorldProgressWidget.generated.h"

class ATunaSweeperWorldProgressActor;
class UBorder;
class UButton;
class UTextBlock;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperWorldProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|World Progress")
	void SetProgressActor(ATunaSweeperWorldProgressActor* InProgressActor);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	UFUNCTION()
	void HandleUseItemClicked();

	UFUNCTION()
	void HandleRepairClicked();

	UFUNCTION()
	void HandleCloseClicked();

	void BuildProgressWidget();
	void RefreshView();
	void ClosePanel();

	UPROPERTY(Transient)
	TWeakObjectPtr<ATunaSweeperWorldProgressActor> ActiveProgressActor;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ProgressText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InventoryText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> UseItemButtonText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RepairButtonText;

	UPROPERTY(Transient)
	TObjectPtr<UButton> UseItemButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RepairButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CloseButton;
};
