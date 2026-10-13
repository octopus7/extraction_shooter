#include "Game/TunaSweeperSaveToolInternal.h"
#if WITH_EDITOR
#include "Game/TunaSweeperSafeSave.h"
#include "Game/TunaSweeperGameInstanceShared.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "HAL/CriticalSection.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperSaveTool::Internal
{
    namespace Snapshots
    {
        bool Read(const FString& Path, TArray<uint8>& Bytes)
        {
            const int64 Size = IFileManager::Get().FileSize(*Path);
            return Size > 0 && Size <= 64 * 1024 * 1024 && FFileHelper::LoadFileToArray(Bytes, *Path);
        }

        FString Directory(const FString& Path, const FString& Flavor, int32 Slot)
        {
            // Include the source root so separate accounts/workspaces cannot share snapshots.
            const FTCHARToUTF8 Utf8(*Path.ToLower());
            FSHAHash Hash;
            FSHA1::HashBuffer(Utf8.Get(), Utf8.Length(), Hash.Hash);
            return FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("DeveloperSaveSnapshots") /
                Flavor / FString::Printf(TEXT("Slot%02d"), Slot) / Hash.ToString());
        }

        bool WriteCopy(const FString& Directory, const TArray<uint8>& Bytes, bool bBeforeRestore, FString& OutPath)
        {
            if (!IFileManager::Get().MakeDirectory(*Directory, true)) return false;
            const FString Id = (bBeforeRestore ? TEXT("before-restore-") : TEXT("snapshot-")) +
                FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".sav");
            OutPath = Directory / Id;
            TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*OutPath, FILEWRITE_NoReplaceExisting));
            if (!Writer) return false;
            Writer->Serialize(const_cast<uint8*>(Bytes.GetData()), Bytes.Num());
            Writer->Flush();
            const bool bWritten = !Writer->IsError() && Writer->Close();
            Writer.Reset();
            TArray<uint8> Check;
            if (bWritten && Read(OutPath, Check) && Check == Bytes) return true;
            IFileManager::Get().Delete(*OutPath);
            return false;
        }

        bool ValidId(const FString& Id)
        {
            if (Id.IsEmpty() || Id.Len() > 120 || !Id.EndsWith(TEXT(".sav")) ||
                !(Id.StartsWith(TEXT("snapshot-")) || Id.StartsWith(TEXT("before-restore-")))) return false;
            for (TCHAR C : Id) if (!FChar::IsAlnum(C) && C != TEXT('-') && C != TEXT('.')) return false;
            return FPaths::GetCleanFilename(Id) == Id && !Id.Contains(TEXT(".."));
        }
    }

    FObject ExecuteSnapshot(const FObject& Request)
    {
        const FString Op = Request->GetStringField(TEXT("operation"));
        const bool bRestore = Op == TEXT("snapshot-restore");
        if (Op != TEXT("snapshot-list") && Op != TEXT("snapshot-create") && !bRestore) return Result(TEXT("unknown_operation"));
        if (!Fields(Request, { TEXT("version"), TEXT("operation"), TEXT("savePath"), TEXT("flavor"), TEXT("slot"), TEXT("snapshotId") }) ||
            (!bRestore && Request->HasField(TEXT("snapshotId")))) return Result(TEXT("invalid_request"));
        if (GEngine)
            for (const FWorldContext& Context : GEngine->GetWorldContexts())
                if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) return Result(TEXT("game_running"));
        FString Path, Flavor;
        int32 Slot = 0;
        if (!Request->TryGetStringField(TEXT("savePath"), Path) || Path.IsEmpty() || FPaths::IsRelative(Path) ||
            !Request->TryGetStringField(TEXT("flavor"), Flavor) || (Flavor != TEXT("Demo") && Flavor != TEXT("Main")) ||
            !Integer(Request, TEXT("slot"), Slot, 1, Flavor == TEXT("Demo") ? 1 : 3)) return Result(TEXT("invalid_identity"));
        Path = FPaths::ConvertRelativePathToFull(Path);
        FPaths::NormalizeFilename(Path);
        FPaths::CollapseRelativeDirectories(Path);
        if (FPaths::GetCleanFilename(Path) != FString::Printf(TEXT("TunaSweeperSave_Slot%02d.sav"), Slot)) return Result(TEXT("invalid_identity"));
        const FString Directory = Snapshots::Directory(Path, Flavor, Slot);
        FSystemWideCriticalSection Lock(TEXT("TunaSweeperSaveTool"), FTimespan::Zero());
        if (!Lock.IsValid()) return Result(TEXT("tool_busy"));
        auto Validator = [&](const USaveGame& Object)
        {
            const auto* Save = Cast<UTunaSweeperSaveGame>(&Object);
            return Save && Object.GetClass() == UTunaSweeperSaveGame::StaticClass() &&
                Save->BuildFlavor.ToString() == Flavor && Save->SaveSlotIndex == Slot &&
                Save->SaveVersion >= TunaSweeperSave::MinimumSupportedSaveVersion && Save->SaveVersion <= TunaSweeperSave::CurrentSaveVersion;
        };
        if (Op == TEXT("snapshot-list"))
        {
            TArray<FString> Files;
            IFileManager::Get().FindFiles(Files, *(Directory / TEXT("*.sav")), true, false);
            Files.Sort([&](const FString& A, const FString& B)
            { return IFileManager::Get().GetTimeStamp(*(Directory / A)) > IFileManager::Get().GetTimeStamp(*(Directory / B)); });
            TArray<TSharedPtr<FJsonValue>> Rows;
            for (const FString& Id : Files)
            {
                if (!Snapshots::ValidId(Id)) continue;
                auto Row = MakeShared<FJsonObject>();
                Row->SetStringField(TEXT("snapshotId"), Id);
                const FString Stamp = Id.Mid(Id.StartsWith(TEXT("before-restore-")) ? 15 : 9, 15);
                Row->SetStringField(TEXT("createdAt"), FString::Printf(TEXT("%s-%s-%s %s:%s:%s"),
                    *Stamp.Mid(0, 4), *Stamp.Mid(4, 2), *Stamp.Mid(6, 2), *Stamp.Mid(9, 2), *Stamp.Mid(11, 2), *Stamp.Mid(13, 2)));
                Row->SetBoolField(TEXT("beforeRestore"), Id.StartsWith(TEXT("before-restore-")));
                Rows.Add(MakeShared<FJsonValueObject>(Row));
            }
            auto R = Result(); R->SetArrayField(TEXT("snapshots"), Rows); R->SetStringField(TEXT("directory"), Directory); return R;
        }
        TArray<uint8> Original;
        if (!Snapshots::Read(Path, Original)) return Result(TEXT("save_unreadable"));
        TStrongObjectPtr<USaveGame> Current(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator));
        if (!Current) return Result(TEXT("save_invalid"));
        if (!bRestore)
        {
            FString Copy;
            if (!Snapshots::WriteCopy(Directory, Original, false, Copy)) return Result(TEXT("backup_failed"));
            if (!TunaSweeperSafeSave::LoadVerifiedSaveFile(Copy, Validator)) return Result(TEXT("save_invalid"));
            auto R = Result(); R->SetStringField(TEXT("snapshotId"), FPaths::GetCleanFilename(Copy));
            R->SetStringField(TEXT("snapshotPath"), Copy); return R;
        }
        FString Id;
        if (!Request->TryGetStringField(TEXT("snapshotId"), Id) || !Snapshots::ValidId(Id)) return Result(TEXT("invalid_snapshot"));
        TArray<uint8> SnapshotBytes;
        if (!Snapshots::Read(Directory / Id, SnapshotBytes)) return Result(TEXT("invalid_snapshot"));
        TStrongObjectPtr<USaveGame> Snapshot(TunaSweeperSafeSave::LoadVerifiedSaveFile(Directory / Id, Validator));
        if (!Snapshot) return Result(TEXT("invalid_snapshot"));
        TArray<uint8> Check;
        if (!Snapshots::Read(Path, Check) || Check != Original) return Result(TEXT("stale_save"));
        FString Backup;
        if (!Snapshots::WriteCopy(Directory, Original, true, Backup)) return Result(TEXT("backup_failed"));
        // Preserve every gameplay field and the original save time; do not touch account-wide saves.
        if (!TunaSweeperSafeSave::SaveGameFileFailClosed(Snapshot.Get(), Path, Validator)) return Result(TEXT("save_failed"));
        auto R = Result(); R->SetStringField(TEXT("backupPath"), Backup); return R;
    }
}
#endif
