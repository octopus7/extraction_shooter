#include "UI/TunaSweeperHousingPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Styling/SlateBrush.h"
#include "Subsystem/TunaSweeperHousingSubsystem.h"
#include "TimerManager.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"

namespace TunaSweeperHousingPanel
{
	constexpr float StoreHoldSeconds = 0.65f;
	using TunaSweeperUiText::ResolveUiText;

	FSlateBrush MakePanelBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		float Radius,
		const FLinearColor& OutlineColor,
		float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}
}

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperHousingPanel, Log, All);

void UTunaSweeperHousingFacilityEntryWidget::InitializeEntry(
	const FTunaSweeperHousingFacilityView& InView,
	FTunaSweeperHousingEntryClickedDelegate InClickedDelegate,
	FTunaSweeperHousingEntryStoreDelegate InStoreDelegate)
{
	View = InView;
	ClickedDelegate = InClickedDelegate;
	StoreDelegate = InStoreDelegate;
	ClearStoreHoldTimer();
	PressedSeconds = 0.0f;
	bPressed = false;
	bLongPressTriggered = false;
	RefreshEntryView();
}

TSharedRef<SWidget> UTunaSweeperHousingFacilityEntryWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildEntryWidget();
	return Super::RebuildWidget();
}

void UTunaSweeperHousingFacilityEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildEntryWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (EntryButton)
	{
		EntryButton->OnPressed.RemoveDynamic(this, &UTunaSweeperHousingFacilityEntryWidget::HandleEntryPressed);
		EntryButton->OnPressed.AddDynamic(this, &UTunaSweeperHousingFacilityEntryWidget::HandleEntryPressed);
		EntryButton->OnReleased.RemoveDynamic(this, &UTunaSweeperHousingFacilityEntryWidget::HandleEntryReleased);
		EntryButton->OnReleased.AddDynamic(this, &UTunaSweeperHousingFacilityEntryWidget::HandleEntryReleased);
	}

	RefreshEntryView();
}

void UTunaSweeperHousingFacilityEntryWidget::NativeDestruct()
{
	ClearStoreHoldTimer();

	if (EntryButton)
	{
		EntryButton->OnPressed.RemoveDynamic(this, &UTunaSweeperHousingFacilityEntryWidget::HandleEntryPressed);
		EntryButton->OnReleased.RemoveDynamic(this, &UTunaSweeperHousingFacilityEntryWidget::HandleEntryReleased);
	}

	Super::NativeDestruct();
}

void UTunaSweeperHousingFacilityEntryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bPressed || bLongPressTriggered || !View.bCanStore)
	{
		return;
	}

	PressedSeconds += FMath::Max(0.0f, InDeltaTime);
	if (PressedSeconds >= TunaSweeperHousingPanel::StoreHoldSeconds)
	{
		HandleStoreHoldElapsed();
	}
}

void UTunaSweeperHousingFacilityEntryWidget::BuildEntryWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	EntryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("HousingFacilityEntryButton"));
	UVerticalBox* EntryStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HousingFacilityEntryStack"));
	NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HousingFacilityNameText"));
	StateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HousingFacilityStateText"));
	DetailText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HousingFacilityDetailText"));
	if (!EntryButton || !EntryStack || !NameText || !StateText || !DetailText)
	{
		return;
	}

	WidgetTree->RootWidget = EntryButton;
	EntryButton->SetContent(EntryStack);

	NameText->SetAutoWrapText(true);
	NameText->SetWrapTextAt(284.0f);
	TunaSweeperUIFont::ApplyFont(NameText, 17, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* NameSlot = EntryStack->AddChildToVerticalBox(NameText))
	{
		NameSlot->SetPadding(FMargin(10.0f, 8.0f, 10.0f, 2.0f));
	}

	StateText->SetAutoWrapText(false);
	TunaSweeperUIFont::ApplyFont(StateText, 13, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* StateSlot = EntryStack->AddChildToVerticalBox(StateText))
	{
		StateSlot->SetPadding(FMargin(10.0f, 0.0f, 10.0f, 2.0f));
	}

	DetailText->SetAutoWrapText(true);
	DetailText->SetWrapTextAt(284.0f);
	TunaSweeperUIFont::ApplyFont(DetailText, 12);
	if (UVerticalBoxSlot* DetailSlot = EntryStack->AddChildToVerticalBox(DetailText))
	{
		DetailSlot->SetPadding(FMargin(10.0f, 0.0f, 10.0f, 9.0f));
	}
}

void UTunaSweeperHousingFacilityEntryWidget::RefreshEntryView()
{
	if (EntryButton)
	{
		const bool bActionable = View.bCanStartPlacement || View.bCanStore;
		EntryButton->SetIsEnabled(bActionable);
		EntryButton->SetRenderOpacity(bActionable ? 0.94f : 0.58f);
		EntryButton->SetBackgroundColor(
			View.BuildState == ETunaSweeperHousingFacilityBuildState::Buildable ||
				View.BuildState == ETunaSweeperHousingFacilityBuildState::Stored
				? FLinearColor(0.08f, 0.20f, 0.24f, 0.92f)
				: View.BuildState == ETunaSweeperHousingFacilityBuildState::Placed
					? FLinearColor(0.15f, 0.14f, 0.09f, 0.92f)
					: FLinearColor(0.10f, 0.09f, 0.095f, 0.86f));
	}

	if (NameText)
	{
		NameText->SetText(View.DisplayName);
		NameText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.98f, 1.0f, 1.0f)));
	}

	if (StateText)
	{
		StateText->SetText(View.StateText);
		StateText->SetColorAndOpacity(FSlateColor(
			View.BuildState == ETunaSweeperHousingFacilityBuildState::InsufficientMaterials
				? FLinearColor(1.0f, 0.48f, 0.42f, 1.0f)
				: FLinearColor(0.55f, 0.88f, 1.0f, 1.0f)));
	}

	if (DetailText)
	{
		const FString Detail = FString::Printf(
			TEXT("%dx%d  %s"),
			FMath::Max(1, View.SizeX),
			FMath::Max(1, View.SizeY),
			*View.MaterialsText.ToString());
		DetailText->SetText(FText::FromString(Detail));
		DetailText->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.78f, 0.80f, 1.0f)));
	}
}

void UTunaSweeperHousingFacilityEntryWidget::HandleEntryClicked()
{
	if (bLongPressTriggered)
	{
		bLongPressTriggered = false;
		return;
	}

	if (View.bCanStartPlacement && ClickedDelegate.IsBound())
	{
		ClickedDelegate.Execute(View.FacilityId, View.InstanceId);
	}
}

void UTunaSweeperHousingFacilityEntryWidget::HandleEntryPressed()
{
	ClearStoreHoldTimer();
	bPressed = true;
	bLongPressTriggered = false;
	PressedSeconds = 0.0f;

	if (View.bCanStartPlacement && ClickedDelegate.IsBound())
	{
		bPressed = false;
		ClickedDelegate.Execute(View.FacilityId, View.InstanceId);
		return;
	}

	if (View.bCanStore && View.InstanceId.IsValid() && StoreDelegate.IsBound())
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				StoreHoldTimerHandle,
				this,
				&UTunaSweeperHousingFacilityEntryWidget::HandleStoreHoldElapsed,
				TunaSweeperHousingPanel::StoreHoldSeconds,
				false);
		}
	}
}

void UTunaSweeperHousingFacilityEntryWidget::HandleEntryReleased()
{
	ClearStoreHoldTimer();
	bPressed = false;
	PressedSeconds = 0.0f;
	bLongPressTriggered = false;
}

void UTunaSweeperHousingFacilityEntryWidget::ClearStoreHoldTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(StoreHoldTimerHandle);
	}
}

void UTunaSweeperHousingFacilityEntryWidget::HandleStoreHoldElapsed()
{
	ClearStoreHoldTimer();
	if (!bPressed || bLongPressTriggered || !View.bCanStore || !View.InstanceId.IsValid() || !StoreDelegate.IsBound())
	{
		return;
	}

	bLongPressTriggered = true;
	bPressed = false;
	PressedSeconds = 0.0f;
	StoreDelegate.Execute(View.InstanceId);
}

void UTunaSweeperHousingPanelWidget::RefreshHousingPanel()
{
	BuildPanelWidget();

	UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	if (!HousingSubsystem)
	{
		return;
	}

	TArray<FTunaSweeperHousingFacilityView> FacilityViews;
	HousingSubsystem->GetFacilityViews(FacilityViews);
	RebuildFacilityEntries(FacilityViews);

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (TitleText)
	{
		TitleText->SetText(TunaSweeperHousingPanel::ResolveUiText(
			TunaGameInstance,
			TEXT("ui.housing.title"),
			TEXT("Bunker Housing")));
	}
	if (GuideText)
	{
		if (HousingSubsystem->HasActivePlacement())
		{
			GuideText->SetText(TunaSweeperHousingPanel::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.housing.guide.place"),
				TEXT("Q/E rotate. Click floor to place.")));
			GuideText->SetColorAndOpacity(FSlateColor(
				HousingSubsystem->GetActivePlacementStatus() == ETunaSweeperHousingPlacementStatus::Valid
					? FLinearColor(0.60f, 0.92f, 1.0f, 1.0f)
					: FLinearColor(1.0f, 0.42f, 0.38f, 1.0f)));
		}
		else
		{
			GuideText->SetText(TunaSweeperHousingPanel::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.housing.guide.select"),
				TEXT("Click stored or ready facilities. Hold placed entries to store.")));
			GuideText->SetColorAndOpacity(FSlateColor(FLinearColor(0.66f, 0.78f, 0.82f, 1.0f)));
		}
	}
}

TSharedRef<SWidget> UTunaSweeperHousingPanelWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	BuildPanelWidget();
	return Super::RebuildWidget();
}

void UTunaSweeperHousingPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildPanelWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		HousingSubsystem->OnHousingStateChanged.RemoveAll(this);
		HousingSubsystem->OnHousingStateChanged.AddUObject(this, &UTunaSweeperHousingPanelWidget::HandleHousingStateChanged);
	}
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperHousingPanelWidget::RefreshHousingPanel);
	}

	RefreshHousingPanel();
}

void UTunaSweeperHousingPanelWidget::NativeDestruct()
{
	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		HousingSubsystem->OnHousingStateChanged.RemoveAll(this);
	}
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTunaSweeperHousingPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	if (GuideText && HousingSubsystem && HousingSubsystem->HasActivePlacement())
	{
		GuideText->SetColorAndOpacity(FSlateColor(
			HousingSubsystem->GetActivePlacementStatus() == ETunaSweeperHousingPlacementStatus::Valid
				? FLinearColor(0.60f, 0.92f, 1.0f, 1.0f)
				: FLinearColor(1.0f, 0.42f, 0.38f, 1.0f)));
	}
}

void UTunaSweeperHousingPanelWidget::BuildPanelWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	RootPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HousingRootPanel"));
	PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("HousingPanelStack"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HousingTitleText"));
	GuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HousingGuideText"));
	FacilityListScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("HousingFacilityList"));
	if (!RootPanel || !PanelStack || !TitleText || !GuideText || !FacilityListScrollBox)
	{
		return;
	}

	WidgetTree->RootWidget = RootPanel;
	RootPanel->SetPadding(FMargin(14.0f, 12.0f));
	RootPanel->SetBrush(TunaSweeperHousingPanel::MakePanelBrush(
		FVector2D(360.0f, 540.0f),
		FLinearColor(0.012f, 0.018f, 0.022f, 0.78f),
		8.0f,
		FLinearColor(0.34f, 0.78f, 0.92f, 0.42f),
		1.0f));
	RootPanel->SetContent(PanelStack);

	TitleText->SetText(TunaSweeperHousingPanel::ResolveUiText(
		GetGameInstance<UTunaSweeperGameInstance>(),
		TEXT("ui.housing.title"),
		TEXT("Bunker Housing")));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.96f, 1.0f, 1.0f)));
	TunaSweeperUIFont::ApplyFont(TitleText, 21, ETunaSweeperUIFontWeight::Bold);
	if (UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
	}

	GuideText->SetAutoWrapText(true);
	GuideText->SetWrapTextAt(316.0f);
	TunaSweeperUIFont::ApplyFont(GuideText, 13);
	if (UVerticalBoxSlot* GuideSlot = PanelStack->AddChildToVerticalBox(GuideText))
	{
		GuideSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	}

	if (UVerticalBoxSlot* ListSlot = PanelStack->AddChildToVerticalBox(FacilityListScrollBox))
	{
		FSlateChildSize Size;
		Size.SizeRule = ESlateSizeRule::Fill;
		Size.Value = 1.0f;
		ListSlot->SetSize(Size);
	}
}

void UTunaSweeperHousingPanelWidget::RebuildFacilityEntries(
	const TArray<FTunaSweeperHousingFacilityView>& FacilityViews)
{
	if (!FacilityListScrollBox)
	{
		return;
	}

	FacilityListScrollBox->ClearChildren();
	for (const FTunaSweeperHousingFacilityView& View : FacilityViews)
	{
		UTunaSweeperHousingFacilityEntryWidget* EntryWidget =
			CreateWidget<UTunaSweeperHousingFacilityEntryWidget>(
				GetOwningPlayer(),
				UTunaSweeperHousingFacilityEntryWidget::StaticClass());
		if (!EntryWidget)
		{
			continue;
		}

		EntryWidget->InitializeEntry(
			View,
			FTunaSweeperHousingEntryClickedDelegate::CreateUObject(
				this,
				&UTunaSweeperHousingPanelWidget::HandleFacilityClicked),
			FTunaSweeperHousingEntryStoreDelegate::CreateUObject(
				this,
				&UTunaSweeperHousingPanelWidget::HandleFacilityStoreRequested));
		FacilityListScrollBox->AddChild(EntryWidget);
	}
}

void UTunaSweeperHousingPanelWidget::HandleHousingStateChanged()
{
	RefreshHousingPanel();
}

void UTunaSweeperHousingPanelWidget::HandleFacilityClicked(FName FacilityId, FGuid InstanceId)
{
	if (ATunaSweeperPlayerController* TunaPlayerController = GetOwningPlayer<ATunaSweeperPlayerController>())
	{
		TunaPlayerController->StartHousingFacilityPlacement(FacilityId, InstanceId);
		return;
	}

	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		HousingSubsystem->StartPlacement(FacilityId, InstanceId);
	}
}

void UTunaSweeperHousingPanelWidget::HandleFacilityStoreRequested(FGuid InstanceId)
{
	if (UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr)
	{
		if (!HousingSubsystem->StoreFacility(InstanceId, true))
		{
			UE_LOG(
				LogTunaSweeperHousingPanel,
				Warning,
				TEXT("Failed to store housing facility from panel. InstanceId=%s"),
				*InstanceId.ToString());
		}
	}
}
