#include "TunaSweeperIntroMenuWidgetShared.h"
#include "Settings/TunaSweeperGameUserSettings.h"
#include "Title/TunaSweeperDisplaySettings.h"

void UTunaSweeperIntroMenuWidget::LoadTitleGraphicsSettings()
{
	if (const UTunaSweeperGameUserSettings* Settings = UTunaSweeperGameUserSettings::Get())
	{
		PreferredDLSSMode = Settings->GetPreferredDLSSMode();
	}
}

void UTunaSweeperIntroMenuWidget::SaveTitleGraphicsSettings() const
{
	UTunaSweeperGameUserSettings* Settings = UTunaSweeperGameUserSettings::Get();
	if (!Settings)
	{
		return;
	}

	Settings->SetPreferredDLSSMode(PreferredDLSSMode);
	Settings->SaveSettings();
}

void UTunaSweeperIntroMenuWidget::ApplyDLSSSetting(ETunaSweeperTitleDLSSMode DLSSMode)
{
	PreferredDLSSMode = DLSSMode;
	ApplyDLSSModeToRuntime(PreferredDLSSMode);
	SaveTitleGraphicsSettings();
	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ApplyDLSSModeToRuntime(ETunaSweeperTitleDLSSMode DLSSMode) const
{
	if (DLSSMode == ETunaSweeperTitleDLSSMode::Off || !IsDLSSModeAvailable(DLSSMode))
	{
		UDLSSLibrary::SetDLSSMode(GetWorld(), UDLSSMode::Off);
		if (IConsoleVariable* ScreenPercentageCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
		{
			ScreenPercentageCVar->Set(100.0f, ECVF_SetByGameSetting);
		}
		return;
	}

	const UDLSSMode RuntimeDLSSMode = TunaSweeperTitleGraphicsSettings::ToDLSSMode(DLSSMode);
	UDLSSLibrary::SetDLSSMode(GetWorld(), RuntimeDLSSMode);
}

bool UTunaSweeperIntroMenuWidget::IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode DLSSMode) const
{
	if (DLSSMode == ETunaSweeperTitleDLSSMode::Off)
	{
		return true;
	}

	return UDLSSLibrary::IsDLSSSupported() &&
		UDLSSLibrary::IsDLSSModeSupported(TunaSweeperTitleGraphicsSettings::ToDLSSMode(DLSSMode));
}

FText UTunaSweeperIntroMenuWidget::BuildWindowModeText(EWindowMode::Type WindowMode) const
{
	switch (WindowMode)
	{
	case EWindowMode::Fullscreen:
		return ResolveUiText(FName(TEXT("ui.settings.fullscreen")), FText::FromString(TEXT("\uC804\uCCB4\uD654\uBA74\uBAA8\uB4DC")));
	case EWindowMode::WindowedFullscreen:
		return ResolveUiText(FName(TEXT("ui.settings.borderless")), FText::FromString(TEXT("\uD14C\uB450\uB9AC \uC5C6\uB294 \uCC3D\uBAA8\uB4DC")));
	case EWindowMode::Windowed:
	default:
		return ResolveUiText(FName(TEXT("ui.settings.windowed")), FText::FromString(TEXT("\uCC3D\uBAA8\uB4DC")));
	}
}

FText UTunaSweeperIntroMenuWidget::BuildDLSSModeText(ETunaSweeperTitleDLSSMode DLSSMode) const
{
	switch (DLSSMode)
	{
	case ETunaSweeperTitleDLSSMode::Quality:
		return ResolveUiText(FName(TEXT("ui.settings.dlss.quality")), FText::FromString(TEXT("\uD488\uC9C8")));
	case ETunaSweeperTitleDLSSMode::Balanced:
		return ResolveUiText(FName(TEXT("ui.settings.dlss.balanced")), FText::FromString(TEXT("\uADE0\uD615")));
	case ETunaSweeperTitleDLSSMode::Performance:
		return ResolveUiText(FName(TEXT("ui.settings.dlss.performance")), FText::FromString(TEXT("\uC131\uB2A5")));
	case ETunaSweeperTitleDLSSMode::Off:
	default:
		return ResolveUiText(FName(TEXT("ui.settings.dlss.off")), FText::FromString(TEXT("\uB044\uAE30")));
	}
}

FText UTunaSweeperIntroMenuWidget::BuildLanguageNameText(ETunaSweeperItemTextLanguage Language) const
{
	switch (Language)
	{
	case ETunaSweeperItemTextLanguage::Korean:
		return ResolveUiText(FName(TEXT("ui.language.korean")), FText::GetEmpty());
	case ETunaSweeperItemTextLanguage::Japanese:
		return ResolveUiText(FName(TEXT("ui.language.japanese")), FText::GetEmpty());
	case ETunaSweeperItemTextLanguage::English:
	default:
		return ResolveUiText(FName(TEXT("ui.language.english")), FText::GetEmpty());
	}
}

FText UTunaSweeperIntroMenuWidget::ResolveUiText(FName StringKey, const FText& FallbackText) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return TunaGameInstance
		? TunaGameInstance->ResolveLocalizedText(StringKey, bPauseSettingsMode ? FText::GetEmpty() : FallbackText)
		: (bPauseSettingsMode ? FText::GetEmpty() : FallbackText);
}

void UTunaSweeperIntroMenuWidget::SetNamedText(FName WidgetName, const FText& Text) const
{
	if (!WidgetTree || WidgetName.IsNone())
	{
		return;
	}

	if (UTextBlock* TextBlock = Cast<UTextBlock>(FindIntroWidget(WidgetName)))
	{
		TextBlock->SetText(Text);
	}
}

void UTunaSweeperIntroMenuWidget::ApplyDisplaySettings(EWindowMode::Type WindowMode)
{
	if (!GEngine)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
	{
		GameUserSettings->SetFullscreenMode(WindowMode);
		TunaSweeperDisplaySettings::ClampUnsupportedFullscreenResolution(*GameUserSettings);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ApplyResolutionSetting(const FIntPoint& Resolution)
{
	if (!GEngine)
	{
		return;
	}

	if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
	{
		GameUserSettings->SetScreenResolution(Resolution);
		TunaSweeperDisplaySettings::ClampUnsupportedFullscreenResolution(*GameUserSettings);
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();
	}

	RefreshSettingsPanel();
}
