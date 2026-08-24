#include "Game/TunaSweeperSafeSave.h"

#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace TunaSweeperSafeSave
{
	namespace
	{
		constexpr uint32 EnvelopeMagic = 0x54535750;
		constexpr uint32 EnvelopeVersion = 1;
		constexpr int64 EnvelopeHeaderSize = sizeof(uint32) + sizeof(uint32) + sizeof(int64) + sizeof(uint32);

		bool DeleteFileIfPresent(const FString& FilePath)
		{
			return !FPaths::FileExists(FilePath) ||
				IFileManager::Get().Delete(*FilePath, false, true);
		}

		bool WriteBytesAndFlush(const FString& FilePath, const TArray<uint8>& Bytes)
		{
			if (Bytes.IsEmpty() || !IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), true))
			{
				return false;
			}

			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			TUniquePtr<IFileHandle> FileHandle(PlatformFile.OpenWrite(*FilePath, false, false));
			if (!FileHandle || !FileHandle->Write(Bytes.GetData(), Bytes.Num()))
			{
				return false;
			}

			return FileHandle->Flush(true);
		}

		bool EncodeSaveEnvelope(USaveGame* SaveGame, TArray<uint8>& OutBytes)
		{
			TArray<uint8> Payload;
			if (!SaveGame || !UGameplayStatics::SaveGameToMemory(SaveGame, Payload) || Payload.IsEmpty())
			{
				return false;
			}

			OutBytes.Reset();
			FMemoryWriter Writer(OutBytes, true);
			uint32 Magic = EnvelopeMagic;
			uint32 Version = EnvelopeVersion;
			int64 PayloadSize = Payload.Num();
			uint32 PayloadCrc = FCrc::MemCrc32(Payload.GetData(), Payload.Num());
			Writer << Magic;
			Writer << Version;
			Writer << PayloadSize;
			Writer << PayloadCrc;
			Writer.Serialize(Payload.GetData(), Payload.Num());
			return !Writer.IsError();
		}

		bool DecodeSaveEnvelope(const TArray<uint8>& FileBytes, TArray<uint8>& OutPayload)
		{
			if (FileBytes.Num() < static_cast<int32>(sizeof(uint32)))
			{
				return false;
			}

			FMemoryReader Reader(FileBytes, true);
			uint32 Magic = 0;
			Reader << Magic;
			if (Magic != EnvelopeMagic)
			{
				OutPayload = FileBytes;
				return true;
			}

			uint32 Version = 0;
			int64 PayloadSize = 0;
			uint32 ExpectedPayloadCrc = 0;
			Reader << Version;
			Reader << PayloadSize;
			Reader << ExpectedPayloadCrc;
			if (Reader.IsError() ||
				Version != EnvelopeVersion ||
				PayloadSize <= 0 ||
				PayloadSize > MAX_int32 ||
				Reader.Tell() != EnvelopeHeaderSize ||
				Reader.Tell() + PayloadSize != FileBytes.Num())
			{
				return false;
			}

			OutPayload.SetNumUninitialized(static_cast<int32>(PayloadSize));
			Reader.Serialize(OutPayload.GetData(), PayloadSize);
			return !Reader.IsError() &&
				FCrc::MemCrc32(OutPayload.GetData(), OutPayload.Num()) == ExpectedPayloadCrc;
		}

		bool WriteVerifiedCandidate(
			USaveGame* SaveGame,
			const FString& CandidateFilePath,
			const FSaveValidator& Validator)
		{
			TArray<uint8> EncodedBytes;
			if (!EncodeSaveEnvelope(SaveGame, EncodedBytes) ||
				!WriteBytesAndFlush(CandidateFilePath, EncodedBytes))
			{
				DeleteFileIfPresent(CandidateFilePath);
				return false;
			}

			if (!LoadVerifiedSaveFile(CandidateFilePath, Validator))
			{
				DeleteFileIfPresent(CandidateFilePath);
				return false;
			}

			return true;
		}

		bool CopyVerifiedFile(
			const FString& SourceFilePath,
			const FString& DestinationFilePath,
			const FSaveValidator& Validator)
		{
			TArray<uint8> SourceBytes;
			const FString DestinationCandidateFilePath = GetCandidateFilePath(DestinationFilePath);
			DeleteFileIfPresent(DestinationCandidateFilePath);
			if (!FFileHelper::LoadFileToArray(SourceBytes, *SourceFilePath) ||
				!WriteBytesAndFlush(DestinationCandidateFilePath, SourceBytes) ||
				!LoadVerifiedSaveFile(DestinationCandidateFilePath, Validator) ||
				!IFileManager::Get().Move(
					*DestinationFilePath,
					*DestinationCandidateFilePath,
					true,
					true,
					false,
					true) ||
				!LoadVerifiedSaveFile(DestinationFilePath, Validator))
			{
				DeleteFileIfPresent(DestinationCandidateFilePath);
				return false;
			}

			return true;
		}
	}

	FString GetCandidateFilePath(const FString& PrimaryFilePath)
	{
		return PrimaryFilePath + TEXT(".candidate");
	}

	FString GetPreviousFilePath(const FString& PrimaryFilePath)
	{
		return PrimaryFilePath + TEXT(".previous");
	}

	USaveGame* LoadVerifiedSaveFile(
		const FString& SaveFilePath,
		const FSaveValidator& Validator)
	{
		TArray<uint8> FileBytes;
		TArray<uint8> Payload;
		if (!FFileHelper::LoadFileToArray(FileBytes, *SaveFilePath) ||
			!DecodeSaveEnvelope(FileBytes, Payload))
		{
			return nullptr;
		}

		USaveGame* SaveGame = UGameplayStatics::LoadGameFromMemory(Payload);
		return SaveGame && Validator(*SaveGame) ? SaveGame : nullptr;
	}

	USaveGame* LoadSaveFileWithRecovery(
		const FString& PrimaryFilePath,
		const TArray<FString>& RecoveryFilePaths,
		const FSaveValidator& Validator,
		FString* OutRecoveryFilePath)
	{
		if (OutRecoveryFilePath)
		{
			OutRecoveryFilePath->Reset();
		}

		DeleteFileIfPresent(GetCandidateFilePath(PrimaryFilePath));
		if (USaveGame* PrimarySave = LoadVerifiedSaveFile(PrimaryFilePath, Validator))
		{
			return PrimarySave;
		}

		for (const FString& RecoveryFilePath : RecoveryFilePaths)
		{
			USaveGame* RecoverySave = LoadVerifiedSaveFile(RecoveryFilePath, Validator);
			if (!RecoverySave)
			{
				continue;
			}

			if (OutRecoveryFilePath)
			{
				*OutRecoveryFilePath = RecoveryFilePath;
			}

			if (CopyVerifiedFile(RecoveryFilePath, PrimaryFilePath, Validator))
			{
				return LoadVerifiedSaveFile(PrimaryFilePath, Validator);
			}

			return RecoverySave;
		}

		return nullptr;
	}

	bool SaveGameFileFailClosed(
		USaveGame* SaveGame,
		const FString& PrimaryFilePath,
		const FSaveValidator& Validator)
	{
		if (!SaveGame || !Validator(*SaveGame))
		{
			return false;
		}

		const FString CandidateFilePath = GetCandidateFilePath(PrimaryFilePath);
		const FString PreviousFilePath = GetPreviousFilePath(PrimaryFilePath);
		DeleteFileIfPresent(CandidateFilePath);

		if (FPaths::FileExists(PrimaryFilePath))
		{
			if (!LoadVerifiedSaveFile(PrimaryFilePath, Validator))
			{
				return false;
			}
		}
		else if (FPaths::FileExists(PreviousFilePath))
		{
			return false;
		}

		if (!WriteVerifiedCandidate(SaveGame, CandidateFilePath, Validator))
		{
			return false;
		}

		if (FPaths::FileExists(PrimaryFilePath) &&
			!CopyVerifiedFile(PrimaryFilePath, PreviousFilePath, Validator))
		{
			DeleteFileIfPresent(CandidateFilePath);
			return false;
		}

		if (!IFileManager::Get().Move(
				*PrimaryFilePath,
				*CandidateFilePath,
				true,
				true,
				false,
				true) ||
			!LoadVerifiedSaveFile(PrimaryFilePath, Validator))
		{
			DeleteFileIfPresent(CandidateFilePath);
			if (FPaths::FileExists(PreviousFilePath))
			{
				CopyVerifiedFile(PreviousFilePath, PrimaryFilePath, Validator);
			}
			return false;
		}

		return true;
	}

	bool DeleteSaveArtifacts(const FString& PrimaryFilePath)
	{
		const FString FilePaths[] = {
			PrimaryFilePath,
			GetCandidateFilePath(PrimaryFilePath),
			GetPreviousFilePath(PrimaryFilePath),
			GetCandidateFilePath(GetPreviousFilePath(PrimaryFilePath))
		};

		bool bDeletedAll = true;
		for (const FString& FilePath : FilePaths)
		{
			bDeletedAll = DeleteFileIfPresent(FilePath) && bDeletedAll;
		}
		return bDeletedAll;
	}
}
