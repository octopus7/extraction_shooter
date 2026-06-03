#include "Subsystem/TunaSweeperToastSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Stats/Stats.h"
#include "UI/TunaSweeperToastWidget.h"

void UTunaSweeperToastSubsystem::Deinitialize()
{
	ClearToasts();
	Super::Deinitialize();
}

void UTunaSweeperToastSubsystem::Tick(float DeltaTime)
{
	if (!bToastActive)
	{
		StartNextToast();
		return;
	}

	ActiveToastElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	if (ActiveToastElapsedSeconds >= ActiveToastDurationSeconds)
	{
		FinishActiveToast();
		StartNextToast();
		return;
	}

	UpdateActiveToastOpacity();
}

TStatId UTunaSweeperToastSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTunaSweeperToastSubsystem, STATGROUP_Tickables);
}

bool UTunaSweeperToastSubsystem::IsTickable() const
{
	return !HasAnyFlags(RF_ClassDefaultObject) && (bToastActive || ToastQueue.Num() > 0);
}

bool UTunaSweeperToastSubsystem::ShowToast(const FText& MessageText, float DurationSeconds)
{
	if (MessageText.IsEmpty())
	{
		return false;
	}

	FTunaSweeperToastRequest Request;
	Request.Message = MessageText;
	Request.DurationSeconds = FMath::Max(DurationSeconds, MinimumToastDurationSeconds);
	ToastQueue.Add(Request);

	if (!bToastActive)
	{
		StartNextToast();
	}

	return true;
}

bool UTunaSweeperToastSubsystem::ShowLocalizedToast(
	FName StringKey,
	const FText& FallbackText,
	float DurationSeconds)
{
	return ShowToast(ResolveToastText(StringKey, FallbackText), DurationSeconds);
}

bool UTunaSweeperToastSubsystem::ShowSaveSlotDeletedToast()
{
	return ShowLocalizedToast(
		FName(TEXT("ui.toast.save_slot_deleted")),
		FText::FromString(TEXT("\uC0AD\uC81C \uB418\uC5C8\uC2B5\uB2C8\uB2E4")),
		2.0f);
}

bool UTunaSweeperToastSubsystem::ShowQuestCompletedToast(const FText& QuestTitle)
{
	if (QuestTitle.IsEmpty())
	{
		return ShowLocalizedToast(
			FName(TEXT("ui.toast.quest_completed_generic")),
			FText::FromString(TEXT("\uD018\uC2A4\uD2B8 \uB2EC\uC131")),
			2.5f);
	}

	const FText Pattern = ResolveToastText(
		FName(TEXT("ui.toast.quest_completed")),
		FText::FromString(TEXT("{0} \uB2EC\uC131")));

	FFormatOrderedArguments Arguments;
	Arguments.Add(QuestTitle);
	return ShowToast(FText::Format(Pattern, Arguments), 2.5f);
}

void UTunaSweeperToastSubsystem::ClearToasts()
{
	ToastQueue.Reset();
	bToastActive = false;
	ActiveToastElapsedSeconds = 0.0f;
	ActiveToastDurationSeconds = 0.0f;

	if (ActiveToastWidget)
	{
		ActiveToastWidget->RemoveFromParent();
		ActiveToastWidget = nullptr;
	}
}

bool UTunaSweeperToastSubsystem::EnsureToastWidget()
{
	if (ActiveToastWidget && ActiveToastWidget->IsInViewport())
	{
		return true;
	}

	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}

	if (!ActiveToastWidget)
	{
		ActiveToastWidget = CreateWidget<UTunaSweeperToastWidget>(
			GameInstance,
			UTunaSweeperToastWidget::StaticClass());
	}

	if (!ActiveToastWidget)
	{
		return false;
	}

	if (!ActiveToastWidget->IsInViewport())
	{
		ActiveToastWidget->AddToViewport(ToastViewportZOrder);
	}
	ActiveToastWidget->SetVisibility(ESlateVisibility::HitTestInvisible);

	return true;
}

bool UTunaSweeperToastSubsystem::StartNextToast()
{
	if (bToastActive || ToastQueue.Num() <= 0)
	{
		return false;
	}

	if (!EnsureToastWidget())
	{
		return false;
	}

	const FTunaSweeperToastRequest Request = ToastQueue[0];
	ToastQueue.RemoveAt(0);

	ActiveToastElapsedSeconds = 0.0f;
	ActiveToastDurationSeconds = FMath::Max(Request.DurationSeconds, MinimumToastDurationSeconds);
	bToastActive = true;

	ActiveToastWidget->SetToastMessage(Request.Message);
	UpdateActiveToastOpacity();
	return true;
}

void UTunaSweeperToastSubsystem::FinishActiveToast()
{
	bToastActive = false;
	ActiveToastElapsedSeconds = 0.0f;
	ActiveToastDurationSeconds = 0.0f;

	if (ActiveToastWidget)
	{
		ActiveToastWidget->SetToastOpacity(0.0f);
	}
}

void UTunaSweeperToastSubsystem::UpdateActiveToastOpacity()
{
	if (!ActiveToastWidget || !bToastActive)
	{
		return;
	}

	const float RemainingSeconds = FMath::Max(0.0f, ActiveToastDurationSeconds - ActiveToastElapsedSeconds);
	float ToastAlpha = 1.0f;
	if (ToastFadeInSeconds > 0.0f && ActiveToastElapsedSeconds < ToastFadeInSeconds)
	{
		ToastAlpha = FMath::Min(ToastAlpha, ActiveToastElapsedSeconds / ToastFadeInSeconds);
	}
	if (ToastFadeOutSeconds > 0.0f && RemainingSeconds < ToastFadeOutSeconds)
	{
		ToastAlpha = FMath::Min(ToastAlpha, RemainingSeconds / ToastFadeOutSeconds);
	}

	ActiveToastWidget->SetToastOpacity(ToastAlpha);
}

FText UTunaSweeperToastSubsystem::ResolveToastText(FName StringKey, const FText& FallbackText) const
{
	if (StringKey.IsNone())
	{
		return FallbackText;
	}

	const UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return TunaGameInstance
		? TunaGameInstance->ResolveLocalizedText(StringKey, FallbackText)
		: FallbackText;
}
