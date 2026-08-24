#pragma once

#include "CoreMinimal.h"

enum class ETunaSweeperBuildFlavor : uint8
{
	Main,
	Demo
};

namespace TunaSweeperBuildFlavor
{
	TUNASWEEPER_API ETunaSweeperBuildFlavor Get();
	TUNASWEEPER_API bool IsDemo();
	TUNASWEEPER_API FName GetName();
	TUNASWEEPER_API FString GetSaveGameDirectory();
	TUNASWEEPER_API FString GetExternalMainPayloadRoot();
	TUNASWEEPER_API FString GetStagedMainPayloadRoot();
	TUNASWEEPER_API FString GetMainPayloadRoot();
	TUNASWEEPER_API FString GetQuestDefinitionsPath();
	TUNASWEEPER_API void GetQuestTextStringPaths(TArray<FString>& OutPaths);
	TUNASWEEPER_API FString GetScenarioDefinitionsPath();
	TUNASWEEPER_API FString GetScenarioTextStringsPath();
	TUNASWEEPER_API FName ResolveInitialGameplayLevel(FName DemoLevel, FName MainFallbackLevel);
	TUNASWEEPER_API int32 GetMaximumSaveSlotIndex();
	TUNASWEEPER_API FName GetBunkerLevelName();
	TUNASWEEPER_API FName GetRaidGameplayLevelName();
	TUNASWEEPER_API bool IsRaidGameplayLevelName(FName LevelName);
	TUNASWEEPER_API FName ResolveGameplayLevelName(FName LevelName);
	TUNASWEEPER_API FString GetRuntimePlacementDataPath(const TCHAR* FileName);
}
