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
	TempOpenLootItems.Reset();
	bHasGeneratedTempOpenLootItems = false;
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
