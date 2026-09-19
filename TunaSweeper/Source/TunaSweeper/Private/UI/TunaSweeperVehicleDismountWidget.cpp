#include "UI/TunaSweeperVehicleDismountWidget.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UI/TunaSweeperUIFont.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UTunaSweeperVehicleDismountWidget::SetVehicle(ATunaSweeperATVActor* InVehicle)
{
	Vehicle = InVehicle;
	// Apply the whole slot atomically; SetDesiredSizeInViewport resets anchors in UE 5.7.
	if (auto* Viewport = UGameViewportSubsystem::Get(GetWorld()))
	{
		auto ViewportSlot = Viewport->GetWidgetSlot(this);
		ViewportSlot.Anchors = FAnchors(0.5f, 0.82f);
		ViewportSlot.Alignment = FVector2D(0.5f, 0.5f);
		ViewportSlot.Offsets = FMargin(0, -14, 180, 86);
		Viewport->SetWidgetSlot(this, ViewportSlot);
	}
	RefreshState();
}

void UTunaSweeperVehicleDismountWidget::SetDismountHintVisible(bool bVisible)
{
	bDismountHintVisible = bVisible;
	RefreshState();
}

void UTunaSweeperVehicleDismountWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	RefreshState();
}

void UTunaSweeperVehicleDismountWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshState();
}

void UTunaSweeperVehicleDismountWidget::RefreshState()
{
	if (DurabilityBar) DurabilityBar->SetPercent(Vehicle.IsValid() ? Vehicle->GetDurabilityRatio() : 0.0f);
	if (DismountPanel) DismountPanel->SetVisibility(bDismountHintVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	if (const auto* GI = GetGameInstance<UTunaSweeperGameInstance>())
	{
		if (DismountText) DismountText->SetText(GI->ResolveLocalizedText(TEXT("ui.vehicle.dismount"), FText::GetEmpty()));
		if (DismountKeyText) DismountKeyText->SetText(GI->ResolveLocalizedText(TEXT("ui.key.x"), FText::GetEmpty()));
	}
}
