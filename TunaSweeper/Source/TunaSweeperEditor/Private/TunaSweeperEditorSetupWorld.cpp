#include "TunaSweeperEditorSetupShared.h"

namespace TunaSweeperEditorSetup
{
	UTexture2D* EnsureWarpPointNoiseTexture()
	{
		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, WarpPointNoiseTextureAssetName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!Texture)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *InteractionAssetPath, *WarpPointNoiseTextureAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			Texture = NewObject<UTexture2D>(
				Package,
				*WarpPointNoiseTextureAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!Texture)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Texture);
		}

		constexpr int32 TextureSize = 256;
		TArray<uint8> Pixels;
		Pixels.SetNumZeroed(TextureSize * TextureSize * 4);
		for (int32 Y = 0; Y < TextureSize; ++Y)
		{
			for (int32 X = 0; X < TextureSize; ++X)
			{
				const float LargeNoise = FMath::PerlinNoise2D(FVector2D(X * 0.042f, Y * 0.042f)) * 0.5f + 0.5f;
				const float SmallNoise = FMath::PerlinNoise2D(FVector2D((X + 71) * 0.115f, (Y - 43) * 0.115f)) * 0.5f + 0.5f;
				const uint8 Value = static_cast<uint8>(
					FMath::Clamp((LargeNoise * 0.72f + SmallNoise * 0.28f) * 255.0f, 0.0f, 255.0f));
				const int32 PixelIndex = (Y * TextureSize + X) * 4;
				Pixels[PixelIndex + 0] = Value;
				Pixels[PixelIndex + 1] = Value;
				Pixels[PixelIndex + 2] = Value;
				Pixels[PixelIndex + 3] = 255;
			}
		}

		Texture->Modify();
		Texture->Source.Init(TextureSize, TextureSize, 1, 1, TSF_BGRA8, Pixels.GetData());
		Texture->SRGB = false;
		Texture->CompressionSettings = TC_Grayscale;
		Texture->LODGroup = TEXTUREGROUP_Effects;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();

		if (!SaveAsset(Texture))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Texture;
	}

	UMaterial* EnsureWarpPointEnergyMaterial(UTexture2D* NoiseTexture)
	{
		if (!NoiseTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, WarpPointEnergyMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				WarpPointEnergyMaterialAssetName,
				InteractionAssetPath,
				UMaterial::StaticClass(),
				MaterialFactory);

			Material = Cast<UMaterial>(CreatedAsset);
			if (!Material)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(Material);
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVectorParameter* ColorParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParameter->Material = Material;
		ColorParameter->ParameterName = TEXT("EnergyColor");
		ColorParameter->DefaultValue = FLinearColor(0.0f, 0.86f, 0.95f, 1.0f);
		ColorParameter->MaterialExpressionEditorX = -760;
		ColorParameter->MaterialExpressionEditorY = -120;
		Material->GetExpressionCollection().AddExpression(ColorParameter);

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -960;
		TextureCoordinateExpression->MaterialExpressionEditorY = 160;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionPanner* PannerExpression = NewObject<UMaterialExpressionPanner>(Material);
		PannerExpression->Material = Material;
		PannerExpression->Coordinate.Connect(0, TextureCoordinateExpression);
		PannerExpression->SpeedX = 0.08f;
		PannerExpression->SpeedY = 0.18f;
		PannerExpression->MaterialExpressionEditorX = -760;
		PannerExpression->MaterialExpressionEditorY = 160;
		Material->GetExpressionCollection().AddExpression(PannerExpression);

		UMaterialExpressionTextureSampleParameter2D* NoiseSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		NoiseSample->Material = Material;
		NoiseSample->ParameterName = TEXT("NoiseTexture");
		NoiseSample->Texture = NoiseTexture;
		NoiseSample->SamplerType = SAMPLERTYPE_LinearGrayscale;
		NoiseSample->Coordinates.Connect(0, PannerExpression);
		NoiseSample->MaterialExpressionEditorX = -540;
		NoiseSample->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(NoiseSample);

		UMaterialExpressionMultiply* NoiseScale = NewObject<UMaterialExpressionMultiply>(Material);
		NoiseScale->Material = Material;
		NoiseScale->A.Connect(1, NoiseSample);
		NoiseScale->ConstB = 0.65f;
		NoiseScale->MaterialExpressionEditorX = -300;
		NoiseScale->MaterialExpressionEditorY = 150;
		Material->GetExpressionCollection().AddExpression(NoiseScale);

		UMaterialExpressionAdd* NoiseBias = NewObject<UMaterialExpressionAdd>(Material);
		NoiseBias->Material = Material;
		NoiseBias->A.Connect(0, NoiseScale);
		NoiseBias->ConstB = 0.35f;
		NoiseBias->MaterialExpressionEditorX = -100;
		NoiseBias->MaterialExpressionEditorY = 150;
		Material->GetExpressionCollection().AddExpression(NoiseBias);

		UMaterialExpressionMultiply* ColorByNoise = NewObject<UMaterialExpressionMultiply>(Material);
		ColorByNoise->Material = Material;
		ColorByNoise->A.Connect(0, ColorParameter);
		ColorByNoise->B.Connect(0, NoiseBias);
		ColorByNoise->MaterialExpressionEditorX = 110;
		ColorByNoise->MaterialExpressionEditorY = 20;
		Material->GetExpressionCollection().AddExpression(ColorByNoise);

		UMaterialExpressionScalarParameter* IntensityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		IntensityParameter->Material = Material;
		IntensityParameter->ParameterName = TEXT("Intensity");
		IntensityParameter->DefaultValue = 8.0f;
		IntensityParameter->MaterialExpressionEditorX = -100;
		IntensityParameter->MaterialExpressionEditorY = -160;
		Material->GetExpressionCollection().AddExpression(IntensityParameter);

		UMaterialExpressionMultiply* EmissiveMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveMultiply->Material = Material;
		EmissiveMultiply->A.Connect(0, ColorByNoise);
		EmissiveMultiply->B.Connect(0, IntensityParameter);
		EmissiveMultiply->MaterialExpressionEditorX = 340;
		EmissiveMultiply->MaterialExpressionEditorY = -40;
		Material->GetExpressionCollection().AddExpression(EmissiveMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, ColorParameter);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveMultiply);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.35f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.15f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	bool ConfigureLevelTravelBlueprint(UBlueprint* LevelTravelBlueprint)
	{
		if (!LevelTravelBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(LevelTravelBlueprint);

		ATunaSweeperLevelTravelInteractableActor* Defaults = LevelTravelBlueprint->GeneratedClass
			? Cast<ATunaSweeperLevelTravelInteractableActor>(LevelTravelBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(LevelTravelBlueprint));
			return false;
		}

		LevelTravelBlueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigureLevelTravelDefaults(
			NAME_None,
			FText::FromString(TEXT("Travel")),
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))),
			TSoftObjectPtr<UMediaSource>(),
			TSoftClassPtr<UTunaSweeperLevelTransitionWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, LevelTransitionVideoWidgetAssetName))),
			FText::GetEmpty());
		Defaults->ConfigureLevelTravelVisualDefaults(
			TSoftObjectPtr<UStaticMesh>(),
			FVector(0.75f, 0.75f, 0.75f),
			FVector::ZeroVector);
		FBlueprintEditorUtils::MarkBlueprintAsModified(LevelTravelBlueprint);
		FKismetEditorUtilities::CompileBlueprint(LevelTravelBlueprint);
		LevelTravelBlueprint->MarkPackageDirty();
		return SaveAsset(LevelTravelBlueprint);
	}

	bool ConfigureWarpPointBlueprint(UBlueprint* WarpPointBlueprint)
	{
		if (!WarpPointBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WarpPointBlueprint);

		ATunaSweeperWarpPointActor* Defaults = WarpPointBlueprint->GeneratedClass
			? Cast<ATunaSweeperWarpPointActor>(WarpPointBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(WarpPointBlueprint));
			return false;
		}

		WarpPointBlueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigureWarpPointDefaults(
			NAME_None,
			NAME_None,
			FText::FromString(TEXT("\uC0C1\uD638\uC791\uC6A9")),
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))),
			TSoftObjectPtr<UMaterialInterface>(
				FSoftObjectPath(GetAssetObjectPath(InteractionAssetPath, WarpPointEnergyMaterialAssetName))),
			TSoftObjectPtr<UStaticMesh>(
				FSoftObjectPath(TEXT("/Engine/BasicShapes/Sphere.Sphere"))),
			FVector(1.25f, 1.25f, 1.25f),
			FVector::ZeroVector,
			FVector(180.0f, 0.0f, 0.0f),
			true);
		FBlueprintEditorUtils::MarkBlueprintAsModified(WarpPointBlueprint);
		FKismetEditorUtilities::CompileBlueprint(WarpPointBlueprint);
		WarpPointBlueprint->MarkPackageDirty();
		return SaveAsset(WarpPointBlueprint);
	}

	bool ConfigureSelfDestructBlueprint(UBlueprint* SelfDestructBlueprint)
	{
		if (!SelfDestructBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(SelfDestructBlueprint);

		ATunaSweeperSelfDestructInteractableActor* Defaults = SelfDestructBlueprint->GeneratedClass
			? Cast<ATunaSweeperSelfDestructInteractableActor>(SelfDestructBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(SelfDestructBlueprint));
			return false;
		}

		SelfDestructBlueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigureSelfDestructDefaults(
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))),
			TSoftClassPtr<UTunaSweeperSpeechBubbleWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, SpeechBubbleWidgetAssetName))));
		FBlueprintEditorUtils::MarkBlueprintAsModified(SelfDestructBlueprint);
		FKismetEditorUtilities::CompileBlueprint(SelfDestructBlueprint);
		SelfDestructBlueprint->MarkPackageDirty();
		return SaveAsset(SelfDestructBlueprint);
	}

	bool ConfigureSelfDestructActorInstance(AActor* Actor)
	{
		ATunaSweeperSelfDestructInteractableActor* SelfDestructActor = Cast<ATunaSweeperSelfDestructInteractableActor>(Actor);
		if (!SelfDestructActor)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s is not an ATunaSweeperSelfDestructInteractableActor."), *GetNameSafe(Actor));
			return false;
		}

		SelfDestructActor->Modify();
		SelfDestructActor->ConfigureSelfDestructDefaults(
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))),
			TSoftClassPtr<UTunaSweeperSpeechBubbleWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, SpeechBubbleWidgetAssetName))));
		SelfDestructActor->MarkPackageDirty();
		return true;
	}

	bool ConfigurePickupItemIconWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !BuildPickupItemIconWidgetTree(WidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return SaveAsset(WidgetBlueprint);
	}

	bool ConfigurePickupItemBlueprint(UBlueprint* PickupItemBlueprint, int32 ItemId)
	{
		if (!PickupItemBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(PickupItemBlueprint);

		ATunaSweeperPickupItemActor* Defaults = PickupItemBlueprint->GeneratedClass
			? Cast<ATunaSweeperPickupItemActor>(PickupItemBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(PickupItemBlueprint));
			return false;
		}

		PickupItemBlueprint->Modify();
		Defaults->ConfigurePickupItemDefaults(
			ItemId,
			TSoftClassPtr<UTunaSweeperPickupItemIconWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, PickupItemIconWidgetAssetName))));
		FBlueprintEditorUtils::MarkBlueprintAsModified(PickupItemBlueprint);
		FKismetEditorUtilities::CompileBlueprint(PickupItemBlueprint);
		PickupItemBlueprint->MarkPackageDirty();
		return SaveAsset(PickupItemBlueprint);
	}

	bool ConfigureItemSpawnBlueprint(UBlueprint* ItemSpawnBlueprint)
	{
		if (!ItemSpawnBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(ItemSpawnBlueprint);

		ATunaSweeperItemSpawnInteractableActor* Defaults = ItemSpawnBlueprint->GeneratedClass
			? Cast<ATunaSweeperItemSpawnInteractableActor>(ItemSpawnBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(ItemSpawnBlueprint));
			return false;
		}

		ItemSpawnBlueprint->Modify();
		Defaults->ConfigureItemSpawnDefaults(
			TSoftClassPtr<ATunaSweeperPickupItemActor>(
				FSoftObjectPath(GetAssetClassPath(InteractionAssetPath, PickupItemAssetName))));
		if (UTunaSweeperInteractableComponent* InteractableComponent = Defaults->GetInteractableComponent())
		{
			InteractableComponent->SetInteractionTypeAndDisplayName(
				ETunaSweeperInteractionType::ItemSpawn,
				FText::FromString(TEXT("\uC544\uC774\uD15C\uC2A4\uD3F0")));
		}
		FBlueprintEditorUtils::MarkBlueprintAsModified(ItemSpawnBlueprint);
		FKismetEditorUtilities::CompileBlueprint(ItemSpawnBlueprint);
		ItemSpawnBlueprint->MarkPackageDirty();
		return SaveAsset(ItemSpawnBlueprint);
	}

	bool ConfigureLootContainerBlueprint(UBlueprint* LootContainerBlueprint, int32 ContainerDefinitionId, int32 ContentsId)
	{
		if (!LootContainerBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(LootContainerBlueprint);

		ATunaSweeperLootContainerActor* Defaults = LootContainerBlueprint->GeneratedClass
			? Cast<ATunaSweeperLootContainerActor>(LootContainerBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(LootContainerBlueprint));
			return false;
		}

		LootContainerBlueprint->Modify();
		Defaults->ConfigureLootContainerDefaults(ContainerDefinitionId, ContentsId);
		if (UTunaSweeperInteractableComponent* InteractableComponent = Defaults->GetInteractableComponent())
		{
			InteractableComponent->SetInteractionTypeAndDisplayName(
				ETunaSweeperInteractionType::LootContainerOpen,
				FText::FromString(TEXT("\uC5F4\uAE30")));
		}
		FBlueprintEditorUtils::MarkBlueprintAsModified(LootContainerBlueprint);
		FKismetEditorUtilities::CompileBlueprint(LootContainerBlueprint);
		LootContainerBlueprint->MarkPackageDirty();
		return SaveAsset(LootContainerBlueprint);
	}

	bool ConfigureLootContainerSpawnBlueprint(UBlueprint* LootContainerSpawnBlueprint)
	{
		if (!LootContainerSpawnBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(LootContainerSpawnBlueprint);

		ATunaSweeperLootContainerSpawnInteractableActor* Defaults = LootContainerSpawnBlueprint->GeneratedClass
			? Cast<ATunaSweeperLootContainerSpawnInteractableActor>(LootContainerSpawnBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(LootContainerSpawnBlueprint));
			return false;
		}

		LootContainerSpawnBlueprint->Modify();
		Defaults->ConfigureLootContainerSpawnDefaults(
			TSoftClassPtr<ATunaSweeperLootContainerActor>(
				FSoftObjectPath(GetAssetClassPath(InteractionAssetPath, LootContainerAssetName))));
		if (UTunaSweeperInteractableComponent* InteractableComponent = Defaults->GetInteractableComponent())
		{
			InteractableComponent->SetInteractionTypeAndDisplayName(
				ETunaSweeperInteractionType::LootContainerSpawn,
				FText::FromString(TEXT("\uC0C1\uC790\uC2A4\uD3F0")));
		}
		FBlueprintEditorUtils::MarkBlueprintAsModified(LootContainerSpawnBlueprint);
		FKismetEditorUtilities::CompileBlueprint(LootContainerSpawnBlueprint);
		LootContainerSpawnBlueprint->MarkPackageDirty();
		return SaveAsset(LootContainerSpawnBlueprint);
	}

	bool ConfigurePickupItemActorInstance(AActor* Actor, int32 ItemId)
	{
		ATunaSweeperPickupItemActor* PickupItemActor = Cast<ATunaSweeperPickupItemActor>(Actor);
		if (!PickupItemActor)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s is not an ATunaSweeperPickupItemActor."), *GetNameSafe(Actor));
			return false;
		}

		PickupItemActor->Modify();
		PickupItemActor->ConfigurePickupItemDefaults(
			ItemId,
			TSoftClassPtr<UTunaSweeperPickupItemIconWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, PickupItemIconWidgetAssetName))));
		PickupItemActor->MarkPackageDirty();
		return true;
	}

	bool ConfigureItemSpawnActorInstance(AActor* Actor)
	{
		ATunaSweeperItemSpawnInteractableActor* ItemSpawnActor = Cast<ATunaSweeperItemSpawnInteractableActor>(Actor);
		if (!ItemSpawnActor)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s is not an ATunaSweeperItemSpawnInteractableActor."), *GetNameSafe(Actor));
			return false;
		}

		ItemSpawnActor->Modify();
		ItemSpawnActor->ConfigureItemSpawnDefaults(
			TSoftClassPtr<ATunaSweeperPickupItemActor>(
				FSoftObjectPath(GetAssetClassPath(InteractionAssetPath, PickupItemAssetName))));
		if (UTunaSweeperInteractableComponent* InteractableComponent = ItemSpawnActor->GetInteractableComponent())
		{
			InteractableComponent->SetInteractionTypeAndDisplayName(
				ETunaSweeperInteractionType::ItemSpawn,
				FText::FromString(TEXT("\uC544\uC774\uD15C\uC2A4\uD3F0")));
		}
		ItemSpawnActor->MarkPackageDirty();
		return true;
	}

	bool ConfigureLootContainerActorInstance(AActor* Actor, int32 ContainerDefinitionId, int32 ContentsId)
	{
		ATunaSweeperLootContainerActor* LootContainerActor = Cast<ATunaSweeperLootContainerActor>(Actor);
		if (!LootContainerActor)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s is not an ATunaSweeperLootContainerActor."), *GetNameSafe(Actor));
			return false;
		}

		LootContainerActor->Modify();
		LootContainerActor->ConfigureLootContainerDefaults(ContainerDefinitionId, ContentsId);
		if (UTunaSweeperInteractableComponent* InteractableComponent = LootContainerActor->GetInteractableComponent())
		{
			InteractableComponent->SetInteractionTypeAndDisplayName(
				ETunaSweeperInteractionType::LootContainerOpen,
				FText::FromString(TEXT("\uC5F4\uAE30")));
		}
		LootContainerActor->MarkPackageDirty();
		return true;
	}

	bool ConfigureLootContainerSpawnActorInstance(AActor* Actor)
	{
		ATunaSweeperLootContainerSpawnInteractableActor* SpawnActor = Cast<ATunaSweeperLootContainerSpawnInteractableActor>(Actor);
		if (!SpawnActor)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s is not an ATunaSweeperLootContainerSpawnInteractableActor."), *GetNameSafe(Actor));
			return false;
		}

		SpawnActor->Modify();
		SpawnActor->ConfigureLootContainerSpawnDefaults(
			TSoftClassPtr<ATunaSweeperLootContainerActor>(
				FSoftObjectPath(GetAssetClassPath(InteractionAssetPath, LootContainerAssetName))));
		if (UTunaSweeperInteractableComponent* InteractableComponent = SpawnActor->GetInteractableComponent())
		{
			InteractableComponent->SetInteractionTypeAndDisplayName(
				ETunaSweeperInteractionType::LootContainerSpawn,
				FText::FromString(TEXT("\uC0C1\uC790\uC2A4\uD3F0")));
		}
		SpawnActor->MarkPackageDirty();
		return true;
	}

	AActor* FindActorByLabel(UWorld* World, const FString& ActorLabel)
	{
		if (!World)
		{
			return nullptr;
		}

		for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			if (ActorIt->GetActorLabel() == ActorLabel)
			{
				return *ActorIt;
			}
		}

		return nullptr;
	}

	UWorld* LoadEditorMapForSetup(const FString& MapPackagePath)
	{
		if (UWorld* CurrentWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr)
		{
			if (CurrentWorld->GetPackage()->GetName() == MapPackagePath)
			{
				return CurrentWorld;
			}
		}

		const FString MapFilename = FPackageName::LongPackageNameToFilename(
			MapPackagePath,
			FPackageName::GetMapPackageExtension());
		UWorld* LoadedWorld = UEditorLoadingAndSavingUtils::LoadMap(MapFilename);
		if (!LoadedWorld || LoadedWorld->GetPackage()->GetName() != MapPackagePath)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load map %s."), *MapPackagePath);
			return nullptr;
		}

		return LoadedWorld;
	}

	bool IsEditorWorldReadyForMapSetup()
	{
		UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!EditorWorld || !EditorWorld->GetPackage())
		{
			return false;
		}

		return !EditorWorld->GetPackage()->GetName().StartsWith(TEXT("/Temp/"));
	}

	bool ConfigureEditorMapCaptureActorInstance(AActor* Actor, bool bAutoDetectBounds)
	{
		ATunaSweeperMapCaptureActor* MapCaptureActor = Cast<ATunaSweeperMapCaptureActor>(Actor);
		if (!MapCaptureActor)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s is not an ATunaSweeperMapCaptureActor."), *GetNameSafe(Actor));
			return false;
		}

		MapCaptureActor->Modify();
		MapCaptureActor->SetActorRotation(FRotator::ZeroRotator);
		if (bAutoDetectBounds)
		{
			MapCaptureActor->AutoDetectCaptureBounds();
		}
		MapCaptureActor->MarkPackageDirty();
		return true;
	}

	bool PlaceEditorMapCaptureActorInRaidMap(UBlueprint* MapCaptureBlueprint)
	{
		if (!MapCaptureBlueprint || !MapCaptureBlueprint->GeneratedClass)
		{
			return false;
		}

		UWorld* RaidWorld = LoadEditorMapForSetup(RaidMapPackagePath);
		if (!RaidWorld)
		{
			return false;
		}

		bool bPlacedOrUpdated = false;
		if (AActor* ExistingActor = FindActorByLabel(RaidWorld, EditorMapCaptureActorLabel))
		{
			bPlacedOrUpdated = ConfigureEditorMapCaptureActorInstance(ExistingActor, false);
		}
		else
		{
			RaidWorld->PersistentLevel->Modify();

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.OverrideLevel = RaidWorld->PersistentLevel;
			SpawnParameters.Name = MakeUniqueObjectName(
				RaidWorld->PersistentLevel,
				MapCaptureBlueprint->GeneratedClass,
				FName(*EditorMapCaptureActorLabel));
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			AActor* SpawnedActor = RaidWorld->SpawnActor<AActor>(
				MapCaptureBlueprint->GeneratedClass,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParameters);
			if (!SpawnedActor)
			{
				return false;
			}

			SpawnedActor->SetActorLabel(EditorMapCaptureActorLabel);
			bPlacedOrUpdated = ConfigureEditorMapCaptureActorInstance(SpawnedActor, true);
		}

		const bool bSaved = bPlacedOrUpdated && UEditorLoadingAndSavingUtils::SaveMap(RaidWorld, RaidMapPackagePath);
		LoadEditorMapForSetup(IntroMapPackagePath);
		return bSaved;
	}

	bool EnsureEditorMapCaptureBlueprintAndRaidPlacement()
	{
		UBlueprint* MapCaptureBlueprint = EnsureBlueprint(
			EditorMapCaptureAssetPath,
			EditorMapCaptureBlueprintAssetName,
			ATunaSweeperMapCaptureActor::StaticClass());

		return MapCaptureBlueprint && PlaceEditorMapCaptureActorInRaidMap(MapCaptureBlueprint);
	}

	bool EnsureOpeningScenarioMap()
	{
		const FString MapFilename = FPackageName::LongPackageNameToFilename(
			OpeningScenarioMapPackagePath,
			FPackageName::GetMapPackageExtension());
		if (FPaths::FileExists(MapFilename))
		{
			return true;
		}

		UWorld* OpeningScenarioWorld = UEditorLoadingAndSavingUtils::NewBlankMap(false);
		if (!OpeningScenarioWorld)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create opening scenario map."));
			return false;
		}

		const bool bSaved = UEditorLoadingAndSavingUtils::SaveMap(OpeningScenarioWorld, OpeningScenarioMapPackagePath);
		if (!bSaved)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *OpeningScenarioMapPackagePath);
			return false;
		}

		LoadEditorMapForSetup(IntroMapPackagePath);
		return true;
	}

	bool EnsureOpeningScenarioPresentationSetup()
	{
		return EnsureOpeningScenarioUiTextures() && EnsureOpeningScenarioMap();
	}

	bool PlacePickupItemActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location, int32 ItemId)
	{
		if (!World || !ActorBlueprint || !ActorBlueprint->GeneratedClass)
		{
			return false;
		}

		if (AActor* ExistingActor = FindActorByLabel(World, ActorLabel))
		{
			ExistingActor->Modify();
			ExistingActor->SetActorLocation(Location);
			ExistingActor->SetActorRotation(FRotator::ZeroRotator);
			return ConfigurePickupItemActorInstance(ExistingActor, ItemId);
		}

		World->PersistentLevel->Modify();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ActorBlueprint->GeneratedClass, FName(*ActorLabel));
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* SpawnedActor = World->SpawnActor<AActor>(ActorBlueprint->GeneratedClass, Location, FRotator::ZeroRotator, SpawnParameters);
		if (!SpawnedActor)
		{
			return false;
		}

		SpawnedActor->SetActorLabel(ActorLabel);
		if (!ConfigurePickupItemActorInstance(SpawnedActor, ItemId))
		{
			return false;
		}
		SpawnedActor->MarkPackageDirty();
		return true;
	}

	bool PlaceItemSpawnActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location)
	{
		if (!World || !ActorBlueprint || !ActorBlueprint->GeneratedClass)
		{
			return false;
		}

		if (AActor* ExistingActor = FindActorByLabel(World, ActorLabel))
		{
			ExistingActor->Modify();
			ExistingActor->SetActorLocation(Location);
			ExistingActor->SetActorRotation(FRotator::ZeroRotator);
			return ConfigureItemSpawnActorInstance(ExistingActor);
		}

		World->PersistentLevel->Modify();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ActorBlueprint->GeneratedClass, FName(*ActorLabel));
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* SpawnedActor = World->SpawnActor<AActor>(ActorBlueprint->GeneratedClass, Location, FRotator::ZeroRotator, SpawnParameters);
		if (!SpawnedActor)
		{
			return false;
		}

		SpawnedActor->SetActorLabel(ActorLabel);
		if (!ConfigureItemSpawnActorInstance(SpawnedActor))
		{
			return false;
		}
		SpawnedActor->MarkPackageDirty();
		return true;
	}

	bool PlaceLootContainerActor(
		UWorld* World,
		UBlueprint* ActorBlueprint,
		const FString& ActorLabel,
		const FVector& Location,
		int32 ContainerDefinitionId,
		int32 ContentsId)
	{
		if (!World || !ActorBlueprint || !ActorBlueprint->GeneratedClass)
		{
			return false;
		}

		if (AActor* ExistingActor = FindActorByLabel(World, ActorLabel))
		{
			ExistingActor->Modify();
			ExistingActor->SetActorLocation(Location);
			ExistingActor->SetActorRotation(FRotator::ZeroRotator);
			return ConfigureLootContainerActorInstance(ExistingActor, ContainerDefinitionId, ContentsId);
		}

		World->PersistentLevel->Modify();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ActorBlueprint->GeneratedClass, FName(*ActorLabel));
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* SpawnedActor = World->SpawnActor<AActor>(ActorBlueprint->GeneratedClass, Location, FRotator::ZeroRotator, SpawnParameters);
		if (!SpawnedActor)
		{
			return false;
		}

		SpawnedActor->SetActorLabel(ActorLabel);
		if (!ConfigureLootContainerActorInstance(SpawnedActor, ContainerDefinitionId, ContentsId))
		{
			return false;
		}
		SpawnedActor->MarkPackageDirty();
		return true;
	}

	bool PlaceLootContainerSpawnActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location)
	{
		if (!World || !ActorBlueprint || !ActorBlueprint->GeneratedClass)
		{
			return false;
		}

		if (AActor* ExistingActor = FindActorByLabel(World, ActorLabel))
		{
			ExistingActor->Modify();
			ExistingActor->SetActorLocation(Location);
			ExistingActor->SetActorRotation(FRotator::ZeroRotator);
			return ConfigureLootContainerSpawnActorInstance(ExistingActor);
		}

		World->PersistentLevel->Modify();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ActorBlueprint->GeneratedClass, FName(*ActorLabel));
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* SpawnedActor = World->SpawnActor<AActor>(ActorBlueprint->GeneratedClass, Location, FRotator::ZeroRotator, SpawnParameters);
		if (!SpawnedActor)
		{
			return false;
		}

		SpawnedActor->SetActorLabel(ActorLabel);
		if (!ConfigureLootContainerSpawnActorInstance(SpawnedActor))
		{
			return false;
		}
		SpawnedActor->MarkPackageDirty();
		return true;
	}

	bool PlaceSelfDestructActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location)
	{
		if (!World || !ActorBlueprint || !ActorBlueprint->GeneratedClass)
		{
			return false;
		}

		if (AActor* ExistingActor = FindActorByLabel(World, ActorLabel))
		{
			ExistingActor->Modify();
			ExistingActor->SetActorLocation(Location);
			ExistingActor->SetActorRotation(FRotator::ZeroRotator);
			return ConfigureSelfDestructActorInstance(ExistingActor);
		}

		World->PersistentLevel->Modify();

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.OverrideLevel = World->PersistentLevel;
		SpawnParameters.Name = MakeUniqueObjectName(World->PersistentLevel, ActorBlueprint->GeneratedClass, FName(*ActorLabel));
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AActor* SpawnedActor = World->SpawnActor<AActor>(ActorBlueprint->GeneratedClass, Location, FRotator::ZeroRotator, SpawnParameters);
		if (!SpawnedActor)
		{
			return false;
		}

		SpawnedActor->SetActorLabel(ActorLabel);
		if (!ConfigureSelfDestructActorInstance(SpawnedActor))
		{
			return false;
		}
		SpawnedActor->MarkPackageDirty();
		return true;
	}

	bool PlacePickupItemAndSpawnerActorsInRaidMap(UBlueprint* PickupItemBlueprint, UBlueprint* ItemSpawnBlueprint)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World || World->GetPackage()->GetName() != RaidMapPackagePath)
		{
			return false;
		}

		const bool bPlacedActors =
			PlacePickupItemActor(
				World,
				PickupItemBlueprint,
				TEXT("TS_PickupItem_Sample"),
				FVector(950.0f, 50.0f, 8.0f),
				1001) &&
			PlaceItemSpawnActor(
				World,
				ItemSpawnBlueprint,
				TEXT("TS_Interact_ItemSpawn"),
				FVector(950.0f, -200.0f, 80.0f));

		if (!bPlacedActors)
		{
			return false;
		}

		return UEditorLoadingAndSavingUtils::SaveMap(World, RaidMapPackagePath);
	}

	bool EnsurePickupItemAndSpawnerAssetsAndMapPlacement()
	{
		UWidgetBlueprint* PickupItemIconWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			PickupItemIconWidgetAssetName,
			UTunaSweeperPickupItemIconWidget::StaticClass());
		UBlueprint* PickupItemBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			PickupItemAssetName,
			ATunaSweeperPickupItemActor::StaticClass());
		UBlueprint* ItemSpawnBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			ItemSpawnInteractionAssetName,
			ATunaSweeperItemSpawnInteractableActor::StaticClass());

		if (!PickupItemIconWidgetBlueprint || !PickupItemBlueprint || !ItemSpawnBlueprint)
		{
			return false;
		}

		const bool bConfigured =
			ConfigurePickupItemIconWidgetBlueprint(PickupItemIconWidgetBlueprint) &&
			ConfigurePickupItemBlueprint(PickupItemBlueprint, 1001) &&
			ConfigureItemSpawnBlueprint(ItemSpawnBlueprint);

		return bConfigured;
	}

	bool PlaceLootContainerAndSpawnerActorsInRaidMap(UBlueprint* LootContainerBlueprint, UBlueprint* LootContainerSpawnBlueprint)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World || World->GetPackage()->GetName() != RaidMapPackagePath)
		{
			return false;
		}

		const bool bPlacedActors =
			PlaceLootContainerActor(
				World,
				LootContainerBlueprint,
				TEXT("TS_LootContainer_Sample"),
				FVector(1220.0f, 50.0f, 40.0f),
				7001,
				8001) &&
			PlaceLootContainerSpawnActor(
				World,
				LootContainerSpawnBlueprint,
				TEXT("TS_Interact_LootContainerSpawn"),
				FVector(1220.0f, -220.0f, 80.0f));

		if (!bPlacedActors)
		{
			return false;
		}

		return UEditorLoadingAndSavingUtils::SaveMap(World, RaidMapPackagePath);
	}

	bool EnsureLootContainerAndSpawnerAssetsAndMapPlacement()
	{
		UBlueprint* LootContainerBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			LootContainerAssetName,
			ATunaSweeperLootContainerActor::StaticClass());
		UBlueprint* LootContainerSpawnBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			LootContainerSpawnInteractionAssetName,
			ATunaSweeperLootContainerSpawnInteractableActor::StaticClass());

		if (!LootContainerBlueprint || !LootContainerSpawnBlueprint)
		{
			return false;
		}

		const bool bConfigured =
			ConfigureLootContainerBlueprint(LootContainerBlueprint, 7001, 8001) &&
			ConfigureLootContainerSpawnBlueprint(LootContainerSpawnBlueprint);

		return bConfigured;
	}

	bool PlaceSelfDestructActorInRaidMap(UBlueprint* SelfDestructBlueprint)
	{
		if (!SelfDestructBlueprint)
		{
			return false;
		}

		UWorld* RaidWorld = LoadEditorMapForSetup(RaidMapPackagePath);
		const bool bPlaced =
			RaidWorld &&
			PlaceSelfDestructActor(
				RaidWorld,
				SelfDestructBlueprint,
				TEXT("TS_Interact_SelfDestruct"),
				FVector(1520.0f, -220.0f, 80.0f)) &&
			UEditorLoadingAndSavingUtils::SaveMap(RaidWorld, RaidMapPackagePath);

		LoadEditorMapForSetup(IntroMapPackagePath);
		return bPlaced;
	}

	bool EnsureSelfDestructInteractionSetup()
	{
		UWidgetBlueprint* SpeechBubbleWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			SpeechBubbleWidgetAssetName,
			UTunaSweeperSpeechBubbleWidget::StaticClass());
		UBlueprint* SelfDestructBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			SelfDestructInteractionAssetName,
			ATunaSweeperSelfDestructInteractableActor::StaticClass());

		if (!SpeechBubbleWidgetBlueprint || !SelfDestructBlueprint)
		{
			return false;
		}

		const bool bConfigured =
			ConfigureSpeechBubbleWidgetBlueprint(SpeechBubbleWidgetBlueprint) &&
			ConfigureSelfDestructBlueprint(SelfDestructBlueprint);

		return bConfigured;
	}

	bool PlaceTitlePresentationActorInIntroMap(UBlueprint* TitlePresentationBlueprint)
	{
		if (!TitlePresentationBlueprint || !TitlePresentationBlueprint->GeneratedClass)
		{
			return false;
		}

		UWorld* IntroWorld = LoadEditorMapForSetup(IntroMapPackagePath);
		if (!IntroWorld)
		{
			return false;
		}

		IntroWorld->PersistentLevel->Modify();
		AActor* TitlePresentationActor = FindActorByLabel(IntroWorld, TitlePresentationActorLabel);
		if (TitlePresentationActor && TitlePresentationActor->GetClass() != TitlePresentationBlueprint->GeneratedClass)
		{
			IntroWorld->DestroyActor(TitlePresentationActor);
			TitlePresentationActor = nullptr;
		}

		if (!TitlePresentationActor)
		{
			FActorSpawnParameters SpawnParameters;
			SpawnParameters.OverrideLevel = IntroWorld->PersistentLevel;
			SpawnParameters.Name = MakeUniqueObjectName(
				IntroWorld->PersistentLevel,
				TitlePresentationBlueprint->GeneratedClass,
				FName(*TitlePresentationActorLabel));
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			TitlePresentationActor = IntroWorld->SpawnActor<AActor>(
				TitlePresentationBlueprint->GeneratedClass,
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParameters);
		}

		if (!TitlePresentationActor)
		{
			return false;
		}

		TitlePresentationActor->Modify();
		TitlePresentationActor->SetActorLabel(TitlePresentationActorLabel);
		TitlePresentationActor->SetActorLocationAndRotation(FVector::ZeroVector, FRotator::ZeroRotator);
		if (ATunaSweeperTitlePresentationActor* TypedTitlePresentationActor =
			Cast<ATunaSweeperTitlePresentationActor>(TitlePresentationActor))
		{
			TypedTitlePresentationActor->ApplyRecommendedPresentationLayout();
		}
		TitlePresentationActor->MarkPackageDirty();
		return UEditorLoadingAndSavingUtils::SaveMap(IntroWorld, IntroMapPackagePath);
	}

	bool EnsureTitlePresentationSetup()
	{
		UBlueprint* TitlePresentationBlueprint = EnsureBlueprint(
			UITitleTextureAssetPath,
			TitlePresentationActorAssetName,
			ATunaSweeperTitlePresentationActor::StaticClass());

		if (!TitlePresentationBlueprint)
		{
			return false;
		}

		TitlePresentationBlueprint->Modify();
		FKismetEditorUtilities::CompileBlueprint(TitlePresentationBlueprint);
		if (ATunaSweeperTitlePresentationActor* BlueprintDefaults =
			Cast<ATunaSweeperTitlePresentationActor>(TitlePresentationBlueprint->GeneratedClass->GetDefaultObject()))
		{
			BlueprintDefaults->Modify();
			BlueprintDefaults->ApplyRecommendedPresentationLayout();
		}
		TitlePresentationBlueprint->MarkPackageDirty();
		return SaveAsset(TitlePresentationBlueprint) &&
			PlaceTitlePresentationActorInIntroMap(TitlePresentationBlueprint);
	}

	bool EnsureIntroMenuAndLevelTravelSetup()
	{
		UWidgetBlueprint* IntroMenuWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			IntroMenuWidgetAssetName,
			UTunaSweeperIntroMenuWidget::StaticClass());
		UBlueprint* LevelTravelBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			LevelTravelInteractionAssetName,
			ATunaSweeperLevelTravelInteractableActor::StaticClass());

		if (!IntroMenuWidgetBlueprint || !LevelTravelBlueprint)
		{
			return false;
		}

		const bool bConfigured =
			SetProjectStartupMapsToIntro() &&
			EnsureTitleUiTextures() &&
			ConfigureIntroMenuWidgetBlueprint(IntroMenuWidgetBlueprint) &&
			EnsureTitlePresentationSetup() &&
			ConfigureLevelTravelBlueprint(LevelTravelBlueprint);

		return bConfigured;
	}

	bool EnsureIntroMenuGraphicsSettingsSetup()
	{
		UWidgetBlueprint* IntroMenuWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			IntroMenuWidgetAssetName,
			UTunaSweeperIntroMenuWidget::StaticClass());

		return ConfigureIntroMenuWidgetBlueprint(IntroMenuWidgetBlueprint);
	}

	bool EnsureBunkerToRaidTransitionVideoSetup()
	{
		UWidgetBlueprint* LevelTransitionWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			LevelTransitionVideoWidgetAssetName,
			UTunaSweeperLevelTransitionWidget::StaticClass());
		UBlueprint* LevelTravelBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			LevelTravelInteractionAssetName,
			ATunaSweeperLevelTravelInteractableActor::StaticClass());
		UMediaSource* BunkerToRaidMediaSource = LoadObject<UMediaSource>(
			nullptr,
			*GetAssetObjectPath(VideoAssetPath, BunkerToRaidMediaSourceAssetName));

		if (!LevelTransitionWidgetBlueprint || !LevelTravelBlueprint || !BunkerToRaidMediaSource)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing level transition video setup asset."));
			return false;
		}

		const bool bConfigured =
			ConfigureLevelTransitionVideoWidgetBlueprint(LevelTransitionWidgetBlueprint) &&
			ConfigureLevelTravelBlueprint(LevelTravelBlueprint);

		return bConfigured;
	}

	bool EnsureFirstOutingQuestSetup()
	{
		UWidgetBlueprint* QuestMenuWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			QuestMenuWidgetAssetName,
			UTunaSweeperMenuQuestWidget::StaticClass());
		UWidgetBlueprint* QuestInteractionWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			QuestInteractionWidgetAssetName,
			UTunaSweeperInteractionQuestWidget::StaticClass());

		if (!QuestMenuWidgetBlueprint || !QuestInteractionWidgetBlueprint)
		{
			return false;
		}

		const bool bConfigured =
			ConfigureQuestWidgetBlueprint(QuestMenuWidgetBlueprint) &&
			ConfigureQuestWidgetBlueprint(QuestInteractionWidgetBlueprint);

		return bConfigured;
	}

	bool EnsureWorldProgressInteractionAssets()
	{
		UBlueprint* TransparentObstacleBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			TransparentObstacleAssetName,
			ATunaSweeperTransparentObstacleActor::StaticClass());
		UBlueprint* BrokenBridgeBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			WorldProgressBrokenBridgeAssetName,
			ATunaSweeperWorldProgressActor::StaticClass());
		UBlueprint* RepairedBridgeBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			WorldProgressRepairedBridgeAssetName,
			ATunaSweeperWorldProgressCompletedActor::StaticClass());

		return TransparentObstacleBlueprint && BrokenBridgeBlueprint && RepairedBridgeBlueprint;
	}

	bool EnsureWarpPointInteractionAssets()
	{
		UTexture2D* NoiseTexture = EnsureWarpPointNoiseTexture();
		UMaterial* WarpPointMaterial = EnsureWarpPointEnergyMaterial(NoiseTexture);
		UBlueprint* WarpPointBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			WarpPointInteractionAssetName,
			ATunaSweeperWarpPointActor::StaticClass());

		return NoiseTexture && WarpPointMaterial && ConfigureWarpPointBlueprint(WarpPointBlueprint);
	}

}
