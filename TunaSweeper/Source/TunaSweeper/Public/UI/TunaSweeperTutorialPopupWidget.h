#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperTutorialPopupWidget.generated.h"

/** Presentation-only base. The entire layout is authored and saved in the WBP asset. */
UCLASS()
class TUNASWEEPER_API UTunaSweeperTutorialPopupWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	/** Designer widget name -> existing UITextStrings.csv key. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial|Localization")
	TMap<FName, FName> LocalizedTextKeys;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Tutorial|Localization")
	ETunaSweeperItemTextLanguage PreviewLanguage = ETunaSweeperItemTextLanguage::Korean;

	UFUNCTION(BlueprintCallable, Category="Tutorial|Localization")
	void RefreshLocalizedText();

protected:
	virtual void NativePreConstruct() override;
};
