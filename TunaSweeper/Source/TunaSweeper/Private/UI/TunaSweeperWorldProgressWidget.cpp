#include "UI/TunaSweeperWorldProgressWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "InputCoreTypes.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Styling/SlateBrush.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperWorldProgressWidget
{
	constexpr float PanelWidth = 520.0f;
	constexpr float PanelHeight = 300.0f;
	constexpr float PanelPadding = 28.0f;
}

namespace
{
	FSlateBrush MakeProgressRoundedBoxBrush(
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

	FSlateFontInfo MakeProgressFont(UTextBlock* TextBlock, int32 Size)
	{
		return TunaSweeperUIFont::MakeFont(TextBlock, Size);
	}
}

void UTunaSweeperWorldProgressWidget::SetProgressActor(ATunaSweeperWorldProgressActor* InProgressActor)
{
	ActiveProgressActor = InProgressActor;
	RefreshView();
}

TSharedRef<SWidget> UTunaSweeperWorldProgressWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildProgressWidget();
	}

	return Super::RebuildWidget();
}

void UTunaSweeperWorldProgressWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	BuildProgressWidget();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (UseItemButton)
	{
		UseItemButton->OnClicked.RemoveDynamic(this, &UTunaSweeperWorldProgressWidget::HandleUseItemClicked);
		UseItemButton->OnClicked.AddDynamic(this, &UTunaSweeperWorldProgressWidget::HandleUseItemClicked);
	}

	if (RepairButton)
	{
		RepairButton->OnClicked.RemoveDynamic(this, &UTunaSweeperWorldProgressWidget::HandleRepairClicked);
		RepairButton->OnClicked.AddDynamic(this, &UTunaSweeperWorldProgressWidget::HandleRepairClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UTunaSweeperWorldProgressWidget::HandleCloseClicked);
		CloseButton->OnClicked.AddDynamic(this, &UTunaSweeperWorldProgressWidget::HandleCloseClicked);
	}

	RefreshView();
}

FReply UTunaSweeperWorldProgressWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		ClosePanel();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UTunaSweeperWorldProgressWidget::HandleUseItemClicked()
{
	if (ATunaSweeperWorldProgressActor* ProgressActor = ActiveProgressActor.Get())
	{
		ProgressActor->UseAvailableRequiredItems(true);
	}

	RefreshView();
}

void UTunaSweeperWorldProgressWidget::HandleRepairClicked()
{
	ATunaSweeperWorldProgressActor* ProgressActor = ActiveProgressActor.Get();
	if (ProgressActor && ProgressActor->Repair(true))
	{
		ClosePanel();
		return;
	}

	RefreshView();
}

void UTunaSweeperWorldProgressWidget::HandleCloseClicked()
{
	ClosePanel();
}

void UTunaSweeperWorldProgressWidget::BuildProgressWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("WorldProgressRoot"));
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("WorldProgressBackdrop"));
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("WorldProgressPanel"));
	UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(),
		TEXT("WorldProgressPanelStack"));
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("WorldProgressHeaderRow"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("WorldProgressTitleText"));
	CloseButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("WorldProgressCloseButton"));
	UTextBlock* CloseText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("WorldProgressCloseText"));
	ProgressText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("WorldProgressProgressText"));
	InventoryText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("WorldProgressInventoryText"));
	UHorizontalBox* ButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TEXT("WorldProgressButtonRow"));
	UseItemButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("WorldProgressUseItemButton"));
	UseItemButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("WorldProgressUseItemButtonText"));
	RepairButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(),
		TEXT("WorldProgressRepairButton"));
	RepairButtonText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("WorldProgressRepairButtonText"));

	if (!RootCanvas ||
		!Backdrop ||
		!Panel ||
		!PanelStack ||
		!HeaderRow ||
		!TitleText ||
		!CloseButton ||
		!CloseText ||
		!ProgressText ||
		!InventoryText ||
		!ButtonRow ||
		!UseItemButton ||
		!UseItemButtonText ||
		!RepairButton ||
		!RepairButtonText)
	{
		return;
	}

	WidgetTree->RootWidget = RootCanvas;
	RootCanvas->SetVisibility(ESlateVisibility::Visible);

	Backdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));
	if (UCanvasPanelSlot* BackdropSlot = RootCanvas->AddChildToCanvas(Backdrop))
	{
		BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		BackdropSlot->SetOffsets(FMargin(0.0f));
		BackdropSlot->SetAlignment(FVector2D::ZeroVector);
		BackdropSlot->SetZOrder(0);
	}

	Panel->SetBrush(MakeProgressRoundedBoxBrush(
		FVector2D(TunaSweeperWorldProgressWidget::PanelWidth, TunaSweeperWorldProgressWidget::PanelHeight),
		FLinearColor(0.11f, 0.12f, 0.13f, 0.96f),
		8.0f,
		FLinearColor(0.45f, 0.50f, 0.52f, 0.65f),
		1.25f));
	Panel->SetPadding(FMargin(TunaSweeperWorldProgressWidget::PanelPadding));
	if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(Panel))
	{
		PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		PanelSlot->SetPosition(FVector2D::ZeroVector);
		PanelSlot->SetSize(FVector2D(
			TunaSweeperWorldProgressWidget::PanelWidth,
			TunaSweeperWorldProgressWidget::PanelHeight));
		PanelSlot->SetZOrder(1);
	}
	Panel->SetContent(PanelStack);

	TitleText->SetFont(MakeProgressFont(TitleText, 25));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.98f, 1.0f, 1.0f)));
	TitleText->SetJustification(ETextJustify::Left);
	if (UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(TitleText))
	{
		TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleSlot->SetVerticalAlignment(VAlign_Center);
	}

	CloseText->SetText(FText::FromString(TEXT("X")));
	CloseText->SetFont(MakeProgressFont(CloseText, 18));
	CloseText->SetJustification(ETextJustify::Center);
	CloseButton->SetContent(CloseText);
	if (UHorizontalBoxSlot* CloseSlot = HeaderRow->AddChildToHorizontalBox(CloseButton))
	{
		CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		CloseSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* HeaderSlot = PanelStack->AddChildToVerticalBox(HeaderRow))
	{
		HeaderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));
	}

	ProgressText->SetFont(MakeProgressFont(ProgressText, 22));
	ProgressText->SetColorAndOpacity(FSlateColor(FLinearColor(0.90f, 0.94f, 0.96f, 0.98f)));
	ProgressText->SetJustification(ETextJustify::Left);
	if (UVerticalBoxSlot* ProgressSlot = PanelStack->AddChildToVerticalBox(ProgressText))
	{
		ProgressSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		ProgressSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
	}

	InventoryText->SetFont(MakeProgressFont(InventoryText, 19));
	InventoryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.68f, 0.80f, 0.84f, 0.95f)));
	InventoryText->SetJustification(ETextJustify::Left);
	if (UVerticalBoxSlot* InventorySlot = PanelStack->AddChildToVerticalBox(InventoryText))
	{
		InventorySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UseItemButtonText->SetText(FText::FromString(TEXT("\uBAA9\uC7AC \uC0AC\uC6A9")));
	UseItemButtonText->SetFont(MakeProgressFont(UseItemButtonText, 19));
	UseItemButtonText->SetJustification(ETextJustify::Center);
	UseItemButton->SetContent(UseItemButtonText);
	if (UHorizontalBoxSlot* UseItemSlot = ButtonRow->AddChildToHorizontalBox(UseItemButton))
	{
		UseItemSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UseItemSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
	}

	RepairButtonText->SetText(FText::FromString(TEXT("\uC218\uB9AC")));
	RepairButtonText->SetFont(MakeProgressFont(RepairButtonText, 19));
	RepairButtonText->SetJustification(ETextJustify::Center);
	RepairButton->SetContent(RepairButtonText);
	if (UHorizontalBoxSlot* RepairSlot = ButtonRow->AddChildToHorizontalBox(RepairButton))
	{
		RepairSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		RepairSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
	}

	if (UVerticalBoxSlot* ButtonSlot = PanelStack->AddChildToVerticalBox(ButtonRow))
	{
		ButtonSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
	}
}

void UTunaSweeperWorldProgressWidget::RefreshView()
{
	ATunaSweeperWorldProgressActor* ProgressActor = ActiveProgressActor.Get();
	if (!ProgressActor)
	{
		if (UseItemButton)
		{
			UseItemButton->SetIsEnabled(false);
		}
		if (RepairButton)
		{
			RepairButton->SetIsEnabled(false);
		}
		return;
	}

	const int32 ProgressQuantity = ProgressActor->GetProgressQuantity();
	const int32 RequiredQuantity = ProgressActor->GetRequiredQuantity();
	const int32 OwnedQuantity = ProgressActor->GetOwnedRequiredItemCount();
	const FText ItemName = ProgressActor->GetRequiredItemDisplayName();

	if (TitleText)
	{
		TitleText->SetText(ProgressActor->GetDisplayName());
	}

	if (ProgressText)
	{
		ProgressText->SetText(FText::Format(
			FText::FromString(TEXT("\uD22C\uC785\uB41C {0} {1}/{2}")),
			ItemName,
			FText::AsNumber(ProgressQuantity),
			FText::AsNumber(RequiredQuantity)));
	}

	if (InventoryText)
	{
		InventoryText->SetText(FText::Format(
			FText::FromString(TEXT("\uC18C\uC9C0 {0} {1}/{2}")),
			ItemName,
			FText::AsNumber(OwnedQuantity),
			FText::AsNumber(RequiredQuantity)));
	}

	if (UseItemButton)
	{
		UseItemButton->SetIsEnabled(OwnedQuantity > 0 && ProgressQuantity < RequiredQuantity);
	}

	if (RepairButton)
	{
		RepairButton->SetIsEnabled(ProgressActor->IsRepairReady());
	}
}

void UTunaSweeperWorldProgressWidget::ClosePanel()
{
	RemoveFromParent();

	if (ATunaSweeperPlayerController* TunaPlayerController = GetOwningPlayer<ATunaSweeperPlayerController>())
	{
		TunaPlayerController->ApplyDefaultGameInputMode();
	}
}
