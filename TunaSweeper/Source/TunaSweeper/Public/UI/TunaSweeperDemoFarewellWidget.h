#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperDemoFarewellWidget.generated.h"
class UTexture2D;
UCLASS()
class TUNASWEEPER_API UTunaSweeperDemoFarewellWidget : public UUserWidget
{
    GENERATED_BODY()
    friend class FTunaDemoEndingAssetsTest;
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ending")
    TObjectPtr<UTexture2D> Illustration;
    FSimpleDelegate OnContinue;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnKeyDown(const FGeometry&, const FKeyEvent&) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) override;
    virtual FReply NativeOnMouseWheel(const FGeometry&, const FPointerEvent&) override;
private:
    FReply Continue();
    double AcceptInputAfter = 0;
    bool bLeaving = false;
};
