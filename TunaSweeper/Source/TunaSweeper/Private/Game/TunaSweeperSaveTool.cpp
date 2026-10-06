#include "Inventory/TunaSweeperSaveTool.h"
#if WITH_EDITOR
#include "Game/TunaSweeperSaveToolInternal.h"
#include "Game/TunaSweeperSafeSave.h"
#include "Game/TunaSweeperGameInstanceShared.h"
#include "HAL/CriticalSection.h"
#include "HAL/FileManager.h"
#include "JsonObjectConverter.h"
#include "Misc/SecureHash.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperSaveTool::Internal
{
	FObject Result(const FString& Code)
	{
		auto R = MakeShared<FJsonObject>();
		R->SetNumberField(TEXT("version"), 1);
		R->SetBoolField(TEXT("ok"), Code == TEXT("ok"));
		R->SetStringField(TEXT("code"), Code);
		return R;
	}

	static FString Hash(const TArray<uint8>& Bytes)
	{
		FSHAHash Digest;
		FSHA1::HashBuffer(Bytes.GetData(), Bytes.Num(), Digest.Hash);
		return Digest.ToString();
	}

	static bool ReadBytes(const FString& Path, TArray<uint8>& Bytes)
	{
		const int64 Size = IFileManager::Get().FileSize(*Path);
		return Size > 0 && Size <= 64 * 1024 * 1024 && FFileHelper::LoadFileToArray(Bytes, *Path);
	}

	static FObject Definition(UTunaSweeperItemDataSubsystem& Data, const FTunaSweeperItemDefinition& D)
	{
		auto Row = MakeShared<FJsonObject>();
		Row->SetNumberField(TEXT("itemId"), D.Id);
		Row->SetStringField(TEXT("nameStringKey"), D.NameStringKey.ToString());
		Row->SetStringField(TEXT("category"), D.CategoryTag.ToString());
		Row->SetStringField(TEXT("equipmentSlotTag"), D.EquipmentSlotTag.ToString());
		Row->SetNumberField(TEXT("maxStack"), Data.ResolveItemMaxStackQuantity(D));
		Row->SetNumberField(TEXT("magazineCapacity"), D.MagazineCapacity);
		Row->SetNumberField(TEXT("defense"), D.DefenseValue);
		Row->SetBoolField(TEXT("canAdd"), AllowedItem(D));
		auto Names = MakeShared<FJsonObject>();
		for (const auto& Language : { TPair<const TCHAR*, ETunaSweeperItemTextLanguage>(TEXT("ko"), ETunaSweeperItemTextLanguage::Korean),
			TPair<const TCHAR*, ETunaSweeperItemTextLanguage>(TEXT("en"), ETunaSweeperItemTextLanguage::English),
			TPair<const TCHAR*, ETunaSweeperItemTextLanguage>(TEXT("ja"), ETunaSweeperItemTextLanguage::Japanese) })
		{
			FText Name;
			if (Data.TryGetItemNameText(D.Id, Language.Value, Name)) Names->SetStringField(Language.Key, Name.ToString());
		}
		Row->SetObjectField(TEXT("names"), Names);
		return Row;
	}

	static void Snapshot(FObject R, UTunaSweeperSaveGame& Save, UTunaSweeperItemDataSubsystem& Data)
	{
		auto Json = MakeShared<FJsonObject>();
		// Local save ticks and UTC research ticks remain exact in the JSON protocol.
		FJsonObjectConverter::CustomExportCallback Export;
		Export.BindLambda([](FProperty* Property, const void* Value) -> TSharedPtr<FJsonValue>
		{
			if (const auto* Int = CastField<FInt64Property>(Property)) return MakeShared<FJsonValueString>(LexToString(Int->GetPropertyValue(Value)));
			return nullptr;
		});
		FJsonObjectConverter::UStructToJsonObject(Save.GetClass(), &Save, Json, 0, 0, &Export);
		R->SetObjectField(TEXT("save"), Json);
		R->SetNumberField(TEXT("inventoryCapacity"), Capacity(Save, Data));
		TArray<TSharedPtr<FJsonValue>> Items;
		TSet<int32> Added;
		for (const auto& Item : Save.ItemInstances)
		{
			FTunaSweeperItemDefinition D;
			if (!Added.Contains(Item.ItemId) && Data.TryGetItemDefinition(Item.ItemId, D))
			{
				Added.Add(Item.ItemId);
				Items.Add(MakeShared<FJsonValueObject>(Definition(Data, D)));
			}
		}
		R->SetArrayField(TEXT("itemDefinitions"), Items);
	}

	static bool Metadata(const USaveGame& Object, const FString& Flavor, int32 Slot)
	{
		const auto* Save = Cast<UTunaSweeperSaveGame>(&Object);
		return Save && Save->GetClass() == UTunaSweeperSaveGame::StaticClass() && Save->BuildFlavor.ToString() == Flavor &&
			Save->SaveSlotIndex == Slot && Save->SaveVersion >= TunaSweeperSave::MinimumSupportedSaveVersion &&
			Save->SaveVersion <= TunaSweeperSave::CurrentSaveVersion;
	}
}

TSharedPtr<FJsonObject> TunaSweeperSaveTool::Execute(const TSharedPtr<FJsonObject>& Request)
{
	using namespace Internal;
	int32 Version = 0;
	FString Op;
	if (!Request || !Integer(Request, TEXT("version"), Version, 1, 1) || !Request->TryGetStringField(TEXT("operation"), Op)) return Result(TEXT("invalid_request"));
	const bool bEdit = Op == TEXT("add") || Op == TEXT("preset-apply");
	bool bCommit = false;
	if (Request->HasField(TEXT("commit")) && (!bEdit || !Request->TryGetBoolField(TEXT("commit"), bCommit) || Request->Values[TEXT("commit")]->Type != EJson::Boolean)) return Result(TEXT("invalid_request"));
	const bool bRead = Op == TEXT("inspect") || Op == TEXT("validate") || Op == TEXT("preset-export");
	if (!(bRead || bEdit || Op == TEXT("catalog") || Op == TEXT("list"))) return Result(TEXT("unknown_operation"));
	if (Op == TEXT("catalog") && !Fields(Request, { TEXT("version"), TEXT("operation") })) return Result(TEXT("invalid_request"));
	if (Op == TEXT("list") && !Fields(Request, { TEXT("version"), TEXT("operation"), TEXT("directory") })) return Result(TEXT("invalid_request"));
	if (bRead && !Fields(Request, { TEXT("version"), TEXT("operation"), TEXT("savePath"), TEXT("flavor"), TEXT("slot") })) return Result(TEXT("invalid_request"));
	if (bEdit && !Fields(Request, { TEXT("version"), TEXT("operation"), TEXT("savePath"), TEXT("flavor"), TEXT("slot"), TEXT("commit"), TEXT("expectedHash"), Op == TEXT("add") ? TEXT("items") : TEXT("preset") })) return Result(TEXT("invalid_request"));

	TStrongObjectPtr<UGameInstance> Owner(NewObject<UGameInstance>());
	TStrongObjectPtr<UTunaSweeperItemDataSubsystem> Data(NewObject<UTunaSweeperItemDataSubsystem>(Owner.Get()));
	if (!Data->LoadItemData()) return Result(TEXT("item_data_unavailable"));
	if (Op == TEXT("catalog"))
	{
		auto R = Result();
		TArray<FTunaSweeperItemDefinition> Definitions;
		Data->GetAllItemDefinitions(Definitions);
		Definitions.Sort([](const auto& A, const auto& B) { return A.Id < B.Id; });
		TArray<TSharedPtr<FJsonValue>> Rows;
		for (const auto& D : Definitions) Rows.Add(MakeShared<FJsonValueObject>(Definition(*Data, D)));
		R->SetArrayField(TEXT("items"), Rows);
		return R;
	}
	if (Op == TEXT("list"))
	{
		FString Directory;
		if (!Request->TryGetStringField(TEXT("directory"), Directory) || FPaths::IsRelative(Directory) || !FPaths::DirectoryExists(Directory)) return Result(TEXT("invalid_directory"));
		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(Directory / TEXT("TunaSweeperSave_Slot*.sav")), true, false);
		Files.Sort();
		TArray<TSharedPtr<FJsonValue>> Rows;
		for (const auto& File : Files)
		{
			auto Row = MakeShared<FJsonObject>();
			FString Path = Directory / File;
			Row->SetStringField(TEXT("savePath"), Path);
			TArray<uint8> Bytes;
			TStrongObjectPtr<USaveGame> Object(ReadBytes(Path, Bytes) ? TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, [](const USaveGame& S) { return S.GetClass() == UTunaSweeperSaveGame::StaticClass(); }) : nullptr);
			auto* Save = Cast<UTunaSweeperSaveGame>(Object.Get());
			Row->SetBoolField(TEXT("readable"), Save != nullptr);
			if (Save)
			{
				Row->SetNumberField(TEXT("slot"), Save->SaveSlotIndex);
				Row->SetNumberField(TEXT("saveVersion"), Save->SaveVersion);
				Row->SetStringField(TEXT("flavor"), Save->BuildFlavor.ToString());
				Row->SetStringField(TEXT("hash"), Hash(Bytes));
			}
			Rows.Add(MakeShared<FJsonValueObject>(Row));
		}
		auto R = Result(); R->SetArrayField(TEXT("slots"), Rows); return R;
	}

	FString Path, Flavor;
	int32 Slot = 0;
	if (!Request->TryGetStringField(TEXT("savePath"), Path) || Path.IsEmpty() || FPaths::IsRelative(Path) ||
		!Request->TryGetStringField(TEXT("flavor"), Flavor) || (Flavor != TEXT("Demo") && Flavor != TEXT("Main")) ||
		!Integer(Request, TEXT("slot"), Slot, 1, Flavor == TEXT("Demo") ? 1 : 3)) return Result(TEXT("invalid_identity"));
	Path = FPaths::ConvertRelativePathToFull(Path);
	FPaths::NormalizeFilename(Path);
	if (bEdit && FPaths::GetCleanFilename(Path) != FString::Printf(TEXT("TunaSweeperSave_Slot%02d.sav"), Slot)) return Result(TEXT("invalid_save_filename"));
	FSystemWideCriticalSection Lock(TEXT("TunaSweeperSaveTool"), FTimespan::Zero());
	if (!Lock.IsValid()) return Result(TEXT("tool_busy"));
	TArray<uint8> Original;
	if (!ReadBytes(Path, Original)) return Result(TEXT("save_unreadable"));
	auto Validator = [&](const USaveGame& Object) { return Metadata(Object, Flavor, Slot); };
	TStrongObjectPtr<UTunaSweeperSaveGame> Save(Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator)));
	if (!Save) return Result(TEXT("save_invalid"));
	const FString OriginalHash = Hash(Original);
	FString Expected;
	if (Request->HasField(TEXT("expectedHash")) && (!Request->TryGetStringField(TEXT("expectedHash"), Expected) || !Expected.Equals(OriginalHash, ESearchCase::IgnoreCase))) return Result(TEXT("stale_save"));
	const FString Validation = Validate(*Save, *Data);
	if (Op == TEXT("inspect") || Op == TEXT("validate"))
	{
		auto R = Result(Op == TEXT("validate") && !Validation.IsEmpty() ? Validation : TEXT("ok"));
		Snapshot(R, *Save, *Data);
		R->SetStringField(TEXT("hash"), OriginalHash);
		R->SetStringField(TEXT("validationCode"), Validation.IsEmpty() ? TEXT("ok") : Validation);
		return R;
	}
	if (!Validation.IsEmpty()) return Result(Validation);
	if (Save->SaveVersion != TunaSweeperSave::CurrentSaveVersion) return Result(TEXT("edit_version_unsupported"));
	if (Op == TEXT("preset-export"))
	{
		auto R = Result(); R->SetObjectField(TEXT("preset"), ExportPreset(*Save)); R->SetStringField(TEXT("hash"), OriginalHash); return R;
	}
	FString Error;
	if (Op == TEXT("add")) Error = Add(*Save, *Data, Request);
	else
	{
		const FObject* Preset = nullptr;
		if (!Request->TryGetObjectField(TEXT("preset"), Preset)) return Result(TEXT("invalid_preset"));
		Error = ApplyPreset(*Save, *Data, *Preset);
	}
	if (!Error.IsEmpty()) return Result(Error);
	auto R = Result();
	R->SetStringField(TEXT("sourceHash"), OriginalHash);
	R->SetBoolField(TEXT("committed"), false);
	if (bCommit)
	{
		TArray<uint8> Current;
		if (!ReadBytes(Path, Current) || Current != Original) return Result(TEXT("stale_save"));
		const FString Backup = Path + TEXT(".tool-backup-") + FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S")) + TEXT("-") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
		TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*Backup, FILEWRITE_NoReplaceExisting));
		if (!Writer) return Result(TEXT("backup_failed"));
		Writer->Serialize(Original.GetData(), Original.Num());
		Writer->Flush();
		const bool bWritten = !Writer->IsError() && Writer->Close();
		Writer.Reset();
		TArray<uint8> Check;
		if (!bWritten || !ReadBytes(Backup, Check) || Check != Original) return Result(TEXT("backup_failed"));
		Save->LastSavedAtTicks = FDateTime::Now().GetTicks();
		if (!TunaSweeperSafeSave::SaveGameFileFailClosed(Save.Get(), Path, Validator))
		{
			auto Failed = Result(TEXT("save_failed")); Failed->SetStringField(TEXT("backupPath"), Backup); return Failed;
		}
		TStrongObjectPtr<UTunaSweeperSaveGame> Reload(Cast<UTunaSweeperSaveGame>(TunaSweeperSafeSave::LoadVerifiedSaveFile(Path, Validator)));
		if (!Reload || !Validate(*Reload, *Data).IsEmpty())
		{
			auto Failed = Result(TEXT("post_write_validation_failed")); Failed->SetStringField(TEXT("backupPath"), Backup); return Failed;
		}
		R->SetStringField(TEXT("backupPath"), Backup);
		R->SetBoolField(TEXT("committed"), true);
		if (ReadBytes(Path, Check)) R->SetStringField(TEXT("hash"), Hash(Check));
	}
	Snapshot(R, *Save, *Data);
	return R;
}
#endif
