#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperPracticeDummyHealthBarWidget.generated.h"

class UBorder;
class UProgressBar;
class USizeBox;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperPracticeDummyHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Practice Dummy")
	void SetHealthFraction(float InHealthFraction);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildWidgetTree();
	void RefreshHealthBar();

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> BackgroundBorder;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> HealthProgressBar;

	float HealthFraction = 1.0f;
};
