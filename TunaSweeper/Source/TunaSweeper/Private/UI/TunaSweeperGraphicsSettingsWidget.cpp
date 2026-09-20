#include "UI/TunaSweeperGraphicsSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "DLSSLibrary.h"
#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "HAL/IConsoleManager.h"
#include "RHIGlobals.h"
#include "UI/TunaSweeperGraphicsQualityRowWidget.h"
#include "UI/TunaSweeperOptionRowWidget.h"
#include "UI/TunaSweeperUIStyle.h"

namespace TunaSweeperGraphicsSettingsWidget
{
	constexpr float ResolutionConfirmationDuration = 15.0f;

	int32& GetQuality(Scalability::FQualityLevels& Levels, ETunaSweeperScalabilityOption Option)
	{
		switch (Option)
		{
		case ETunaSweeperScalabilityOption::Texture: return Levels.TextureQuality;
		case ETunaSweeperScalabilityOption::Shadow: return Levels.ShadowQuality;
		case ETunaSweeperScalabilityOption::GlobalIllumination: return Levels.GlobalIlluminationQuality;
		case ETunaSweeperScalabilityOption::Reflection: return Levels.ReflectionQuality;
		case ETunaSweeperScalabilityOption::ViewDistance: return Levels.ViewDistanceQuality;
		case ETunaSweeperScalabilityOption::Effects: return Levels.EffectsQuality;
		case ETunaSweeperScalabilityOption::PostProcess: return Levels.PostProcessQuality;
		case ETunaSweeperScalabilityOption::Foliage: return Levels.FoliageQuality;
		case ETunaSweeperScalabilityOption::Shading: return Levels.ShadingQuality;
		case ETunaSweeperScalabilityOption::Landscape: return Levels.LandscapeQuality;
		case ETunaSweeperScalabilityOption::AntiAliasing:
		default: return Levels.AntiAliasingQuality;
		}
	}

	int32 GetQuality(const Scalability::FQualityLevels& Levels, ETunaSweeperScalabilityOption Option)
	{
		Scalability::FQualityLevels Copy = Levels;
		return GetQuality(Copy, Option);
	}

	bool QualityLevelsEqual(const Scalability::FQualityLevels& Left, const Scalability::FQualityLevels& Right)
	{
		return FMath::IsNearlyEqual(Left.ResolutionQuality, Right.ResolutionQuality) &&
			Left.ViewDistanceQuality == Right.ViewDistanceQuality &&
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

	bool StatesEqual(const FTunaSweeperGraphicsSettingsState& Left, const FTunaSweeperGraphicsSettingsState& Right)
	{
		return QualityLevelsEqual(Left.QualityLevels, Right.QualityLevels) &&
			Left.Resolution == Right.Resolution && Left.WindowMode == Right.WindowMode &&
			Left.Preset == Right.Preset && Left.DLSSMode == Right.DLSSMode &&
			FMath::IsNearlyEqual(Left.FrameRateLimit, Right.FrameRateLimit) &&
			Left.bVSyncEnabled == Right.bVSyncEnabled &&
			Left.bDynamicResolutionEnabled == Right.bDynamicResolutionEnabled &&
			Left.bMotionBlurEnabled == Right.bMotionBlurEnabled &&
			Left.bHardwareRayTracingEnabled == Right.bHardwareRayTracingEnabled;
	}

	UDLSSMode ToRuntimeDLSSMode(ETunaSweeperTitleDLSSMode Mode)
	{
		switch (Mode)
		{
		case ETunaSweeperTitleDLSSMode::Quality: return UDLSSMode::Quality;
		case ETunaSweeperTitleDLSSMode::Balanced: return UDLSSMode::Balanced;
		case ETunaSweeperTitleDLSSMode::Performance: return UDLSSMode::Performance;
		case ETunaSweeperTitleDLSSMode::Off:
		default: return UDLSSMode::Off;
		}
	}
}

TArray<FIntPoint> TunaSweeperGraphicsSettingsOptions::BuildResolutionCandidates(
	const FIntPoint& Pending,
	const FIntPoint& Applied)
{
	TArray<FIntPoint> Resolutions = {
		FIntPoint(1280, 720),
		FIntPoint(1600, 900),
		FIntPoint(1920, 1080),
		FIntPoint(2560, 1440),
		FIntPoint(3840, 2160)
	};
	Resolutions.AddUnique(Applied);
	Resolutions.AddUnique(Pending);
	Resolutions.Sort([](const FIntPoint& Left, const FIntPoint& Right)
	{
		const int64 LeftPixels = static_cast<int64>(Left.X) * Left.Y;
		const int64 RightPixels = static_cast<int64>(Right.X) * Right.Y;
		return LeftPixels == RightPixels ? Left.X < Right.X : LeftPixels < RightPixels;
	});
	return Resolutions;
}

TArray<float> TunaSweeperGraphicsSettingsOptions::BuildFrameRateCandidates(float Pending, float Applied)
{
	TArray<float> FrameRates = { 0.0f, 60.0f, 120.0f, 144.0f };
	auto AddUniqueNearlyEqual = [&FrameRates](float Value)
	{
		if (!FrameRates.ContainsByPredicate([Value](float Candidate) { return FMath::IsNearlyEqual(Candidate, Value); }))
		{
			FrameRates.Add(Value);
		}
	};
	AddUniqueNearlyEqual(Applied);
	AddUniqueNearlyEqual(Pending);
	FrameRates.Sort();
	return FrameRates;
}

TSharedRef<SWidget> UTunaSweeperGraphicsSettingsWidget::RebuildWidget()
{
	BuildRuntimeWidgetTree();
	AdaptAuthoredWidgetTree();
	return Super::RebuildWidget();
}

void UTunaSweeperGraphicsSettingsWidget::BuildRuntimeWidgetTree()
{
	if (WidgetTree && WidgetTree->RootWidget)
	{
		return;
	}
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
	}

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphicsSettingsRoot"));
	WidgetTree->RootWidget = Root;
	GraphicsStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GraphicsStatusText"));
	TunaSweeperUIStyle::ApplyLabel(GraphicsStatusText, 14);
	Root->AddChildToVerticalBox(GraphicsStatusText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 5.0f));

	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("GraphicsSettingsScroll"));
	Scroll->SetScrollBarVisibility(ESlateVisibility::Visible);
	Scroll->SetScrollbarThickness(FVector2D(6.0f, 6.0f));
	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphicsSettingsContent"));
	Scroll->AddChild(Content);
	if (UVerticalBoxSlot* GraphicsScrollSlot = Root->AddChildToVerticalBox(Scroll))
	{
		GraphicsScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	auto AddHeading = [this, Content](const TCHAR* Name)
	{
		UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TunaSweeperUIStyle::ApplyLabel(Heading, 15);
		if (UVerticalBoxSlot* HeadingSlot = Content->AddChildToVerticalBox(Heading))
		{
			HeadingSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 2.0f));
		}
	};
	auto AddOptionRow = [this, Content](const TCHAR* Name, TObjectPtr<UTunaSweeperOptionRowWidget>& OutRow)
	{
		OutRow = WidgetTree->ConstructWidget<UTunaSweeperOptionRowWidget>(UTunaSweeperOptionRowWidget::StaticClass(), Name);
		if (UVerticalBoxSlot* RowSlot = Content->AddChildToVerticalBox(OutRow))
		{
			RowSlot->SetPadding(FMargin(0.0f, 2.0f));
		}
	};
	auto AddButtonRow = [this, Content](const TArray<UWidget*>& Buttons)
	{
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
		for (UWidget* Widget : Buttons)
		{
			if (UHorizontalBoxSlot* ButtonSlot = Row->AddChildToHorizontalBox(Widget))
			{
				ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 7.0f, 0.0f));
			}
		}
		if (UVerticalBoxSlot* ButtonRowSlot = Content->AddChildToVerticalBox(Row))
		{
			ButtonRowSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 8.0f));
		}
	};

#define MAKE_GRAPHICS_BUTTON(Name, Width) \
	Name = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT(#Name)); \
	Name##Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT(#Name "Text")); \
	Name##Text->SetJustification(ETextJustify::Center); \
	TunaSweeperUIStyle::ApplyLabel(Name##Text, 13); \
	Name->SetContent(Name##Text); \
	USizeBox* Name##Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT(#Name "Box")); \
	Name##Box->SetWidthOverride(Width); \
	Name##Box->SetHeightOverride(44.0f); \
	Name##Box->SetContent(Name)

	AddOptionRow(TEXT("PresetOptionRow"), PresetOptionRow);
	AddOptionRow(TEXT("WindowModeOptionRow"), WindowModeOptionRow);
	AddOptionRow(TEXT("ResolutionOptionRow"), ResolutionOptionRow);
	AddOptionRow(TEXT("DLSSOptionRow"), DLSSOptionRow);

	AddHeading(TEXT("QualityHeading"));
#define MAKE_QUALITY_ROW(Name) \
	Name = WidgetTree->ConstructWidget<UTunaSweeperGraphicsQualityRowWidget>(UTunaSweeperGraphicsQualityRowWidget::StaticClass(), TEXT(#Name)); \
	Content->AddChildToVerticalBox(Name)->SetPadding(FMargin(0.0f, 2.0f))
	MAKE_QUALITY_ROW(TextureQualityRow);
	MAKE_QUALITY_ROW(ShadowQualityRow);
	MAKE_QUALITY_ROW(GlobalIlluminationQualityRow);
	MAKE_QUALITY_ROW(ReflectionQualityRow);
	MAKE_QUALITY_ROW(ViewDistanceQualityRow);
	MAKE_QUALITY_ROW(EffectsQualityRow);
	MAKE_QUALITY_ROW(PostProcessQualityRow);
	MAKE_QUALITY_ROW(FoliageQualityRow);
	MAKE_QUALITY_ROW(ShadingQualityRow);
	MAKE_QUALITY_ROW(LandscapeQualityRow);
	MAKE_QUALITY_ROW(AntiAliasingQualityRow);
#undef MAKE_QUALITY_ROW

	AddHeading(TEXT("PerformanceHeading"));
	MAKE_GRAPHICS_BUTTON(VSyncToggleButton, 260.0f);
	MAKE_GRAPHICS_BUTTON(MotionBlurToggleButton, 260.0f);
	MAKE_GRAPHICS_BUTTON(DynamicResolutionToggleButton, 260.0f);
	MAKE_GRAPHICS_BUTTON(HardwareRayTracingToggleButton, 300.0f);
	AddButtonRow({ VSyncToggleButtonBox, MotionBlurToggleButtonBox });
	AddButtonRow({ DynamicResolutionToggleButtonBox, HardwareRayTracingToggleButtonBox });
	AddOptionRow(TEXT("FrameRateOptionRow"), FrameRateOptionRow);

	MAKE_GRAPHICS_BUTTON(ApplyGraphicsSettingsButton, 145.0f);
	MAKE_GRAPHICS_BUTTON(CancelGraphicsSettingsButton, 145.0f);
	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("GraphicsActionRow"));
	ActionRow->AddChildToHorizontalBox(ApplyGraphicsSettingsButtonBox)->SetPadding(FMargin(0.0f, 8.0f, 10.0f, 0.0f));
	ActionRow->AddChildToHorizontalBox(CancelGraphicsSettingsButtonBox)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	Root->AddChildToVerticalBox(ActionRow);

	MAKE_GRAPHICS_BUTTON(ConfirmResolutionButton, 90.0f);
	MAKE_GRAPHICS_BUTTON(RevertResolutionButton, 110.0f);
#undef MAKE_GRAPHICS_BUTTON
	UBorder* ConfirmationBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResolutionConfirmationPanel"));
	ResolutionConfirmationPanel = ConfirmationBorder;
	ConfirmationBorder->SetPadding(FMargin(10.0f));
	ConfirmationBorder->SetBrushColor(FLinearColor(0.12f, 0.075f, 0.035f, 0.98f));
	UHorizontalBox* ConfirmationRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	ResolutionConfirmationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResolutionConfirmationText"));
	TunaSweeperUIStyle::ApplyLabel(ResolutionConfirmationText, 13);
	ConfirmationRow->AddChildToHorizontalBox(ResolutionConfirmationText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ConfirmationRow->AddChildToHorizontalBox(ConfirmResolutionButtonBox)->SetPadding(FMargin(8.0f, 0.0f));
	ConfirmationRow->AddChildToHorizontalBox(RevertResolutionButtonBox);
	ConfirmationBorder->SetContent(ConfirmationRow);
	ConfirmationBorder->SetVisibility(ESlateVisibility::Collapsed);
	Root->AddChildToVerticalBox(ConfirmationBorder)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
}

void UTunaSweeperGraphicsSettingsWidget::AdaptAuthoredWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	auto ReplaceChoiceRow = [this](TObjectPtr<UTunaSweeperOptionRowWidget>& OutRow, const TCHAR* RowName, const TCHAR* AnchorBoxName, const TCHAR* HeadingName)
	{
		if (OutRow)
		{
			return;
		}
		UWidget* AnchorBox = WidgetTree->FindWidget(FName(AnchorBoxName));
		UPanelWidget* OldRow = AnchorBox ? Cast<UPanelWidget>(AnchorBox->GetParent()) : nullptr;
		UPanelWidget* Container = OldRow ? Cast<UPanelWidget>(OldRow->GetParent()) : nullptr;
		if (!Container)
		{
			return;
		}

		const int32 InsertIndex = Container->GetChildIndex(OldRow);
		Container->RemoveChild(OldRow);
		OutRow = WidgetTree->ConstructWidget<UTunaSweeperOptionRowWidget>(UTunaSweeperOptionRowWidget::StaticClass(), RowName);
		UPanelSlot* NewSlot = Container->InsertChildAt(InsertIndex, OutRow);
		if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(NewSlot))
		{
			VerticalSlot->SetPadding(FMargin(0.0f, 2.0f));
		}
		if (HeadingName && HeadingName[0] != TCHAR('\0'))
		{
			if (UWidget* Heading = WidgetTree->FindWidget(FName(HeadingName)))
			{
				Heading->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	};

	ReplaceChoiceRow(PresetOptionRow, TEXT("PresetOptionRow"), TEXT("PresetAutoButtonBox"), TEXT("PresetHeading"));
	ReplaceChoiceRow(WindowModeOptionRow, TEXT("WindowModeOptionRow"), TEXT("WindowedModeButtonBox"), TEXT("WindowHeading"));
	ReplaceChoiceRow(ResolutionOptionRow, TEXT("ResolutionOptionRow"), TEXT("Resolution1280ButtonBox"), TEXT("ResolutionHeading"));
	ReplaceChoiceRow(DLSSOptionRow, TEXT("DLSSOptionRow"), TEXT("DLSSOffButtonBox"), TEXT("DLSSHeading"));
	ReplaceChoiceRow(FrameRateOptionRow, TEXT("FrameRateOptionRow"), TEXT("FrameRateUnlimitedButtonBox"), nullptr);

	USizeBox* VSyncBox = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("VSyncToggleButtonBox")));
	UHorizontalBox* OldToggleRow = VSyncBox ? Cast<UHorizontalBox>(VSyncBox->GetParent()) : nullptr;
	UPanelWidget* ToggleContainer = OldToggleRow ? Cast<UPanelWidget>(OldToggleRow->GetParent()) : nullptr;
	if (VSyncBox && OldToggleRow && ToggleContainer && OldToggleRow->GetFName() != FName(TEXT("GraphicsToggleRow1")))
	{
		USizeBox* MotionBlurBox = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("MotionBlurToggleButtonBox")));
		USizeBox* DynamicResolutionBox = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("DynamicResolutionToggleButtonBox")));
		USizeBox* RayTracingBox = Cast<USizeBox>(WidgetTree->FindWidget(TEXT("HardwareRayTracingToggleButtonBox")));
		if (MotionBlurBox && DynamicResolutionBox && RayTracingBox &&
			MotionBlurBox->GetParent() == OldToggleRow &&
			DynamicResolutionBox->GetParent() == OldToggleRow &&
			RayTracingBox->GetParent() == OldToggleRow)
		{
			const int32 InsertIndex = ToggleContainer->GetChildIndex(OldToggleRow);
			for (USizeBox* Box : { VSyncBox, MotionBlurBox, DynamicResolutionBox, RayTracingBox })
			{
				OldToggleRow->RemoveChild(Box);
				Box->SetWidthOverride(300.0f);
				Box->SetHeightOverride(44.0f);
			}
			ToggleContainer->RemoveChild(OldToggleRow);

			UVerticalBox* ToggleRows = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphicsToggleRows"));
			UHorizontalBox* FirstRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("GraphicsToggleRow1"));
			UHorizontalBox* SecondRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("GraphicsToggleRow2"));
			FirstRow->AddChildToHorizontalBox(VSyncBox)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 4.0f));
			FirstRow->AddChildToHorizontalBox(MotionBlurBox)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
			SecondRow->AddChildToHorizontalBox(DynamicResolutionBox)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
			SecondRow->AddChildToHorizontalBox(RayTracingBox);
			ToggleRows->AddChildToVerticalBox(FirstRow);
			ToggleRows->AddChildToVerticalBox(SecondRow);
			if (UVerticalBoxSlot* AddedSlot = Cast<UVerticalBoxSlot>(ToggleContainer->InsertChildAt(InsertIndex, ToggleRows)))
			{
				AddedSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 8.0f));
			}
		}
	}
}

void UTunaSweeperGraphicsSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	AdaptAuthoredWidgetTree();
	BindButtons();
	ConfigureOptionRows();
	ConfigureQualityRows();
	if (ResolutionConfirmationPanel)
	{
		ResolutionConfirmationPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	RefreshFromSettings();
}

void UTunaSweeperGraphicsSettingsWidget::NativeDestruct()
{
	if (bResolutionConfirmationActive)
	{
		RevertResolution();
	}
	for (UTunaSweeperGraphicsQualityRowWidget* Row : {
		TextureQualityRow.Get(), ShadowQualityRow.Get(), GlobalIlluminationQualityRow.Get(),
		ReflectionQualityRow.Get(), ViewDistanceQualityRow.Get(), EffectsQualityRow.Get(),
		PostProcessQualityRow.Get(), FoliageQualityRow.Get(), ShadingQualityRow.Get(),
		LandscapeQualityRow.Get(), AntiAliasingQualityRow.Get() })
	{
		if (Row)
		{
			Row->OnQualityStepRequested.RemoveAll(this);
		}
	}
	for (UTunaSweeperOptionRowWidget* Row : {
		PresetOptionRow.Get(), WindowModeOptionRow.Get(), ResolutionOptionRow.Get(),
		DLSSOptionRow.Get(), FrameRateOptionRow.Get() })
	{
		if (Row)
		{
			Row->OnStepRequested.RemoveAll(this);
		}
	}
	Super::NativeDestruct();
}

void UTunaSweeperGraphicsSettingsWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bResolutionConfirmationActive)
	{
		return;
	}

	ResolutionConfirmationSecondsRemaining -= InDeltaTime;
	if (ResolutionConfirmationText)
	{
		ResolutionConfirmationText->SetText(FText::Format(
			ResolveUiText(FName(TEXT("ui.settings.resolution_confirm_countdown"))),
			FText::AsNumber(FMath::Max(0, FMath::CeilToInt(ResolutionConfirmationSecondsRemaining)))));
	}
	if (ResolutionConfirmationSecondsRemaining <= 0.0f)
	{
		RevertResolution();
	}
}

void UTunaSweeperGraphicsSettingsWidget::RefreshFromSettings()
{
	if (bResolutionConfirmationActive)
	{
		return;
	}
	if (const UTunaSweeperGameUserSettings* Settings = UTunaSweeperGameUserSettings::Get())
	{
		AppliedState = Settings->CaptureGraphicsState();
		PendingState = AppliedState;
		bHasSettingsSnapshot = true;
	}
	RefreshVisualState();
}

void UTunaSweeperGraphicsSettingsWidget::DiscardPendingChanges()
{
	if (bResolutionConfirmationActive)
	{
		RevertResolution();
		return;
	}
	if (bHasSettingsSnapshot)
	{
		PendingState = AppliedState;
		RefreshVisualState();
	}
}

bool UTunaSweeperGraphicsSettingsWidget::CancelResolutionConfirmation()
{
	if (!bResolutionConfirmationActive) return false;
	RevertResolution();
	return true;
}

bool UTunaSweeperGraphicsSettingsWidget::HasPendingChanges() const
{
	return bHasSettingsSnapshot &&
		!TunaSweeperGraphicsSettingsWidget::StatesEqual(AppliedState, PendingState);
}

void UTunaSweeperGraphicsSettingsWidget::BindButtons()
{
#define BIND_GRAPHICS_BUTTON(Button, Handler) \
	if (Button) { Button->OnClicked.RemoveDynamic(this, &UTunaSweeperGraphicsSettingsWidget::Handler); Button->OnClicked.AddDynamic(this, &UTunaSweeperGraphicsSettingsWidget::Handler); }
	BIND_GRAPHICS_BUTTON(VSyncToggleButton, HandleVSyncToggleClicked);
	BIND_GRAPHICS_BUTTON(MotionBlurToggleButton, HandleMotionBlurToggleClicked);
	BIND_GRAPHICS_BUTTON(DynamicResolutionToggleButton, HandleDynamicResolutionToggleClicked);
	BIND_GRAPHICS_BUTTON(HardwareRayTracingToggleButton, HandleHardwareRayTracingToggleClicked);
	BIND_GRAPHICS_BUTTON(ApplyGraphicsSettingsButton, HandleApplyClicked);
	BIND_GRAPHICS_BUTTON(CancelGraphicsSettingsButton, HandleCancelClicked);
	BIND_GRAPHICS_BUTTON(ConfirmResolutionButton, HandleConfirmResolutionClicked);
	BIND_GRAPHICS_BUTTON(RevertResolutionButton, HandleRevertResolutionClicked);
#undef BIND_GRAPHICS_BUTTON

	TunaSweeperUIStyle::ApplyButton(ApplyGraphicsSettingsButton, TunaSweeperUIStyle::EButtonRole::Primary);
	TunaSweeperUIStyle::ApplyButton(CancelGraphicsSettingsButton, TunaSweeperUIStyle::EButtonRole::Secondary);
	TunaSweeperUIStyle::ApplyButton(ConfirmResolutionButton, TunaSweeperUIStyle::EButtonRole::Primary);
	TunaSweeperUIStyle::ApplyButton(RevertResolutionButton, TunaSweeperUIStyle::EButtonRole::Danger);
}

void UTunaSweeperGraphicsSettingsWidget::ConfigureOptionRows()
{
	struct FRowBinding
	{
		UTunaSweeperOptionRowWidget* Row;
		FName LabelKey;
		void (UTunaSweeperGraphicsSettingsWidget::*Handler)(int32);
	};

	const FRowBinding Rows[] = {
		{ PresetOptionRow, FName(TEXT("ui.settings.preset")), &UTunaSweeperGraphicsSettingsWidget::HandlePresetStepRequested },
		{ WindowModeOptionRow, FName(TEXT("ui.settings.window_mode")), &UTunaSweeperGraphicsSettingsWidget::HandleWindowModeStepRequested },
		{ ResolutionOptionRow, FName(TEXT("ui.settings.resolution")), &UTunaSweeperGraphicsSettingsWidget::HandleResolutionStepRequested },
		{ DLSSOptionRow, FName(TEXT("ui.settings.dlss")), &UTunaSweeperGraphicsSettingsWidget::HandleDLSSStepRequested },
		{ FrameRateOptionRow, FName(TEXT("ui.settings.frame_rate")), &UTunaSweeperGraphicsSettingsWidget::HandleFrameRateStepRequested }
	};

	for (const FRowBinding& Binding : Rows)
	{
		if (Binding.Row)
		{
			Binding.Row->Configure(ResolveUiText(Binding.LabelKey));
			Binding.Row->OnStepRequested.RemoveAll(this);
			Binding.Row->OnStepRequested.AddUObject(this, Binding.Handler);
		}
	}
}

void UTunaSweeperGraphicsSettingsWidget::ConfigureQualityRows()
{
	struct FRowDefinition
	{
		UTunaSweeperGraphicsQualityRowWidget* Row;
		ETunaSweeperScalabilityOption Option;
		const TCHAR* Key;
	};

	const FRowDefinition Rows[] = {
		{ TextureQualityRow, ETunaSweeperScalabilityOption::Texture, TEXT("ui.settings.texture") },
		{ ShadowQualityRow, ETunaSweeperScalabilityOption::Shadow, TEXT("ui.settings.shadow") },
		{ GlobalIlluminationQualityRow, ETunaSweeperScalabilityOption::GlobalIllumination, TEXT("ui.settings.global_illumination") },
		{ ReflectionQualityRow, ETunaSweeperScalabilityOption::Reflection, TEXT("ui.settings.reflection") },
		{ ViewDistanceQualityRow, ETunaSweeperScalabilityOption::ViewDistance, TEXT("ui.settings.view_distance") },
		{ EffectsQualityRow, ETunaSweeperScalabilityOption::Effects, TEXT("ui.settings.effects") },
		{ PostProcessQualityRow, ETunaSweeperScalabilityOption::PostProcess, TEXT("ui.settings.post_process") },
		{ FoliageQualityRow, ETunaSweeperScalabilityOption::Foliage, TEXT("ui.settings.foliage") },
		{ ShadingQualityRow, ETunaSweeperScalabilityOption::Shading, TEXT("ui.settings.shading") },
		{ LandscapeQualityRow, ETunaSweeperScalabilityOption::Landscape, TEXT("ui.settings.landscape") },
		{ AntiAliasingQualityRow, ETunaSweeperScalabilityOption::AntiAliasing, TEXT("ui.settings.anti_aliasing") }
	};

	for (const FRowDefinition& Definition : Rows)
	{
		if (Definition.Row)
		{
			Definition.Row->Configure(Definition.Option, ResolveUiText(FName(Definition.Key)));
			Definition.Row->OnQualityStepRequested.RemoveAll(this);
			Definition.Row->OnQualityStepRequested.AddUObject(this, &UTunaSweeperGraphicsSettingsWidget::HandleQualityStepRequested);
		}
	}
}

void UTunaSweeperGraphicsSettingsWidget::RefreshVisualState()
{
	if (WidgetTree) if (UTextBlock* Header = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("GraphicsSectionTitleText"))))
		Header->SetText(ResolveUiText(FName(TEXT("ui.settings.graphics"))));
	ConfigureQualityRows();
	if (!bHasSettingsSnapshot)
	{
		return;
	}
	if (WidgetTree)
	{
		if (UTextBlock* Heading = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("QualityHeading"))))
		{
			Heading->SetText(ResolveUiText(FName(TEXT("ui.settings.individual_quality"))));
			TunaSweeperUIStyle::ApplyLabel(Heading, 15);
		}
		if (UTextBlock* Heading = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("PerformanceHeading"))))
		{
			Heading->SetText(ResolveUiText(FName(TEXT("ui.settings.performance_effects"))));
			TunaSweeperUIStyle::ApplyLabel(Heading, 15);
		}
	}

	const UTunaSweeperGameUserSettings* Settings = UTunaSweeperGameUserSettings::Get();
	if (GraphicsStatusText && Settings)
	{
		FText Status = BuildPresetText(PendingState.Preset);
		if (PendingState.Preset == ETunaSweeperGraphicsPreset::Auto)
		{
			Status = FText::Format(
				ResolveUiText(FName(TEXT("ui.settings.preset.auto_details"))),
				Status,
				BuildPresetText(Settings->GetResolvedAutoGraphicsPreset()),
				FText::AsNumber(Settings->GetLastDetectedDedicatedVideoMemoryMB()));
		}
		if (HasPendingChanges())
		{
			Status = FText::FromString(Status.ToString() + ResolveUiText(FName(TEXT("ui.settings.pending_suffix"))).ToString());
		}
		GraphicsStatusText->SetText(Status);
	}

	RefreshOptionRows();

	if (VSyncToggleButtonText) VSyncToggleButtonText->SetText(ResolveUiText(FName(TEXT("ui.settings.vsync"))));
	if (MotionBlurToggleButtonText) MotionBlurToggleButtonText->SetText(ResolveUiText(FName(TEXT("ui.settings.motion_blur"))));
	if (DynamicResolutionToggleButtonText) DynamicResolutionToggleButtonText->SetText(ResolveUiText(FName(TEXT("ui.settings.dynamic_resolution"))));
	if (HardwareRayTracingToggleButtonText) HardwareRayTracingToggleButtonText->SetText(ResolveUiText(FName(TEXT("ui.settings.hardware_ray_tracing"))));
	TunaSweeperUIStyle::SetCheckButton(WidgetTree, VSyncToggleButton, VSyncToggleButtonText, PendingState.bVSyncEnabled);
	TunaSweeperUIStyle::SetCheckButton(WidgetTree, MotionBlurToggleButton, MotionBlurToggleButtonText, PendingState.bMotionBlurEnabled);
	TunaSweeperUIStyle::SetCheckButton(WidgetTree, DynamicResolutionToggleButton, DynamicResolutionToggleButtonText, PendingState.bDynamicResolutionEnabled);
	TunaSweeperUIStyle::SetCheckButton(WidgetTree, HardwareRayTracingToggleButton, HardwareRayTracingToggleButtonText, PendingState.bHardwareRayTracingEnabled);
	if (HardwareRayTracingToggleButton) HardwareRayTracingToggleButton->SetIsEnabled(GRHISupportsRayTracing);

	if (ApplyGraphicsSettingsButtonText) ApplyGraphicsSettingsButtonText->SetText(ResolveUiText(FName(TEXT("ui.common.apply"))));
	if (CancelGraphicsSettingsButtonText) CancelGraphicsSettingsButtonText->SetText(ResolveUiText(FName(TEXT("ui.common.cancel"))));
	if (ConfirmResolutionButtonText) ConfirmResolutionButtonText->SetText(ResolveUiText(FName(TEXT("ui.common.keep"))));
	if (RevertResolutionButtonText) RevertResolutionButtonText->SetText(ResolveUiText(FName(TEXT("ui.common.revert"))));
	if (ApplyGraphicsSettingsButton) ApplyGraphicsSettingsButton->SetIsEnabled(HasPendingChanges() && !bResolutionConfirmationActive);
	if (CancelGraphicsSettingsButton) CancelGraphicsSettingsButton->SetIsEnabled(HasPendingChanges() && !bResolutionConfirmationActive);

	RefreshQualityRows();
}

void UTunaSweeperGraphicsSettingsWidget::RefreshOptionRows()
{
	ConfigureOptionRows();

	if (PresetOptionRow)
	{
		PresetOptionRow->SetValue(BuildPresetText(PendingState.Preset));
		if (PendingState.Preset == ETunaSweeperGraphicsPreset::Custom)
		{
			PresetOptionRow->SetStepEnabled(true, true);
		}
		else
		{
			const int32 Index = static_cast<int32>(PendingState.Preset);
			PresetOptionRow->SetStepEnabled(Index > 0, Index < static_cast<int32>(ETunaSweeperGraphicsPreset::Epic));
		}
	}

	const TArray<EWindowMode::Type> WindowModes = {
		EWindowMode::Windowed,
		EWindowMode::WindowedFullscreen,
		EWindowMode::Fullscreen
	};
	if (WindowModeOptionRow)
	{
		const int32 Index = WindowModes.IndexOfByKey(PendingState.WindowMode);
		WindowModeOptionRow->SetValue(BuildWindowModeText(PendingState.WindowMode));
		WindowModeOptionRow->SetStepEnabled(Index > 0, Index != INDEX_NONE && Index < WindowModes.Num() - 1);
	}

	const TArray<FIntPoint> Resolutions = TunaSweeperGraphicsSettingsOptions::BuildResolutionCandidates(
		PendingState.Resolution,
		AppliedState.Resolution);
	if (ResolutionOptionRow)
	{
		const int32 Index = Resolutions.IndexOfByKey(PendingState.Resolution);
		ResolutionOptionRow->SetValue(BuildResolutionText(PendingState.Resolution));
		ResolutionOptionRow->SetStepEnabled(Index > 0, Index != INDEX_NONE && Index < Resolutions.Num() - 1);
	}

	TArray<ETunaSweeperTitleDLSSMode> DLSSModes;
	for (ETunaSweeperTitleDLSSMode Mode : {
		ETunaSweeperTitleDLSSMode::Off,
		ETunaSweeperTitleDLSSMode::Quality,
		ETunaSweeperTitleDLSSMode::Balanced,
		ETunaSweeperTitleDLSSMode::Performance })
	{
		if (IsDLSSModeAvailable(Mode) || Mode == PendingState.DLSSMode)
		{
			DLSSModes.Add(Mode);
		}
	}
	if (DLSSOptionRow)
	{
		const int32 Index = DLSSModes.IndexOfByKey(PendingState.DLSSMode);
		FText DisplayText = BuildDLSSModeText(PendingState.DLSSMode);
		if (!IsDLSSModeAvailable(PendingState.DLSSMode))
		{
			DisplayText = FText::Format(ResolveUiText(FName(TEXT("ui.settings.unavailable_suffix"))), DisplayText);
		}
		DLSSOptionRow->SetValue(DisplayText);
		DLSSOptionRow->SetStepEnabled(Index > 0, Index != INDEX_NONE && Index < DLSSModes.Num() - 1);
	}

	const TArray<float> FrameRates = TunaSweeperGraphicsSettingsOptions::BuildFrameRateCandidates(
		PendingState.FrameRateLimit,
		AppliedState.FrameRateLimit);
	if (FrameRateOptionRow)
	{
		const int32 Index = FrameRates.IndexOfByPredicate([this](float Value) { return FMath::IsNearlyEqual(Value, PendingState.FrameRateLimit); });
		FrameRateOptionRow->SetValue(BuildFrameRateText(PendingState.FrameRateLimit));
		FrameRateOptionRow->SetStepEnabled(Index > 0, Index != INDEX_NONE && Index < FrameRates.Num() - 1);
	}
}

void UTunaSweeperGraphicsSettingsWidget::RefreshQualityRows()
{
	struct FRow { UTunaSweeperGraphicsQualityRowWidget* Widget; ETunaSweeperScalabilityOption Option; };
	const FRow Rows[] = {
		{ TextureQualityRow, ETunaSweeperScalabilityOption::Texture }, { ShadowQualityRow, ETunaSweeperScalabilityOption::Shadow },
		{ GlobalIlluminationQualityRow, ETunaSweeperScalabilityOption::GlobalIllumination }, { ReflectionQualityRow, ETunaSweeperScalabilityOption::Reflection },
		{ ViewDistanceQualityRow, ETunaSweeperScalabilityOption::ViewDistance }, { EffectsQualityRow, ETunaSweeperScalabilityOption::Effects },
		{ PostProcessQualityRow, ETunaSweeperScalabilityOption::PostProcess }, { FoliageQualityRow, ETunaSweeperScalabilityOption::Foliage },
		{ ShadingQualityRow, ETunaSweeperScalabilityOption::Shading }, { LandscapeQualityRow, ETunaSweeperScalabilityOption::Landscape },
		{ AntiAliasingQualityRow, ETunaSweeperScalabilityOption::AntiAliasing }
	};
	for (const FRow& Row : Rows)
	{
		if (Row.Widget)
		{
			const int32 Quality = TunaSweeperGraphicsSettingsWidget::GetQuality(PendingState.QualityLevels, Row.Option);
			Row.Widget->SetQualityLevel(Quality, BuildQualityText(Quality));
			const bool bCanEdit = Row.Option != ETunaSweeperScalabilityOption::AntiAliasing || PendingState.DLSSMode == ETunaSweeperTitleDLSSMode::Off;
			Row.Widget->SetIsEnabled(bCanEdit);
			Row.Widget->SetStepEnabled(bCanEdit && Quality > 0, bCanEdit && Quality < 3);
		}
	}
}

void UTunaSweeperGraphicsSettingsWidget::SelectPreset(ETunaSweeperGraphicsPreset Preset)
{
	PendingState.Preset = Preset;
	ETunaSweeperGraphicsPreset QualityPreset = Preset;
	if (Preset == ETunaSweeperGraphicsPreset::Auto)
	{
		if (UTunaSweeperGameUserSettings* Settings = UTunaSweeperGameUserSettings::Get())
		{
			QualityPreset = Settings->RefreshAutoDetection();
		}
		else
		{
			QualityPreset = ETunaSweeperGraphicsPreset::Low;
		}
	}
	if (QualityPreset != ETunaSweeperGraphicsPreset::Custom)
	{
		PendingState.QualityLevels = UTunaSweeperGameUserSettings::BuildQualityLevelsForPreset(QualityPreset, PendingState.QualityLevels.ResolutionQuality);
	}
	RefreshVisualState();
}

void UTunaSweeperGraphicsSettingsWidget::SetWindowMode(EWindowMode::Type WindowMode)
{
	PendingState.WindowMode = WindowMode;
	RefreshVisualState();
}

void UTunaSweeperGraphicsSettingsWidget::SetResolution(const FIntPoint& Resolution)
{
	PendingState.Resolution = Resolution;
	RefreshVisualState();
}

void UTunaSweeperGraphicsSettingsWidget::SetDLSSMode(ETunaSweeperTitleDLSSMode Mode)
{
	if (IsDLSSModeAvailable(Mode))
	{
		PendingState.DLSSMode = Mode;
		RefreshVisualState();
	}
}

void UTunaSweeperGraphicsSettingsWidget::SetFrameRateLimit(float FrameRateLimit)
{
	PendingState.FrameRateLimit = FrameRateLimit;
	RefreshVisualState();
}

void UTunaSweeperGraphicsSettingsWidget::ApplyDLSSModeToRuntime(ETunaSweeperTitleDLSSMode Mode) const
{
	if (!IsDLSSModeAvailable(Mode))
	{
		Mode = ETunaSweeperTitleDLSSMode::Off;
	}
	UDLSSLibrary::SetDLSSMode(GetWorld(), TunaSweeperGraphicsSettingsWidget::ToRuntimeDLSSMode(Mode));
	if (Mode == ETunaSweeperTitleDLSSMode::Off)
	{
		if (IConsoleVariable* ScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage")))
		{
			ScreenPercentage->Set(100.0f, ECVF_SetByGameSetting);
		}
	}
}

bool UTunaSweeperGraphicsSettingsWidget::IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode Mode) const
{
	return Mode == ETunaSweeperTitleDLSSMode::Off ||
		(UDLSSLibrary::IsDLSSSupported() && UDLSSLibrary::IsDLSSModeSupported(TunaSweeperGraphicsSettingsWidget::ToRuntimeDLSSMode(Mode)));
}

FText UTunaSweeperGraphicsSettingsWidget::ResolveUiText(FName StringKey) const
{
	if (const UTunaSweeperGameInstance* GameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		return GameInstance->ResolveLocalizedText(StringKey, FText::GetEmpty());
	}
	return FText::GetEmpty();
}

FText UTunaSweeperGraphicsSettingsWidget::BuildPresetText(ETunaSweeperGraphicsPreset Preset) const
{
	switch (Preset)
	{
	case ETunaSweeperGraphicsPreset::Auto: return ResolveUiText(FName(TEXT("ui.settings.preset.auto")));
	case ETunaSweeperGraphicsPreset::Low: return ResolveUiText(FName(TEXT("ui.settings.preset.low")));
	case ETunaSweeperGraphicsPreset::Medium: return ResolveUiText(FName(TEXT("ui.settings.preset.medium")));
	case ETunaSweeperGraphicsPreset::High: return ResolveUiText(FName(TEXT("ui.settings.preset.high")));
	case ETunaSweeperGraphicsPreset::Epic: return ResolveUiText(FName(TEXT("ui.settings.preset.epic")));
	case ETunaSweeperGraphicsPreset::Custom:
	default: return ResolveUiText(FName(TEXT("ui.settings.preset.custom")));
	}
}

FText UTunaSweeperGraphicsSettingsWidget::BuildQualityText(int32 Quality) const
{
	switch (FMath::Clamp(Quality, 0, 3))
	{
	case 0: return BuildPresetText(ETunaSweeperGraphicsPreset::Low);
	case 1: return BuildPresetText(ETunaSweeperGraphicsPreset::Medium);
	case 2: return BuildPresetText(ETunaSweeperGraphicsPreset::High);
	default: return BuildPresetText(ETunaSweeperGraphicsPreset::Epic);
	}
}

FText UTunaSweeperGraphicsSettingsWidget::BuildWindowModeText(EWindowMode::Type WindowMode) const
{
	switch (WindowMode)
	{
	case EWindowMode::Fullscreen: return ResolveUiText(FName(TEXT("ui.settings.fullscreen")));
	case EWindowMode::WindowedFullscreen: return ResolveUiText(FName(TEXT("ui.settings.borderless")));
	case EWindowMode::Windowed:
	default: return ResolveUiText(FName(TEXT("ui.settings.windowed")));
	}
}

FText UTunaSweeperGraphicsSettingsWidget::BuildDLSSModeText(ETunaSweeperTitleDLSSMode Mode) const
{
	switch (Mode)
	{
	case ETunaSweeperTitleDLSSMode::Quality: return ResolveUiText(FName(TEXT("ui.settings.dlss.quality")));
	case ETunaSweeperTitleDLSSMode::Balanced: return ResolveUiText(FName(TEXT("ui.settings.dlss.balanced")));
	case ETunaSweeperTitleDLSSMode::Performance: return ResolveUiText(FName(TEXT("ui.settings.dlss.performance")));
	case ETunaSweeperTitleDLSSMode::Off:
	default: return ResolveUiText(FName(TEXT("ui.settings.dlss.off")));
	}
}

FText UTunaSweeperGraphicsSettingsWidget::BuildResolutionText(const FIntPoint& Resolution) const
{
	return FText::Format(
		ResolveUiText(FName(TEXT("ui.settings.resolution_value"))),
		FText::AsNumber(Resolution.X, &FNumberFormattingOptions::DefaultNoGrouping()),
		FText::AsNumber(Resolution.Y, &FNumberFormattingOptions::DefaultNoGrouping()));
}

FText UTunaSweeperGraphicsSettingsWidget::BuildFrameRateText(float FrameRateLimit) const
{
	if (FrameRateLimit <= 0.0f)
	{
		return ResolveUiText(FName(TEXT("ui.settings.fps_unlimited")));
	}
	return FText::Format(
		ResolveUiText(FName(TEXT("ui.settings.fps_value"))),
		FText::AsNumber(FMath::RoundToInt(FrameRateLimit)));
}

void UTunaSweeperGraphicsSettingsWidget::BeginResolutionConfirmation()
{
	bResolutionConfirmationActive = true;
	ResolutionConfirmationSecondsRemaining = TunaSweeperGraphicsSettingsWidget::ResolutionConfirmationDuration;
	if (ResolutionConfirmationPanel)
	{
		ResolutionConfirmationPanel->SetVisibility(ESlateVisibility::Visible);
	}
	RefreshVisualState();
}

void UTunaSweeperGraphicsSettingsWidget::ConfirmResolution()
{
	if (!bResolutionConfirmationActive)
	{
		return;
	}
	if (UTunaSweeperGameUserSettings* Settings = UTunaSweeperGameUserSettings::Get())
	{
		Settings->ConfirmVideoMode();
		Settings->SaveSettings();
		AppliedState = Settings->CaptureGraphicsState();
		PendingState = AppliedState;
	}
	bResolutionConfirmationActive = false;
	if (ResolutionConfirmationPanel) ResolutionConfirmationPanel->SetVisibility(ESlateVisibility::Collapsed);
	RefreshVisualState();
}

void UTunaSweeperGraphicsSettingsWidget::RevertResolution()
{
	if (!bResolutionConfirmationActive)
	{
		return;
	}
	if (UTunaSweeperGameUserSettings* Settings = UTunaSweeperGameUserSettings::Get())
	{
		Settings->ApplyGraphicsState(AppliedState, false);
		Settings->ConfirmVideoMode();
		Settings->SaveSettings();
		ApplyDLSSModeToRuntime(AppliedState.DLSSMode);
	}
	PendingState = AppliedState;
	bResolutionConfirmationActive = false;
	if (ResolutionConfirmationPanel) ResolutionConfirmationPanel->SetVisibility(ESlateVisibility::Collapsed);
	RefreshVisualState();
}

void UTunaSweeperGraphicsSettingsWidget::HandleQualityStepRequested(ETunaSweeperScalabilityOption Option, int32 Delta)
{
	int32& Quality = TunaSweeperGraphicsSettingsWidget::GetQuality(PendingState.QualityLevels, Option);
	Quality = FMath::Clamp(Quality + Delta, 0, 3);
	PendingState.Preset = ETunaSweeperGraphicsPreset::Custom;
	RefreshVisualState();
}

void UTunaSweeperGraphicsSettingsWidget::HandlePresetStepRequested(int32 Delta)
{
	if (Delta == 0)
	{
		return;
	}
	if (PendingState.Preset == ETunaSweeperGraphicsPreset::Custom)
	{
		SelectPreset(Delta < 0 ? ETunaSweeperGraphicsPreset::Epic : ETunaSweeperGraphicsPreset::Auto);
		return;
	}
	const int32 NextIndex = FMath::Clamp(
		static_cast<int32>(PendingState.Preset) + FMath::Sign(Delta),
		static_cast<int32>(ETunaSweeperGraphicsPreset::Auto),
		static_cast<int32>(ETunaSweeperGraphicsPreset::Epic));
	SelectPreset(static_cast<ETunaSweeperGraphicsPreset>(NextIndex));
}

void UTunaSweeperGraphicsSettingsWidget::HandleWindowModeStepRequested(int32 Delta)
{
	const TArray<EWindowMode::Type> Modes = {
		EWindowMode::Windowed,
		EWindowMode::WindowedFullscreen,
		EWindowMode::Fullscreen
	};
	const int32 Index = Modes.IndexOfByKey(PendingState.WindowMode);
	if (Index != INDEX_NONE && Delta != 0)
	{
		SetWindowMode(Modes[FMath::Clamp(Index + FMath::Sign(Delta), 0, Modes.Num() - 1)]);
	}
}

void UTunaSweeperGraphicsSettingsWidget::HandleResolutionStepRequested(int32 Delta)
{
	const TArray<FIntPoint> Resolutions = TunaSweeperGraphicsSettingsOptions::BuildResolutionCandidates(
		PendingState.Resolution,
		AppliedState.Resolution);
	const int32 Index = Resolutions.IndexOfByKey(PendingState.Resolution);
	if (Index != INDEX_NONE && Delta != 0)
	{
		SetResolution(Resolutions[FMath::Clamp(Index + FMath::Sign(Delta), 0, Resolutions.Num() - 1)]);
	}
}

void UTunaSweeperGraphicsSettingsWidget::HandleDLSSStepRequested(int32 Delta)
{
	TArray<ETunaSweeperTitleDLSSMode> Modes;
	for (ETunaSweeperTitleDLSSMode Mode : {
		ETunaSweeperTitleDLSSMode::Off,
		ETunaSweeperTitleDLSSMode::Quality,
		ETunaSweeperTitleDLSSMode::Balanced,
		ETunaSweeperTitleDLSSMode::Performance })
	{
		if (IsDLSSModeAvailable(Mode) || Mode == PendingState.DLSSMode)
		{
			Modes.Add(Mode);
		}
	}
	const int32 Index = Modes.IndexOfByKey(PendingState.DLSSMode);
	if (Index != INDEX_NONE && Delta != 0)
	{
		SetDLSSMode(Modes[FMath::Clamp(Index + FMath::Sign(Delta), 0, Modes.Num() - 1)]);
	}
}

void UTunaSweeperGraphicsSettingsWidget::HandleFrameRateStepRequested(int32 Delta)
{
	const TArray<float> FrameRates = TunaSweeperGraphicsSettingsOptions::BuildFrameRateCandidates(
		PendingState.FrameRateLimit,
		AppliedState.FrameRateLimit);
	const int32 Index = FrameRates.IndexOfByPredicate([this](float Value) { return FMath::IsNearlyEqual(Value, PendingState.FrameRateLimit); });
	if (Index != INDEX_NONE && Delta != 0)
	{
		SetFrameRateLimit(FrameRates[FMath::Clamp(Index + FMath::Sign(Delta), 0, FrameRates.Num() - 1)]);
	}
}

#define PRESET_HANDLER(Name, Value) void UTunaSweeperGraphicsSettingsWidget::Name() { SelectPreset(Value); }
PRESET_HANDLER(HandlePresetAutoClicked, ETunaSweeperGraphicsPreset::Auto)
PRESET_HANDLER(HandlePresetLowClicked, ETunaSweeperGraphicsPreset::Low)
PRESET_HANDLER(HandlePresetMediumClicked, ETunaSweeperGraphicsPreset::Medium)
PRESET_HANDLER(HandlePresetHighClicked, ETunaSweeperGraphicsPreset::High)
PRESET_HANDLER(HandlePresetEpicClicked, ETunaSweeperGraphicsPreset::Epic)
#undef PRESET_HANDLER

void UTunaSweeperGraphicsSettingsWidget::HandleWindowedModeClicked() { SetWindowMode(EWindowMode::Windowed); }
void UTunaSweeperGraphicsSettingsWidget::HandleBorderlessWindowModeClicked() { SetWindowMode(EWindowMode::WindowedFullscreen); }
void UTunaSweeperGraphicsSettingsWidget::HandleFullscreenModeClicked() { SetWindowMode(EWindowMode::Fullscreen); }
void UTunaSweeperGraphicsSettingsWidget::HandleResolution1280Clicked() { SetResolution(FIntPoint(1280, 720)); }
void UTunaSweeperGraphicsSettingsWidget::HandleResolution1600Clicked() { SetResolution(FIntPoint(1600, 900)); }
void UTunaSweeperGraphicsSettingsWidget::HandleResolution1920Clicked() { SetResolution(FIntPoint(1920, 1080)); }
void UTunaSweeperGraphicsSettingsWidget::HandleResolution2560Clicked() { SetResolution(FIntPoint(2560, 1440)); }
void UTunaSweeperGraphicsSettingsWidget::HandleResolution3840Clicked() { SetResolution(FIntPoint(3840, 2160)); }
void UTunaSweeperGraphicsSettingsWidget::HandleDLSSOffClicked() { SetDLSSMode(ETunaSweeperTitleDLSSMode::Off); }
void UTunaSweeperGraphicsSettingsWidget::HandleDLSSQualityClicked() { SetDLSSMode(ETunaSweeperTitleDLSSMode::Quality); }
void UTunaSweeperGraphicsSettingsWidget::HandleDLSSBalancedClicked() { SetDLSSMode(ETunaSweeperTitleDLSSMode::Balanced); }
void UTunaSweeperGraphicsSettingsWidget::HandleDLSSPerformanceClicked() { SetDLSSMode(ETunaSweeperTitleDLSSMode::Performance); }
void UTunaSweeperGraphicsSettingsWidget::HandleVSyncToggleClicked() { PendingState.bVSyncEnabled = !PendingState.bVSyncEnabled; RefreshVisualState(); }
void UTunaSweeperGraphicsSettingsWidget::HandleFrameRateUnlimitedClicked() { SetFrameRateLimit(0.0f); }
void UTunaSweeperGraphicsSettingsWidget::HandleFrameRate60Clicked() { SetFrameRateLimit(60.0f); }
void UTunaSweeperGraphicsSettingsWidget::HandleFrameRate120Clicked() { SetFrameRateLimit(120.0f); }
void UTunaSweeperGraphicsSettingsWidget::HandleFrameRate144Clicked() { SetFrameRateLimit(144.0f); }
void UTunaSweeperGraphicsSettingsWidget::HandleMotionBlurToggleClicked() { PendingState.bMotionBlurEnabled = !PendingState.bMotionBlurEnabled; RefreshVisualState(); }
void UTunaSweeperGraphicsSettingsWidget::HandleDynamicResolutionToggleClicked() { PendingState.bDynamicResolutionEnabled = !PendingState.bDynamicResolutionEnabled; RefreshVisualState(); }
void UTunaSweeperGraphicsSettingsWidget::HandleHardwareRayTracingToggleClicked() { PendingState.bHardwareRayTracingEnabled = !PendingState.bHardwareRayTracingEnabled; RefreshVisualState(); }

void UTunaSweeperGraphicsSettingsWidget::HandleApplyClicked()
{
	UTunaSweeperGameUserSettings* Settings = UTunaSweeperGameUserSettings::Get();
	if (!Settings || !HasPendingChanges())
	{
		return;
	}
	const bool bDisplayModeChanged = PendingState.Resolution != AppliedState.Resolution || PendingState.WindowMode != AppliedState.WindowMode;
	Settings->ApplyGraphicsState(PendingState, false);
	ApplyDLSSModeToRuntime(PendingState.DLSSMode);
	if (bDisplayModeChanged)
	{
		BeginResolutionConfirmation();
	}
	else
	{
		Settings->SaveSettings();
		AppliedState = Settings->CaptureGraphicsState();
		PendingState = AppliedState;
		RefreshVisualState();
	}
}

void UTunaSweeperGraphicsSettingsWidget::HandleCancelClicked() { DiscardPendingChanges(); }
void UTunaSweeperGraphicsSettingsWidget::HandleConfirmResolutionClicked() { ConfirmResolution(); }
void UTunaSweeperGraphicsSettingsWidget::HandleRevertResolutionClicked() { RevertResolution(); }
