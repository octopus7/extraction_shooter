#include "TunaSweeperMapCaptureActorDetails.h"

#include "AssetToolsModule.h"
#include "AutomatedAssetImportData.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "IDetailCustomization.h"
#include "Map/TunaSweeperMapCaptureActor.h"
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

	void ConfigureImportedMapTexture(UTexture2D* Texture)
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

	bool ImportMapTexture(const FString& InSourceFile, const FString& InDestinationPath, const FString& InAssetName)
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

		const FString ObjectPath = GetAssetObjectPath(DestinationPath, AssetName);
		UTexture2D* ImportedTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!ImportedTexture)
		{
			UE_LOG(LogTunaSweeperMapCaptureDetails, Error, TEXT("Failed to load captured map texture: %s"), *ObjectPath);
			return false;
		}

		ConfigureImportedMapTexture(ImportedTexture);
		UE_LOG(LogTunaSweeperMapCaptureDetails, Display, TEXT("Imported captured map texture: %s"), *ObjectPath);
		return true;
	}

	bool ImportLastCapturedMapTexture(ATunaSweeperMapCaptureActor& Actor)
	{
		return ImportMapTexture(
			Actor.GetLastWrittenRgbPngAbsolutePathForEditor(),
			Actor.ResolveImportDestinationPathForEditor(),
			Actor.ResolveImportAssetNameForEditor());
	}

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
