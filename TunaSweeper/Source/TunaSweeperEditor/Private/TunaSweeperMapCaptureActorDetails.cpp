#include "TunaSweeperMapCaptureActorDetails.h"

#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AutomatedAssetImportData.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "IDetailCustomization.h"
#include "Map/TunaSweeperMapCaptureActor.h"
#include "Map/TunaSweeperMapDefinition.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ObjectTools.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "UObject/SavePackage.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "TunaSweeperMapCaptureActorDetails"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperMapCaptureDetails, Log, All);

namespace
{
	const FName MapCaptureActorClassName(TEXT("TunaSweeperMapCaptureActor"));
	const FString DefaultMapTextureDestinationPath = TEXT("/Game/UI/Map");
	const FString MapRegistryAssetName = TEXT("DA_UIMapRegistry");

	enum class EMapCaptureDetailAction : uint8
	{
		AutoDetectBounds,
		CaptureRgbPng,
		AutoDetectAndCapture,
		CaptureAndImport,
		AutoDetectCaptureAndImport
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

		const FString PackageFileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		return UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	}

	bool ConfigureImportedMapTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return false;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_BC7;
		Texture->MipGenSettings = TMGS_SimpleAverage;
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->SRGB = true;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->Filter = TF_Bilinear;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		return SaveAsset(Texture);
	}

	template <typename AssetType>
	AssetType* CreateOrLoadDataAsset(const FString& AssetPath, const FString& AssetName)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		if (AssetType* ExistingAsset = LoadObject<AssetType>(nullptr, *ObjectPath))
		{
			return ExistingAsset;
		}

		const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *AssetName);
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}

		AssetType* NewAsset = NewObject<AssetType>(
			Package,
			*AssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (NewAsset)
		{
			FAssetRegistryModule::AssetCreated(NewAsset);
		}
		return NewAsset;
	}

	bool SaveMapMetadata(ATunaSweeperMapCaptureActor& Actor, UTexture2D* Texture, const FString& DestinationPath)
	{
		UWorld* World = Actor.GetWorld();
		if (!World || !Texture)
		{
			return false;
		}

		const FString LevelName = FPackageName::GetShortName(World->GetOutermost()->GetName());
		const FString DefinitionAssetName = FString::Printf(TEXT("DA_UIMap_%s"), *LevelName);
		UTunaSweeperMapDefinition* Definition =
			CreateOrLoadDataAsset<UTunaSweeperMapDefinition>(DestinationPath, DefinitionAssetName);
		UTunaSweeperMapRegistry* Registry =
			CreateOrLoadDataAsset<UTunaSweeperMapRegistry>(DefaultMapTextureDestinationPath, MapRegistryAssetName);
		if (!Definition || !Registry)
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Failed to create map metadata assets for %s."), *LevelName);
			return false;
		}

		Definition->Modify();
		Definition->DefinitionVersion = UTunaSweeperMapDefinition::CurrentDefinitionVersion;
		Definition->MapId = FName(*LevelName);
		Definition->World = TSoftObjectPtr<UWorld>(World);
		Definition->Texture = TSoftObjectPtr<UTexture2D>(Texture);
		Definition->CaptureCenter = Actor.GetActorLocation();
		Definition->CaptureWorldSize = Actor.GetCaptureWorldSizeForEditor();
		Definition->CaptureYawDegrees = Actor.GetActorRotation().Yaw;
		Definition->TextureSize = FIntPoint(
			UTunaSweeperMapDefinition::FixedTextureResolution,
			UTunaSweeperMapDefinition::FixedTextureResolution);
		Definition->ContentPixelMin = Actor.GetLastContentPixelMinForEditor();
		Definition->ContentPixelSize = Actor.GetLastContentPixelSizeForEditor();
		Definition->MarkPackageDirty();

		if (!Definition->IsValidDefinition())
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Generated invalid map metadata for %s."), *LevelName);
			return false;
		}

		Registry->Modify();
		Registry->Definitions.RemoveAll([Definition](const UTunaSweeperMapDefinition* Entry)
		{
			return !Entry || (Entry != Definition && Entry->MapId == Definition->MapId);
		});
		Registry->Definitions.AddUnique(Definition);
		Registry->MarkPackageDirty();

		if (!SaveAsset(Definition) || !SaveAsset(Registry))
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Failed to save map metadata for %s."), *LevelName);
			return false;
		}

		UE_LOG(
			LogTunaSweeperMapCaptureDetails,
			Display,
			TEXT("Saved map metadata: %s/%s (content min=%s size=%s)."),
			*DestinationPath,
			*DefinitionAssetName,
			*Definition->ContentPixelMin.ToString(),
			*Definition->ContentPixelSize.ToString());
		return true;
	}

	bool ImportMapTexture(
		ATunaSweeperMapCaptureActor& Actor,
		const FString& InSourceFile,
		const FString& InDestinationPath,
		const FString& InAssetName)
	{
		FString SourceFile = InSourceFile;
		FPaths::NormalizeFilename(SourceFile);
		SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
		FPaths::CollapseRelativeDirectories(SourceFile);

		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Missing captured map PNG: %s"), *SourceFile);
			return false;
		}

		FString DestinationPath = InDestinationPath.IsEmpty() ? DefaultMapTextureDestinationPath : InDestinationPath;
		DestinationPath.RemoveFromEnd(TEXT("/"));

		FString AssetName = ObjectTools::SanitizeObjectName(InAssetName);
		AssetName = FPaths::GetBaseFilename(AssetName);

		if (AssetName.IsEmpty() || !FPackageName::IsValidLongPackageName(DestinationPath))
		{
			UE_LOG(
				LogTunaSweeperMapCaptureDetails,
				Error,
				TEXT("Invalid captured map import destination: path=%s asset=%s"),
				*DestinationPath,
				*AssetName);
			return false;
		}

		FString ImportFile = SourceFile;
		if (FPaths::GetBaseFilename(SourceFile) != AssetName)
		{
			const FString ImportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TunaSweeperMapTextureImport"));
			IFileManager::Get().MakeDirectory(*ImportDirectory, true);

			FString Extension = FPaths::GetExtension(SourceFile);
			if (Extension.IsEmpty())
			{
				Extension = TEXT("png");
			}

			ImportFile = FPaths::Combine(ImportDirectory, AssetName + TEXT(".") + Extension);
			if (IFileManager::Get().Copy(*ImportFile, *SourceFile, true, true) != COPY_OK)
			{
				UE_LOG(
					LogTunaSweeperMapCaptureDetails,
					Error,
					TEXT("Failed to stage captured map import source: %s -> %s"),
					*SourceFile,
					*ImportFile);
				return false;
			}
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(ImportFile);
		ImportData->bReplaceExisting = true;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Failed to import captured map texture: %s"), *ImportFile);
			return false;
		}

		UTexture2D* ImportedTexture = nullptr;
		for (UObject* ImportedAsset : ImportedAssets)
		{
			if (UTexture2D* ImportedTextureCandidate = Cast<UTexture2D>(ImportedAsset))
			{
				ImportedTexture = ImportedTextureCandidate;
				break;
			}
		}

		const FString ObjectPath = GetAssetObjectPath(DestinationPath, AssetName);
		if (!ImportedTexture)
		{
			ImportedTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		}
		if (!ImportedTexture)
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Failed to load captured map texture: %s"), *ObjectPath);
			return false;
		}

		if (ImportedTexture->GetSizeX() != UTunaSweeperMapDefinition::FixedTextureResolution ||
			ImportedTexture->GetSizeY() != UTunaSweeperMapDefinition::FixedTextureResolution)
		{
			UE_LOG(
				LogTunaSweeperMapCaptureDetails,
				Error,
				TEXT("Captured map texture is not 2048x2048: %s (%dx%d)"),
				*ObjectPath,
				ImportedTexture->GetSizeX(),
				ImportedTexture->GetSizeY());
			return false;
		}

		if (!ConfigureImportedMapTexture(ImportedTexture))
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Failed to save captured map texture: %s"), *ObjectPath);
			return false;
		}

		if (!SaveMapMetadata(Actor, ImportedTexture, DestinationPath))
		{
			return false;
		}

		UE_LOG(LogTunaSweeperMapCaptureDetails, Display, TEXT("Imported captured map texture: %s"), *ObjectPath);
		return true;
	}

	bool ImportLastCapturedMapTexture(ATunaSweeperMapCaptureActor& Actor)
	{
		return ImportMapTexture(
			Actor,
			Actor.GetLastWrittenRgbPngAbsolutePathForEditor(),
			Actor.ResolveImportDestinationPathForEditor(),
			Actor.ResolveImportAssetNameForEditor());
	}

	void GenerateMapAssetsFromConsole(const TArray<FString>& Args, UWorld* World)
	{
		if (Args.Num() != 5)
		{
			UE_LOG(
				LogTunaSweeperMapCaptureDetails,
				Error,
				TEXT("Usage: TunaSweeper.MapCapture.Generate CenterX CenterY SizeX SizeY Yaw"));
			return;
		}

		if (!World)
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Map capture console command has no editor world."));
			return;
		}

		ATunaSweeperMapCaptureActor* CaptureActor = nullptr;
		for (TActorIterator<ATunaSweeperMapCaptureActor> It(World); It; ++It)
		{
			CaptureActor = *It;
			break;
		}

		if (!CaptureActor)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.ObjectFlags |= RF_Transient;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			CaptureActor = World->SpawnActor<ATunaSweeperMapCaptureActor>(
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParameters);
		}

		if (!CaptureActor)
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Failed to create map capture actor."));
			return;
		}

		const FVector Center(
			FCString::Atod(*Args[0]),
			FCString::Atod(*Args[1]),
			CaptureActor->GetActorLocation().Z);
		const FVector2D WorldSize(
			FCString::Atod(*Args[2]),
			FCString::Atod(*Args[3]));
		const float YawDegrees = FCString::Atof(*Args[4]);
		CaptureActor->ConfigureCaptureBoundsForEditor(Center, WorldSize, YawDegrees);

		const bool bSucceeded =
			CaptureActor->RunCaptureOpaqueRgbPngForEditor() &&
			ImportLastCapturedMapTexture(*CaptureActor);
		if (bSucceeded)
		{
			UE_LOG(
				LogTunaSweeperMapCaptureDetails,
				Display,
				TEXT("Map capture console generation succeeded for %s."),
				*World->GetOutermost()->GetName());
		}
		else
		{
			UE_LOG(
				LogTunaSweeperMapCaptureDetails,
				Error,
				TEXT("Map capture console generation failed for %s."),
				*World->GetOutermost()->GetName());
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperMapCaptureQuit")))
		{
			FPlatformMisc::RequestExitWithStatus(
				false,
				bSucceeded ? 0 : 1,
				TEXT("TunaSweeperMapCaptureGenerate"));
		}
	}

	FAutoConsoleCommandWithWorldAndArgs GenerateMapAssetsConsoleCommand(
		TEXT("TunaSweeper.MapCapture.Generate"),
		TEXT("Generate the current world's fixed 2048 map texture and metadata. Args: CenterX CenterY SizeX SizeY Yaw"),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GenerateMapAssetsFromConsole));

	FText GetActionTransactionText(EMapCaptureDetailAction Action)
	{
		switch (Action)
		{
		case EMapCaptureDetailAction::AutoDetectBounds:
			return LOCTEXT("AutoDetectBoundsTransaction", "Auto Detect Map Capture Bounds");
		case EMapCaptureDetailAction::CaptureRgbPng:
			return LOCTEXT("CaptureRgbPngTransaction", "Capture Map RGB PNG");
		case EMapCaptureDetailAction::AutoDetectAndCapture:
			return LOCTEXT("AutoDetectAndCaptureTransaction", "Auto Detect and Capture Map RGB PNG");
		case EMapCaptureDetailAction::CaptureAndImport:
			return LOCTEXT("CaptureAndImportTransaction", "Capture and Import Map Texture");
		case EMapCaptureDetailAction::AutoDetectCaptureAndImport:
			return LOCTEXT("AutoDetectCaptureAndImportTransaction", "Auto Detect, Capture, and Import Map Texture");
		default:
			return LOCTEXT("MapCaptureActionTransaction", "Run Map Capture Action");
		}
	}

	void ExecuteAction(
		const TArray<TWeakObjectPtr<ATunaSweeperMapCaptureActor>>& Actors,
		EMapCaptureDetailAction Action)
	{
		bool bHasValidActor = false;
		for (const TWeakObjectPtr<ATunaSweeperMapCaptureActor>& ActorPtr : Actors)
		{
			if (ActorPtr.IsValid())
			{
				bHasValidActor = true;
				break;
			}
		}

		if (!bHasValidActor)
		{
			return;
		}

		const FScopedTransaction Transaction(GetActionTransactionText(Action));
		for (const TWeakObjectPtr<ATunaSweeperMapCaptureActor>& ActorPtr : Actors)
		{
			ATunaSweeperMapCaptureActor* Actor = ActorPtr.Get();
			if (!Actor)
			{
				continue;
			}

			Actor->Modify();
			switch (Action)
			{
			case EMapCaptureDetailAction::AutoDetectBounds:
				Actor->AutoDetectCaptureBounds();
				break;
			case EMapCaptureDetailAction::CaptureRgbPng:
				Actor->CaptureOpaqueRgbPng();
				break;
			case EMapCaptureDetailAction::AutoDetectAndCapture:
				Actor->AutoDetectBoundsAndCaptureOpaqueRgbPng();
				break;
			case EMapCaptureDetailAction::CaptureAndImport:
				if (Actor->RunCaptureOpaqueRgbPngForEditor())
				{
					ImportLastCapturedMapTexture(*Actor);
				}
				break;
			case EMapCaptureDetailAction::AutoDetectCaptureAndImport:
				if (Actor->RunAutoDetectBoundsAndCaptureOpaqueRgbPngForEditor())
				{
					ImportLastCapturedMapTexture(*Actor);
				}
				break;
			default:
				break;
			}
		}

		if (GEditor)
		{
			GEditor->RedrawLevelEditingViewports(true);
		}
	}

	TSharedRef<SWidget> MakeActionButton(
		TArray<TWeakObjectPtr<ATunaSweeperMapCaptureActor>> Actors,
		EMapCaptureDetailAction Action,
		const FText& ButtonText,
		const FText& TooltipText)
	{
		return SNew(SButton)
			.Text(ButtonText)
			.ToolTipText(TooltipText)
			.OnClicked_Lambda(
				[Actors = MoveTemp(Actors), Action]()
				{
					ExecuteAction(Actors, Action);
					return FReply::Handled();
				});
	}

	class FTunaSweeperMapCaptureActorDetails final : public IDetailCustomization
	{
	public:
		static TSharedRef<IDetailCustomization> MakeInstance()
		{
			return MakeShared<FTunaSweeperMapCaptureActorDetails>();
		}

		virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override
		{
			TArray<TWeakObjectPtr<ATunaSweeperMapCaptureActor>> Actors;
			TArray<TWeakObjectPtr<UObject>> Objects;
			DetailBuilder.GetObjectsBeingCustomized(Objects);

			for (const TWeakObjectPtr<UObject>& ObjectPtr : Objects)
			{
				if (ATunaSweeperMapCaptureActor* Actor = Cast<ATunaSweeperMapCaptureActor>(ObjectPtr.Get()))
				{
					Actors.Add(Actor);
				}
			}

			IDetailCategoryBuilder& Category = DetailBuilder.EditCategory(
				TEXT("Map Capture"),
				LOCTEXT("MapCaptureCategory", "Map Capture"),
				ECategoryPriority::Important);

			Category.AddCustomRow(LOCTEXT("MapCaptureActionsFilter", "Map Capture Actions Auto Detect Capture RGB PNG"))
				.WholeRowContent()
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 2.0f, 0.0f, 4.0f)
					[
						SNew(STextBlock)
						.Font(IDetailLayoutBuilder::GetDetailFont())
						.Text(LOCTEXT("MapCaptureActionsHelp", "Generate the editor-only map image for this level, or import it into /Game UI content."))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							MakeActionButton(
								Actors,
								EMapCaptureDetailAction::AutoDetectBounds,
								LOCTEXT("AutoDetectBoundsButton", "Auto Detect Bounds"),
								LOCTEXT("AutoDetectBoundsTooltip", "Detect capture bounds from level geometry using downward traces."))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							MakeActionButton(
								Actors,
								EMapCaptureDetailAction::CaptureRgbPng,
								LOCTEXT("CaptureRgbPngButton", "Capture RGB PNG"),
								LOCTEXT("CaptureRgbPngTooltip", "Capture the current bounds to an opaque RGB PNG."))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							MakeActionButton(
								Actors,
								EMapCaptureDetailAction::AutoDetectAndCapture,
								LOCTEXT("AutoDetectAndCaptureButton", "Auto Detect + Capture"),
								LOCTEXT("AutoDetectAndCaptureTooltip", "Detect bounds, then immediately capture the opaque RGB PNG."))
						]
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.Padding(0.0f, 6.0f, 0.0f, 0.0f)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding(0.0f, 0.0f, 6.0f, 0.0f)
						[
							MakeActionButton(
								Actors,
								EMapCaptureDetailAction::CaptureAndImport,
								LOCTEXT("CaptureAndImportButton", "Capture + Import"),
								LOCTEXT("CaptureAndImportTooltip", "Capture the current bounds, then import or overwrite the current level map texture asset."))
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							MakeActionButton(
								Actors,
								EMapCaptureDetailAction::AutoDetectCaptureAndImport,
								LOCTEXT("AutoDetectCaptureAndImportButton", "Auto Detect + Capture + Import"),
								LOCTEXT("AutoDetectCaptureAndImportTooltip", "Detect bounds, capture the RGB PNG, then import or overwrite the current level map texture asset."))
						]
					]
				];
		}
	};
}

namespace TunaSweeperMapCaptureActorDetails
{
	void Register()
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.RegisterCustomClassLayout(
			MapCaptureActorClassName,
			FOnGetDetailCustomizationInstance::CreateStatic(&FTunaSweeperMapCaptureActorDetails::MakeInstance));
		PropertyModule.NotifyCustomizationModuleChanged();
	}

	void Unregister()
	{
		if (!FModuleManager::Get().IsModuleLoaded(TEXT("PropertyEditor")))
		{
			return;
		}

		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
		PropertyModule.UnregisterCustomClassLayout(MapCaptureActorClassName);
		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

#undef LOCTEXT_NAMESPACE
