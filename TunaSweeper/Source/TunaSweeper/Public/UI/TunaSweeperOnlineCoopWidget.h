#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Online/TunaSweeperOnlineCoopTypes.h"
#include "TunaSweeperOnlineCoopWidget.generated.h"
class UButton; class UEditableTextBox; class UTextBlock;
UCLASS()
class TUNASWEEPER_API UTunaSweeperOnlineCoopWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintCallable,Category="TunaSweeper|Online Coop UI") void HostSession();
 UFUNCTION(BlueprintCallable,Category="TunaSweeper|Online Coop UI") void JoinWithEnteredCode();
 UFUNCTION(BlueprintCallable,Category="TunaSweeper|Online Coop UI") void LeaveSession();
 UFUNCTION(BlueprintCallable,Category="TunaSweeper|Online Coop UI") void RefreshLocalizedText();
protected:
 virtual void NativeConstruct() override;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> HostButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> JoinButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UButton> LeaveButton;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UEditableTextBox> InviteCodeInput;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> InviteCodeText;
 UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> StatusText;
private:
 void HandleStateChanged(ETunaSweeperOnlineCoopState State,ETunaSweeperOnlineCoopError Error);
 UFUNCTION()
 void HandleInviteCodeReady(FString InviteCode);
 class UTunaSweeperOnlineCoopSubsystem* GetCoopSubsystem() const;
};



