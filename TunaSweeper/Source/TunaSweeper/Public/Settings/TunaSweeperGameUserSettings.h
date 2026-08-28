#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"
#include "Settings/TunaSweeperGraphicsSettingsTypes.h"
#include "TunaSweeperGameUserSettings.generated.h"

struct FTunaSweeperGraphicsSettingsState
{
	Scalability::FQualityLevels QualityLevels;
	FIntPoint Resolution = FIntPoint::ZeroValue;
	EWindowMode::Type WindowMode = EWindowMode::Windowed;
	ETunaSweeperGraphicsPreset Preset = ETunaSweeperGraphicsPreset::Custom;
	ETunaSweeperTitleDLSSMode DLSSMode = ETunaSweeperTitleDLSSMode::Off;
	float FrameRateLimit = 0.0f;
	bool bVSyncEnabled = false;
	bool bDynamicResolutionEnabled = false;
	bool bMotionBlurEnabled = true;
	bool bHardwareRayTracingEnabled = true;
};

UCLASS(Config=GameUserSettings, BlueprintType)
class TUNASWEEPER_API UTunaSweeperGameUserSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	static UTunaSweeperGameUserSettings* Get();

	virtual void SetToDefaults() override;
	virtual void LoadSettings(bool bForceReload = false) override;

	void InitializeForCurrentHardware();
	ETunaSweeperGraphicsPreset RefreshAutoDetection();
	void ApplyCustomGraphicsOverrides() const;

	FTunaSweeperGraphicsSettingsState CaptureGraphicsState() const;
	void ApplyGraphicsState(const FTunaSweeperGraphicsSettingsState& State, bool bSaveSettings);

	ETunaSweeperGraphicsPreset GetSelectedGraphicsPreset() const { return SelectedGraphicsPreset; }
	ETunaSweeperGraphicsPreset GetResolvedAutoGraphicsPreset() const { return ResolvedAutoGraphicsPreset; }
	ETunaSweeperTitleDLSSMode GetPreferredDLSSMode() const { return PreferredDLSSMode; }
	int32 GetLastDetectedDedicatedVideoMemoryMB() const { return LastDetectedDedicatedVideoMemoryMB; }
	bool IsMotionBlurEnabled() const { return bMotionBlurEnabled; }
	bool IsHardwareRayTracingEnabled() const { return bHardwareRayTracingEnabled; }

	void SetSelectedGraphicsPreset(ETunaSweeperGraphicsPreset Preset);
	void SetPreferredDLSSMode(ETunaSweeperTitleDLSSMode Mode) { PreferredDLSSMode = Mode; }
	void SetMotionBlurEnabled(bool bEnabled) { bMotionBlurEnabled = bEnabled; }
	void SetHardwareRayTracingEnabled(bool bEnabled) { bHardwareRayTracingEnabled = bEnabled; }

	int32 GetScalabilityOptionQuality(ETunaSweeperScalabilityOption Option) const;
	void SetScalabilityOptionQuality(ETunaSweeperScalabilityOption Option, int32 Quality);

	static ETunaSweeperGraphicsPreset ResolveAutoPresetForDedicatedVideoMemory(int64 DedicatedVideoMemoryBytes);
	static Scalability::FQualityLevels BuildQualityLevelsForPreset(
		ETunaSweeperGraphicsPreset Preset,
		float ResolutionQualityToPreserve);
	static ETunaSweeperGraphicsPreset MatchNamedPreset(const Scalability::FQualityLevels& QualityLevels);

private:
	void MigrateLegacyGraphicsSettings(bool bHadPersistedUserSettings);
	int64 QueryDedicatedVideoMemoryBytes() const;
	void ApplyQualityPreset(ETunaSweeperGraphicsPreset Preset);

	UPROPERTY(Config)
	int32 GraphicsSettingsSchemaVersion = 0;

	UPROPERTY(Config)
	ETunaSweeperGraphicsPreset SelectedGraphicsPreset = ETunaSweeperGraphicsPreset::Auto;

	UPROPERTY(Config)
	ETunaSweeperGraphicsPreset ResolvedAutoGraphicsPreset = ETunaSweeperGraphicsPreset::Low;

	UPROPERTY(Config)
	ETunaSweeperTitleDLSSMode PreferredDLSSMode = ETunaSweeperTitleDLSSMode::Performance;

	UPROPERTY(Config)
	int32 LastDetectedDedicatedVideoMemoryMB = 0;

	UPROPERTY(Config)
	bool bMotionBlurEnabled = true;

	UPROPERTY(Config)
	bool bHardwareRayTracingEnabled = true;
};
