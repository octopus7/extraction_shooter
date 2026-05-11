#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AutomatedAssetImportData.h"
#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ListViewBase.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Containers/Ticker.h"
#include "Engine/Blueprint.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "Factories/BlueprintFactory.h"
#include "FileHelpers.h"
#include "Game/TunaSweeperGameMode.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameMapsSettings.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperItemSpawnInteractableActor.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Interaction/TunaSweeperLootContainerSpawnInteractableActor.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Player/TunaSweeperPlayerController.h"
#include "TunaSweeperEditorRunOnce.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "UI/TunaSweeperHudExternalPanelWidget.h"
#include "UI/TunaSweeperHudInventoryAreaWidget.h"
#include "UI/TunaSweeperHudItemInfoPanelWidget.h"
#include "UI/TunaSweeperHudQuickSlotBarWidget.h"
#include "UI/TunaSweeperHudTopReserveWidget.h"
#include "UI/TunaSweeperItemThumbnailSlotWidget.h"
#include "UI/TunaSweeperLootContainerWidget.h"
#include "UI/TunaSweeperPickupItemIconWidget.h"
#include "UI/TunaSweeperTempOpenLootTileEntryWidget.h"
#include "UI/TunaSweeperTempOpenLootWidget.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "Weapon/TunaSweeperWeapon.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperEditor, Log, All);

namespace TunaSweeperEditorSetup
{
	const FString GameInstanceTaskId = TEXT("2026-05-10_CreateGameInstanceBlueprint");
	const FString TopDownShooterTaskId = TEXT("2026-05-10_CreateTopDownShooterAssets");
	const FString InteractionTaskId = TEXT("2026-05-11_CreateComponentBasedInteractionAssetsAndPlaceActorsV2");
	const FString InteractionInputTaskId = TEXT("2026-05-11_SetInteractInputToFKey");
	const FString InteractionMarkerAlignmentTaskId = TEXT("2026-05-10_RebuildInteractionMarkerAlignmentV2");
	const FString TempOpenLootUiTaskId = TEXT("2026-05-10_CreateTempOpenLootTileViewAndIconsV2");
	const FString PickupItemAndSpawnerTaskId = TEXT("2026-05-11_CreatePickupItemAndSpawnerAssetsV3");
	const FString CommonGameHudTaskId = TEXT("2026-05-11_CreateCommonGameHudAssetsV3");
	const FString InventoryInputTaskId = TEXT("2026-05-11_AddInventoryInput");
	const FString LootContainerAndSpawnerTaskId = TEXT("2026-05-11_CreateLootContainerAndSpawnerAssetsV1");
	const FString CannedTunaIconImportTaskId = TEXT("2026-05-11_ImportCannedTunaIconV1");
	const FString GameInstanceAssetPath = TEXT("/Game/Core");
	const FString GameInstanceAssetName = TEXT("BP_TunaSweeperGameInstance");
	const FString GameModeAssetName = TEXT("BP_TunaSweeperGameMode");
	const FString PlayerAssetPath = TEXT("/Game/Characters/Player");
	const FString PlayerAssetName = TEXT("BP_TunaSweeperPlayerCharacter");
	const FString EnemyAssetPath = TEXT("/Game/Characters/Enemy");
	const FString EnemyAssetName = TEXT("BP_TunaSweeperEnemy");
	const FString WeaponAssetPath = TEXT("/Game/Weapons");
	const FString WeaponAssetName = TEXT("BP_TunaSweeperWeapon");
	const FString ProjectileAssetName = TEXT("BP_TunaSweeperProjectile");
	const FString InputAssetPath = TEXT("/Game/Input");
	const FString MoveActionName = TEXT("IA_Move");
	const FString FireActionName = TEXT("IA_Fire");
	const FString AimActionName = TEXT("IA_Aim");
	const FString InteractActionName = TEXT("IA_Interact");
	const FString InventoryActionName = TEXT("IA_Inventory");
	const FString MappingContextName = TEXT("IMC_Player");
	const FString UIAssetPath = TEXT("/Game/UI");
	const FString UIIconAssetPath = TEXT("/Game/UI/Icons");
	const FString InteractionMarkerAssetName = TEXT("WBP_InteractionMarker");
	const FString PickupItemIconWidgetAssetName = TEXT("WBP_PickupItemIcon");
	const FString TempOpenLootWidgetAssetName = TEXT("WBP_TempOpenLootTileView");
	const FString TempOpenLootEntryWidgetAssetName = TEXT("WBP_TempOpenLootTileEntry");
	const FString GameHudWidgetAssetName = TEXT("WBP_GameHud");
	const FString HudTopReserveWidgetAssetName = TEXT("WBP_HudTopReserve");
	const FString HudBottomStatusWidgetAssetName = TEXT("WBP_HudBottomStatus");
	const FString HudQuickSlotBarWidgetAssetName = TEXT("WBP_HudQuickSlotBar");
	const FString HudInventoryAreaWidgetAssetName = TEXT("WBP_HudInventoryArea");
	const FString HudItemInfoPanelWidgetAssetName = TEXT("WBP_HudItemInfoPanel");
	const FString HudExternalPanelWidgetAssetName = TEXT("WBP_HudExternalPanel");
	const FString ItemThumbnailSlotWidgetAssetName = TEXT("WBP_ItemThumbnailSlot");
	const FString LootContainerWidgetAssetName = TEXT("WBP_LootContainerPanel");
	const FString InteractionAssetPath = TEXT("/Game/Interaction");
	const FString DialogueInteractionAssetName = TEXT("BP_Interact_Dialogue");
	const FString PickupInteractionAssetName = TEXT("BP_Interact_Pickup");
	const FString OpenInteractionAssetName = TEXT("BP_Interact_Open");
	const FString PickupItemAssetName = TEXT("BP_PickupItem");
	const FString ItemSpawnInteractionAssetName = TEXT("BP_Interact_ItemSpawn");
	const FString LootContainerAssetName = TEXT("BP_LootContainer");
	const FString LootContainerSpawnInteractionAssetName = TEXT("BP_Interact_LootContainerSpawn");
	const FString RaidMapPackagePath = TEXT("/Game/RaidMap");

	FString GetGameInstanceObjectPath()
	{
		return FString::Printf(TEXT("%s/%s.%s"), *GameInstanceAssetPath, *GameInstanceAssetName, *GameInstanceAssetName);
	}

	FString GetGameInstanceClassPath()
	{
		return FString::Printf(TEXT("%s_C"), *GetGameInstanceObjectPath());
	}

	FString GetAssetObjectPath(const FString& AssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *AssetName, *AssetName);
	}

	FString GetAssetClassPath(const FString& AssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s_C"), *GetAssetObjectPath(AssetPath, AssetName));
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

		const FString PackageFileName = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(PackageFileName), true);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;

		return UPackage::SavePackage(Package, Asset, *PackageFileName, SaveArgs);
	}

	bool SetProjectGameInstanceToBlueprint()
	{
		UGameMapsSettings* GameMapsSettings = GetMutableDefault<UGameMapsSettings>();
		if (!GameMapsSettings)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Could not load GameMapsSettings."));
			return false;
		}

		const FSoftClassPath GameInstanceClassPath(GetGameInstanceClassPath());
		if (GameMapsSettings->GameInstanceClass.ToString() != GameInstanceClassPath.ToString())
		{
			GameMapsSettings->GameInstanceClass = GameInstanceClassPath;
			GameMapsSettings->SaveConfig();
		}

		const FString DefaultEngineIni = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini"));
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameInstanceClass"),
			*GameInstanceClassPath.ToString(),
			DefaultEngineIni);
		GConfig->Flush(false, DefaultEngineIni);

		FString SavedGameInstanceClass;
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameInstanceClass"),
			SavedGameInstanceClass,
			DefaultEngineIni);

		return SavedGameInstanceClass == GameInstanceClassPath.ToString();
	}

	UBlueprint* EnsureBlueprint(const FString& AssetPath, const FString& AssetName, UClass* ParentClass)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		if (UBlueprint* ExistingBlueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath))
		{
			if (!ExistingBlueprint->ParentClass || !ExistingBlueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s already exists, but it is not based on %s."), *ObjectPath, *GetNameSafe(ParentClass));
				return nullptr;
			}

			if (!ExistingBlueprint->GeneratedClass)
			{
				FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
			}

			return ExistingBlueprint;
		}

		UBlueprintFactory* BlueprintFactory = NewObject<UBlueprintFactory>();
		BlueprintFactory->ParentClass = ParentClass;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			AssetName,
			AssetPath,
			UBlueprint::StaticClass(),
			BlueprintFactory);

		UBlueprint* CreatedBlueprint = Cast<UBlueprint>(CreatedAsset);
		if (!CreatedBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(CreatedBlueprint);
		FAssetRegistryModule::AssetCreated(CreatedBlueprint);
		CreatedBlueprint->MarkPackageDirty();

		if (!SaveAsset(CreatedBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return CreatedBlueprint;
	}

	template <typename AssetType>
	AssetType* EnsureDataAsset(const FString& AssetPath, const FString& AssetName)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		if (AssetType* ExistingAsset = LoadObject<AssetType>(nullptr, *ObjectPath))
		{
			return ExistingAsset;
		}

		const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *AssetName);
		UPackage* Package = CreatePackage(*PackageName);
		AssetType* CreatedAsset = NewObject<AssetType>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional);
		if (!CreatedAsset)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(CreatedAsset);
		CreatedAsset->MarkPackageDirty();
		return CreatedAsset;
	}

	UInputAction* EnsureInputAction(const FString& AssetName, EInputActionValueType ValueType, EInputActionAccumulationBehavior AccumulationBehavior)
	{
		UInputAction* Action = EnsureDataAsset<UInputAction>(InputAssetPath, AssetName);
		if (!Action)
		{
			return nullptr;
		}

		Action->ValueType = ValueType;
		Action->AccumulationBehavior = AccumulationBehavior;
		Action->MarkPackageDirty();
		SaveAsset(Action);
		return Action;
	}

	void AddSwizzleModifier(FEnhancedActionKeyMapping& Mapping, UObject* Outer, EInputAxisSwizzle Order)
	{
		UInputModifierSwizzleAxis* SwizzleModifier = NewObject<UInputModifierSwizzleAxis>(Outer, NAME_None, RF_Transactional);
		SwizzleModifier->Order = Order;
		Mapping.Modifiers.Add(SwizzleModifier);
	}

	void AddNegateModifier(FEnhancedActionKeyMapping& Mapping, UObject* Outer)
	{
		UInputModifierNegate* NegateModifier = NewObject<UInputModifierNegate>(Outer, NAME_None, RF_Transactional);
		NegateModifier->bX = true;
		NegateModifier->bY = false;
		NegateModifier->bZ = false;
		Mapping.Modifiers.Add(NegateModifier);
	}

	UInputMappingContext* EnsureInputMappingContext(UInputAction* MoveAction, UInputAction* FireAction, UInputAction* AimAction)
	{
		if (!MoveAction || !FireAction || !AimAction)
		{
			return nullptr;
		}

		UInputMappingContext* MappingContext = EnsureDataAsset<UInputMappingContext>(InputAssetPath, MappingContextName);
		if (!MappingContext)
		{
			return nullptr;
		}

		MappingContext->UnmapAll();

		FEnhancedActionKeyMapping& WMapping = MappingContext->MapKey(MoveAction, EKeys::W);
		AddSwizzleModifier(WMapping, MappingContext, EInputAxisSwizzle::YXZ);

		FEnhancedActionKeyMapping& SMapping = MappingContext->MapKey(MoveAction, EKeys::S);
		AddNegateModifier(SMapping, MappingContext);
		AddSwizzleModifier(SMapping, MappingContext, EInputAxisSwizzle::YXZ);

		FEnhancedActionKeyMapping& AMapping = MappingContext->MapKey(MoveAction, EKeys::A);
		AddNegateModifier(AMapping, MappingContext);

		MappingContext->MapKey(MoveAction, EKeys::D);
		MappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
		MappingContext->MapKey(AimAction, EKeys::RightMouseButton);

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, fire, and aim input."));
		MappingContext->MarkPackageDirty();
		SaveAsset(MappingContext);
		return MappingContext;
	}

	bool HasInputMapping(const UInputMappingContext* MappingContext, const UInputAction* Action, const FKey& Key)
	{
		if (!MappingContext || !Action)
		{
			return false;
		}

		for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
		{
			if (Mapping.Action == Action && Mapping.Key == Key)
			{
				return true;
			}
		}

		return false;
	}

	bool EnsureInteractionInputAssets()
	{
		UInputAction* InteractAction = EnsureInputAction(
			InteractActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!InteractAction || !MappingContext)
		{
			return false;
		}

		MappingContext->UnmapKey(InteractAction, EKeys::E);

		if (!HasInputMapping(MappingContext, InteractAction, EKeys::F))
		{
			MappingContext->MapKey(InteractAction, EKeys::F);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, fire, aim, and interaction input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureInventoryInputAssets()
	{
		UInputAction* InventoryAction = EnsureInputAction(
			InventoryActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!InventoryAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, InventoryAction, EKeys::Tab))
		{
			MappingContext->MapKey(InventoryAction, EKeys::Tab);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, and inventory input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool ConfigureGameModeBlueprint(UBlueprint* GameModeBlueprint, UBlueprint* PlayerBlueprint)
	{
		if (!GameModeBlueprint || !PlayerBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(PlayerBlueprint);
		FKismetEditorUtilities::CompileBlueprint(GameModeBlueprint);

		UClass* PlayerClass = PlayerBlueprint->GeneratedClass;
		UClass* PlayerControllerClass = ATunaSweeperPlayerController::StaticClass();
		AGameModeBase* GameModeDefaults = GameModeBlueprint->GeneratedClass
			? Cast<AGameModeBase>(GameModeBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;

		if (!PlayerClass || !PlayerControllerClass || !GameModeDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure BP_TunaSweeperGameMode defaults."));
			return false;
		}

		GameModeBlueprint->Modify();
		GameModeDefaults->Modify();
		GameModeDefaults->DefaultPawnClass = PlayerClass;
		GameModeDefaults->PlayerControllerClass = PlayerControllerClass;
		GameModeBlueprint->MarkPackageDirty();

		return SaveAsset(GameModeBlueprint);
	}

	bool SetProjectGameModeToBlueprint()
	{
		const FString GameModeClassPath = GetAssetClassPath(GameInstanceAssetPath, GameModeAssetName);
		UGameMapsSettings::SetGlobalDefaultGameMode(GameModeClassPath);

		if (UGameMapsSettings* GameMapsSettings = GetMutableDefault<UGameMapsSettings>())
		{
			GameMapsSettings->SaveConfig();
		}

		const FString DefaultEngineIni = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini"));
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GlobalDefaultGameMode"),
			*GameModeClassPath,
			DefaultEngineIni);
		GConfig->Flush(false, DefaultEngineIni);

		FString SavedGameModeClass;
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GlobalDefaultGameMode"),
			SavedGameModeClass,
			DefaultEngineIni);

		return SavedGameModeClass == GameModeClassPath;
	}

	bool EnsureGameInstanceBlueprint()
	{
		const FString ObjectPath = GetGameInstanceObjectPath();

		if (UBlueprint* ExistingBlueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath))
		{
			if (ExistingBlueprint->ParentClass != UTunaSweeperGameInstance::StaticClass())
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s already exists, but its parent class is not UTunaSweeperGameInstance."), *ObjectPath);
				return false;
			}

			return SetProjectGameInstanceToBlueprint();
		}

		UBlueprintFactory* BlueprintFactory = NewObject<UBlueprintFactory>();
		BlueprintFactory->ParentClass = UTunaSweeperGameInstance::StaticClass();

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			GameInstanceAssetName,
			GameInstanceAssetPath,
			UBlueprint::StaticClass(),
			BlueprintFactory);

		UBlueprint* CreatedBlueprint = Cast<UBlueprint>(CreatedAsset);
		if (!CreatedBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(CreatedBlueprint);
		FAssetRegistryModule::AssetCreated(CreatedBlueprint);
		CreatedBlueprint->MarkPackageDirty();

		if (!SaveAsset(CreatedBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return false;
		}

		return SetProjectGameInstanceToBlueprint();
	}

	bool EnsureTopDownShooterAssets()
	{
		UInputAction* MoveAction = EnsureInputAction(MoveActionName, EInputActionValueType::Axis2D, EInputActionAccumulationBehavior::Cumulative);
		UInputAction* FireAction = EnsureInputAction(FireActionName, EInputActionValueType::Boolean, EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputAction* AimAction = EnsureInputAction(AimActionName, EInputActionValueType::Boolean, EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputMappingContext* MappingContext = EnsureInputMappingContext(MoveAction, FireAction, AimAction);

		UBlueprint* ProjectileBlueprint = EnsureBlueprint(WeaponAssetPath, ProjectileAssetName, ATunaSweeperProjectile::StaticClass());
		UBlueprint* WeaponBlueprint = EnsureBlueprint(WeaponAssetPath, WeaponAssetName, ATunaSweeperWeapon::StaticClass());
		UBlueprint* PlayerBlueprint = EnsureBlueprint(PlayerAssetPath, PlayerAssetName, ATunaSweeperTopDownCharacter::StaticClass());
		UBlueprint* EnemyBlueprint = EnsureBlueprint(EnemyAssetPath, EnemyAssetName, ATunaSweeperEnemyCharacter::StaticClass());
		UBlueprint* GameModeBlueprint = EnsureBlueprint(GameInstanceAssetPath, GameModeAssetName, ATunaSweeperGameMode::StaticClass());

		const bool bAssetsCreated =
			MoveAction &&
			FireAction &&
			AimAction &&
			MappingContext &&
			ProjectileBlueprint &&
			WeaponBlueprint &&
			PlayerBlueprint &&
			EnemyBlueprint &&
			GameModeBlueprint;

		if (!bAssetsCreated)
		{
			return false;
		}

		return ConfigureGameModeBlueprint(GameModeBlueprint, PlayerBlueprint) && SetProjectGameModeToBlueprint();
	}

	void RegisterWidgetVariable(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget)
	{
		if (!WidgetBlueprint || !Widget)
		{
			return;
		}

		Widget->bIsVariable = true;
		if (!WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(Widget->GetFName()))
		{
			WidgetBlueprint->OnVariableAdded(Widget->GetFName());
		}
	}

	void UnregisterWidgetVariable(UWidgetBlueprint* WidgetBlueprint, const FName& VariableName)
	{
		if (WidgetBlueprint && WidgetBlueprint->WidgetVariableNameToGuidMap.Contains(VariableName))
		{
			WidgetBlueprint->OnVariableRemoved(VariableName);
		}
	}

	void ClearWidgetTreeForRebuild(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> ExistingWidgets;
		WidgetBlueprint->WidgetTree->GetAllWidgets(ExistingWidgets);

		for (UWidget* ExistingWidget : ExistingWidgets)
		{
			if (ExistingWidget && ExistingWidget->bIsVariable)
			{
				UnregisterWidgetVariable(WidgetBlueprint, ExistingWidget->GetFName());
			}
		}

		if (WidgetBlueprint->WidgetTree->RootWidget)
		{
			WidgetBlueprint->WidgetTree->RemoveWidget(WidgetBlueprint->WidgetTree->RootWidget);
			WidgetBlueprint->WidgetTree->RootWidget = nullptr;
		}

		for (UWidget* ExistingWidget : ExistingWidgets)
		{
			if (ExistingWidget)
			{
				ExistingWidget->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}
	}

	void ConfigureTextBlock(UTextBlock* TextBlock, const FText& Text, const FLinearColor& Color, int32 FontSize)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = FontSize;

		TextBlock->SetText(Text);
		TextBlock->SetFont(FontInfo);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(ETextJustify::Center);
	}

	void ConfigureTextBlockLeft(UTextBlock* TextBlock, const FText& Text, const FLinearColor& Color, int32 FontSize)
	{
		ConfigureTextBlock(TextBlock, Text, Color, FontSize);
		if (TextBlock)
		{
			TextBlock->SetJustification(ETextJustify::Left);
		}
	}

	FSlateBrush MakeRoundedBoxBrush(const FVector2D& ImageSize, const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(5.0f, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FSlateBrush MakeCircularBrush(const FVector2D& ImageSize, const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
		Brush.OutlineSettings.Width = OutlineWidth;
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	const TArray<FString>& GetTempOpenLootIconAssetNames()
	{
		static const TArray<FString> IconAssetNames = {
			TEXT("T_UIIcon_Pistol"),
			TEXT("T_UIIcon_Rifle"),
			TEXT("T_UIIcon_Shotgun"),
			TEXT("T_UIIcon_PistolAmmo"),
			TEXT("T_UIIcon_RifleAmmo"),
			TEXT("T_UIIcon_ShotgunAmmo"),
			TEXT("T_UIIcon_CannedFood"),
			TEXT("T_UIIcon_CannedTuna"),
			TEXT("T_UIIcon_WaterBottle"),
			TEXT("T_UIIcon_EnergyBar"),
			TEXT("T_UIIcon_Bandage"),
			TEXT("T_UIIcon_FirstAidKit"),
			TEXT("T_UIIcon_Painkillers"),
			TEXT("T_UIIcon_Antibiotics"),
			TEXT("T_UIIcon_BodyArmor"),
			TEXT("T_UIIcon_Backpack"),
			TEXT("T_UIIcon_ValuablesCrate")
		};

		return IconAssetNames;
	}

	FString GetTempOpenLootIconSourcePath(const FString& IconAssetName)
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT(".."),
			TEXT("GeneratedImages"),
			TEXT("ItemIcons"),
			TEXT("Split"),
			IconAssetName + TEXT(".png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	void ConfigureImportedIconTexture(UTexture2D* Texture)
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

	bool EnsureTempOpenLootIconTextures()
	{
		TArray<FString> FilesToImport;

		for (const FString& IconAssetName : GetTempOpenLootIconAssetNames())
		{
			const FString ObjectPath = GetAssetObjectPath(UIIconAssetPath, IconAssetName);
			if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath))
			{
				ConfigureImportedIconTexture(ExistingTexture);
				continue;
			}

			const FString SourcePath = GetTempOpenLootIconSourcePath(IconAssetName);
			if (!FPaths::FileExists(SourcePath))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing temp open loot icon source: %s"), *SourcePath);
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
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import temp open loot icons."));
				return false;
			}
		}

		for (const FString& IconAssetName : GetTempOpenLootIconAssetNames())
		{
			UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *GetAssetObjectPath(UIIconAssetPath, IconAssetName));
			if (!Texture)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported temp open loot icon: %s"), *IconAssetName);
				return false;
			}

			ConfigureImportedIconTexture(Texture);
		}

		return true;
	}

	bool EnsureCannedTunaIconTexture()
	{
		const FString IconAssetName = TEXT("T_UIIcon_CannedTuna");
		if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *GetAssetObjectPath(UIIconAssetPath, IconAssetName)))
		{
			ConfigureImportedIconTexture(ExistingTexture);
			return true;
		}

		const FString SourcePath = GetTempOpenLootIconSourcePath(IconAssetName);
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

	void SetListViewEntryWidgetClass(UListViewBase* ListViewBase, TSubclassOf<UUserWidget> EntryWidgetClass)
	{
		if (!ListViewBase || !EntryWidgetClass)
		{
			return;
		}

		if (FClassProperty* EntryWidgetClassProperty = FindFProperty<FClassProperty>(UListViewBase::StaticClass(), TEXT("EntryWidgetClass")))
		{
			EntryWidgetClassProperty->SetPropertyValue_InContainer(ListViewBase, EntryWidgetClass);
		}
	}

	UWidgetBlueprint* EnsureWidgetBlueprint(const FString& AssetPath, const FString& AssetName, UClass* ParentClass)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		if (UWidgetBlueprint* ExistingBlueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath))
		{
			if (!ExistingBlueprint->ParentClass || !ExistingBlueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s already exists, but it is not based on %s."), *ObjectPath, *GetNameSafe(ParentClass));
				return nullptr;
			}

			if (!ExistingBlueprint->GeneratedClass)
			{
				FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
			}

			return ExistingBlueprint;
		}

		UWidgetBlueprintFactory* WidgetBlueprintFactory = NewObject<UWidgetBlueprintFactory>();
		WidgetBlueprintFactory->ParentClass = ParentClass;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			AssetName,
			AssetPath,
			UWidgetBlueprint::StaticClass(),
			WidgetBlueprintFactory);

		UWidgetBlueprint* CreatedBlueprint = Cast<UWidgetBlueprint>(CreatedAsset);
		if (!CreatedBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		FAssetRegistryModule::AssetCreated(CreatedBlueprint);
		CreatedBlueprint->MarkPackageDirty();
		return CreatedBlueprint;
	}

	bool BuildItemThumbnailSlotWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* SlotBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SlotBackground"));
		UVerticalBox* SlotStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SlotStack"));
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IconBox"));
		UImage* ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIconImage"));
		UTextBlock* ItemQuantityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemQuantityText"));
		UTextBlock* ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));

		if (!RootSizeBox || !SlotBackground || !SlotStack || !IconBox || !ItemIconImage || !ItemQuantityText || !ItemNameText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(96.0f);
		RootSizeBox->SetHeightOverride(116.0f);
		RootSizeBox->SetContent(SlotBackground);

		SlotBackground->SetPadding(FMargin(7.0f));
		SlotBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(96.0f, 116.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.24f, 0.27f, 0.31f, 0.95f),
			1.0f));
		SlotBackground->SetContent(SlotStack);

		IconBox->SetWidthOverride(76.0f);
		IconBox->SetHeightOverride(64.0f);
		IconBox->SetContent(ItemIconImage);
		ItemIconImage->SetColorAndOpacity(FLinearColor::White);

		UVerticalBoxSlot* IconSlot = SlotStack->AddChildToVerticalBox(IconBox);
		if (IconSlot)
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlock(ItemQuantityText, FText::FromString(TEXT("x1")), FLinearColor::White, 13);
		UVerticalBoxSlot* QuantitySlot = SlotStack->AddChildToVerticalBox(ItemQuantityText);
		if (QuantitySlot)
		{
			QuantitySlot->SetHorizontalAlignment(HAlign_Right);
			QuantitySlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlock(ItemNameText, FText::FromString(TEXT("Item")), FLinearColor(0.82f, 0.88f, 0.94f, 1.0f), 11);
		ItemNameText->SetAutoWrapText(true);
		UVerticalBoxSlot* NameSlot = SlotStack->AddChildToVerticalBox(ItemNameText);
		if (NameSlot)
		{
			NameSlot->SetHorizontalAlignment(HAlign_Fill);
			NameSlot->SetVerticalAlignment(VAlign_Bottom);
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		RegisterWidgetVariable(WidgetBlueprint, SlotBackground);
		RegisterWidgetVariable(WidgetBlueprint, ItemIconImage);
		RegisterWidgetVariable(WidgetBlueprint, ItemQuantityText);
		RegisterWidgetVariable(WidgetBlueprint, ItemNameText);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudTopReserveWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* ReservedBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ReservedBackground"));
		if (!RootSizeBox || !ReservedBackground)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetHeightOverride(88.0f);
		RootSizeBox->SetContent(ReservedBackground);

		ReservedBackground->SetPadding(FMargin(24.0f, 10.0f));
		ReservedBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(1280.0f, 88.0f),
			FLinearColor(0.005f, 0.006f, 0.008f, 0.35f),
			FLinearColor(0.15f, 0.17f, 0.19f, 0.35f),
			1.0f));

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudBottomStatusWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
		UVerticalBox* StatusStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("StatusStack"));
		UHorizontalBox* WeightRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WeightRow"));
		UTextBlock* WeightText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeightText"));
		UBorder* WeightWarningIcon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WeightWarningIcon"));
		UTextBlock* WeightWarningText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WeightWarningText"));
		UOverlay* GaugeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("GaugeOverlay"));
		UProgressBar* CarryWeightGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CarryWeightGauge"));
		USizeBox* MaxWeightTick = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MaxWeightTick"));
		UBorder* MaxWeightTickLine = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MaxWeightTickLine"));
		UHorizontalBox* VitalsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VitalsRow"));
		UTextBlock* HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
		UTextBlock* HungerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HungerText"));
		UTextBlock* HydrationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HydrationText"));

		if (!RootSizeBox || !PanelBackground || !StatusStack || !WeightRow || !WeightText || !WeightWarningIcon || !WeightWarningText ||
			!GaugeOverlay || !CarryWeightGauge || !MaxWeightTick || !MaxWeightTickLine || !VitalsRow || !HealthText || !HungerText || !HydrationText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(300.0f);
		RootSizeBox->SetHeightOverride(118.0f);
		RootSizeBox->SetContent(PanelBackground);

		PanelBackground->SetPadding(FMargin(14.0f, 10.0f));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(300.0f, 118.0f),
			FLinearColor(0.01f, 0.012f, 0.014f, 0.82f),
			FLinearColor(0.22f, 0.25f, 0.29f, 0.85f),
			1.0f));
		PanelBackground->SetContent(StatusStack);

		ConfigureTextBlockLeft(WeightText, FText::FromString(TEXT("0/50 kg")), FLinearColor::White, 18);
		UHorizontalBoxSlot* WeightTextSlot = WeightRow->AddChildToHorizontalBox(WeightText);
		if (WeightTextSlot)
		{
			WeightTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			WeightTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		WeightWarningIcon->SetVisibility(ESlateVisibility::Hidden);
		WeightWarningIcon->SetPadding(FMargin(5.0f, 2.0f));
		WeightWarningIcon->SetBrush(MakeRoundedBoxBrush(
			FVector2D(34.0f, 24.0f),
			FLinearColor(0.9f, 0.18f, 0.08f, 0.95f),
			FLinearColor(1.0f, 0.75f, 0.25f, 1.0f),
			1.0f));
		ConfigureTextBlock(WeightWarningText, FText::FromString(TEXT("KG")), FLinearColor::White, 12);
		WeightWarningIcon->SetContent(WeightWarningText);
		UHorizontalBoxSlot* WarningSlot = WeightRow->AddChildToHorizontalBox(WeightWarningIcon);
		if (WarningSlot)
		{
			WarningSlot->SetHorizontalAlignment(HAlign_Right);
			WarningSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* WeightRowSlot = StatusStack->AddChildToVerticalBox(WeightRow);
		if (WeightRowSlot)
		{
			WeightRowSlot->SetHorizontalAlignment(HAlign_Fill);
			WeightRowSlot->SetVerticalAlignment(VAlign_Top);
		}

		CarryWeightGauge->SetPercent(0.0f);
		CarryWeightGauge->SetFillColorAndOpacity(FLinearColor(0.95f, 0.82f, 0.18f, 1.0f));
		UOverlaySlot* GaugeSlot = GaugeOverlay->AddChildToOverlay(CarryWeightGauge);
		if (GaugeSlot)
		{
			GaugeSlot->SetHorizontalAlignment(HAlign_Fill);
			GaugeSlot->SetVerticalAlignment(VAlign_Fill);
		}

		MaxWeightTick->SetWidthOverride(2.0f);
		MaxWeightTick->SetHeightOverride(18.0f);
		MaxWeightTickLine->SetBrush(MakeRoundedBoxBrush(
			FVector2D(2.0f, 18.0f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.8f),
			FLinearColor::Transparent,
			0.0f));
		MaxWeightTick->SetContent(MaxWeightTickLine);
		UOverlaySlot* TickSlot = GaugeOverlay->AddChildToOverlay(MaxWeightTick);
		if (TickSlot)
		{
			TickSlot->SetHorizontalAlignment(HAlign_Center);
			TickSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* GaugeOverlaySlot = StatusStack->AddChildToVerticalBox(GaugeOverlay);
		if (GaugeOverlaySlot)
		{
			GaugeOverlaySlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 10.0f));
			GaugeOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			GaugeOverlaySlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlockLeft(HealthText, FText::FromString(TEXT("HP 100")), FLinearColor(0.98f, 0.38f, 0.32f, 1.0f), 15);
		ConfigureTextBlockLeft(HungerText, FText::FromString(TEXT("Food 100")), FLinearColor(0.95f, 0.72f, 0.28f, 1.0f), 15);
		ConfigureTextBlockLeft(HydrationText, FText::FromString(TEXT("Water 100")), FLinearColor(0.35f, 0.72f, 0.98f, 1.0f), 15);

		for (UTextBlock* VitalsText : { HealthText, HungerText, HydrationText })
		{
			UHorizontalBoxSlot* VitalsSlot = VitalsRow->AddChildToHorizontalBox(VitalsText);
			if (VitalsSlot)
			{
				VitalsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				VitalsSlot->SetVerticalAlignment(VAlign_Center);
			}
		}

		UVerticalBoxSlot* VitalsRowSlot = StatusStack->AddChildToVerticalBox(VitalsRow);
		if (VitalsRowSlot)
		{
			VitalsRowSlot->SetHorizontalAlignment(HAlign_Fill);
			VitalsRowSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		RegisterWidgetVariable(WidgetBlueprint, WeightText);
		RegisterWidgetVariable(WidgetBlueprint, HealthText);
		RegisterWidgetVariable(WidgetBlueprint, HungerText);
		RegisterWidgetVariable(WidgetBlueprint, HydrationText);
		RegisterWidgetVariable(WidgetBlueprint, CarryWeightGauge);
		RegisterWidgetVariable(WidgetBlueprint, WeightWarningIcon);
		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, PanelBackground);
		RegisterWidgetVariable(WidgetBlueprint, StatusStack);
		RegisterWidgetVariable(WidgetBlueprint, WeightRow);
		RegisterWidgetVariable(WidgetBlueprint, GaugeOverlay);
		RegisterWidgetVariable(WidgetBlueprint, MaxWeightTick);
		RegisterWidgetVariable(WidgetBlueprint, VitalsRow);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudQuickSlotBarWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UHorizontalBox* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotRow"));
		if (!RootSizeBox || !SlotRow)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetHeightOverride(118.0f);
		RootSizeBox->SetContent(SlotRow);

		const FString DefaultIconPaths[8] = {
			TEXT("/Game/UI/Icons/T_UIIcon_Pistol.T_UIIcon_Pistol"),
			TEXT("/Game/UI/Icons/T_UIIcon_Rifle.T_UIIcon_Rifle"),
			TEXT("/Game/UI/Icons/T_UIIcon_Bandage.T_UIIcon_Bandage"),
			TEXT("/Game/UI/Icons/T_UIIcon_FirstAidKit.T_UIIcon_FirstAidKit"),
			TEXT("/Game/UI/Icons/T_UIIcon_CannedFood.T_UIIcon_CannedFood"),
			TEXT("/Game/UI/Icons/T_UIIcon_WaterBottle.T_UIIcon_WaterBottle"),
			TEXT("/Game/UI/Icons/T_UIIcon_Painkillers.T_UIIcon_Painkillers"),
			TEXT("/Game/UI/Icons/T_UIIcon_EnergyBar.T_UIIcon_EnergyBar")
		};

		for (int32 SlotNumber = 1; SlotNumber <= 8; ++SlotNumber)
		{
			const bool bWeaponSlot = SlotNumber <= 2;
			const float SlotSize = bWeaponSlot ? 82.0f : 56.0f;
			const float IconSize = bWeaponSlot ? 68.0f : 42.0f;

			USizeBox* SlotSizeBox = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*FString::Printf(TEXT("QuickSlot%dSizeBox"), SlotNumber)));
			UBorder* SlotBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("QuickSlot%dBackground"), SlotNumber)));
			UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				FName(*FString::Printf(TEXT("QuickSlot%dOverlay"), SlotNumber)));
			UImage* SlotIcon = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				FName(*FString::Printf(TEXT("QuickSlot%dIcon"), SlotNumber)));
			UTextBlock* SlotNumberText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*FString::Printf(TEXT("QuickSlot%dNumberText"), SlotNumber)));
			UBorder* SelectionFrame = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("QuickSlot%dSelectionFrame"), SlotNumber)));

			if (!SlotSizeBox || !SlotBackground || !SlotOverlay || !SlotIcon || !SlotNumberText || !SelectionFrame)
			{
				return false;
			}

			SlotSizeBox->SetWidthOverride(SlotSize);
			SlotSizeBox->SetHeightOverride(SlotSize);
			SlotSizeBox->SetContent(SlotBackground);

			SlotBackground->SetPadding(FMargin(4.0f));
			SlotBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(SlotSize, SlotSize),
				FLinearColor(0.012f, 0.014f, 0.016f, 0.88f),
				FLinearColor(0.22f, 0.25f, 0.3f, 0.95f),
				1.0f));
			SlotBackground->SetContent(SlotOverlay);

			if (UTexture2D* DefaultIcon = LoadObject<UTexture2D>(nullptr, *DefaultIconPaths[SlotNumber - 1]))
			{
				SlotIcon->SetBrushFromTexture(DefaultIcon, true);
			}
			SlotIcon->SetDesiredSizeOverride(FVector2D(IconSize, IconSize));
			SlotIcon->SetColorAndOpacity(FLinearColor::White);

			UOverlaySlot* IconSlot = SlotOverlay->AddChildToOverlay(SlotIcon);
			if (IconSlot)
			{
				IconSlot->SetHorizontalAlignment(HAlign_Center);
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetPadding(FMargin((SlotSize - IconSize) * 0.25f));
			}

			ConfigureTextBlock(SlotNumberText, FText::AsNumber(SlotNumber), FLinearColor(0.82f, 0.88f, 0.94f, 1.0f), bWeaponSlot ? 16 : 13);
			UOverlaySlot* NumberSlot = SlotOverlay->AddChildToOverlay(SlotNumberText);
			if (NumberSlot)
			{
				NumberSlot->SetHorizontalAlignment(HAlign_Left);
				NumberSlot->SetVerticalAlignment(VAlign_Top);
				NumberSlot->SetPadding(FMargin(3.0f, 1.0f, 0.0f, 0.0f));
			}

			SelectionFrame->SetVisibility(SlotNumber == 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
			SelectionFrame->SetBrush(MakeRoundedBoxBrush(
				FVector2D(SlotSize, SlotSize),
				FLinearColor::Transparent,
				FLinearColor(0.98f, 0.82f, 0.22f, 1.0f),
				2.0f));
			UOverlaySlot* SelectionSlot = SlotOverlay->AddChildToOverlay(SelectionFrame);
			if (SelectionSlot)
			{
				SelectionSlot->SetHorizontalAlignment(HAlign_Fill);
				SelectionSlot->SetVerticalAlignment(VAlign_Fill);
			}

			UHorizontalBoxSlot* SlotRowSlot = SlotRow->AddChildToHorizontalBox(SlotSizeBox);
			if (SlotRowSlot)
			{
				SlotRowSlot->SetPadding(FMargin(SlotNumber == 1 ? 0.0f : 8.0f, 0.0f, 0.0f, 0.0f));
				SlotRowSlot->SetVerticalAlignment(VAlign_Bottom);
			}

			RegisterWidgetVariable(WidgetBlueprint, SlotIcon);
			RegisterWidgetVariable(WidgetBlueprint, SelectionFrame);
		}

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	UBorder* BuildHudSimplePanel(
		UWidgetTree* WidgetTree,
		const FName& PanelName,
		const FText& Title,
		const FVector2D& PanelSize,
		const FLinearColor& AccentColor)
	{
		if (!WidgetTree)
		{
			return nullptr;
		}

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), PanelName);
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), FName(*(PanelName.ToString() + TEXT("Stack"))));
		UTextBlock* TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(PanelName.ToString() + TEXT("TitleText"))));
		if (!Panel || !PanelStack || !TitleText)
		{
			return nullptr;
		}

		Panel->SetPadding(FMargin(14.0f));
		Panel->SetBrush(MakeRoundedBoxBrush(
			PanelSize,
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			AccentColor,
			1.0f));
		Panel->SetContent(PanelStack);

		ConfigureTextBlockLeft(TitleText, Title, FLinearColor::White, 18);
		UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(TitleText);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Top);
		}

		return Panel;
	}

	bool BuildHudInventoryAreaWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !EntryWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* InventoryPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryPanel"));
		UVerticalBox* InventoryStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryStack"));
		UTextBlock* InventoryTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryTitleText"));
		UHorizontalBox* EquipmentAndBagRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("EquipmentAndBagRow"));
		UTileView* EquipmentReserveTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("EquipmentReserveTileView"));
		UBorder* AuxiliaryBagPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AuxiliaryBagPanel"));
		UTileView* AuxiliaryBagTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("AuxiliaryBagTileView"));
		UTileView* InventoryTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("InventoryTileView"));

		if (!RootSizeBox || !InventoryPanel || !InventoryStack || !InventoryTitleText || !EquipmentAndBagRow ||
			!EquipmentReserveTileView || !AuxiliaryBagPanel || !AuxiliaryBagTileView || !InventoryTileView)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(500.0f);
		RootSizeBox->SetContent(InventoryPanel);

		InventoryPanel->SetPadding(FMargin(14.0f));
		InventoryPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(500.0f, 620.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.28f, 0.36f, 0.44f, 1.0f),
			1.0f));
		InventoryPanel->SetContent(InventoryStack);

		ConfigureTextBlockLeft(InventoryTitleText, FText::FromString(TEXT("Inventory")), FLinearColor::White, 18);
		UVerticalBoxSlot* TitleSlot = InventoryStack->AddChildToVerticalBox(InventoryTitleText);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Top);
		}

		EquipmentReserveTileView->SetEntryWidth(82.0f);
		EquipmentReserveTileView->SetEntryHeight(82.0f);
		SetListViewEntryWidgetClass(EquipmentReserveTileView, EntryWidgetClass);
		UHorizontalBoxSlot* EquipmentSlot = EquipmentAndBagRow->AddChildToHorizontalBox(EquipmentReserveTileView);
		if (EquipmentSlot)
		{
			EquipmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			EquipmentSlot->SetVerticalAlignment(VAlign_Top);
		}

		AuxiliaryBagPanel->SetPadding(FMargin(6.0f));
		AuxiliaryBagPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(98.0f, 180.0f),
			FLinearColor(0.02f, 0.025f, 0.03f, 0.88f),
			FLinearColor(0.28f, 0.44f, 0.36f, 1.0f),
			1.0f));
		AuxiliaryBagTileView->SetEntryWidth(82.0f);
		AuxiliaryBagTileView->SetEntryHeight(82.0f);
		SetListViewEntryWidgetClass(AuxiliaryBagTileView, EntryWidgetClass);
		AuxiliaryBagPanel->SetContent(AuxiliaryBagTileView);
		UHorizontalBoxSlot* BagSlot = EquipmentAndBagRow->AddChildToHorizontalBox(AuxiliaryBagPanel);
		if (BagSlot)
		{
			BagSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
			BagSlot->SetVerticalAlignment(VAlign_Top);
		}

		UVerticalBoxSlot* ReserveRowSlot = InventoryStack->AddChildToVerticalBox(EquipmentAndBagRow);
		if (ReserveRowSlot)
		{
			ReserveRowSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 12.0f));
			ReserveRowSlot->SetHorizontalAlignment(HAlign_Fill);
			ReserveRowSlot->SetVerticalAlignment(VAlign_Top);
		}

		InventoryTileView->SetEntryWidth(96.0f);
		InventoryTileView->SetEntryHeight(116.0f);
		SetListViewEntryWidgetClass(InventoryTileView, EntryWidgetClass);
		UVerticalBoxSlot* InventoryTileSlot = InventoryStack->AddChildToVerticalBox(InventoryTileView);
		if (InventoryTileSlot)
		{
			InventoryTileSlot->SetHorizontalAlignment(HAlign_Fill);
			InventoryTileSlot->SetVerticalAlignment(VAlign_Fill);
			InventoryTileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		RegisterWidgetVariable(WidgetBlueprint, InventoryPanel);
		RegisterWidgetVariable(WidgetBlueprint, AuxiliaryBagPanel);
		RegisterWidgetVariable(WidgetBlueprint, EquipmentReserveTileView);
		RegisterWidgetVariable(WidgetBlueprint, AuxiliaryBagTileView);
		RegisterWidgetVariable(WidgetBlueprint, InventoryTileView);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudItemInfoPanelWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
		UTextBlock* SelectedItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemNameText"));
		UTextBlock* SelectedItemDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemDescriptionText"));
		UBorder* ModdingPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModdingPanel"));
		UTextBlock* ModdingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModdingText"));

		if (!RootSizeBox || !PanelBackground || !PanelStack || !SelectedItemNameText || !SelectedItemDescriptionText || !ModdingPanel || !ModdingText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(330.0f);
		RootSizeBox->SetContent(PanelBackground);

		PanelBackground->SetPadding(FMargin(14.0f));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(330.0f, 620.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.36f, 0.34f, 0.54f, 1.0f),
			1.0f));
		PanelBackground->SetContent(PanelStack);

		ConfigureTextBlockLeft(SelectedItemNameText, FText::FromString(TEXT("No Item")), FLinearColor::White, 20);
		UVerticalBoxSlot* NameSlot = PanelStack->AddChildToVerticalBox(SelectedItemNameText);
		if (NameSlot)
		{
			NameSlot->SetHorizontalAlignment(HAlign_Fill);
			NameSlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlockLeft(SelectedItemDescriptionText, FText::GetEmpty(), FLinearColor(0.75f, 0.8f, 0.86f, 1.0f), 15);
		SelectedItemDescriptionText->SetAutoWrapText(true);
		UVerticalBoxSlot* DescriptionSlot = PanelStack->AddChildToVerticalBox(SelectedItemDescriptionText);
		if (DescriptionSlot)
		{
			DescriptionSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
			DescriptionSlot->SetVerticalAlignment(VAlign_Top);
			DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ModdingPanel->SetVisibility(ESlateVisibility::Collapsed);
		ModdingPanel->SetPadding(FMargin(10.0f));
		ModdingPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(300.0f, 120.0f),
			FLinearColor(0.03f, 0.034f, 0.04f, 0.92f),
			FLinearColor(0.56f, 0.50f, 0.78f, 1.0f),
			1.0f));
		ConfigureTextBlockLeft(ModdingText, FText::FromString(TEXT("Modding")), FLinearColor::White, 16);
		ModdingPanel->SetContent(ModdingText);

		UVerticalBoxSlot* ModdingSlot = PanelStack->AddChildToVerticalBox(ModdingPanel);
		if (ModdingSlot)
		{
			ModdingSlot->SetHorizontalAlignment(HAlign_Fill);
			ModdingSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		RegisterWidgetVariable(WidgetBlueprint, SelectedItemNameText);
		RegisterWidgetVariable(WidgetBlueprint, SelectedItemDescriptionText);
		RegisterWidgetVariable(WidgetBlueprint, ModdingPanel);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildLootContainerWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !EntryWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
		UTextBlock* ContainerTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContainerTitleText"));
		UTileView* ContainerTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("ContainerTileView"));

		if (!RootSizeBox || !PanelBackground || !PanelStack || !ContainerTitleText || !ContainerTileView)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(500.0f);
		RootSizeBox->SetHeightOverride(306.0f);
		RootSizeBox->SetContent(PanelBackground);

		PanelBackground->SetPadding(FMargin(14.0f));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(500.0f, 306.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.44f, 0.34f, 0.26f, 1.0f),
			1.0f));
		PanelBackground->SetContent(PanelStack);

		ConfigureTextBlockLeft(ContainerTitleText, FText::FromString(TEXT("Container")), FLinearColor::White, 18);
		UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(ContainerTitleText);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Top);
		}

		ContainerTileView->SetEntryWidth(96.0f);
		ContainerTileView->SetEntryHeight(116.0f);
		SetListViewEntryWidgetClass(ContainerTileView, EntryWidgetClass);
		UVerticalBoxSlot* TileViewSlot = PanelStack->AddChildToVerticalBox(ContainerTileView);
		if (TileViewSlot)
		{
			TileViewSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			TileViewSlot->SetHorizontalAlignment(HAlign_Fill);
			TileViewSlot->SetVerticalAlignment(VAlign_Fill);
			TileViewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, ContainerTitleText);
		RegisterWidgetVariable(WidgetBlueprint, ContainerTileView);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudExternalPanelWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> LootContainerWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !LootContainerWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UOverlay* PanelOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("PanelOverlay"));
		UOverlay* LootingBoxPanel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LootingBoxPanel"));
		UUserWidget* LootContainerWidget = WidgetTree->ConstructWidget<UUserWidget>(LootContainerWidgetClass, TEXT("LootContainerWidget"));
		UBorder* ShopPanel = BuildHudSimplePanel(
			WidgetTree,
			TEXT("ShopPanel"),
			FText::FromString(TEXT("Shop")),
			FVector2D(420.0f, 620.0f),
			FLinearColor(0.28f, 0.40f, 0.50f, 1.0f));
		UBorder* StoragePanel = BuildHudSimplePanel(
			WidgetTree,
			TEXT("StoragePanel"),
			FText::FromString(TEXT("Storage")),
			FVector2D(420.0f, 620.0f),
			FLinearColor(0.38f, 0.42f, 0.32f, 1.0f));

		if (!RootSizeBox || !PanelOverlay || !LootingBoxPanel || !LootContainerWidget || !ShopPanel || !StoragePanel)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(500.0f);
		RootSizeBox->SetContent(PanelOverlay);

		UOverlaySlot* LootContainerSlot = LootingBoxPanel->AddChildToOverlay(LootContainerWidget);
		if (LootContainerSlot)
		{
			LootContainerSlot->SetHorizontalAlignment(HAlign_Fill);
			LootContainerSlot->SetVerticalAlignment(VAlign_Top);
		}

		TArray<UWidget*> ExternalPanels = { LootingBoxPanel, ShopPanel, StoragePanel };
		for (UWidget* Panel : ExternalPanels)
		{
			Panel->SetVisibility(ESlateVisibility::Collapsed);
			UOverlaySlot* PanelSlot = PanelOverlay->AddChildToOverlay(Panel);
			if (PanelSlot)
			{
				PanelSlot->SetHorizontalAlignment(HAlign_Fill);
				PanelSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}

		RegisterWidgetVariable(WidgetBlueprint, LootingBoxPanel);
		RegisterWidgetVariable(WidgetBlueprint, ShopPanel);
		RegisterWidgetVariable(WidgetBlueprint, StoragePanel);
		RegisterWidgetVariable(WidgetBlueprint, LootContainerWidget);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildGameHudWidgetTree(
		UWidgetBlueprint* WidgetBlueprint,
		TSubclassOf<UUserWidget> TopReserveWidgetClass,
		TSubclassOf<UUserWidget> BottomStatusWidgetClass,
		TSubclassOf<UUserWidget> QuickSlotBarWidgetClass,
		TSubclassOf<UUserWidget> InventoryAreaWidgetClass,
		TSubclassOf<UUserWidget> ItemInfoPanelWidgetClass,
		TSubclassOf<UUserWidget> ExternalPanelWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !TopReserveWidgetClass || !BottomStatusWidgetClass ||
			!QuickSlotBarWidgetClass || !InventoryAreaWidgetClass || !ItemInfoPanelWidgetClass || !ExternalPanelWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UUserWidget* TopStatusReserveWidget = WidgetTree->ConstructWidget<UUserWidget>(TopReserveWidgetClass, TEXT("TopStatusReserveWidget"));
		UHorizontalBox* CenterContentPanel = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CenterContentPanel"));
		UUserWidget* InventoryAreaWidget = WidgetTree->ConstructWidget<UUserWidget>(InventoryAreaWidgetClass, TEXT("InventoryAreaWidget"));
		UUserWidget* ItemInfoPanelWidget = WidgetTree->ConstructWidget<UUserWidget>(ItemInfoPanelWidgetClass, TEXT("ItemInfoPanelWidget"));
		UUserWidget* ExternalPanelWidget = WidgetTree->ConstructWidget<UUserWidget>(ExternalPanelWidgetClass, TEXT("ExternalPanelWidget"));
		UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomRow"));
		UUserWidget* BottomStatusWidget = WidgetTree->ConstructWidget<UUserWidget>(BottomStatusWidgetClass, TEXT("BottomStatusWidget"));
		USizeBox* BottomGap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BottomGap"));
		UUserWidget* QuickSlotBarWidget = WidgetTree->ConstructWidget<UUserWidget>(QuickSlotBarWidgetClass, TEXT("QuickSlotBarWidget"));

		if (!RootCanvas || !TopStatusReserveWidget || !CenterContentPanel || !InventoryAreaWidget || !ItemInfoPanelWidget ||
			!ExternalPanelWidget || !BottomRow || !BottomStatusWidget || !BottomGap || !QuickSlotBarWidget)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		UCanvasPanelSlot* TopSlot = RootCanvas->AddChildToCanvas(TopStatusReserveWidget);
		if (TopSlot)
		{
			TopSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
			TopSlot->SetOffsets(FMargin(24.0f, 16.0f, 24.0f, 88.0f));
			TopSlot->SetAlignment(FVector2D(0.0f, 0.0f));
		}

		CenterContentPanel->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(CenterContentPanel);
		if (CenterSlot)
		{
			CenterSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterSlot->SetPosition(FVector2D(0.0f, -20.0f));
			CenterSlot->SetSize(FVector2D(1380.0f, 620.0f));
		}

		UHorizontalBoxSlot* InventorySlot = CenterContentPanel->AddChildToHorizontalBox(InventoryAreaWidget);
		if (InventorySlot)
		{
			InventorySlot->SetVerticalAlignment(VAlign_Fill);
			InventorySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		UHorizontalBoxSlot* ItemInfoSlot = CenterContentPanel->AddChildToHorizontalBox(ItemInfoPanelWidget);
		if (ItemInfoSlot)
		{
			ItemInfoSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
			ItemInfoSlot->SetVerticalAlignment(VAlign_Fill);
			ItemInfoSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		UHorizontalBoxSlot* ExternalSlot = CenterContentPanel->AddChildToHorizontalBox(ExternalPanelWidget);
		if (ExternalSlot)
		{
			ExternalSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
			ExternalSlot->SetVerticalAlignment(VAlign_Fill);
			ExternalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		UCanvasPanelSlot* BottomSlot = RootCanvas->AddChildToCanvas(BottomRow);
		if (BottomSlot)
		{
			BottomSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
			BottomSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			BottomSlot->SetPosition(FVector2D(0.0f, -20.0f));
			BottomSlot->SetSize(FVector2D(980.0f, 124.0f));
		}

		UHorizontalBoxSlot* BottomStatusSlot = BottomRow->AddChildToHorizontalBox(BottomStatusWidget);
		if (BottomStatusSlot)
		{
			BottomStatusSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		BottomGap->SetWidthOverride(34.0f);
		UHorizontalBoxSlot* GapSlot = BottomRow->AddChildToHorizontalBox(BottomGap);
		if (GapSlot)
		{
			GapSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UHorizontalBoxSlot* QuickSlotSlot = BottomRow->AddChildToHorizontalBox(QuickSlotBarWidget);
		if (QuickSlotSlot)
		{
			QuickSlotSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		RegisterWidgetVariable(WidgetBlueprint, TopStatusReserveWidget);
		RegisterWidgetVariable(WidgetBlueprint, BottomStatusWidget);
		RegisterWidgetVariable(WidgetBlueprint, QuickSlotBarWidget);
		RegisterWidgetVariable(WidgetBlueprint, CenterContentPanel);
		RegisterWidgetVariable(WidgetBlueprint, InventoryAreaWidget);
		RegisterWidgetVariable(WidgetBlueprint, ItemInfoPanelWidget);
		RegisterWidgetVariable(WidgetBlueprint, ExternalPanelWidget);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildPickupItemIconWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UImage* ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIconImage"));
		if (!RootSizeBox || !ItemIconImage)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(96.0f);
		RootSizeBox->SetHeightOverride(96.0f);
		RootSizeBox->SetContent(ItemIconImage);

		if (UTexture2D* DefaultIconTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Icons/T_UIIcon_Pistol.T_UIIcon_Pistol")))
		{
			ItemIconImage->SetBrushFromTexture(DefaultIconTexture, true);
		}
		ItemIconImage->SetColorAndOpacity(FLinearColor::White);
		ItemIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		ItemIconImage->SetOpacity(1.0f);

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, ItemIconImage);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildTempOpenLootEntryWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		USizeBox* RootSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RootSizeBox"));
		UBorder* TileBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("TileBackground"));
		UVerticalBox* TileStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("TileStack"));
		USizeBox* IconSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IconSizeBox"));
		UImage* TempItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("TempItemIconImage"));
		UTextBlock* TempItemQuantityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TempItemQuantityText"));

		if (!RootSizeBox || !TileBackground || !TileStack || !IconSizeBox || !TempItemIconImage || !TempItemQuantityText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(128.0f);
		RootSizeBox->SetHeightOverride(150.0f);
		RootSizeBox->SetContent(TileBackground);

		TileBackground->SetPadding(FMargin(8.0f));
		TileBackground->SetBrush(MakeRoundedBoxBrush(FVector2D(128.0f, 150.0f), FLinearColor(0.015f, 0.018f, 0.02f, 0.92f), FLinearColor(0.25f, 0.28f, 0.32f, 1.0f), 1.0f));
		TileBackground->SetContent(TileStack);

		IconSizeBox->SetWidthOverride(104.0f);
		IconSizeBox->SetHeightOverride(104.0f);
		IconSizeBox->SetContent(TempItemIconImage);

		UVerticalBoxSlot* IconSlot = TileStack->AddChildToVerticalBox(IconSizeBox);
		if (IconSlot)
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlock(TempItemQuantityText, FText::FromString(TEXT("x1")), FLinearColor::White, 18);
		UVerticalBoxSlot* QuantitySlot = TileStack->AddChildToVerticalBox(TempItemQuantityText);
		if (QuantitySlot)
		{
			QuantitySlot->SetHorizontalAlignment(HAlign_Center);
			QuantitySlot->SetVerticalAlignment(VAlign_Center);
			QuantitySlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
		}

		RegisterWidgetVariable(WidgetBlueprint, TempItemIconImage);
		RegisterWidgetVariable(WidgetBlueprint, TempItemQuantityText);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildTempOpenLootTileViewWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !EntryWidgetClass)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
		UTextBlock* HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HeaderText"));
		UButton* TempCloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("TempCloseButton"));
		UTextBlock* CloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseButtonText"));
		UTileView* TempLootTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("TempLootTileView"));

		if (!RootCanvas || !PanelBackground || !PanelStack || !HeaderRow || !HeaderText || !TempCloseButton || !CloseButtonText || !TempLootTileView)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBackground);
		if (PanelSlot)
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetPosition(FVector2D::ZeroVector);
			PanelSlot->SetSize(FVector2D(620.0f, 760.0f));
		}

		PanelBackground->SetPadding(FMargin(18.0f));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(FVector2D(620.0f, 760.0f), FLinearColor(0.015f, 0.016f, 0.018f, 0.96f), FLinearColor(0.18f, 0.2f, 0.23f, 1.0f), 1.0f));
		PanelBackground->SetContent(PanelStack);

		ConfigureTextBlock(HeaderText, FText::FromString(TEXT("Temp Loot Test")), FLinearColor::White, 22);
		UHorizontalBoxSlot* HeaderTextSlot = HeaderRow->AddChildToHorizontalBox(HeaderText);
		if (HeaderTextSlot)
		{
			HeaderTextSlot->SetHorizontalAlignment(HAlign_Left);
			HeaderTextSlot->SetVerticalAlignment(VAlign_Center);
			HeaderTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ConfigureTextBlock(CloseButtonText, FText::FromString(TEXT("Close")), FLinearColor::White, 16);
		TempCloseButton->SetContent(CloseButtonText);
		UHorizontalBoxSlot* CloseButtonSlot = HeaderRow->AddChildToHorizontalBox(TempCloseButton);
		if (CloseButtonSlot)
		{
			CloseButtonSlot->SetHorizontalAlignment(HAlign_Right);
			CloseButtonSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* HeaderSlot = PanelStack->AddChildToVerticalBox(HeaderRow);
		if (HeaderSlot)
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
			HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
			HeaderSlot->SetVerticalAlignment(VAlign_Top);
		}

		TempLootTileView->SetEntryWidth(128.0f);
		TempLootTileView->SetEntryHeight(150.0f);
		SetListViewEntryWidgetClass(TempLootTileView, EntryWidgetClass);

		UVerticalBoxSlot* TileViewSlot = PanelStack->AddChildToVerticalBox(TempLootTileView);
		if (TileViewSlot)
		{
			TileViewSlot->SetHorizontalAlignment(HAlign_Fill);
			TileViewSlot->SetVerticalAlignment(VAlign_Fill);
			TileViewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		RegisterWidgetVariable(WidgetBlueprint, TempLootTileView);
		RegisterWidgetVariable(WidgetBlueprint, TempCloseButton);
		RegisterWidgetVariable(WidgetBlueprint, CloseButtonText);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool EnsureTempOpenLootTileViewAssets()
	{
		if (!EnsureTempOpenLootIconTextures())
		{
			return false;
		}

		UWidgetBlueprint* EntryWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			TempOpenLootEntryWidgetAssetName,
			UTunaSweeperTempOpenLootTileEntryWidget::StaticClass());
		if (!EntryWidgetBlueprint || !BuildTempOpenLootEntryWidgetTree(EntryWidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(EntryWidgetBlueprint);
		EntryWidgetBlueprint->MarkPackageDirty();
		if (!SaveAsset(EntryWidgetBlueprint))
		{
			return false;
		}

		UWidgetBlueprint* TileViewWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			TempOpenLootWidgetAssetName,
			UTunaSweeperTempOpenLootWidget::StaticClass());
		const TSubclassOf<UUserWidget> EntryWidgetClass = EntryWidgetBlueprint->GeneratedClass.Get();
		if (!TileViewWidgetBlueprint || !BuildTempOpenLootTileViewWidgetTree(TileViewWidgetBlueprint, EntryWidgetClass))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(TileViewWidgetBlueprint);
		TileViewWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(TileViewWidgetBlueprint);
	}

	bool EnsureCommonGameHudAssets()
	{
		UWidgetBlueprint* ItemThumbnailWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			ItemThumbnailSlotWidgetAssetName,
			UTunaSweeperItemThumbnailSlotWidget::StaticClass());
		UWidgetBlueprint* TopReserveWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudTopReserveWidgetAssetName,
			UTunaSweeperHudTopReserveWidget::StaticClass());
		UWidgetBlueprint* BottomStatusWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudBottomStatusWidgetAssetName,
			UTunaSweeperHudBottomStatusWidget::StaticClass());
		UWidgetBlueprint* QuickSlotWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudQuickSlotBarWidgetAssetName,
			UTunaSweeperHudQuickSlotBarWidget::StaticClass());
		UWidgetBlueprint* InventoryAreaWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudInventoryAreaWidgetAssetName,
			UTunaSweeperHudInventoryAreaWidget::StaticClass());
		UWidgetBlueprint* ItemInfoPanelWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudItemInfoPanelWidgetAssetName,
			UTunaSweeperHudItemInfoPanelWidget::StaticClass());
		UWidgetBlueprint* ExternalPanelWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudExternalPanelWidgetAssetName,
			UTunaSweeperHudExternalPanelWidget::StaticClass());
		UWidgetBlueprint* LootContainerWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			LootContainerWidgetAssetName,
			UTunaSweeperLootContainerWidget::StaticClass());

		if (!ItemThumbnailWidgetBlueprint || !TopReserveWidgetBlueprint || !BottomStatusWidgetBlueprint || !QuickSlotWidgetBlueprint ||
			!InventoryAreaWidgetBlueprint || !ItemInfoPanelWidgetBlueprint || !ExternalPanelWidgetBlueprint || !LootContainerWidgetBlueprint)
		{
			return false;
		}

		if (!BuildItemThumbnailSlotWidgetTree(ItemThumbnailWidgetBlueprint))
		{
			return false;
		}
		FKismetEditorUtilities::CompileBlueprint(ItemThumbnailWidgetBlueprint);
		ItemThumbnailWidgetBlueprint->MarkPackageDirty();
		if (!SaveAsset(ItemThumbnailWidgetBlueprint))
		{
			return false;
		}

		const TSubclassOf<UUserWidget> ItemThumbnailWidgetClass = ItemThumbnailWidgetBlueprint->GeneratedClass.Get();
		if (!ItemThumbnailWidgetClass)
		{
			return false;
		}

		const bool bChildWidgetsBuilt =
			BuildHudTopReserveWidgetTree(TopReserveWidgetBlueprint) &&
			BuildHudBottomStatusWidgetTree(BottomStatusWidgetBlueprint) &&
			BuildHudQuickSlotBarWidgetTree(QuickSlotWidgetBlueprint) &&
			BuildHudInventoryAreaWidgetTree(InventoryAreaWidgetBlueprint, ItemThumbnailWidgetClass) &&
			BuildHudItemInfoPanelWidgetTree(ItemInfoPanelWidgetBlueprint) &&
			BuildLootContainerWidgetTree(LootContainerWidgetBlueprint, ItemThumbnailWidgetClass);

		if (!bChildWidgetsBuilt)
		{
			return false;
		}

		for (UWidgetBlueprint* ChildWidgetBlueprint : {
			TopReserveWidgetBlueprint,
			BottomStatusWidgetBlueprint,
			QuickSlotWidgetBlueprint,
			InventoryAreaWidgetBlueprint,
			ItemInfoPanelWidgetBlueprint,
			LootContainerWidgetBlueprint
		})
		{
			FKismetEditorUtilities::CompileBlueprint(ChildWidgetBlueprint);
			ChildWidgetBlueprint->MarkPackageDirty();
			if (!SaveAsset(ChildWidgetBlueprint))
			{
				return false;
			}
		}

		if (!BuildHudExternalPanelWidgetTree(ExternalPanelWidgetBlueprint, LootContainerWidgetBlueprint->GeneratedClass.Get()))
		{
			return false;
		}
		FKismetEditorUtilities::CompileBlueprint(ExternalPanelWidgetBlueprint);
		ExternalPanelWidgetBlueprint->MarkPackageDirty();
		if (!SaveAsset(ExternalPanelWidgetBlueprint))
		{
			return false;
		}

		UWidgetBlueprint* GameHudWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			GameHudWidgetAssetName,
			UTunaSweeperGameHudWidget::StaticClass());
		if (!GameHudWidgetBlueprint)
		{
			return false;
		}

		if (!BuildGameHudWidgetTree(
			GameHudWidgetBlueprint,
			TopReserveWidgetBlueprint->GeneratedClass.Get(),
			BottomStatusWidgetBlueprint->GeneratedClass.Get(),
			QuickSlotWidgetBlueprint->GeneratedClass.Get(),
			InventoryAreaWidgetBlueprint->GeneratedClass.Get(),
			ItemInfoPanelWidgetBlueprint->GeneratedClass.Get(),
			ExternalPanelWidgetBlueprint->GeneratedClass.Get()))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(GameHudWidgetBlueprint);
		GameHudWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(GameHudWidgetBlueprint);
	}

	bool BuildInteractionMarkerWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();

		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UHorizontalBox* MarkerRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MarkerRoot"));
		USizeBox* MarkerSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MarkerSizeBox"));
		UOverlay* MarkerOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MarkerOverlay"));
		USizeBox* RingImage = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RingImage"));
		UImage* RingBrushImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RingBrushImage"));
		USizeBox* FilledImage = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FilledImage"));
		UImage* FilledBrushImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("FilledBrushImage"));
		UBorder* LabelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LabelBackground"));
		UTextBlock* DisplayNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DisplayNameText"));

		if (!RootCanvas || !MarkerRoot || !MarkerSizeBox || !MarkerOverlay || !RingImage || !RingBrushImage || !FilledImage || !FilledBrushImage || !LabelBackground || !DisplayNameText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		UCanvasPanelSlot* RootSlot = RootCanvas->AddChildToCanvas(MarkerRoot);
		if (RootSlot)
		{
			RootSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			RootSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			RootSlot->SetPosition(FVector2D::ZeroVector);
			RootSlot->SetSize(FVector2D(300.0f, 56.0f));
		}

		MarkerRoot->SetRenderOpacity(0.0f);

		MarkerSizeBox->SetWidthOverride(56.0f);
		MarkerSizeBox->SetHeightOverride(56.0f);
		MarkerSizeBox->SetContent(MarkerOverlay);

		UHorizontalBoxSlot* MarkerSlot = MarkerRoot->AddChildToHorizontalBox(MarkerSizeBox);
		if (MarkerSlot)
		{
			MarkerSlot->SetHorizontalAlignment(HAlign_Center);
			MarkerSlot->SetVerticalAlignment(VAlign_Center);
		}

		RingImage->SetWidthOverride(34.0f);
		RingImage->SetHeightOverride(34.0f);
		RingImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		RingBrushImage->SetBrush(MakeCircularBrush(FVector2D(34.0f, 34.0f), FLinearColor::Transparent, FLinearColor::White, 3.0f));
		RingImage->SetContent(RingBrushImage);

		UOverlaySlot* RingSlot = MarkerOverlay->AddChildToOverlay(RingImage);
		if (RingSlot)
		{
			RingSlot->SetHorizontalAlignment(HAlign_Center);
			RingSlot->SetVerticalAlignment(VAlign_Center);
		}

		FilledImage->SetWidthOverride(12.0f);
		FilledImage->SetHeightOverride(12.0f);
		FilledBrushImage->SetBrush(MakeCircularBrush(FVector2D(12.0f, 12.0f), FLinearColor::White, FLinearColor::Transparent, 0.0f));
		FilledImage->SetContent(FilledBrushImage);

		UOverlaySlot* FilledSlot = MarkerOverlay->AddChildToOverlay(FilledImage);
		if (FilledSlot)
		{
			FilledSlot->SetHorizontalAlignment(HAlign_Center);
			FilledSlot->SetVerticalAlignment(VAlign_Center);
		}

		LabelBackground->SetBrushColor(FLinearColor::White);
		LabelBackground->SetPadding(FMargin(12.0f, 4.0f));

		ConfigureTextBlock(DisplayNameText, FText::FromString(TEXT("Interact")), FLinearColor::Black, 18);
		LabelBackground->SetContent(DisplayNameText);

		UHorizontalBoxSlot* LabelSlot = MarkerRoot->AddChildToHorizontalBox(LabelBackground);
		if (LabelSlot)
		{
			LabelSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			LabelSlot->SetHorizontalAlignment(HAlign_Left);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		RegisterWidgetVariable(WidgetBlueprint, MarkerRoot);
		RegisterWidgetVariable(WidgetBlueprint, RingImage);
		RegisterWidgetVariable(WidgetBlueprint, FilledImage);
		RegisterWidgetVariable(WidgetBlueprint, LabelBackground);
		RegisterWidgetVariable(WidgetBlueprint, DisplayNameText);

		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	UWidgetBlueprint* EnsureInteractionMarkerWidgetBlueprint();

	bool RebuildInteractionMarkerWidgetAlignment()
	{
		const FString ObjectPath = GetAssetObjectPath(UIAssetPath, InteractionMarkerAssetName);
		UWidgetBlueprint* MarkerWidgetBlueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath);
		if (!MarkerWidgetBlueprint)
		{
			MarkerWidgetBlueprint = EnsureInteractionMarkerWidgetBlueprint();
		}

		if (!MarkerWidgetBlueprint)
		{
			return false;
		}

		if (!BuildInteractionMarkerWidgetTree(MarkerWidgetBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to rebuild marker alignment for %s."), *ObjectPath);
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(MarkerWidgetBlueprint);
		MarkerWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(MarkerWidgetBlueprint);
	}

	UWidgetBlueprint* EnsureInteractionMarkerWidgetBlueprint()
	{
		const FString ObjectPath = GetAssetObjectPath(UIAssetPath, InteractionMarkerAssetName);
		if (UWidgetBlueprint* ExistingBlueprint = LoadObject<UWidgetBlueprint>(nullptr, *ObjectPath))
		{
			if (!ExistingBlueprint->ParentClass || !ExistingBlueprint->ParentClass->IsChildOf(UTunaSweeperInteractionMarkerWidget::StaticClass()))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s already exists, but it is not based on UTunaSweeperInteractionMarkerWidget."), *ObjectPath);
				return nullptr;
			}

			if (!ExistingBlueprint->WidgetTree || !ExistingBlueprint->WidgetTree->RootWidget)
			{
				if (!BuildInteractionMarkerWidgetTree(ExistingBlueprint))
				{
					UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to build widget tree for %s."), *ObjectPath);
					return nullptr;
				}
			}

			if (!ExistingBlueprint->GeneratedClass)
			{
				FKismetEditorUtilities::CompileBlueprint(ExistingBlueprint);
			}

			SaveAsset(ExistingBlueprint);
			return ExistingBlueprint;
		}

		UWidgetBlueprintFactory* WidgetBlueprintFactory = NewObject<UWidgetBlueprintFactory>();
		WidgetBlueprintFactory->ParentClass = UTunaSweeperInteractionMarkerWidget::StaticClass();

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
			InteractionMarkerAssetName,
			UIAssetPath,
			UWidgetBlueprint::StaticClass(),
			WidgetBlueprintFactory);

		UWidgetBlueprint* CreatedBlueprint = Cast<UWidgetBlueprint>(CreatedAsset);
		if (!CreatedBlueprint)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *ObjectPath);
			return nullptr;
		}

		if (!BuildInteractionMarkerWidgetTree(CreatedBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to build widget tree for %s."), *ObjectPath);
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(CreatedBlueprint);
		FAssetRegistryModule::AssetCreated(CreatedBlueprint);
		CreatedBlueprint->MarkPackageDirty();

		if (!SaveAsset(CreatedBlueprint))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return CreatedBlueprint;
	}

	bool ConfigureInteractableBlueprint(UBlueprint* InteractableBlueprint, ETunaSweeperInteractionType InteractionType, const FText& DisplayName)
	{
		if (!InteractableBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(InteractableBlueprint);

		UClass* GeneratedClass = InteractableBlueprint->GeneratedClass;
		ATunaSweeperInteractableActor* Defaults = GeneratedClass
			? Cast<ATunaSweeperInteractableActor>(GeneratedClass->GetDefaultObject())
			: nullptr;

		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(InteractableBlueprint));
			return false;
		}

		InteractableBlueprint->Modify();
		Defaults->Modify();
		if (UTunaSweeperInteractableComponent* InteractableComponent = Defaults->GetInteractableComponent())
		{
			InteractableComponent->Modify();
		}
		Defaults->ConfigureInteractionDefaults(
			InteractionType,
			DisplayName,
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))));
		FBlueprintEditorUtils::MarkBlueprintAsModified(InteractableBlueprint);
		FKismetEditorUtilities::CompileBlueprint(InteractableBlueprint);
		InteractableBlueprint->MarkPackageDirty();

		return SaveAsset(InteractableBlueprint);
	}

	bool ConfigureInteractableActorInstance(AActor* Actor, ETunaSweeperInteractionType InteractionType, const FText& DisplayName)
	{
		if (!Actor)
		{
			return false;
		}

		UTunaSweeperInteractableComponent* InteractableComponent = Actor->FindComponentByClass<UTunaSweeperInteractableComponent>();
		if (!InteractableComponent)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s does not have a UTunaSweeperInteractableComponent."), *GetNameSafe(Actor));
			return false;
		}

		Actor->Modify();
		InteractableComponent->Modify();
		InteractableComponent->ConfigureInteractionDefaults(
			InteractionType,
			DisplayName,
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))));
		Actor->MarkPackageDirty();
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

	bool PlaceInteractionActor(
		UWorld* World,
		UBlueprint* ActorBlueprint,
		const FString& ActorLabel,
		const FVector& Location,
		ETunaSweeperInteractionType InteractionType,
		const FText& DisplayName)
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
			return ConfigureInteractableActorInstance(ExistingActor, InteractionType, DisplayName);
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
		if (!ConfigureInteractableActorInstance(SpawnedActor, InteractionType, DisplayName))
		{
			return false;
		}
		SpawnedActor->MarkPackageDirty();
		return true;
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

	bool PlaceInteractionActorsInRaidMap(UBlueprint* DialogueBlueprint, UBlueprint* PickupBlueprint, UBlueprint* OpenBlueprint)
	{
		UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
		if (!World || World->GetPackage()->GetName() != RaidMapPackagePath)
		{
			return false;
		}

		const bool bPlacedActors =
			PlaceInteractionActor(
				World,
				DialogueBlueprint,
				TEXT("TS_Interact_Dialogue"),
				FVector(200.0f, -200.0f, 80.0f),
				ETunaSweeperInteractionType::Dialogue,
				FText::FromString(TEXT("\uB300\uD654"))) &&
			PlaceInteractionActor(
				World,
				PickupBlueprint,
				TEXT("TS_Interact_Pickup"),
				FVector(450.0f, -200.0f, 80.0f),
				ETunaSweeperInteractionType::Pickup,
				FText::FromString(TEXT("\uC90D\uAE30"))) &&
			PlaceInteractionActor(
				World,
				OpenBlueprint,
				TEXT("TS_Interact_Open"),
				FVector(700.0f, -200.0f, 80.0f),
				ETunaSweeperInteractionType::Open,
				FText::FromString(TEXT("\uC5F4\uAE30")));

		if (!bPlacedActors)
		{
			return false;
		}

		return UEditorLoadingAndSavingUtils::SaveMap(World, RaidMapPackagePath);
	}

	bool EnsureInteractionAssetsAndMapPlacement()
	{
		UWidgetBlueprint* MarkerWidgetBlueprint = EnsureInteractionMarkerWidgetBlueprint();
		UBlueprint* DialogueBlueprint = EnsureBlueprint(InteractionAssetPath, DialogueInteractionAssetName, ATunaSweeperInteractableActor::StaticClass());
		UBlueprint* PickupBlueprint = EnsureBlueprint(InteractionAssetPath, PickupInteractionAssetName, ATunaSweeperInteractableActor::StaticClass());
		UBlueprint* OpenBlueprint = EnsureBlueprint(InteractionAssetPath, OpenInteractionAssetName, ATunaSweeperInteractableActor::StaticClass());

		if (!MarkerWidgetBlueprint || !DialogueBlueprint || !PickupBlueprint || !OpenBlueprint)
		{
			return false;
		}

		const bool bConfigured =
			ConfigureInteractableBlueprint(DialogueBlueprint, ETunaSweeperInteractionType::Dialogue, FText::FromString(TEXT("\uB300\uD654"))) &&
			ConfigureInteractableBlueprint(PickupBlueprint, ETunaSweeperInteractionType::Pickup, FText::FromString(TEXT("\uC90D\uAE30"))) &&
			ConfigureInteractableBlueprint(OpenBlueprint, ETunaSweeperInteractionType::Open, FText::FromString(TEXT("\uC5F4\uAE30")));

		return bConfigured && PlaceInteractionActorsInRaidMap(DialogueBlueprint, PickupBlueprint, OpenBlueprint);
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

		return bConfigured && PlacePickupItemAndSpawnerActorsInRaidMap(PickupItemBlueprint, ItemSpawnBlueprint);
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

		return bConfigured && PlaceLootContainerAndSpawnerActorsInRaidMap(LootContainerBlueprint, LootContainerSpawnBlueprint);
	}

	void ScheduleInteractionAssetsAndMapPlacement()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(InteractionTaskId))
		{
			return;
		}

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[](float)
				{
					UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
					if (!EditorWorld || EditorWorld->GetPackage()->GetName() != RaidMapPackagePath)
					{
						return true;
					}

					FTunaSweeperEditorRunOnce::Run(
						InteractionTaskId,
						[]()
						{
							return EnsureInteractionAssetsAndMapPlacement();
						});

					return !FTunaSweeperEditorRunOnce::HasCompleted(InteractionTaskId);
				}),
			1.0f);
	}

	void SchedulePickupItemAndSpawnerAssetsAndMapPlacement()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(PickupItemAndSpawnerTaskId))
		{
			return;
		}

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[](float)
				{
					UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
					if (!EditorWorld || EditorWorld->GetPackage()->GetName() != RaidMapPackagePath)
					{
						return true;
					}

					FTunaSweeperEditorRunOnce::Run(
						PickupItemAndSpawnerTaskId,
						[]()
						{
							return EnsurePickupItemAndSpawnerAssetsAndMapPlacement();
						});

					return !FTunaSweeperEditorRunOnce::HasCompleted(PickupItemAndSpawnerTaskId);
				}),
			1.0f);
	}

	void ScheduleLootContainerAndSpawnerAssetsAndMapPlacement()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(LootContainerAndSpawnerTaskId))
		{
			return;
		}

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[](float)
				{
					UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
					if (!EditorWorld || EditorWorld->GetPackage()->GetName() != RaidMapPackagePath)
					{
						return true;
					}

					FTunaSweeperEditorRunOnce::Run(
						LootContainerAndSpawnerTaskId,
						[]()
						{
							return EnsureLootContainerAndSpawnerAssetsAndMapPlacement();
						});

					return !FTunaSweeperEditorRunOnce::HasCompleted(LootContainerAndSpawnerTaskId);
				}),
			1.0f);
	}
}

class FTunaSweeperEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		if (IsRunningCommandlet())
		{
			return;
		}

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::GameInstanceTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureGameInstanceBlueprint();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::TopDownShooterTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureTopDownShooterAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::InteractionInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureInteractionInputAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::InventoryInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureInventoryInputAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::InteractionMarkerAlignmentTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::RebuildInteractionMarkerWidgetAlignment();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::TempOpenLootUiTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureTempOpenLootTileViewAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::CommonGameHudTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::CannedTunaIconImportTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCannedTunaIconTexture();
			});

		TunaSweeperEditorSetup::ScheduleInteractionAssetsAndMapPlacement();
		TunaSweeperEditorSetup::SchedulePickupItemAndSpawnerAssetsAndMapPlacement();
		TunaSweeperEditorSetup::ScheduleLootContainerAndSpawnerAssetsAndMapPlacement();
	}
};

IMPLEMENT_MODULE(FTunaSweeperEditorModule, TunaSweeperEditor)
