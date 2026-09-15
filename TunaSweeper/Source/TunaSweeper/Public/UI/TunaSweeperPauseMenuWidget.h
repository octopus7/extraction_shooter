#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperPauseMenuWidget.generated.h"

class UBorder;
class UButton;
class UTextBlock;
class UVerticalBox;
class UTunaSweeperIntroMenuWidget;

UCLASS()
class TUNASWEEPER_API UTunaSweeperPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void InitializePauseMenu(bool bRaid);
	void ShowExitFailure();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildWidgetTree();
	void RefreshTexts();
	FText Text(FName Key) const;
	UTextBlock* AddText(UVerticalBox* Parent, FName Name, int32 Size);
	UButton* AddButton(UVerticalBox* Parent, FName Name, UTextBlock*& Label);
	void ShowConfirmation(bool bQuit);
	void HandleSettingsClosed();
	UFUNCTION() void HandleResume();
	UFUNCTION() void HandleSettings();
	UFUNCTION() void HandleTitle();
	UFUNCTION() void HandleQuit();
	UFUNCTION() void HandleConfirm();
	UFUNCTION() void HandleCancel();

	UPROPERTY(Transient) TObjectPtr<UBorder> MenuPanel;
	UPROPERTY(Transient) TObjectPtr<UBorder> ConfirmationPanel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeadingText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ResumeText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SettingsText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> QuitText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmationHeading;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WarningText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ConfirmText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CancelText;
	UPROPERTY(Transient) TObjectPtr<UButton> ResumeButton;
	UPROPERTY(Transient) TObjectPtr<UButton> CancelButton;
	UPROPERTY(Transient) TArray<TObjectPtr<UButton>> MenuButtons;
	UPROPERTY(Transient) TObjectPtr<UTunaSweeperIntroMenuWidget> SettingsWidget;
	bool bRaidContext = false;
	bool bQuitRequested = false;
	bool bConfirmationOpen = false;
	bool bExitFailed = false;
};
