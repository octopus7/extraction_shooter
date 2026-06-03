#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperToastWidget.generated.h"

class UBorder;
class UTextBlock;

UCLASS()
class TUNASWEEPER_API UTunaSweeperToastWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetToastMessage(const FText& MessageText);
	void SetToastOpacity(float InOpacity);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void EnsureToastLayout();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> ToastPanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ToastText;
};
