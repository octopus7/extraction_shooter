#include "Game/TunaSweeperGameInstance.h"

#include "Engine/Texture2D.h"

namespace TunaSweeperTempOpenLoot
{
	struct FTempLootSeed
	{
		const TCHAR* DisplayName;
		const TCHAR* TexturePath;
		int32 MinQuantity;
		int32 MaxQuantity;
	};

	const FTempLootSeed TempLootSeeds[] = {
		{ TEXT("Pistol"), TEXT("/Game/UI/Icons/T_UIIcon_Pistol.T_UIIcon_Pistol"), 1, 2 },
		{ TEXT("Rifle"), TEXT("/Game/UI/Icons/T_UIIcon_Rifle.T_UIIcon_Rifle"), 1, 1 },
		{ TEXT("Shotgun"), TEXT("/Game/UI/Icons/T_UIIcon_Shotgun.T_UIIcon_Shotgun"), 1, 1 },
		{ TEXT("Pistol Ammo"), TEXT("/Game/UI/Icons/T_UIIcon_PistolAmmo.T_UIIcon_PistolAmmo"), 18, 72 },
		{ TEXT("Rifle Ammo"), TEXT("/Game/UI/Icons/T_UIIcon_RifleAmmo.T_UIIcon_RifleAmmo"), 30, 120 },
		{ TEXT("Shotgun Ammo"), TEXT("/Game/UI/Icons/T_UIIcon_ShotgunAmmo.T_UIIcon_ShotgunAmmo"), 6, 32 },
		{ TEXT("Canned Food"), TEXT("/Game/UI/Icons/T_UIIcon_CannedFood.T_UIIcon_CannedFood"), 1, 5 },
		{ TEXT("Water Bottle"), TEXT("/Game/UI/Icons/T_UIIcon_WaterBottle.T_UIIcon_WaterBottle"), 1, 4 },
		{ TEXT("Energy Bar"), TEXT("/Game/UI/Icons/T_UIIcon_EnergyBar.T_UIIcon_EnergyBar"), 1, 6 },
		{ TEXT("Bandage"), TEXT("/Game/UI/Icons/T_UIIcon_Bandage.T_UIIcon_Bandage"), 1, 8 },
		{ TEXT("First Aid Kit"), TEXT("/Game/UI/Icons/T_UIIcon_FirstAidKit.T_UIIcon_FirstAidKit"), 1, 3 },
		{ TEXT("Painkillers"), TEXT("/Game/UI/Icons/T_UIIcon_Painkillers.T_UIIcon_Painkillers"), 1, 6 },
		{ TEXT("Antibiotics"), TEXT("/Game/UI/Icons/T_UIIcon_Antibiotics.T_UIIcon_Antibiotics"), 1, 5 },
		{ TEXT("Body Armor"), TEXT("/Game/UI/Icons/T_UIIcon_BodyArmor.T_UIIcon_BodyArmor"), 1, 1 },
		{ TEXT("Backpack"), TEXT("/Game/UI/Icons/T_UIIcon_Backpack.T_UIIcon_Backpack"), 1, 1 },
		{ TEXT("Valuables Crate"), TEXT("/Game/UI/Icons/T_UIIcon_ValuablesCrate.T_UIIcon_ValuablesCrate"), 1, 2 },
	};
}

void FTunaSweeperPlayerHudState::NormalizeWeightLimits()
{
	CurrentCarryWeight = FMath::Max(0.0f, CurrentCarryWeight);
	MaxCarryWeight = FMath::Max(1.0f, MaxCarryWeight);
	MovementBlockedWeight = FMath::Max(MovementBlockedWeight, MaxCarryWeight * 2.0f);
	Health = FMath::Clamp(Health, 0.0f, 100.0f);
	Hunger = FMath::Clamp(Hunger, 0.0f, 100.0f);
	Hydration = FMath::Clamp(Hydration, 0.0f, 100.0f);
}

bool FTunaSweeperPlayerHudState::IsCarryWeightOverLimit() const
{
	return MaxCarryWeight > 0.0f && CurrentCarryWeight >= MaxCarryWeight;
}

bool FTunaSweeperPlayerHudState::IsCarryWeightMovementBlocked() const
{
	return MovementBlockedWeight > 0.0f && CurrentCarryWeight >= MovementBlockedWeight;
}

float FTunaSweeperPlayerHudState::GetCarryWeightMovementSpeedMultiplier() const
{
	if (IsCarryWeightMovementBlocked())
	{
		return 0.0f;
	}

	return IsCarryWeightOverLimit() ? 0.5f : 1.0f;
}

void UTunaSweeperGameInstance::SetGameplayInfo(FName Key, const FString& Value)
{
	if (!Key.IsNone())
	{
		GameplayInfo.Add(Key, Value);
	}
}

bool UTunaSweeperGameInstance::TryGetGameplayInfo(FName Key, FString& OutValue) const
{
	if (const FString* FoundValue = GameplayInfo.Find(Key))
	{
		OutValue = *FoundValue;
		return true;
	}

	OutValue.Reset();
	return false;
}

void UTunaSweeperGameInstance::SetNumberSetting(FName Key, float Value)
{
	if (!Key.IsNone())
	{
		NumberSettings.Add(Key, Value);
	}
}

bool UTunaSweeperGameInstance::TryGetNumberSetting(FName Key, float& OutValue) const
{
	if (const float* FoundValue = NumberSettings.Find(Key))
	{
		OutValue = *FoundValue;
		return true;
	}

	OutValue = 0.0f;
	return false;
}

void UTunaSweeperGameInstance::SetBoolSetting(FName Key, bool bValue)
{
	if (!Key.IsNone())
	{
		BoolSettings.Add(Key, bValue);
	}
}

bool UTunaSweeperGameInstance::TryGetBoolSetting(FName Key, bool& bOutValue) const
{
	if (const bool* FoundValue = BoolSettings.Find(Key))
	{
		bOutValue = *FoundValue;
		return true;
	}

	bOutValue = false;
	return false;
}

void UTunaSweeperGameInstance::ClearRuntimeState()
{
	GameplayInfo.Reset();
	NumberSettings.Reset();
	BoolSettings.Reset();
	PlayerHudState = FTunaSweeperPlayerHudState();
	TempOpenLootItems.Reset();
	bHasGeneratedTempOpenLootItems = false;
}

void UTunaSweeperGameInstance::SetPlayerHudState(const FTunaSweeperPlayerHudState& InHudState)
{
	PlayerHudState = InHudState;
	PlayerHudState.NormalizeWeightLimits();
}

void UTunaSweeperGameInstance::SetCarryWeight(float CurrentCarryWeight, float MaxCarryWeight, float MovementBlockedWeight)
{
	PlayerHudState.CurrentCarryWeight = CurrentCarryWeight;
	PlayerHudState.MaxCarryWeight = MaxCarryWeight;
	PlayerHudState.MovementBlockedWeight = MovementBlockedWeight;
	PlayerHudState.NormalizeWeightLimits();
}

float UTunaSweeperGameInstance::GetCarryWeightMovementSpeedMultiplier() const
{
	FTunaSweeperPlayerHudState NormalizedHudState = PlayerHudState;
	NormalizedHudState.NormalizeWeightLimits();
	return NormalizedHudState.GetCarryWeightMovementSpeedMultiplier();
}

const TArray<FTunaSweeperTempOpenLootItemData>& UTunaSweeperGameInstance::GetOrCreateTempOpenLootItems()
{
	if (!bHasGeneratedTempOpenLootItems)
	{
		GenerateTempOpenLootItems();
	}

	return TempOpenLootItems;
}

void UTunaSweeperGameInstance::GetTempOpenLootItems(TArray<FTunaSweeperTempOpenLootItemData>& OutItems)
{
	OutItems = GetOrCreateTempOpenLootItems();
}

void UTunaSweeperGameInstance::GenerateTempOpenLootItems()
{
	TempOpenLootItems.Reset();

	FRandomStream QuantityStream(static_cast<int32>(FDateTime::Now().GetTicks() & 0x7fffffff));
	for (const TunaSweeperTempOpenLoot::FTempLootSeed& Seed : TunaSweeperTempOpenLoot::TempLootSeeds)
	{
		FTunaSweeperTempOpenLootItemData ItemData;
		ItemData.DisplayName = FText::FromString(FString(Seed.DisplayName));
		ItemData.Quantity = QuantityStream.RandRange(Seed.MinQuantity, Seed.MaxQuantity);
		ItemData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Seed.TexturePath));
		TempOpenLootItems.Add(ItemData);
	}

	bHasGeneratedTempOpenLootItems = true;
}
