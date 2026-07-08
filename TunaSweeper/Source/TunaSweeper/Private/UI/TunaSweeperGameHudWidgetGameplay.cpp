#include "TunaSweeperGameHudWidgetShared.h"

bool UTunaSweeperGameHudWidget::IsDialogueSequenceActive() const
{
	const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer());
	return TunaPlayerController && TunaPlayerController->IsDialogueSequenceActive();
}

bool UTunaSweeperGameHudWidget::IsHousingModeActive() const
{
	const UTunaSweeperHousingSubsystem* HousingSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperHousingSubsystem>()
		: nullptr;
	return HousingSubsystem && HousingSubsystem->IsHousingModeOpen();
}

bool UTunaSweeperGameHudWidget::IsGameplayBottomHudSuppressed() const
{
	return IsDialogueSequenceActive() || IsInventoryUiOpen() || IsHousingModeActive();
}

bool UTunaSweeperGameHudWidget::IsBunkerMap() const
{
	const UWorld* World = GetWorld();
	return World && World->GetMapName().EndsWith(TEXT("BunkerMap"));
}

FName UTunaSweeperGameHudWidget::GetSelectedWeaponTypeTag() const
{
	const APlayerController* PlayerController = GetOwningPlayer();
	const ATunaSweeperTopDownCharacter* TunaCharacter = PlayerController
		? Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn())
		: nullptr;
	if (!TunaCharacter || TunaCharacter->IsDead())
	{
		return NAME_None;
	}

	const int32 SelectedWeaponSlotNumber = TunaCharacter->GetSelectedWeaponSlotNumber();
	if (SelectedWeaponSlotNumber <= 0)
	{
		return NAME_None;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return NAME_None;
	}

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		return NAME_None;
	}

	return WeaponDefinition.WeaponTypeTag;
}

bool UTunaSweeperGameHudWidget::IsWeaponCrosshairSuppressed() const
{
	if (IsInventoryUiOpen() || IsDialogueSequenceActive() || IsHousingModeActive())
	{
		return true;
	}

	return IsReloadGaugeReplacingCrosshair();
}

bool UTunaSweeperGameHudWidget::IsReloadGaugeReplacingCrosshair(const ATunaSweeperTopDownCharacter* TunaCharacter) const
{
	if (IsInventoryUiOpen() || IsDialogueSequenceActive() || IsHousingModeActive())
	{
		return false;
	}

	const ATunaSweeperTopDownCharacter* ResolvedCharacter = TunaCharacter;
	if (!ResolvedCharacter)
	{
		const APlayerController* PlayerController = GetOwningPlayer();
		ResolvedCharacter = PlayerController
			? Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn())
			: nullptr;
	}

	return ResolvedCharacter && ResolvedCharacter->IsWeaponReloading();
}

bool UTunaSweeperGameHudWidget::TryGetWeaponCrosshairLocalPosition(const FGeometry& AllottedGeometry, FVector2D& OutLocalPosition) const
{
	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	const FVector2D CursorAbsolutePosition = FSlateApplication::Get().GetCursorPos();
	const FVector2D CursorLocalPosition = AllottedGeometry.AbsoluteToLocal(CursorAbsolutePosition);
	const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
	if (CursorLocalPosition.X < 0.0f ||
		CursorLocalPosition.Y < 0.0f ||
		CursorLocalPosition.X > LocalSize.X ||
		CursorLocalPosition.Y > LocalSize.Y)
	{
		return false;
	}

	OutLocalPosition = CursorLocalPosition;
	if (const APlayerController* PlayerController = GetOwningPlayer())
	{
		if (const ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn()))
		{
			OutLocalPosition += TunaCharacter->GetWeaponRecoilCrosshairScreenOffset();
		}
	}

	return true;
}

void UTunaSweeperGameHudWidget::UpdateCenterCancelableActionGaugePlacement(
	const FGeometry& AllottedGeometry,
	bool bUseCrosshairPosition)
{
	if (!CenterCancelableActionGaugeRoot)
	{
		return;
	}

	UCanvasPanelSlot* GaugeSlot = Cast<UCanvasPanelSlot>(CenterCancelableActionGaugeRoot->Slot);
	if (!GaugeSlot)
	{
		return;
	}

	if (!bCenterCancelableActionGaugeSlotLayoutCached)
	{
		DefaultCenterCancelableActionGaugeAnchors = GaugeSlot->GetAnchors();
		DefaultCenterCancelableActionGaugeAlignment = GaugeSlot->GetAlignment();
		DefaultCenterCancelableActionGaugePosition = GaugeSlot->GetPosition();
		DefaultCenterCancelableActionGaugeSize = GaugeSlot->GetSize();
		bCenterCancelableActionGaugeSlotLayoutCached = true;
	}

	GaugeSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	GaugeSlot->SetSize(DefaultCenterCancelableActionGaugeSize);

	FVector2D CrosshairLocalPosition = FVector2D::ZeroVector;
	if (bUseCrosshairPosition && TryGetWeaponCrosshairLocalPosition(AllottedGeometry, CrosshairLocalPosition))
	{
		GaugeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		GaugeSlot->SetPosition(CrosshairLocalPosition);
		return;
	}

	GaugeSlot->SetAnchors(DefaultCenterCancelableActionGaugeAnchors);
	GaugeSlot->SetAlignment(DefaultCenterCancelableActionGaugeAlignment);
	GaugeSlot->SetPosition(DefaultCenterCancelableActionGaugePosition);
	GaugeSlot->SetSize(DefaultCenterCancelableActionGaugeSize);
}

void UTunaSweeperGameHudWidget::UpdateMouseCursorForReloadGauge(bool bShouldHideCursor)
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (!PlayerController)
	{
		bReloadGaugeHidMouseCursor = false;
		return;
	}

	if (bShouldHideCursor)
	{
		PlayerController->CurrentMouseCursor = EMouseCursor::None;
		bReloadGaugeHidMouseCursor = true;
		return;
	}

	if (bReloadGaugeHidMouseCursor)
	{
		PlayerController->CurrentMouseCursor = PlayerController->DefaultMouseCursor;
		bReloadGaugeHidMouseCursor = false;
	}
}

void UTunaSweeperGameHudWidget::UpdateCrosshairState(float InDeltaTime)
{
	float TargetAimAlpha = 0.0f;
	if (!IsWeaponCrosshairSuppressed() && IsPrecisionCrosshairWeaponType(GetSelectedWeaponTypeTag()))
	{
		const APlayerController* PlayerController = GetOwningPlayer();
		const ATunaSweeperTopDownCharacter* TunaCharacter = PlayerController
			? Cast<ATunaSweeperTopDownCharacter>(PlayerController->GetPawn())
			: nullptr;
		TargetAimAlpha = TunaCharacter && TunaCharacter->IsAiming() ? 1.0f : 0.0f;
	}

	PrecisionCrosshairAimAlpha = FMath::FInterpTo(
		PrecisionCrosshairAimAlpha,
		TargetAimAlpha,
		FMath::Max(0.0f, InDeltaTime),
		FMath::Max(0.0f, PrecisionCrosshairAimInterpSpeed));
	if (FMath::IsNearlyEqual(PrecisionCrosshairAimAlpha, TargetAimAlpha, 0.001f))
	{
		PrecisionCrosshairAimAlpha = TargetAimAlpha;
	}
}
