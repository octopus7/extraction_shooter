#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

namespace TunaSweeperSafeSave
{
	using FSaveValidator = TFunction<bool(const USaveGame&)>;

	FString GetCandidateFilePath(const FString& PrimaryFilePath);
	FString GetPreviousFilePath(const FString& PrimaryFilePath);

	USaveGame* LoadVerifiedSaveFile(
		const FString& SaveFilePath,
		const FSaveValidator& Validator);

	USaveGame* LoadSaveFileWithRecovery(
		const FString& PrimaryFilePath,
		const TArray<FString>& RecoveryFilePaths,
		const FSaveValidator& Validator,
		FString* OutRecoveryFilePath = nullptr);

	bool SaveGameFileFailClosed(
		USaveGame* SaveGame,
		const FString& PrimaryFilePath,
		const FSaveValidator& Validator);

	bool DeleteSaveArtifacts(const FString& PrimaryFilePath);
}
