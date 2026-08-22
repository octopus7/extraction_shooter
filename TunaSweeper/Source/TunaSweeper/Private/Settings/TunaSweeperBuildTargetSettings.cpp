#include "Settings/TunaSweeperBuildTargetSettings.h"

namespace TunaSweeperBuildTarget
{
	const TCHAR* ResolveTargetName(ETunaSweeperBuildTarget BuildTarget)
	{
		switch (BuildTarget)
		{
		case ETunaSweeperBuildTarget::NoStoreDemo: return TEXT("TunaSweeperNoStoreDemo");
		case ETunaSweeperBuildTarget::SteamFull: return TEXT("TunaSweeper");
		case ETunaSweeperBuildTarget::SteamDemo: return TEXT("TunaSweeperDemo");
		case ETunaSweeperBuildTarget::StoveFull: return TEXT("TunaSweeperStove");
		case ETunaSweeperBuildTarget::StoveDemo: return TEXT("TunaSweeperStoveDemo");
		case ETunaSweeperBuildTarget::NoStoreFull:
		default: return TEXT("TunaSweeperNoStore");
		}
	}
}

FString UTunaSweeperBuildTargetSettings::GetDistributionChannel() const
{
	switch (BuildTarget)
	{
	case ETunaSweeperBuildTarget::SteamFull:
	case ETunaSweeperBuildTarget::SteamDemo: return TEXT("Steam");
	case ETunaSweeperBuildTarget::StoveFull:
	case ETunaSweeperBuildTarget::StoveDemo: return TEXT("Stove");
	case ETunaSweeperBuildTarget::NoStoreFull:
	case ETunaSweeperBuildTarget::NoStoreDemo:
	default: return TEXT("None");
	}
}

bool UTunaSweeperBuildTargetSettings::IsDemoBuild() const
{
	return BuildTarget == ETunaSweeperBuildTarget::NoStoreDemo ||
		BuildTarget == ETunaSweeperBuildTarget::SteamDemo ||
		BuildTarget == ETunaSweeperBuildTarget::StoveDemo;
}

#if WITH_EDITOR
#include "Misc/ConfigCacheIni.h"
#include "UObject/UnrealType.h"

void UTunaSweeperBuildTargetSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UTunaSweeperBuildTargetSettings, BuildTarget))
	{
		ApplyPackagingTarget();
	}
}

void UTunaSweeperBuildTargetSettings::ApplyPackagingTarget()
{
	static const TCHAR* PackagingSection = TEXT("/Script/UnrealEd.ProjectPackagingSettings");
	static const TCHAR* BuildTargetKey = TEXT("BuildTarget");
	const FString TargetName = TunaSweeperBuildTarget::ResolveTargetName(BuildTarget);
	GConfig->SetString(PackagingSection, BuildTargetKey, *TargetName, GGameIni);
	GConfig->Flush(false, GGameIni);

	UClass* PackagingSettingsClass = FindObject<UClass>(nullptr, TEXT("/Script/DeveloperToolSettings.ProjectPackagingSettings"));
	UObject* PackagingSettings = PackagingSettingsClass ? PackagingSettingsClass->GetDefaultObject() : nullptr;
	FStrProperty* BuildTargetProperty = PackagingSettingsClass ? FindFProperty<FStrProperty>(PackagingSettingsClass, BuildTargetKey) : nullptr;
	if (PackagingSettings && BuildTargetProperty)
	{
		BuildTargetProperty->SetPropertyValue_InContainer(PackagingSettings, TargetName);
		PackagingSettings->SaveConfig();
	}
}
#endif
