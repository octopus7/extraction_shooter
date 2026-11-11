#pragma once
// Authored by gpt-6-astra; reasoning effort: medium.
#include "Components/SpinBox.h"
#include "CoreMinimal.h"
#include "IconByteSpinBoxGPT6Astra.generated.h"

/** Native spin-box input with per-pixel contrast across its byte-value fill.
 * The containing controls and layout remain authored in the Widget Blueprint.
 */
UCLASS()
class TUNASWEEPER_API UIconByteSpinBoxGPT6Astra : public USpinBox
{
    GENERATED_BODY()
  public:
    UPROPERTY(EditAnywhere, Category = "Byte appearance")
    FLinearColor FilledBackground = FLinearColor(0.12f, 0.12f, 0.12f);
    UPROPERTY(EditAnywhere, Category = "Byte appearance")
    FLinearColor UnfilledBackground = FLinearColor(0.78f, 0.78f, 0.78f);
    UPROPERTY(EditAnywhere, Category = "Byte appearance")
    FLinearColor FilledForeground = FLinearColor(0.98f, 0.98f, 0.98f);
    UPROPERTY(EditAnywhere, Category = "Byte appearance")
    FLinearColor UnfilledForeground = FLinearColor(0.018f, 0.018f, 0.018f);
#if WITH_DEV_AUTOMATION_TESTS
    void SetEditingTextForAutomation(const FText &Text);
    void CommitEditingTextForAutomation(const FText &Text);
#endif
  protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
};
