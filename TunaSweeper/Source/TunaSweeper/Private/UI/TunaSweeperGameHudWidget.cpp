#include "UI/TunaSweeperGameHudWidget.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/Widget.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
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

void UTunaSweeperGameHudWidget::ShowInventoryOnlyPanel()
{
	SetCenterPanelsVisible(true);
	SetInventoryAreaVisible(true);
	SetItemInfoPanelVisible(false);
	ShowExternalPanel(ETunaSweeperHudExternalPanelMode::None);
}

void UTunaSweeperGameHudWidget::ToggleInventoryOnlyPanel()
{
	const bool bCenterVisible = CenterContentPanel && CenterContentPanel->GetVisibility() != ESlateVisibility::Collapsed;

	if (!bCenterVisible)
	{
		ShowInventoryOnlyPanel();
	}
	else
	{
		SetInventoryAreaVisible(false);
		SetItemInfoPanelVisible(false);
		ShowExternalPanel(ETunaSweeperHudExternalPanelMode::None);
		SetCenterPanelsVisible(false);
	}
}

void UTunaSweeperGameHudWidget::ShowLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance)
{
	SetCenterPanelsVisible(true);
	SetInventoryAreaVisible(true);
	SetItemInfoPanelVisible(false);

	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ExternalPanelWidget->SetLootContainerInstance(ContainerInstance);
	}
}

void UTunaSweeperGameHudWidget::RefreshBottomStatusFromGameInstance()
{
	if (!BottomStatusWidget)
	{
		return;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperPlayerHudState HudState = TunaGameInstance ? TunaGameInstance->PlayerHudState : FTunaSweeperPlayerHudState();

	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		const APawn* Pawn = PlayerController->GetPawn();
		const UTunaSweeperVitalsComponent* VitalsComponent = nullptr;
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(Pawn))
		{
			VitalsComponent = TunaCharacter->GetVitalsComponent();
		}
		else if (Pawn)
		{
			VitalsComponent = Pawn->FindComponentByClass<UTunaSweeperVitalsComponent>();
		}

		if (VitalsComponent)
		{
			const FTunaSweeperVitalsState& VitalsState = VitalsComponent->GetVitalsState();
			HudState.Health = VitalsState.Health;
			HudState.Food = VitalsState.Food;
			HudState.Hydration = VitalsState.Hydration;
		}
	}

	BottomStatusWidget->SetHudState(HudState);
}
