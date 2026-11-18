#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "ForestScatterMask.h"

#include "Components/DecalComponent.h"
#include "Engine/DecalActor.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterProjectedMaskTest, "TunaSweeper.ForestScatter.Mask.ProjectedGroundColors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterProjectedMaskTest::RunTest(const FString& Parameters)
{
	UMaterialInterface* ForestMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/MainRaid/EditorPreviews/VillageLayout/MI_Layout_Forest.MI_Layout_Forest"));
	UMaterialInterface* LayoutMaterial = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/MainRaid/EditorPreviews/VillageLayout/M_LocalLayoutPreview.M_LocalLayoutPreview"));
	if (!TestNotNull(TEXT("Forest guide material"), ForestMaterial) || !TestNotNull(TEXT("Layout material"), LayoutMaterial))
	{
		return false;
	}

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };

	// Representative swatches from the forest guide, with a transparent texel and a neutral annotation.
	const FColor Swatches[] = {
		FColor(123, 175, 110), FColor(222, 193, 110), FColor(88, 165, 201), FColor(139, 139, 139),
		FColor(129, 141, 131), FColor(123, 175, 110, 0), FColor(55, 112, 52), FColor(0, 190, 190)
	};
	UTexture2D* Texture = UTexture2D::CreateTransient(4, 2);
	Texture->Source.Init(4, 2, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Swatches));
	Texture->Filter = TF_Nearest;
	UMaterialInstanceDynamic* ForestInstance = UMaterialInstanceDynamic::Create(ForestMaterial, World);
	ForestInstance->SetTextureParameterValue(TEXT("PreviewTexture"), Texture);

	const FTransform Transform(FRotator(-90, 0, 0), FVector(900, -320, 70), FVector(2, 3, 4));
	ADecalActor* Forest = World->SpawnActor<ADecalActor>();
	Forest->SetActorTransform(Transform);
	Forest->GetDecal()->DecalSize = FVector(5, 8, 20);
	Forest->GetDecal()->SetDecalMaterial(ForestInstance);

	FForestScatterMask Mask;
	TestTrue(TEXT("Find the authored forest guide through a child instance"), Mask.Build(World, {}));
	TestTrue(TEXT("Grass in the first column"), Mask.IsGreen(FVector(840, -332, 70)));
	TestFalse(TEXT("Yellow road"), Mask.IsGreen(FVector(880, -332, 70)));
	TestFalse(TEXT("Blue water"), Mask.IsGreen(FVector(920, -332, 70)));
	TestFalse(TEXT("Neutral annotation"), Mask.IsGreen(FVector(960, -332, 70)));
	TestFalse(TEXT("Gray-green compression or annotation"), Mask.IsGreen(FVector(840, -308, 70)));
	TestFalse(TEXT("Transparent green"), Mask.IsGreen(FVector(880, -308, 70)));
	TestTrue(TEXT("Shaded forest green"), Mask.IsGreen(FVector(920, -308, 70)));
	TestFalse(TEXT("Cyan water"), Mask.IsGreen(FVector(960, -308, 70)));
	TestFalse(TEXT("Outside the projected rectangle"), Mask.IsGreen(FVector(810, -332, 70)));
	TestFalse(TEXT("Outside the projection depth"), Mask.IsGreen(FVector(840, -332, 81)));

	ADecalActor* Overlay = World->SpawnActor<ADecalActor>();
	Overlay->SetActorTransform(Transform);
	Overlay->GetDecal()->DecalSize = Forest->GetDecal()->DecalSize;
	Overlay->GetDecal()->SetSortOrder(5);
	UMaterialInstanceDynamic* OverlayInstance = UMaterialInstanceDynamic::Create(LayoutMaterial, World);
	const FColor Road(222, 193, 110);
	UTexture2D* RoadTexture = UTexture2D::CreateTransient(1, 1);
	RoadTexture->Source.Init(1, 1, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(&Road));
	OverlayInstance->SetTextureParameterValue(TEXT("PreviewTexture"), RoadTexture);
	Overlay->GetDecal()->SetDecalMaterial(OverlayInstance);

	TestTrue(TEXT("Automatic selection still finds the forest guide"), Mask.Build(World, {}));
	TestTrue(TEXT("Unrelated layout decals do not override the forest"), Mask.IsGreen(FVector(840, -332, 70)));
	const TArray<TObjectPtr<ADecalActor>> Explicit = {Forest, Overlay};
	TestTrue(TEXT("Load explicitly selected layout instances"), Mask.Build(World, Explicit));
	TestFalse(TEXT("The higher-priority yellow overlay wins"), Mask.IsGreen(FVector(840, -332, 70)));
	OverlayInstance->SetScalarParameterValue(TEXT("PreviewOpacity"), 0.0f);
	TestTrue(TEXT("Rebuild after changing an instance parameter"), Mask.Build(World, Explicit));
	TestTrue(TEXT("A zero-opacity overlay exposes the lower grass guide"), Mask.IsGreen(FVector(840, -332, 70)));
	TestFalse(TEXT("A missing world clears the cache"), Mask.Build(nullptr, {}));
	TestFalse(TEXT("Previous samples are no longer present"), Mask.IsGreen(FVector(840, -332, 70)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterGrayMarksTest, "TunaSweeper.ForestScatter.Mask.GrayMarksAndCliffs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterGrayMarksTest::RunTest(const FString& Parameters)
{
	UMaterialInterface* Layout = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/MainRaid/EditorPreviews/VillageLayout/M_LocalLayoutPreview.M_LocalLayoutPreview"));
	if (!TestNotNull(TEXT("Layout material"), Layout))
	{
		return false;
	}

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ADecalActor* Guide = World->SpawnActor<ADecalActor>();
	Guide->SetActorTransform(FTransform(FRotator(-90, 0, 0), FVector::ZeroVector));
	Guide->GetDecal()->DecalSize = FVector(30, 100, 100);
	UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Layout, World);
	Guide->GetDecal()->SetDecalMaterial(Instance);
	const TArray<TObjectPtr<ADecalActor>> Selection = {Guide};

	for (int32 Scale : {1, 4})
	{
		const int32 Size = 200 * Scale;
		const FColor Grass(123, 175, 110), Gray(139, 139, 139), Shade(69, 104, 75);
		TArray<FColor> Pixels;
		Pixels.Init(Grass, Size * Size);
		for (int32 Row = 0; Row < Size; ++Row)
		{
			for (int32 Column = 0; Column < Size; ++Column)
			{
				const int32 X = Column / Scale, Y = Row / Scale;
				FColor& Pixel = Pixels[Row * Size + Column];
				if (X >= 90 && X <= 95) { Pixel = FColor(222, 193, 110); }
				if (X >= 135 && X <= 140) { Pixel = FColor(88, 165, 201); }
				for (const int32 Center : {0, 45, 85, 130})
				{
					if (FMath::Square(X - Center) + FMath::Square(Y - 58) <= 16) { Pixel = Gray; }
				}
				if (X >= 30 && X < 70 && Y >= 110 && Y < 149) { Pixel = X % 8 < 4 ? Gray : Shade; }
				if (X >= 105 && X < 117 && Y >= 120 && Y < 123) { Pixel = Gray; }
				if (X >= 160 && X < 175 && Y >= 130 && Y < 145) { Pixel = Shade; }
			}
		}
		UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size);
		Texture->Filter = TF_Nearest;
		Texture->Source.Init(Size, Size, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Pixels.GetData()));
		Instance->SetTextureParameterValue(TEXT("PreviewTexture"), Texture);
		FForestScatterMask Mask;
		TestTrue(TEXT("Read the guide"), Mask.Build(World, Selection));
		auto IsGreenAt = [&Mask](int32 X, int32 Y) { return Mask.IsGreen(FVector(X - 99.5, Y - 99.5, 0)); };
		TestTrue(TEXT("Small neutral circle enclosed by grass"), IsGreenAt(45, 58));
		TestFalse(TEXT("Gray circle touching a road"), IsGreenAt(85, 58));
		TestFalse(TEXT("Gray circle touching water"), IsGreenAt(130, 58));
		TestFalse(TEXT("Gray circle clipped by the guide edge"), IsGreenAt(0, 58));
		TestFalse(TEXT("Long narrow gray annotation"), IsGreenAt(110, 121));
		TestFalse(TEXT("Large gray cliff"), IsGreenAt(50, 130));
		TestFalse(TEXT("Muted green-gray stripe within the cliff"), IsGreenAt(53, 130));
		TestTrue(TEXT("The same green shade apart from any gray cliff"), IsGreenAt(167, 137));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FForestScatterRaidForestGuideTest, "TunaSweeper.ForestScatter.Mask.RaidForestGuide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FForestScatterRaidForestGuideTest::RunTest(const FString& Parameters)
{
	UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr,
		TEXT("/Game/MainRaid/EditorPreviews/VillageLayout/MI_Layout_Forest.MI_Layout_Forest"));
	UTexture2D* Source = LoadObject<UTexture2D>(nullptr,
		TEXT("/Game/MainRaid/EditorPreviews/VillageLayout/T_Layout_Forest_Draft.T_Layout_Forest_Draft"));
	if (!TestNotNull(TEXT("RaidForest material"), Material) || !TestNotNull(TEXT("RaidForest texture"), Source))
	{
		return false;
	}
	UTexture* Effective = nullptr;
	Material->GetTextureParameterValue(FName(TEXT("PreviewTexture")), Effective);
	TestTrue(TEXT("RaidForest material projects the expected layout"), Effective == Source);
	TestEqual(TEXT("Forest sketch source width"), Source->Source.GetSizeX(), int64(400));
	TestEqual(TEXT("Forest sketch source height"), Source->Source.GetSizeY(), int64(400));

	const UWorld::InitializationValues Init = UWorld::InitializationValues()
		.AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false);
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Init);
	ON_SCOPE_EXIT { World->DestroyWorld(false); };
	ADecalActor* Guide = World->SpawnActor<ADecalActor>();
	Guide->SetActorTransform(FTransform(FRotator(-90, 0, 0), FVector::ZeroVector));
	Guide->GetDecal()->DecalSize = FVector(30, 200, 200);
	Guide->GetDecal()->SetDecalMaterial(Material);
	FForestScatterMask Mask;
	TestTrue(TEXT("Find RaidForest's unmodified guide"), Mask.Build(World, {}));
	auto AtPixel = [&Mask](FIntPoint Pixel)
	{
		return Mask.IsGreen(FVector(Pixel.X - 199.5, Pixel.Y - 199.5, 0));
	};
	for (const FIntPoint Pixel : {FIntPoint(94, 223), FIntPoint(132, 235), FIntPoint(219, 233),
		FIntPoint(254, 276), FIntPoint(123, 367)})
	{
		TestTrue(FString::Printf(TEXT("Grass under enclosed round marker %s"), *Pixel.ToString()), AtPixel(Pixel));
	}
	for (const FIntPoint Pixel : {FIntPoint(72, 102), FIntPoint(96, 114), FIntPoint(128, 118),
		FIntPoint(140, 130), FIntPoint(302, 32)})
	{
		TestFalse(FString::Printf(TEXT("Exclude cliff pigment %s"), *Pixel.ToString()), AtPixel(Pixel));
	}
	TestFalse(TEXT("Exclude the yellow road"), AtPixel(FIntPoint(85, 30)));
	TestFalse(TEXT("Exclude the blue stream"), AtPixel(FIntPoint(240, 135)));
	TestTrue(TEXT("Include the green clearing"), AtPixel(FIntPoint(13, 28)));
	return true;
}

#endif
