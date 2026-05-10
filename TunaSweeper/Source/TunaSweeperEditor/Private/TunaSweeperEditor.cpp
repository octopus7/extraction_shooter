#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Containers/Ticker.h"
#include "Engine/Blueprint.h"
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
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Player/TunaSweeperPlayerController.h"
#include "TunaSweeperEditorRunOnce.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"
#include "UObject/SavePackage.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "Weapon/TunaSweeperWeapon.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperEditor, Log, All);

namespace TunaSweeperEditorSetup
{
	const FString GameInstanceTaskId = TEXT("2026-05-10_CreateGameInstanceBlueprint");
	const FString TopDownShooterTaskId = TEXT("2026-05-10_CreateTopDownShooterAssets");
	const FString InteractionTaskId = TEXT("2026-05-10_CreateInteractionAssetsAndPlaceActors");
	const FString InteractionInputTaskId = TEXT("2026-05-10_AddInteractionInput");
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
	const FString MappingContextName = TEXT("IMC_Player");
	const FString UIAssetPath = TEXT("/Game/UI");
	const FString InteractionMarkerAssetName = TEXT("WBP_InteractionMarker");
	const FString InteractionAssetPath = TEXT("/Game/Interaction");
	const FString DialogueInteractionAssetName = TEXT("BP_Interact_Dialogue");
	const FString PickupInteractionAssetName = TEXT("BP_Interact_Pickup");
	const FString OpenInteractionAssetName = TEXT("BP_Interact_Open");
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

		if (!HasInputMapping(MappingContext, InteractAction, EKeys::E))
		{
			MappingContext->MapKey(InteractAction, EKeys::E);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, fire, aim, and interaction input."));
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
		if (!WidgetBlueprint || !Widget || Widget->bIsVariable)
		{
			return;
		}

		Widget->bIsVariable = true;
		WidgetBlueprint->OnVariableAdded(Widget->GetFName());
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

	bool BuildInteractionMarkerWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UHorizontalBox* MarkerRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MarkerRoot"));
		USizeBox* MarkerSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MarkerSizeBox"));
		UOverlay* MarkerOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MarkerOverlay"));
		UTextBlock* RingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RingText"));
		UTextBlock* FilledText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FilledText"));
		UBorder* LabelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LabelBackground"));
		UTextBlock* DisplayNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DisplayNameText"));

		if (!RootCanvas || !MarkerRoot || !MarkerSizeBox || !MarkerOverlay || !RingText || !FilledText || !LabelBackground || !DisplayNameText)
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
			RootSlot->SetSize(FVector2D(300.0f, 64.0f));
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

		ConfigureTextBlock(RingText, FText::FromString(TEXT("\u25CB")), FLinearColor::White, 48);
		RingText->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

		UOverlaySlot* RingSlot = MarkerOverlay->AddChildToOverlay(RingText);
		if (RingSlot)
		{
			RingSlot->SetHorizontalAlignment(HAlign_Center);
			RingSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlock(FilledText, FText::FromString(TEXT("\u25CF")), FLinearColor::White, 18);

		UOverlaySlot* FilledSlot = MarkerOverlay->AddChildToOverlay(FilledText);
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
		RegisterWidgetVariable(WidgetBlueprint, RingText);
		RegisterWidgetVariable(WidgetBlueprint, FilledText);
		RegisterWidgetVariable(WidgetBlueprint, LabelBackground);
		RegisterWidgetVariable(WidgetBlueprint, DisplayNameText);

		WidgetBlueprint->MarkPackageDirty();
		return true;
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
		Defaults->ConfigureInteractionDefaults(
			InteractionType,
			DisplayName,
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))));
		InteractableBlueprint->MarkPackageDirty();

		return SaveAsset(InteractableBlueprint);
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

	bool PlaceInteractionActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location)
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
			return true;
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
			PlaceInteractionActor(World, DialogueBlueprint, TEXT("TS_Interact_Dialogue"), FVector(200.0f, -200.0f, 80.0f)) &&
			PlaceInteractionActor(World, PickupBlueprint, TEXT("TS_Interact_Pickup"), FVector(450.0f, -200.0f, 80.0f)) &&
			PlaceInteractionActor(World, OpenBlueprint, TEXT("TS_Interact_Open"), FVector(700.0f, -200.0f, 80.0f));

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

		TunaSweeperEditorSetup::ScheduleInteractionAssetsAndMapPlacement();
	}
};

IMPLEMENT_MODULE(FTunaSweeperEditorModule, TunaSweeperEditor)
