#include "UI/TunaSweeperGameHudWidget.h"

#include "Components/Widget.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "UI/TunaSweeperHudExternalPanelWidget.h"
#include "UI/TunaSweeperHudInventoryAreaWidget.h"
#include "UI/TunaSweeperHudItemInfoPanelWidget.h"

void UTunaSweeperGameHudWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetCenterPanelsVisible(false);
	RefreshBottomStatusFromGameInstance();
}

void UTunaSweeperGameHudWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	RefreshBottomStatusFromGameInstance();
}

void UTunaSweeperGameHudWidget::SetCenterPanelsVisible(bool bVisible)
{
	if (CenterContentPanel)
	{
		CenterContentPanel->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperGameHudWidget::SetInventoryAreaVisible(bool bVisible)
{
	if (bVisible)
	{
		SetCenterPanelsVisible(true);
	}

	if (InventoryAreaWidget)
	{
		InventoryAreaWidget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperGameHudWidget::SetItemInfoPanelVisible(bool bVisible)
{
	if (bVisible)
	{
		SetCenterPanelsVisible(true);
	}

	if (ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperGameHudWidget::ShowExternalPanel(ETunaSweeperHudExternalPanelMode PanelMode)
{
	if (PanelMode != ETunaSweeperHudExternalPanelMode::None)
	{
		SetCenterPanelsVisible(true);
	}

	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetVisibility(
			PanelMode == ETunaSweeperHudExternalPanelMode::None
				? ESlateVisibility::Collapsed
				: ESlateVisibility::SelfHitTestInvisible);
		ExternalPanelWidget->SetExternalPanelMode(PanelMode);
	}
}

void UTunaSweeperGameHudWidget::RefreshBottomStatusFromGameInstance()
{
	if (!BottomStatusWidget)
	{
		return;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (TunaGameInstance)
	{
		BottomStatusWidget->SetHudState(TunaGameInstance->PlayerHudState);
	}
}
