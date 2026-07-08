#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "GenericPlatform/GenericWindow.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TimerManager.h"
#include "TunaSweeperIntroMenuWidget.generated.h"

class UButton;
class UBorder;
class UHorizontalBox;
class UImage;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UTexture2D;
class UVerticalBox;
class UWidget;
class UTunaSweeperScreenFadeWidget;
class UTunaSweeperTitleWindParticleWidget;

enum class ETunaSweeperTitleDLSSMode : uint8
{
	Off = 0,
	Quality,
	Balanced,
	Performance
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperIntroMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void PrepareForInitialViewport();

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	FName StartTargetLevelName = FName(TEXT("BunkerMap"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float StartTransitionFadeSeconds = 0.8f;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> MainMenuPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SaveSlotPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SaveSlotActionRow;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> StartButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StartButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CurrentSaveSlotText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotSelectButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CreditsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> AlwaysNewStartButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> AlwaysNewStartButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveSlot1Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SaveSlot1Text;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveSlot2Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SaveSlot2Text;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SaveSlot3Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SaveSlot3Text;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> PrimarySaveSlotButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PrimarySaveSlotButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> DeleteSaveSlotButtonBox;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DeleteSaveSlotButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeleteSaveSlotButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UImage> DeleteSaveSlotHoldProgressFill;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UImage> DeleteHoldGaugeFill;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackToMainMenuButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DeleteConfirmPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmDeleteButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelDeleteButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SettingsPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SettingsStatusText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsGraphicsTabButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SettingsInterfaceTabButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> GraphicsSettingsPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> InterfaceSettingsPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> WindowedModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BorderlessWindowModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> FullscreenModeButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> Resolution1280Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> Resolution1600Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> Resolution1920Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> Resolution2560Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> Resolution3840Button;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DLSSOffButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DLSSQualityButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DLSSBalancedButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> DLSSPerformanceButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackFromSettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> LanguageEnglishButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LanguageEnglishButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> LanguageKoreanButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LanguageKoreanButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> LanguageJapaneseButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LanguageJapaneseButtonText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> ConfirmInterfaceSettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> CancelInterfaceSettingsButton;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CreditsPanel;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> CreditsScrollBox;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> CreditsScrollBox2;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> CreditsScrollBox3;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CreditsText;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CreditsText2;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CreditsText3;

	UPROPERTY(BlueprintReadOnly, Category = "Intro", meta = (BindWidgetOptional))
	TObjectPtr<UButton> BackFromCreditsButton;

private:
	UFUNCTION()
	void HandleStartClicked();

	UFUNCTION()
	void HandleSlotSelectClicked();

	UFUNCTION()
	void HandleSettingsClicked();

	UFUNCTION()
	void HandleCreditsClicked();

	UFUNCTION()
	void HandleQuitClicked();

	UFUNCTION()
	void HandleAlwaysNewStartClicked();

	UFUNCTION()
	void HandleDifficultyFarmingClicked();

	UFUNCTION()
	void HandleDifficultyNormalClicked();

	UFUNCTION()
	void HandleDifficultyHardClicked();

	UFUNCTION()
	void HandleDifficultyStartClicked();

	UFUNCTION()
	void HandleDifficultyBackClicked();

	UFUNCTION()
	void HandleSaveSlot1Focused();

	UFUNCTION()
	void HandleSaveSlot2Focused();

	UFUNCTION()
	void HandleSaveSlot3Focused();

	UFUNCTION()
	void HandlePrimarySaveSlotClicked();

	UFUNCTION()
	void HandleDeleteSaveSlotClicked();

	UFUNCTION()
	void HandleDeleteSaveSlotPressed();

	UFUNCTION()
	void HandleDeleteSaveSlotReleased();

	UFUNCTION()
	void HandleBackToMainMenuClicked();

	UFUNCTION()
	void HandleConfirmDeleteClicked();

	UFUNCTION()
	void HandleCancelDeleteClicked();

	UFUNCTION()
	void HandleSettingsGraphicsTabClicked();

	UFUNCTION()
	void HandleSettingsInterfaceTabClicked();

	UFUNCTION()
	void HandleWindowedModeClicked();

	UFUNCTION()
	void HandleBorderlessWindowModeClicked();

	UFUNCTION()
	void HandleFullscreenModeClicked();

	UFUNCTION()
	void HandleResolution1280Clicked();

	UFUNCTION()
	void HandleResolution1600Clicked();

	UFUNCTION()
	void HandleResolution1920Clicked();

	UFUNCTION()
	void HandleResolution2560Clicked();

	UFUNCTION()
	void HandleResolution3840Clicked();

	UFUNCTION()
	void HandleDLSSOffClicked();

	UFUNCTION()
	void HandleDLSSQualityClicked();

	UFUNCTION()
	void HandleDLSSBalancedClicked();

	UFUNCTION()
	void HandleDLSSPerformanceClicked();

	UFUNCTION()
	void HandleBackFromSettingsClicked();

	UFUNCTION()
	void HandleLanguageEnglishClicked();

	UFUNCTION()
	void HandleLanguageKoreanClicked();

	UFUNCTION()
	void HandleLanguageJapaneseClicked();

	UFUNCTION()
	void HandleConfirmInterfaceSettingsClicked();

	UFUNCTION()
	void HandleCancelInterfaceSettingsClicked();

	UFUNCTION()
	void HandleBackFromCreditsClicked();

	void HandleLanguageChanged();

	void ShowMainMenu();
	void ShowDifficultySelection();
	void ShowSaveSlotSelection();
	void ShowSettingsPanel();
	void ShowGraphicsSettingsTab();
	void ShowInterfaceSettingsTab();
	void ShowCreditsPanel();
	void HideOverlayPanels();
	void SetTitleLogoVisible(bool bVisible);
	void SelectSaveSlot(int32 SaveSlotIndex);
	void RefreshMainMenu();
	void RefreshDifficultySelectionPanel();
	void RefreshSaveSlotMenu();
	void EnsureSettingsPanelLayout();
	void RefreshSettingsPanel();
	void RefreshInterfaceSettingsPanel();
	void RefreshSettingsSelectionStyles(const FIntPoint& CurrentResolution, EWindowMode::Type CurrentWindowMode);
	void RefreshInterfaceSelectionStyles();
	void RefreshLocalizedTexts();
	void RefreshSaveSlotButton(int32 SaveSlotIndex, UButton* SlotButton, UTextBlock* SlotText);
	void ApplySaveSlotButtonStyle(UButton* SlotButton, bool bSelected);
	void EnsureSaveSlotSelectionRingWidgets();
	void EnsureSaveSlotSelectionRingContent(
		UButton* SlotButton,
		UTextBlock* SlotText,
		TObjectPtr<UImage>& RingImage,
		const TCHAR* ContentWidgetName,
		const TCHAR* RingWidgetName);
	void ConfigureSaveSlotSelectionRingImage(UImage* RingImage);
	UTexture2D* GetOrCreateSaveSlotSelectionRingTexture();
	void UpdateSaveSlotSelectionRingAnimation(float InDeltaTime);
	void SetSaveSlotSelectionRingSelected(UImage* RingImage, bool bSelected) const;
	void LoadTitleGraphicsSettings();
	void SaveTitleGraphicsSettings() const;
	void ApplyDLSSSetting(ETunaSweeperTitleDLSSMode DLSSMode);
	void ApplyDLSSModeToRuntime(ETunaSweeperTitleDLSSMode DLSSMode) const;
	bool IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode DLSSMode) const;
	void ApplySettingsChoiceButtonStyle(
		UButton* Button,
		const FVector2D& ButtonSize,
		bool bSelected,
		bool bPrimary = false) const;
	void ApplySettingsTabButtonStyle(UButton* Button, const FVector2D& ButtonSize, bool bSelected) const;
	FText BuildWindowModeText(EWindowMode::Type WindowMode) const;
	FText BuildDLSSModeText(ETunaSweeperTitleDLSSMode DLSSMode) const;
	FText BuildLanguageNameText(ETunaSweeperItemTextLanguage Language) const;
	FText BuildLanguageOptionText(ETunaSweeperItemTextLanguage Language, bool bSelected) const;
	FText ResolveUiText(FName StringKey, const FText& FallbackText) const;
	void SetNamedText(FName WidgetName, const FText& Text) const;
	void EnsureDifficultySelectionPanel();
	void SelectDifficultyStage(int32 DifficultyStage);
	void RefreshDifficultyOption(
		int32 DifficultyStage,
		UButton* Button,
		UImage* BackgroundImage,
		UBorder* SelectionBorder,
		UTextBlock* TitleText,
		UTextBlock* DescriptionText);
	void ApplyDifficultyButtonStyle(UButton* Button) const;
	void ConfigureDifficultyCardBackground(UImage* BackgroundImage, bool bSelected);
	void ConfigureDifficultyActionButtonBackground(UImage* BackgroundImage, bool bSelected);
	void ConfigureDifficultySelectionBorder(UBorder* SelectionBorder, bool bSelected);
	void ConfigureDifficultyIcon(UImage* IconImage, int32 DifficultyStage);
	void LoadDifficultyDefinitions();
	FText BuildDifficultyTitleText(int32 DifficultyStage) const;
	FText BuildDifficultyDescriptionText(int32 DifficultyStage) const;
	UTexture2D* LoadDifficultyTexture(TObjectPtr<UTexture2D>& TextureCache, const TCHAR* TexturePath);
	void EnsureAlwaysNewStartButton();
	void SetAlwaysNewStartButtonVisible(bool bVisible);
	FText BuildCurrentSaveSlotText(int32 SaveSlotIndex) const;
	FText BuildSaveSlotButtonText(int32 SaveSlotIndex) const;
	FString BuildCreditsRollText() const;
	FString BuildCreditsColumnText(int32 ColumnIndex) const;
	FString FormatSaveTime(int64 LastSavedAtTicks) const;
	FString FormatPlayTime(float TotalPlaySeconds) const;
	FText BuildSaveSlotDifficultyText(int32 DifficultyStage, bool bDifficultySelected = true) const;
	bool IsSaveSlotSelectionVisible() const;
	bool IsDifficultySelectionVisible() const;
	bool IsCreditsPanelVisible() const;
	bool CanDeleteSelectedSaveSlot() const;
	void ApplyDisplaySettings(EWindowMode::Type WindowMode);
	void ApplyResolutionSetting(const FIntPoint& Resolution);
	void ExecuteSelectedSaveSlotDelete();
	void ResetDeleteHoldProgress();
	void SetDeleteHoldProgress(float Progress);
	void ShowDeleteConfirmDialog();
	void HideDeleteConfirmDialog();
	void HideLegacyDeleteHoldGaugeWidgets();
	void EnsureDeleteSaveSlotHoldProgressWidget();
	void ConfigureDeleteSaveSlotHoldProgressFill();
	void ResetTitleViewportLayoutState();
	void EnsureTitleWindParticleOverlay();
	void ApplyTitleMenuButtonContentLayout();
	UWidget* BuildTitleMenuButtonContent(
		const FText& Icon,
		UTextBlock* LabelText,
		const FText& Label,
		int32 LabelFontSize,
		int32 IconFontSize);
	void BeginTravelToLevel(FName TargetLevelName);
	void BeginStartTravel(bool bAlwaysNewStart);
	void ReloadIntroLevel();
	void OpenPendingStartTargetLevel();
	void SetStartTravelControlsEnabled(bool bEnabled);

	struct FDifficultyOptionText
	{
		int32 DifficultyStage = 1;
		FText Title;
		FText Description;
	};

	int32 SelectedSaveSlotIndex = INDEX_NONE;
	int32 SelectedDifficultyStage = INDEX_NONE;
	float DeleteHoldElapsedSeconds = 0.0f;
	float CreditsScrollOffset = 0.0f;
	float SaveSlotSelectionRingAngle = 0.0f;
	bool bDeleteHoldActive = false;
	bool bDeleteConfirmVisible = false;
	bool bStartTravelPending = false;
	bool bTitleMenuButtonContentLayoutApplied = false;
	bool bShowingInterfaceSettingsTab = false;
	bool bDifficultyDefinitionsLoaded = false;
	ETunaSweeperTitleDLSSMode PreferredDLSSMode = ETunaSweeperTitleDLSSMode::Performance;
	ETunaSweeperItemTextLanguage PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::English;
	FName PendingStartTargetLevelName = NAME_None;
	FTimerHandle StartTravelTimerHandle;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> AlwaysNewStartButtonContainer;

	UPROPERTY(Transient)
	TObjectPtr<UWidget> DifficultySelectPanel;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DifficultyBackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DifficultyFarmingButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DifficultyNormalButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DifficultyHardButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DifficultyStartButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DifficultyBackButton;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DifficultyFarmingBackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DifficultyNormalBackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DifficultyHardBackgroundImage;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DifficultyFarmingSelectionBorder;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DifficultyNormalSelectionBorder;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> DifficultyHardSelectionBorder;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DifficultyFarmingIconImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DifficultyNormalIconImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> DifficultyHardIconImage;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyFarmingTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyNormalTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyHardTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyFarmingDescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyNormalDescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyHardDescriptionText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyStartButtonText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DifficultyBackButtonText;

	UPROPERTY(Transient)
	TObjectPtr<UImage> GeneratedSaveSlot1SelectionRingImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> GeneratedSaveSlot2SelectionRingImage;

	UPROPERTY(Transient)
	TObjectPtr<UImage> GeneratedSaveSlot3SelectionRingImage;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> SaveSlotSelectionRingTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperScreenFadeWidget> StartTravelFadeWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperTitleWindParticleWidget> TitleWindParticleOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> DifficultyBackgroundTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> DifficultyCardFrameTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> DifficultyActionButtonTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> DifficultyFarmingIconTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> DifficultyNormalIconTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> DifficultyHardIconTexture;

	TArray<FDifficultyOptionText> DifficultyOptionTexts;

	static constexpr float DeleteHoldDurationSeconds = 3.0f;
	static constexpr float CreditsScrollSpeed = 34.0f;
};
