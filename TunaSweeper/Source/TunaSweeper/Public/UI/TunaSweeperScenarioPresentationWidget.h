#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperScenarioPresentationWidget.generated.h"

class UBorder;
class UImage;
class UTextBlock;

enum class ETunaSweeperScenarioPresentationPhase : uint8
{
	FadeIn,
	DisplayLine,
	FadeOut
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperScenarioPresentationWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void BuildPresentationWidget();
	void InitializeMonologueLines();
	void BeginCurrentLine();
	void UpdateMonologueTextPlacement();
	void UpdateMonologueTypewriter(float DeltaTime);
	void UpdateVisibleMonologueText();
	void ResetSystemTextTypewriter();
	void UpdateSystemTextTypewriter(float DeltaTime);
	void AdvanceOrFillLine();
	void AdvanceLine();
	void StartFadeOut();
	void TravelToBunker();
	void SetFadeOverlayOpacity(float Opacity);
	float GetCurrentLineAutoAdvanceSeconds() const;
	bool IsCurrentLineFullyVisible() const;

	UPROPERTY(Transient)
	TObjectPtr<UImage> BackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> VignetteOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> FadeOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> MonologueText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> PromptText;

	TArray<FText> MonologueLines;
	FString SystemTitleFullText;
	FString SystemStatusFullText;
	FString CurrentMonologueFullText;
	float SystemTypewriterElapsedSeconds = 0.0f;
	float MonologueTypewriterAccumulator = 0.0f;
	int32 SystemTitleVisibleCharacters = 0;
	int32 SystemStatusVisibleCharacters = 0;
	int32 MonologueVisibleCharacterCount = 0;
	int32 CurrentLineIndex = 0;
	float PhaseElapsedSeconds = 0.0f;
	ETunaSweeperScenarioPresentationPhase Phase = ETunaSweeperScenarioPresentationPhase::FadeIn;
	bool bTravelStarted = false;
};
