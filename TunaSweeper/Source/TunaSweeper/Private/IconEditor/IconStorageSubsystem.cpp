#include "IconEditor/IconStorageSubsystem.h"

#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Settings/TunaSweeperBuildFlavor.h"

#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#else
#include <cstdio>
#endif

FString UIconStorageSubsystem::GetIconFilePath() const
{
	return FPaths::Combine(TunaSweeperBuildFlavor::GetSaveGameDirectory(), TEXT("GlobalIcon.rgb"));
}

EIconLoadResult UIconStorageSubsystem::LoadRgbIcon(TArray<uint8>& OutPixels) const
{
	OutPixels.Init(0, ByteCount);
	const FString Filename = GetIconFilePath();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.FileExists(*Filename)) return EIconLoadResult::Missing;

	TUniquePtr<IFileHandle> Reader(PlatformFile.OpenRead(*Filename));
	if (!Reader) return EIconLoadResult::ReadFailed;
	if (Reader->Size() != ByteCount) return EIconLoadResult::Invalid;

	TArray<uint8> Loaded;
	Loaded.SetNumUninitialized(ByteCount);
	if (!Reader->Read(Loaded.GetData(), ByteCount)) return EIconLoadResult::ReadFailed;
	OutPixels = MoveTemp(Loaded);
	return EIconLoadResult::Loaded;
}

bool UIconStorageSubsystem::TrySaveIcon(const TArray<uint8>& Pixels) const
{
	if (Pixels.Num() != ByteCount) return false;
	const FString Filename = GetIconFilePath();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.CreateDirectoryTree(*FPaths::GetPath(Filename))) return false;

	// Unique siblings avoid sharing partial writes between editor/game processes.
	const FString Candidate = Filename + TEXT(".") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".candidate");
	bool bWritten = false;
	{
		TUniquePtr<IFileHandle> Writer(PlatformFile.OpenWrite(*Candidate));
		bWritten = Writer && Writer->Write(Pixels.GetData(), ByteCount) && Writer->Flush(true);
	}
	TArray<uint8> ReadBack;
	if (!bWritten || !FFileHelper::LoadFileToArray(ReadBack, *Candidate) || ReadBack != Pixels)
	{
		PlatformFile.DeleteFile(*Candidate);
		return false;
	}

	// Do not use IFileManager::Move(replace=true): its generic implementation can
	// delete the original before the rename. Rename the same-directory file in one OS operation.
#if PLATFORM_WINDOWS
	const FString PhysicalCandidate = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*Candidate);
	const FString PhysicalFilename = IFileManager::Get().ConvertToAbsolutePathForExternalAppForWrite(*Filename);
	const bool bReplaced = ::MoveFileExW(*PhysicalCandidate, *PhysicalFilename,
		MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
	const bool bReplaced = std::rename(TCHAR_TO_UTF8(*Candidate), TCHAR_TO_UTF8(*Filename)) == 0;
#endif
	if (!bReplaced) PlatformFile.DeleteFile(*Candidate);
	return bReplaced;
}
