#include "TunaSweeperIntroMenuWidgetShared.h"

void UTunaSweeperIntroMenuWidget::ExecuteSelectedSaveSlotDelete()
{
	const int32 DeletedSaveSlotIndex = SelectedSaveSlotIndex;
	if (!CanDeleteSelectedSaveSlot())
	{
		ResetDeleteHoldProgress();
		return;
	}

	bool bDeleted = false;
	if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
	{
		bDeleted = TunaGameInstance->DeleteSaveSlot(DeletedSaveSlotIndex);
	}

	HideDeleteConfirmDialog();
	ResetDeleteHoldProgress();
	RefreshSaveSlotMenu();
	RefreshMainMenu();

	if (bDeleted)
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTunaSweeperToastSubsystem* ToastSubsystem = GameInstance->GetSubsystem<UTunaSweeperToastSubsystem>())
			{
				ToastSubsystem->ShowSaveSlotDeletedToast();
			}
		}
	}
}

void UTunaSweeperIntroMenuWidget::ResetDeleteHoldProgress()
{
	bDeleteHoldActive = false;
	DeleteHoldElapsedSeconds = 0.0f;
	SetDeleteHoldProgress(0.0f);
}

void UTunaSweeperIntroMenuWidget::SetDeleteHoldProgress(float Progress)
{
	EnsureDeleteSaveSlotHoldProgressWidget();
	HideLegacyDeleteHoldGaugeWidgets();

	const float ClampedProgress = FMath::Clamp(Progress, 0.0f, 1.0f);
	if (!DeleteSaveSlotHoldProgressFill)
	{
		return;
	}

	DeleteSaveSlotHoldProgressFill->SetVisibility(ClampedProgress > 0.0f
		? ESlateVisibility::HitTestInvisible
		: ESlateVisibility::Collapsed);
	DeleteSaveSlotHoldProgressFill->SetRenderOpacity(ClampedProgress > 0.0f ? 1.0f : 0.0f);
	DeleteSaveSlotHoldProgressFill->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
	DeleteSaveSlotHoldProgressFill->SetRenderScale(FVector2D(ClampedProgress, 1.0f));
}

void UTunaSweeperIntroMenuWidget::ShowDeleteConfirmDialog()
{
	bDeleteConfirmVisible = true;
	SetDeleteHoldProgress(1.0f);

	if (DeleteConfirmPanel)
	{
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Visible);
	}
}

void UTunaSweeperIntroMenuWidget::HideDeleteConfirmDialog()
{
	bDeleteConfirmVisible = false;

	if (DeleteConfirmPanel)
	{
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperIntroMenuWidget::HideLegacyDeleteHoldGaugeWidgets()
{
	if (DeleteHoldGaugeFill)
	{
		DeleteHoldGaugeFill->SetVisibility(ESlateVisibility::Collapsed);
		DeleteHoldGaugeFill->SetRenderOpacity(0.0f);
		DeleteHoldGaugeFill->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		DeleteHoldGaugeFill->SetRenderScale(FVector2D::ZeroVector);
	}

	if (!WidgetTree)
	{
		return;
	}

	static const FName LegacyGaugeWidgetNames[] = {
		FName(TEXT("DeleteHoldGaugeBox")),
		FName(TEXT("DeleteHoldGaugeOverlay")),
		FName(TEXT("DeleteHoldGaugeRing")),
		FName(TEXT("DeleteHoldGaugeFill")),
	};

	for (const FName& WidgetName : LegacyGaugeWidgetNames)
	{
		if (UWidget* GaugeWidget = FindIntroWidget(WidgetName))
		{
			GaugeWidget->SetVisibility(ESlateVisibility::Collapsed);
			GaugeWidget->SetRenderOpacity(0.0f);
		}
	}
}

void UTunaSweeperIntroMenuWidget::EnsureDeleteSaveSlotHoldProgressWidget()
{
    if (!DeleteSaveSlotHoldProgressFill)
    {
        DeleteSaveSlotHoldProgressFill = Cast<UImage>(FindIntroWidget(TEXT("DeleteSaveSlotHoldProgressFill")));
    }
    ConfigureDeleteSaveSlotHoldProgressFill();
}

void UTunaSweeperIntroMenuWidget::ConfigureDeleteSaveSlotHoldProgressFill()
{
	if (!DeleteSaveSlotHoldProgressFill)
	{
		return;
	}

	FSlateBrush ProgressFillBrush;
	ProgressFillBrush.DrawAs = ESlateBrushDrawType::Box;
	ProgressFillBrush.TintColor = FSlateColor(FLinearColor(0.86f, 0.26f, 0.18f, 0.50f));
	DeleteSaveSlotHoldProgressFill->SetBrush(ProgressFillBrush);
	DeleteSaveSlotHoldProgressFill->SetVisibility(ESlateVisibility::Collapsed);
	DeleteSaveSlotHoldProgressFill->SetRenderOpacity(0.0f);
	DeleteSaveSlotHoldProgressFill->SetRenderTransformPivot(FVector2D(0.0f, 0.5f));
	DeleteSaveSlotHoldProgressFill->SetRenderScale(FVector2D(0.0f, 1.0f));
}

