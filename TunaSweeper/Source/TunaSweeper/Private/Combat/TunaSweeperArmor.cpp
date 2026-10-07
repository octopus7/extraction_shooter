#include "Combat/TunaSweeperArmor.h"

#include "Engine/DamageEvents.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Weapon/TunaSweeperProjectile.h"

int32 TunaSweeperArmor::ResolvePenetrationTier(const FDamageEvent& Event, const AActor* Causer)
{
	const ATunaSweeperProjectile* Projectile = Cast<ATunaSweeperProjectile>(Causer);
	return Event.IsOfType(FPointDamageEvent::ClassID) && Projectile ? Projectile->GetPenetrationTier() : 0;
}

float TunaSweeperArmor::ItemDefense(UTunaSweeperItemDataSubsystem* Items, int32 ItemId,
	FName RequiredSlot, int32 PenetrationTier)
{
	FTunaSweeperItemDefinition Item;
	return Items && Items->TryGetItemDefinition(ItemId, Item) && Item.EquipmentSlotTag == RequiredSlot
		? EffectiveDefense(Item.DefenseValue, Item.ArmorTier, PenetrationTier) : 0.0f;
}
