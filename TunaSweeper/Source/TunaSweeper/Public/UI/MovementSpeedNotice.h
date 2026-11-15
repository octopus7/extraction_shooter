#pragma once
// Authored by gpt-6-astra. Reasoning effort is not exposed in this session.
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MovementSpeedNotice.generated.h"

class UTextBlock;

UCLASS()
class TUNASWEEPER_API UMovementSpeedNotice : public UUserWidget
{
    GENERATED_BODY()
public:
    void UpdateSpeed(float Multiplier);
protected:
    virtual void NativeConstruct() override;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SpeedText;
private:
    float DisplayedMultiplier = 0.0f;
    int32 DisplayedLanguage = INDEX_NONE;
};
