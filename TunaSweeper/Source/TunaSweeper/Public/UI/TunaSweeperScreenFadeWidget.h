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
	void StartFadeToBlack(float DurationSeconds = 1.0f, FSimpleDelegate InFadeFinishedDelegate = FSimpleDelegate());

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	enum class EFadeDirection : uint8
	{
		FromBlack,
		ToBlack
	};

	void BuildFadeWidget();
	void SetFadeOpacity(float Opacity);

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FadeBorder;

	FSimpleDelegate FadeFinishedDelegate;
	EFadeDirection FadeDirection = EFadeDirection::FromBlack;
	float FadeDurationSeconds = 1.0f;
	float FadeElapsedSeconds = 0.0f;
	bool bFadeActive = false;
};
