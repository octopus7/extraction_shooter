#include "UI/TunaSweeperHudTopReserveWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUIStyle.h"

namespace TunaSweeperHudTopReserve
{
	const TCHAR* ModeIconAtlasPath = TEXT("/Game/UI/Icons/T_UI_Mode_ColorAtlas.T_UI_Mode_ColorAtlas");
	constexpr float ModeIconSize = 36.0f;

	int32 ResolveIconCell(ETunaSweeperHudMode Mode)
	{
		switch (Mode)
		{
		case ETunaSweeperHudMode::Inventory:
			return 0;
		case ETunaSweeperHudMode::Quest:
			return 1;
		case ETunaSweeperHudMode::Map:
			return 2;
		case ETunaSweeperHudMode::Memo:
			return 3;
		case ETunaSweeperHudMode::Research:
			return 4;
		default:
			return INDEX_NONE;
		}
	}
}

void UTunaSweeperHudTopReserveWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	CacheNamedWidgets();

	if (InventoryModeButton)
	{
		InventoryModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleInventoryModeClicked);
		InventoryModeButton->OnClicked.AddDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleInventoryModeClicked);
	}

	if (QuestModeButton)
	{
		QuestModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleQuestModeClicked);
		QuestModeButton->OnClicked.AddDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleQuestModeClicked);
	}

	if (MapModeButton)
	{
		MapModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMapModeClicked);
		MapModeButton->OnClicked.AddDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMapModeClicked);
	}

	if (MemoModeButton)
	{
		MemoModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMemoModeClicked);
		MemoModeButton->OnClicked.AddDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMemoModeClicked);
	}
	if (ResearchModeButton)
	{
		ResearchModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleResearchModeClicked);
		ResearchModeButton->OnClicked.AddDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleResearchModeClicked);
	}

	RefreshTabVisuals();
}

void UTunaSweeperHudTopReserveWidget::NativeDestruct()
{
	if (InventoryModeButton)
	{
		InventoryModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleInventoryModeClicked);
	}

	if (QuestModeButton)
	{
		QuestModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleQuestModeClicked);
	}

	if (MapModeButton)
	{
		MapModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMapModeClicked);
	}

	if (MemoModeButton)
	{
		MemoModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleMemoModeClicked);
	}
	if (ResearchModeButton)
	{
		ResearchModeButton->OnClicked.RemoveDynamic(this, &UTunaSweeperHudTopReserveWidget::HandleResearchModeClicked);
	}

	Super::NativeDestruct();
}

void UTunaSweeperHudTopReserveWidget::SetActiveMode(ETunaSweeperHudMode InActiveMode)
{
	ActiveMode = InActiveMode;
	RefreshTabVisuals();
}

void UTunaSweeperHudTopReserveWidget::RefreshTabVisuals()
{
	CacheNamedWidgets();
	SetTabVisual(ETunaSweeperHudMode::Inventory, InventoryModeButton, InventoryModeIcon, TEXT("InventoryModeIcon"));
	SetTabVisual(ETunaSweeperHudMode::Quest, QuestModeButton, QuestModeIcon, TEXT("QuestModeIcon"));
	SetTabVisual(ETunaSweeperHudMode::Map, MapModeButton, MapModeIcon, TEXT("MapModeIcon"));
	SetTabVisual(ETunaSweeperHudMode::Memo, MemoModeButton, MemoModeIcon, TEXT("MemoModeIcon"));
	SetTabVisual(ETunaSweeperHudMode::Research, ResearchModeButton, ResearchModeIcon, TEXT("ResearchModeIcon"));
	const ESlateVisibility ResearchVisibility = TunaSweeperBuildFlavor::IsDemo()
		? ESlateVisibility::Collapsed : ESlateVisibility::Visible;
	if (ResearchModeButton) ResearchModeButton->SetVisibility(ResearchVisibility);
	if (UWidget* ResearchFrame = WidgetTree ? WidgetTree->FindWidget(TEXT("ResearchModeButtonFrame")) : nullptr)
		ResearchFrame->SetVisibility(ResearchVisibility);
}

void UTunaSweeperHudTopReserveWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!InventoryModeButton)
	{
		InventoryModeButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("InventoryModeButton"))));
	}
	if (!QuestModeButton)
	{
		QuestModeButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("QuestModeButton"))));
	}
	if (!MapModeButton)
	{
		MapModeButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("MapModeButton"))));
	}
	if (!MemoModeButton)
	{
		MemoModeButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("MemoModeButton"))));
	}
	if (!ResearchModeButton)
	{
		ResearchModeButton = Cast<UButton>(WidgetTree->FindWidget(FName(TEXT("ResearchModeButton"))));
	}

	if (!InventoryModeIcon)
	{
		InventoryModeIcon = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("InventoryModeIcon"))));
	}
	if (!QuestModeIcon)
	{
		QuestModeIcon = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("QuestModeIcon"))));
	}
	if (!MapModeIcon)
	{
		MapModeIcon = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("MapModeIcon"))));
	}
	if (!MemoModeIcon)
	{
		MemoModeIcon = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("MemoModeIcon"))));
	}
	if (!ResearchModeIcon)
	{
		ResearchModeIcon = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("ResearchModeIcon"))));
	}
}

UImage* UTunaSweeperHudTopReserveWidget::EnsureTabIcon(
	ETunaSweeperHudMode Mode,
	UButton* Button,
	TObjectPtr<UImage>& Icon,
	const TCHAR* IconWidgetName)
{
	if (!WidgetTree)
	{
		return Icon;
	}

	if (!Icon)
	{
		const FName DesiredName(IconWidgetName);
		const FName IconName = WidgetTree->FindWidget(DesiredName)
			? MakeUniqueObjectName(WidgetTree, UImage::StaticClass(), DesiredName)
			: DesiredName;
		Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
	}

	if (!Icon)
	{
		return nullptr;
	}

	if (Button && Icon->GetParent() != Button)
	{
		Icon->RemoveFromParent();
		Button->SetContent(Icon);
	}

	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Icon->Slot))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Center);
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
		ButtonSlot->SetPadding(FMargin(0.0f));
	}

	const int32 IconCell = TunaSweeperHudTopReserve::ResolveIconCell(Mode);
	if (IconCell != INDEX_NONE)
	{
		if (UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, TunaSweeperHudTopReserve::ModeIconAtlasPath))
		{
			FSlateBrush Brush = Icon->GetBrush();
			Brush.SetResourceObject(IconTexture);
			Brush.DrawAs = ESlateBrushDrawType::Image;
			Brush.ImageSize = FVector2D(TunaSweeperHudTopReserve::ModeIconSize);
			const FVector2f CellSize(1.0f / 3.0f, 0.5f);
			const FVector2f MinUv((IconCell % 3) * CellSize.X, (IconCell / 3) * CellSize.Y);
			Brush.SetUVRegion(FBox2f(MinUv, MinUv + CellSize));
			Icon->SetBrush(Brush);
		}
	}

	Icon->SetDesiredSizeOverride(FVector2D(
		TunaSweeperHudTopReserve::ModeIconSize,
		TunaSweeperHudTopReserve::ModeIconSize));
	Icon->SetVisibility(ESlateVisibility::HitTestInvisible);
	Icon->SetOpacity(1.0f);
	return Icon;
}

void UTunaSweeperHudTopReserveWidget::SetTabVisual(
	ETunaSweeperHudMode Mode,
	UButton* Button,
	TObjectPtr<UImage>& Icon,
	const TCHAR* IconWidgetName)
{
	const bool bActive = ActiveMode == Mode;

	if (Button)
	{
		TunaSweeperUIStyle::ApplyButton(Button, TunaSweeperUIStyle::EButtonRole::Tab, bActive);
		FButtonStyle CompactTabStyle = Button->GetStyle();
		CompactTabStyle.SetNormalPadding(FMargin(5.0f));
		CompactTabStyle.SetPressedPadding(FMargin(5.0f, 6.0f, 5.0f, 4.0f));
		for (FSlateBrush* Brush : {&CompactTabStyle.Normal, &CompactTabStyle.Hovered, &CompactTabStyle.Pressed, &CompactTabStyle.Disabled})
			Brush->OutlineSettings.CornerRadii = FVector4(0.0f, 0.0f, 5.0f, 5.0f);
		Button->SetStyle(CompactTabStyle);
	}

	UImage* ResolvedIcon = EnsureTabIcon(Mode, Button, Icon, IconWidgetName);
	if (ResolvedIcon)
	{
		ResolvedIcon->SetColorAndOpacity(FLinearColor::White);
	}
}

void UTunaSweeperHudTopReserveWidget::HandleInventoryModeClicked()
{
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Inventory);
}

void UTunaSweeperHudTopReserveWidget::HandleQuestModeClicked()
{
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Quest);
}

void UTunaSweeperHudTopReserveWidget::HandleMapModeClicked()
{
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Map);
}

void UTunaSweeperHudTopReserveWidget::HandleMemoModeClicked()
{
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Memo);
}

void UTunaSweeperHudTopReserveWidget::HandleResearchModeClicked()
{
	if (TunaSweeperBuildFlavor::IsDemo()) return;
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Research);
}
