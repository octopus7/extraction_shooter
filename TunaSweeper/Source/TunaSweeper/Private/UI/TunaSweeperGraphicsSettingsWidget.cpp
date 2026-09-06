#include "UI/TunaSweeperGraphicsSettingsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
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
#include "UI/TunaSweeperUIFont.h"

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

TSharedRef<SWidget> UTunaSweeperGraphicsSettingsWidget::RebuildWidget()
{
	BuildRuntimeWidgetTree();
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
	TunaSweeperUIFont::ApplyFont(GraphicsStatusText, 14);
	GraphicsStatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.72f, 0.84f, 0.84f, 1.0f)));
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

	auto AddHeading = [this, Content](const TCHAR* Name, const TCHAR* Text)
	{
		UTextBlock* Heading = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Heading->SetText(FText::FromString(Text));
		Heading->SetColorAndOpacity(FSlateColor(FLinearColor(0.63f, 0.78f, 0.78f, 1.0f)));
		TunaSweeperUIFont::ApplyFont(Heading, 15);
		if (UVerticalBoxSlot* HeadingSlot = Content->AddChildToVerticalBox(Heading))
		{
			HeadingSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 2.0f));
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

#define MAKE_GRAPHICS_BUTTON(Name, Label, Width) \
	Name = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT(#Name)); \
	Name##Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT(#Name "Text")); \
	Name##Text->SetText(FText::FromString(Label)); \
	Name##Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.95f, 0.96f, 1.0f))); \
	Name##Text->SetJustification(ETextJustify::Center); \
	TunaSweeperUIFont::ApplyFont(Name##Text, 13); \
	Name->SetContent(Name##Text); \
	Name->SetBackgroundColor(FLinearColor(0.12f, 0.16f, 0.18f, 1.0f)); \
	USizeBox* Name##Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT(#Name "Box")); \
	Name##Box->SetWidthOverride(Width); \
	Name##Box->SetHeightOverride(36.0f); \
	Name##Box->SetContent(Name)

	AddHeading(TEXT("PresetHeading"), TEXT("전체 품질 프리셋"));
	MAKE_GRAPHICS_BUTTON(PresetAutoButton, TEXT("자동"), 110.0f);
	MAKE_GRAPHICS_BUTTON(PresetLowButton, TEXT("낮음"), 100.0f);
	MAKE_GRAPHICS_BUTTON(PresetMediumButton, TEXT("중간"), 100.0f);
	MAKE_GRAPHICS_BUTTON(PresetHighButton, TEXT("높음"), 100.0f);
	MAKE_GRAPHICS_BUTTON(PresetEpicButton, TEXT("최고"), 100.0f);
	AddButtonRow({ PresetAutoButtonBox, PresetLowButtonBox, PresetMediumButtonBox, PresetHighButtonBox, PresetEpicButtonBox });

	AddHeading(TEXT("WindowHeading"), TEXT("화면 모드"));
	MAKE_GRAPHICS_BUTTON(WindowedModeButton, TEXT("창모드"), 145.0f);
	MAKE_GRAPHICS_BUTTON(BorderlessWindowModeButton, TEXT("테두리 없는 창"), 190.0f);
	MAKE_GRAPHICS_BUTTON(FullscreenModeButton, TEXT("전체화면"), 145.0f);
	AddButtonRow({ WindowedModeButtonBox, BorderlessWindowModeButtonBox, FullscreenModeButtonBox });

	AddHeading(TEXT("ResolutionHeading"), TEXT("해상도"));
	MAKE_GRAPHICS_BUTTON(Resolution1280Button, TEXT("1280 x 720"), 130.0f);
	MAKE_GRAPHICS_BUTTON(Resolution1600Button, TEXT("1600 x 900"), 130.0f);
	MAKE_GRAPHICS_BUTTON(Resolution1920Button, TEXT("1920 x 1080"), 138.0f);
	MAKE_GRAPHICS_BUTTON(Resolution2560Button, TEXT("2560 x 1440"), 138.0f);
	MAKE_GRAPHICS_BUTTON(Resolution3840Button, TEXT("3840 x 2160"), 138.0f);
	AddButtonRow({ Resolution1280ButtonBox, Resolution1600ButtonBox, Resolution1920ButtonBox, Resolution2560ButtonBox, Resolution3840ButtonBox });

	AddHeading(TEXT("DLSSHeading"), TEXT("DLSS"));
	MAKE_GRAPHICS_BUTTON(DLSSOffButton, TEXT("끄기"), 120.0f);
	MAKE_GRAPHICS_BUTTON(DLSSQualityButton, TEXT("품질"), 120.0f);
	MAKE_GRAPHICS_BUTTON(DLSSBalancedButton, TEXT("균형"), 120.0f);
	MAKE_GRAPHICS_BUTTON(DLSSPerformanceButton, TEXT("성능"), 120.0f);
	AddButtonRow({ DLSSOffButtonBox, DLSSQualityButtonBox, DLSSBalancedButtonBox, DLSSPerformanceButtonBox });

	AddHeading(TEXT("QualityHeading"), TEXT("개별 품질"));
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

	AddHeading(TEXT("PerformanceHeading"), TEXT("성능 및 효과"));
	MAKE_GRAPHICS_BUTTON(VSyncToggleButton, TEXT("[ ] 수직 동기화"), 165.0f);
	MAKE_GRAPHICS_BUTTON(MotionBlurToggleButton, TEXT("[x] 모션 블러"), 165.0f);
	MAKE_GRAPHICS_BUTTON(DynamicResolutionToggleButton, TEXT("[ ] 동적 해상도"), 175.0f);
	MAKE_GRAPHICS_BUTTON(HardwareRayTracingToggleButton, TEXT("[x] 하드웨어 RT"), 190.0f);
	AddButtonRow({ VSyncToggleButtonBox, MotionBlurToggleButtonBox, DynamicResolutionToggleButtonBox, HardwareRayTracingToggleButtonBox });
	MAKE_GRAPHICS_BUTTON(FrameRateUnlimitedButton, TEXT("제한 없음"), 145.0f);
	MAKE_GRAPHICS_BUTTON(FrameRate60Button, TEXT("60 FPS"), 110.0f);
	MAKE_GRAPHICS_BUTTON(FrameRate120Button, TEXT("120 FPS"), 110.0f);
	MAKE_GRAPHICS_BUTTON(FrameRate144Button, TEXT("144 FPS"), 110.0f);
	AddButtonRow({ FrameRateUnlimitedButtonBox, FrameRate60ButtonBox, FrameRate120ButtonBox, FrameRate144ButtonBox });

	MAKE_GRAPHICS_BUTTON(ApplyGraphicsSettingsButton, TEXT("적용"), 145.0f);
	MAKE_GRAPHICS_BUTTON(CancelGraphicsSettingsButton, TEXT("취소"), 145.0f);
	UHorizontalBox* ActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("GraphicsActionRow"));
	ActionRow->AddChildToHorizontalBox(ApplyGraphicsSettingsButtonBox)->SetPadding(FMargin(0.0f, 8.0f, 10.0f, 0.0f));
	ActionRow->AddChildToHorizontalBox(CancelGraphicsSettingsButtonBox)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	Root->AddChildToVerticalBox(ActionRow);

	MAKE_GRAPHICS_BUTTON(ConfirmResolutionButton, TEXT("유지"), 90.0f);
	MAKE_GRAPHICS_BUTTON(RevertResolutionButton, TEXT("되돌리기"), 110.0f);
#undef MAKE_GRAPHICS_BUTTON
	UBorder* ConfirmationBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ResolutionConfirmationPanel"));
	ResolutionConfirmationPanel = ConfirmationBorder;
	ConfirmationBorder->SetPadding(FMargin(10.0f));
	ConfirmationBorder->SetBrushColor(FLinearColor(0.12f, 0.075f, 0.035f, 0.98f));
	UHorizontalBox* ConfirmationRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	ResolutionConfirmationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResolutionConfirmationText"));
	ResolutionConfirmationText->SetText(FText::FromString(TEXT("이 화면 설정을 유지할까요? 15초 후 복구됩니다.")));
	ResolutionConfirmationText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	TunaSweeperUIFont::ApplyFont(ResolutionConfirmationText, 13);
	ConfirmationRow->AddChildToHorizontalBox(ResolutionConfirmationText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	ConfirmationRow->AddChildToHorizontalBox(ConfirmResolutionButtonBox)->SetPadding(FMargin(8.0f, 0.0f));
	ConfirmationRow->AddChildToHorizontalBox(RevertResolutionButtonBox);
	ConfirmationBorder->SetContent(ConfirmationRow);
	ConfirmationBorder->SetVisibility(ESlateVisibility::Collapsed);
	Root->AddChildToVerticalBox(ConfirmationBorder)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
}

void UTunaSweeperGraphicsSettingsWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BindButtons();
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
			ResolveUiText(FName(TEXT("ui.settings.resolution_confirm_countdown")), FText::FromString(TEXT("이 화면 설정을 유지할까요? {0}초 후 복구됩니다."))),
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

bool UTunaSweeperGraphicsSettingsWidget::HasPendingChanges() const
{
	return bHasSettingsSnapshot &&
		!TunaSweeperGraphicsSettingsWidget::StatesEqual(AppliedState, PendingState);
}

void UTunaSweeperGraphicsSettingsWidget::BindButtons()
{
#define BIND_GRAPHICS_BUTTON(Button, Handler) \
	if (Button) { Button->OnClicked.RemoveDynamic(this, &UTunaSweeperGraphicsSettingsWidget::Handler); Button->OnClicked.AddDynamic(this, &UTunaSweeperGraphicsSettingsWidget::Handler); }
	BIND_GRAPHICS_BUTTON(PresetAutoButton, HandlePresetAutoClicked);
	BIND_GRAPHICS_BUTTON(PresetLowButton, HandlePresetLowClicked);
	BIND_GRAPHICS_BUTTON(PresetMediumButton, HandlePresetMediumClicked);
	BIND_GRAPHICS_BUTTON(PresetHighButton, HandlePresetHighClicked);
	BIND_GRAPHICS_BUTTON(PresetEpicButton, HandlePresetEpicClicked);
	BIND_GRAPHICS_BUTTON(WindowedModeButton, HandleWindowedModeClicked);
	BIND_GRAPHICS_BUTTON(BorderlessWindowModeButton, HandleBorderlessWindowModeClicked);
	BIND_GRAPHICS_BUTTON(FullscreenModeButton, HandleFullscreenModeClicked);
	BIND_GRAPHICS_BUTTON(Resolution1280Button, HandleResolution1280Clicked);
	BIND_GRAPHICS_BUTTON(Resolution1600Button, HandleResolution1600Clicked);
	BIND_GRAPHICS_BUTTON(Resolution1920Button, HandleResolution1920Clicked);
	BIND_GRAPHICS_BUTTON(Resolution2560Button, HandleResolution2560Clicked);
	BIND_GRAPHICS_BUTTON(Resolution3840Button, HandleResolution3840Clicked);
	BIND_GRAPHICS_BUTTON(DLSSOffButton, HandleDLSSOffClicked);
	BIND_GRAPHICS_BUTTON(DLSSQualityButton, HandleDLSSQualityClicked);
	BIND_GRAPHICS_BUTTON(DLSSBalancedButton, HandleDLSSBalancedClicked);
	BIND_GRAPHICS_BUTTON(DLSSPerformanceButton, HandleDLSSPerformanceClicked);
	BIND_GRAPHICS_BUTTON(VSyncToggleButton, HandleVSyncToggleClicked);
	BIND_GRAPHICS_BUTTON(FrameRateUnlimitedButton, HandleFrameRateUnlimitedClicked);
	BIND_GRAPHICS_BUTTON(FrameRate60Button, HandleFrameRate60Clicked);
	BIND_GRAPHICS_BUTTON(FrameRate120Button, HandleFrameRate120Clicked);
	BIND_GRAPHICS_BUTTON(FrameRate144Button, HandleFrameRate144Clicked);
	BIND_GRAPHICS_BUTTON(MotionBlurToggleButton, HandleMotionBlurToggleClicked);
	BIND_GRAPHICS_BUTTON(DynamicResolutionToggleButton, HandleDynamicResolutionToggleClicked);
	BIND_GRAPHICS_BUTTON(HardwareRayTracingToggleButton, HandleHardwareRayTracingToggleClicked);
	BIND_GRAPHICS_BUTTON(ApplyGraphicsSettingsButton, HandleApplyClicked);
	BIND_GRAPHICS_BUTTON(CancelGraphicsSettingsButton, HandleCancelClicked);
	BIND_GRAPHICS_BUTTON(ConfirmResolutionButton, HandleConfirmResolutionClicked);
	BIND_GRAPHICS_BUTTON(RevertResolutionButton, HandleRevertResolutionClicked);
#undef BIND_GRAPHICS_BUTTON
}

void UTunaSweeperGraphicsSettingsWidget::ConfigureQualityRows()
{
	struct FRowDefinition
	{
		UTunaSweeperGraphicsQualityRowWidget* Row;
		ETunaSweeperScalabilityOption Option;
		const TCHAR* Key;
		const TCHAR* Label;
	};

	const FRowDefinition Rows[] = {
		{ TextureQualityRow, ETunaSweeperScalabilityOption::Texture, TEXT("ui.settings.texture"), TEXT("텍스처") },
		{ ShadowQualityRow, ETunaSweeperScalabilityOption::Shadow, TEXT("ui.settings.shadow"), TEXT("그림자") },
		{ GlobalIlluminationQualityRow, ETunaSweeperScalabilityOption::GlobalIllumination, TEXT("ui.settings.global_illumination"), TEXT("전역 조명") },
		{ ReflectionQualityRow, ETunaSweeperScalabilityOption::Reflection, TEXT("ui.settings.reflection"), TEXT("반사") },
		{ ViewDistanceQualityRow, ETunaSweeperScalabilityOption::ViewDistance, TEXT("ui.settings.view_distance"), TEXT("가시 거리") },
		{ EffectsQualityRow, ETunaSweeperScalabilityOption::Effects, TEXT("ui.settings.effects"), TEXT("효과") },
		{ PostProcessQualityRow, ETunaSweeperScalabilityOption::PostProcess, TEXT("ui.settings.post_process"), TEXT("후처리") },
		{ FoliageQualityRow, ETunaSweeperScalabilityOption::Foliage, TEXT("ui.settings.foliage"), TEXT("식생") },
		{ ShadingQualityRow, ETunaSweeperScalabilityOption::Shading, TEXT("ui.settings.shading"), TEXT("셰이딩") },
		{ LandscapeQualityRow, ETunaSweeperScalabilityOption::Landscape, TEXT("ui.settings.landscape"), TEXT("지형") },
		{ AntiAliasingQualityRow, ETunaSweeperScalabilityOption::AntiAliasing, TEXT("ui.settings.anti_aliasing"), TEXT("안티앨리어싱") }
	};

	for (const FRowDefinition& Definition : Rows)
	{
		if (Definition.Row)
		{
			Definition.Row->Configure(Definition.Option, ResolveUiText(FName(Definition.Key), FText::FromString(Definition.Label)));
			Definition.Row->OnQualityStepRequested.RemoveAll(this);
			Definition.Row->OnQualityStepRequested.AddUObject(this, &UTunaSweeperGraphicsSettingsWidget::HandleQualityStepRequested);
		}
	}
}

void UTunaSweeperGraphicsSettingsWidget::RefreshVisualState()
{
	if (WidgetTree) if (UTextBlock* Header = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("GraphicsSectionTitleText"))))
		Header->SetText(ResolveUiText(FName(TEXT("ui.settings.graphics")), FText::FromString(TEXT("그래픽"))));
	if (!bHasSettingsSnapshot)
	{
		return;
	}

	const UTunaSweeperGameUserSettings* Settings = UTunaSweeperGameUserSettings::Get();
	if (GraphicsStatusText && Settings)
	{
		const ETunaSweeperGraphicsPreset DisplayPreset = PendingState.Preset == ETunaSweeperGraphicsPreset::Auto
			? Settings->GetResolvedAutoGraphicsPreset()
			: PendingState.Preset;
		const FString AutoSuffix = PendingState.Preset == ETunaSweeperGraphicsPreset::Auto
			? FString::Printf(TEXT(" (%s, VRAM %d MB)"), *BuildPresetText(DisplayPreset).ToString(), Settings->GetLastDetectedDedicatedVideoMemoryMB())
			: FString();
		GraphicsStatusText->SetText(FText::FromString(FString::Printf(
			TEXT("%s%s%s"),
			*BuildPresetText(PendingState.Preset).ToString(),
			*AutoSuffix,
			HasPendingChanges() ? TEXT(" · 적용 대기") : TEXT(""))));
	}

	SetChoiceButtonText(PresetAutoButtonText, BuildPresetText(ETunaSweeperGraphicsPreset::Auto), PendingState.Preset == ETunaSweeperGraphicsPreset::Auto);
	SetChoiceButtonText(PresetLowButtonText, BuildPresetText(ETunaSweeperGraphicsPreset::Low), PendingState.Preset == ETunaSweeperGraphicsPreset::Low);
	SetChoiceButtonText(PresetMediumButtonText, BuildPresetText(ETunaSweeperGraphicsPreset::Medium), PendingState.Preset == ETunaSweeperGraphicsPreset::Medium);
	SetChoiceButtonText(PresetHighButtonText, BuildPresetText(ETunaSweeperGraphicsPreset::High), PendingState.Preset == ETunaSweeperGraphicsPreset::High);
	SetChoiceButtonText(PresetEpicButtonText, BuildPresetText(ETunaSweeperGraphicsPreset::Epic), PendingState.Preset == ETunaSweeperGraphicsPreset::Epic);

	SetChoiceButtonText(WindowedModeButtonText, ResolveUiText(FName(TEXT("ui.settings.windowed")), FText::FromString(TEXT("창모드"))), PendingState.WindowMode == EWindowMode::Windowed);
	SetChoiceButtonText(BorderlessWindowModeButtonText, ResolveUiText(FName(TEXT("ui.settings.borderless")), FText::FromString(TEXT("테두리 없는 창모드"))), PendingState.WindowMode == EWindowMode::WindowedFullscreen);
	SetChoiceButtonText(FullscreenModeButtonText, ResolveUiText(FName(TEXT("ui.settings.fullscreen")), FText::FromString(TEXT("전체화면"))), PendingState.WindowMode == EWindowMode::Fullscreen);

	SetChoiceButtonText(Resolution1280ButtonText, FText::FromString(TEXT("1280 x 720")), PendingState.Resolution == FIntPoint(1280, 720));
	SetChoiceButtonText(Resolution1600ButtonText, FText::FromString(TEXT("1600 x 900")), PendingState.Resolution == FIntPoint(1600, 900));
	SetChoiceButtonText(Resolution1920ButtonText, FText::FromString(TEXT("1920 x 1080")), PendingState.Resolution == FIntPoint(1920, 1080));
	SetChoiceButtonText(Resolution2560ButtonText, FText::FromString(TEXT("2560 x 1440")), PendingState.Resolution == FIntPoint(2560, 1440));
	SetChoiceButtonText(Resolution3840ButtonText, FText::FromString(TEXT("3840 x 2160")), PendingState.Resolution == FIntPoint(3840, 2160));

	SetChoiceButtonText(DLSSOffButtonText, BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Off), PendingState.DLSSMode == ETunaSweeperTitleDLSSMode::Off);
	SetChoiceButtonText(DLSSQualityButtonText, BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Quality), PendingState.DLSSMode == ETunaSweeperTitleDLSSMode::Quality);
	SetChoiceButtonText(DLSSBalancedButtonText, BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Balanced), PendingState.DLSSMode == ETunaSweeperTitleDLSSMode::Balanced);
	SetChoiceButtonText(DLSSPerformanceButtonText, BuildDLSSModeText(ETunaSweeperTitleDLSSMode::Performance), PendingState.DLSSMode == ETunaSweeperTitleDLSSMode::Performance);
	if (DLSSOffButton) DLSSOffButton->SetIsEnabled(true);
	if (DLSSQualityButton) DLSSQualityButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Quality));
	if (DLSSBalancedButton) DLSSBalancedButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Balanced));
	if (DLSSPerformanceButton) DLSSPerformanceButton->SetIsEnabled(IsDLSSModeAvailable(ETunaSweeperTitleDLSSMode::Performance));

	if (VSyncToggleButtonText) VSyncToggleButtonText->SetText(BuildToggleText(ResolveUiText(FName(TEXT("ui.settings.vsync")), FText::FromString(TEXT("수직 동기화"))), PendingState.bVSyncEnabled));
	if (MotionBlurToggleButtonText) MotionBlurToggleButtonText->SetText(BuildToggleText(ResolveUiText(FName(TEXT("ui.settings.motion_blur")), FText::FromString(TEXT("모션 블러"))), PendingState.bMotionBlurEnabled));
	if (DynamicResolutionToggleButtonText) DynamicResolutionToggleButtonText->SetText(BuildToggleText(ResolveUiText(FName(TEXT("ui.settings.dynamic_resolution")), FText::FromString(TEXT("동적 해상도"))), PendingState.bDynamicResolutionEnabled));
	if (HardwareRayTracingToggleButtonText) HardwareRayTracingToggleButtonText->SetText(BuildToggleText(ResolveUiText(FName(TEXT("ui.settings.hardware_ray_tracing")), FText::FromString(TEXT("하드웨어 레이 트레이싱"))), PendingState.bHardwareRayTracingEnabled));
	if (HardwareRayTracingToggleButton) HardwareRayTracingToggleButton->SetIsEnabled(GRHISupportsRayTracing);

	SetChoiceButtonText(FrameRateUnlimitedButtonText, ResolveUiText(FName(TEXT("ui.settings.fps_unlimited")), FText::FromString(TEXT("제한 없음"))), PendingState.FrameRateLimit <= 0.0f);
	SetChoiceButtonText(FrameRate60ButtonText, FText::FromString(TEXT("60 FPS")), FMath::IsNearlyEqual(PendingState.FrameRateLimit, 60.0f));
	SetChoiceButtonText(FrameRate120ButtonText, FText::FromString(TEXT("120 FPS")), FMath::IsNearlyEqual(PendingState.FrameRateLimit, 120.0f));
	SetChoiceButtonText(FrameRate144ButtonText, FText::FromString(TEXT("144 FPS")), FMath::IsNearlyEqual(PendingState.FrameRateLimit, 144.0f));

	if (ApplyGraphicsSettingsButtonText) ApplyGraphicsSettingsButtonText->SetText(ResolveUiText(FName(TEXT("ui.common.apply")), FText::FromString(TEXT("적용"))));
	if (CancelGraphicsSettingsButtonText) CancelGraphicsSettingsButtonText->SetText(ResolveUiText(FName(TEXT("ui.common.cancel")), FText::FromString(TEXT("취소"))));
	if (ConfirmResolutionButtonText) ConfirmResolutionButtonText->SetText(ResolveUiText(FName(TEXT("ui.common.keep")), FText::FromString(TEXT("유지"))));
	if (RevertResolutionButtonText) RevertResolutionButtonText->SetText(ResolveUiText(FName(TEXT("ui.common.revert")), FText::FromString(TEXT("되돌리기"))));
	if (ApplyGraphicsSettingsButton) ApplyGraphicsSettingsButton->SetIsEnabled(HasPendingChanges() && !bResolutionConfirmationActive);
	if (CancelGraphicsSettingsButton) CancelGraphicsSettingsButton->SetIsEnabled(HasPendingChanges() && !bResolutionConfirmationActive);

	RefreshQualityRows();
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
			Row.Widget->SetIsEnabled(Row.Option != ETunaSweeperScalabilityOption::AntiAliasing || PendingState.DLSSMode == ETunaSweeperTitleDLSSMode::Off);
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

FText UTunaSweeperGraphicsSettingsWidget::ResolveUiText(FName StringKey, const FText& FallbackText) const
{
	if (const UTunaSweeperGameInstance* GameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		return GameInstance->ResolveLocalizedText(StringKey, FallbackText);
	}
	return FallbackText;
}

FText UTunaSweeperGraphicsSettingsWidget::BuildPresetText(ETunaSweeperGraphicsPreset Preset) const
{
	switch (Preset)
	{
	case ETunaSweeperGraphicsPreset::Auto: return ResolveUiText(FName(TEXT("ui.settings.preset.auto")), FText::FromString(TEXT("자동")));
	case ETunaSweeperGraphicsPreset::Low: return ResolveUiText(FName(TEXT("ui.settings.preset.low")), FText::FromString(TEXT("낮음")));
	case ETunaSweeperGraphicsPreset::Medium: return ResolveUiText(FName(TEXT("ui.settings.preset.medium")), FText::FromString(TEXT("중간")));
	case ETunaSweeperGraphicsPreset::High: return ResolveUiText(FName(TEXT("ui.settings.preset.high")), FText::FromString(TEXT("높음")));
	case ETunaSweeperGraphicsPreset::Epic: return ResolveUiText(FName(TEXT("ui.settings.preset.epic")), FText::FromString(TEXT("최고")));
	case ETunaSweeperGraphicsPreset::Custom:
	default: return ResolveUiText(FName(TEXT("ui.settings.preset.custom")), FText::FromString(TEXT("사용자 지정")));
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

FText UTunaSweeperGraphicsSettingsWidget::BuildDLSSModeText(ETunaSweeperTitleDLSSMode Mode) const
{
	switch (Mode)
	{
	case ETunaSweeperTitleDLSSMode::Quality: return ResolveUiText(FName(TEXT("ui.settings.dlss.quality")), FText::FromString(TEXT("품질")));
	case ETunaSweeperTitleDLSSMode::Balanced: return ResolveUiText(FName(TEXT("ui.settings.dlss.balanced")), FText::FromString(TEXT("균형")));
	case ETunaSweeperTitleDLSSMode::Performance: return ResolveUiText(FName(TEXT("ui.settings.dlss.performance")), FText::FromString(TEXT("성능")));
	case ETunaSweeperTitleDLSSMode::Off:
	default: return ResolveUiText(FName(TEXT("ui.settings.dlss.off")), FText::FromString(TEXT("끄기")));
	}
}

FText UTunaSweeperGraphicsSettingsWidget::BuildToggleText(const FText& Label, bool bEnabled) const
{
	return FText::FromString(FString::Printf(TEXT("[%s] %s"), bEnabled ? TEXT("x") : TEXT(" "), *Label.ToString()));
}

void UTunaSweeperGraphicsSettingsWidget::SetChoiceButtonText(UTextBlock* TextBlock, const FText& Label, bool bSelected) const
{
	if (TextBlock)
	{
		TextBlock->SetText(FText::FromString(FString::Printf(TEXT("%s %s"), bSelected ? TEXT("✓") : TEXT(" "), *Label.ToString())));
	}
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
