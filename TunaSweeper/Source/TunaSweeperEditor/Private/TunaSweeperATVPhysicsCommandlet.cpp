#include "TunaSweeperATVPhysicsCommandlet.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"

int32 UTunaSweeperATVPhysicsCommandlet::Main(const FString& Params)
{
	const FString PackageName = TEXT("/Game/Meshes/Props/ATV/PA_ATV");
	if (FPackageName::DoesPackageExist(PackageName)) return 0;
	UPackage* Package = CreatePackage(*PackageName);
	auto* Asset = NewObject<UPhysicsAsset>(Package, TEXT("PA_ATV"), RF_Public | RF_Standalone | RF_Transactional);
	auto* Body = NewObject<USkeletalBodySetup>(Asset, NAME_None, RF_Transactional);
	Body->BoneName = TEXT("root");
	Body->PhysicsType = PhysType_Default;
	Body->CollisionTraceFlag = CTF_UseSimpleAsComplex;
	FKBoxElem Box(150.0f, 72.0f, 60.0f);
	Box.Center = FVector(-5, 0, 60);
	Body->AggGeom.BoxElems.Add(Box);
	Asset->SkeletalBodySetups.Add(Body);
	Asset->UpdateBodySetupIndexMap();
	Asset->UpdateBoundsBodiesArray();
	Body->CreatePhysicsMeshes();
	FAssetRegistryModule::AssetCreated(Asset);
	Package->MarkPackageDirty();
	FSavePackageArgs Save;
	Save.TopLevelFlags = RF_Public | RF_Standalone;
	Save.SaveFlags = SAVE_NoError;
	const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	return UPackage::SavePackage(Package, Asset, *Filename, Save) ? 0 : 1;
}
