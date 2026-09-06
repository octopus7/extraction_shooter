#pragma once

#include "UI/TunaSweeperIntroMenuWidget.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "DLSSLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Dom/JsonObject.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineExternalUIInterface.h"
#include "PixelFormat.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Slate/WidgetTransform.h"
#include "Styling/SlateBrush.h"
#include "Subsystem/TunaSweeperBgmSubsystem.h"
#include "Subsystem/TunaSweeperToastSubsystem.h"
#include "TimerManager.h"
#include "UI/TunaSweeperScreenFadeWidget.h"
#include "UI/TunaSweeperTitleWindParticleWidget.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperTitleGraphicsSettings
{
	inline const TCHAR* SectionName = TEXT("TunaSweeper.GraphicsSettings");
	inline const TCHAR* DLSSModeKey = TEXT("DLSSMode");

	inline UDLSSMode ToDLSSMode(ETunaSweeperTitleDLSSMode Mode)
	{
		switch (Mode)
		{
		case ETunaSweeperTitleDLSSMode::Quality:
			return UDLSSMode::Quality;
		case ETunaSweeperTitleDLSSMode::Balanced:
			return UDLSSMode::Balanced;
		case ETunaSweeperTitleDLSSMode::Performance:
			return UDLSSMode::Performance;
		case ETunaSweeperTitleDLSSMode::Off:
		default:
			return UDLSSMode::Off;
		}
	}

	inline ETunaSweeperTitleDLSSMode ToTitleDLSSMode(int32 ConfigValue)
	{
		switch (ConfigValue)
		{
		case 1:
			return ETunaSweeperTitleDLSSMode::Quality;
		case 2:
			return ETunaSweeperTitleDLSSMode::Balanced;
		case 3:
			return ETunaSweeperTitleDLSSMode::Performance;
		case 0:
		default:
			return ETunaSweeperTitleDLSSMode::Off;
		}
	}

	inline int32 ToConfigValue(ETunaSweeperTitleDLSSMode Mode)
	{
		switch (Mode)
		{
		case ETunaSweeperTitleDLSSMode::Quality:
			return 1;
		case ETunaSweeperTitleDLSSMode::Balanced:
			return 2;
		case ETunaSweeperTitleDLSSMode::Performance:
			return 3;
		case ETunaSweeperTitleDLSSMode::Off:
		default:
			return 0;
		}
	}
}

namespace TunaSweeperDifficultySelect
{
	inline const TCHAR* DefinitionsJsonRelativePath = TEXT("Data/DifficultyDefinitions.json");
	inline const TCHAR* BackgroundTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyBackground.T_DifficultyBackground");
	inline const TCHAR* CardFrameTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyCardFrame.T_DifficultyCardFrame");
	inline const TCHAR* ActionButtonTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyActionButton.T_DifficultyActionButton");
	inline const TCHAR* FarmingIconTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyIcon_Farming.T_DifficultyIcon_Farming");
	inline const TCHAR* NormalIconTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyIcon_Normal.T_DifficultyIcon_Normal");
	inline const TCHAR* HardIconTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyIcon_Hard.T_DifficultyIcon_Hard");

	inline FText MakeFallbackTitle(int32 DifficultyStage)
	{
		switch (DifficultyStage)
		{
		case 1:
			return FText::FromString(TEXT("\uD30C\uBC0D"));
		case 2:
			return FText::FromString(TEXT("\uC77C\uBC18"));
		case 3:
			return FText::FromString(TEXT("\uC5B4\uB824\uC6C0"));
		default:
			return FText::FromString(TEXT("\uD30C\uBC0D"));
		}
	}

	inline FText MakeFallbackDescription(int32 DifficultyStage)
	{
		switch (DifficultyStage)
		{
		case 1:
			return FText::FromString(TEXT("\uD30C\uBC0D\uACFC \uD0D0\uC0C9\uC5D0 \uC5EC\uC720\uAC00 \uC788\uB294 \uC2DC\uC791 \uB09C\uC774\uB3C4\uC785\uB2C8\uB2E4."));
		case 2:
			return FText::FromString(TEXT("\uC0DD\uC874\uACFC \uC804\uD22C\uAC00 \uADE0\uD615 \uC788\uAC8C \uC9C4\uD589\uB418\uB294 \uAE30\uBCF8 \uB09C\uC774\uB3C4\uC785\uB2C8\uB2E4."));
		case 3:
			return FText::FromString(TEXT("\uC790\uC6D0\uACFC \uC804\uD22C \uC555\uBC15\uC774 \uCEE4\uC9C0\uB294 \uB3C4\uC804 \uB09C\uC774\uB3C4\uC785\uB2C8\uB2E4."));
		default:
			return FText::GetEmpty();
		}
	}

	inline FString GetDefinitionsJsonPath()
	{
		return FPaths::Combine(FPaths::ProjectContentDir(), DefinitionsJsonRelativePath);
	}
}

namespace TunaSweeperSettingsUi
{
	constexpr float ButtonCornerRadius = 2.0f;
	const FLinearColor Accent(0.32f, 0.90f, 0.96f, 1.0f);

	inline FSlateBrush MakeRoundedBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		float CornerRadius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(
			CornerRadius,
			FSlateColor(OutlineColor),
			OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}
}
