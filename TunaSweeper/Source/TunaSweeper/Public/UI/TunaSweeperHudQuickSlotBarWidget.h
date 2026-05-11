#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperHudQuickSlotBarWidget.generated.h"

class UImage;
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
	void SetSelectedQuickSlot(int32 SlotNumber);

protected:
	virtual void NativeConstruct() override;
	virtual void NativePreConstruct() override;

private:
	void CacheNamedWidgets();
	int32 GetSlotIndex(int32 SlotNumber) const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> SlotIconImages;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> SlotSelectionFrames;
};

