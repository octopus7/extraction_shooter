#include "TunaSweeperIntroMenuWidgetShared.h"

void UTunaSweeperIntroMenuWidget::LoadTitleGraphicsSettings()
{
	int32 DLSSModeValue = TunaSweeperTitleGraphicsSettings::ToConfigValue(PreferredDLSSMode);
	if (GConfig)
	{
		GConfig->GetInt(
			TunaSweeperTitleGraphicsSettings::SectionName,
			TunaSweeperTitleGraphicsSettings::DLSSModeKey,
			DLSSModeValue,
			GGameUserSettingsIni);
	}

	PreferredDLSSMode = TunaSweeperTitleGraphicsSettings::ToTitleDLSSMode(DLSSModeValue);
}

void UTunaSweeperIntroMenuWidget::SaveTitleGraphicsSettings() const
{
	if (!GConfig)
	{
		return;
	}

	GConfig->SetInt(
		TunaSweeperTitleGraphicsSettings::SectionName,
		TunaSweeperTitleGraphicsSettings::DLSSModeKey,
		TunaSweeperTitleGraphicsSettings::ToConfigValue(PreferredDLSSMode),
		GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
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
		return FText::FromString(TEXT("\uD55C\uAD6D\uC5B4"));
	case ETunaSweeperItemTextLanguage::Japanese:
		return FText::FromString(TEXT("\u65E5\u672C\u8A9E"));
	case ETunaSweeperItemTextLanguage::English:
	default:
		return FText::FromString(TEXT("English"));
	}
}

FText UTunaSweeperIntroMenuWidget::BuildLanguageOptionText(
	ETunaSweeperItemTextLanguage Language,
	bool bSelected) const
{
	return FText::FromString(FString::Printf(
		TEXT("%s %s"),
		bSelected ? TEXT("[x]") : TEXT("[ ]"),
		*BuildLanguageNameText(Language).ToString()));
}

FText UTunaSweeperIntroMenuWidget::ResolveUiText(FName StringKey, const FText& FallbackText) const
{
	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return TunaGameInstance
		? TunaGameInstance->ResolveLocalizedText(StringKey, FallbackText)
		: FallbackText;
}

void UTunaSweeperIntroMenuWidget::SetNamedText(FName WidgetName, const FText& Text) const
{
	if (!WidgetTree || WidgetName.IsNone())
	{
		return;
	}

	if (UTextBlock* TextBlock = Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName)))
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
		GameUserSettings->ApplySettings(false);
		GameUserSettings->SaveSettings();
	}

	RefreshSettingsPanel();
}
