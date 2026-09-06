#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Settings/TunaSweeperGameUserSettings.h"
#include "TunaSweeperGraphicsSettingsWidget.generated.h"

class UButton;
class UTextBlock;
class UWidget;
class UTunaSweeperGraphicsQualityRowWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperGraphicsSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	void BuildEditorTemplate() { BuildRuntimeWidgetTree(); }
#endif
	void RefreshFromSettings();
	void DiscardPendingChanges();
	bool HasPendingChanges() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;


	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> GraphicsStatusText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PresetAutoButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PresetAutoButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PresetLowButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PresetLowButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PresetMediumButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PresetMediumButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PresetHighButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PresetHighButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> PresetEpicButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> PresetEpicButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> WindowedModeButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> WindowedModeButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> BorderlessWindowModeButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> BorderlessWindowModeButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> FullscreenModeButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> FullscreenModeButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> Resolution1280Button;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Resolution1280ButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> Resolution1600Button;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Resolution1600ButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> Resolution1920Button;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Resolution1920ButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> Resolution2560Button;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Resolution2560ButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> Resolution3840Button;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> Resolution3840ButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> DLSSOffButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DLSSOffButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> DLSSQualityButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DLSSQualityButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> DLSSBalancedButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DLSSBalancedButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> DLSSPerformanceButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DLSSPerformanceButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> VSyncToggleButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> VSyncToggleButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> FrameRateUnlimitedButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> FrameRateUnlimitedButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> FrameRate60Button;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> FrameRate60ButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> FrameRate120Button;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> FrameRate120ButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> FrameRate144Button;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> FrameRate144ButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> MotionBlurToggleButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> MotionBlurToggleButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> DynamicResolutionToggleButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> DynamicResolutionToggleButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> HardwareRayTracingToggleButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> HardwareRayTracingToggleButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ApplyGraphicsSettingsButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ApplyGraphicsSettingsButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> CancelGraphicsSettingsButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> CancelGraphicsSettingsButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> ConfirmResolutionButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ConfirmResolutionButtonText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UButton> RevertResolutionButton;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> RevertResolutionButtonText;


	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UWidget> ResolutionConfirmationPanel;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> ResolutionConfirmationText;

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> TextureQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> ShadowQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> GlobalIlluminationQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> ReflectionQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> ViewDistanceQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> EffectsQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> PostProcessQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> FoliageQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> ShadingQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> LandscapeQualityRow;
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTunaSweeperGraphicsQualityRowWidget> AntiAliasingQualityRow;

private:
	void BuildRuntimeWidgetTree();
	void BindButtons();
	void ConfigureQualityRows();
	void RefreshVisualState();
	void RefreshQualityRows();
	void SelectPreset(ETunaSweeperGraphicsPreset Preset);
	void SetWindowMode(EWindowMode::Type WindowMode);
	void SetResolution(const FIntPoint& Resolution);
	void SetDLSSMode(ETunaSweeperTitleDLSSMode Mode);
	void SetFrameRateLimit(float FrameRateLimit);
	void ApplyDLSSModeToRuntime(ETunaSweeperTitleDLSSMode Mode) const;
	bool IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode Mode) const;
	FText ResolveUiText(FName StringKey, const FText& FallbackText) const;
	FText BuildPresetText(ETunaSweeperGraphicsPreset Preset) const;
	FText BuildQualityText(int32 Quality) const;
	FText BuildDLSSModeText(ETunaSweeperTitleDLSSMode Mode) const;
	FText BuildToggleText(const FText& Label, bool bEnabled) const;
	void SetChoiceButtonText(UTextBlock* TextBlock, const FText& Label, bool bSelected) const;
	void BeginResolutionConfirmation();
	void ConfirmResolution();
	void RevertResolution();

	void HandleQualityStepRequested(ETunaSweeperScalabilityOption Option, int32 Delta);


	UFUNCTION()
	void HandlePresetAutoClicked();

	UFUNCTION()
	void HandlePresetLowClicked();

	UFUNCTION()
	void HandlePresetMediumClicked();

	UFUNCTION()
	void HandlePresetHighClicked();

	UFUNCTION()
	void HandlePresetEpicClicked();

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
	void HandleVSyncToggleClicked();

	UFUNCTION()
	void HandleFrameRateUnlimitedClicked();

	UFUNCTION()
	void HandleFrameRate60Clicked();

	UFUNCTION()
	void HandleFrameRate120Clicked();

	UFUNCTION()
	void HandleFrameRate144Clicked();

	UFUNCTION()
	void HandleMotionBlurToggleClicked();

	UFUNCTION()
	void HandleDynamicResolutionToggleClicked();

	UFUNCTION()
	void HandleHardwareRayTracingToggleClicked();

	UFUNCTION()
	void HandleApplyClicked();

	UFUNCTION()
	void HandleCancelClicked();

	UFUNCTION()
	void HandleConfirmResolutionClicked();

	UFUNCTION()
	void HandleRevertResolutionClicked();

	FTunaSweeperGraphicsSettingsState AppliedState;
	FTunaSweeperGraphicsSettingsState PendingState;
	float ResolutionConfirmationSecondsRemaining = 0.0f;
	bool bHasSettingsSnapshot = false;
	bool bResolutionConfirmationActive = false;
};
