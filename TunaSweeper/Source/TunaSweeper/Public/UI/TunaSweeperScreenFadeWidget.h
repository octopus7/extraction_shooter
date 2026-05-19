#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperScreenFadeWidget.generated.h"

class UBorder;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperScreenFadeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void StartFadeFromBlack(float DurationSeconds = 1.0f);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildFadeWidget();
	void SetFadeOpacity(float Opacity);

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FadeBorder;

	float FadeDurationSeconds = 1.0f;
	float FadeElapsedSeconds = 0.0f;
	bool bFadeActive = false;
};
