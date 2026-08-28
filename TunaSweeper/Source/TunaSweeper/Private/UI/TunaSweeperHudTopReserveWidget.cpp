#include "UI/TunaSweeperHudTopReserveWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "UI/TunaSweeperUIFont.h"

namespace TunaSweeperHudTopReserve
{
	const TCHAR* InventoryModeIconPath = TEXT("/Game/UI/Icons/T_UI_Mode_Inventory.T_UI_Mode_Inventory");
	const TCHAR* QuestModeIconPath = TEXT("/Game/UI/Icons/T_UI_Mode_Quest.T_UI_Mode_Quest");
	const TCHAR* MapModeIconPath = TEXT("/Game/UI/Icons/T_UI_Mode_Map.T_UI_Mode_Map");
	const TCHAR* MemoModeIconPath = TEXT("/Game/UI/Icons/T_UI_Mode_Memo.T_UI_Mode_Memo");
	const TCHAR* ResearchModeIconPath = TEXT("/Game/UI/Icons/T_UI_Mode_Memo.T_UI_Mode_Memo");
	constexpr float ModeIconSize = 28.0f;

	const TCHAR* ResolveIconPath(ETunaSweeperHudMode Mode)
	{
		switch (Mode)
		{
		case ETunaSweeperHudMode::Inventory:
			return InventoryModeIconPath;
		case ETunaSweeperHudMode::Quest:
			return QuestModeIconPath;
		case ETunaSweeperHudMode::Map:
			return MapModeIconPath;
		case ETunaSweeperHudMode::Memo:
			return MemoModeIconPath;
		case ETunaSweeperHudMode::Research:
			return ResearchModeIconPath;
		default:
			return nullptr;
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

	if (const TCHAR* IconPath = TunaSweeperHudTopReserve::ResolveIconPath(Mode))
	{
		if (UTexture2D* IconTexture = LoadObject<UTexture2D>(nullptr, IconPath))
		{
			Icon->SetBrushFromTexture(IconTexture, true);
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
		Button->SetRenderOpacity(bActive ? 1.0f : 0.72f);
	}

	UImage* ResolvedIcon = EnsureTabIcon(Mode, Button, Icon, IconWidgetName);
	if (ResolvedIcon)
	{
		ResolvedIcon->SetColorAndOpacity(
			bActive
				? FLinearColor(0.82f, 0.98f, 0.88f, 1.0f)
				: FLinearColor(0.74f, 0.80f, 0.82f, 1.0f));
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
	OnHudModeSelected.Broadcast(ETunaSweeperHudMode::Research);
}
