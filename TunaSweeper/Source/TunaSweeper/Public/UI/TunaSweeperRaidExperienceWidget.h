#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Game/TunaSweeperGameInstance.h"
#include "TunaSweeperRaidExperienceWidget.generated.h"

class UBorder;
class UProgressBar;
class USizeBox;
class UTextBlock;
class UVerticalBox;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperRaidExperienceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	FSimpleMulticastDelegate OnAnimationFinished;
	FSimpleMulticastDelegate OnContinueRequested;

	void StartExperiencePresentation(const FTunaSweeperExperienceAnimationState& InAnimationState);
	void SetContinueReady(bool bReady);
	bool IsExperienceAnimationFinished() const { return bAnimationFinished; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

private:
	void BuildWidgetTree();
	void RefreshStaticText();
	void RefreshDisplayedExperience(int64 DisplayExperiencePoints);
	void RefreshStatusText();
	void HandleLanguageChanged();
	bool TryRequestContinue();

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RootBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LevelText;

	UPROPERTY(Transient)
	TObjectPtr<UProgressBar> ExperienceProgressBar;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> ExperienceProgressBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ExperienceText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GainText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	FTunaSweeperExperienceAnimationState AnimationState;
	int64 LastDisplayedExperiencePoints = 0;
	float AnimationElapsedSeconds = 0.0f;
	bool bAnimationFinished = false;
	bool bContinueReady = false;
	bool bContinueRequested = false;
};
