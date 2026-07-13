#include "TunaSweeperTopDownCharacterShared.h"

void ATunaSweeperTopDownCharacter::EnsureEquippedWeaponActor()
{
	if (EquippedWeapon || !GetWorld())
	{
		return;
	}

	TSubclassOf<ATunaSweeperWeapon> LoadedWeaponClass = ResolveEquippedWeaponClass();
	if (!LoadedWeaponClass)
	{
		LoadedWeaponClass = ATunaSweeperWeapon::StaticClass();
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.Instigator = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	EquippedWeapon = GetWorld()->SpawnActor<ATunaSweeperWeapon>(LoadedWeaponClass, GetActorTransform(), SpawnParameters);
	if (EquippedWeapon && WeaponAttachPoint)
	{
		EquippedWeapon->AttachToComponent(WeaponAttachPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		if (bMeleeWeaponSelected)
		{
			EquippedWeapon->ConfigureMeleeVisual();
			ApplyEquippedMeleeWeaponVisual();
		}
		else
		{
			if (EquippedWeapon->GetClass() == ATunaSweeperWeapon::StaticClass())
			{
				EquippedWeapon->ConfigureGunVisual();
			}
		}
		EquippedWeapon->SetActorHiddenInGame(bHousingModeVisualHidden);
		ApplyEquippedWeaponAttachmentVisuals();
	}
}

TSubclassOf<ATunaSweeperWeapon> ATunaSweeperTopDownCharacter::ResolveEquippedWeaponClass() const
{
	if (bMeleeWeaponSelected)
	{
		return ATunaSweeperWeapon::StaticClass();
	}

	TSoftClassPtr<ATunaSweeperWeapon> WeaponClassToLoad = DefaultWeaponClass;

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
		TunaGameInstance &&
		SelectedWeaponSlotNumber > 0 &&
		TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition) &&
		WeaponDefinition.CategoryTag == TunaSweeperEquippedWeaponVisual::GunCategoryTag)
	{
		WeaponClassToLoad = TSoftClassPtr<ATunaSweeperWeapon>(TunaSweeperEquippedWeaponVisual::AssaultRifleClassPath);
	}

	if (TSubclassOf<ATunaSweeperWeapon> LoadedWeaponClass = WeaponClassToLoad.LoadSynchronous())
	{
		return LoadedWeaponClass;
	}

	return DefaultWeaponClass.LoadSynchronous();
}

void ATunaSweeperTopDownCharacter::ApplyEquippedWeaponAttachmentVisuals()
{
	UpdateEquippedWeaponLaserSightBeam();
}

void ATunaSweeperTopDownCharacter::UpdateEquippedWeaponLaserSightBeam()
{
	if (!EquippedWeapon)
	{
		return;
	}

	const bool bShouldEnableLaserSight = !bHousingModeVisualHidden && IsSelectedWeaponLaserSightEquipped();
	if (EquippedWeapon->IsLaserSightEnabled() != bShouldEnableLaserSight)
	{
		EquippedWeapon->SetLaserSightEnabled(bShouldEnableLaserSight);
	}

	if (bShouldEnableLaserSight)
	{
		EquippedWeapon->UpdateLaserSightBeam(AimDirection, AimWorldPoint, bHasAimWorldPoint);
	}
}

bool ATunaSweeperTopDownCharacter::IsSelectedWeaponLaserSightEquipped() const
{
	if (bMeleeWeaponSelected || SelectedWeaponSlotNumber <= 0)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return false;
	}

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition) ||
		WeaponDefinition.WeaponTypeTag != TunaSweeperEquippedWeaponVisual::RifleWeaponTypeTag)
	{
		return false;
	}

	const FGuid* AttachmentUid = WeaponInstance.AttachmentSlots.Find(
		TunaSweeperEquippedWeaponVisual::TacticalAttachmentSlotTag);
	if (!AttachmentUid || !AttachmentUid->IsValid())
	{
		return false;
	}

	FTunaSweeperItemInstance AttachmentInstance;
	FTunaSweeperItemDefinition AttachmentDefinition;
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	return ItemDataSubsystem &&
		TunaGameInstance->TryGetItemInstance(*AttachmentUid, AttachmentInstance) &&
		ItemDataSubsystem->TryGetItemDefinition(AttachmentInstance.ItemId, AttachmentDefinition) &&
		AttachmentDefinition.Id == TunaSweeperEquippedWeaponVisual::LaserSightItemId &&
		AttachmentDefinition.AttachmentSlotTag == TunaSweeperEquippedWeaponVisual::TacticalAttachmentSlotTag;
}

void ATunaSweeperTopDownCharacter::ClearEquippedWeaponActor()
{
	CancelMeleeSwing();

	if (EquippedWeapon)
	{
		EquippedWeapon->Destroy();
		EquippedWeapon = nullptr;
	}

	bWeaponAttachedForRoll = false;
	SavedWeaponAttachParent.Reset();
	SavedWeaponAttachSocketName = NAME_None;
	SavedWeaponRelativeTransform = FTransform::Identity;
}

void ATunaSweeperTopDownCharacter::FireWeapon()
{
	if (IsGameplayActionInputLocked())
	{
		CancelActiveGameplayActions();
		return;
	}

	if (bIsDead || !CanUseSelectedWeaponSlot())
	{
		if (bMeleeWeaponSelected)
		{
			StartMeleeAttack();
		}
		return;
	}

	if (bIsReloading)
	{
		CancelReload();
	}
	if (bIsUsingItem)
	{
		CancelItemUse();
	}

	EnsureEquippedWeaponActor();
	if (!EquippedWeapon)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	FName ProjectileHitEffectId = NAME_None;
	FName ImpactProfileId = NAME_None;
	FName WeaponTypeTag = NAME_None;
	float ProjectileDamageMultiplier = 1.0f;
	int32 ProjectileDamageBonus = 0;
	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition))
	{
		WeaponTypeTag = WeaponDefinition.WeaponTypeTag;
		const int32 LoadedAmmoItemId = WeaponInstance.LoadedAmmoItemId != INDEX_NONE
			? WeaponInstance.LoadedAmmoItemId
			: WeaponInstance.SelectedAmmoItemId;
		if (LoadedAmmoItemId != INDEX_NONE)
		{
			if (UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>())
			{
				FTunaSweeperItemDefinition AmmoDefinition;
				if (ItemDataSubsystem->TryGetItemDefinition(LoadedAmmoItemId, AmmoDefinition))
				{
					ImpactProfileId = AmmoDefinition.ImpactProfileId;
					ProjectileHitEffectId = AmmoDefinition.ProjectileHitEffectId;
					ProjectileDamageMultiplier =
						TunaSweeperDataValues::ToRatioFloat(AmmoDefinition.ProjectileDamageMultiplier);
					ProjectileDamageBonus = AmmoDefinition.ProjectileDamageBonus;
				}
			}
		}
	}

	const UWorld* World = GetWorld();
	const bool bIsBunkerMap = World &&
		World->GetMapName().EndsWith(TEXT("BunkerMap"));
	if (TunaGameInstance->GetWeaponLoadedAmmoCount(SelectedWeaponSlotNumber) <= 0)
	{
		return;
	}

	const float SpreadHalfAngleDegrees = ResolveWeaponSpreadHalfAngleDegrees(WeaponTypeTag);
	const bool bFired = EquippedWeapon->FireWithAimIntent(
		AimDirection,
		this,
		ImpactProfileId,
		ProjectileHitEffectId,
		WeaponTypeTag,
		ProjectileDamageMultiplier,
		ProjectileDamageBonus,
		SpreadHalfAngleDegrees,
		AimWorldPoint,
		bHasAimWorldPoint,
		bHasAimIntent ? AimIntentActor.Get() : nullptr,
		bHasAimIntent ? AimIntentComponent.Get() : nullptr,
		AimIntentWorldPoint,
		bHasAimIntent);
	if (!bFired)
	{
		return;
	}

	if (!bIsBunkerMap && !TunaGameInstance->TryConsumeLoadedAmmoForWeaponSlot(SelectedWeaponSlotNumber))
	{
		return;
	}

	AddWeaponSpreadRecoilShot(WeaponTypeTag);
}

bool ATunaSweeperTopDownCharacter::CanUseSelectedWeaponSlot()
{
	if (SelectedWeaponSlotNumber <= 0)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	return TunaGameInstance && TunaGameInstance->IsEquipmentWeaponSlotOccupied(SelectedWeaponSlotNumber);
}

bool ATunaSweeperTopDownCharacter::CanUseSelectedMeleeWeapon()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	return TunaGameInstance && TunaGameInstance->IsEquipmentMeleeSlotOccupied();
}

bool ATunaSweeperTopDownCharacter::SelectWeaponSlot(int32 SlotNumber)
{
	if (bIsDead || SlotNumber < 1 || SlotNumber > 2)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance || !TunaGameInstance->IsEquipmentWeaponSlotOccupied(SlotNumber))
	{
		return false;
	}

	if (SelectedWeaponSlotNumber != SlotNumber || bMeleeWeaponSelected)
	{
		CancelReload();
		CancelItemUse();
		CloseAmmoSelection();
		CancelMeleeSwing();
		ClearEquippedWeaponActor();
		ResetWeaponSpreadRecoil();
	}

	SelectedWeaponSlotNumber = SlotNumber;
	bMeleeWeaponSelected = false;
	TunaGameInstance->SetRuntimeSelectedWeaponSlotNumber(SlotNumber);
	EnsureEquippedWeaponActor();
	return true;
}

bool ATunaSweeperTopDownCharacter::SelectMeleeWeapon()
{
	if (bIsDead || !CanUseSelectedMeleeWeapon())
	{
		return false;
	}

	if (!bMeleeWeaponSelected || SelectedWeaponSlotNumber != 0)
	{
		CancelReload();
		CancelItemUse();
		CloseAmmoSelection();
		CancelMeleeSwing();
		ClearEquippedWeaponActor();
		ResetWeaponSpreadRecoil();
	}

	SelectedWeaponSlotNumber = 0;
	bMeleeWeaponSelected = true;
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->SetRuntimeSelectedMeleeWeapon();
	}
	EnsureEquippedWeaponActor();
	return true;
}

bool ATunaSweeperTopDownCharacter::RestoreRuntimeSelectedWeaponSelection()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return false;
	}

	bool bRestoreMeleeWeapon = false;
	int32 RestoreWeaponSlotNumber = 1;
	if (!TunaGameInstance->TryGetRuntimeSelectedWeaponSelection(bRestoreMeleeWeapon, RestoreWeaponSlotNumber))
	{
		return false;
	}

	return bRestoreMeleeWeapon
		? SelectMeleeWeapon()
		: SelectWeaponSlot(RestoreWeaponSlotNumber);
}

void ATunaSweeperTopDownCharacter::RefreshSelectedWeaponAfterInventoryChanged()
{
	if (SelectedWeaponSlotNumber > 0 && CanUseSelectedWeaponSlot())
	{
		ApplyEquippedWeaponAttachmentVisuals();
		return;
	}
	if (bMeleeWeaponSelected && CanUseSelectedMeleeWeapon())
	{
		ApplyEquippedWeaponAttachmentVisuals();
		return;
	}

	CancelReload();
	CancelItemUse();
	CloseAmmoSelection();
	CancelMeleeSwing();
	SelectedWeaponSlotNumber = 0;
	bMeleeWeaponSelected = false;
	ClearEquippedWeaponActor();
	ResetWeaponSpreadRecoil();
}

void ATunaSweeperTopDownCharacter::HandleInventoryStateChanged()
{
	RefreshSelectedWeaponAfterInventoryChanged();
	RefreshCarryWeightConditionDebuffs();
}

void ATunaSweeperTopDownCharacter::UpdateWeaponSpreadRecoil(float DeltaSeconds)
{
	FName SelectedWeaponTypeTag = NAME_None;
	if (!TryGetSelectedWeaponTypeTag(SelectedWeaponTypeTag))
	{
		ResetWeaponSpreadRecoil();
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperWeaponSpreadRecoilDefinition RecoilDefinition;
	if (!TunaGameInstance || !TunaGameInstance->TryGetWeaponSpreadRecoilDefinition(SelectedWeaponTypeTag, RecoilDefinition))
	{
		ResetWeaponSpreadRecoil();
		return;
	}

	if (EquippedWeapon)
	{
		EquippedWeapon->ConfigureRuntimeSpreadRecoil(SelectedWeaponTypeTag, RecoilDefinition);
	}
}

void ATunaSweeperTopDownCharacter::ResetWeaponSpreadRecoil()
{
	if (EquippedWeapon)
	{
		EquippedWeapon->ResetRuntimeSpreadRecoil();
	}
}

bool ATunaSweeperTopDownCharacter::TryGetSelectedWeaponTypeTag(FName& OutWeaponTypeTag) const
{
	OutWeaponTypeTag = NAME_None;
	if (bMeleeWeaponSelected || SelectedWeaponSlotNumber <= 0)
	{
		return false;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return false;
	}

	FTunaSweeperItemInstance WeaponInstance;
	FTunaSweeperItemDefinition WeaponDefinition;
	if (!TunaGameInstance->TryGetEquipmentWeaponSlotItem(SelectedWeaponSlotNumber, WeaponInstance, WeaponDefinition) ||
		WeaponDefinition.WeaponTypeTag.IsNone())
	{
		return false;
	}

	OutWeaponTypeTag = WeaponDefinition.WeaponTypeTag;
	return true;
}

float ATunaSweeperTopDownCharacter::ResolveWeaponSpreadHalfAngleDegrees(FName WeaponTypeTag) const
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperWeaponSpreadRecoilDefinition RecoilDefinition;
	if (!EquippedWeapon ||
		!TunaGameInstance ||
		!TunaGameInstance->TryGetWeaponSpreadRecoilDefinition(WeaponTypeTag, RecoilDefinition))
	{
		return 0.0f;
	}

	EquippedWeapon->ConfigureRuntimeSpreadRecoil(WeaponTypeTag, RecoilDefinition);
	return bIsAiming
		? EquippedWeapon->GetRuntimeAimedSpreadHalfAngleDegrees()
		: EquippedWeapon->GetRuntimeSpreadHalfAngleDegrees() * FMath::Max(1.0f, HipFireSpreadMultiplier);
}

void ATunaSweeperTopDownCharacter::AddWeaponSpreadRecoilShot(FName WeaponTypeTag)
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperWeaponSpreadRecoilDefinition RecoilDefinition;
	if (!EquippedWeapon ||
		!TunaGameInstance ||
		!TunaGameInstance->TryGetWeaponSpreadRecoilDefinition(WeaponTypeTag, RecoilDefinition))
	{
		return;
	}

	EquippedWeapon->ConfigureRuntimeSpreadRecoil(WeaponTypeTag, RecoilDefinition);
	EquippedWeapon->AddRuntimeSpreadRecoilShot();
}

