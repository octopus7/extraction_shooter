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
	void RefreshFromSettings();
	void DiscardPendingChanges();
	bool HasPendingChanges() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

#define TUNA_GRAPHICS_BUTTON(Name) \
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional)) TObjectPtr<UButton> Name; \
	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> Name##Text

	UPROPERTY(BlueprintReadOnly, Category="Graphics", meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> GraphicsStatusText;

	TUNA_GRAPHICS_BUTTON(PresetAutoButton);
	TUNA_GRAPHICS_BUTTON(PresetLowButton);
	TUNA_GRAPHICS_BUTTON(PresetMediumButton);
	TUNA_GRAPHICS_BUTTON(PresetHighButton);
	TUNA_GRAPHICS_BUTTON(PresetEpicButton);

	TUNA_GRAPHICS_BUTTON(WindowedModeButton);
	TUNA_GRAPHICS_BUTTON(BorderlessWindowModeButton);
	TUNA_GRAPHICS_BUTTON(FullscreenModeButton);

	TUNA_GRAPHICS_BUTTON(Resolution1280Button);
	TUNA_GRAPHICS_BUTTON(Resolution1600Button);
	TUNA_GRAPHICS_BUTTON(Resolution1920Button);
	TUNA_GRAPHICS_BUTTON(Resolution2560Button);
	TUNA_GRAPHICS_BUTTON(Resolution3840Button);

	TUNA_GRAPHICS_BUTTON(DLSSOffButton);
	TUNA_GRAPHICS_BUTTON(DLSSQualityButton);
	TUNA_GRAPHICS_BUTTON(DLSSBalancedButton);
	TUNA_GRAPHICS_BUTTON(DLSSPerformanceButton);

	TUNA_GRAPHICS_BUTTON(VSyncToggleButton);
	TUNA_GRAPHICS_BUTTON(FrameRateUnlimitedButton);
	TUNA_GRAPHICS_BUTTON(FrameRate60Button);
	TUNA_GRAPHICS_BUTTON(FrameRate120Button);
	TUNA_GRAPHICS_BUTTON(FrameRate144Button);
	TUNA_GRAPHICS_BUTTON(MotionBlurToggleButton);
	TUNA_GRAPHICS_BUTTON(DynamicResolutionToggleButton);
	TUNA_GRAPHICS_BUTTON(HardwareRayTracingToggleButton);

	TUNA_GRAPHICS_BUTTON(ApplyGraphicsSettingsButton);
	TUNA_GRAPHICS_BUTTON(CancelGraphicsSettingsButton);
	TUNA_GRAPHICS_BUTTON(ConfirmResolutionButton);
	TUNA_GRAPHICS_BUTTON(RevertResolutionButton);

#undef TUNA_GRAPHICS_BUTTON

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

#define TUNA_GRAPHICS_HANDLER(Name) UFUNCTION() void Name()
	TUNA_GRAPHICS_HANDLER(HandlePresetAutoClicked);
	TUNA_GRAPHICS_HANDLER(HandlePresetLowClicked);
	TUNA_GRAPHICS_HANDLER(HandlePresetMediumClicked);
	TUNA_GRAPHICS_HANDLER(HandlePresetHighClicked);
	TUNA_GRAPHICS_HANDLER(HandlePresetEpicClicked);
	TUNA_GRAPHICS_HANDLER(HandleWindowedModeClicked);
	TUNA_GRAPHICS_HANDLER(HandleBorderlessWindowModeClicked);
	TUNA_GRAPHICS_HANDLER(HandleFullscreenModeClicked);
	TUNA_GRAPHICS_HANDLER(HandleResolution1280Clicked);
	TUNA_GRAPHICS_HANDLER(HandleResolution1600Clicked);
	TUNA_GRAPHICS_HANDLER(HandleResolution1920Clicked);
	TUNA_GRAPHICS_HANDLER(HandleResolution2560Clicked);
	TUNA_GRAPHICS_HANDLER(HandleResolution3840Clicked);
	TUNA_GRAPHICS_HANDLER(HandleDLSSOffClicked);
	TUNA_GRAPHICS_HANDLER(HandleDLSSQualityClicked);
	TUNA_GRAPHICS_HANDLER(HandleDLSSBalancedClicked);
	TUNA_GRAPHICS_HANDLER(HandleDLSSPerformanceClicked);
	TUNA_GRAPHICS_HANDLER(HandleVSyncToggleClicked);
	TUNA_GRAPHICS_HANDLER(HandleFrameRateUnlimitedClicked);
	TUNA_GRAPHICS_HANDLER(HandleFrameRate60Clicked);
	TUNA_GRAPHICS_HANDLER(HandleFrameRate120Clicked);
	TUNA_GRAPHICS_HANDLER(HandleFrameRate144Clicked);
	TUNA_GRAPHICS_HANDLER(HandleMotionBlurToggleClicked);
	TUNA_GRAPHICS_HANDLER(HandleDynamicResolutionToggleClicked);
	TUNA_GRAPHICS_HANDLER(HandleHardwareRayTracingToggleClicked);
	TUNA_GRAPHICS_HANDLER(HandleApplyClicked);
	TUNA_GRAPHICS_HANDLER(HandleCancelClicked);
	TUNA_GRAPHICS_HANDLER(HandleConfirmResolutionClicked);
	TUNA_GRAPHICS_HANDLER(HandleRevertResolutionClicked);
#undef TUNA_GRAPHICS_HANDLER

	FTunaSweeperGraphicsSettingsState AppliedState;
	FTunaSweeperGraphicsSettingsState PendingState;
	float ResolutionConfirmationSecondsRemaining = 0.0f;
	bool bHasSettingsSnapshot = false;
	bool bResolutionConfirmationActive = false;
};
