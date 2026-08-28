#include "Settings/TunaSweeperGameUserSettings.h"

#include "DynamicRHI.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Parse.h"
#include "RHIGlobals.h"
#include "RHIStats.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperGraphicsSettings, Log, All);

namespace TunaSweeperGraphicsSettings
{
	constexpr int32 CurrentSchemaVersion = 1;
	constexpr int64 AutoLowThresholdBytes = 4ll * 1024ll * 1024ll * 1024ll;
	const TCHAR* LegacySectionName = TEXT("TunaSweeper.GraphicsSettings");
	const TCHAR* LegacyDLSSModeKey = TEXT("DLSSMode");

	bool QualityLevelsMatch(
		const Scalability::FQualityLevels& Left,
		const Scalability::FQualityLevels& Right)
	{
		return Left.ViewDistanceQuality == Right.ViewDistanceQuality &&
			Left.AntiAliasingQuality == Right.AntiAliasingQuality &&
			Left.ShadowQuality == Right.ShadowQuality &&
			Left.GlobalIlluminationQuality == Right.GlobalIlluminationQuality &&
			Left.ReflectionQuality == Right.ReflectionQuality &&
			Left.PostProcessQuality == Right.PostProcessQuality &&
			Left.TextureQuality == Right.TextureQuality &&
			Left.EffectsQuality == Right.EffectsQuality &&
			Left.FoliageQuality == Right.FoliageQuality &&
			Left.ShadingQuality == Right.ShadingQuality &&
			Left.LandscapeQuality == Right.LandscapeQuality;
	}

	ETunaSweeperTitleDLSSMode SanitizeDLSSMode(int32 Value)
	{
		switch (Value)
		{
		case 1:
			return ETunaSweeperTitleDLSSMode::Quality;
		case 2:
			return ETunaSweeperTitleDLSSMode::Balanced;
		case 3:
			return ETunaSweeperTitleDLSSMode::Performance;
		default:
			return ETunaSweeperTitleDLSSMode::Off;
		}
	}
}

UTunaSweeperGameUserSettings* UTunaSweeperGameUserSettings::Get()
{
	return GEngine ? Cast<UTunaSweeperGameUserSettings>(GEngine->GetGameUserSettings()) : nullptr;
}

void UTunaSweeperGameUserSettings::SetToDefaults()
{
	Super::SetToDefaults();
	GraphicsSettingsSchemaVersion = TunaSweeperGraphicsSettings::CurrentSchemaVersion;
	SelectedGraphicsPreset = ETunaSweeperGraphicsPreset::Auto;
	ResolvedAutoGraphicsPreset = ETunaSweeperGraphicsPreset::Low;
	PreferredDLSSMode = ETunaSweeperTitleDLSSMode::Performance;
	LastDetectedDedicatedVideoMemoryMB = 0;
	bMotionBlurEnabled = true;
	bHardwareRayTracingEnabled = true;
}

void UTunaSweeperGameUserSettings::LoadSettings(bool bForceReload)
{
	int32 ExistingSchemaVersion = 0;
	const bool bHadSchemaVersion = GConfig && GConfig->GetInt(
		TEXT("/Script/TunaSweeper.TunaSweeperGameUserSettings"),
		TEXT("GraphicsSettingsSchemaVersion"),
		ExistingSchemaVersion,
		GGameUserSettingsIni);
	const bool bHadPersistedUserSettings = IFileManager::Get().FileExists(*GGameUserSettingsIni);

	Super::LoadSettings(bForceReload);

	if (!bHadSchemaVersion || ExistingSchemaVersion < TunaSweeperGraphicsSettings::CurrentSchemaVersion)
	{
		MigrateLegacyGraphicsSettings(bHadPersistedUserSettings);
	}
}

void UTunaSweeperGameUserSettings::InitializeForCurrentHardware()
{
	if (SelectedGraphicsPreset == ETunaSweeperGraphicsPreset::Auto)
	{
		RefreshAutoDetection();
		ApplyQualityPreset(ResolvedAutoGraphicsPreset);

		UE_LOG(
			LogTunaSweeperGraphicsSettings,
			Display,
			TEXT("Auto graphics detected %d MB dedicated VRAM and selected %s."),
			LastDetectedDedicatedVideoMemoryMB,
			ResolvedAutoGraphicsPreset == ETunaSweeperGraphicsPreset::Epic ? TEXT("Epic") : TEXT("Low"));
	}

	ApplyNonResolutionSettings();
	ApplyCustomGraphicsOverrides();
	SaveSettings();
}

ETunaSweeperGraphicsPreset UTunaSweeperGameUserSettings::RefreshAutoDetection()
{
	const int64 DedicatedVideoMemoryBytes = QueryDedicatedVideoMemoryBytes();
	LastDetectedDedicatedVideoMemoryMB = DedicatedVideoMemoryBytes > 0
		? static_cast<int32>(DedicatedVideoMemoryBytes / (1024ll * 1024ll))
		: 0;
	ResolvedAutoGraphicsPreset = ResolveAutoPresetForDedicatedVideoMemory(DedicatedVideoMemoryBytes);
	return ResolvedAutoGraphicsPreset;
}

void UTunaSweeperGameUserSettings::ApplyCustomGraphicsOverrides() const
{
	if (IConsoleVariable* PerTextureStreamingBias =
		IConsoleManager::Get().FindConsoleVariable(TEXT("r.Streaming.UsePerTextureBias")))
	{
		PerTextureStreamingBias->Set(1, ECVF_SetByGameSetting);
	}

	if (IConsoleVariable* MotionBlurQuality = IConsoleManager::Get().FindConsoleVariable(TEXT("r.MotionBlurQuality")))
	{
		const int32 EnabledQuality = GetPostProcessingQuality() >= 3 ? 4 : 3;
		MotionBlurQuality->Set(bMotionBlurEnabled ? EnabledQuality : 0, ECVF_SetByGameSetting);
	}

	if (IConsoleVariable* HardwareRayTracing = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.HardwareRayTracing")))
	{
		HardwareRayTracing->Set(bHardwareRayTracingEnabled && GRHISupportsRayTracing ? 1 : 0, ECVF_SetByGameSetting);
	}
}

FTunaSweeperGraphicsSettingsState UTunaSweeperGameUserSettings::CaptureGraphicsState() const
{
	FTunaSweeperGraphicsSettingsState State;
	State.QualityLevels = ScalabilityQuality;
	State.Resolution = GetScreenResolution();
	State.WindowMode = GetFullscreenMode();
	State.Preset = SelectedGraphicsPreset;
	State.DLSSMode = PreferredDLSSMode;
	State.FrameRateLimit = GetFrameRateLimit();
	State.bVSyncEnabled = IsVSyncEnabled();
	State.bDynamicResolutionEnabled = IsDynamicResolutionEnabled();
	State.bMotionBlurEnabled = bMotionBlurEnabled;
	State.bHardwareRayTracingEnabled = bHardwareRayTracingEnabled;
	return State;
}

void UTunaSweeperGameUserSettings::ApplyGraphicsState(
	const FTunaSweeperGraphicsSettingsState& State,
	bool bSaveSettings)
{
	ScalabilityQuality = State.QualityLevels;
	SetScreenResolution(State.Resolution);
	SetFullscreenMode(State.WindowMode);
	SetVSyncEnabled(State.bVSyncEnabled);
	SetDynamicResolutionEnabled(State.bDynamicResolutionEnabled);
	SetFrameRateLimit(FMath::Max(0.0f, State.FrameRateLimit));
	SelectedGraphicsPreset = State.Preset;
	PreferredDLSSMode = State.DLSSMode;
	bMotionBlurEnabled = State.bMotionBlurEnabled;
	bHardwareRayTracingEnabled = State.bHardwareRayTracingEnabled;

	ApplyNonResolutionSettings();
	ApplyResolutionSettings(false);
	ApplyCustomGraphicsOverrides();
	if (bSaveSettings)
	{
		SaveSettings();
	}
}

void UTunaSweeperGameUserSettings::SetSelectedGraphicsPreset(ETunaSweeperGraphicsPreset Preset)
{
	SelectedGraphicsPreset = Preset;
	if (Preset == ETunaSweeperGraphicsPreset::Auto)
	{
		RefreshAutoDetection();
		ApplyQualityPreset(ResolvedAutoGraphicsPreset);
	}
	else if (Preset != ETunaSweeperGraphicsPreset::Custom)
	{
		ApplyQualityPreset(Preset);
	}
}

int32 UTunaSweeperGameUserSettings::GetScalabilityOptionQuality(ETunaSweeperScalabilityOption Option) const
{
	switch (Option)
	{
	case ETunaSweeperScalabilityOption::Texture:
		return GetTextureQuality();
	case ETunaSweeperScalabilityOption::Shadow:
		return GetShadowQuality();
	case ETunaSweeperScalabilityOption::GlobalIllumination:
		return GetGlobalIlluminationQuality();
	case ETunaSweeperScalabilityOption::Reflection:
		return GetReflectionQuality();
	case ETunaSweeperScalabilityOption::ViewDistance:
		return GetViewDistanceQuality();
	case ETunaSweeperScalabilityOption::Effects:
		return GetVisualEffectQuality();
	case ETunaSweeperScalabilityOption::PostProcess:
		return GetPostProcessingQuality();
	case ETunaSweeperScalabilityOption::Foliage:
		return GetFoliageQuality();
	case ETunaSweeperScalabilityOption::Shading:
		return GetShadingQuality();
	case ETunaSweeperScalabilityOption::Landscape:
		return ScalabilityQuality.LandscapeQuality;
	case ETunaSweeperScalabilityOption::AntiAliasing:
	default:
		return GetAntiAliasingQuality();
	}
}

void UTunaSweeperGameUserSettings::SetScalabilityOptionQuality(
	ETunaSweeperScalabilityOption Option,
	int32 Quality)
{
	const int32 ClampedQuality = FMath::Clamp(Quality, 0, 3);
	switch (Option)
	{
	case ETunaSweeperScalabilityOption::Texture:
		SetTextureQuality(ClampedQuality);
		break;
	case ETunaSweeperScalabilityOption::Shadow:
		SetShadowQuality(ClampedQuality);
		break;
	case ETunaSweeperScalabilityOption::GlobalIllumination:
		SetGlobalIlluminationQuality(ClampedQuality);
		break;
	case ETunaSweeperScalabilityOption::Reflection:
		SetReflectionQuality(ClampedQuality);
		break;
	case ETunaSweeperScalabilityOption::ViewDistance:
		SetViewDistanceQuality(ClampedQuality);
		break;
	case ETunaSweeperScalabilityOption::Effects:
		SetVisualEffectQuality(ClampedQuality);
		break;
	case ETunaSweeperScalabilityOption::PostProcess:
		SetPostProcessingQuality(ClampedQuality);
		break;
	case ETunaSweeperScalabilityOption::Foliage:
		SetFoliageQuality(ClampedQuality);
		break;
	case ETunaSweeperScalabilityOption::Shading:
		SetShadingQuality(ClampedQuality);
		break;
	case ETunaSweeperScalabilityOption::Landscape:
		ScalabilityQuality.LandscapeQuality = ClampedQuality;
		break;
	case ETunaSweeperScalabilityOption::AntiAliasing:
	default:
		SetAntiAliasingQuality(ClampedQuality);
		break;
	}
	SelectedGraphicsPreset = ETunaSweeperGraphicsPreset::Custom;
}

ETunaSweeperGraphicsPreset UTunaSweeperGameUserSettings::ResolveAutoPresetForDedicatedVideoMemory(
	int64 DedicatedVideoMemoryBytes)
{
	return DedicatedVideoMemoryBytes >= TunaSweeperGraphicsSettings::AutoLowThresholdBytes
		? ETunaSweeperGraphicsPreset::Epic
		: ETunaSweeperGraphicsPreset::Low;
}

Scalability::FQualityLevels UTunaSweeperGameUserSettings::BuildQualityLevelsForPreset(
	ETunaSweeperGraphicsPreset Preset,
	float ResolutionQualityToPreserve)
{
	Scalability::FQualityLevels Levels;
	const int32 UniformQuality = Preset == ETunaSweeperGraphicsPreset::Medium
		? 1
		: (Preset == ETunaSweeperGraphicsPreset::High ? 2 : 3);
	Levels.SetFromSingleQualityLevel(UniformQuality);
	Levels.ResolutionQuality = ResolutionQualityToPreserve;

	if (Preset == ETunaSweeperGraphicsPreset::Low)
	{
		Levels.TextureQuality = 0;
		Levels.ShadowQuality = 1;
		Levels.GlobalIlluminationQuality = 1;
		Levels.ReflectionQuality = 0;
		Levels.ViewDistanceQuality = 1;
		Levels.EffectsQuality = 1;
		Levels.PostProcessQuality = 0;
		Levels.FoliageQuality = 0;
		Levels.ShadingQuality = 1;
		Levels.LandscapeQuality = 1;
		Levels.AntiAliasingQuality = 1;
	}
	else if (Preset == ETunaSweeperGraphicsPreset::Medium)
	{
		Levels.AntiAliasingQuality = 2;
	}

	return Levels;
}

ETunaSweeperGraphicsPreset UTunaSweeperGameUserSettings::MatchNamedPreset(
	const Scalability::FQualityLevels& QualityLevels)
{
	for (const ETunaSweeperGraphicsPreset Preset : {
			ETunaSweeperGraphicsPreset::Low,
			ETunaSweeperGraphicsPreset::Medium,
			ETunaSweeperGraphicsPreset::High,
			ETunaSweeperGraphicsPreset::Epic })
	{
		if (TunaSweeperGraphicsSettings::QualityLevelsMatch(
			QualityLevels,
			BuildQualityLevelsForPreset(Preset, QualityLevels.ResolutionQuality)))
		{
			return Preset;
		}
	}

	return ETunaSweeperGraphicsPreset::Custom;
}

void UTunaSweeperGameUserSettings::MigrateLegacyGraphicsSettings(bool bHadPersistedUserSettings)
{
	SelectedGraphicsPreset = bHadPersistedUserSettings
		? MatchNamedPreset(ScalabilityQuality)
		: ETunaSweeperGraphicsPreset::Auto;

	int32 LegacyDLSSMode = static_cast<int32>(PreferredDLSSMode);
	if (GConfig && GConfig->GetInt(
		TunaSweeperGraphicsSettings::LegacySectionName,
		TunaSweeperGraphicsSettings::LegacyDLSSModeKey,
		LegacyDLSSMode,
		GGameUserSettingsIni))
	{
		PreferredDLSSMode = TunaSweeperGraphicsSettings::SanitizeDLSSMode(LegacyDLSSMode);
	}

	GraphicsSettingsSchemaVersion = TunaSweeperGraphicsSettings::CurrentSchemaVersion;
	SaveSettings();
}

int64 UTunaSweeperGameUserSettings::QueryDedicatedVideoMemoryBytes() const
{
#if !UE_BUILD_SHIPPING
	int32 TestVideoMemoryMB = 0;
	if (FParse::Value(FCommandLine::Get(), TEXT("TunaGraphicsTestVRAMMB="), TestVideoMemoryMB))
	{
		return static_cast<int64>(FMath::Max(0, TestVideoMemoryMB)) * 1024ll * 1024ll;
	}
#endif

	if (!GDynamicRHI)
	{
		return -1;
	}

	FTextureMemoryStats TextureMemoryStats;
	RHIGetTextureMemoryStats(TextureMemoryStats);
	return TextureMemoryStats.DedicatedVideoMemory;
}

void UTunaSweeperGameUserSettings::ApplyQualityPreset(ETunaSweeperGraphicsPreset Preset)
{
	if (Preset == ETunaSweeperGraphicsPreset::Auto)
	{
		Preset = ResolvedAutoGraphicsPreset;
	}
	if (Preset == ETunaSweeperGraphicsPreset::Custom)
	{
		return;
	}

	ScalabilityQuality = BuildQualityLevelsForPreset(Preset, ScalabilityQuality.ResolutionQuality);
}
