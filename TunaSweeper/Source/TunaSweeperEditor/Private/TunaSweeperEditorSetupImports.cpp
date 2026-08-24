#include "TunaSweeperEditorSetupShared.h"

namespace TunaSweeperEditorSetup
{
	const TArray<FString>& GetItemIconAssetNames()
	{
		static const TArray<FString> IconAssetNames = {
			TEXT("T_UIIcon_Pistol"),
			TEXT("T_UIIcon_Rifle"),
			TEXT("T_UIIcon_Shotgun"),
			TEXT("T_UIIcon_PistolAmmo"),
			TEXT("T_UIIcon_RifleAmmo"),
			TEXT("T_UIIcon_ShotgunAmmo"),
			TEXT("T_UIIcon_RifleExtendedMagazine"),
			TEXT("T_UIIcon_RedDotOptic"),
			TEXT("T_UIIcon_CannedFood"),
			TEXT("T_UIIcon_CannedTuna"),
			TEXT("T_UIIcon_WaterBottle"),
			TEXT("T_UIIcon_EnergyBar"),
			TEXT("T_UIIcon_Bandage"),
			TEXT("T_UIIcon_FirstAidKit"),
			TEXT("T_UIIcon_Painkillers"),
			TEXT("T_UIIcon_Antibiotics"),
			TEXT("T_UIIcon_CombatKnife"),
			TEXT("T_UIIcon_BallisticHelmet"),
			TEXT("T_UIIcon_BodyArmor"),
			TEXT("T_UIIcon_TacticalSunglasses"),
			TEXT("T_UIIcon_TacticalEarphones_Tier1"),
			TEXT("T_UIIcon_TacticalEarphones_Tier2"),
			TEXT("T_UIIcon_TacticalJacket"),
			TEXT("T_UIIcon_Backpack"),
			TEXT("T_UIIcon_Backpack_Tier1"),
			TEXT("T_UIIcon_Backpack_Tier2"),
			TEXT("T_UIIcon_Backpack_Tier3"),
			TEXT("T_UIIcon_Backpack_Tier4"),
			TEXT("T_UIIcon_ValuablesCrate")
		};

		return IconAssetNames;
	}

	const TArray<FString>& GetEquipmentIconAssetNames()
	{
		static const TArray<FString> IconAssetNames = {
			TEXT("T_UIIcon_CombatKnife"),
			TEXT("T_UIIcon_BallisticHelmet"),
			TEXT("T_UIIcon_BodyArmor"),
			TEXT("T_UIIcon_TacticalSunglasses"),
			TEXT("T_UIIcon_TacticalEarphones_Tier1"),
			TEXT("T_UIIcon_TacticalEarphones_Tier2"),
			TEXT("T_UIIcon_TacticalJacket"),
			TEXT("T_UIIcon_RifleExtendedMagazine"),
			TEXT("T_UIIcon_RedDotOptic"),
			TEXT("T_UIIcon_Backpack_Tier1"),
			TEXT("T_UIIcon_Backpack_Tier2"),
			TEXT("T_UIIcon_Backpack_Tier3"),
			TEXT("T_UIIcon_Backpack_Tier4")
		};

		return IconAssetNames;
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

	FString GetWorkspaceFilePath(const FString& RelativePath)
	{
		FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), RelativePath));
		FPaths::CollapseRelativeDirectories(FilePath);
		return FilePath;
	}

	bool EnsureTitleUiTextures()
	{
		UTexture2D* LogoTexture = nullptr;
		UTexture2D* FishTexture = nullptr;

		const bool bLogoImported = ImportUiTexture(
			FUiTextureImportArgs{
				GetWorkspaceFilePath(TEXT("Docs/Story/tuna_sweeper_logo_transparent.png")),
				UITitleTextureAssetPath,
				TitleLogoTextureAssetName,
				true
			},
			&LogoTexture);
		const bool bFishImported = ImportUiTexture(
			FUiTextureImportArgs{
				GetWorkspaceFilePath(TEXT("Docs/Story/tuna_sweeper_fish_transparent.png")),
				UITitleTextureAssetPath,
				TitleFishTextureAssetName,
				true
			},
			&FishTexture);

		return bLogoImported && LogoTexture && bFishImported && FishTexture;
	}

	bool EnsureOpeningScenarioUiTextures()
	{
		UTexture2D* BackgroundTexture = nullptr;
		const bool bBackgroundImported = ImportUiTexture(
			FUiTextureImportArgs{
				GetWorkspaceFilePath(TEXT("chatgpt/opening_light_particles.png")),
				UIStoryTextureAssetPath,
				OpeningScenarioBackgroundTextureAssetName,
				true
			},
			&BackgroundTexture);

		return bBackgroundImported && BackgroundTexture;
	}

	bool EnsureItemIconTextures()
	{
		TArray<FString> FilesToImport;

		for (const FString& IconAssetName : GetItemIconAssetNames())
		{
			const FString ObjectPath = GetAssetObjectPath(UIIconAssetPath, IconAssetName);
			if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath))
			{
				ConfigureImportedIconTexture(ExistingTexture);
				continue;
			}

			const FString SourcePath = GetItemIconSourcePath(IconAssetName);
			if (!FPaths::FileExists(SourcePath))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing item icon source: %s"), *SourcePath);
				return false;
			}

			FilesToImport.Add(SourcePath);
		}

		if (FilesToImport.Num() > 0)
		{
			UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
			ImportData->DestinationPath = UIIconAssetPath;
			ImportData->Filenames = FilesToImport;
			ImportData->bReplaceExisting = false;
			ImportData->bSkipReadOnly = true;

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
			if (ImportedAssets.Num() == 0)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import item icons."));
				return false;
			}
		}

		for (const FString& IconAssetName : GetItemIconAssetNames())
		{
			UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *GetAssetObjectPath(UIIconAssetPath, IconAssetName));
			if (!Texture)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported item icon: %s"), *IconAssetName);
				return false;
			}

			ConfigureImportedIconTexture(Texture);
		}

		return true;
	}

	bool ImportIconTextureSources(const TArray<FString>& IconAssetNames, bool bReplaceExisting)
	{
		TArray<FString> FilesToImport;
		for (const FString& IconAssetName : IconAssetNames)
		{
			const FString SourcePath = GetItemIconSourcePath(IconAssetName);
			if (!FPaths::FileExists(SourcePath))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing icon source: %s"), *SourcePath);
				return false;
			}

			FilesToImport.Add(SourcePath);
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = UIIconAssetPath;
		ImportData->Filenames = FilesToImport;
		ImportData->bReplaceExisting = bReplaceExisting;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import icon textures."));
			return false;
		}

		for (const FString& IconAssetName : IconAssetNames)
		{
			UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *GetAssetObjectPath(UIIconAssetPath, IconAssetName));
			if (!Texture)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load icon texture: %s"), *IconAssetName);
				return false;
			}

			ConfigureImportedIconTexture(Texture);
		}

		return true;
	}

	bool EnsureEquipmentIconTextures()
	{
		return ImportIconTextureSources(GetEquipmentIconAssetNames(), true);
	}

	bool EnsureCannedTunaIconTexture()
	{
		const FString IconAssetName = TEXT("T_UIIcon_CannedTuna");
		if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *GetAssetObjectPath(UIIconAssetPath, IconAssetName)))
		{
			ConfigureImportedIconTexture(ExistingTexture);
			return true;
		}

		const FString SourcePath = GetItemIconSourcePath(IconAssetName);
		if (!FPaths::FileExists(SourcePath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing canned tuna icon source: %s"), *SourcePath);
			return false;
		}

		TArray<FString> FilesToImport;
		FilesToImport.Add(SourcePath);

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = UIIconAssetPath;
		ImportData->Filenames = FilesToImport;
		ImportData->bReplaceExisting = false;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import canned tuna icon."));
			return false;
		}

		UTexture2D* ImportedTexture = LoadObject<UTexture2D>(nullptr, *GetAssetObjectPath(UIIconAssetPath, IconAssetName));
		if (!ImportedTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported canned tuna icon."));
			return false;
		}

		ConfigureImportedIconTexture(ImportedTexture);
		return true;
	}

	bool ImportHudStatusIconTexture(const FString& SourceFileName, const FString& AssetName)
	{
		FUiTextureImportArgs Args;
		Args.SourceFile = GetGeneratedUiImageSourcePath(SourceFileName);
		Args.DestinationPath = UIIconAssetPath;
		Args.AssetName = AssetName;
		Args.bReplaceExisting = true;

		UTexture2D* ImportedTexture = nullptr;
		const bool bImported = ImportUiTexture(Args, &ImportedTexture);
		if (bImported)
		{
			ConfigureImportedIconTexture(ImportedTexture);
		}
		return bImported;
	}

	bool EnsureHudStatusIconTextures()
	{
		return
			ImportHudStatusIconTexture(TEXT("T_UI_Hud_Status_Heart.png"), HudStatusHeartIconAssetName) &&
			ImportHudStatusIconTexture(TEXT("T_UI_Hud_Status_WaterDrop.png"), HudStatusWaterIconAssetName) &&
			ImportHudStatusIconTexture(TEXT("T_UI_Hud_Status_Meat.png"), HudStatusMeatIconAssetName);
	}

}
