#include "SogActorFactory.h"

#include "AssetRegistry/AssetData.h"
#include "Materials/MaterialInterface.h"
#include "SogAsset.h"
#include "SogSplatActor.h"
#include "SogSplatComponent.h"
#include "SogSplatEditorUtils.h"

#define LOCTEXT_NAMESPACE "SogActorFactory"

USogActorFactory::USogActorFactory()
{
	DisplayName = LOCTEXT("SogSplatDisplayName", "SOG Splat");
	NewActorClass = ASogSplatActor::StaticClass();
	bUseSurfaceOrientation = false;
}

bool USogActorFactory::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	const bool bIsSogAsset = AssetData.IsValid()
		&& (AssetData.AssetClassPath == USogAsset::StaticClass()->GetClassPathName()
			|| AssetData.IsInstanceOf(USogAsset::StaticClass(), EResolveClass::Yes));

	if (!bIsSogAsset)
	{
		OutErrorMsg = LOCTEXT("NoSogAsset", "A valid SOG asset must be specified.");
		return false;
	}

	return true;
}

void USogActorFactory::PostSpawnActor(UObject* Asset, AActor* NewActor)
{
	Super::PostSpawnActor(Asset, NewActor);

	USogAsset* SogAsset = Cast<USogAsset>(Asset);
	ASogSplatActor* SogActor = Cast<ASogSplatActor>(NewActor);
	if (!SogAsset || !SogActor)
	{
		return;
	}

	if (SogAsset->DefaultMaterial.IsNull())
	{
		if (UMaterialInterface* DefaultMaterial = SogSplatEditorUtils::EnsureDefaultSogMaterial())
		{
			SogAsset->Modify();
			SogAsset->DefaultMaterial = TSoftObjectPtr<UMaterialInterface>(DefaultMaterial);
			SogAsset->MarkPackageDirty();
		}
	}

	if (USogSplatComponent* SogComponent = SogActor->GetSogSplatComponent())
	{
		SogComponent->SetSogAsset(SogAsset);
	}
}

UObject* USogActorFactory::GetAssetFromActorInstance(AActor* ActorInstance)
{
	ASogSplatActor* SogActor = Cast<ASogSplatActor>(ActorInstance);
	if (!SogActor)
	{
		return nullptr;
	}

	USogSplatComponent* SogComponent = SogActor->GetSogSplatComponent();
	return SogComponent ? SogComponent->SourceAsset : nullptr;
}

#undef LOCTEXT_NAMESPACE
