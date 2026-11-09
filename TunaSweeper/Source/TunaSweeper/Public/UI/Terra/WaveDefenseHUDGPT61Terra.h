#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WaveDefenseHUDGPT61Terra.generated.h"
class AWaveDefenseDirector;
class UTextBlock;
// Model: GPT-6.1 Terra; reasoning effort: high.
UCLASS(Abstract)
class TUNASWEEPER_API UWaveDefenseHUDGPT61Terra : public UUserWidget
{
    GENERATED_BODY()
public:
    void SetDirector(AWaveDefenseDirector* InDirector);
protected:
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DetailText;
private:
    TWeakObjectPtr<AWaveDefenseDirector> Director;
};
