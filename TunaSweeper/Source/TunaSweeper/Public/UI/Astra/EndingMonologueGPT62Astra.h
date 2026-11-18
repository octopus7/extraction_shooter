#pragma once

#include "Blueprint/UserWidget.h"
#include "EndingMonologueGPT62Astra.generated.h"

class UTextBlock;

// Authoring model: gpt-6.2-astra. Reasoning effort: max.
// The centered text and its black background are authored in the Widget Blueprint.
UCLASS(Abstract, Blueprintable)
class TUNASWEEPER_API UEndingMonologueGPT62Astra : public UUserWidget
{
    GENERATED_BODY()
public:
    explicit UEndingMonologueGPT62Astra(const FObjectInitializer& ObjectInitializer);
    void PlayLine(const FText& Line, FSimpleDelegate OnContinue);
    void ShowSaveRetry(const FText& Message, FSimpleDelegate OnRetry);

protected:
    virtual void NativeTick(const FGeometry& Geometry, float DeltaSeconds) override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;

    UPROPERTY(meta=(BindWidget))
    TObjectPtr<UTextBlock> MonologueText;

private:
    void Continue();
    double StartedAt = 0.0;
    bool bWaitingForRetry = false;
    FSimpleDelegate ContinueAction;
};
