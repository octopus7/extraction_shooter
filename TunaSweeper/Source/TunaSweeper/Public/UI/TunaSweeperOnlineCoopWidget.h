#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "Online/TunaSweeperOnlineCoopTypes.h"
#include "TunaSweeperOnlineCoopWidget.generated.h"

class UButton;
class UEditableTextBox;
class UTextBlock;
class UTunaSweeperOnlineCoopSubsystem;

UCLASS(Blueprintable)
class TUNASWEEPER_API UTunaSweeperOnlineCoopWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void SetReturnFocusWidget(UWidget* Widget) { ReturnFocusWidget = Widget; }
	UFUNCTION(BlueprintCallable, Category="TunaSweeper|Online Coop UI") void HostSession();
	UFUNCTION(BlueprintCallable, Category="TunaSweeper|Online Coop UI") void ConnectOnline();
	UFUNCTION(BlueprintCallable, Category="TunaSweeper|Online Coop UI") void JoinWithEnteredCode();
	UFUNCTION(BlueprintCallable, Category="TunaSweeper|Online Coop UI") void LeaveSession();
	UFUNCTION(BlueprintCallable, Category="TunaSweeper|Online Coop UI") void RefreshLocalizedText();
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> ConnectButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> HostButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> JoinButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> LeaveButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> CloseButton;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UEditableTextBox> InviteCodeInput;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> DescriptionText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ConnectButtonText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> HostButtonText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> JoinButtonText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LeaveButtonText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> CloseButtonText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> InviteCodeLabel;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> InviteCodeText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> ErrorText;
private:
	TWeakObjectPtr<UWidget> ReturnFocusWidget;
	// UE 5.7 forwards the supplied style address to Slate; retain it for the widget lifetime.
	UPROPERTY(Transient) FEditableTextBoxStyle InviteInputStyle;
	UFUNCTION() void HandleStateChanged(ETunaSweeperOnlineCoopState State, ETunaSweeperOnlineCoopError Error);
	UFUNCTION() void HandleInviteCodeReady(FString InviteCode);
	UFUNCTION() void ClosePanel();
	UTunaSweeperOnlineCoopSubsystem* GetCoopSubsystem() const;
	void RefreshState();
};
