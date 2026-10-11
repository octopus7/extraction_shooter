#include "Subsystem/TunaSweeperAchievementSubsystem.h"

#include "Achievement/TunaSweeperAchievementModel.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"

bool UTunaSweeperAchievementSubsystem::IsGameplayReportingAllowed() const
{
	const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	return Instance && !Instance->IsCombatTestSession() && GetWorld() && GetWorld()->IsGameWorld();
}

void UTunaSweeperAchievementSubsystem::ReportGameplayEvent(ETunaSweeperAchievementEvent Event, FName TargetId)
{
	if (!IsGameplayReportingAllowed()) return;
	ProcessProgressChanged(TunaSweeperAchievementModel::RecordGameplayEvent(ProgressState, Event, TargetId));
}

void UTunaSweeperAchievementSubsystem::ReportWeaponFired(int32 WeaponItemId)
{
	if (WeaponItemId > 0)
		ReportGameplayEvent(ETunaSweeperAchievementEvent::WeaponFired, FName(*FString::FromInt(WeaponItemId)));
}

FText UTunaSweeperAchievementSubsystem::GetAchievementName(FName AchievementId) const
{
	const auto* Definition = FindDefinition(AchievementId);
	const auto* Instance = Cast<UTunaSweeperGameInstance>(GetGameInstance());
	const auto* Text = Instance ? Instance->GetSubsystem<UTunaSweeperTextSubsystem>() : nullptr;
	FText Result;
	if (Definition && Text)
		Text->TryGetTextByKey(Definition->NameStringKey, Instance->GetCurrentTextLanguage(), Result);
	return Result;
}

void UTunaSweeperAchievementSubsystem::RefreshGameplaySnapshot()
{
	if (bRefreshingSnapshot || !IsGameplayReportingAllowed()) return;
	TGuardValue<bool> Guard(bRefreshingSnapshot, true);
	auto* Instance = CastChecked<UTunaSweeperGameInstance>(GetGameInstance());
	GameplaySnapshot.OwnedItemCounts.Reset();
	GameplaySnapshot.RestoredFacilityIds.Reset();
	TSet<FGuid> Counted;
	const auto CountSlots = [&](const TArray<FTunaSweeperInventorySlot>& Slots)
	{
		for (const auto& Slot : Slots)
		{
			if (!Slot.ItemUid.IsValid() || Counted.Contains(Slot.ItemUid)) continue;
			Counted.Add(Slot.ItemUid);
			FTunaSweeperItemInstance Item;
			if (Instance->TryGetItemInstance(Slot.ItemUid, Item) && Item.ItemId > 0 && Item.Quantity > 0)
				GameplaySnapshot.OwnedItemCounts.FindOrAdd(Item.ItemId) += Item.Quantity;
		}
	};
	// Quick slots may reference the same item; UID deduplication prevents double counting.
	CountSlots(Instance->GetInventorySlots());
	CountSlots(Instance->GetEquipmentSlots());
	CountSlots(Instance->GetAuxiliaryBagSlots());
	CountSlots(Instance->GetUsableQuickSlots());
	CountSlots(Instance->GetStorageSlots());
	for (const auto& Definition : Definitions)
	{
		if (!TunaSweeperAchievementModel::CanUnlock(Definition)) continue;
		for (FName Id : Definition.RequiredFacilityIds)
		{
			FTunaSweeperWorldProgressSaveData State;
			if (Instance->TryGetWorldProgressState(Id, State) && State.State == ETunaSweeperWorldProgressState::Completed)
				GameplaySnapshot.RestoredFacilityIds.Add(Id);
		}
	}
	TArray<FName> Unlocked;
	TunaSweeperAchievementModel::EvaluateDefinitions(Definitions, ProgressState, Unlocked, GameplaySnapshot);
	PendingNotifications.Append(Unlocked);
	bProgressDirty |= !Unlocked.IsEmpty();
	FlushPendingProgress();
	if (!Unlocked.IsEmpty() && !bProgressDirty) TryStartPlatformSync();
}

void UTunaSweeperAchievementSubsystem::HandleGameplayWorldReady(UWorld* World)
{
	if (!World || World->GetGameInstance() != GetGameInstance() || !World->IsGameWorld() || GameplayWorld == World) return;
	GameplayWorld = World;
	const FName Level(*UGameplayStatics::GetCurrentLevelName(World, true));
	if (!IsGameplayReportingAllowed()) { CancelRaid(); return; }

	// Capture was made only after the extraction hold completed. A request, failed
	// load, death, or arrival at a different destination cannot commit the unlock.
	if (!PendingExtractionDestination.IsNone() && Level == PendingExtractionDestination)
	{
		GameplaySnapshot.bSuccessfulExtraction = true;
		GameplaySnapshot.RaidKills = PendingExtraction.RaidKills;
		GameplaySnapshot.RaidSeconds = PendingExtraction.RaidSeconds;
		GameplaySnapshot.Health = PendingExtraction.Health;
		GameplaySnapshot.MaxHealth = PendingExtraction.MaxHealth;
		ProcessProgressChanged(true);
		GameplaySnapshot.bSuccessfulExtraction = false;
	}
	const bool bRaidWorld = TunaSweeperBuildFlavor::IsRaidGameplayLevelName(Level);
	const bool bContinueRaid = bRaidWorld && RaidState.bActive && Level == PendingRegionDestination;
	const double PreviousSeconds = PendingRegionRaidSeconds;
	if (!bContinueRaid) CancelRaid();
	PendingRegionDestination = NAME_None;
	PendingRegionRaidSeconds = 0;
	FString PackageName = World->GetOutermost()->GetName();
	if (!World->StreamingLevelsPrefix.IsEmpty()) PackageName.ReplaceInline(*World->StreamingLevelsPrefix, TEXT(""));
	const FName PackagePath(*PackageName);
	for (const auto& Definition : Definitions)
	{
		if (!TunaSweeperAchievementModel::CanUnlock(Definition) || !Definition.MapPaths.Contains(PackagePath)) continue;
		if (Definition.ConditionType == ETunaSweeperAchievementConditionType::LocationReached)
			ReportLocationReached(Definition.TargetId);
	}
	if (bContinueRaid) RaidState.StartedAt = World->GetTimeSeconds() - PreviousSeconds;
	else if (bRaidWorld) RaidState.Begin(World->GetTimeSeconds());
	RefreshGameplaySnapshot();
}

void UTunaSweeperAchievementSubsystem::CaptureCompletedExtraction(APawn* Player, FName Destination)
{
	if (!IsGameplayReportingAllowed() || !RaidState.bActive || !Player || !Player->IsLocallyControlled() ||
		Player->GetWorld() != GetWorld() || Destination.IsNone()) return;
	const auto* Vitals = Player->FindComponentByClass<UTunaSweeperVitalsComponent>();
	if (!Vitals) return;
	const auto& State = Vitals->GetVitalsState();
	if (!FMath::IsFinite(State.Health) || !FMath::IsFinite(State.MaxHealth) || State.Health <= 0 || State.MaxHealth <= 0) return;
	PendingExtraction = {};
	PendingExtraction.RaidKills = RaidState.Kills;
	PendingExtraction.RaidSeconds = GetWorld()->GetTimeSeconds() - RaidState.StartedAt;
	PendingExtraction.Health = State.Health;
	PendingExtraction.MaxHealth = State.MaxHealth;
	PendingExtractionDestination = FName(*FPackageName::GetShortName(Destination.ToString()));
}

void UTunaSweeperAchievementSubsystem::DiscardPendingExtraction(UWorld* World)
{
	if (World && World->GetGameInstance() != GetGameInstance()) return;
	PendingExtraction = {};
	PendingExtractionDestination = NAME_None;
}

void UTunaSweeperAchievementSubsystem::CancelRaid()
{
	PendingRegionDestination = NAME_None;
	PendingRegionRaidSeconds = 0;
	RaidState.Reset();
	PendingExtraction = {};
	PendingExtractionDestination = NAME_None;
}

void UTunaSweeperAchievementSubsystem::PrepareRegionTravel(UWorld* World, FName Destination)
{
	PendingRegionDestination = NAME_None;
	PendingRegionRaidSeconds = 0;
	if (World && World == GetWorld() && RaidState.bActive &&
		TunaSweeperBuildFlavor::IsRaidGameplayLevelName(FName(*UGameplayStatics::GetCurrentLevelName(World, true))) &&
		TunaSweeperBuildFlavor::IsRaidGameplayLevelName(Destination))
	{
		PendingRegionDestination = FName(*FPackageName::GetShortName(Destination.ToString()));
		PendingRegionRaidSeconds = FMath::Max(0.0, World->GetTimeSeconds() - RaidState.StartedAt);
	}
}
