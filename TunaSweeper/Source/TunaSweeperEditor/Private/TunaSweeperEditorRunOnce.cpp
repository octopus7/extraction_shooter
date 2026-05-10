#include "TunaSweeperEditorRunOnce.h"

#include "Misc/ConfigCacheIni.h"

namespace TunaSweeperEditorRunOnce
{
	const TCHAR* SectionName = TEXT("TunaSweeperEditor.RunOnce");
	const TCHAR* CompletedTasksKey = TEXT("CompletedTasks");
}

bool FTunaSweeperEditorRunOnce::HasCompleted(const FString& TaskId)
{
	TArray<FString> CompletedTasks;
	GetCompletedTasks(CompletedTasks);
	return CompletedTasks.Contains(TaskId);
}

bool FTunaSweeperEditorRunOnce::Run(const FString& TaskId, TFunctionRef<bool()> Task)
{
	if (HasCompleted(TaskId))
	{
		return false;
	}

	if (!Task())
	{
		return false;
	}

	MarkCompleted(TaskId);
	return true;
}

void FTunaSweeperEditorRunOnce::MarkCompleted(const FString& TaskId)
{
	if (TaskId.IsEmpty() || !GConfig)
	{
		return;
	}

	TArray<FString> CompletedTasks;
	GetCompletedTasks(CompletedTasks);

	if (!CompletedTasks.Contains(TaskId))
	{
		CompletedTasks.Add(TaskId);
		GConfig->SetArray(TunaSweeperEditorRunOnce::SectionName, TunaSweeperEditorRunOnce::CompletedTasksKey, CompletedTasks, GEditorPerProjectIni);
		GConfig->Flush(false, GEditorPerProjectIni);
	}
}

void FTunaSweeperEditorRunOnce::GetCompletedTasks(TArray<FString>& OutTasks)
{
	OutTasks.Reset();

	if (GConfig)
	{
		GConfig->GetArray(TunaSweeperEditorRunOnce::SectionName, TunaSweeperEditorRunOnce::CompletedTasksKey, OutTasks, GEditorPerProjectIni);
	}
}
