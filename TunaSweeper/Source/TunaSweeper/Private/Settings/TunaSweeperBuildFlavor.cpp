#include "Settings/TunaSweeperBuildFlavor.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Settings/TunaSweeperBuildTargetSettings.h"

namespace TunaSweeperBuildFlavor
{
	namespace
	{
		constexpr const TCHAR* MainPayloadManifestName = TEXT("main-payload.json");
		constexpr const TCHAR* MainRaidLevelPath = TEXT("/Game/MainRaid/RaidMap");
		constexpr const TCHAR* DemoRaidLevelName = TEXT("DemoRaidMap");
		constexpr const TCHAR* DemoBoxRaidLevelName = TEXT("DemoBoxRaidMap");

		FString GetPublicDataPath(const TCHAR* FileName)
		{
			return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"), FileName));
		}

		FString NormalizeLevelName(FName LevelName)
		{
			FString Result = LevelName.ToString();
			int32 SlashIndex = INDEX_NONE;
			if (Result.FindLastChar(TEXT('/'), SlashIndex))
			{
				Result.RightChopInline(SlashIndex + 1);
			}
			while (Result.StartsWith(TEXT("UEDPIE_")))
			{
				int32 PrefixEnd = INDEX_NONE;
				if (!Result.FindChar(TEXT('_'), PrefixEnd))
				{
					break;
				}
				Result.RightChopInline(PrefixEnd + 1);
				if (Result.Len() > 0 && FChar::IsDigit(Result[0]))
				{
					int32 NextUnderscore = INDEX_NONE;
					if (Result.FindChar(TEXT('_'), NextUnderscore))
					{
						Result.RightChopInline(NextUnderscore + 1);
					}
				}
			}
			return Result;
		}
	}

	ETunaSweeperBuildFlavor Get()
	{
#if WITH_EDITOR
		if (const UTunaSweeperBuildTargetSettings* Settings = GetDefault<UTunaSweeperBuildTargetSettings>())
		{
			return Settings->IsDemoBuild() ? ETunaSweeperBuildFlavor::Demo : ETunaSweeperBuildFlavor::Main;
		}
		return ETunaSweeperBuildFlavor::Main;
#else
		return TUNASWEEPER_DEMO != 0 ? ETunaSweeperBuildFlavor::Demo : ETunaSweeperBuildFlavor::Main;
#endif
	}

	bool IsDemo()
	{
		return Get() == ETunaSweeperBuildFlavor::Demo;
	}

	FName GetName()
	{
		return IsDemo() ? FName(TEXT("Demo")) : FName(TEXT("Main"));
	}

	FString GetSaveGameDirectory()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectSavedDir(), TEXT("SaveGames"), GetName().ToString()));
	}

	FString GetExternalMainPayloadRoot()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(), TEXT("External"), TEXT("MainPayload")));
	}

	FString GetStagedMainPayloadRoot()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectContentDir(), TEXT("Data"), TEXT("MainPayloadStaged")));
	}

	FString GetMainPayloadRoot()
	{
		const FString ExternalRoot = GetExternalMainPayloadRoot();
		if (FPaths::FileExists(FPaths::Combine(ExternalRoot, MainPayloadManifestName)))
		{
			return ExternalRoot;
		}
		return GetStagedMainPayloadRoot();
	}

	FString GetQuestDefinitionsPath()
	{
		return IsDemo()
			? GetPublicDataPath(TEXT("QuestDefinitions.json"))
			: FPaths::Combine(GetMainPayloadRoot(), TEXT("Data"), TEXT("QuestDefinitions.json"));
	}

	void GetQuestTextStringPaths(TArray<FString>& OutPaths)
	{
		OutPaths.Reset();
		OutPaths.Add(GetPublicDataPath(TEXT("QuestTextStrings.csv")));
		if (!IsDemo())
		{
			OutPaths.Add(FPaths::Combine(GetMainPayloadRoot(), TEXT("Data"), TEXT("QuestTextStrings.csv")));
		}
	}

	FString GetScenarioDefinitionsPath()
	{
		return IsDemo()
			? GetPublicDataPath(TEXT("ScenarioDefinitions.json"))
			: FPaths::Combine(GetMainPayloadRoot(), TEXT("Data"), TEXT("ScenarioDefinitions.json"));
	}

	FString GetScenarioTextStringsPath()
	{
		return IsDemo()
			? GetPublicDataPath(TEXT("ScenarioTextStrings.csv"))
			: FPaths::Combine(GetMainPayloadRoot(), TEXT("Data"), TEXT("ScenarioTextStrings.csv"));
	}

	FName ResolveInitialGameplayLevel(FName DemoLevel, FName MainFallbackLevel)
	{
		if (IsDemo())
		{
			return DemoLevel;
		}

		FString ManifestText;
		const FString ManifestPath = FPaths::Combine(GetMainPayloadRoot(), MainPayloadManifestName);
		if (!FFileHelper::LoadFileToString(ManifestText, *ManifestPath))
		{
			return MainFallbackLevel;
		}

		TSharedPtr<FJsonObject> Manifest;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ManifestText);
		FString InitialLevel;
		if (!FJsonSerializer::Deserialize(Reader, Manifest) ||
			!Manifest.IsValid() ||
			!Manifest->TryGetStringField(TEXT("initial_level"), InitialLevel) ||
			InitialLevel.IsEmpty())
		{
			return MainFallbackLevel;
		}

		return FName(*InitialLevel);
	}

	int32 GetMaximumSaveSlotIndex()
	{
		return IsDemo() ? 1 : 3;
	}

	FName GetBunkerLevelName()
	{
		return FName(TEXT("BunkerMap"));
	}

	FName GetRaidGameplayLevelName()
	{
		if (IsDemo())
		{
			const UTunaSweeperBuildTargetSettings* Settings = GetDefault<UTunaSweeperBuildTargetSettings>();
			return FName(Settings && Settings->bUseBoxRaidLevel
				? DemoBoxRaidLevelName
				: DemoRaidLevelName);
		}

		FString ManifestText;
		const FString ManifestPath = FPaths::Combine(GetMainPayloadRoot(), MainPayloadManifestName);
		TSharedPtr<FJsonObject> Manifest;
		FString RaidLevel;
		if (FFileHelper::LoadFileToString(ManifestText, *ManifestPath) &&
			FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ManifestText), Manifest) &&
			Manifest.IsValid() && Manifest->TryGetStringField(TEXT("raid_level"), RaidLevel) && !RaidLevel.IsEmpty())
		{
			return RaidLevel == TEXT("RaidMap") ? FName(MainRaidLevelPath) : FName(*RaidLevel);
		}

		return FName(MainRaidLevelPath);
	}

	bool IsRaidGameplayLevelName(FName LevelName)
	{
		return NormalizeLevelName(LevelName) == NormalizeLevelName(GetRaidGameplayLevelName());
	}

	FName ResolveGameplayLevelName(FName LevelName)
	{
		const FString Normalized = NormalizeLevelName(LevelName);
		if (Normalized == TEXT("RaidMap") ||
			Normalized == DemoRaidLevelName ||
			Normalized == DemoBoxRaidLevelName)
		{
			return GetRaidGameplayLevelName();
		}
		return LevelName;
	}

	FString GetRuntimePlacementDataPath(const TCHAR* FileName)
	{
		if (IsDemo())
		{
			return GetPublicDataPath(FileName);
		}

		const FString ProtectedPath = FPaths::Combine(GetMainPayloadRoot(), TEXT("Data"), FileName);
		return FPaths::FileExists(ProtectedPath)
			? ProtectedPath
			: GetPublicDataPath(*FPaths::Combine(TEXT("MainRuntimeDefaults"), FileName));
	}
}
