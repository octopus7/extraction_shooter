#include "Settings/TunaSweeperBuildFlavor.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#if WITH_EDITOR
#include "Settings/TunaSweeperBuildTargetSettings.h"
#endif

namespace TunaSweeperBuildFlavor
{
	namespace
	{
		constexpr const TCHAR* MainPayloadManifestName = TEXT("main-payload.json");

		FString GetPublicDataPath(const TCHAR* FileName)
		{
			return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Data"), FileName));
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
}
