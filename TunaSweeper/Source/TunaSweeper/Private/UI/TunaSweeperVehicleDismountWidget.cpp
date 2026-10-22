#include "UI/TunaSweeperVehicleDismountWidget.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperVehicleFuelWidget.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "Blueprint/GameViewportSubsystem.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/ProgressBar.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

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
	RefreshFuelGauge();
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
	if (!FuelGauge && GetOwningPlayer())
	{
		FuelGauge = CreateWidget<UTunaSweeperVehicleFuelWidget>(GetOwningPlayer(), UTunaSweeperVehicleFuelWidget::StaticClass());
		if (FuelGauge)
		{
			FuelGauge->SetVisibility(ESlateVisibility::Collapsed);
			FuelGauge->AddToPlayerScreen(21);
		}
	}
	RefreshState();
	RefreshFuelGauge();
}

void UTunaSweeperVehicleDismountWidget::NativeDestruct()
{
	if (FuelGauge) FuelGauge->RemoveFromParent();
	FuelGauge = nullptr;
	Super::NativeDestruct();
}

void UTunaSweeperVehicleDismountWidget::SetVisibility(ESlateVisibility InVisibility)
{
	const bool bChanged = GetVisibility() != InVisibility;
	Super::SetVisibility(InVisibility);
	// A collapsed HUD stops ticking, so synchronize its independently positioned gauge immediately.
	if (bChanged) RefreshFuelGauge();
}

void UTunaSweeperVehicleDismountWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshState();
	RefreshFuelGauge();
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

void UTunaSweeperVehicleDismountWidget::RefreshFuelGauge()
{
	if (!FuelGauge) return;
	const auto* ATV = Vehicle.Get();
	auto* Controller = GetOwningPlayer();
	const auto* Rider = ATV && ATV->MountComponent ? ATV->MountComponent->GetRider() : nullptr;
	if (!IsVisible() || !ATV || ATV->IsActorBeingDestroyed() || ATV->IsVehicleDestroyed() ||
		!Controller || !Rider || Rider->IsDead() || Rider->GetController() != Controller)
	{
		FuelGauge->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FVector Center = ATV->VehicleMesh ? ATV->VehicleMesh->Bounds.Origin : ATV->GetActorLocation();
	const FVector Extent = ATV->VehicleMesh ? ATV->VehicleMesh->Bounds.BoxExtent : FVector(80.0f, 60.0f, 50.0f);
	FVector2D ScreenCenter;
	// This projection already removes viewport DPI scaling and is relative to this player's screen.
	if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(Controller, Center, ScreenCenter, true))
	{
		FuelGauge->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const FVector2D ScreenSize = UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(Controller).GetLocalSize();
	constexpr float GaugeWidth = 112.0f;
	constexpr float GaugeHeight = 28.0f;
	constexpr float Gap = 12.0f;
	constexpr float ScreenMargin = 8.0f;
	if (ScreenSize.X < GaugeWidth + ScreenMargin * 2.0f || ScreenSize.Y < GaugeHeight + ScreenMargin * 2.0f ||
		ScreenCenter.X < 0.0f || ScreenCenter.X > ScreenSize.X || ScreenCenter.Y < 0.0f || ScreenCenter.Y > ScreenSize.Y)
	{
		FuelGauge->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	double LeftEdge = ScreenCenter.X;
	double RightEdge = ScreenCenter.X;
	for (int32 Corner = 0; Corner < 8; ++Corner)
	{
		const FVector CornerOffset(Extent.X * ((Corner & 1) ? 1.0f : -1.0f),
			Extent.Y * ((Corner & 2) ? 1.0f : -1.0f), Extent.Z * ((Corner & 4) ? 1.0f : -1.0f));
		FVector2D CornerPosition;
		if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(Controller, Center + CornerOffset, CornerPosition, true))
		{
			LeftEdge = FMath::Min(LeftEdge, CornerPosition.X);
			RightEdge = FMath::Max(RightEdge, CornerPosition.X);
		}
	}

	FVector2D Position(RightEdge + Gap, ScreenCenter.Y - GaugeHeight * 0.5f);
	if (Position.X + GaugeWidth > ScreenSize.X - ScreenMargin) Position.X = LeftEdge - Gap - GaugeWidth;
	Position.X = FMath::Clamp(Position.X, static_cast<double>(ScreenMargin), ScreenSize.X - GaugeWidth - ScreenMargin);
	Position.Y = FMath::Clamp(Position.Y, static_cast<double>(ScreenMargin), ScreenSize.Y - GaugeHeight - ScreenMargin);
	if (auto* Viewport = UGameViewportSubsystem::Get(GetWorld()))
	{
		auto GaugeSlot = Viewport->GetWidgetSlot(FuelGauge);
		GaugeSlot.Anchors = FAnchors(0.0f, 0.0f);
		GaugeSlot.Alignment = FVector2D::ZeroVector;
		GaugeSlot.Offsets = FMargin(Position.X, Position.Y, GaugeWidth, GaugeHeight);
		Viewport->SetWidgetSlot(FuelGauge, GaugeSlot);
		FuelGauge->SetFuelRatio(ATV->GetFuelRatio());
		FuelGauge->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else FuelGauge->SetVisibility(ESlateVisibility::Collapsed);
}
