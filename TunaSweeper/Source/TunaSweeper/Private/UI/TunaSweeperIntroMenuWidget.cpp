#include "UI/TunaSweeperIntroMenuWidget.h"

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
	const TCHAR* SectionName = TEXT("TunaSweeper.GraphicsSettings");
	const TCHAR* DLSSModeKey = TEXT("DLSSMode");

	UDLSSMode ToDLSSMode(ETunaSweeperTitleDLSSMode Mode)
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

	ETunaSweeperTitleDLSSMode ToTitleDLSSMode(int32 ConfigValue)
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

	int32 ToConfigValue(ETunaSweeperTitleDLSSMode Mode)
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
	const TCHAR* DefinitionsJsonRelativePath = TEXT("Data/DifficultyDefinitions.json");
	const TCHAR* BackgroundTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyBackground.T_DifficultyBackground");
	const TCHAR* CardFrameTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyCardFrame.T_DifficultyCardFrame");
	const TCHAR* ActionButtonTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyActionButton.T_DifficultyActionButton");
	const TCHAR* FarmingIconTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyIcon_Farming.T_DifficultyIcon_Farming");
	const TCHAR* NormalIconTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyIcon_Normal.T_DifficultyIcon_Normal");
	const TCHAR* HardIconTexturePath = TEXT("/Game/UI/Difficulty/T_DifficultyIcon_Hard.T_DifficultyIcon_Hard");

	FText MakeFallbackTitle(int32 DifficultyStage)
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

	FText MakeFallbackDescription(int32 DifficultyStage)
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

	FString GetDefinitionsJsonPath()
	{
		return FPaths::Combine(FPaths::ProjectContentDir(), DefinitionsJsonRelativePath);
	}
}

namespace TunaSweeperSettingsUi
{
	constexpr float PanelWidth = 840.0f;
	constexpr float PanelHeight = 780.0f;
	constexpr float PanelLeft = 164.0f;
	constexpr float SectionCornerRadius = 8.0f;
	constexpr float ButtonCornerRadius = 7.0f;
	const FLinearColor PanelFill(0.015f, 0.025f, 0.030f, 0.86f);
	const FLinearColor PanelOutline(0.58f, 0.70f, 0.70f, 0.62f);
	const FLinearColor SectionFill(0.018f, 0.038f, 0.044f, 0.64f);
	const FLinearColor SectionOutline(0.46f, 0.58f, 0.58f, 0.40f);
	const FLinearColor TextPrimary(0.95f, 0.98f, 0.97f, 1.0f);
	const FLinearColor TextMuted(0.70f, 0.80f, 0.79f, 1.0f);
	const FLinearColor Accent(0.32f, 0.90f, 0.96f, 1.0f);

	FSlateBrush MakeRoundedBoxBrush(
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

	template <typename WidgetType>
	WidgetType* FindWidget(UWidgetTree* WidgetTree, const TCHAR* WidgetName)
	{
		return WidgetTree
			? Cast<WidgetType>(WidgetTree->FindWidget(FName(WidgetName)))
			: nullptr;
	}

	template <typename WidgetType>
	WidgetType* FindOrConstructWidget(UWidgetTree* WidgetTree, const TCHAR* WidgetName)
	{
		if (!WidgetTree)
		{
			return nullptr;
		}

		if (WidgetType* ExistingWidget = FindWidget<WidgetType>(WidgetTree, WidgetName))
		{
			return ExistingWidget;
		}

		return WidgetTree->ConstructWidget<WidgetType>(
			WidgetType::StaticClass(),
			FName(WidgetName));
	}

	void ConfigureCanvasSlot(
		UWidget* Widget,
		const FAnchors& Anchors,
		const FVector2D& Alignment,
		const FVector2D& Position,
		const FVector2D& Size,
		int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetAnchors(Anchors);
			CanvasSlot->SetAlignment(Alignment);
			CanvasSlot->SetPosition(Position);
			CanvasSlot->SetSize(Size);
			CanvasSlot->SetZOrder(ZOrder);
		}
	}

	void ConfigureFillCanvasSlot(UWidget* Widget, int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			CanvasSlot->SetOffsets(FMargin(0.0f));
			CanvasSlot->SetAlignment(FVector2D::ZeroVector);
			CanvasSlot->SetZOrder(ZOrder);
		}
	}

	void ConfigureSizeBox(UWidgetTree* WidgetTree, const TCHAR* WidgetName, float Width, float Height)
	{
		if (USizeBox* SizeBox = FindWidget<USizeBox>(WidgetTree, WidgetName))
		{
			SizeBox->SetWidthOverride(Width);
			SizeBox->SetHeightOverride(Height);
		}
	}

	void ConfigureTextBlock(
		UWidgetTree* WidgetTree,
		const TCHAR* WidgetName,
		float FontSize,
		const FLinearColor& Color,
		ETunaSweeperUIFontWeight Weight = ETunaSweeperUIFontWeight::Preserve,
		ETextJustify::Type Justification = ETextJustify::Left)
	{
		if (UTextBlock* TextBlock = FindWidget<UTextBlock>(WidgetTree, WidgetName))
		{
			TextBlock->SetColorAndOpacity(FSlateColor(Color));
			TextBlock->SetJustification(Justification);
			TextBlock->SetMargin(FMargin(0.0f));
			TextBlock->SetAutoWrapText(true);
			TunaSweeperUIFont::ApplyFont(TextBlock, FontSize, Weight);
		}
	}

	void AddVerticalChild(UVerticalBox* Parent, UWidget* Child, const FMargin& Padding)
	{
		if (!Parent || !Child)
		{
			return;
		}

		Child->RemoveFromParent();
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Child))
		{
			Slot->SetPadding(Padding);
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	void AddHorizontalChild(UHorizontalBox* Parent, UWidget* Child, const FMargin& Padding)
	{
		if (!Parent || !Child)
		{
			return;
		}

		Child->RemoveFromParent();
		if (UHorizontalBoxSlot* Slot = Parent->AddChildToHorizontalBox(Child))
		{
			Slot->SetPadding(Padding);
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	void BuildSettingsSection(
		UWidgetTree* WidgetTree,
		UVerticalBox* Parent,
		const TCHAR* SectionBorderName,
		const TCHAR* SectionStackName,
		UWidget* LabelWidget,
		UWidget* ControlWidget,
		const FMargin& OuterPadding)
	{
		UBorder* SectionBorder = FindOrConstructWidget<UBorder>(WidgetTree, SectionBorderName);
		UVerticalBox* SectionStack = FindOrConstructWidget<UVerticalBox>(WidgetTree, SectionStackName);
		if (!SectionBorder || !SectionStack)
		{
			return;
		}

		SectionStack->ClearChildren();
		SectionBorder->SetPadding(FMargin(18.0f, 14.0f, 18.0f, 16.0f));
		SectionBorder->SetBrush(MakeRoundedBoxBrush(
			FVector2D(760.0f, 116.0f),
			SectionFill,
			SectionOutline,
			1.0f,
			SectionCornerRadius));
		SectionBorder->SetContent(SectionStack);

		AddVerticalChild(SectionStack, LabelWidget, FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		AddVerticalChild(SectionStack, ControlWidget, FMargin(0.0f));
		AddVerticalChild(Parent, SectionBorder, OuterPadding);
	}
}

void UTunaSweeperIntroMenuWidget::PrepareForInitialViewport()
{
	ResetTitleViewportLayoutState();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	EnsureSettingsPanelLayout();
	ApplyTitleMenuButtonContentLayout();
	EnsureAlwaysNewStartButton();
	EnsureDifficultySelectionPanel();
	EnsureDeleteSaveSlotHoldProgressWidget();
	HideLegacyDeleteHoldGaugeWidgets();
	EnsureTitleWindParticleOverlay();
	InvalidateLayoutAndVolatility();
	ForceLayoutPrepass();
}

void UTunaSweeperIntroMenuWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ResetTitleViewportLayoutState();
	HideLegacyDeleteHoldGaugeWidgets();
}

void UTunaSweeperIntroMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ResetTitleViewportLayoutState();
	SetIsFocusable(true);
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	EnsureTitleWindParticleOverlay();
	EnsureDeleteSaveSlotHoldProgressWidget();
	EnsureSaveSlotSelectionRingWidgets();
	EnsureDifficultySelectionPanel();
	HideLegacyDeleteHoldGaugeWidgets();
	EnsureSettingsPanelLayout();

	if (StartButton)
	{
		StartButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleStartClicked);
		StartButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleStartClicked);
	}

	if (SlotSelectButton)
	{
		SlotSelectButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked);
		SlotSelectButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked);
	}

	if (SettingsButton)
	{
		SettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsClicked);
		SettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsClicked);
	}

	if (CreditsButton)
	{
		CreditsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCreditsClicked);
		CreditsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCreditsClicked);
	}

	if (QuitButton)
	{
		QuitButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleQuitClicked);
		QuitButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleQuitClicked);
	}

	EnsureAlwaysNewStartButton();
	if (AlwaysNewStartButton)
	{
		AlwaysNewStartButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleAlwaysNewStartClicked);
		AlwaysNewStartButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleAlwaysNewStartClicked);
	}

	if (DifficultyFarmingButton)
	{
		DifficultyFarmingButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyFarmingClicked);
		DifficultyFarmingButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyFarmingClicked);
	}

	if (DifficultyNormalButton)
	{
		DifficultyNormalButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyNormalClicked);
		DifficultyNormalButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyNormalClicked);
	}

	if (DifficultyHardButton)
	{
		DifficultyHardButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyHardClicked);
		DifficultyHardButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyHardClicked);
	}

	if (DifficultyStartButton)
	{
		DifficultyStartButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyStartClicked);
		DifficultyStartButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyStartClicked);
	}

	if (DifficultyBackButton)
	{
		DifficultyBackButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyBackClicked);
		DifficultyBackButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDifficultyBackClicked);
	}

	if (SaveSlot1Button)
	{
		SaveSlot1Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
		SaveSlot1Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
		SaveSlot1Button->OnHovered.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
		SaveSlot1Button->OnHovered.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused);
	}

	if (SaveSlot2Button)
	{
		SaveSlot2Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
		SaveSlot2Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
		SaveSlot2Button->OnHovered.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
		SaveSlot2Button->OnHovered.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused);
	}

	if (SaveSlot3Button)
	{
		SaveSlot3Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
		SaveSlot3Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
		SaveSlot3Button->OnHovered.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
		SaveSlot3Button->OnHovered.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused);
	}

	if (PrimarySaveSlotButton)
	{
		PrimarySaveSlotButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandlePrimarySaveSlotClicked);
		PrimarySaveSlotButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandlePrimarySaveSlotClicked);
	}

	if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotClicked);
		DeleteSaveSlotButton->OnPressed.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotPressed);
		DeleteSaveSlotButton->OnPressed.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotPressed);
		DeleteSaveSlotButton->OnReleased.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotReleased);
		DeleteSaveSlotButton->OnReleased.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotReleased);
	}

	if (BackToMainMenuButton)
	{
		BackToMainMenuButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked);
		BackToMainMenuButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked);
	}

	if (ConfirmDeleteButton)
	{
		ConfirmDeleteButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked);
		ConfirmDeleteButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked);
	}

	if (CancelDeleteButton)
	{
		CancelDeleteButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked);
		CancelDeleteButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked);
	}

	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsGraphicsTabClicked);
		SettingsGraphicsTabButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsGraphicsTabClicked);
	}

	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsInterfaceTabClicked);
		SettingsInterfaceTabButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleSettingsInterfaceTabClicked);
	}

	if (WindowedModeButton)
	{
		WindowedModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked);
		WindowedModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked);
	}

	if (BorderlessWindowModeButton)
	{
		BorderlessWindowModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBorderlessWindowModeClicked);
		BorderlessWindowModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBorderlessWindowModeClicked);
	}

	if (FullscreenModeButton)
	{
		FullscreenModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleFullscreenModeClicked);
		FullscreenModeButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleFullscreenModeClicked);
	}

	if (Resolution1280Button)
	{
		Resolution1280Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1280Clicked);
		Resolution1280Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1280Clicked);
	}

	if (Resolution1600Button)
	{
		Resolution1600Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1600Clicked);
		Resolution1600Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1600Clicked);
	}

	if (Resolution1920Button)
	{
		Resolution1920Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1920Clicked);
		Resolution1920Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution1920Clicked);
	}

	if (Resolution2560Button)
	{
		Resolution2560Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution2560Clicked);
		Resolution2560Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution2560Clicked);
	}

	if (Resolution3840Button)
	{
		Resolution3840Button->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution3840Clicked);
		Resolution3840Button->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleResolution3840Clicked);
	}

	if (DLSSOffButton)
	{
		DLSSOffButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSOffClicked);
		DLSSOffButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSOffClicked);
	}

	if (DLSSQualityButton)
	{
		DLSSQualityButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSQualityClicked);
		DLSSQualityButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSQualityClicked);
	}

	if (DLSSBalancedButton)
	{
		DLSSBalancedButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSBalancedClicked);
		DLSSBalancedButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSBalancedClicked);
	}

	if (DLSSPerformanceButton)
	{
		DLSSPerformanceButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSPerformanceClicked);
		DLSSPerformanceButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleDLSSPerformanceClicked);
	}

	if (BackFromSettingsButton)
	{
		BackFromSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked);
		BackFromSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked);
	}

	if (LanguageEnglishButton)
	{
		LanguageEnglishButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageEnglishClicked);
		LanguageEnglishButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageEnglishClicked);
	}

	if (LanguageKoreanButton)
	{
		LanguageKoreanButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageKoreanClicked);
		LanguageKoreanButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageKoreanClicked);
	}

	if (LanguageJapaneseButton)
	{
		LanguageJapaneseButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageJapaneseClicked);
		LanguageJapaneseButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleLanguageJapaneseClicked);
	}

	if (ConfirmInterfaceSettingsButton)
	{
		ConfirmInterfaceSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmInterfaceSettingsClicked);
		ConfirmInterfaceSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleConfirmInterfaceSettingsClicked);
	}

	if (CancelInterfaceSettingsButton)
	{
		CancelInterfaceSettingsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelInterfaceSettingsClicked);
		CancelInterfaceSettingsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleCancelInterfaceSettingsClicked);
	}

	if (BackFromCreditsButton)
	{
		BackFromCreditsButton->OnClicked.RemoveDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked);
		BackFromCreditsButton->OnClicked.AddDynamic(this, &UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperIntroMenuWidget::HandleLanguageChanged);
	}

	ApplyTitleMenuButtonContentLayout();
	RefreshLocalizedTexts();
	LoadTitleGraphicsSettings();
	ApplyDLSSModeToRuntime(PreferredDLSSMode);

	if (CreditsText)
	{
		CreditsText->SetText(FText::FromString(BuildCreditsColumnText(0)));
	}
	if (CreditsText2)
	{
		CreditsText2->SetText(FText::FromString(BuildCreditsColumnText(1)));
	}
	if (CreditsText3)
	{
		CreditsText3->SetText(FText::FromString(BuildCreditsColumnText(2)));
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	ResetDeleteHoldProgress();
	HideDeleteConfirmDialog();
	HideOverlayPanels();
	ShowMainMenu();
	InvalidateLayoutAndVolatility();
	ForceLayoutPrepass();
}

void UTunaSweeperIntroMenuWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

FReply UTunaSweeperIntroMenuWidget::NativeOnPreviewKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (!InKeyEvent.IsRepeat() && InKeyEvent.GetKey() == EKeys::R)
	{
		ReloadIntroLevel();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

void UTunaSweeperIntroMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (IsCreditsPanelVisible() && CreditsScrollBox)
	{
		CreditsScrollOffset += InDeltaTime * CreditsScrollSpeed;
		CreditsScrollBox->SetScrollOffset(CreditsScrollOffset);
		if (CreditsScrollBox2)
		{
			CreditsScrollBox2->SetScrollOffset(CreditsScrollOffset);
		}
		if (CreditsScrollBox3)
		{
			CreditsScrollBox3->SetScrollOffset(CreditsScrollOffset);
		}
		if (CreditsScrollOffset > 3600.0f)
		{
			CreditsScrollOffset = 0.0f;
			CreditsScrollBox->SetScrollOffset(0.0f);
			if (CreditsScrollBox2)
			{
				CreditsScrollBox2->SetScrollOffset(0.0f);
			}
			if (CreditsScrollBox3)
			{
				CreditsScrollBox3->SetScrollOffset(0.0f);
			}
		}
	}

	if (!IsSaveSlotSelectionVisible())
	{
		if (bDeleteHoldActive || DeleteHoldElapsedSeconds > 0.0f)
		{
			ResetDeleteHoldProgress();
		}
		return;
	}

	if (SaveSlot1Button && SaveSlot1Button->HasKeyboardFocus())
	{
		SelectSaveSlot(1);
	}
	else if (SaveSlot2Button && SaveSlot2Button->HasKeyboardFocus())
	{
		SelectSaveSlot(2);
	}
	else if (SaveSlot3Button && SaveSlot3Button->HasKeyboardFocus())
	{
		SelectSaveSlot(3);
	}

	UpdateSaveSlotSelectionRingAnimation(InDeltaTime);

	if (!bDeleteHoldActive)
	{
		return;
	}

	if (bDeleteConfirmVisible || !CanDeleteSelectedSaveSlot())
	{
		ResetDeleteHoldProgress();
		return;
	}

	DeleteHoldElapsedSeconds += InDeltaTime;
	const float HoldProgress = FMath::Clamp(DeleteHoldElapsedSeconds / DeleteHoldDurationSeconds, 0.0f, 1.0f);
	SetDeleteHoldProgress(HoldProgress);

	if (HoldProgress >= 1.0f)
	{
		ExecuteSelectedSaveSlotDelete();
	}
}

void UTunaSweeperIntroMenuWidget::HandleStartClicked()
{
	BeginStartTravel(false);
}

void UTunaSweeperIntroMenuWidget::HandleSlotSelectClicked()
{
	ShowSaveSlotSelection();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsClicked()
{
	ShowSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleCreditsClicked()
{
	ShowCreditsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleQuitClicked()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
}

void UTunaSweeperIntroMenuWidget::HandleAlwaysNewStartClicked()
{
	BeginStartTravel(true);
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyFarmingClicked()
{
	SelectDifficultyStage(1);
	if (DifficultyFarmingButton)
	{
		DifficultyFarmingButton->SetUserFocus(GetOwningPlayer());
	}
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyNormalClicked()
{
	SelectDifficultyStage(2);
	if (DifficultyNormalButton)
	{
		DifficultyNormalButton->SetUserFocus(GetOwningPlayer());
	}
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyHardClicked()
{
	SelectDifficultyStage(3);
	if (DifficultyHardButton)
	{
		DifficultyHardButton->SetUserFocus(GetOwningPlayer());
	}
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyStartClicked()
{
	if (bStartTravelPending || SelectedDifficultyStage == INDEX_NONE)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance ||
		!TunaGameInstance->SetActiveSaveSlotDifficultyStage(SelectedDifficultyStage, true))
	{
		return;
	}

	BeginTravelToLevel(TunaGameInstance->ResolveInitialGameplayLevelName());
}

void UTunaSweeperIntroMenuWidget::HandleDifficultyBackClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::BeginStartTravel(bool bAlwaysNewStart)
{
	if (bStartTravelPending)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	FName TargetLevelName = StartTargetLevelName;
	if (TunaGameInstance)
	{
		const int32 ActiveSaveSlotIndex = TunaGameInstance->GetActiveSaveSlotIndex();
		if (bAlwaysNewStart)
		{
			if (!TunaGameInstance->DeleteSaveSlotAndStartNewGame(ActiveSaveSlotIndex))
			{
				return;
			}
		}
		else
		{
			const FTunaSweeperSaveSlotSummary Summary = TunaGameInstance->GetSaveSlotSummary(ActiveSaveSlotIndex);
			TunaGameInstance->ActivateSaveSlot(ActiveSaveSlotIndex, !Summary.bHasData);
		}
		if (!TunaGameInstance->IsActiveSaveSlotDifficultySelected())
		{
			ShowDifficultySelection();
			return;
		}
		TargetLevelName = TunaGameInstance->ResolveInitialGameplayLevelName();
	}

	BeginTravelToLevel(TargetLevelName);
}

void UTunaSweeperIntroMenuWidget::BeginTravelToLevel(FName TargetLevelName)
{
	if (bStartTravelPending || TargetLevelName.IsNone())
	{
		return;
	}

	bStartTravelPending = true;
	PendingStartTargetLevelName = TargetLevelName;
	SetStartTravelControlsEnabled(false);

	const float FadeDuration = FMath::Max(0.01f, StartTransitionFadeSeconds);
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperBgmSubsystem* BgmSubsystem = GameInstance->GetSubsystem<UTunaSweeperBgmSubsystem>())
		{
			BgmSubsystem->FadeOutAndStop(FadeDuration);
		}
	}

	StartTravelFadeWidget = CreateWidget<UTunaSweeperScreenFadeWidget>(
		GetOwningPlayer(),
		UTunaSweeperScreenFadeWidget::StaticClass());
	if (StartTravelFadeWidget)
	{
		StartTravelFadeWidget->AddToViewport(1000);
		StartTravelFadeWidget->StartFadeToBlack(
			FadeDuration,
			FSimpleDelegate::CreateUObject(this, &UTunaSweeperIntroMenuWidget::OpenPendingStartTargetLevel));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			StartTravelTimerHandle,
			this,
			&UTunaSweeperIntroMenuWidget::OpenPendingStartTargetLevel,
			FadeDuration,
			false);
	}
	else
	{
		OpenPendingStartTargetLevel();
	}
}

void UTunaSweeperIntroMenuWidget::ReloadIntroLevel()
{
	if (bStartTravelPending)
	{
		return;
	}

	bStartTravelPending = true;
	PendingStartTargetLevelName = NAME_None;
	SetStartTravelControlsEnabled(false);
	UGameplayStatics::OpenLevel(this, FName(TEXT("IntroMap")));
}

void UTunaSweeperIntroMenuWidget::HandleSaveSlot1Focused()
{
	SelectSaveSlot(1);
}

void UTunaSweeperIntroMenuWidget::HandleSaveSlot2Focused()
{
	SelectSaveSlot(2);
}

void UTunaSweeperIntroMenuWidget::HandleSaveSlot3Focused()
{
	SelectSaveSlot(3);
}

void UTunaSweeperIntroMenuWidget::HandlePrimarySaveSlotClicked()
{
	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	if (!TunaGameInstance)
	{
		return;
	}

	TunaGameInstance->SetActiveSaveSlotIndex(SelectedSaveSlotIndex);
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotClicked()
{
	HandleDeleteSaveSlotPressed();
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotPressed()
{
	if (!CanDeleteSelectedSaveSlot())
	{
		ResetDeleteHoldProgress();
		return;
	}

	bDeleteHoldActive = true;
	DeleteHoldElapsedSeconds = 0.0f;
	SetDeleteHoldProgress(0.0f);
}

void UTunaSweeperIntroMenuWidget::HandleDeleteSaveSlotReleased()
{
	ResetDeleteHoldProgress();
}

void UTunaSweeperIntroMenuWidget::HandleBackToMainMenuClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleConfirmDeleteClicked()
{
	ExecuteSelectedSaveSlotDelete();
}

void UTunaSweeperIntroMenuWidget::HandleCancelDeleteClicked()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsGraphicsTabClicked()
{
	ShowGraphicsSettingsTab();
}

void UTunaSweeperIntroMenuWidget::HandleSettingsInterfaceTabClicked()
{
	ShowInterfaceSettingsTab();
}

void UTunaSweeperIntroMenuWidget::HandleWindowedModeClicked()
{
	ApplyDisplaySettings(EWindowMode::Windowed);
}

void UTunaSweeperIntroMenuWidget::HandleBorderlessWindowModeClicked()
{
	ApplyDisplaySettings(EWindowMode::WindowedFullscreen);
}

void UTunaSweeperIntroMenuWidget::HandleFullscreenModeClicked()
{
	ApplyDisplaySettings(EWindowMode::Fullscreen);
}

void UTunaSweeperIntroMenuWidget::HandleResolution1280Clicked()
{
	ApplyResolutionSetting(FIntPoint(1280, 720));
}

void UTunaSweeperIntroMenuWidget::HandleResolution1600Clicked()
{
	ApplyResolutionSetting(FIntPoint(1600, 900));
}

void UTunaSweeperIntroMenuWidget::HandleResolution1920Clicked()
{
	ApplyResolutionSetting(FIntPoint(1920, 1080));
}

void UTunaSweeperIntroMenuWidget::HandleResolution2560Clicked()
{
	ApplyResolutionSetting(FIntPoint(2560, 1440));
}

void UTunaSweeperIntroMenuWidget::HandleResolution3840Clicked()
{
	ApplyResolutionSetting(FIntPoint(3840, 2160));
}

void UTunaSweeperIntroMenuWidget::HandleDLSSOffClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Off);
}

void UTunaSweeperIntroMenuWidget::HandleDLSSQualityClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Quality);
}

void UTunaSweeperIntroMenuWidget::HandleDLSSBalancedClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Balanced);
}

void UTunaSweeperIntroMenuWidget::HandleDLSSPerformanceClicked()
{
	ApplyDLSSSetting(ETunaSweeperTitleDLSSMode::Performance);
}

void UTunaSweeperIntroMenuWidget::HandleBackFromSettingsClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageEnglishClicked()
{
	PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::English;
	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageKoreanClicked()
{
	PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::Korean;
	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageJapaneseClicked()
{
	PendingInterfaceLanguage = ETunaSweeperItemTextLanguage::Japanese;
	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::HandleConfirmInterfaceSettingsClicked()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		TunaGameInstance->SetCurrentTextLanguage(PendingInterfaceLanguage, true);
	}

	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleCancelInterfaceSettingsClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleBackFromCreditsClicked()
{
	ShowMainMenu();
}

void UTunaSweeperIntroMenuWidget::HandleLanguageChanged()
{
	bTitleMenuButtonContentLayoutApplied = false;
	ApplyTitleMenuButtonContentLayout();
	RefreshLocalizedTexts();
	RefreshMainMenu();
	if (IsSaveSlotSelectionVisible())
	{
		RefreshSaveSlotMenu();
	}
	if (SettingsPanel && SettingsPanel->GetVisibility() == ESlateVisibility::Visible)
	{
		RefreshSettingsPanel();
	}
	if (IsDifficultySelectionVisible())
	{
		RefreshDifficultySelectionPanel();
	}
}

void UTunaSweeperIntroMenuWidget::ShowMainMenu()
{
	HideOverlayPanels();
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Visible);
	}
	SetTitleLogoVisible(true);
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	SetAlwaysNewStartButtonVisible(true);
	RefreshMainMenu();
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::ShowDifficultySelection()
{
	EnsureDifficultySelectionPanel();
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	HideOverlayPanels();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(false);
	SetAlwaysNewStartButtonVisible(false);

	SelectedDifficultyStage = INDEX_NONE;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		if (TunaGameInstance->IsActiveSaveSlotDifficultySelected())
		{
			SelectedDifficultyStage = FMath::Clamp(TunaGameInstance->GetActiveSaveSlotDifficultyStage(), 1, 3);
		}
	}

	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Visible);
	}

	RefreshDifficultySelectionPanel();
}

void UTunaSweeperIntroMenuWidget::ShowSaveSlotSelection()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	HideOverlayPanels();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Visible);
	}

	SelectedSaveSlotIndex = INDEX_NONE;
	SaveSlotSelectionRingAngle = 0.0f;
	SetAlwaysNewStartButtonVisible(false);
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::ShowSettingsPanel()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(false);
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	EnsureSettingsPanelLayout();
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}

	SetAlwaysNewStartButtonVisible(false);
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		PendingInterfaceLanguage = TunaGameInstance->GetCurrentTextLanguage();
	}
	ShowGraphicsSettingsTab();
}

void UTunaSweeperIntroMenuWidget::ShowGraphicsSettingsTab()
{
	bShowingInterfaceSettingsTab = false;

	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(false);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(true);
	}

	RefreshSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowInterfaceSettingsTab()
{
	bShowingInterfaceSettingsTab = true;

	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		PendingInterfaceLanguage = TunaGameInstance->GetCurrentTextLanguage();
	}
	if (GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	if (SettingsGraphicsTabButton)
	{
		SettingsGraphicsTabButton->SetIsEnabled(true);
	}
	if (SettingsInterfaceTabButton)
	{
		SettingsInterfaceTabButton->SetIsEnabled(false);
	}

	RefreshInterfaceSettingsPanel();
}

void UTunaSweeperIntroMenuWidget::ShowCreditsPanel()
{
	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();

	if (MainMenuPanel)
	{
		MainMenuPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SaveSlotPanel)
	{
		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);
	if (CreditsText)
	{
		CreditsText->SetText(FText::FromString(BuildCreditsColumnText(0)));
	}
	if (CreditsText2)
	{
		CreditsText2->SetText(FText::FromString(BuildCreditsColumnText(1)));
	}
	if (CreditsText3)
	{
		CreditsText3->SetText(FText::FromString(BuildCreditsColumnText(2)));
	}
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Visible);
	}
	SetAlwaysNewStartButtonVisible(false);
	if (CreditsScrollBox)
	{
		CreditsScrollOffset = 0.0f;
		CreditsScrollBox->SetScrollOffset(0.0f);
	}
	if (CreditsScrollBox2)
	{
		CreditsScrollBox2->SetScrollOffset(0.0f);
	}
	if (CreditsScrollBox3)
	{
		CreditsScrollBox3->SetScrollOffset(0.0f);
	}
}

void UTunaSweeperIntroMenuWidget::HideOverlayPanels()
{
	if (DifficultySelectPanel)
	{
		DifficultySelectPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SettingsPanel)
	{
		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (CreditsPanel)
	{
		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetTitleLogoVisible(true);

	CreditsScrollOffset = 0.0f;
}

void UTunaSweeperIntroMenuWidget::SetTitleLogoVisible(bool bVisible)
{
	if (!WidgetTree)
	{
		return;
	}

	if (UWidget* LogoWidget = WidgetTree->FindWidget(FName(TEXT("LogoImage"))))
	{
		LogoWidget->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperIntroMenuWidget::SelectSaveSlot(int32 SaveSlotIndex)
{
	if (SelectedSaveSlotIndex == SaveSlotIndex)
	{
		return;
	}

	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	SelectedSaveSlotIndex = FMath::Clamp(SaveSlotIndex, 1, 3);
	SaveSlotSelectionRingAngle = 0.0f;
	RefreshSaveSlotMenu();
}

void UTunaSweeperIntroMenuWidget::RefreshMainMenu()
{
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(TunaGameInstance->GetActiveSaveSlotIndex());
	}

	if (CurrentSaveSlotText)
	{
		CurrentSaveSlotText->SetText(BuildCurrentSaveSlotText(Summary.SaveSlotIndex));
	}

	if (StartButtonText)
	{
		if (!Summary.bHasData)
		{
			StartButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.title.new_game")),
				FText::FromString(TEXT("\uC0C8\uAC8C\uC784 \uC2DC\uC791"))));
		}
		else if (!Summary.bDifficultySelected)
		{
			StartButtonText->SetText(FText::FromString(TEXT("\uB09C\uC774\uB3C4 \uC120\uD0DD")));
		}
		else
		{
			StartButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.title.continue")),
				FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30"))));
		}
	}
}

void UTunaSweeperIntroMenuWidget::RefreshSaveSlotMenu()
{
	RefreshSaveSlotButton(1, SaveSlot1Button, SaveSlot1Text);
	RefreshSaveSlotButton(2, SaveSlot2Button, SaveSlot2Text);
	RefreshSaveSlotButton(3, SaveSlot3Button, SaveSlot3Text);

	if (SaveSlotActionRow)
	{
		SaveSlotActionRow->SetVisibility(SelectedSaveSlotIndex == INDEX_NONE
			? ESlateVisibility::Collapsed
			: ESlateVisibility::Visible);
	}

	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return;
	}

	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(SelectedSaveSlotIndex);
	}
	else
	{
		Summary.SaveSlotIndex = SelectedSaveSlotIndex;
	}

	if (PrimarySaveSlotButtonText)
	{
		PrimarySaveSlotButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.title.primary_save_slot")),
			FText::FromString(TEXT("\uC138\uC774\uBE0C \uC2AC\uB86F \uC120\uD0DD"))));
	}

	if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->SetIsEnabled(Summary.bHasData);
	}
	if (DeleteSaveSlotButtonBox)
	{
		DeleteSaveSlotButtonBox->SetVisibility(Summary.bHasData
			? ESlateVisibility::Visible
			: ESlateVisibility::Hidden);
	}
	else if (DeleteSaveSlotButton)
	{
		DeleteSaveSlotButton->SetVisibility(Summary.bHasData
			? ESlateVisibility::Visible
			: ESlateVisibility::Hidden);
	}

	if (DeleteSaveSlotButtonText)
	{
		DeleteSaveSlotButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.title.delete_hold")),
			FText::FromString(TEXT("\uAE38\uAC8C \uB20C\uB7EC \uC0AD\uC81C\uD558\uAE30"))));
		DeleteSaveSlotButtonText->SetJustification(ETextJustify::Center);
		DeleteSaveSlotButtonText->SetMargin(FMargin(0.0f));
		DeleteSaveSlotButtonText->SetColorAndOpacity(FSlateColor(Summary.bHasData
			? FLinearColor::White
			: FLinearColor(0.55f, 0.60f, 0.62f, 1.0f)));
	}

	if (!Summary.bHasData)
	{
		ResetDeleteHoldProgress();
	}
}

void UTunaSweeperIntroMenuWidget::EnsureSettingsPanelLayout()
{
	using namespace TunaSweeperSettingsUi;

	if (!WidgetTree || !SettingsPanel)
	{
		return;
	}

	UCanvasPanel* SettingsCanvas = Cast<UCanvasPanel>(SettingsPanel.Get());
	if (!SettingsCanvas)
	{
		return;
	}

	UBorder* SettingsBackdrop = FindOrConstructWidget<UBorder>(WidgetTree, TEXT("SettingsBackdrop"));
	UBorder* SettingsContentBackground = FindOrConstructWidget<UBorder>(WidgetTree, TEXT("SettingsContentBackground"));
	UVerticalBox* SettingsContentStack = FindOrConstructWidget<UVerticalBox>(WidgetTree, TEXT("SettingsContentStack"));
	UTextBlock* SettingsTitleText = FindOrConstructWidget<UTextBlock>(WidgetTree, TEXT("SettingsTitleText"));
	if (!SettingsStatusText)
	{
		SettingsStatusText = FindOrConstructWidget<UTextBlock>(WidgetTree, TEXT("SettingsStatusText"));
	}
	UHorizontalBox* SettingsTabRow = FindOrConstructWidget<UHorizontalBox>(WidgetTree, TEXT("SettingsTabRow"));
	if (!GraphicsSettingsPanel)
	{
		GraphicsSettingsPanel = FindOrConstructWidget<UVerticalBox>(WidgetTree, TEXT("GraphicsSettingsPanel"));
	}
	if (!InterfaceSettingsPanel)
	{
		InterfaceSettingsPanel = FindOrConstructWidget<UVerticalBox>(WidgetTree, TEXT("InterfaceSettingsPanel"));
	}

	UVerticalBox* GraphicsPanel = Cast<UVerticalBox>(GraphicsSettingsPanel.Get());
	UVerticalBox* InterfacePanel = Cast<UVerticalBox>(InterfaceSettingsPanel.Get());
	if (!SettingsBackdrop || !SettingsContentBackground || !SettingsContentStack || !SettingsTitleText ||
		!SettingsStatusText || !SettingsTabRow || !GraphicsPanel || !InterfacePanel)
	{
		return;
	}

	if (SettingsBackdrop->GetParent() != SettingsCanvas)
	{
		SettingsBackdrop->RemoveFromParent();
		SettingsCanvas->AddChildToCanvas(SettingsBackdrop);
	}
	SettingsBackdrop->SetBrush(MakeRoundedBoxBrush(
		FVector2D(1920.0f, 1080.0f),
		FLinearColor(0.006f, 0.010f, 0.012f, 0.58f),
		FLinearColor::Transparent,
		0.0f,
		0.0f));
	ConfigureFillCanvasSlot(SettingsBackdrop, 0);

	if (SettingsContentBackground->GetParent() != SettingsCanvas)
	{
		SettingsContentBackground->RemoveFromParent();
		SettingsCanvas->AddChildToCanvas(SettingsContentBackground);
	}
	SettingsContentBackground->SetPadding(FMargin(32.0f, 28.0f, 32.0f, 24.0f));
	SettingsContentBackground->SetBrush(MakeRoundedBoxBrush(
		FVector2D(PanelWidth, PanelHeight),
		PanelFill,
		PanelOutline,
		1.2f,
		8.0f));
	SettingsContentBackground->SetContent(SettingsContentStack);
	ConfigureCanvasSlot(
		SettingsContentBackground,
		FAnchors(0.0f, 0.5f),
		FVector2D(0.0f, 0.5f),
		FVector2D(PanelLeft, 0.0f),
		FVector2D(PanelWidth, PanelHeight),
		1);

	if (USizeBox* BackButtonBox = FindWidget<USizeBox>(WidgetTree, TEXT("BackFromSettingsButtonBox")))
	{
		BackButtonBox->RemoveFromParent();
		if (BackButtonBox->GetParent() != SettingsCanvas)
		{
			SettingsCanvas->AddChildToCanvas(BackButtonBox);
		}
		BackButtonBox->SetWidthOverride(52.0f);
		BackButtonBox->SetHeightOverride(52.0f);
		ConfigureCanvasSlot(
			BackButtonBox,
			FAnchors(0.0f, 0.0f),
			FVector2D::ZeroVector,
			FVector2D(34.0f, 24.0f),
			FVector2D(52.0f, 52.0f),
			2);
	}

	ConfigureTextBlock(WidgetTree, TEXT("SettingsTitleText"), 32.0f, TextPrimary, ETunaSweeperUIFontWeight::Bold);
	ConfigureTextBlock(WidgetTree, TEXT("SettingsStatusText"), 15.0f, TextMuted);
	ConfigureTextBlock(WidgetTree, TEXT("SettingsGraphicsTabButtonText"), 15.0f, TextPrimary, ETunaSweeperUIFontWeight::Bold, ETextJustify::Center);
	ConfigureTextBlock(WidgetTree, TEXT("SettingsInterfaceTabButtonText"), 15.0f, TextPrimary, ETunaSweeperUIFontWeight::Bold, ETextJustify::Center);
	ConfigureTextBlock(WidgetTree, TEXT("WindowModeLabelText"), 15.0f, TextMuted, ETunaSweeperUIFontWeight::Bold);
	ConfigureTextBlock(WidgetTree, TEXT("ResolutionLabelText"), 15.0f, TextMuted, ETunaSweeperUIFontWeight::Bold);
	ConfigureTextBlock(WidgetTree, TEXT("DLSSLabelText"), 15.0f, TextMuted, ETunaSweeperUIFontWeight::Bold);
	ConfigureTextBlock(WidgetTree, TEXT("LanguageLabelText"), 15.0f, TextMuted, ETunaSweeperUIFontWeight::Bold);

	for (const TCHAR* ButtonTextName : {
		TEXT("WindowedModeButtonText"),
		TEXT("BorderlessWindowModeButtonText"),
		TEXT("FullscreenModeButtonText"),
		TEXT("Resolution1280ButtonText"),
		TEXT("Resolution1600ButtonText"),
		TEXT("Resolution1920ButtonText"),
		TEXT("Resolution2560ButtonText"),
		TEXT("Resolution3840ButtonText"),
		TEXT("DLSSOffButtonText"),
		TEXT("DLSSQualityButtonText"),
		TEXT("DLSSBalancedButtonText"),
		TEXT("DLSSPerformanceButtonText"),
		TEXT("LanguageEnglishButtonText"),
		TEXT("LanguageKoreanButtonText"),
		TEXT("LanguageJapaneseButtonText"),
		TEXT("ConfirmInterfaceSettingsButtonText"),
		TEXT("CancelInterfaceSettingsButtonText"),
		TEXT("BackFromSettingsButtonText")
		})
	{
		ConfigureTextBlock(WidgetTree, ButtonTextName, 15.0f, TextPrimary, ETunaSweeperUIFontWeight::Bold, ETextJustify::Center);
	}

	ConfigureSizeBox(WidgetTree, TEXT("GraphicsTabButtonBox"), 142.0f, 38.0f);
	ConfigureSizeBox(WidgetTree, TEXT("InterfaceTabButtonBox"), 158.0f, 38.0f);
	ConfigureSizeBox(WidgetTree, TEXT("WindowedModeButtonBox"), 160.0f, 44.0f);
	ConfigureSizeBox(WidgetTree, TEXT("BorderlessWindowModeButtonBox"), 236.0f, 44.0f);
	ConfigureSizeBox(WidgetTree, TEXT("FullscreenModeButtonBox"), 184.0f, 44.0f);
	ConfigureSizeBox(WidgetTree, TEXT("Resolution1280ButtonBox"), 660.0f, 42.0f);
	ConfigureSizeBox(WidgetTree, TEXT("Resolution1600ButtonBox"), 660.0f, 42.0f);
	ConfigureSizeBox(WidgetTree, TEXT("Resolution1920ButtonBox"), 660.0f, 42.0f);
	ConfigureSizeBox(WidgetTree, TEXT("Resolution2560ButtonBox"), 660.0f, 42.0f);
	ConfigureSizeBox(WidgetTree, TEXT("Resolution3840ButtonBox"), 660.0f, 42.0f);
	ConfigureSizeBox(WidgetTree, TEXT("DLSSOffButtonBox"), 146.0f, 42.0f);
	ConfigureSizeBox(WidgetTree, TEXT("DLSSQualityButtonBox"), 146.0f, 42.0f);
	ConfigureSizeBox(WidgetTree, TEXT("DLSSBalancedButtonBox"), 146.0f, 42.0f);
	ConfigureSizeBox(WidgetTree, TEXT("DLSSPerformanceButtonBox"), 146.0f, 42.0f);
	ConfigureSizeBox(WidgetTree, TEXT("LanguageEnglishButtonBox"), 660.0f, 46.0f);
	ConfigureSizeBox(WidgetTree, TEXT("LanguageKoreanButtonBox"), 660.0f, 46.0f);
	ConfigureSizeBox(WidgetTree, TEXT("LanguageJapaneseButtonBox"), 660.0f, 46.0f);
	ConfigureSizeBox(WidgetTree, TEXT("ConfirmInterfaceSettingsButtonBox"), 160.0f, 46.0f);
	ConfigureSizeBox(WidgetTree, TEXT("CancelInterfaceSettingsButtonBox"), 160.0f, 46.0f);

	SettingsContentStack->ClearChildren();
	AddVerticalChild(SettingsContentStack, SettingsTitleText, FMargin(0.0f, 0.0f, 0.0f, 8.0f));
	AddVerticalChild(SettingsContentStack, SettingsStatusText, FMargin(0.0f, 0.0f, 0.0f, 16.0f));

	SettingsTabRow->ClearChildren();
	AddHorizontalChild(SettingsTabRow, FindWidget<USizeBox>(WidgetTree, TEXT("GraphicsTabButtonBox")), FMargin(0.0f, 0.0f, 8.0f, 0.0f));
	AddHorizontalChild(SettingsTabRow, FindWidget<USizeBox>(WidgetTree, TEXT("InterfaceTabButtonBox")), FMargin(0.0f));
	AddVerticalChild(SettingsContentStack, SettingsTabRow, FMargin(0.0f, 0.0f, 0.0f, 12.0f));

	UHorizontalBox* WindowModeRow = FindOrConstructWidget<UHorizontalBox>(WidgetTree, TEXT("WindowModeRow"));
	UVerticalBox* ResolutionButtonStack = FindOrConstructWidget<UVerticalBox>(WidgetTree, TEXT("ResolutionButtonStack"));
	UHorizontalBox* DLSSButtonRow = FindOrConstructWidget<UHorizontalBox>(WidgetTree, TEXT("DLSSButtonRow"));
	if (WindowModeRow && ResolutionButtonStack && DLSSButtonRow)
	{
		WindowModeRow->ClearChildren();
		AddHorizontalChild(WindowModeRow, FindWidget<USizeBox>(WidgetTree, TEXT("WindowedModeButtonBox")), FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		AddHorizontalChild(WindowModeRow, FindWidget<USizeBox>(WidgetTree, TEXT("BorderlessWindowModeButtonBox")), FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		AddHorizontalChild(WindowModeRow, FindWidget<USizeBox>(WidgetTree, TEXT("FullscreenModeButtonBox")), FMargin(0.0f));

		ResolutionButtonStack->ClearChildren();
		AddVerticalChild(ResolutionButtonStack, FindWidget<USizeBox>(WidgetTree, TEXT("Resolution1280ButtonBox")), FMargin(0.0f, 0.0f, 0.0f, 5.0f));
		AddVerticalChild(ResolutionButtonStack, FindWidget<USizeBox>(WidgetTree, TEXT("Resolution1600ButtonBox")), FMargin(0.0f, 0.0f, 0.0f, 5.0f));
		AddVerticalChild(ResolutionButtonStack, FindWidget<USizeBox>(WidgetTree, TEXT("Resolution1920ButtonBox")), FMargin(0.0f, 0.0f, 0.0f, 5.0f));
		AddVerticalChild(ResolutionButtonStack, FindWidget<USizeBox>(WidgetTree, TEXT("Resolution2560ButtonBox")), FMargin(0.0f, 0.0f, 0.0f, 5.0f));
		AddVerticalChild(ResolutionButtonStack, FindWidget<USizeBox>(WidgetTree, TEXT("Resolution3840ButtonBox")), FMargin(0.0f));

		DLSSButtonRow->ClearChildren();
		AddHorizontalChild(DLSSButtonRow, FindWidget<USizeBox>(WidgetTree, TEXT("DLSSOffButtonBox")), FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		AddHorizontalChild(DLSSButtonRow, FindWidget<USizeBox>(WidgetTree, TEXT("DLSSQualityButtonBox")), FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		AddHorizontalChild(DLSSButtonRow, FindWidget<USizeBox>(WidgetTree, TEXT("DLSSBalancedButtonBox")), FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		AddHorizontalChild(DLSSButtonRow, FindWidget<USizeBox>(WidgetTree, TEXT("DLSSPerformanceButtonBox")), FMargin(0.0f));

		GraphicsPanel->ClearChildren();
		BuildSettingsSection(
			WidgetTree,
			GraphicsPanel,
			TEXT("SettingsWindowModeSection"),
			TEXT("SettingsWindowModeSectionStack"),
			FindWidget<UTextBlock>(WidgetTree, TEXT("WindowModeLabelText")),
			WindowModeRow,
			FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		BuildSettingsSection(
			WidgetTree,
			GraphicsPanel,
			TEXT("SettingsResolutionSection"),
			TEXT("SettingsResolutionSectionStack"),
			FindWidget<UTextBlock>(WidgetTree, TEXT("ResolutionLabelText")),
			ResolutionButtonStack,
			FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		BuildSettingsSection(
			WidgetTree,
			GraphicsPanel,
			TEXT("SettingsDLSSSection"),
			TEXT("SettingsDLSSSectionStack"),
			FindWidget<UTextBlock>(WidgetTree, TEXT("DLSSLabelText")),
			DLSSButtonRow,
			FMargin(0.0f));
	}

	UVerticalBox* LanguageButtonStack = FindOrConstructWidget<UVerticalBox>(WidgetTree, TEXT("LanguageButtonStack"));
	UHorizontalBox* InterfaceActionButtonRow = FindOrConstructWidget<UHorizontalBox>(WidgetTree, TEXT("InterfaceActionButtonRow"));
	if (LanguageButtonStack && InterfaceActionButtonRow)
	{
		LanguageButtonStack->ClearChildren();
		AddVerticalChild(LanguageButtonStack, FindWidget<USizeBox>(WidgetTree, TEXT("LanguageEnglishButtonBox")), FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		AddVerticalChild(LanguageButtonStack, FindWidget<USizeBox>(WidgetTree, TEXT("LanguageKoreanButtonBox")), FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		AddVerticalChild(LanguageButtonStack, FindWidget<USizeBox>(WidgetTree, TEXT("LanguageJapaneseButtonBox")), FMargin(0.0f));

		InterfaceActionButtonRow->ClearChildren();
		AddHorizontalChild(InterfaceActionButtonRow, FindWidget<USizeBox>(WidgetTree, TEXT("CancelInterfaceSettingsButtonBox")), FMargin(0.0f, 0.0f, 10.0f, 0.0f));
		AddHorizontalChild(InterfaceActionButtonRow, FindWidget<USizeBox>(WidgetTree, TEXT("ConfirmInterfaceSettingsButtonBox")), FMargin(0.0f));

		InterfacePanel->ClearChildren();
		BuildSettingsSection(
			WidgetTree,
			InterfacePanel,
			TEXT("SettingsLanguageSection"),
			TEXT("SettingsLanguageSectionStack"),
			FindWidget<UTextBlock>(WidgetTree, TEXT("LanguageLabelText")),
			LanguageButtonStack,
			FMargin(0.0f, 0.0f, 0.0f, 18.0f));
		AddVerticalChild(InterfacePanel, InterfaceActionButtonRow, FMargin(0.0f));
	}

	AddVerticalChild(SettingsContentStack, GraphicsPanel, FMargin(0.0f));
	AddVerticalChild(SettingsContentStack, InterfacePanel, FMargin(0.0f));

	GraphicsPanel->SetVisibility(bShowingInterfaceSettingsTab ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
	InterfacePanel->SetVisibility(bShowingInterfaceSettingsTab ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

	ApplySettingsTabButtonStyle(SettingsGraphicsTabButton, FVector2D(142.0f, 38.0f), !bShowingInterfaceSettingsTab);
	ApplySettingsTabButtonStyle(SettingsInterfaceTabButton, FVector2D(158.0f, 38.0f), bShowingInterfaceSettingsTab);
	ApplySettingsChoiceButtonStyle(BackFromSettingsButton, FVector2D(52.0f, 52.0f), false);
	InvalidateLayoutAndVolatility();
}

void UTunaSweeperIntroMenuWidget::RefreshSettingsPanel()
{
	EnsureSettingsPanelLayout();

	if (bShowingInterfaceSettingsTab)
	{
		RefreshInterfaceSettingsPanel();
		return;
	}

	FIntPoint CurrentResolution(0, 0);
	EWindowMode::Type CurrentWindowMode = EWindowMode::Windowed;
	if (GEngine)
	{
		if (UGameUserSettings* GameUserSettings = GEngine->GetGameUserSettings())
		{
			CurrentResolution = GameUserSettings->GetScreenResolution();
			CurrentWindowMode = GameUserSettings->GetFullscreenMode();
		}
	}

	if (SettingsStatusText)
	{
		const bool bDLSSSupported = UDLSSLibrary::IsDLSSSupported();
		const FText DLSSStatusText = bDLSSSupported
			? BuildDLSSModeText(PreferredDLSSMode)
			: ResolveUiText(
				FName(TEXT("ui.settings.dlss.unavailable")),
				FText::FromString(TEXT("\uC0AC\uC6A9 \uBD88\uAC00")));
		SettingsStatusText->SetText(FText::Format(
			ResolveUiText(
				FName(TEXT("ui.settings.current_graphics")),
				FText::FromString(TEXT("\uD604\uC7AC: {0} / {1}x{2} / DLSS {3}"))),
			BuildWindowModeText(CurrentWindowMode),
			FText::AsNumber(CurrentResolution.X),
			FText::AsNumber(CurrentResolution.Y),
			DLSSStatusText));
	}

	SetNamedText(
		FName(TEXT("Resolution1280ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(1280, 720) ? TEXT("\u2713 1280 x 720") : TEXT("1280 x 720")));
	SetNamedText(
		FName(TEXT("Resolution1600ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(1600, 900) ? TEXT("\u2713 1600 x 900") : TEXT("1600 x 900")));
	SetNamedText(
		FName(TEXT("Resolution1920ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(1920, 1080) ? TEXT("\u2713 1920 x 1080") : TEXT("1920 x 1080")));
	SetNamedText(
		FName(TEXT("Resolution2560ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(2560, 1440) ? TEXT("\u2713 2560 x 1440") : TEXT("2560 x 1440")));
	SetNamedText(
		FName(TEXT("Resolution3840ButtonText")),
		FText::FromString(CurrentResolution == FIntPoint(3840, 2160) ? TEXT("\u2713 3840 x 2160") : TEXT("3840 x 2160")));

	if (DLSSOffButton)
	{
		DLSSOffButton->SetIsEnabled(true);
	}
	if (DLSSQualityButton)
	{
		DLSSQualityButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Quality));
	}
	if (DLSSBalancedButton)
	{
		DLSSBalancedButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Balanced));
	}
	if (DLSSPerformanceButton)
	{
		DLSSPerformanceButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Performance));
	}

	RefreshSettingsSelectionStyles(CurrentResolution, CurrentWindowMode);
}

void UTunaSweeperIntroMenuWidget::RefreshInterfaceSettingsPanel()
{
	EnsureSettingsPanelLayout();

	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	const ETunaSweeperItemTextLanguage CurrentLanguage = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	if (SettingsStatusText)
	{
		SettingsStatusText->SetText(FText::Format(
			ResolveUiText(
				FName(TEXT("ui.settings.current_language")),
				FText::FromString(TEXT("\uD604\uC7AC \uC5B8\uC5B4: {0}"))),
			BuildLanguageNameText(CurrentLanguage)));
	}

	if (LanguageEnglishButtonText)
	{
		LanguageEnglishButtonText->SetText(BuildLanguageOptionText(
			ETunaSweeperItemTextLanguage::English,
			PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::English));
	}
	if (LanguageKoreanButtonText)
	{
		LanguageKoreanButtonText->SetText(BuildLanguageOptionText(
			ETunaSweeperItemTextLanguage::Korean,
			PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Korean));
	}
	if (LanguageJapaneseButtonText)
	{
		LanguageJapaneseButtonText->SetText(BuildLanguageOptionText(
			ETunaSweeperItemTextLanguage::Japanese,
			PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Japanese));
	}

	if (LanguageEnglishButton)
	{
		LanguageEnglishButton->SetIsEnabled(true);
	}
	if (LanguageKoreanButton)
	{
		LanguageKoreanButton->SetIsEnabled(true);
	}
	if (LanguageJapaneseButton)
	{
		LanguageJapaneseButton->SetIsEnabled(true);
	}
	if (ConfirmInterfaceSettingsButton)
	{
		ConfirmInterfaceSettingsButton->SetIsEnabled(true);
	}
	if (CancelInterfaceSettingsButton)
	{
		CancelInterfaceSettingsButton->SetIsEnabled(true);
	}

	RefreshInterfaceSelectionStyles();
}

void UTunaSweeperIntroMenuWidget::RefreshSettingsSelectionStyles(
	const FIntPoint& CurrentResolution,
	EWindowMode::Type CurrentWindowMode)
{
	ApplySettingsTabButtonStyle(SettingsGraphicsTabButton, FVector2D(142.0f, 38.0f), true);
	ApplySettingsTabButtonStyle(SettingsInterfaceTabButton, FVector2D(158.0f, 38.0f), false);

	ApplySettingsChoiceButtonStyle(
		WindowedModeButton,
		FVector2D(160.0f, 44.0f),
		CurrentWindowMode == EWindowMode::Windowed);
	ApplySettingsChoiceButtonStyle(
		BorderlessWindowModeButton,
		FVector2D(236.0f, 44.0f),
		CurrentWindowMode == EWindowMode::WindowedFullscreen);
	ApplySettingsChoiceButtonStyle(
		FullscreenModeButton,
		FVector2D(184.0f, 44.0f),
		CurrentWindowMode == EWindowMode::Fullscreen);

	ApplySettingsChoiceButtonStyle(
		Resolution1280Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(1280, 720));
	ApplySettingsChoiceButtonStyle(
		Resolution1600Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(1600, 900));
	ApplySettingsChoiceButtonStyle(
		Resolution1920Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(1920, 1080));
	ApplySettingsChoiceButtonStyle(
		Resolution2560Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(2560, 1440));
	ApplySettingsChoiceButtonStyle(
		Resolution3840Button,
		FVector2D(660.0f, 42.0f),
		CurrentResolution == FIntPoint(3840, 2160));

	ApplySettingsChoiceButtonStyle(
		DLSSOffButton,
		FVector2D(146.0f, 42.0f),
		PreferredDLSSMode == ETunaSweeperTitleDLSSMode::Off);
	ApplySettingsChoiceButtonStyle(
		DLSSQualityButton,
		FVector2D(146.0f, 42.0f),
		PreferredDLSSMode == ETunaSweeperTitleDLSSMode::Quality);
	ApplySettingsChoiceButtonStyle(
		DLSSBalancedButton,
		FVector2D(146.0f, 42.0f),
		PreferredDLSSMode == ETunaSweeperTitleDLSSMode::Balanced);
	ApplySettingsChoiceButtonStyle(
		DLSSPerformanceButton,
		FVector2D(146.0f, 42.0f),
		PreferredDLSSMode == ETunaSweeperTitleDLSSMode::Performance);
}

void UTunaSweeperIntroMenuWidget::RefreshInterfaceSelectionStyles()
{
	ApplySettingsTabButtonStyle(SettingsGraphicsTabButton, FVector2D(142.0f, 38.0f), false);
	ApplySettingsTabButtonStyle(SettingsInterfaceTabButton, FVector2D(158.0f, 38.0f), true);

	ApplySettingsChoiceButtonStyle(
		LanguageEnglishButton,
		FVector2D(660.0f, 46.0f),
		PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::English);
	ApplySettingsChoiceButtonStyle(
		LanguageKoreanButton,
		FVector2D(660.0f, 46.0f),
		PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Korean);
	ApplySettingsChoiceButtonStyle(
		LanguageJapaneseButton,
		FVector2D(660.0f, 46.0f),
		PendingInterfaceLanguage == ETunaSweeperItemTextLanguage::Japanese);
	ApplySettingsChoiceButtonStyle(
		CancelInterfaceSettingsButton,
		FVector2D(160.0f, 46.0f),
		false);
	ApplySettingsChoiceButtonStyle(
		ConfirmInterfaceSettingsButton,
		FVector2D(160.0f, 46.0f),
		false,
		true);
}

void UTunaSweeperIntroMenuWidget::ApplySettingsChoiceButtonStyle(
	UButton* Button,
	const FVector2D& ButtonSize,
	bool bSelected,
	bool bPrimary) const
{
	if (!Button)
	{
		return;
	}

	using namespace TunaSweeperSettingsUi;

	const FLinearColor NormalFill = bSelected
		? FLinearColor(0.04f, 0.25f, 0.28f, 0.92f)
		: (bPrimary ? FLinearColor(0.05f, 0.34f, 0.38f, 0.92f) : FLinearColor(0.022f, 0.034f, 0.040f, 0.80f));
	const FLinearColor HoveredFill = bSelected
		? FLinearColor(0.06f, 0.36f, 0.40f, 0.98f)
		: (bPrimary ? FLinearColor(0.07f, 0.44f, 0.48f, 0.98f) : FLinearColor(0.045f, 0.075f, 0.085f, 0.92f));
	const FLinearColor PressedFill = NormalFill * 0.78f;
	const FLinearColor Outline = bSelected || bPrimary
		? Accent
		: FLinearColor(0.56f, 0.66f, 0.66f, 0.70f);

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(MakeRoundedBoxBrush(
		ButtonSize,
		NormalFill,
		Outline,
		bSelected || bPrimary ? 1.8f : 1.0f,
		ButtonCornerRadius));
	ButtonStyle.SetHovered(MakeRoundedBoxBrush(
		ButtonSize,
		HoveredFill,
		FLinearColor(0.82f, 0.98f, 1.0f, 1.0f),
		bSelected || bPrimary ? 2.2f : 1.4f,
		ButtonCornerRadius));
	ButtonStyle.SetPressed(MakeRoundedBoxBrush(
		ButtonSize,
		PressedFill,
		Outline * 0.84f,
		1.0f,
		ButtonCornerRadius));
	ButtonStyle.SetDisabled(MakeRoundedBoxBrush(
		ButtonSize,
		bSelected ? NormalFill : FLinearColor(0.018f, 0.024f, 0.028f, 0.58f),
		bSelected ? Outline : FLinearColor(0.30f, 0.36f, 0.36f, 0.42f),
		bSelected ? 1.6f : 0.8f,
		ButtonCornerRadius));
	ButtonStyle.SetNormalPadding(FMargin(0.0f));
	ButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));

	Button->SetStyle(ButtonStyle);
	Button->SetClickMethod(EButtonClickMethod::DownAndUp);
}

void UTunaSweeperIntroMenuWidget::ApplySettingsTabButtonStyle(
	UButton* Button,
	const FVector2D& ButtonSize,
	bool bSelected) const
{
	if (!Button)
	{
		return;
	}

	using namespace TunaSweeperSettingsUi;

	const FLinearColor Fill = bSelected
		? FLinearColor(0.035f, 0.19f, 0.21f, 0.92f)
		: FLinearColor(0.018f, 0.030f, 0.036f, 0.78f);
	const FLinearColor HoveredFill = bSelected
		? FLinearColor(0.05f, 0.28f, 0.31f, 0.98f)
		: FLinearColor(0.035f, 0.070f, 0.080f, 0.94f);
	const FLinearColor Outline = bSelected
		? Accent
		: FLinearColor(0.46f, 0.56f, 0.56f, 0.68f);

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(MakeRoundedBoxBrush(
		ButtonSize,
		Fill,
		Outline,
		bSelected ? 1.8f : 1.0f,
		TunaSweeperSettingsUi::ButtonCornerRadius));
	ButtonStyle.SetHovered(MakeRoundedBoxBrush(
		ButtonSize,
		HoveredFill,
		FLinearColor(0.78f, 0.98f, 1.0f, 1.0f),
		2.0f,
		TunaSweeperSettingsUi::ButtonCornerRadius));
	ButtonStyle.SetPressed(MakeRoundedBoxBrush(
		ButtonSize,
		Fill * 0.78f,
		Outline,
		1.0f,
		TunaSweeperSettingsUi::ButtonCornerRadius));
	ButtonStyle.SetDisabled(MakeRoundedBoxBrush(
		ButtonSize,
		Fill,
		Outline,
		bSelected ? 1.8f : 1.0f,
		TunaSweeperSettingsUi::ButtonCornerRadius));
	ButtonStyle.SetNormalPadding(FMargin(0.0f));
	ButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));

	Button->SetStyle(ButtonStyle);
	Button->SetClickMethod(EButtonClickMethod::DownAndUp);
}

void UTunaSweeperIntroMenuWidget::RefreshLocalizedTexts()
{
	SetNamedText(
		FName(TEXT("SaveSlotPanelTitleText")),
		ResolveUiText(FName(TEXT("ui.title.slot_select")), FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD"))));
	SetNamedText(
		FName(TEXT("BackToMainMenuButtonText")),
		ResolveUiText(FName(TEXT("ui.common.back")), FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30"))));
	SetNamedText(
		FName(TEXT("DeleteConfirmTitleText")),
		ResolveUiText(FName(TEXT("ui.title.delete_confirm_title")), FText::FromString(TEXT("\uC2AC\uB86F \uC0AD\uC81C"))));
	SetNamedText(
		FName(TEXT("DeleteConfirmMessageText")),
		ResolveUiText(FName(TEXT("ui.title.delete_confirm_message")), FText::FromString(TEXT("\uC120\uD0DD\uD55C \uC800\uC7A5 \uB370\uC774\uD130\uB97C \uC0AD\uC81C\uD560\uAE4C\uC694?"))));
	SetNamedText(
		FName(TEXT("ConfirmDeleteButtonText")),
		ResolveUiText(FName(TEXT("ui.common.delete")), FText::FromString(TEXT("\uC0AD\uC81C\uD558\uAE30"))));
	SetNamedText(
		FName(TEXT("CancelDeleteButtonText")),
		ResolveUiText(FName(TEXT("ui.common.cancel")), FText::FromString(TEXT("\uCDE8\uC18C"))));
	SetNamedText(
		FName(TEXT("SettingsTitleText")),
		ResolveUiText(FName(TEXT("ui.title.settings")), FText::FromString(TEXT("\uC124\uC815"))));
	SetNamedText(
		FName(TEXT("SettingsGraphicsTabButtonText")),
		ResolveUiText(FName(TEXT("ui.settings.graphics")), FText::FromString(TEXT("\uADF8\uB798\uD53D"))));
	SetNamedText(
		FName(TEXT("SettingsInterfaceTabButtonText")),
		ResolveUiText(FName(TEXT("ui.settings.interface")), FText::FromString(TEXT("\uC778\uD130\uD398\uC774\uC2A4"))));
	SetNamedText(
		FName(TEXT("WindowModeLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.window_mode")), FText::FromString(TEXT("\uD654\uBA74 \uBAA8\uB4DC"))));
	SetNamedText(
		FName(TEXT("WindowedModeButtonText")),
		BuildWindowModeText(EWindowMode::Windowed));
	SetNamedText(
		FName(TEXT("BorderlessWindowModeButtonText")),
		BuildWindowModeText(EWindowMode::WindowedFullscreen));
	SetNamedText(
		FName(TEXT("FullscreenModeButtonText")),
		BuildWindowModeText(EWindowMode::Fullscreen));
	SetNamedText(
		FName(TEXT("ResolutionLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.resolution")), FText::FromString(TEXT("\uD574\uC0C1\uB3C4"))));
	SetNamedText(
		FName(TEXT("DLSSLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.dlss")), FText::FromString(TEXT("DLSS"))));
	SetNamedText(
		FName(TEXT("DLSSOffButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Off));
	SetNamedText(
		FName(TEXT("DLSSQualityButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Quality));
	SetNamedText(
		FName(TEXT("DLSSBalancedButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Balanced));
	SetNamedText(
		FName(TEXT("DLSSPerformanceButtonText")),
		BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Performance));
	SetNamedText(
		FName(TEXT("LanguageLabelText")),
		ResolveUiText(FName(TEXT("ui.settings.language")), FText::FromString(TEXT("\uC5B8\uC5B4"))));
	SetNamedText(
		FName(TEXT("ConfirmInterfaceSettingsButtonText")),
		ResolveUiText(FName(TEXT("ui.common.confirm")), FText::FromString(TEXT("\uACB0\uC815"))));
	SetNamedText(
		FName(TEXT("CancelInterfaceSettingsButtonText")),
		ResolveUiText(FName(TEXT("ui.common.cancel")), FText::FromString(TEXT("\uCDE8\uC18C"))));
	SetNamedText(
		FName(TEXT("CreditsTitleText")),
		ResolveUiText(FName(TEXT("ui.title.credits")), FText::FromString(TEXT("\uD06C\uB808\uB527"))));
	SetNamedText(
		FName(TEXT("BackFromCreditsButtonText")),
		ResolveUiText(FName(TEXT("ui.common.back")), FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30"))));

	if (AlwaysNewStartButtonText)
	{
		AlwaysNewStartButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.title.always_new_start")),
			FText::FromString(TEXT("\uD56D\uC0C1\uC0C8\uB85C\uC2DC\uC791"))));
	}
	if (DifficultyTitleText)
	{
		DifficultyTitleText->SetText(FText::FromString(TEXT("\uB09C\uC774\uB3C4 \uC120\uD0DD")));
	}
	if (DifficultyStartButtonText)
	{
		DifficultyStartButtonText->SetText(FText::FromString(TEXT("\uAC8C\uC784 \uC2DC\uC791")));
	}
	if (DifficultyBackButtonText)
	{
		DifficultyBackButtonText->SetText(ResolveUiText(
			FName(TEXT("ui.common.back")),
			FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30"))));
	}
}

void UTunaSweeperIntroMenuWidget::RefreshSaveSlotButton(int32 SaveSlotIndex, UButton* SlotButton, UTextBlock* SlotText)
{
	const bool bSelected = SaveSlotIndex == SelectedSaveSlotIndex;

	if (SlotText)
	{
		SlotText->SetText(BuildSaveSlotButtonText(SaveSlotIndex));
		SlotText->SetJustification(ETextJustify::Center);
		SlotText->SetMargin(FMargin(0.0f));
		SlotText->SetColorAndOpacity(FSlateColor(bSelected
			? FLinearColor::White
			: FLinearColor(0.74f, 0.80f, 0.84f, 1.0f)));
	}

	if (SlotButton)
	{
		SlotButton->SetIsEnabled(true);
		ApplySaveSlotButtonStyle(SlotButton, bSelected);
	}

	UImage* RingImage = nullptr;
	switch (SaveSlotIndex)
	{
	case 1:
		RingImage = GeneratedSaveSlot1SelectionRingImage;
		break;
	case 2:
		RingImage = GeneratedSaveSlot2SelectionRingImage;
		break;
	case 3:
		RingImage = GeneratedSaveSlot3SelectionRingImage;
		break;
	default:
		break;
	}

	SetSaveSlotSelectionRingSelected(RingImage, bSelected);
}

void UTunaSweeperIntroMenuWidget::ApplySaveSlotButtonStyle(UButton* SlotButton, bool bSelected)
{
	if (!SlotButton)
	{
		return;
	}

	const FVector2D ButtonSize(700.0f, 112.0f);
	const float CornerRadius = 11.0f;
	const FLinearColor NormalFill(0.025f, 0.045f, 0.050f, 0.56f);
	const FLinearColor HoveredFill(0.055f, 0.095f, 0.105f, 0.76f);
	const FLinearColor PressedFill = NormalFill * 0.75f;
	const FLinearColor NormalOutline = bSelected
		? FLinearColor(0.98f, 1.0f, 0.92f, 1.0f)
		: FLinearColor(0.78f, 0.84f, 0.82f, 0.88f);
	const FLinearColor HoveredOutline = bSelected
		? FLinearColor(1.0f, 1.0f, 0.96f, 1.0f)
		: FLinearColor(0.96f, 0.98f, 0.95f, 1.0f);
	const FLinearColor PressedOutline = bSelected
		? FLinearColor(0.94f, 0.98f, 0.88f, 1.0f)
		: FLinearColor(0.60f, 0.68f, 0.68f, 0.90f);

	auto MakeSlotBrush = [ButtonSize, CornerRadius](const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ButtonSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(CornerRadius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	};

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(MakeSlotBrush(NormalFill, NormalOutline, bSelected ? 2.8f : 1.3f));
	ButtonStyle.SetHovered(MakeSlotBrush(HoveredFill, HoveredOutline, bSelected ? 3.2f : 1.7f));
	ButtonStyle.SetPressed(MakeSlotBrush(PressedFill, PressedOutline, bSelected ? 2.2f : 1.0f));
	ButtonStyle.SetNormalPadding(FMargin(0.0f));
	ButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));
	SlotButton->SetStyle(ButtonStyle);
	SlotButton->SetClickMethod(EButtonClickMethod::DownAndUp);
}

void UTunaSweeperIntroMenuWidget::EnsureSaveSlotSelectionRingWidgets()
{
	EnsureSaveSlotSelectionRingContent(
		SaveSlot1Button,
		SaveSlot1Text,
		GeneratedSaveSlot1SelectionRingImage,
		TEXT("SaveSlot1Content"),
		TEXT("SaveSlot1SelectionRingImage"));
	EnsureSaveSlotSelectionRingContent(
		SaveSlot2Button,
		SaveSlot2Text,
		GeneratedSaveSlot2SelectionRingImage,
		TEXT("SaveSlot2Content"),
		TEXT("SaveSlot2SelectionRingImage"));
	EnsureSaveSlotSelectionRingContent(
		SaveSlot3Button,
		SaveSlot3Text,
		GeneratedSaveSlot3SelectionRingImage,
		TEXT("SaveSlot3Content"),
		TEXT("SaveSlot3SelectionRingImage"));
}

void UTunaSweeperIntroMenuWidget::EnsureSaveSlotSelectionRingContent(
	UButton* SlotButton,
	UTextBlock* SlotText,
	TObjectPtr<UImage>& RingImage,
	const TCHAR* ContentWidgetName,
	const TCHAR* RingWidgetName)
{
	if (!WidgetTree || !SlotButton || !SlotText)
	{
		return;
	}

	const FName ContentName(ContentWidgetName);
	const FName RingName(RingWidgetName);
	UOverlay* ContentOverlay = Cast<UOverlay>(WidgetTree->FindWidget(ContentName));
	if (!ContentOverlay)
	{
		ContentOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), ContentName);
	}
	if (!ContentOverlay)
	{
		return;
	}

	ContentOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ContentOverlay->SetClipping(EWidgetClipping::ClipToBounds);

	if (!RingImage)
	{
		RingImage = Cast<UImage>(WidgetTree->FindWidget(RingName));
	}
	if (!RingImage)
	{
		RingImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), RingName);
	}
	if (RingImage)
	{
		ConfigureSaveSlotSelectionRingImage(RingImage);
		if (RingImage->GetParent() != ContentOverlay)
		{
			RingImage->RemoveFromParent();
			UOverlaySlot* RingSlot = ContentOverlay->AddChildToOverlay(RingImage);
			if (RingSlot)
			{
				RingSlot->SetHorizontalAlignment(HAlign_Right);
				RingSlot->SetVerticalAlignment(VAlign_Center);
				RingSlot->SetPadding(FMargin(0.0f, 0.0f, 28.0f, 0.0f));
			}
		}
	}

	if (SlotButton->GetContent() != ContentOverlay)
	{
		SlotButton->SetContent(ContentOverlay);
	}

	SlotText->RemoveFromParent();
	UOverlaySlot* TextSlot = ContentOverlay->AddChildToOverlay(SlotText);
	if (TextSlot)
	{
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(56.0f, 0.0f));
	}
	SlotText->SetJustification(ETextJustify::Center);
	SlotText->SetMargin(FMargin(0.0f));
}

void UTunaSweeperIntroMenuWidget::ConfigureSaveSlotSelectionRingImage(UImage* RingImage)
{
	if (!RingImage)
	{
		return;
	}

	FSlateBrush RingBrush;
	RingBrush.DrawAs = ESlateBrushDrawType::Image;
	RingBrush.SetResourceObject(GetOrCreateSaveSlotSelectionRingTexture());
	RingBrush.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.95f));
	RingBrush.SetImageSize(FVector2D(28.0f, 28.0f));

	RingImage->SetBrush(RingBrush);
	RingImage->SetVisibility(ESlateVisibility::Collapsed);
	RingImage->SetRenderOpacity(0.0f);
	RingImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
}

UTexture2D* UTunaSweeperIntroMenuWidget::GetOrCreateSaveSlotSelectionRingTexture()
{
	if (SaveSlotSelectionRingTexture)
	{
		return SaveSlotSelectionRingTexture;
	}

	constexpr int32 TextureSize = 32;
	constexpr float InnerRadius = 9.5f;
	constexpr float OuterRadius = 13.5f;
	constexpr float GapStart = 0.72f;
	constexpr float GapEnd = 0.96f;
	const FVector2D Center((TextureSize - 1) * 0.5f, (TextureSize - 1) * 0.5f);

	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(TextureSize * TextureSize);

	for (int32 Y = 0; Y < TextureSize; ++Y)
	{
		for (int32 X = 0; X < TextureSize; ++X)
		{
			const FVector2D Delta(static_cast<float>(X) - Center.X, static_cast<float>(Y) - Center.Y);
			const float Radius = Delta.Size();
			if (Radius < InnerRadius || Radius > OuterRadius)
			{
				continue;
			}

			float Angle = FMath::Atan2(Delta.Y, Delta.X) / (2.0f * PI);
			if (Angle < 0.0f)
			{
				Angle += 1.0f;
			}
			if (Angle >= GapStart && Angle <= GapEnd)
			{
				continue;
			}

			const float RingEdgeAlpha = FMath::Clamp(
				FMath::Min(Radius - InnerRadius, OuterRadius - Radius) / 1.35f,
				0.0f,
				1.0f);
			const float TailAlpha = Angle < GapStart
				? FMath::Lerp(0.45f, 1.0f, Angle / GapStart)
				: 0.85f;
			const uint8 Alpha = static_cast<uint8>(FMath::RoundToInt(220.0f * RingEdgeAlpha * TailAlpha));
			Pixels[Y * TextureSize + X] = FColor(236, 255, 221, Alpha);
		}
	}

	SaveSlotSelectionRingTexture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8);
	if (!SaveSlotSelectionRingTexture)
	{
		return nullptr;
	}

	SaveSlotSelectionRingTexture->NeverStream = true;
	SaveSlotSelectionRingTexture->SRGB = true;

	FTexturePlatformData* PlatformData = SaveSlotSelectionRingTexture->GetPlatformData();
	if (!PlatformData || PlatformData->Mips.Num() == 0)
	{
		SaveSlotSelectionRingTexture->UpdateResource();
		return SaveSlotSelectionRingTexture;
	}

	void* TextureData = PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	PlatformData->Mips[0].BulkData.Unlock();
	SaveSlotSelectionRingTexture->UpdateResource();

	return SaveSlotSelectionRingTexture;
}

void UTunaSweeperIntroMenuWidget::UpdateSaveSlotSelectionRingAnimation(float InDeltaTime)
{
	if (SelectedSaveSlotIndex == INDEX_NONE || !IsSaveSlotSelectionVisible())
	{
		SetSaveSlotSelectionRingSelected(GeneratedSaveSlot1SelectionRingImage, false);
		SetSaveSlotSelectionRingSelected(GeneratedSaveSlot2SelectionRingImage, false);
		SetSaveSlotSelectionRingSelected(GeneratedSaveSlot3SelectionRingImage, false);
		return;
	}

	SaveSlotSelectionRingAngle = FMath::Fmod(SaveSlotSelectionRingAngle + InDeltaTime * 140.0f, 360.0f);
	const float RingOpacity = 0.82f + 0.08f * FMath::Sin(FMath::DegreesToRadians(SaveSlotSelectionRingAngle));

	auto ApplyRingTransform = [this, RingOpacity](UImage* RingImage, bool bSelected)
	{
		if (!RingImage)
		{
			return;
		}

		if (!bSelected)
		{
			RingImage->SetVisibility(ESlateVisibility::Collapsed);
			RingImage->SetRenderOpacity(0.0f);
			return;
		}

		FWidgetTransform RingTransform;
		RingTransform.Angle = SaveSlotSelectionRingAngle;
		RingTransform.Scale = FVector2D(1.0f, 1.0f);

		RingImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		RingImage->SetRenderTransform(RingTransform);
		RingImage->SetRenderOpacity(RingOpacity);
	};

	ApplyRingTransform(GeneratedSaveSlot1SelectionRingImage, SelectedSaveSlotIndex == 1);
	ApplyRingTransform(GeneratedSaveSlot2SelectionRingImage, SelectedSaveSlotIndex == 2);
	ApplyRingTransform(GeneratedSaveSlot3SelectionRingImage, SelectedSaveSlotIndex == 3);
}

void UTunaSweeperIntroMenuWidget::SetSaveSlotSelectionRingSelected(UImage* RingImage, bool bSelected) const
{
	if (!RingImage)
	{
		return;
	}

	RingImage->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bSelected)
	{
		RingImage->SetRenderOpacity(0.0f);
	}
}

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

void UTunaSweeperIntroMenuWidget::EnsureDifficultySelectionPanel()
{
	if (DifficultySelectPanel || !WidgetTree)
	{
		return;
	}

	LoadDifficultyDefinitions();

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	UOverlay* Panel = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(),
		TEXT("DifficultySelectPanel"));
	if (!Panel)
	{
		return;
	}
	DifficultySelectPanel = Panel;
	Panel->SetVisibility(ESlateVisibility::Collapsed);

	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		PanelSlot->SetOffsets(FMargin(0.0f));
		PanelSlot->SetAlignment(FVector2D::ZeroVector);
		PanelSlot->SetZOrder(12);
	}

	DifficultyBackgroundImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("DifficultyBackgroundImage"));
	if (DifficultyBackgroundImage)
	{
		FSlateBrush BackgroundBrush;
		BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
		BackgroundBrush.TintColor = FSlateColor(FLinearColor::White);
		BackgroundBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));
		if (UTexture2D* BackgroundTexture = LoadDifficultyTexture(
			DifficultyBackgroundTexture,
			TunaSweeperDifficultySelect::BackgroundTexturePath))
		{
			BackgroundBrush.SetResourceObject(BackgroundTexture);
		}
		DifficultyBackgroundImage->SetBrush(BackgroundBrush);
		DifficultyBackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* BackgroundSlot = Panel->AddChildToOverlay(DifficultyBackgroundImage))
		{
			BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
			BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	UImage* ReadabilityWash = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("DifficultyReadabilityWash"));
	if (ReadabilityWash)
	{
		FSlateBrush WashBrush;
		WashBrush.DrawAs = ESlateBrushDrawType::Box;
		WashBrush.TintColor = FSlateColor(FLinearColor(1.0f, 0.985f, 0.93f, 0.38f));
		ReadabilityWash->SetBrush(WashBrush);
		ReadabilityWash->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UOverlaySlot* WashSlot = Panel->AddChildToOverlay(ReadabilityWash))
		{
			WashSlot->SetHorizontalAlignment(HAlign_Fill);
			WashSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	USizeBox* ContentBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("DifficultyContentBox"));
	UVerticalBox* ContentStack = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("DifficultyContentStack"));
	if (!ContentBox || !ContentStack)
	{
		return;
	}
	ContentBox->SetWidthOverride(1120.0f);
	ContentBox->SetHeightOverride(690.0f);
	ContentBox->SetContent(ContentStack);

	if (UOverlaySlot* ContentSlot = Panel->AddChildToOverlay(ContentBox))
	{
		ContentSlot->SetHorizontalAlignment(HAlign_Center);
		ContentSlot->SetVerticalAlignment(VAlign_Center);
		ContentSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
	}

	DifficultyTitleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("DifficultyTitleText"));
	if (DifficultyTitleText)
	{
		DifficultyTitleText->SetText(FText::FromString(TEXT("\uB09C\uC774\uB3C4 \uC120\uD0DD")));
		DifficultyTitleText->SetJustification(ETextJustify::Center);
		DifficultyTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.10f, 0.18f, 0.20f, 1.0f)));
		TunaSweeperUIFont::ApplyFont(DifficultyTitleText, 42.0f);
		if (UVerticalBoxSlot* TitleSlot = ContentStack->AddChildToVerticalBox(DifficultyTitleText))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Center);
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	auto BuildOptionButton = [this](
		const TCHAR* ButtonName,
		const TCHAR* OverlayName,
		const TCHAR* BackgroundName,
		const TCHAR* BorderName,
		const TCHAR* IconName,
		const TCHAR* TitleName,
		const TCHAR* DescriptionName,
		int32 DifficultyStage,
		TObjectPtr<UButton>& OutButton,
		TObjectPtr<UImage>& OutBackgroundImage,
		TObjectPtr<UBorder>& OutSelectionBorder,
		TObjectPtr<UImage>& OutIconImage,
		TObjectPtr<UTextBlock>& OutTitleText,
		TObjectPtr<UTextBlock>& OutDescriptionText) -> UButton*
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
		UOverlay* ButtonOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), OverlayName);
		if (!Button || !ButtonOverlay)
		{
			return nullptr;
		}

		OutButton = Button;
		ApplyDifficultyButtonStyle(Button);

		OutBackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), BackgroundName);
		if (OutBackgroundImage)
		{
			ConfigureDifficultyCardBackground(OutBackgroundImage, false);
			if (UOverlaySlot* BackgroundSlot = ButtonOverlay->AddChildToOverlay(OutBackgroundImage))
			{
				BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
				BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}

		OutSelectionBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), BorderName);
		if (OutSelectionBorder)
		{
			ConfigureDifficultySelectionBorder(OutSelectionBorder, false);
			if (UOverlaySlot* BorderSlot = ButtonOverlay->AddChildToOverlay(OutSelectionBorder))
			{
				BorderSlot->SetHorizontalAlignment(HAlign_Fill);
				BorderSlot->SetVerticalAlignment(VAlign_Fill);
				BorderSlot->SetPadding(FMargin(16.0f));
			}
		}

		UVerticalBox* CardStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		OutIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		UVerticalBox* TextStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		OutTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TitleName);
		OutDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), DescriptionName);
		if (CardStack && IconBox && OutIconImage && TextStack && OutTitleText && OutDescriptionText)
		{
			IconBox->SetWidthOverride(184.0f);
			IconBox->SetHeightOverride(156.0f);
			ConfigureDifficultyIcon(OutIconImage, DifficultyStage);
			IconBox->SetContent(OutIconImage);

			if (UVerticalBoxSlot* IconSlot = CardStack->AddChildToVerticalBox(IconBox))
			{
				IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				IconSlot->SetHorizontalAlignment(HAlign_Center);
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetPadding(FMargin(0.0f, 50.0f, 0.0f, 16.0f));
			}

			OutTitleText->SetText(BuildDifficultyTitleText(DifficultyStage));
			OutTitleText->SetJustification(ETextJustify::Center);
			OutTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.11f, 0.20f, 0.22f, 1.0f)));
			TunaSweeperUIFont::ApplyFont(OutTitleText, 32.0f);
			if (UVerticalBoxSlot* OptionTitleSlot = TextStack->AddChildToVerticalBox(OutTitleText))
			{
				OptionTitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				OptionTitleSlot->SetHorizontalAlignment(HAlign_Fill);
				OptionTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
			}

			OutDescriptionText->SetText(BuildDifficultyDescriptionText(DifficultyStage));
			OutDescriptionText->SetAutoWrapText(true);
			OutDescriptionText->SetWrapTextAt(230.0f);
			OutDescriptionText->SetJustification(ETextJustify::Center);
			OutDescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.21f, 0.30f, 0.32f, 0.95f)));
			TunaSweeperUIFont::ApplyFont(OutDescriptionText, 18.0f);
			if (UVerticalBoxSlot* DescriptionSlot = TextStack->AddChildToVerticalBox(OutDescriptionText))
			{
				DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
				DescriptionSlot->SetVerticalAlignment(VAlign_Top);
			}

			if (UVerticalBoxSlot* TextSlot = CardStack->AddChildToVerticalBox(TextStack))
			{
				TextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				TextSlot->SetHorizontalAlignment(HAlign_Fill);
				TextSlot->SetVerticalAlignment(VAlign_Fill);
				TextSlot->SetPadding(FMargin(30.0f, 0.0f, 30.0f, 34.0f));
			}

			if (UOverlaySlot* CardSlot = ButtonOverlay->AddChildToOverlay(CardStack))
			{
				CardSlot->SetHorizontalAlignment(HAlign_Fill);
				CardSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}

		Button->SetContent(ButtonOverlay);
		return Button;
	};

	UHorizontalBox* OptionRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("DifficultyOptionRow"));
	if (OptionRow)
	{
		if (UVerticalBoxSlot* OptionRowSlot = ContentStack->AddChildToVerticalBox(OptionRow))
		{
			OptionRowSlot->SetHorizontalAlignment(HAlign_Center);
			OptionRowSlot->SetVerticalAlignment(VAlign_Center);
			OptionRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 0.0f));
			OptionRowSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	auto AddOptionButton = [this, OptionRow](UButton* Button, float LeftPadding)
	{
		if (!Button || !OptionRow)
		{
			return;
		}

		USizeBox* ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		if (!ButtonBox)
		{
			return;
		}
		ButtonBox->SetWidthOverride(320.0f);
		ButtonBox->SetHeightOverride(455.0f);
		ButtonBox->SetContent(Button);
		if (UHorizontalBoxSlot* OptionSlot = OptionRow->AddChildToHorizontalBox(ButtonBox))
		{
			OptionSlot->SetHorizontalAlignment(HAlign_Center);
			OptionSlot->SetVerticalAlignment(VAlign_Center);
			OptionSlot->SetPadding(FMargin(LeftPadding, 0.0f, 0.0f, 0.0f));
			OptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	};

	AddOptionButton(BuildOptionButton(
		TEXT("DifficultyFarmingButton"),
		TEXT("DifficultyFarmingButtonOverlay"),
		TEXT("DifficultyFarmingBackgroundImage"),
		TEXT("DifficultyFarmingSelectionBorder"),
		TEXT("DifficultyFarmingIconImage"),
		TEXT("DifficultyFarmingTitleText"),
		TEXT("DifficultyFarmingDescriptionText"),
		1,
		DifficultyFarmingButton,
		DifficultyFarmingBackgroundImage,
		DifficultyFarmingSelectionBorder,
		DifficultyFarmingIconImage,
		DifficultyFarmingTitleText,
		DifficultyFarmingDescriptionText), 0.0f);

	AddOptionButton(BuildOptionButton(
		TEXT("DifficultyNormalButton"),
		TEXT("DifficultyNormalButtonOverlay"),
		TEXT("DifficultyNormalBackgroundImage"),
		TEXT("DifficultyNormalSelectionBorder"),
		TEXT("DifficultyNormalIconImage"),
		TEXT("DifficultyNormalTitleText"),
		TEXT("DifficultyNormalDescriptionText"),
		2,
		DifficultyNormalButton,
		DifficultyNormalBackgroundImage,
		DifficultyNormalSelectionBorder,
		DifficultyNormalIconImage,
		DifficultyNormalTitleText,
		DifficultyNormalDescriptionText), 22.0f);

	AddOptionButton(BuildOptionButton(
		TEXT("DifficultyHardButton"),
		TEXT("DifficultyHardButtonOverlay"),
		TEXT("DifficultyHardBackgroundImage"),
		TEXT("DifficultyHardSelectionBorder"),
		TEXT("DifficultyHardIconImage"),
		TEXT("DifficultyHardTitleText"),
		TEXT("DifficultyHardDescriptionText"),
		3,
		DifficultyHardButton,
		DifficultyHardBackgroundImage,
		DifficultyHardSelectionBorder,
		DifficultyHardIconImage,
		DifficultyHardTitleText,
		DifficultyHardDescriptionText), 22.0f);

	USpacer* ActionSpacer = WidgetTree->ConstructWidget<USpacer>(USpacer::StaticClass());
	if (ActionSpacer)
	{
		ActionSpacer->SetSize(FVector2D(1.0f, 22.0f));
		ContentStack->AddChildToVerticalBox(ActionSpacer);
	}

	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("DifficultyActionRow"));
	if (ActionRow)
	{
		auto BuildActionButton = [this](
			const TCHAR* ButtonName,
			const TCHAR* TextName,
			const FText& Label,
			TObjectPtr<UTextBlock>& OutText) -> UButton*
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
			UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
			UImage* Background = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			OutText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TextName);
			if (!Button || !Overlay || !Background || !OutText)
			{
				return nullptr;
			}

			ApplyDifficultyButtonStyle(Button);
			ConfigureDifficultyActionButtonBackground(Background, false);
			Background->SetRenderOpacity(0.92f);
			if (UOverlaySlot* BackgroundSlot = Overlay->AddChildToOverlay(Background))
			{
				BackgroundSlot->SetHorizontalAlignment(HAlign_Fill);
				BackgroundSlot->SetVerticalAlignment(VAlign_Fill);
			}

			OutText->SetText(Label);
			OutText->SetJustification(ETextJustify::Center);
			OutText->SetColorAndOpacity(FSlateColor(FLinearColor(0.10f, 0.19f, 0.21f, 1.0f)));
			TunaSweeperUIFont::ApplyFont(OutText, 24.0f);
			if (UOverlaySlot* TextSlot = Overlay->AddChildToOverlay(OutText))
			{
				TextSlot->SetHorizontalAlignment(HAlign_Fill);
				TextSlot->SetVerticalAlignment(VAlign_Center);
				TextSlot->SetPadding(FMargin(22.0f, 0.0f));
			}

			Button->SetContent(Overlay);
			return Button;
		};

		DifficultyBackButton = BuildActionButton(
			TEXT("DifficultyBackButton"),
			TEXT("DifficultyBackButtonText"),
			FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30")),
			DifficultyBackButtonText);
		DifficultyStartButton = BuildActionButton(
			TEXT("DifficultyStartButton"),
			TEXT("DifficultyStartButtonText"),
			FText::FromString(TEXT("\uAC8C\uC784 \uC2DC\uC791")),
			DifficultyStartButtonText);

		if (DifficultyBackButton)
		{
			USizeBox* BackBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			if (BackBox)
			{
				BackBox->SetWidthOverride(260.0f);
				BackBox->SetHeightOverride(82.0f);
				BackBox->SetContent(DifficultyBackButton);
				if (UHorizontalBoxSlot* BackSlot = ActionRow->AddChildToHorizontalBox(BackBox))
				{
					BackSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
					BackSlot->SetPadding(FMargin(0.0f, 0.0f, 18.0f, 0.0f));
				}
			}
		}

		if (DifficultyStartButton)
		{
			USizeBox* StartBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			if (StartBox)
			{
				StartBox->SetWidthOverride(430.0f);
				StartBox->SetHeightOverride(82.0f);
				StartBox->SetContent(DifficultyStartButton);
				if (UHorizontalBoxSlot* StartSlot = ActionRow->AddChildToHorizontalBox(StartBox))
				{
					StartSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				}
			}
		}

		if (UVerticalBoxSlot* ActionSlot = ContentStack->AddChildToVerticalBox(ActionRow))
		{
			ActionSlot->SetHorizontalAlignment(HAlign_Center);
			ActionSlot->SetVerticalAlignment(VAlign_Center);
			ActionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	RefreshDifficultySelectionPanel();
}

void UTunaSweeperIntroMenuWidget::SelectDifficultyStage(int32 DifficultyStage)
{
	SelectedDifficultyStage = FMath::Clamp(DifficultyStage, 1, 3);
	RefreshDifficultySelectionPanel();
}

void UTunaSweeperIntroMenuWidget::RefreshDifficultySelectionPanel()
{
	LoadDifficultyDefinitions();

	if (DifficultyTitleText)
	{
		DifficultyTitleText->SetText(FText::FromString(TEXT("\uB09C\uC774\uB3C4 \uC120\uD0DD")));
	}
	if (DifficultyStartButtonText)
	{
		DifficultyStartButtonText->SetText(FText::FromString(TEXT("\uAC8C\uC784 \uC2DC\uC791")));
	}
	if (DifficultyBackButtonText)
	{
		DifficultyBackButtonText->SetText(FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30")));
	}

	RefreshDifficultyOption(
		1,
		DifficultyFarmingButton,
		DifficultyFarmingBackgroundImage,
		DifficultyFarmingSelectionBorder,
		DifficultyFarmingTitleText,
		DifficultyFarmingDescriptionText);
	RefreshDifficultyOption(
		2,
		DifficultyNormalButton,
		DifficultyNormalBackgroundImage,
		DifficultyNormalSelectionBorder,
		DifficultyNormalTitleText,
		DifficultyNormalDescriptionText);
	RefreshDifficultyOption(
		3,
		DifficultyHardButton,
		DifficultyHardBackgroundImage,
		DifficultyHardSelectionBorder,
		DifficultyHardTitleText,
		DifficultyHardDescriptionText);

	if (DifficultyStartButton)
	{
		DifficultyStartButton->SetIsEnabled(SelectedDifficultyStage != INDEX_NONE && !bStartTravelPending);
	}
}

void UTunaSweeperIntroMenuWidget::RefreshDifficultyOption(
	int32 DifficultyStage,
	UButton* Button,
	UImage* BackgroundImage,
	UBorder* SelectionBorder,
	UTextBlock* TitleText,
	UTextBlock* DescriptionText)
{
	const bool bSelected = SelectedDifficultyStage == DifficultyStage;
	if (Button)
	{
		Button->SetIsEnabled(!bStartTravelPending);
	}
	ConfigureDifficultyCardBackground(BackgroundImage, bSelected);
	ConfigureDifficultySelectionBorder(SelectionBorder, bSelected);
	if (TitleText)
	{
		TitleText->SetText(BuildDifficultyTitleText(DifficultyStage));
		TitleText->SetColorAndOpacity(FSlateColor(bSelected
			? FLinearColor(0.06f, 0.16f, 0.18f, 1.0f)
			: FLinearColor(0.11f, 0.20f, 0.22f, 1.0f)));
	}
	if (DescriptionText)
	{
		DescriptionText->SetText(BuildDifficultyDescriptionText(DifficultyStage));
		DescriptionText->SetColorAndOpacity(FSlateColor(bSelected
			? FLinearColor(0.12f, 0.24f, 0.26f, 1.0f)
			: FLinearColor(0.21f, 0.30f, 0.32f, 0.95f)));
	}
}

void UTunaSweeperIntroMenuWidget::ApplyDifficultyButtonStyle(UButton* Button) const
{
	if (!Button)
	{
		return;
	}

	FSlateBrush EmptyBrush;
	EmptyBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
	EmptyBrush.TintColor = FSlateColor(FLinearColor::Transparent);

	FButtonStyle ButtonStyle;
	ButtonStyle.SetNormal(EmptyBrush);
	ButtonStyle.SetHovered(EmptyBrush);
	ButtonStyle.SetPressed(EmptyBrush);
	ButtonStyle.SetDisabled(EmptyBrush);
	ButtonStyle.SetNormalPadding(FMargin(0.0f));
	ButtonStyle.SetPressedPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
	Button->SetStyle(ButtonStyle);
	Button->SetClickMethod(EButtonClickMethod::DownAndUp);
}

void UTunaSweeperIntroMenuWidget::ConfigureDifficultyCardBackground(UImage* BackgroundImage, bool bSelected)
{
	if (!BackgroundImage)
	{
		return;
	}

	FSlateBrush BackgroundBrush;
	BackgroundBrush.DrawAs = ESlateBrushDrawType::Box;
	BackgroundBrush.Margin = FMargin(0.18f);
	BackgroundBrush.SetImageSize(FVector2D(320.0f, 455.0f));
	BackgroundBrush.TintColor = FSlateColor(bSelected
		? FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)
		: FLinearColor(0.94f, 0.98f, 0.98f, 0.92f));
	if (UTexture2D* ButtonTexture = LoadDifficultyTexture(
		DifficultyCardFrameTexture,
		TunaSweeperDifficultySelect::CardFrameTexturePath))
	{
		BackgroundBrush.SetResourceObject(ButtonTexture);
	}
	BackgroundImage->SetBrush(BackgroundBrush);
	BackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	BackgroundImage->SetRenderOpacity(bSelected ? 1.0f : 0.90f);
}

void UTunaSweeperIntroMenuWidget::ConfigureDifficultyActionButtonBackground(UImage* BackgroundImage, bool bSelected)
{
	if (!BackgroundImage)
	{
		return;
	}

	FSlateBrush BackgroundBrush;
	BackgroundBrush.DrawAs = ESlateBrushDrawType::Box;
	BackgroundBrush.Margin = FMargin(0.25f, 0.36f);
	BackgroundBrush.SetImageSize(FVector2D(420.0f, 82.0f));
	BackgroundBrush.TintColor = FSlateColor(bSelected
		? FLinearColor::White
		: FLinearColor(0.96f, 0.99f, 0.99f, 0.93f));
	if (UTexture2D* ButtonTexture = LoadDifficultyTexture(
		DifficultyActionButtonTexture,
		TunaSweeperDifficultySelect::ActionButtonTexturePath))
	{
		BackgroundBrush.SetResourceObject(ButtonTexture);
	}
	BackgroundImage->SetBrush(BackgroundBrush);
	BackgroundImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	BackgroundImage->SetRenderOpacity(bSelected ? 1.0f : 0.92f);
}

void UTunaSweeperIntroMenuWidget::ConfigureDifficultySelectionBorder(UBorder* SelectionBorder, bool bSelected)
{
	if (!SelectionBorder)
	{
		return;
	}

	FSlateBrush BorderBrush;
	BorderBrush.DrawAs = ESlateBrushDrawType::RoundedBox;
	BorderBrush.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.0f));
	BorderBrush.OutlineSettings = FSlateBrushOutlineSettings(
		32.0f,
		FSlateColor(FLinearColor(1.0f, 0.76f, 0.22f, 1.0f)),
		bSelected ? 5.0f : 0.0f);
	BorderBrush.OutlineSettings.bUseBrushTransparency = false;
	SelectionBorder->SetBrush(BorderBrush);
	SelectionBorder->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UTunaSweeperIntroMenuWidget::ConfigureDifficultyIcon(UImage* IconImage, int32 DifficultyStage)
{
	if (!IconImage)
	{
		return;
	}

	UTexture2D* IconTexture = nullptr;
	switch (FMath::Clamp(DifficultyStage, 1, 3))
	{
	case 1:
		IconTexture = LoadDifficultyTexture(
			DifficultyFarmingIconTexture,
			TunaSweeperDifficultySelect::FarmingIconTexturePath);
		break;
	case 2:
		IconTexture = LoadDifficultyTexture(
			DifficultyNormalIconTexture,
			TunaSweeperDifficultySelect::NormalIconTexturePath);
		break;
	case 3:
	default:
		IconTexture = LoadDifficultyTexture(
			DifficultyHardIconTexture,
			TunaSweeperDifficultySelect::HardIconTexturePath);
		break;
	}

	FSlateBrush IconBrush;
	IconBrush.DrawAs = ESlateBrushDrawType::Image;
	IconBrush.SetImageSize(FVector2D(150.0f, 150.0f));
	IconBrush.TintColor = FSlateColor(FLinearColor::White);
	if (IconTexture)
	{
		IconBrush.SetResourceObject(IconTexture);
	}
	IconImage->SetBrush(IconBrush);
	IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UTunaSweeperIntroMenuWidget::LoadDifficultyDefinitions()
{
	if (bDifficultyDefinitionsLoaded)
	{
		return;
	}

	DifficultyOptionTexts.Reset();
	for (int32 DifficultyStage = 1; DifficultyStage <= 3; ++DifficultyStage)
	{
		FDifficultyOptionText OptionText;
		OptionText.DifficultyStage = DifficultyStage;
		OptionText.Title = TunaSweeperDifficultySelect::MakeFallbackTitle(DifficultyStage);
		OptionText.Description = TunaSweeperDifficultySelect::MakeFallbackDescription(DifficultyStage);
		DifficultyOptionTexts.Add(OptionText);
	}

	FString JsonContent;
	const FString JsonPath = TunaSweeperDifficultySelect::GetDefinitionsJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		bDifficultyDefinitionsLoaded = true;
		return;
	}

	TSharedPtr<FJsonValue> RootValue;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, RootValue) || !RootValue.IsValid())
	{
		bDifficultyDefinitionsLoaded = true;
		return;
	}

	TArray<TSharedPtr<FJsonValue>> RootArrayValues;
	const TArray<TSharedPtr<FJsonValue>>* DifficultyValues = nullptr;
	if (RootValue->Type == EJson::Array)
	{
		RootArrayValues = RootValue->AsArray();
		DifficultyValues = &RootArrayValues;
	}
	else if (RootValue->Type == EJson::Object)
	{
		const TSharedPtr<FJsonObject> RootObject = RootValue->AsObject();
		if (RootObject.IsValid())
		{
			RootObject->TryGetArrayField(TEXT("difficulties"), DifficultyValues) ||
				RootObject->TryGetArrayField(TEXT("difficulty_options"), DifficultyValues);
		}
	}

	if (!DifficultyValues)
	{
		bDifficultyDefinitionsLoaded = true;
		return;
	}

	for (const TSharedPtr<FJsonValue>& DifficultyValue : *DifficultyValues)
	{
		const TSharedPtr<FJsonObject>* DifficultyObjectPtr = nullptr;
		if (!DifficultyValue.IsValid() ||
			!DifficultyValue->TryGetObject(DifficultyObjectPtr) ||
			!DifficultyObjectPtr ||
			!DifficultyObjectPtr->IsValid())
		{
			continue;
		}

		const TSharedPtr<FJsonObject>& DifficultyObject = *DifficultyObjectPtr;
		double NumericStage = 0.0;
		if (!DifficultyObject->TryGetNumberField(TEXT("difficulty_stage"), NumericStage) &&
			!DifficultyObject->TryGetNumberField(TEXT("stage"), NumericStage) &&
			!DifficultyObject->TryGetNumberField(TEXT("id"), NumericStage))
		{
			continue;
		}

		const int32 DifficultyStage = FMath::Clamp(FMath::RoundToInt(NumericStage), 1, 3);
		FDifficultyOptionText* OptionText = DifficultyOptionTexts.FindByPredicate(
			[DifficultyStage](const FDifficultyOptionText& Candidate)
			{
				return Candidate.DifficultyStage == DifficultyStage;
			});
		if (!OptionText)
		{
			continue;
		}

		FString StringValue;
		if (DifficultyObject->TryGetStringField(TEXT("title"), StringValue) ||
			DifficultyObject->TryGetStringField(TEXT("name"), StringValue) ||
			DifficultyObject->TryGetStringField(TEXT("label"), StringValue))
		{
			OptionText->Title = FText::FromString(StringValue);
		}
		if (DifficultyObject->TryGetStringField(TEXT("description"), StringValue) ||
			DifficultyObject->TryGetStringField(TEXT("desc"), StringValue))
		{
			OptionText->Description = FText::FromString(StringValue);
		}
	}

	bDifficultyDefinitionsLoaded = true;
}

FText UTunaSweeperIntroMenuWidget::BuildDifficultyTitleText(int32 DifficultyStage) const
{
	if (const FDifficultyOptionText* OptionText = DifficultyOptionTexts.FindByPredicate(
		[DifficultyStage](const FDifficultyOptionText& Candidate)
		{
			return Candidate.DifficultyStage == DifficultyStage;
		}))
	{
		return OptionText->Title;
	}

	return TunaSweeperDifficultySelect::MakeFallbackTitle(DifficultyStage);
}

FText UTunaSweeperIntroMenuWidget::BuildDifficultyDescriptionText(int32 DifficultyStage) const
{
	if (const FDifficultyOptionText* OptionText = DifficultyOptionTexts.FindByPredicate(
		[DifficultyStage](const FDifficultyOptionText& Candidate)
		{
			return Candidate.DifficultyStage == DifficultyStage;
		}))
	{
		return OptionText->Description;
	}

	return TunaSweeperDifficultySelect::MakeFallbackDescription(DifficultyStage);
}

UTexture2D* UTunaSweeperIntroMenuWidget::LoadDifficultyTexture(
	TObjectPtr<UTexture2D>& TextureCache,
	const TCHAR* TexturePath)
{
	if (!TextureCache && TexturePath)
	{
		TextureCache = LoadObject<UTexture2D>(nullptr, TexturePath);
	}

	return TextureCache;
}

void UTunaSweeperIntroMenuWidget::EnsureAlwaysNewStartButton()
{
	if (AlwaysNewStartButton)
	{
		if (!AlwaysNewStartButtonContainer)
		{
			AlwaysNewStartButtonContainer = AlwaysNewStartButton;
		}
		if (AlwaysNewStartButtonText)
		{
			AlwaysNewStartButtonText->SetText(ResolveUiText(
				FName(TEXT("ui.title.always_new_start")),
				FText::FromString(TEXT("\uD56D\uC0C1\uC0C8\uB85C\uC2DC\uC791"))));
		}
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	USizeBox* AlwaysNewStartButtonBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("AlwaysNewStartButtonBox"));
	AlwaysNewStartButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("AlwaysNewStartButton"));
	AlwaysNewStartButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("AlwaysNewStartButtonText"));

	if (!AlwaysNewStartButtonBox || !AlwaysNewStartButton || !AlwaysNewStartButtonText)
	{
		return;
	}

	auto MakeBoxBrush = [](const FLinearColor& Color)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Color);
		return Brush;
	};

	FButtonStyle DebugButtonStyle;
	DebugButtonStyle.SetNormal(MakeBoxBrush(FLinearColor(0.34f, 0.03f, 0.02f, 0.76f)));
	DebugButtonStyle.SetHovered(MakeBoxBrush(FLinearColor(0.58f, 0.06f, 0.04f, 0.90f)));
	DebugButtonStyle.SetPressed(MakeBoxBrush(FLinearColor(0.22f, 0.02f, 0.015f, 0.95f)));
	DebugButtonStyle.SetNormalPadding(FMargin(0.0f));
	DebugButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));
	AlwaysNewStartButton->SetStyle(DebugButtonStyle);
	AlwaysNewStartButton->SetClickMethod(EButtonClickMethod::DownAndUp);

	AlwaysNewStartButtonText->SetText(ResolveUiText(
		FName(TEXT("ui.title.always_new_start")),
		FText::FromString(TEXT("\uD56D\uC0C1\uC0C8\uB85C\uC2DC\uC791"))));
	AlwaysNewStartButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.92f, 0.88f, 1.0f)));
	AlwaysNewStartButtonText->SetJustification(ETextJustify::Center);
	TunaSweeperUIFont::ApplyFont(AlwaysNewStartButtonText, 16, ETunaSweeperUIFontWeight::Bold);

	AlwaysNewStartButtonBox->SetWidthOverride(180.0f);
	AlwaysNewStartButtonBox->SetHeightOverride(42.0f);
	AlwaysNewStartButton->SetContent(AlwaysNewStartButtonText);
	AlwaysNewStartButtonBox->SetContent(AlwaysNewStartButton);
	AlwaysNewStartButtonContainer = AlwaysNewStartButtonBox;

	UCanvasPanelSlot* DebugButtonSlot = RootCanvas->AddChildToCanvas(AlwaysNewStartButtonBox);
	if (DebugButtonSlot)
	{
		DebugButtonSlot->SetAnchors(FAnchors(1.0f, 0.0f));
		DebugButtonSlot->SetAlignment(FVector2D(1.0f, 0.0f));
		DebugButtonSlot->SetPosition(FVector2D(-36.0f, 36.0f));
		DebugButtonSlot->SetSize(FVector2D(180.0f, 42.0f));
		DebugButtonSlot->SetZOrder(200);
	}
}

void UTunaSweeperIntroMenuWidget::SetAlwaysNewStartButtonVisible(bool bVisible)
{
	EnsureAlwaysNewStartButton();
	if (AlwaysNewStartButtonContainer)
	{
		AlwaysNewStartButtonContainer->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (AlwaysNewStartButton)
	{
		AlwaysNewStartButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

FText UTunaSweeperIntroMenuWidget::BuildCurrentSaveSlotText(int32 SaveSlotIndex) const
{
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(SaveSlotIndex);
	}
	else
	{
		Summary.SaveSlotIndex = SaveSlotIndex;
	}

	if (!Summary.bHasData)
	{
		return FText::Format(
			FText::FromString(TEXT("{0} - {1}")),
			FText::Format(
				ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
				FText::AsNumber(SaveSlotIndex)),
			ResolveUiText(FName(TEXT("ui.title.empty_slot")), FText::FromString(TEXT("\uBE48 \uC2AC\uB86F"))));
	}

	return FText::Format(
		ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
		FText::AsNumber(SaveSlotIndex));
}

FText UTunaSweeperIntroMenuWidget::BuildSaveSlotButtonText(int32 SaveSlotIndex) const
{
	FTunaSweeperSaveSlotSummary Summary;
	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		Summary = TunaGameInstance->GetSaveSlotSummary(SaveSlotIndex);
	}
	else
	{
		Summary.SaveSlotIndex = SaveSlotIndex;
	}

	if (!Summary.bHasData)
	{
		const TArray<FString> Lines = {
			FText::Format(
				ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
				FText::AsNumber(SaveSlotIndex)).ToString(),
			ResolveUiText(FName(TEXT("ui.title.empty_slot")), FText::FromString(TEXT("\uBE48 \uC2AC\uB86F"))).ToString(),
			ResolveUiText(FName(TEXT("ui.title.start_new_game")), FText::FromString(TEXT("\uC0C8 \uAC8C\uC784 \uC2DC\uC791"))).ToString()
		};
		return FText::FromString(FString::Join(Lines, LINE_TERMINATOR));
	}

	const TArray<FString> Lines = {
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.slot_label")), FText::FromString(TEXT("\uC2AC\uB86F {0}"))),
			FText::AsNumber(SaveSlotIndex)).ToString(),
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.play_time_pattern")), FText::FromString(TEXT("\uD50C\uB808\uC774\uC2DC\uAC04 : {0}"))),
			FText::FromString(FormatPlayTime(Summary.TotalPlaySeconds))).ToString(),
		FText::Format(
			ResolveUiText(FName(TEXT("ui.title.difficulty_pattern")), FText::FromString(TEXT("\uB09C\uC774\uB3C4 : {0}"))),
			BuildSaveSlotDifficultyText(Summary.DifficultyStage, Summary.bDifficultySelected)).ToString()
	};
	return FText::FromString(FString::Join(Lines, LINE_TERMINATOR));
}

FString UTunaSweeperIntroMenuWidget::BuildCreditsRollText() const
{
	const FString CreditsFilePath = FPaths::Combine(
		FPaths::ProjectContentDir(),
		TEXT("UI"),
		TEXT("Credits"),
		TEXT("StaffRoll.txt"));

	FString CreditsTextFromFile;
	if (FFileHelper::LoadFileToString(CreditsTextFromFile, *CreditsFilePath) &&
		!CreditsTextFromFile.TrimStartAndEnd().IsEmpty())
	{
		return CreditsTextFromFile;
	}

	return FString(
		TEXT("Tuna Sweeper\n\n")
		TEXT("A Game by BlenG\n\n\n")
		TEXT("Direction\nBlenG\n\n")
		TEXT("Game Design\nBlenG\n\n")
		TEXT("Programming\nBlenG\n\n")
		TEXT("Art Direction\nBlenG\n\n")
		TEXT("UI Design\nBlenG\n\n")
		TEXT("Scenario\nBlenG\n\n")
		TEXT("Level Design\nBlenG\n\n")
		TEXT("Audio Direction\nBlenG\n\n")
		TEXT("QA\nBlenG\n\n\n")
		TEXT("Thank you for playing.\n"));
}

FString UTunaSweeperIntroMenuWidget::BuildCreditsColumnText(int32 ColumnIndex) const
{
	TArray<FString> Lines;
	BuildCreditsRollText().ParseIntoArrayLines(Lines, false);

	if (Lines.IsEmpty())
	{
		return FString();
	}

	const int32 ClampedColumnIndex = FMath::Clamp(ColumnIndex, 0, 2);
	const int32 LinesPerColumn = FMath::Max(1, FMath::DivideAndRoundUp(Lines.Num(), 3));
	const int32 StartIndex = ClampedColumnIndex * LinesPerColumn;
	const int32 EndIndex = FMath::Min(StartIndex + LinesPerColumn, Lines.Num());

	FString ColumnText;
	for (int32 LineIndex = StartIndex; LineIndex < EndIndex; ++LineIndex)
	{
		if (!ColumnText.IsEmpty())
		{
			ColumnText += LINE_TERMINATOR;
		}
		ColumnText += Lines[LineIndex];
	}

	return ColumnText;
}

FString UTunaSweeperIntroMenuWidget::FormatSaveTime(int64 LastSavedAtTicks) const
{
	if (LastSavedAtTicks <= 0)
	{
		return FString(TEXT("--"));
	}

	return FDateTime(LastSavedAtTicks).ToString(TEXT("%Y-%m-%d %H:%M"));
}

FString UTunaSweeperIntroMenuWidget::FormatPlayTime(float TotalPlaySeconds) const
{
	const int32 TotalMinutes = FMath::FloorToInt(FMath::Max(0.0f, TotalPlaySeconds) / 60.0f);
	const int32 Hours = TotalMinutes / 60;
	const int32 Minutes = TotalMinutes % 60;
	return FString::Printf(TEXT("%02d:%02d"), Hours, Minutes);
}

FText UTunaSweeperIntroMenuWidget::BuildSaveSlotDifficultyText(int32 DifficultyStage, bool bDifficultySelected) const
{
	if (!bDifficultySelected)
	{
		return FText::FromString(TEXT("\uBBF8\uC120\uD0DD"));
	}

	switch (FMath::Clamp(DifficultyStage, 1, 3))
	{
	case 1:
		return ResolveUiText(FName(TEXT("ui.title.difficulty.farming")), FText::FromString(TEXT("\uD30C\uBC0D")));
	case 2:
		return FText::FromString(TEXT("\uC77C\uBC18"));
	case 3:
		return FText::FromString(TEXT("\uC5B4\uB824\uC6C0"));
	default:
		return ResolveUiText(FName(TEXT("ui.title.difficulty.farming")), FText::FromString(TEXT("\uD30C\uBC0D")));
	}
}

bool UTunaSweeperIntroMenuWidget::IsSaveSlotSelectionVisible() const
{
	return SaveSlotPanel && SaveSlotPanel->GetVisibility() == ESlateVisibility::Visible;
}

bool UTunaSweeperIntroMenuWidget::IsDifficultySelectionVisible() const
{
	return DifficultySelectPanel && DifficultySelectPanel->GetVisibility() == ESlateVisibility::Visible;
}

bool UTunaSweeperIntroMenuWidget::IsCreditsPanelVisible() const
{
	return CreditsPanel && CreditsPanel->GetVisibility() == ESlateVisibility::Visible;
}

bool UTunaSweeperIntroMenuWidget::CanDeleteSelectedSaveSlot() const
{
	if (SelectedSaveSlotIndex == INDEX_NONE)
	{
		return false;
	}

	if (const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		return TunaGameInstance->GetSaveSlotSummary(SelectedSaveSlotIndex).bHasData;
	}

	return false;
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

void UTunaSweeperIntroMenuWidget::ExecuteSelectedSaveSlotDelete()
{
	const int32 DeletedSaveSlotIndex = SelectedSaveSlotIndex;
	if (!CanDeleteSelectedSaveSlot())
	{
		ResetDeleteHoldProgress();
		return;
	}

	bool bDeleted = false;
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		bDeleted = TunaGameInstance->DeleteSaveSlot(DeletedSaveSlotIndex);
	}

	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	RefreshSaveSlotMenu();
	RefreshMainMenu();

	if (bDeleted)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTunaSweeperToastSubsystem* ToastSubsystem = GameInstance->GetSubsystem<UTunaSweeperToastSubsystem>())
			{
				ToastSubsystem->ShowSaveSlotDeletedToast();
			}
		}
	}
}

void UTunaSweeperIntroMenuWidget::ResetDeleteHoldProgress()
{
	bDeleteHoldActive = false;
	DeleteHoldElapsedSeconds = 0.0f;
	SetDeleteHoldProgress(0.0f);
}

void UTunaSweeperIntroMenuWidget::SetDeleteHoldProgress(float Progress)
{
	EnsureDeleteSaveSlotHoldProgressWidget();
	HideLegacyDeleteHoldGaugeWidgets();

	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	if (!DeleteSaveSlotHoldProgressFill)
	{
		return;
	}

	DeleteSaveSlotHoldProgressFill->SetVisibility(ClampedProgress > 0.0f
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
	DeleteSaveSlotHoldProgressFill->SetRenderOpacity(ClampedProgress > 0.0f ? 1.0f : 0.0f);
	DeleteSaveSlotHoldProgressFill->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
	DeleteSaveSlotHoldProgressFill->SetRenderScale(FVector2D(ClampedProgress, 1.0f));
}

void UTunaSweeperIntroMenuWidget::ShowDeleteConfirmDialog()
{
	bDeleteConfirmVisible = true;
	SetDeleteHoldProgress(1.0f);

	if (DeleteConfirmPanel)
	{
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UTunaSweeperIntroMenuWidget::HideDeleteConfirmDialog()
{
	bDeleteConfirmVisible = false;

	if (DeleteConfirmPanel)
	{
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperIntroMenuWidget::HideLegacyDeleteHoldGaugeWidgets()
{
	if (DeleteHoldGaugeFill)
	{
		DeleteHoldGaugeFill->SetVisibility(ESlateVisibility::Collapsed);
		DeleteHoldGaugeFill->SetRenderOpacity(0.0f);
		DeleteHoldGaugeFill->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		DeleteHoldGaugeFill->SetRenderScale(FVector2D::ZeroVector);
	}

	if (!WidgetTree)
	{
		return;
	}

	static const FName LegacyGaugeWidgetNames[] = {
		FName(TEXT("DeleteHoldGaugeBox")),
		FName(TEXT("DeleteHoldGaugeOverlay")),
		FName(TEXT("DeleteHoldGaugeRing")),
		FName(TEXT("DeleteHoldGaugeFill")),
	};

	for (const FName& WidgetName : LegacyGaugeWidgetNames)
	{
		if (UWidget* GaugeWidget = WidgetTree->FindWidget(WidgetName))
		{
			GaugeWidget->SetVisibility(ESlateVisibility::Collapsed);
			GaugeWidget->SetRenderOpacity(0.0f);
		}
	}
}

void UTunaSweeperIntroMenuWidget::EnsureDeleteSaveSlotHoldProgressWidget()
{
	if (!WidgetTree || !DeleteSaveSlotButton || !DeleteSaveSlotButtonText)
	{
		return;
	}

	if (!DeleteSaveSlotHoldProgressFill)
	{
		DeleteSaveSlotHoldProgressFill = Cast<UImage>(WidgetTree->FindWidget(TEXT("DeleteSaveSlotHoldProgressFill")));
	}

	if (DeleteSaveSlotHoldProgressFill)
	{
		ConfigureDeleteSaveSlotHoldProgressFill();
		return;
	}

	UOverlay* ButtonOverlay = WidgetTree->ConstructWidget<UOverlay>(
		UOverlay::StaticClass(),
		TEXT("DeleteSaveSlotButtonFullProgressOverlay"));
	DeleteSaveSlotHoldProgressFill = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("DeleteSaveSlotHoldProgressFill"));
	if (!ButtonOverlay || !DeleteSaveSlotHoldProgressFill)
	{
		DeleteSaveSlotHoldProgressFill = nullptr;
		return;
	}

	ConfigureDeleteSaveSlotHoldProgressFill();

	if (DeleteSaveSlotButtonBox)
	{
		DeleteSaveSlotButton->RemoveFromParent();
		DeleteSaveSlotButtonText->RemoveFromParent();
		DeleteSaveSlotButtonBox->SetContent(ButtonOverlay);

		if (UOverlaySlot* ButtonSlot = ButtonOverlay->AddChildToOverlay(DeleteSaveSlotButton))
		{
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
			ButtonSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UOverlaySlot* ProgressFillSlot = ButtonOverlay->AddChildToOverlay(DeleteSaveSlotHoldProgressFill))
		{
			ProgressFillSlot->SetHorizontalAlignment(HAlign_Fill);
			ProgressFillSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UOverlaySlot* TextSlot = ButtonOverlay->AddChildToOverlay(DeleteSaveSlotButtonText))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Fill);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
	else
	{
		DeleteSaveSlotButtonText->RemoveFromParent();
		DeleteSaveSlotButton->SetContent(ButtonOverlay);

		if (UOverlaySlot* ProgressFillSlot = ButtonOverlay->AddChildToOverlay(DeleteSaveSlotHoldProgressFill))
		{
			ProgressFillSlot->SetHorizontalAlignment(HAlign_Fill);
			ProgressFillSlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (UOverlaySlot* TextSlot = ButtonOverlay->AddChildToOverlay(DeleteSaveSlotButtonText))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Fill);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	DeleteSaveSlotButtonText->SetVisibility(ESlateVisibility::HitTestInvisible);
	DeleteSaveSlotButtonText->SetJustification(ETextJustify::Center);
	DeleteSaveSlotButtonText->SetMargin(FMargin(0.0f));
}

void UTunaSweeperIntroMenuWidget::ConfigureDeleteSaveSlotHoldProgressFill()
{
	if (!DeleteSaveSlotHoldProgressFill)
	{
		return;
	}

	FSlateBrush ProgressFillBrush;
	ProgressFillBrush.DrawAs = ESlateBrushDrawType::Box;
	ProgressFillBrush.TintColor = FSlateColor(FLinearColor(0.86f, 0.26f, 0.18f, 0.50f));
	DeleteSaveSlotHoldProgressFill->SetBrush(ProgressFillBrush);
	DeleteSaveSlotHoldProgressFill->SetVisibility(ESlateVisibility::Collapsed);
	DeleteSaveSlotHoldProgressFill->SetRenderOpacity(0.0f);
	DeleteSaveSlotHoldProgressFill->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
	DeleteSaveSlotHoldProgressFill->SetRenderScale(FVector2D(0.0f, 1.0f));
}

void UTunaSweeperIntroMenuWidget::ResetTitleViewportLayoutState()
{
	auto ResetWidgetTransform = [](UWidget* Widget)
	{
		if (!Widget)
		{
			return;
		}

		Widget->SetRenderTransform(FWidgetTransform());
		Widget->SetRenderTransformPivot(FVector2D::ZeroVector);
		Widget->SetRenderScale(FVector2D::UnitVector);
	};

	ResetWidgetTransform(this);
	ResetWidgetTransform(GetRootWidget());
	ResetWidgetTransform(MainMenuPanel.Get());
	ResetWidgetTransform(SaveSlotPanel.Get());
	ResetWidgetTransform(SettingsPanel.Get());
	ResetWidgetTransform(CreditsPanel.Get());

	InvalidateLayoutAndVolatility();
}

void UTunaSweeperIntroMenuWidget::ApplyTitleMenuButtonContentLayout()
{
	if (bTitleMenuButtonContentLayoutApplied || !WidgetTree)
	{
		return;
	}

	auto ApplyContent = [this](
		UButton* Button,
		const FText& Icon,
		UTextBlock* ExistingLabelText,
		const FText& Label,
		bool bPrimary)
	{
		if (!Button)
		{
			return;
		}

		UTextBlock* LabelText = ExistingLabelText;
		if (!LabelText)
		{
			LabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		}
		if (!LabelText)
		{
			return;
		}

		Button->SetContent(BuildTitleMenuButtonContent(
			Icon,
			LabelText,
			Label,
			bPrimary ? 28 : 20,
			bPrimary ? 28 : 20));
	};

	ApplyContent(
		StartButton,
		FText::FromString(TEXT("\u25B6")),
		StartButtonText,
		ResolveUiText(FName(TEXT("ui.title.continue")), FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30"))),
		true);
	ApplyContent(
		SlotSelectButton,
		FText::FromString(TEXT("\u25A6")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.slot_select")), FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD"))),
		false);
	ApplyContent(
		SettingsButton,
		FText::FromString(TEXT("\u2699")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.settings")), FText::FromString(TEXT("\uC124\uC815"))),
		false);
	ApplyContent(
		CreditsButton,
		FText::FromString(TEXT("\u24D8")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.credits")), FText::FromString(TEXT("\uD06C\uB808\uB527"))),
		false);
	ApplyContent(
		QuitButton,
		FText::FromString(TEXT("\u00D7")),
		nullptr,
		ResolveUiText(FName(TEXT("ui.title.quit")), FText::FromString(TEXT("\uC885\uB8CC"))),
		false);

	bTitleMenuButtonContentLayoutApplied = true;
}

UWidget* UTunaSweeperIntroMenuWidget::BuildTitleMenuButtonContent(
	const FText& Icon,
	UTextBlock* LabelText,
	const FText& Label,
	int32 LabelFontSize,
	int32 IconFontSize)
{
	if (!WidgetTree || !LabelText)
	{
		return LabelText;
	}

	UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	UTextBlock* IconText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	USizeBox* BalanceBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
	if (!Content || !IconBox || !IconText || !BalanceBox)
	{
		return LabelText;
	}

	const FLinearColor MenuTextColor(0.94f, 0.92f, 0.84f, 1.0f);
	IconText->SetText(Icon);
	TunaSweeperUIFont::ApplyFont(IconText, IconFontSize);
	IconText->SetColorAndOpacity(FSlateColor(MenuTextColor));
	IconText->SetJustification(ETextJustify::Center);

	LabelText->RemoveFromParent();
	LabelText->SetText(Label);
	TunaSweeperUIFont::ApplyFont(LabelText, LabelFontSize);
	LabelText->SetColorAndOpacity(FSlateColor(MenuTextColor));
	LabelText->SetJustification(ETextJustify::Center);

	const float IconLaneWidth = IconFontSize >= 28 ? 58.0f : 46.0f;
	IconBox->SetWidthOverride(IconLaneWidth);
	IconBox->SetHeightOverride(IconFontSize + 8.0f);
	IconBox->SetContent(IconText);
	BalanceBox->SetWidthOverride(IconLaneWidth);
	BalanceBox->SetHeightOverride(IconFontSize + 8.0f);

	if (UHorizontalBoxSlot* IconSlot = Content->AddChildToHorizontalBox(IconBox))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetVerticalAlignment(VAlign_Center);
		IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(LabelText))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Center);
		LabelSlot->SetVerticalAlignment(VAlign_Center);
		LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	if (UHorizontalBoxSlot* BalanceSlot = Content->AddChildToHorizontalBox(BalanceBox))
	{
		BalanceSlot->SetHorizontalAlignment(HAlign_Center);
		BalanceSlot->SetVerticalAlignment(VAlign_Center);
		BalanceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}

	return Content;
}

void UTunaSweeperIntroMenuWidget::EnsureTitleWindParticleOverlay()
{
	if (TitleWindParticleOverlay || !WidgetTree)
	{
		return;
	}

	UCanvasPanel* RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		return;
	}

	auto SetCanvasZOrder = [](UWidget* Widget, int32 ZOrder)
	{
		if (UCanvasPanelSlot* CanvasSlot = Widget ? Cast<UCanvasPanelSlot>(Widget->Slot) : nullptr)
		{
			CanvasSlot->SetZOrder(ZOrder);
		}
	};

	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("BackgroundImage")), 0);
	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("LeftScrim")), 2);
	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("LogoImage")), 3);
	SetCanvasZOrder(MainMenuPanel, 4);
	SetCanvasZOrder(WidgetTree->FindWidget(TEXT("VersionText")), 4);
	SetCanvasZOrder(SaveSlotPanel, 10);
	SetCanvasZOrder(SettingsPanel, 10);
	SetCanvasZOrder(CreditsPanel, 10);

	TitleWindParticleOverlay = WidgetTree->ConstructWidget<UTunaSweeperTitleWindParticleWidget>(
		UTunaSweeperTitleWindParticleWidget::StaticClass(),
		TEXT("TitleWindParticleOverlay"));
	if (!TitleWindParticleOverlay)
	{
		return;
	}

	TitleWindParticleOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);

	UCanvasPanelSlot* ParticleSlot = RootCanvas->AddChildToCanvas(TitleWindParticleOverlay);
	if (!ParticleSlot)
	{
		TitleWindParticleOverlay = nullptr;
		return;
	}

	ParticleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	ParticleSlot->SetOffsets(FMargin(0.0f));
	ParticleSlot->SetAlignment(FVector2D::ZeroVector);
	ParticleSlot->SetZOrder(1);
}

void UTunaSweeperIntroMenuWidget::OpenPendingStartTargetLevel()
{
	if (!bStartTravelPending || PendingStartTargetLevelName.IsNone())
	{
		return;
	}

	const FName TargetLevelName = PendingStartTargetLevelName;
	bStartTravelPending = false;
	PendingStartTargetLevelName = NAME_None;
	UGameplayStatics::OpenLevel(this, TargetLevelName);
}

void UTunaSweeperIntroMenuWidget::SetStartTravelControlsEnabled(bool bEnabled)
{
	const TArray<UButton*> ButtonsToUpdate = {
		StartButton.Get(),
		DifficultyFarmingButton.Get(),
		DifficultyNormalButton.Get(),
		DifficultyHardButton.Get(),
		DifficultyStartButton.Get(),
		DifficultyBackButton.Get(),
		SlotSelectButton.Get(),
		SettingsButton.Get(),
		CreditsButton.Get(),
		QuitButton.Get(),
		AlwaysNewStartButton.Get(),
		PrimarySaveSlotButton.Get(),
		DeleteSaveSlotButton.Get(),
		BackToMainMenuButton.Get(),
		ConfirmDeleteButton.Get(),
		CancelDeleteButton.Get(),
		SettingsGraphicsTabButton.Get(),
		SettingsInterfaceTabButton.Get(),
		WindowedModeButton.Get(),
		BorderlessWindowModeButton.Get(),
		FullscreenModeButton.Get(),
		Resolution1280Button.Get(),
		Resolution1600Button.Get(),
		Resolution1920Button.Get(),
		Resolution2560Button.Get(),
		Resolution3840Button.Get(),
		DLSSOffButton.Get(),
		DLSSQualityButton.Get(),
		DLSSBalancedButton.Get(),
		DLSSPerformanceButton.Get(),
		BackFromSettingsButton.Get(),
		LanguageEnglishButton.Get(),
		LanguageKoreanButton.Get(),
		LanguageJapaneseButton.Get(),
		ConfirmInterfaceSettingsButton.Get(),
		CancelInterfaceSettingsButton.Get(),
		BackFromCreditsButton.Get()
	};

	for (UButton* Button : ButtonsToUpdate)
	{
		if (Button)
		{
			Button->SetIsEnabled(bEnabled);
		}
	}
}
