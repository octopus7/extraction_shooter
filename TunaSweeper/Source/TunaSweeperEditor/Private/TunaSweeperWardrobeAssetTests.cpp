#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperOutfitCatalog.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperOutfitComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Interaction/TunaSweeperWardrobeActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "PhysicsEngine/BodySetup.h"
#include "RenderingThread.h"
#include "ShaderCompiler.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperWardrobeAssetTest,
	"TunaSweeper.Wardrobe.Assets", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperWardrobeAssetTest::RunTest(const FString&)
{
	auto* Catalog = LoadObject<UTunaSweeperOutfitCatalog>(nullptr,
		TEXT("/Game/Characters/Player/LunaMk2/Outfits/DA_LunaMk2_Outfits.DA_LunaMk2_Outfits"));
	if (!TestNotNull(TEXT("Outfit catalog loads"), Catalog)) return false;
	TestEqual(TEXT("All six outfits are listed"), Catalog->Outfits.Num(), 6);
	auto* Original = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2.SKM_LunaMk2"));
	if (!TestNotNull(TEXT("Original character remains available"), Original)) return false;
	TSet<FName> Seen;
	for (const FTunaSweeperOutfitDefinition& Outfit : Catalog->Outfits)
	{
		TestFalse(TEXT("Outfit IDs are unique"), Seen.Contains(Outfit.OutfitId));
		Seen.Add(Outfit.OutfitId);
		TestTrue(TEXT("Outfit ID is supported"), TunaSweeperOutfits::IsSupportedOutfitId(Outfit.OutfitId));
		TestFalse(TEXT("Display name uses a string key"), Outfit.DisplayNameStringKey.IsNone());
		auto* Texture = Outfit.Thumbnail.LoadSynchronous();
		if (TestNotNull(TEXT("Outfit has an imported UI image"), Texture))
		{
			TestEqual(TEXT("Imported image width"), Texture->Source.GetSizeX(), int64(1024));
			TestEqual(TEXT("Imported image height"), Texture->Source.GetSizeY(), int64(1536));
			TestEqual(TEXT("UI texture group"), Texture->LODGroup, TEXTUREGROUP_UI);
			TestFalse(TEXT("Transparent alpha is retained"), Texture->CompressionNoAlpha);
			TArray64<uint8> SourcePixels;
			if (TestTrue(TEXT("Imported RGBA source pixels are readable"),
				Texture->Source.GetFormat() == TSF_BGRA8 && Texture->Source.GetMipData(SourcePixels, 0)))
			{
				bool bTransparent = false, bOpaque = false;
				for (int64 Pixel = 3; Pixel < SourcePixels.Num(); Pixel += 4)
				{
					bTransparent |= SourcePixels[Pixel] == 0;
					bOpaque |= SourcePixels[Pixel] > 200;
				}
				TestTrue(TEXT("Thumbnail retains both transparent background and visible subject"), bTransparent && bOpaque);
			}
		}
		if (Outfit.OutfitId == TEXT("Maid")) continue;
		for (USkeletalMesh* Mesh : {Outfit.BodyMesh.LoadSynchronous(), Outfit.ClothingMesh.LoadSynchronous()})
		{
			if (!TestNotNull(TEXT("Both the masked body and clothing are present"), Mesh)) continue;
			TestEqual(TEXT("Original skeleton is reused"), Mesh->GetSkeleton(), Original->GetSkeleton());
			const FReferenceSkeleton& Ref = Mesh->GetRefSkeleton();
			const FReferenceSkeleton& Expected = Original->GetRefSkeleton();
			if (!TestEqual(TEXT("All 130 skeleton bones are kept"), Ref.GetRawBoneNum(), Expected.GetRawBoneNum())) continue;
			for (int32 Bone = 0; Bone < Ref.GetRawBoneNum(); ++Bone)
			{
				TestEqual(TEXT("Bone order is compatible"), Ref.GetBoneName(Bone), Expected.GetBoneName(Bone));
				TestEqual(TEXT("Bone hierarchy is compatible"), Ref.GetParentIndex(Bone), Expected.GetParentIndex(Bone));
				TestTrue(TEXT("Rest translation matches within 0.02 cm"),
					Ref.GetRefBonePose()[Bone].GetTranslation().Equals(Expected.GetRefBonePose()[Bone].GetTranslation(), 0.02f));
				TestTrue(TEXT("Rest rotation is compatible"),
					Ref.GetRefBonePose()[Bone].GetRotation().Equals(Expected.GetRefBonePose()[Bone].GetRotation(), 0.001f));
			}
			TestTrue(TEXT("Mesh uses character centimetres"), Mesh->GetBounds().BoxExtent.Z > 15 && Mesh->GetBounds().BoxExtent.Z < 120);
			for (const FSkeletalMaterial& Material : Mesh->GetMaterials())
				TestNotNull(TEXT("Every material slot is populated"), Material.MaterialInterface.Get());
		}
	}
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
	ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
	UClass* WardrobeClass = LoadClass<ATunaSweeperWardrobeActor>(nullptr, TEXT("/Game/Interaction/BP_Wardrobe.BP_Wardrobe_C"));
	if (!TestNotNull(TEXT("Placeable wardrobe blueprint loads"), WardrobeClass)) return false;
	auto* Wardrobe = World->SpawnActor<ATunaSweeperWardrobeActor>(WardrobeClass);
	auto* Visual = Wardrobe ? Wardrobe->FindComponentByClass<UStaticMeshComponent>() : nullptr;
	if (!TestNotNull(TEXT("Wardrobe has a visible mesh"), Visual)) return false;
	UStaticMesh* Cabinet = Visual->GetStaticMesh();
	if (!TestNotNull(TEXT("Cabinet mesh is authored"), Cabinet)) return false;
	TestTrue(TEXT("Cabinet is full wardrobe height"), Cabinet->GetBounds().BoxExtent.Z > 85 && Cabinet->GetBounds().BoxExtent.Z < 110);
	TestTrue(TEXT("Wardrobe has simple blocking collision"), Cabinet->GetBodySetup() && Cabinet->GetBodySetup()->AggGeom.GetElementCount() > 0);
	TestTrue(TEXT("Wardrobe component uses natural scale"), Visual->GetRelativeScale3D().Equals(FVector::OneVector));

	if (FParse::Param(FCommandLine::Get(), TEXT("WardrobeMeshPreview")))
	{
		if (GShaderCompilingManager) GShaderCompilingManager->FinishAllCompilation();
		Wardrobe->SetActorHiddenInGame(true);
		UClass* PlayerClass = LoadClass<ATunaSweeperTopDownCharacter>(nullptr,
			TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
		if (!TestNotNull(TEXT("Actual player blueprint loads for render"), PlayerClass)) return false;
		auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(PlayerClass);
		auto* LightActor = World->SpawnActor<AActor>();
		auto* Light = NewObject<UDirectionalLightComponent>(LightActor);
		LightActor->SetRootComponent(Light); Light->SetIntensity(5.0f); Light->RegisterComponent();
		Light->SetWorldRotation(FRotator(-35,-35,0));
		auto* CaptureActor = World->SpawnActor<AActor>();
		auto* Capture = NewObject<USceneCaptureComponent2D>(CaptureActor);
		CaptureActor->SetRootComponent(Capture); Capture->RegisterComponent();
		Capture->bCaptureEveryFrame = false; Capture->bCaptureOnMovement = false;
		Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
		Capture->ProjectionType = ECameraProjectionMode::Orthographic; Capture->OrthoWidth = 190;
		auto* Target = NewObject<UTextureRenderTarget2D>();
		Target->ClearColor = FLinearColor(0.12f,0.16f,0.2f); Target->InitAutoFormat(768,1024);
		Capture->TextureTarget = Target;
		Capture->SetWorldLocation(FVector(270,-370,120));
		Capture->SetWorldRotation((FVector(0,0,5) - Capture->GetComponentLocation()).Rotation());
		const FString Directory = FPaths::ProjectSavedDir() / TEXT("WardrobePreview");
		IFileManager::Get().MakeDirectory(*Directory,true);
		for (const FTunaSweeperOutfitDefinition& Outfit : Catalog->Outfits)
		{
			if (!Player->GetOutfitComponent()->ApplyOutfit(Outfit)) continue;
			Player->GetMesh()->TickAnimation(0.0f,false); Player->GetMesh()->RefreshBoneTransforms();
			World->SendAllEndOfFrameUpdates(); FlushRenderingCommands(); Capture->CaptureScene(); FlushRenderingCommands();
			TArray<FColor> Pixels; Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
			TArray64<uint8> Png; FImageUtils::PNGCompressImageArray(768,1024,Pixels,Png);
			TestTrue(TEXT("Runtime outfit preview writes"), FFileHelper::SaveArrayToFile(Png, *(Directory / (Outfit.OutfitId.ToString()+TEXT(".png")))));
		}
	}
	return true;
}
#endif
