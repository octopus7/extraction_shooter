#include "Title/TunaSweeperDisplaySettings.h"

#include "DynamicRHI.h"
#include "GameFramework/GameUserSettings.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperDisplaySettings, Log, All);

bool TunaSweeperDisplaySettings::ClampUnsupportedFullscreenResolution(UGameUserSettings& GameUserSettings)
{
	if (GameUserSettings.GetFullscreenMode() != EWindowMode::Fullscreen || !GDynamicRHI)
	{
		return false;
	}

	FScreenResolutionArray SupportedResolutions;
	if (!RHIGetAvailableResolutions(SupportedResolutions, true) || SupportedResolutions.IsEmpty())
	{
		UE_LOG(
			LogTunaSweeperDisplaySettings,
			Warning,
			TEXT("Could not enumerate fullscreen resolutions; keeping the requested resolution."));
		return false;
	}

	const FIntPoint RequestedResolution = GameUserSettings.GetScreenResolution();
	FIntPoint MaximumSupportedResolution = FIntPoint::ZeroValue;
	uint64 MaximumPixelCount = 0;
	bool bRequestedResolutionIsSupported = false;

	for (const FScreenResolutionRHI& SupportedResolution : SupportedResolutions)
	{
		if (SupportedResolution.Width == 0 || SupportedResolution.Height == 0)
		{
			continue;
		}

		const FIntPoint Resolution(
			static_cast<int32>(SupportedResolution.Width),
			static_cast<int32>(SupportedResolution.Height));
		bRequestedResolutionIsSupported |= Resolution == RequestedResolution;

		const uint64 PixelCount = static_cast<uint64>(SupportedResolution.Width) * SupportedResolution.Height;
		if (PixelCount > MaximumPixelCount ||
			(PixelCount == MaximumPixelCount && Resolution.X > MaximumSupportedResolution.X))
		{
			MaximumSupportedResolution = Resolution;
			MaximumPixelCount = PixelCount;
		}
	}

	if (bRequestedResolutionIsSupported || MaximumPixelCount == 0)
	{
		return false;
	}

	UE_LOG(
		LogTunaSweeperDisplaySettings,
		Warning,
		TEXT("Unsupported fullscreen resolution %dx%d; falling back to maximum supported resolution %dx%d."),
		RequestedResolution.X,
		RequestedResolution.Y,
		MaximumSupportedResolution.X,
		MaximumSupportedResolution.Y);

	GameUserSettings.SetScreenResolution(MaximumSupportedResolution);
	GameUserSettings.ConfirmVideoMode();
	return true;
}
