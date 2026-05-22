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
