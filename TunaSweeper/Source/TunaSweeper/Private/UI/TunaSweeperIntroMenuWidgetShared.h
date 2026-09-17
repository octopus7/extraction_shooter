#pragma once

#include "UI/TunaSweeperIntroMenuWidget.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "DLSSLibrary.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/ScaleBoxSlot.h"
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
#include "UI/TunaSweeperOptionRowWidget.h"
#include "UI/TunaSweeperTitleWindParticleWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUIStyle.h"

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
	inline const TCHAR* FarmingIconTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyIcon_Farming.T_DifficultyIcon_Farming");
	inline const TCHAR* NormalIconTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyIcon_Normal.T_DifficultyIcon_Normal");
	inline const TCHAR* HardIconTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyIcon_Hard.T_DifficultyIcon_Hard");

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
