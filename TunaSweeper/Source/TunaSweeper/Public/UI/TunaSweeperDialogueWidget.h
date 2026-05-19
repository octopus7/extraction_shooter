#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperDialogueWidget.generated.h"

class UBorder;
class UTextBlock;

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperDialogueLine
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Dialogue")
	FText SpeakerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Dialogue")
	FText DialogueText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Dialogue")
	bool bUseCameraFocus = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Dialogue", meta = (EditCondition = "bUseCameraFocus"))
	FVector CameraFocusLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Dialogue", meta = (EditCondition = "bUseCameraFocus", ClampMin = "0.0", UIMin = "0.0"))
	float CameraBlendSeconds = 0.75f;
};

DECLARE_DELEGATE_OneParam(FTunaSweeperDialogueLineActivatedDelegate, const FTunaSweeperDialogueLine&);
DECLARE_DELEGATE(FTunaSweeperDialogueFinishedDelegate);

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperDialogueWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void StartDialogue(const TArray<FTunaSweeperDialogueLine>& InDialogueLines, float InCharactersPerSecond);
	void SetLineActivatedDelegate(FTunaSweeperDialogueLineActivatedDelegate InDelegate);
	void SetFinishedDelegate(FTunaSweeperDialogueFinishedDelegate InDelegate);
	bool IsDialogueRunning() const { return bDialogueRunning; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	void BuildDialogueWidget();
	void BeginCurrentLine();
	void UpdateVisibleDialogueText();
	void AdvanceOrFillLine();
	void FinishDialogue();
	bool IsCurrentLineFullyVisible() const;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DialoguePanel;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SpeakerNameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DialogueBodyText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ContinueMarkerText;

	TArray<FTunaSweeperDialogueLine> DialogueLines;
	FTunaSweeperDialogueLineActivatedDelegate LineActivatedDelegate;
	FTunaSweeperDialogueFinishedDelegate FinishedDelegate;
	FString CurrentFullText;
	float CharactersPerSecond = 5.0f;
	float TypewriterAccumulator = 0.0f;
	int32 CurrentLineIndex = INDEX_NONE;
	int32 VisibleCharacterCount = 0;
	bool bDialogueRunning = false;
};
