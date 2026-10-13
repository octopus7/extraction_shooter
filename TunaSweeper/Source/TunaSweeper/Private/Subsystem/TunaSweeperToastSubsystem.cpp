#include "Subsystem/TunaSweeperToastSubsystem.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "Engine/Texture2D.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Stats/Stats.h"
#include "UI/TunaSweeperToastWidget.h"

void UTunaSweeperToastSubsystem::Deinitialize()
{
	FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
	ClearToasts();
	Super::Deinitialize();
}

void UTunaSweeperToastSubsystem::Tick(float DeltaTime)
{
	TickRewardToasts(DeltaTime);
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
	return !HasAnyFlags(RF_ClassDefaultObject) && (bToastActive || ToastQueue.Num() > 0 || bRewardActive || RewardQueue.Num() > 0);
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

bool UTunaSweeperToastSubsystem::ShowQuestAcceptedToast(const FText& QuestTitle)
{
	if (QuestTitle.IsEmpty())
	{
		return ShowLocalizedToast(TEXT("ui.toast.quest_accepted_generic"),
			FText::FromString(TEXT("\uD018\uC2A4\uD2B8 \uC218\uB77D")), 2.5f);
	}
	const FText Pattern = ResolveToastText(TEXT("ui.toast.quest_accepted"),
		FText::FromString(TEXT("\uD018\uC2A4\uD2B8 \uC218\uB77D: {0}")));
	return ShowToast(FText::Format(Pattern, QuestTitle), 2.5f);
}

void UTunaSweeperToastSubsystem::ClearToasts()
{
	ClearRewardToasts();
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

void UTunaSweeperToastSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::HandleWorldCleanup);
}

void UTunaSweeperToastSubsystem::HandleWorldCleanup(UWorld* World, bool, bool)
{
    if (World && World->IsGameWorld() && World->GetGameInstance() == GetGameInstance()) ClearRewardToasts();
}

bool UTunaSweeperToastSubsystem::ShowItemRewardToast(int32 ItemId, int32 Quantity)
{
    FTunaSweeperItemDefinition Definition;
    auto* Game = GetGameInstance();
    auto* Data = Game ? Game->GetSubsystem<UTunaSweeperItemDataSubsystem>() : nullptr;
    if (Quantity <= 0 || !Data || !Data->TryGetItemDefinition(ItemId, Definition)) return false;
    RewardQueue.Add({ItemId, Quantity});
    if (!bRewardActive) StartNextRewardToast();
    return true;
}

bool UTunaSweeperToastSubsystem::StartNextRewardToast()
{
    if (bRewardActive || RewardQueue.IsEmpty()) return false;
    auto* Game = Cast<UTunaSweeperGameInstance>(GetGameInstance());
    if (!Game || !Game->GetWorld()) return false;
    if (!RewardToastWidget) RewardToastWidget = CreateWidget<UTunaSweeperToastWidget>(Game, UTunaSweeperToastWidget::StaticClass());
    if (!RewardToastWidget) return false;
    if (!RewardToastWidget->IsInViewport()) RewardToastWidget->AddToViewport(ToastViewportZOrder + 1);
    RewardToastWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
    ActiveReward = RewardQueue[0];
    RewardQueue.RemoveAt(0);
    auto* Data = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
    FText Name;
    FTunaSweeperItemDefinition Definition;
    Data->TryGetItemNameText(ActiveReward.ItemId, Game->GetCurrentTextLanguage(), Name);
    Data->TryGetItemDefinition(ActiveReward.ItemId, Definition);
    RewardToastWidget->SetRewardPresentation(
        FText::Format(Game->ResolveLocalizedText(TEXT("ui.toast.item_reward"), FText::GetEmpty()), Name, FText::AsNumber(ActiveReward.Quantity)),
        LoadObject<UTexture2D>(nullptr, *Data->BuildItemIconObjectPath(Definition)));
    RewardToastWidget->SetToastOpacity(1.0f);
    RewardElapsed = 0.0f;
    bRewardActive = true;
    return true;
}

void UTunaSweeperToastSubsystem::TickRewardToasts(float DeltaTime)
{
    if (!bRewardActive) { StartNextRewardToast(); return; }
    RewardElapsed += FMath::Max(0.0f, DeltaTime);
    if (RewardElapsed < 1.0f) return;
    bRewardActive = false;
    ActiveReward = {};
    if (RewardToastWidget) RewardToastWidget->SetToastOpacity(0.0f);
    // Each queued item receives its own full second, even after a long frame.
    StartNextRewardToast();
}

void UTunaSweeperToastSubsystem::ClearRewardToasts()
{
    RewardQueue.Reset(); ActiveReward = {}; RewardElapsed = 0.0f; bRewardActive = false;
    if (RewardToastWidget) { RewardToastWidget->RemoveFromParent(); RewardToastWidget = nullptr; }
}
