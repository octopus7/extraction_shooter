#pragma once

#include "CoreMinimal.h"

class FTunaSweeperEditorRunOnce
{
public:
	static bool HasCompleted(const FString& TaskId);
	static bool Run(const FString& TaskId, TFunctionRef<bool()> Task);
	static void MarkCompleted(const FString& TaskId);

private:
	static void GetCompletedTasks(TArray<FString>& OutTasks);
};
