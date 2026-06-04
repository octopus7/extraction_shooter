#include "Component/TunaSweeperHeadphoneListenerComponent.h"

#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperNoiseSubsystem.h"
#include "UI/TunaSweeperGameHudWidget.h"

namespace
{
	const FName EarCategoryTag(TEXT("item.category.ear"));
	const FName EarEquipmentSlotTag(TEXT("equipment.slot.ear"));
}

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperHeadphoneNoise, Log, All);

UTunaSweeperHeadphoneListenerComponent::UTunaSweeperHeadphoneListenerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTunaSweeperHeadphoneListenerComponent::BeginPlay()
{
	Super::BeginPlay();

	BindDelegates();
	RefreshEquippedHeadphone();
}

void UTunaSweeperHeadphoneListenerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindDelegates();
	Super::EndPlay(EndPlayReason);
}

void UTunaSweeperHeadphoneListenerComponent::BindDelegates()
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
		{
			NoiseSubsystem->OnNoiseReported.RemoveAll(this);
			NoiseSubsystem->OnNoiseReported.AddUObject(this, &UTunaSweeperHeadphoneListenerComponent::HandleNoiseReported);
		}
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetWorld()
		? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>()
		: nullptr)
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperHeadphoneListenerComponent::RefreshEquippedHeadphone);
	}
}

void UTunaSweeperHeadphoneListenerComponent::UnbindDelegates()
{
	UWorld* World = GetWorld();
	if (World)
	{
		if (UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>())
		{
			NoiseSubsystem->OnNoiseReported.RemoveAll(this);
		}
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetWorld()
		? GetWorld()->GetGameInstance<UTunaSweeperGameInstance>()
		: nullptr)
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
	}
}

void UTunaSweeperHeadphoneListenerComponent::RefreshEquippedHeadphone()
{
	bHeadphoneEquipped = false;
	CachedHearingRange = 0.0f;
	CachedSensitivity = 0.0f;
	CachedMinStrength = 0.0f;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = World->GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	if (!TunaGameInstance || !ItemDataSubsystem)
	{
		return;
	}

	const TArray<FTunaSweeperInventorySlot>& EquipmentSlots = TunaGameInstance->GetEquipmentSlots();
	for (const FTunaSweeperInventorySlot& EquipmentSlot : EquipmentSlots)
	{
		FTunaSweeperItemInstance ItemInstance;
		if (!TunaGameInstance->TryGetItemInstance(EquipmentSlot.ItemUid, ItemInstance))
		{
			continue;
		}

		FTunaSweeperItemDefinition ItemDefinition;
		if (!ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition))
		{
			continue;
		}

		if (ItemDefinition.CategoryTag != EarCategoryTag && ItemDefinition.EquipmentSlotTag != EarEquipmentSlotTag)
		{
			continue;
		}

		if (ItemDefinition.HeadphoneHearingRange <= 0.0f && ItemDefinition.HeadphoneSensitivity <= 0.0f)
		{
			continue;
		}

		bHeadphoneEquipped = true;
		CachedHearingRange = ItemDefinition.HeadphoneHearingRange > 0.0f
			? ItemDefinition.HeadphoneHearingRange
			: DefaultHearingRange;
		CachedSensitivity = ItemDefinition.HeadphoneSensitivity > 0.0f
			? ItemDefinition.HeadphoneSensitivity
			: DefaultSensitivity;
		CachedMinStrength = ItemDefinition.HeadphoneMinStrength > 0.0f
			? ItemDefinition.HeadphoneMinStrength
			: DefaultMinStrength;
		if (bEnableNoiseGateDebugLog)
		{
			UE_LOG(
				LogTunaSweeperHeadphoneNoise,
				Log,
				TEXT("Headphone equipped: owner=%s itemId=%d range=%.1fcm sensitivity=%.2f minStrength=%.3f"),
				*GetNameSafe(GetOwner()),
				ItemInstance.ItemId,
				CachedHearingRange,
				CachedSensitivity,
				CachedMinStrength);
		}
		return;
	}

	if (bEnableNoiseGateDebugLog)
	{
		UE_LOG(
			LogTunaSweeperHeadphoneNoise,
			Log,
			TEXT("Headphone not equipped after refresh: owner=%s equipmentSlots=%d"),
			*GetNameSafe(GetOwner()),
			EquipmentSlots.Num());
	}
}

bool UTunaSweeperHeadphoneListenerComponent::IsListenerPawnReady() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsPlayerControlled() && !OwnerPawn->IsHidden();
}

bool UTunaSweeperHeadphoneListenerComponent::ShouldIgnoreNoiseSource(const FTunaSweeperNoiseEvent& NoiseEvent) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return true;
	}

	if (NoiseEvent.SourceActor == OwnerActor || NoiseEvent.InstigatorActor == OwnerActor)
	{
		return true;
	}

	if (NoiseEvent.SourceActor && NoiseEvent.SourceActor->GetOwner() == OwnerActor)
	{
		return true;
	}

	return false;
}

void UTunaSweeperHeadphoneListenerComponent::HandleNoiseReported(const FTunaSweeperNoiseEvent& NoiseEvent)
{
	if (!bHeadphoneEquipped)
	{
		RefreshEquippedHeadphone();
	}

	if (!bHeadphoneEquipped)
	{
		LogNoiseGateDebug(TEXT("no_headphone_equipped"), NoiseEvent);
		return;
	}

	if (!IsListenerPawnReady())
	{
		LogNoiseGateDebug(TEXT("listener_not_ready"), NoiseEvent);
		return;
	}

	if (ShouldIgnoreNoiseSource(NoiseEvent))
	{
		LogNoiseGateDebug(TEXT("ignored_source"), NoiseEvent);
		return;
	}

	UWorld* World = GetWorld();
	AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor)
	{
		LogNoiseGateDebug(TEXT("missing_world_or_owner"), NoiseEvent);
		return;
	}

	const FVector ListenerLocation = OwnerActor->GetActorLocation();
	const float SourceDistance = FVector::Dist2D(ListenerLocation, NoiseEvent.SourceLocation);
	if (SourceDistance <= SelfNoiseIgnoreDistance || SourceDistance < MinVisualNoiseDistance)
	{
		LogNoiseGateDebug(TEXT("too_close"), NoiseEvent, SourceDistance);
		return;
	}

	UTunaSweeperNoiseSubsystem* NoiseSubsystem = World->GetSubsystem<UTunaSweeperNoiseSubsystem>();
	if (!NoiseSubsystem)
	{
		LogNoiseGateDebug(TEXT("missing_noise_subsystem"), NoiseEvent, SourceDistance);
		return;
	}

	FTunaSweeperHeardNoiseEvent HeardNoise;
	if (!NoiseSubsystem->CalculateHeardNoiseAtLocation(
		NoiseEvent,
		ListenerLocation,
		CachedHearingRange,
		CachedSensitivity,
		CachedMinStrength,
		HeardNoise))
	{
		LogNoiseGateDebug(TEXT("attenuation_rejected"), NoiseEvent, SourceDistance);
		return;
	}

	const float CurrentTimeSeconds = World->GetTimeSeconds();
	if (CurrentTimeSeconds - LastRippleSpawnTimeSeconds < RippleCooldownSeconds)
	{
		LogNoiseGateDebug(TEXT("cooldown"), NoiseEvent, SourceDistance, HeardNoise.Strength);
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(OwnerActor);
	ATunaSweeperPlayerController* TunaPlayerController = OwnerPawn
		? Cast<ATunaSweeperPlayerController>(OwnerPawn->GetController())
		: nullptr;
	if (!TunaPlayerController)
	{
		LogNoiseGateDebug(TEXT("missing_player_controller"), NoiseEvent, SourceDistance, HeardNoise.Strength);
		return;
	}

	UTunaSweeperGameHudWidget* GameHudWidget = TunaPlayerController->GetGameHudWidget();
	if (!GameHudWidget)
	{
		LogNoiseGateDebug(TEXT("missing_game_hud_widget"), NoiseEvent, SourceDistance, HeardNoise.Strength);
		return;
	}

	LastRippleSpawnTimeSeconds = CurrentTimeSeconds;
	LogNoiseGateDebug(TEXT("queued_hud_ripple"), NoiseEvent, SourceDistance, HeardNoise.Strength);
	GameHudWidget->AddHeadphoneNoiseRipple(HeardNoise.DirectionFromListener, HeardNoise.Strength);
}

void UTunaSweeperHeadphoneListenerComponent::LogNoiseGateDebug(
	const TCHAR* GateName,
	const FTunaSweeperNoiseEvent& NoiseEvent,
	float SourceDistance,
	float HeardStrength) const
{
	if (!bEnableNoiseGateDebugLog)
	{
		return;
	}

	UE_LOG(
		LogTunaSweeperHeadphoneNoise,
		Log,
		TEXT("Headphone noise %s: owner=%s source=%s instigator=%s tag=%s distance=%.1fcm hearingRange=%.1fcm sensitivity=%.2f minStrength=%.3f loudness=%.2f noiseRange=%.1fcm heardStrength=%.3f equipped=%s"),
		GateName ? GateName : TEXT("unknown"),
		*GetNameSafe(GetOwner()),
		*GetNameSafe(NoiseEvent.SourceActor.Get()),
		*GetNameSafe(NoiseEvent.InstigatorActor.Get()),
		*NoiseEvent.NoiseTag.ToString(),
		SourceDistance,
		CachedHearingRange,
		CachedSensitivity,
		CachedMinStrength,
		NoiseEvent.Loudness,
		NoiseEvent.MaxRange,
		HeardStrength,
		bHeadphoneEquipped ? TEXT("true") : TEXT("false"));
}
