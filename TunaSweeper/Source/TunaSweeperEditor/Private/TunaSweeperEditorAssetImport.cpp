#include "TunaSweeperEditorAssetImport.h"

#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Sound/SoundWave.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperEditor, Log, All);

namespace TunaSweeperEditorAssetImport
{
	const FString UITitleTextureAssetPath = TEXT("/Game/UI/Title");
	const FString AudioBgmAssetPath = TEXT("/Game/Audio/BGM");

	struct FUiTextureImportArgs
	{
		FString SourceFile;
		FString DestinationPath = UITitleTextureAssetPath;
		FString AssetName;
		bool bReplaceExisting = true;
	};

	struct FAudioImportArgs
	{
		FString SourceFile;
		FString DestinationPath = AudioBgmAssetPath;
		FString AssetName = TEXT("Where_the_Birds_Still_Sing");
		bool bReplaceExisting = true;
		bool bLooping = true;
	};

	FString GetAssetObjectPath(const FString& AssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *AssetName, *AssetName);
	}

	bool SaveAsset(UObject* Asset)
	{
		if (!Asset)
		{
			return false;
		}

		UPackage* Package = Asset->GetOutermost();
		if (!Package)
		{
			return false;
		}

		const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		return UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	}

	void ConfigureImportedUiTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->MipGenSettings = TMGS_NoMipmaps;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->SRGB = true;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
	}

	bool ImportUiTexture(const FUiTextureImportArgs& Args, UTexture2D** OutTexture = nullptr)
	{
		if (OutTexture)
		{
			*OutTexture = nullptr;
		}

		FString SourceFile = Args.SourceFile;
		FPaths::NormalizeFilename(SourceFile);
		SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
		FPaths::CollapseRelativeDirectories(SourceFile);

		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing UI texture source: %s"), *SourceFile);
			return false;
		}

		const FString DestinationPath = Args.DestinationPath.IsEmpty() ? UITitleTextureAssetPath : Args.DestinationPath;
		FString AssetName = Args.AssetName.IsEmpty() ? FPaths::GetBaseFilename(SourceFile) : Args.AssetName;
		AssetName = FPaths::GetBaseFilename(AssetName);

		if (AssetName.IsEmpty() || !FPackageName::IsValidLongPackageName(DestinationPath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Invalid UI texture destination: path=%s asset=%s"), *DestinationPath, *AssetName);
			return false;
		}

		const FString ObjectPath = GetAssetObjectPath(DestinationPath, AssetName);
		if (!Args.bReplaceExisting)
		{
			if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath))
			{
				ConfigureImportedUiTexture(ExistingTexture);
				if (OutTexture)
				{
					*OutTexture = ExistingTexture;
				}
				return true;
			}
		}

		FString ImportFile = SourceFile;
		if (FPaths::GetBaseFilename(SourceFile) != AssetName)
		{
			const FString ImportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TunaSweeperUiTextureImport"));
			IFileManager::Get().MakeDirectory(*ImportDirectory, true);
			ImportFile = FPaths::Combine(ImportDirectory, AssetName + TEXT(".") + FPaths::GetExtension(SourceFile));
			if (IFileManager::Get().Copy(*ImportFile, *SourceFile, true, true) != COPY_OK)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to stage UI texture import source: %s -> %s"), *SourceFile, *ImportFile);
				return false;
			}
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(ImportFile);
		ImportData->bReplaceExisting = Args.bReplaceExisting;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import UI texture: %s"), *ImportFile);
			return false;
		}

		UTexture2D* ImportedTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!ImportedTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported UI texture: %s"), *ObjectPath);
			return false;
		}

		ConfigureImportedUiTexture(ImportedTexture);
		if (OutTexture)
		{
			*OutTexture = ImportedTexture;
		}
		return true;
	}

	bool TryReadUiTextureImportArgsFromCommandLine(FUiTextureImportArgs& OutArgs)
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureSource="), SourceFile))
		{
			return false;
		}

		OutArgs.SourceFile = SourceFile;
		FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureDest="), OutArgs.DestinationPath);
		FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureName="), OutArgs.AssetName);
		OutArgs.bReplaceExisting = !FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureNoReplace"));
		return true;
	}

	bool ImportUiTextureFromCommandLineIfRequested()
	{
		FUiTextureImportArgs Args;
		if (!TryReadUiTextureImportArgsFromCommandLine(Args))
		{
			return false;
		}

		const bool bImported = ImportUiTexture(Args);
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureQuit")))
		{
			FPlatformMisc::RequestExit(false);
		}
		return bImported;
	}

	void ConfigureImportedBgmSound(USoundWave* SoundWave, bool bLooping)
	{
		if (!SoundWave)
		{
			return;
		}

		SoundWave->Modify();
		SoundWave->bLooping = bLooping;
		SoundWave->PostEditChange();
		SoundWave->MarkPackageDirty();
		SaveAsset(SoundWave);
	}

	bool ImportAudioAsset(const FAudioImportArgs& Args, USoundWave** OutSoundWave = nullptr)
	{
		if (OutSoundWave)
		{
			*OutSoundWave = nullptr;
		}

		FString SourceFile = Args.SourceFile;
		FPaths::NormalizeFilename(SourceFile);
		SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
		FPaths::CollapseRelativeDirectories(SourceFile);

		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing audio source: %s"), *SourceFile);
			return false;
		}

		const FString DestinationPath = Args.DestinationPath.IsEmpty() ? AudioBgmAssetPath : Args.DestinationPath;
		FString AssetName = Args.AssetName.IsEmpty() ? FPaths::GetBaseFilename(SourceFile) : Args.AssetName;
		AssetName = FPaths::GetBaseFilename(AssetName);

		if (AssetName.IsEmpty() || !FPackageName::IsValidLongPackageName(DestinationPath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Invalid audio destination: path=%s asset=%s"), *DestinationPath, *AssetName);
			return false;
		}

		const FString ObjectPath = GetAssetObjectPath(DestinationPath, AssetName);
		if (!Args.bReplaceExisting)
		{
			if (USoundWave* ExistingSoundWave = LoadObject<USoundWave>(nullptr, *ObjectPath))
			{
				ConfigureImportedBgmSound(ExistingSoundWave, Args.bLooping);
				if (OutSoundWave)
				{
					*OutSoundWave = ExistingSoundWave;
				}
				return true;
			}
		}

		FString ImportFile = SourceFile;
		if (FPaths::GetBaseFilename(SourceFile) != AssetName)
		{
			const FString ImportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TunaSweeperAudioImport"));
			IFileManager::Get().MakeDirectory(*ImportDirectory, true);
			ImportFile = FPaths::Combine(ImportDirectory, AssetName + TEXT(".") + FPaths::GetExtension(SourceFile));
			if (IFileManager::Get().Copy(*ImportFile, *SourceFile, true, true) != COPY_OK)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to stage audio import source: %s -> %s"), *SourceFile, *ImportFile);
				return false;
			}
		}

		FModuleManager::Get().LoadModule(TEXT("AudioEditor"));

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(ImportFile);
		ImportData->bReplaceExisting = Args.bReplaceExisting;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import audio: %s"), *ImportFile);
			return false;
		}

		USoundWave* ImportedSoundWave = LoadObject<USoundWave>(nullptr, *ObjectPath);
		if (!ImportedSoundWave)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported audio: %s"), *ObjectPath);
			return false;
		}

		ConfigureImportedBgmSound(ImportedSoundWave, Args.bLooping);
		if (OutSoundWave)
		{
			*OutSoundWave = ImportedSoundWave;
		}
		return true;
	}

	bool TryReadAudioImportArgsFromCommandLine(FAudioImportArgs& OutArgs)
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportAudioSource="), SourceFile))
		{
			return false;
		}

		OutArgs.SourceFile = SourceFile;
		FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportAudioDest="), OutArgs.DestinationPath);
		FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportAudioName="), OutArgs.AssetName);
		OutArgs.bReplaceExisting = !FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportAudioNoReplace"));
		OutArgs.bLooping = !FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportAudioNoLoop"));
		return true;
	}

	bool ImportAudioFromCommandLineIfRequested()
	{
		FAudioImportArgs Args;
		if (!TryReadAudioImportArgsFromCommandLine(Args))
		{
			return false;
		}

		const bool bImported = ImportAudioAsset(Args);
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportAudioQuit")))
		{
			FPlatformMisc::RequestExit(true);
		}
		return bImported;
	}
}
