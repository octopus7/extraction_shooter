#pragma once
// Authored by gpt-6-astra. Reasoning effort is not exposed in this session.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TeleportConfirmationGPT6Astra.generated.h"

class UButton;
class UTextBlock;
DECLARE_DELEGATE_OneParam(FTeleportConfirmationResolved, bool);
DECLARE_DELEGATE_RetVal(bool, FTeleportConfirmationStillValid);

UCLASS()
class TUNASWEEPER_API UTeleportConfirmationGPT6Astra : public UUserWidget
{
	GENERATED_BODY()
public:
	FTeleportConfirmationResolved OnResolved;
	FTeleportConfirmationStillValid IsStillValid;
	void Cancel();
	void FocusCancel();
	bool HasAuthoredControls() const;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	friend class FTeleportConfirmationWidgetTest;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event) override;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> MessageText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> YesText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> NoText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> YesButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> NoButton;

private:
	UFUNCTION() void Confirm();
	UFUNCTION() void Decline();
	void Resolve(bool bConfirmed);
	bool bResolved = false;
};
