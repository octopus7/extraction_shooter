#include "SogAssetFactory.h"

#include "SogAsset.h"
#include "SogDecoder.h"
#include "SogSplatEditorUtils.h"

USogAssetFactory::USogAssetFactory()
{
	SupportedClass = USogAsset::StaticClass();
	bCreateNew = false;
	bEditorImport = true;
	Formats.Add(TEXT("sog;Spatially Ordered Gaussians"));
}

UObject* USogAssetFactory::FactoryCreateFile(UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, const FString& Filename, const TCHAR* Parms, FFeedbackContext* Warn, bool& bOutOperationCanceled)
{
	bOutOperationCanceled = false;

	USogAsset* SogAsset = NewObject<USogAsset>(InParent, InClass, InName, Flags);
	if (!SogAsset)
	{
		return nullptr;
	}

	FSogDecodeOptions Options;
	FText DecodeError;
	if (!FSogDecoder::DecodeFileToAsset(Filename, Options, SogAsset, DecodeError))
	{
		if (Warn)
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("%s"), *DecodeError.ToString());
		}
		return nullptr;
	}

	if (UMaterialInterface* DefaultMaterial = SogSplatEditorUtils::EnsureDefaultSogMaterial())
	{
		SogAsset->DefaultMaterial = TSoftObjectPtr<UMaterialInterface>(DefaultMaterial);
	}
	else
	{
		SogAsset->DefaultMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(SogSplatEditorUtils::GetDefaultMaterialObjectPath()));
	}

	SogAsset->MarkPackageDirty();
	return SogAsset;
}

bool USogAssetFactory::CanReimport(UObject* Obj, TArray<FString>& OutFilenames)
{
	const USogAsset* SogAsset = Cast<USogAsset>(Obj);
	if (!SogAsset || SogAsset->SourceFilePath.IsEmpty())
	{
		return false;
	}

	OutFilenames.Add(SogAsset->SourceFilePath);
	return true;
}

void USogAssetFactory::SetReimportPaths(UObject* Obj, const TArray<FString>& NewReimportPaths)
{
	USogAsset* SogAsset = Cast<USogAsset>(Obj);
	if (SogAsset && ensure(NewReimportPaths.Num() == 1))
	{
		SogAsset->SourceFilePath = NewReimportPaths[0];
	}
}

EReimportResult::Type USogAssetFactory::Reimport(UObject* Obj)
{
	USogAsset* SogAsset = Cast<USogAsset>(Obj);
	if (!SogAsset || SogAsset->SourceFilePath.IsEmpty())
	{
		return EReimportResult::Failed;
	}

	FText DecodeError;
	if (!FSogDecoder::DecodeFileToAsset(SogAsset->SourceFilePath, SogAsset->DecodeOptions, SogAsset, DecodeError))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to reimport SOG asset %s: %s"), *GetNameSafe(SogAsset), *DecodeError.ToString());
		return EReimportResult::Failed;
	}

	if (UMaterialInterface* DefaultMaterial = SogSplatEditorUtils::EnsureDefaultSogMaterial())
	{
		SogAsset->DefaultMaterial = TSoftObjectPtr<UMaterialInterface>(DefaultMaterial);
	}
	else
	{
		SogAsset->DefaultMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(SogSplatEditorUtils::GetDefaultMaterialObjectPath()));
	}

	SogAsset->MarkPackageDirty();
	return EReimportResult::Succeeded;
}

int32 USogAssetFactory::GetPriority() const
{
	return ImportPriority;
}
