#include "SaveTool/TunaSweeperSaveToolCommandlet.h"
#include "Inventory/TunaSweeperSaveTool.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

UTunaSweeperSaveToolCommandlet::UTunaSweeperSaveToolCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = false;
	ShowErrorCount = false;
}

int32 UTunaSweeperSaveToolCommandlet::Main(const FString& Params)
{
	FString RequestPath, ResponsePath;
	if (!FParse::Value(*Params, TEXT("Request="), RequestPath) || !FParse::Value(*Params, TEXT("Response="), ResponsePath) ||
		FPaths::IsRelative(RequestPath) || FPaths::IsRelative(ResponsePath) || FPaths::GetExtension(ResponsePath) != TEXT("json") ||
		FPaths::IsSamePath(RequestPath, ResponsePath) || FPaths::FileExists(ResponsePath)) return 2;
	FString Input;
	TSharedPtr<FJsonObject> Request;
	const int64 Size = IFileManager::Get().FileSize(*RequestPath);
	if (Size <= 0 || Size > 4 * 1024 * 1024 || !FFileHelper::LoadFileToString(Input, *RequestPath) ||
		!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Input), Request) || !Request) return 2;
	// Reserve a NEW output before any mutation. Never overwrite an existing file.
	TUniquePtr<FArchive> Output(IFileManager::Get().CreateFileWriter(*ResponsePath, FILEWRITE_NoReplaceExisting));
	if (!Output) return 2;
	const auto Response = TunaSweeperSaveTool::Execute(Request);
	FString Json;
	if (!FJsonSerializer::Serialize(Response.ToSharedRef(), TJsonWriterFactory<>::Create(&Json))) return 2;
	FTCHARToUTF8 Utf8(*Json);
	Output->Serialize(const_cast<ANSICHAR*>(Utf8.Get()), Utf8.Length());
	Output->Flush();
	if (Output->IsError() || !Output->Close()) return 2;
	return Response->GetBoolField(TEXT("ok")) ? 0 : 1;
}
