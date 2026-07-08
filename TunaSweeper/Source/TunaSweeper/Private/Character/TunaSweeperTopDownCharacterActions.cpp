#include "TunaSweeperTopDownCharacterShared.h"

float ATunaSweeperTopDownCharacter::GetReloadProgress() const
{
	if (!bIsReloading || ReloadDurationSeconds <= 0.0f)
	{
		return 0.0f;
	}

	if (EquippedWeapon && EquippedWeapon->IsReloadRuntimeActive())
	{
		return EquippedWeapon->GetReloadRuntimeProgress();
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : ReloadStartWorldSeconds;
	return FMath::Clamp((CurrentTime - ReloadStartWorldSeconds) / ReloadDurationSeconds, 0.0f, 1.0f);
}

float ATunaSweeperTopDownCharacter::GetItemUseProgress() const
{
	if (!bIsUsingItem || ItemUseDurationSeconds <= 0.0f)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : ItemUseStartWorldSeconds;
	return FMath::Clamp((CurrentTime - ItemUseStartWorldSeconds) / ItemUseDurationSeconds, 0.0f, 1.0f);
}

float ATunaSweeperTopDownCharacter::GetCancelableActionProgress() const
{
	if (bIsUsingItem)
	{
		return GetItemUseProgress();
	}

	return GetReloadProgress();
}

void ATunaSweeperTopDownCharacter::GetAmmoSelectionItemIds(TArray<int32>& OutAmmoItemIds) const
{
	OutAmmoItemIds = AmmoSelectionItemIds;
}

void ATunaSweeperTopDownCharacter::StartReload()
{
	if (bIsReloading || !CanUseSelectedWeaponSlot())
	{
		return;
	}

	if (bIsUsingItem)
	{
		CancelItemUse();
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	const int32 MagazineCapacity = TunaGameInstance->GetWeaponMagazineCapacity(SelectedWeaponSlotNumber);
	const int32 LoadedAmmoCount = TunaGameInstance->GetWeaponLoadedAmmoCount(SelectedWeaponSlotNumber);
	if (MagazineCapacity <= 0 || LoadedAmmoCount >= MagazineCapacity)
	{
		return;
	}

	int32 ReloadAmmoItemId = TunaGameInstance->GetWeaponSelectedAmmoItemId(SelectedWeaponSlotNumber);
	if (LoadedAmmoCount > 0)
	{
		FTunaSweeperItemInstance WeaponInstance;
		FTunaSweeperItemDefinition WeaponDefinition;
		if (TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition) &&
			WeaponInstance.LoadedAmmoItemId != INDEX_NONE)
		{
			ReloadAmmoItemId = WeaponInstance.LoadedAmmoItemId;
		}
	}

	if (ReloadAmmoItemId == INDEX_NONE || TunaGameInstance->GetWeaponInventoryAmmoCount(SelectedWeaponSlotNumber) <= 0)
	{
		return;
	}

	PendingReloadAmmoItemId = ReloadAmmoItemId;
	ReloadDurationSeconds = FMath::Max(0.01f, TunaGameInstance->GetWeaponReloadSeconds(SelectedWeaponSlotNumber));
	ReloadStartWorldSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	bIsReloading = true;
	CloseAmmoSelection();
	EnsureEquippedWeaponActor();
	if (EquippedWeapon)
	{
		EquippedWeapon->StartReloadRuntime(ReloadDurationSeconds);
	}

	if (GetWorld())
	{
		GetWorldTimerManager().SetTimer(
			ReloadTimerHandle,
			this,
			&ATunaSweeperTopDownCharacter::CompleteReload,
			ReloadDurationSeconds,
			false);
	}
}

void ATunaSweeperTopDownCharacter::CompleteReload()
{
	const int32 ReloadSlotNumber = SelectedWeaponSlotNumber;
	const int32 ReloadAmmoItemId = PendingReloadAmmoItemId;
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->FinishReloadRuntime();
	}

	bIsReloading = false;
	PendingReloadAmmoItemId = INDEX_NONE;
	ReloadStartWorldSeconds = 0.0f;
	ReloadDurationSeconds = 0.0f;

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		int32 LoadedAmmoCount = 0;
		TunaGameInstance->TryReloadWeaponSlot(ReloadSlotNumber, ReloadAmmoItemId, LoadedAmmoCount);
	}
}

void ATunaSweeperTopDownCharacter::CancelReload()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(ReloadTimerHandle);
	}

	bIsReloading = false;
	PendingReloadAmmoItemId = INDEX_NONE;
	ReloadStartWorldSeconds = 0.0f;
	ReloadDurationSeconds = 0.0f;
	if (EquippedWeapon)
	{
		EquippedWeapon->CancelReloadRuntime();
	}
}

bool ATunaSweeperTopDownCharacter::StartItemUseFromSlot(const FTunaSweeperItemSlotReference& SlotReference)
{
	const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController());
	if (bIsDead ||
		bIsRolling ||
		!GetWorld() ||
		(TunaPlayerController && (TunaPlayerController->IsDialogueSequenceActive() || TunaPlayerController->IsHousingModeOpen())))
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance || !TunaGameInstance->CanUseItemInSlot(SlotReference, this))
	{
		return false;
	}

	FGuid ItemUid;
	if (!TunaGameInstance->TryGetSlotItemUid(SlotReference, ItemUid))
	{
		return false;
	}

	const float UseSeconds = TunaGameInstance->GetItemUseSecondsInSlot(SlotReference);
	if (UseSeconds <= 0.0f)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	CancelReload();
	if (bIsUsingItem)
	{
		CancelItemUse();
	}
	CloseAmmoSelection();
	CancelMeleeSwing();
	bFireHeld = false;
	GetWorldTimerManager().ClearTimer(FireTimerHandle);

	PendingItemUseSlotReference = SlotReference;
	PendingItemUseUid = ItemUid;
	ItemUseDurationSeconds = FMath::Max(0.01f, UseSeconds);
	ItemUseStartWorldSeconds = World->GetTimeSeconds();
	bIsUsingItem = true;

	GetWorldTimerManager().SetTimer(
		ItemUseTimerHandle,
		this,
		&ATunaSweeperTopDownCharacter::CompleteItemUse,
		ItemUseDurationSeconds,
		false);

	return true;
}

void ATunaSweeperTopDownCharacter::CompleteItemUse()
{
	const FTunaSweeperItemSlotReference ItemUseSlotReference = PendingItemUseSlotReference;
	const FGuid ItemUseUid = PendingItemUseUid;
	CancelItemUse();

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance || !ItemUseSlotReference.IsValid() || !ItemUseUid.IsValid())
	{
		return;
	}

	FGuid CurrentItemUid;
	if (!TunaGameInstance->TryGetSlotItemUid(ItemUseSlotReference, CurrentItemUid) || CurrentItemUid != ItemUseUid)
	{
		return;
	}

	TunaGameInstance->TryUseItemInSlot(ItemUseSlotReference, this);
}

void ATunaSweeperTopDownCharacter::CancelItemUse()
{
	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(ItemUseTimerHandle);
	}

	bIsUsingItem = false;
	PendingItemUseSlotReference = FTunaSweeperItemSlotReference();
	PendingItemUseUid.Invalidate();
	ItemUseStartWorldSeconds = 0.0f;
	ItemUseDurationSeconds = 0.0f;
}

void ATunaSweeperTopDownCharacter::OpenAmmoSelection()
{
	if (!CanUseSelectedWeaponSlot())
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	TunaGameInstance->GetCompatibleAmmoItemIdsForWeaponSlot(
		SelectedWeaponSlotNumber,
		AmmoSelectionItemIds,
		false);
	if (AmmoSelectionItemIds.Num() <= 0)
	{
		CloseAmmoSelection();
		return;
	}

	const int32 CurrentAmmoItemId = TunaGameInstance->GetWeaponSelectedAmmoItemId(SelectedWeaponSlotNumber);
	AmmoSelectionFocusIndex = AmmoSelectionItemIds.IndexOfByKey(CurrentAmmoItemId);
	if (!AmmoSelectionItemIds.IsValidIndex(AmmoSelectionFocusIndex))
	{
		AmmoSelectionFocusIndex = 0;
	}

	bAmmoSelectionOpen = true;
}

void ATunaSweeperTopDownCharacter::ConfirmAmmoSelection()
{
	if (!bAmmoSelectionOpen || !AmmoSelectionItemIds.IsValidIndex(AmmoSelectionFocusIndex))
	{
		CloseAmmoSelection();
		return;
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		const bool bWasAmmoUnspecified = TunaGameInstance->GetWeaponSelectedAmmoItemId(SelectedWeaponSlotNumber) == INDEX_NONE;
		const bool bAmmoSelected = TunaGameInstance->SetSelectedAmmoItemForWeaponSlot(
			SelectedWeaponSlotNumber,
			AmmoSelectionItemIds[AmmoSelectionFocusIndex]);
		const bool bShouldAutoReload =
			bWasAmmoUnspecified &&
			bAmmoSelected &&
			TunaGameInstance->GetWeaponInventoryAmmoCount(SelectedWeaponSlotNumber) > 0;

		CloseAmmoSelection();
		if (bShouldAutoReload)
		{
			StartReload();
		}
		return;
	}

	CloseAmmoSelection();
}

void ATunaSweeperTopDownCharacter::CloseAmmoSelection()
{
	bAmmoSelectionOpen = false;
	AmmoSelectionItemIds.Reset();
	AmmoSelectionFocusIndex = INDEX_NONE;
}

void ATunaSweeperTopDownCharacter::MoveAmmoSelectionFocus(int32 FocusDelta)
{
	if (!bAmmoSelectionOpen || AmmoSelectionItemIds.Num() <= 0 || FocusDelta == 0)
	{
		return;
	}

	if (!AmmoSelectionItemIds.IsValidIndex(AmmoSelectionFocusIndex))
	{
		AmmoSelectionFocusIndex = 0;
		return;
	}

	const int32 OptionCount = AmmoSelectionItemIds.Num();
	AmmoSelectionFocusIndex = (AmmoSelectionFocusIndex + FocusDelta) % OptionCount;
	if (AmmoSelectionFocusIndex < 0)
	{
		AmmoSelectionFocusIndex += OptionCount;
	}
}

bool ATunaSweeperTopDownCharacter::IsGameplayActionInputLocked() const
{
	const ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetController());
	return TunaPlayerController &&
		(TunaPlayerController->IsInventoryUiOpen() ||
			TunaPlayerController->IsDialogueSequenceActive() ||
			TunaPlayerController->IsHousingModeOpen());
}

bool ATunaSweeperTopDownCharacter::IsCarryWeightMovementBlocked() const
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	return TunaGameInstance && TunaGameInstance->IsCarryWeightMovementBlocked();
}

void ATunaSweeperTopDownCharacter::CancelActiveCancelableAction()
{
	CancelReload();
	CancelItemUse();
}

void ATunaSweeperTopDownCharacter::CancelActiveGameplayActions()
{
	FinishRoll();
	bFireHeld = false;
	bIsAiming = false;
	bSprintInputHeld = false;
	bIsSprinting = false;
	bSprintLockedUntilReleased = false;
	CurrentMoveInput = FVector2D::ZeroVector;
	CancelReload();
	CancelItemUse();
	CloseAmmoSelection();
	CancelMeleeSwing();

	if (GetWorld())
	{
		GetWorldTimerManager().ClearTimer(FireTimerHandle);
	}

	if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
	{
		MovementComponent->StopMovementImmediately();
	}
}
