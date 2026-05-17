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

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
		TunaGameInstance->OnSelectedInventoryItemChanged.AddUObject(this, &UTunaSweeperGameHudWidget::HandleSelectedInventoryItemChanged);
	}

	SetCenterPanelsVisible(false);
	SetItemInfoPanelVisible(false);
	RefreshBottomStatusFromGameInstance();
}

void UTunaSweeperGameHudWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnSelectedInventoryItemChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
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
	HandleSelectedInventoryItemChanged();
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
		if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
		{
			TunaGameInstance->ClearSelectedItemSelection();
		}

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
	HandleSelectedInventoryItemChanged();

	if (ExternalPanelWidget)
	{
		ExternalPanelWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ExternalPanelWidget->SetLootContainerInstance(ContainerInstance);
	}
}

bool UTunaSweeperGameHudWidget::IsInventoryUiOpen() const
{
	auto IsWidgetVisible = [](const UWidget* Widget)
	{
		if (!Widget)
		{
			return false;
		}

		const ESlateVisibility Visibility = Widget->GetVisibility();
		return Visibility != ESlateVisibility::Collapsed && Visibility != ESlateVisibility::Hidden;
	};

	if (CenterContentPanel)
	{
		return IsWidgetVisible(CenterContentPanel);
	}

	return IsWidgetVisible(InventoryAreaWidget) || IsWidgetVisible(ExternalPanelWidget);
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

void UTunaSweeperGameHudWidget::HandleSelectedInventoryItemChanged()
{
	const bool bCenterVisible = CenterContentPanel && CenterContentPanel->GetVisibility() != ESlateVisibility::Collapsed;
	const bool bHasSelection = bCenterVisible &&
		GetGameInstance<UTunaSweeperGameInstance>() &&
		GetGameInstance<UTunaSweeperGameInstance>()->HasSelectedInventoryItem();

	SetItemInfoPanelVisible(bHasSelection);
	if (bHasSelection && ItemInfoPanelWidget)
	{
		ItemInfoPanelWidget->RefreshSelectedItemInfo();
	}
}
