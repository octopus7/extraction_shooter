#include "TunaSweeperGameInstanceShared.h"

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

float UTunaSweeperGameInstance::GetDialogueCharactersPerSecond() const
{
	if (const float* RuntimeCharactersPerSecond = NumberSettings.Find(TunaSweeperDialogue::CharactersPerSecondSettingKey))
	{
		return FMath::Max(0.1f, *RuntimeCharactersPerSecond);
	}

	return FMath::Max(0.1f, GameplaySettings.DialogueCharactersPerSecond);
}

void UTunaSweeperGameInstance::SetVisionDebugEnabled(bool bEnabled)
{
	SetBoolSetting(TunaSweeperDebug::VisionDebugSettingKey, bEnabled);
}

bool UTunaSweeperGameInstance::IsVisionDebugEnabled() const
{
	if (const bool* RuntimeVisionDebug = BoolSettings.Find(TunaSweeperDebug::VisionDebugSettingKey))
	{
		return *RuntimeVisionDebug;
	}

	return GameplaySettings.bEnableVisionDebug;
}

void UTunaSweeperGameInstance::SetBunkerVisionDebugEnabled(bool bEnabled)
{
	SetBoolSetting(TunaSweeperDebug::BunkerVisionDebugSettingKey, bEnabled);
}

bool UTunaSweeperGameInstance::IsBunkerVisionDebugEnabled() const
{
	if (const bool* RuntimeBunkerVisionDebug = BoolSettings.Find(TunaSweeperDebug::BunkerVisionDebugSettingKey))
	{
		return *RuntimeBunkerVisionDebug;
	}

	return GameplaySettings.bEnableBunkerVisionDebug;
}

bool UTunaSweeperGameInstance::TryGetProjectileHitEffectDefinition(
	FName EffectId,
	FTunaSweeperProjectileHitEffectDefinition& OutDefinition) const
{
	OutDefinition = FTunaSweeperProjectileHitEffectDefinition();
	if (EffectId.IsNone())
	{
		return false;
	}

	const UTunaSweeperProjectileHitEffectDataAsset* DataAsset = ProjectileHitEffectDataAsset.LoadSynchronous();
	if (!DataAsset)
	{
		DataAsset = LoadObject<UTunaSweeperProjectileHitEffectDataAsset>(
			nullptr,
			TunaSweeperProjectileHitEffects::DefaultDataAssetPath);
	}

	return DataAsset && DataAsset->TryGetHitEffect(EffectId, OutDefinition);
}

bool UTunaSweeperGameInstance::TryGetWeaponSpreadRecoilDefinition(
	FName WeaponTypeTag,
	FTunaSweeperWeaponSpreadRecoilDefinition& OutDefinition) const
{
	OutDefinition = FTunaSweeperWeaponSpreadRecoilDefinition();
	if (WeaponTypeTag.IsNone())
	{
		return false;
	}

	const UTunaSweeperWeaponSpreadRecoilDataAsset* DataAsset = WeaponSpreadRecoilDataAsset.LoadSynchronous();
	if (!DataAsset)
	{
		DataAsset = LoadObject<UTunaSweeperWeaponSpreadRecoilDataAsset>(
			nullptr,
			TunaSweeperWeaponSpreadRecoil::DefaultDataAssetPath);
	}

	if (DataAsset && DataAsset->TryGetDefinition(WeaponTypeTag, OutDefinition))
	{
		TunaSweeperWeaponSpreadRecoil::NormalizeDefinition(OutDefinition);
		return true;
	}

	if (TunaSweeperWeaponSpreadRecoil::TryGetFallbackDefinition(WeaponTypeTag, OutDefinition))
	{
		TunaSweeperWeaponSpreadRecoil::NormalizeDefinition(OutDefinition);
		return true;
	}

	return false;
}

