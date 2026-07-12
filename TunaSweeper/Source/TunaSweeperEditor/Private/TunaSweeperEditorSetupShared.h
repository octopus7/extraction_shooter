#pragma once

#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperCoverPointActor.h"
#include "Animation/WidgetAnimation.h"
#include "AutomatedAssetImportData.h"
#include "Blueprint/WidgetTree.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Character/TunaSweeperLedRobotCharacterActor.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/ListViewBase.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Containers/Ticker.h"
#include "Dataflow/DataflowSelection.h"
#include "Effect/TunaSweeperProjectileHitBurstActor.h"
#include "Effect/TunaSweeperProjectileHitEffectDataAsset.h"
#include "Engine/Blueprint.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "Editor.h"
#include "Factories/BlueprintFactory.h"
#include "FractureEngineFracturing.h"
#include "FractureEngineSelection.h"
#include "FileHelpers.h"
#include "Game/TunaSweeperGameMode.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameMapsSettings.h"
#include "GameFramework/PlayerStart.h"
#include "HAL/FileManager.h"
#include "IAssetTools.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "GeometryCollection/GeometryCollectionEngineConversion.h"
#include "GeometryCollection/GeometryCollectionObject.h"
#include "GeometryCollection/GeometryCollectionSimulationTypes.h"
#include "GeometryCollection/GeometryCollectionUtility.h"
#include "GeometryCollection/ManagedArrayCollection.h"
#include "Interaction/TunaSweeperBreakableAppleCrateActor.h"
#include "Interaction/TunaSweeperExplosiveBarrelActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperItemSpawnInteractableActor.h"
#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Interaction/TunaSweeperLootContainerSpawnInteractableActor.h"
#include "Interaction/TunaSweeperPhysicsAppleActor.h"
#include "Interaction/TunaSweeperPhysicsCrateFragmentActor.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "Interaction/TunaSweeperSandbagCoverActor.h"
#include "Interaction/TunaSweeperSelfDestructInteractableActor.h"
#include "Interaction/TunaSweeperTransparentObstacleActor.h"
#include "Interaction/TunaSweeperWarpPointActor.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
#include "Lookdev/TunaSweeperLookdevCameraDirectorActor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Map/TunaSweeperMapCaptureActor.h"
#include "MediaSource.h"
#include "Factories/MaterialFactoryNew.h"
#include "Engine/BlendableInterface.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionIf.h"
#include "Materials/MaterialExpressionLength.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSceneTexture.h"
#include "Materials/MaterialExpressionScreenPosition.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTwoSidedSign.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "MeshDescription.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraEmitter.h"
#include "NiagaraActor.h"
#include "NiagaraComponent.h"
#include "NiagaraParameterStore.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemImpl.h"
#include "NiagaraTypes.h"
#include "ObjectTools.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Sound/SoundWave.h"
#include "StaticMeshAttributes.h"
#include "Styling/SlateTypes.h"
#include "TunaSweeperExperimentalVegetation.h"
#include "TunaSweeperFMSoundTool.h"
#include "TunaSweeperLevelOpenTool.h"
#include "TunaSweeperPuddleSkyReflectionMaterial.h"
#include "TunaSweeperProceduralTerrainTest.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "TunaSweeperEditorRunOnce.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"
#include "TunaSweeperMapCaptureActorDetails.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "UI/TunaSweeperHudDebuffBarWidget.h"
#include "UI/TunaSweeperHudExternalPanelWidget.h"
#include "UI/TunaSweeperHudInventoryAreaWidget.h"
#include "UI/TunaSweeperHudItemInfoPanelWidget.h"
#include "UI/TunaSweeperHudQuickSlotBarWidget.h"
#include "UI/TunaSweeperHudStatusRingWidget.h"
#include "UI/TunaSweeperHudTopReserveWidget.h"
#include "UI/TunaSweeperCurrencyDisplayWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperItemThumbnailSlotWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UI/ItemContainerWidget.h"
#include "UI/TunaSweeperPickupItemIconWidget.h"
#include "UI/TunaSweeperQuestWidget.h"
#include "UI/TunaSweeperReloadRingWidget.h"
#include "UI/TunaSweeperSpeechBubbleWidget.h"
#include "UI/TunaSweeperWorkbenchPanelWidget.h"
#include "UI/TunaSweeperWorkbenchRecipeListEntryWidget.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "Weapon/TunaSweeperWeapon.h"
#include "Weapon/TunaSweeperWeaponPresentationDataAsset.h"
#include "Weapon/TunaSweeperWeaponSpreadRecoilDataAsset.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"


DECLARE_LOG_CATEGORY_EXTERN(LogTunaSweeperEditor, Log, All);

namespace TunaSweeperEditorSetup
{
	const FString GameInstanceTaskId = TEXT("2026-05-10_CreateGameInstanceBlueprint");
	const FString TopDownShooterTaskId = TEXT("2026-05-10_CreateTopDownShooterAssets");
	const FString InteractionInputTaskId = TEXT("2026-05-29_SetInteractInputAndFocusWheelV1");
	const FString InteractionMarkerAlignmentTaskId = TEXT("2026-05-25_RebuildInteractionMarkerRequirementPreviewV1");
	const FString PickupItemAndSpawnerTaskId = TEXT("2026-05-11_CreatePickupItemAndSpawnerAssetsV3");
	const FString CommonGameHudTaskId = TEXT("2026-06-02_BottomActionProgressCuteLayoutV1");
	constexpr float GameplayBottomQuickSlotWidth = 694.0f;
	constexpr float GameplayBottomQuickSlotHeight = 208.0f;
	constexpr float GameplayBottomStatusWidth = 274.0f;
	constexpr float GameplayBottomStatusHeight = 58.0f;
	constexpr float GameplayBottomStatusGap = 12.0f;
	constexpr float GameplayBottomPanelWidth = 1120.0f;
	constexpr float GameplayBottomPanelHeight = 208.0f;
	constexpr float HudDebuffBarLeftOffset = 24.0f;
	constexpr float HudDebuffBarBottomOffset = 40.0f;
	const FString WorkbenchPanelWidgetTaskId = TEXT("2026-05-29_CreateWorkbenchPanelWidgetV6");
	const FString ShopRefreshStockButtonTaskId = TEXT("2026-05-29_AddShopRefreshStockButtonV1");
	const FString SplitExternalContainerPanelTaskId = TEXT("2026-05-30_SplitExternalContainerPanelsV1");
	const FString CurrencyCoinUiTaskId = TEXT("2026-05-30_AddCurrencyCoinUiV1");
	const FString HudDebuffBarWidgetTaskId = TEXT("2026-05-30_HudDebuffBarWidgetV2");
	const FString ItemThumbnailSlotLayoutTaskId = TEXT("2026-05-30_RebuildItemThumbnailSlotLayoutV1");
	const FString ItemContainerScrollbarStyleTaskId = TEXT("2026-05-30_ItemContainerScrollbarStyleV1");
	const FString InventoryInputTaskId = TEXT("2026-05-11_AddInventoryInput");
	const FString QuickSlotInputTaskId = TEXT("2026-05-28_AddMeleeQuickSlotInputV1");
	const FString DropInputTaskId = TEXT("2026-05-18_AddDropInputAction");
	const FString AmmoReloadInputTaskId = TEXT("2026-05-19_AddAmmoReloadInputActionsV1");
	const FString CameraModeInputTaskId = TEXT("2026-05-26_AddCameraModeInputV1");
	const FString SprintInputTaskId = TEXT("2026-05-28_AddSprintInputV1");
	const FString RollInputTaskId = TEXT("2026-05-28_AddRollInputV1");
	const FString MapInputTaskId = TEXT("2026-05-28_AddMapInputV1");
	const FString EditorMapCaptureTaskId = TEXT("2026-05-28_CreateEditorMapCaptureBlueprintAndRaidPlacementV1");
	const FString LootContainerAndSpawnerTaskId = TEXT("2026-05-11_CreateLootContainerAndSpawnerAssetsV1");
	const FString LootContainerOccupancyHeaderTaskId = TEXT("2026-05-30_StorageFilterHeaderAboveGridV1");
	const FString CannedTunaIconImportTaskId = TEXT("2026-05-11_ImportCannedTunaIconV1");
	const FString BackpackInventoryTaskId = TEXT("2026-05-16_CreateEquipmentInventoryAssetsV3");
	const FString IntroMenuAndLevelTravelTaskId = TEXT("2026-05-24_CreateTitleIntroMenuPersistentSaveSlotSelectionLevelTravelLadderInitialScaleV1");
	const FString IntroMenuGraphicsSettingsTaskId = TEXT("2026-07-08_BuildCompleteTitleSettingsWbpV1");
	const FString OpeningScenarioPresentationTaskId = TEXT("2026-05-19_CreateOpeningScenarioPresentationV2");
	const FString LevelTransitionVideoTaskId = TEXT("2026-05-16_AddBidirectionalLevelTransitionVideoV3");
	const FString FirstOutingQuestTaskId = TEXT("2026-05-30_UpdateQuestPanelEmptyStateSelectionV2");
	const FString SelfDestructInteractionTaskId = TEXT("2026-05-16_CreateSelfDestructInteractionV1");
	const FString WorldProgressInteractionTaskId = TEXT("2026-05-19_CreateWorldProgressObstacleAssetsV1");
	const FString WarpPointInteractionTaskId = TEXT("2026-05-25_CreateWarpPointInteractionAssetsV1");
	const FString EnemyVisualMaterialTaskId = TEXT("2026-05-19_CreateEnemyAndContainerVisualMaterialsV3");
	const FString ExplosiveBarrelTaskId = TEXT("2026-05-29_CreateExplosiveBarrelAssetsV8");
	const FString BreakableAppleCrateTaskId = TEXT("2026-07-07_CreateBreakableAppleCrateAssetsV5");
	const FString RollingBomberBodyMaterialTaskId = TEXT("2026-05-28_CreateRollingBomberBodyGrayMaterialV1");
	const FString RollingBomberLegMaterialTaskId = TEXT("2026-05-28_CreateRollingBomberLegMetalMaterialV1");
	const FString RollingBomberChargeCylinderEffectTaskId = TEXT("2026-05-28_CreateRollingBomberChargeCylinderEffectV1");
	const FString LocalExplosionEffectTaskId = TEXT("2026-05-29_CreateLocalExplosionFlipbookEffectV3");
	const FString ExtractionSmokeSignalNiagaraSystemTaskId = TEXT("2026-05-29_CreateExtractionSmokeSignalNiagaraSystemV4");
	const FString LookdevFluidExplosionTaskId = TEXT("2026-06-14_CreateLookdevFluidExplosionNiagaraLevelV1");
	const FString ProjectileHitEffectAssetTaskId = TEXT("2026-05-28_CreateProjectileHitEffectAssetsV1");
	const FString WeaponSpreadRecoilAssetTaskId = TEXT("2026-05-28_CreateWeaponSpreadRecoilAssetsV1");
	const FString WeaponPresentationAssetTaskId = TEXT("2026-07-12_CreateWeaponPresentationAssetsV1");
	const FString BaseballBatAssetTaskId = TEXT("2026-05-28_CreateBaseballBatStaticMeshAssetsV1");
	const FString SandbagCoverAssetTaskId = TEXT("2026-06-02_SandbagFourLayerCoverV1");
	const FString VoxelMeshAssetTaskId = TEXT("2026-05-19_CreateSharedVoxelMeshAssetsV1");
	const FString LumberjackMeleeSwingArcAssetTaskId = TEXT("2026-05-20_CreateLumberjackMeleeSwingArcAssetsV2");
	const FString LedExpressionMaterialTaskId = TEXT("2026-05-26_CreateLedExpressionMaterialV1");
	const FString ExperimentalVegetationAssetTaskId = TEXT("2026-05-24_CreateExperimentalVegetationStaticMeshV4");
	const FString TurbulentConiferOcclusionRevealTaskId = TEXT("2026-06-08_UpdateTurbulentConiferOcclusionRevealV1");
	const FString CoverPointAssetTaskId = TEXT("2026-05-16_CreateCoverPointBlueprintV1");
	const FString CanBotBlueprintTaskId = TEXT("2026-05-25_CreateCanBotBlueprintV1");
	const FString GameInstanceAssetPath = TEXT("/Game/Core");
	const FString GameInstanceAssetName = TEXT("BP_TunaSweeperGameInstance");
	const FString GameModeAssetName = TEXT("BP_TunaSweeperGameMode");
	const FString PlayerAssetPath = TEXT("/Game/Characters/Player");
	const FString PlayerAssetName = TEXT("BP_TunaSweeperPlayerCharacter");
	const FString CanBotAssetPath = TEXT("/Game/Characters/CanBot");
	const FString CanBotAssetName = TEXT("BP_CanBot");
	const FString EnemyAssetPath = TEXT("/Game/Characters/Enemy");
	const FString EnemyAssetName = TEXT("BP_TunaSweeperEnemy");
	const FString EnemyBodyMaterialAssetName = TEXT("M_Enemy_Red");
	const FString EnemyGreenMaterialAssetName = TEXT("M_Enemy_Green");
	const FString EnemyBlueMaterialAssetName = TEXT("M_Enemy_Blue");
	const FString EnemySightlineMaterialAssetName = TEXT("M_Enemy_Sightline");
	const FString RollingBomberBodyGrayMaterialAssetName = TEXT("M_RollingBomberBodyGray");
	const FString RollingBomberLegMetalMaterialAssetName = TEXT("M_RollingBomberLegMetal");
	const FString EnemyVoxelBodyMeshAssetName = TEXT("SM_Enemy_VoxelBody");
	const FString EnemyVoxelForwardMarkerMeshAssetName = TEXT("SM_Enemy_VoxelForwardMarker");
	const FString CoverAssetPath = TEXT("/Game/AI/Cover");
	const FString CoverPointAssetName = TEXT("BP_TunaSweeperCoverPoint");
	const FString WeaponAssetPath = TEXT("/Game/Weapons");
	const FString WeaponAssetName = TEXT("BP_TunaSweeperWeapon");
	const FString ProjectileAssetName = TEXT("BP_TunaSweeperProjectile");
	const FString WeaponSpreadRecoilDataAssetName = TEXT("DA_WeaponSpreadRecoil");
	const FString WeaponPresentationAssetPath = TEXT("/Game/Weapons/AudioVisual");
	const FString WeaponPresentationRifleDataAssetName = TEXT("DA_WeaponPresentation_Rifle");
	const FString WeaponPresentationAudioAssetPath = TEXT("/Game/Audio/SFX/Weapons");
	const FString RifleFireSoundAssetName = TEXT("SFX_Rifle_Fire_FM");
	const FString RifleReloadStartSoundAssetName = TEXT("SFX_Rifle_ReloadStart_FM");
	const FString RifleReloadCompleteSoundAssetName = TEXT("SFX_Rifle_ReloadComplete_FM");
	const FString BaseballBatWoodTextureAssetName = TEXT("T_BaseballBat_WoodGrain");
	const FString BaseballBatMaterialAssetName = TEXT("M_BaseballBat_Wood");
	const FString BaseballBatMeshAssetName = TEXT("SM_BaseballBat");
	const FString InputAssetPath = TEXT("/Game/Input");
	const FString MoveActionName = TEXT("IA_Move");
	const FString FireActionName = TEXT("IA_Fire");
	const FString AimActionName = TEXT("IA_Aim");
	const FString InteractActionName = TEXT("IA_Interact");
	const FString InteractionFocusActionName = TEXT("IA_InteractionFocus");
	const FString InventoryActionName = TEXT("IA_Inventory");
	const FString DropActionName = TEXT("IA_Drop");
	const FString ReloadActionName = TEXT("IA_Reload");
	const FString AmmoSelectActionName = TEXT("IA_AmmoSelect");
	const FString AmmoFocusActionName = TEXT("IA_AmmoFocus");
	const FString CameraModeActionName = TEXT("IA_CameraMode");
	const FString SprintActionName = TEXT("IA_Sprint");
	const FString RollActionName = TEXT("IA_Roll");
	const FString MapActionName = TEXT("IA_Map");
	const FString QuickSlotActionNamePrefix = TEXT("IA_QuickSlot");
	const FString MeleeQuickSlotActionName = TEXT("IA_MeleeQuickSlot");
	const FString MappingContextName = TEXT("IMC_Player");
	const FString UIAssetPath = TEXT("/Game/UI");
	const FString VoxelAssetPath = TEXT("/Game/Prototype");
	const FString VoxelVertexColorMaterialAssetName = TEXT("M_Voxel_VertexColor");
	const FString EffectsAssetPath = TEXT("/Game/Effects");
	const FString LookdevAssetPath = TEXT("/Game/EditorOnly/Lookdev");
	const FString LookdevFluidExplosionMapPackagePath = TEXT("/Game/EditorOnly/Lookdev/Lookdev_NiagaraExplosion");
	const FString RollingBomberChargeCylinderMaskTextureAssetName = TEXT("T_RollingBomberChargeCylinderMask");
	const FString RollingBomberChargeCylinderMaterialAssetName = TEXT("M_RollingBomberChargeCylinder");
	const FString RollingBomberChargeCylinderMeshAssetName = TEXT("SM_RollingBomberChargeCylinder_Open");
	const FString LocalExplosionFlipbookTextureAssetName = TEXT("T_LocalExplosionFlipbook");
	const FString LocalExplosionFlipbookMaterialAssetName = TEXT("M_LocalExplosionFlipbook");
	const FString LocalExplosionDistortionMaterialAssetName = TEXT("M_LocalExplosionDistortion");
	const FString LocalExplosionSmokeMaterialAssetName = TEXT("M_LocalExplosionSmoke");
	const FString ExtractionSmokeSignalNiagaraSystemAssetName = TEXT("NS_ExtractionSmokeSignal");
	const FString LookdevFluidExplosionNiagaraSystemAssetName = TEXT("NS_LookdevFluidExplosion_HighCost");
	const FString LookdevExplosionFloorMaterialAssetName = TEXT("M_LookdevExplosionFloor");
	const FString ProjectileHitEffectDataAssetName = TEXT("DA_ProjectileHitEffects");
	const FString ProjectileHitRedBurstActorAssetName = TEXT("BP_ProjectileHit_RedBurst");
	const FString LumberjackMeleeSwingArcMaterialAssetName = TEXT("M_LumberjackMeleeSwingArc");
	const FString LumberjackMeleeSwingArcMeshAssetName = TEXT("SM_LumberjackMeleeSwingArc");
	const FString LedExpressionMaterialAssetName = TEXT("M_LedExpression_VertexColorEmissive");
	const FString UIIconAssetPath = TEXT("/Game/UI/Icons");
	const FString HudModeInventoryIconAssetName = TEXT("T_UI_Mode_Inventory");
	const FString HudModeQuestIconAssetName = TEXT("T_UI_Mode_Quest");
	const FString HudModeMapIconAssetName = TEXT("T_UI_Mode_Map");
	const FString HudModeMemoIconAssetName = TEXT("T_UI_Mode_Memo");
	const FString HudStatusHeartIconAssetName = TEXT("T_UI_Hud_Status_Heart");
	const FString HudStatusWaterIconAssetName = TEXT("T_UI_Hud_Status_WaterDrop");
	const FString HudStatusMeatIconAssetName = TEXT("T_UI_Hud_Status_Meat");
	const FString UITitleTextureAssetPath = TEXT("/Game/UI/Title");
	const FString UIStoryTextureAssetPath = TEXT("/Game/UI/Story");
	const FString TitleBackgroundTextureAssetName = TEXT("Title_C1");
	const FString TitleLogoTextureAssetName = TEXT("tuna_sweeper_logo_transparent");
	const FString OpeningScenarioBackgroundTextureAssetName = TEXT("T_Story_OpeningLightParticles");
	const FString InteractionMarkerAssetName = TEXT("WBP_InteractionMarker");
	const FString PickupItemIconWidgetAssetName = TEXT("WBP_PickupItemIcon");
	const FString GameHudWidgetAssetName = TEXT("WBP_GameHud");
	const FString HudTopReserveWidgetAssetName = TEXT("WBP_HudTopReserve");
	const FString HudBottomStatusWidgetAssetName = TEXT("WBP_HudBottomStatus");
	const FString HudDebuffBarWidgetAssetName = TEXT("WBP_HudDebuffBar");
	const FString HudQuickSlotBarWidgetAssetName = TEXT("WBP_HudQuickSlotBar");
	const FString HudInventoryAreaWidgetAssetName = TEXT("WBP_HudInventoryArea");
	const FString HudItemInfoPanelWidgetAssetName = TEXT("WBP_HudItemInfoPanel");
	const FString HudExternalPanelWidgetAssetName = TEXT("WBP_HudExternalPanel");
	const FString ItemThumbnailSlotWidgetAssetName = TEXT("WBP_ItemThumbnailSlot");
	const FString LootContainerWidgetAssetName = TEXT("WBP_LootContainerPanel");
	const FString StorageContainerWidgetAssetName = TEXT("WBP_StorageContainerPanel");
	const FString ShopContainerWidgetAssetName = TEXT("WBP_ShopContainerPanel");
	const FString WorkbenchPanelWidgetAssetName = TEXT("WBP_WorkbenchPanel");
	const FString WorkbenchRecipeListEntryWidgetAssetName = TEXT("WBP_WorkbenchRecipeListEntry");
	const FString IntroMenuWidgetAssetName = TEXT("WBP_IntroMenu");
	const FString LevelTransitionVideoWidgetAssetName = TEXT("WBP_LevelTransitionVideo");
	const FString QuestMenuWidgetAssetName = TEXT("WBP_QuestMenu");
	const FString QuestInteractionWidgetAssetName = TEXT("WBP_QuestInteraction");
	const FString SpeechBubbleWidgetAssetName = TEXT("WBP_SpeechBubble");
	constexpr float HudTopModeTabButtonWidth = 52.0f;
	constexpr float HudTopModeTabButtonHeight = 46.0f;
	constexpr float HudTopModeTabGap = 8.0f;
	constexpr float HudTopModeTabPaddingX = 12.0f;
	constexpr float HudTopModeTabPaddingY = 10.0f;
	constexpr float HudTopModeTabPanelWidth =
		4.0f * HudTopModeTabButtonWidth + 3.0f * HudTopModeTabGap + 2.0f * HudTopModeTabPaddingX;
	constexpr float HudTopModeTabPanelHeight = HudTopModeTabButtonHeight + 2.0f * HudTopModeTabPaddingY;
	constexpr float HudUtilityPanelLeftInset = 34.0f;
	constexpr float HudUtilityPanelRightInset = 34.0f;
	constexpr float HudUtilityPanelTopOffset = 96.0f;
	constexpr float HudUtilityPanelBottomInset = 40.0f;
	constexpr int32 InventoryTileColumnCount = 5;
	constexpr int32 EquipmentReserveColumnCount = 4;
	constexpr float InventoryTileWidth = 96.0f;
	constexpr float InventoryTileHeight = 96.0f;
	constexpr float InventoryPanelPadding = 14.0f;
	constexpr float InventoryTileViewScrollbarReserveWidth = 22.0f;
	constexpr float InventoryTileViewWidth = InventoryTileColumnCount * InventoryTileWidth + InventoryTileViewScrollbarReserveWidth;
	constexpr float InventoryPanelWidth = InventoryPanelPadding * 2.0f + InventoryTileViewWidth;
	constexpr float EquipmentReserveEntryWidth = 112.0f;
	constexpr float EquipmentReserveEntryHeight = 124.0f;
	constexpr float EquipmentReserveWidth = EquipmentReserveColumnCount * EquipmentReserveEntryWidth;
	constexpr float EquipmentReserveHeight = 2.0f * EquipmentReserveEntryHeight;
	constexpr float InventorySortControlAreaHeight = 34.0f;
	constexpr float AuxiliaryBagPanelPadding = 6.0f;
	constexpr float AuxiliaryBagPanelGap = 4.0f;
	constexpr float AuxiliaryBagPanelWidth = AuxiliaryBagPanelPadding * 2.0f + InventoryTileWidth;
	constexpr float AuxiliaryBagPanelHeight = AuxiliaryBagPanelPadding * 2.0f + 2.0f * InventoryTileHeight;
	constexpr float InventoryAreaPanelWidth = InventoryPanelWidth + AuxiliaryBagPanelGap + AuxiliaryBagPanelWidth;
	constexpr int32 LootContainerTileColumnCount = 5;
	constexpr float LootContainerTileWidth = 96.0f;
	constexpr float LootContainerTileHeight = 96.0f;
	constexpr float LootContainerPanelPadding = 14.0f;
	constexpr float LootContainerTileViewScrollbarReserveWidth = 22.0f;
	constexpr float LootContainerPanelHeaderHeight = 74.0f;
	constexpr float LootContainerPanelWidth =
		LootContainerPanelPadding * 2.0f + LootContainerTileColumnCount * LootContainerTileWidth + LootContainerTileViewScrollbarReserveWidth;
	constexpr float WorkbenchPanelWidth = 780.0f;
	constexpr float WorkbenchPanelHeight = 620.0f;
	constexpr float WorkbenchPanelPadding = 16.0f;
	constexpr float WorkbenchLeftPanelWidth = 340.0f;
	constexpr float WorkbenchTileViewWidth = 318.0f;
	constexpr float WorkbenchTileViewHeight = 468.0f;
	constexpr float WorkbenchTileWidth = 96.0f;
	constexpr float WorkbenchTileHeight = 96.0f;
	const FString InteractionAssetPath = TEXT("/Game/Interaction");
	const FString EditorMapCaptureAssetPath = TEXT("/Game/EditorOnly/MapCapture");
	const FString EditorMapCaptureBlueprintAssetName = TEXT("BP_Editor_MapCaptureActor");
	const FString EditorMapCaptureActorLabel = TEXT("TS_Editor_MapCapture_Raid");
	const FString VideoAssetPath = TEXT("/Game/Movies");
	const FString AudioBgmAssetPath = TEXT("/Game/Audio/BGM");
	const FString BunkerToRaidMediaSourceAssetName = TEXT("MS_BunkerToRaid");
	const FString TitleBgmAssetName = TEXT("Where_the_Birds_Still_Sing");
	const FString PickupItemAssetName = TEXT("BP_PickupItem");
	const FString ItemSpawnInteractionAssetName = TEXT("BP_Interact_ItemSpawn");
	const FString LootContainerAssetName = TEXT("BP_LootContainer");
	const FString LootContainerSpawnInteractionAssetName = TEXT("BP_Interact_LootContainerSpawn");
	const FString CardboardContainerMaterialAssetName = TEXT("M_Container_Cardboard");
	const FString WoodContainerMaterialAssetName = TEXT("M_Container_Wood");
	const FString MetalContainerMaterialAssetName = TEXT("M_Container_Metal");
	const FString SupplyContainerMaterialAssetName = TEXT("M_Container_Supply");
	const FString LevelTravelInteractionAssetName = TEXT("BP_Interact_LevelTravel");
	const FString LevelTravelLadderMeshAssetName = TEXT("SM_LevelTravel_Ladder");
	const FString SelfDestructInteractionAssetName = TEXT("BP_Interact_SelfDestruct");
	const FString WarpPointInteractionAssetName = TEXT("BP_WarpPoint");
	const FString WarpPointEnergyMaterialAssetName = TEXT("M_WarpPointEnergy");
	const FString WarpPointNoiseTextureAssetName = TEXT("T_WarpPointNoise");
	const FString MemoStorageDeviceTextureAssetName = TEXT("T_MemoStorageDevice");
	const FString MemoStorageDeviceMaterialAssetName = TEXT("M_MemoStorageDevice");
	const FString RollingBomberSpawnerTextureAssetName = TEXT("T_RollingBomberSpawner_Mechanic");
	const FString RollingBomberSpawnerMaterialAssetName = TEXT("M_RollingBomberSpawner_Mechanic");
	const FString SandbagCoverTextureAssetName = TEXT("T_SandbagCover_Burlap");
	const FString SandbagCoverMaterialAssetName = TEXT("M_SandbagCover_Burlap");
	const FString SandbagCoverOverlayOutlineMaterialAssetName = TEXT("M_SandbagCover_OverlayOutline");
	const FString SandbagCoverBagMeshAssetName = TEXT("SM_Sandbag_LowPoly");
	const FString SandbagCoverAssetName = TEXT("BP_SandbagCover");
	const FString TransparentObstacleAssetName = TEXT("BP_TransparentObstacle");
	const FString WorldProgressBrokenBridgeAssetName = TEXT("BP_WorldProgress_BrokenBridge");
	const FString WorldProgressRepairedBridgeAssetName = TEXT("BP_WorldProgress_RepairedBridge");
	const FString BrokenBridgeVoxelMeshAssetName = TEXT("SM_Bridge_Broken_Voxel");
	const FString RepairedBridgeVoxelMeshAssetName = TEXT("SM_Bridge_Repaired_Voxel");
	const FString ExplosiveBarrelAssetName = TEXT("BP_ExplosiveBarrel");
	const FString BreakableAppleCrateAssetName = TEXT("BP_BreakableAppleCrate");
	const FString PhysicsAppleAssetName = TEXT("BP_PhysicsApple");
	const FString CrateFragmentAssetName = TEXT("BP_CrateFragment");
	const FString CrateGeometryCollectionAssetName = TEXT("GC_CrateB_Fractured");
	const FString ExplosiveBarrelIntactMeshAssetName = TEXT("SM_ExplosiveBarrel_Intact");
	const FString ExplosiveBarrelDestroyedMeshAssetName = TEXT("SM_ExplosiveBarrel_DestroyedBase");
	const FString ExplosiveBarrelGrayMaterialAssetName = TEXT("M_ExplosiveBarrel_Gray");
	const FString ExplosiveBarrelCharredMaterialAssetName = TEXT("M_ExplosiveBarrel_CharredGray");
	const FString ExplosiveBarrelDetailMaterialAssetName = TEXT("M_ExplosiveBarrel_Detail");
	const FString IntroMapPackagePath = TEXT("/Game/IntroMap");
	const FString OpeningScenarioMapPackagePath = TEXT("/Game/OpeningScenarioMap");
	const FString BunkerMapPackagePath = TEXT("/Game/BunkerMap");
	const FString RaidMapPackagePath = TEXT("/Game/RaidMap");

	struct FUiTextureImportArgs
	{
		FString SourceFile;
		FString DestinationPath = UITitleTextureAssetPath;
		FString AssetName;
		bool bReplaceExisting = true;
	};

	struct FAudioImportArgs
	{
		FString SourceFile;
		FString DestinationPath = AudioBgmAssetPath;
		FString AssetName = TitleBgmAssetName;
		bool bReplaceExisting = true;
		bool bLooping = true;
	};


	struct FEnemyVoxelBox
	{
		int32 X0 = 0;
		int32 Y0 = 0;
		int32 Z0 = 0;
		int32 X1 = 0;
		int32 Y1 = 0;
		int32 Z1 = 0;
		FLinearColor Color = FLinearColor::White;
	};

	struct FBaseballBatRing
	{
		float X = 0.0f;
		float Radius = 0.0f;
	};

// Declarations shared by role-split editor setup translation units.
FString GetGameInstanceObjectPath();
FString GetGameInstanceClassPath();
FString GetAssetObjectPath(const FString& AssetPath, const FString& AssetName);
FString GetAssetClassPath(const FString& AssetPath, const FString& AssetName);
bool SaveAsset(UObject* Asset);
UMaterial* EnsureLedExpressionMaterial();
void AddBoxQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		const FVector3f& D,
		const FVector3f& Normal);
void AddBoxToMesh(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& Min,
		const FVector3f& Max);
void AddColoredBoxQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		const FVector3f& D,
		const FVector3f& Normal,
		const FLinearColor& Color);
FVector3f ConvertVoxelGridPointToMeshPosition(
		int32 X,
		int32 Y,
		int32 Z,
		const FVector3f& Dimensions);
void AddVoxelBoxToMesh(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FEnemyVoxelBox& Box,
		const FVector3f& Dimensions);
void BuildVoxelMeshDescription(
		FMeshDescription& MeshDescription,
		const FName& MaterialSlotName,
		const FVector3f& Dimensions,
		TFunctionRef<void(TArray<FEnemyVoxelBox>&)> AppendBoxes);
void BuildLevelTravelLadderMeshDescription(FMeshDescription& MeshDescription);
UStaticMesh* EnsureLevelTravelLadderMeshAsset();
bool SetProjectGameInstanceToBlueprint();
UBlueprint* EnsureBlueprint(const FString& AssetPath, const FString& AssetName, UClass* ParentClass);
UMaterial* EnsureSolidColorMaterial(
		const FString& AssetPath,
		const FString& AssetName,
		const FLinearColor& BaseColor,
		float Roughness,
		float Metallic,
		float Specular);
UMaterial* EnsureVoxelVertexColorMaterial();
UStaticMesh* EnsureVoxelStaticMeshAsset(
		const FString& AssetPath,
		const FString& AssetName,
		const FName& MaterialSlotName,
		TFunctionRef<void(FMeshDescription&)> BuildMeshDescription,
		UMaterialInterface* VoxelMaterial);
float ComputeSwingArcVertexAlpha(float U, float V);
FVertexInstanceID AddSwingArcVertex(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		const FVector3f& Position,
		const FVector2f& UV,
		const FLinearColor& Color);
void AddSwingArcQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& InnerA,
		const FVector3f& OuterA,
		const FVector3f& OuterB,
		const FVector3f& InnerB,
		float U0,
		float U1);
void BuildLumberjackMeleeSwingArcMeshDescription(FMeshDescription& MeshDescription);
UMaterial* EnsureLumberjackMeleeSwingArcMaterial();
UStaticMesh* EnsureLumberjackMeleeSwingArcMeshAsset(UMaterialInterface* SwingArcMaterial);
bool EnsureLumberjackMeleeSwingArcAssets();
FVertexInstanceID AddBaseballBatVertex(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		const FVector3f& Position,
		const FVector3f& Normal,
		const FVector2f& UV);
void AddBaseballBatSideQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FBaseballBatRing& LeftRing,
		const FBaseballBatRing& RightRing,
		float Angle0,
		float Angle1,
		float U0,
		float U1,
		float V0,
		float V1);
void AddBaseballBatCap(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FBaseballBatRing& Ring,
		bool bRightCap);
void BuildBaseballBatMeshDescription(FMeshDescription& MeshDescription);
UTexture2D* EnsureBaseballBatWoodTexture();
UMaterial* EnsureBaseballBatWoodMaterial(UTexture2D* WoodTexture);
UStaticMesh* EnsureBaseballBatStaticMeshAsset(UMaterialInterface* WoodMaterial);
bool EnsureBaseballBatAssets();
UInputAction* EnsureInputAction(const FString& AssetName, EInputActionValueType ValueType, EInputActionAccumulationBehavior AccumulationBehavior);
FString GetQuickSlotActionName(int32 SlotNumber);
void AddSwizzleModifier(FEnhancedActionKeyMapping& Mapping, UObject* Outer, EInputAxisSwizzle Order);
void AddNegateModifier(FEnhancedActionKeyMapping& Mapping, UObject* Outer);
UInputMappingContext* EnsureInputMappingContext(UInputAction* MoveAction, UInputAction* FireAction, UInputAction* AimAction);
bool HasInputMapping(const UInputMappingContext* MappingContext, const UInputAction* Action, const FKey& Key);
bool EnsureInteractionInputAssets();
bool EnsureInventoryInputAssets();
bool EnsureQuickSlotInputAssets();
bool EnsureDropInputAssets();
bool EnsureAmmoReloadInputAssets();
bool EnsureCameraModeInputAssets();
bool EnsureSprintInputAssets();
bool EnsureRollInputAssets();
bool EnsureMapInputAssets();
bool ConfigureGameModeBlueprint(UBlueprint* GameModeBlueprint, UBlueprint* PlayerBlueprint);
bool SetProjectGameModeToBlueprint();
bool SetProjectStartupMapsToIntro();
bool EnsureGameInstanceBlueprint();
bool EnsureProjectileHitEffectAssets();
bool EnsureWeaponSpreadRecoilAssets();
bool EnsureWeaponPresentationAssets();
bool EnsureTopDownShooterAssets();
bool EnsureCanBotBlueprint();
bool EnsureEnemyVisualMaterialAssets();
bool EnsureRollingBomberBodyGrayMaterial();
bool EnsureRollingBomberLegMetalMaterial();
FVector3f MakeBarrelVertex(float AngleRadians, float Radius, float Height);
FVector3f MakeBarrelRadialNormal(const FVector3f& Position);
FVector3f MakeBarrelRadialTangent(const FVector3f& Position);
FVector3f MakeBarrelSafeTangent(const FVector3f& Normal, const FVector3f& PreferredTangent);
void AddBarrelTriangle(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		const FVector2f& UvA,
		const FVector2f& UvB,
		const FVector2f& UvC,
		const FVector3f& SurfaceNormal);
void AddBarrelQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		const FVector3f& D,
		float U0,
		float U1,
		float V0,
		float V1);
void BuildExplosiveBarrelIntactMeshDescription(FMeshDescription& MeshDescription);
void BuildExplosiveBarrelDestroyedMeshDescription(FMeshDescription& MeshDescription);
UStaticMesh* EnsureExplosiveBarrelStaticMesh(
		const FString& AssetName,
		UMaterialInterface* BarrelMaterial,
		UMaterialInterface* DetailMaterial,
		TFunctionRef<void(FMeshDescription&)> BuildMeshDescription);
bool ConfigureExplosiveBarrelBlueprint(UBlueprint* ExplosiveBarrelBlueprint);
bool EnsureExplosiveBarrelAssets();
UGeometryCollection* EnsureBreakableAppleCrateGeometryCollection();
bool EnsureBreakableAppleCrateAssets();
bool EnsureSharedVoxelMeshAssets();
bool EnsureCoverPointAssets();
void RegisterWidgetVariable(UWidgetBlueprint* WidgetBlueprint, UWidget* Widget);
void UnregisterWidgetVariable(UWidgetBlueprint* WidgetBlueprint, const FName& VariableName);
void SyncWidgetVariableGuidsToSource(UWidgetBlueprint* WidgetBlueprint);
void RegisterAllWidgetsInTree(UWidgetBlueprint* WidgetBlueprint);
void ClearWidgetTreeForRebuild(UWidgetBlueprint* WidgetBlueprint);
void ConfigureTextBlock(UTextBlock* TextBlock, const FText& Text, const FLinearColor& Color, int32 FontSize);
void ConfigureTextBlockLeft(UTextBlock* TextBlock, const FText& Text, const FLinearColor& Color, int32 FontSize);
FSlateBrush MakeRoundedBoxBrush(const FVector2D& ImageSize, const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth);
FSlateBrush MakeRoundedBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		float CornerRadius);
FSlateBrush MakeCircularBrush(const FVector2D& ImageSize, const FLinearColor& FillColor, const FLinearColor& OutlineColor, float OutlineWidth);
FScrollBarStyle MakeItemContainerScrollBarStyle();
void ApplyItemContainerScrollBarStyle(UTileView* TileView);
bool BuildIntroMenuWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool BuildTitleIntroMenuWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool BuildLevelTransitionVideoWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool BuildSpeechBubbleWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool BuildQuestWidgetTree(UWidgetBlueprint* WidgetBlueprint);
FString GetItemIconSourcePath(const FString& IconAssetName);
FString GetGeneratedUiImageSourcePath(const FString& ImageFileName);
void ConfigureImportedIconTexture(UTexture2D* Texture);
void ConfigureImportedUiTexture(UTexture2D* Texture);
void ConfigureImportedWorldTexture(UTexture2D* Texture);
void ConfigureImportedEffectTexture(UTexture2D* Texture);
void ConfigureImportedMaskTexture(UTexture2D* Texture);
bool ImportWorldTexture(
		const FString& InSourceFile,
		const FString& DestinationPath,
		const FString& AssetName,
		UTexture2D** OutTexture);
FString GetRollingBomberChargeCylinderMaskSourcePath();
void AddRollingBomberChargeCylinderQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		float Angle0,
		float Angle1,
		float U0,
		float U1);
void BuildRollingBomberChargeCylinderMeshDescription(FMeshDescription& MeshDescription);
UMaterial* EnsureRollingBomberChargeCylinderMaterial(UTexture2D* MaskTexture);
UStaticMesh* EnsureRollingBomberChargeCylinderMesh(UMaterialInterface* ChargeMaterial);
bool EnsureRollingBomberChargeCylinderEffectAssets();
FString GetLocalExplosionFlipbookSourcePath();
UMaterial* EnsureLocalExplosionFlipbookMaterial(
		UTexture2D* FlipbookTexture,
		const FString& MaterialAssetName,
		bool bSmokeMaterial);
UMaterial* EnsureLocalExplosionDistortionMaterial();
bool EnsureLocalExplosionEffectAssets();
bool SetNiagaraStoreBoolByName(FNiagaraParameterStore& Store, FName ParameterName, bool bValue);
bool SetNiagaraStoreFloatByName(FNiagaraParameterStore& Store, FName ParameterName, float Value);
bool SetNiagaraStoreVec3ByName(FNiagaraParameterStore& Store, FName ParameterName, const FVector& Value);
bool SetNiagaraStoreColorByName(FNiagaraParameterStore& Store, FName ParameterName, const FLinearColor& Value);
int32 ConfigureExtractionSmokeSignalNiagaraScript(UNiagaraScript* Script);
bool ConfigureExtractionSmokeSignalNiagaraSystem(UNiagaraSystem* System);
UObject* LoadExtractionSmokeSignalSourceTemplate();
bool DeleteExistingExtractionSmokeSignalNiagaraSystem(const FString& ObjectPath);
bool EnsureExtractionSmokeSignalNiagaraSystem();
int32 ConfigureLookdevFluidExplosionNiagaraScript(UNiagaraScript* Script);
bool ConfigureLookdevFluidExplosionNiagaraSystem(UNiagaraSystem* System);
UObject* LoadLookdevFluidExplosionSourceTemplate();
bool DeleteExistingLookdevFluidExplosionNiagaraSystem(const FString& ObjectPath);
UNiagaraSystem* EnsureLookdevFluidExplosionNiagaraSystem();
void ApplyLookdevNiagaraComponentOverrides(UNiagaraComponent* NiagaraComponent);
AStaticMeshActor* SpawnLookdevStaticMeshActor(
		UWorld* World,
		const TCHAR* ActorName,
		const TCHAR* ActorLabel,
		UStaticMesh* StaticMesh,
		UMaterialInterface* Material,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale);
bool EnsureLookdevFluidExplosionCaptureLevel(UNiagaraSystem* NiagaraSystem);
bool EnsureLookdevFluidExplosionAssetsAndLevel();
UMaterial* EnsureMemoStorageDeviceMaterial(UTexture2D* StorageTexture);
bool ImportMemoStorageDeviceTextureFromCommandLineIfRequested();
UMaterial* EnsureRollingBomberSpawnerMaterial(UTexture2D* SpawnerTexture);
bool ImportRollingBomberSpawnerTextureFromCommandLineIfRequested();
FString GetSandbagCoverTextureSourcePath();
UMaterial* EnsureSandbagCoverMaterial(UTexture2D* SandbagTexture);
UMaterial* EnsureSandbagCoverOverlayOutlineMaterial();
FVector3f MakeSandbagCoverSafeTangent(const FVector3f& Normal, const FVector3f& PreferredTangent);
void BuildSandbagLoafMeshDescription(
		FMeshDescription& MeshDescription,
		const FVector3f& Extent,
		int32 SegmentCount,
		const TArray<float>& RingPositions,
		const TArray<float>& RingWidthScales,
		const TArray<float>& RingHeightScales,
		const FLinearColor& VertexColor);
void BuildSandbagLowPolyMeshDescription(FMeshDescription& MeshDescription);
UStaticMesh* EnsureSandbagCoverStaticMeshAsset(
		const FString& AssetName,
		UMaterialInterface* SandbagMaterial,
		TFunctionRef<void(FMeshDescription&)> BuildMeshDescription);
bool EnsureSandbagCoverAssets();
bool ImportSandbagCoverTextureFromCommandLineIfRequested();
bool ImportUiTexture(const FUiTextureImportArgs& Args, UTexture2D** OutTexture);
bool TryReadUiTextureImportArgsFromCommandLine(FUiTextureImportArgs& OutArgs);
bool ImportUiTextureFromCommandLineIfRequested();
void ConfigureImportedBgmSound(USoundWave* SoundWave, bool bLooping);
bool ImportAudioAsset(const FAudioImportArgs& Args, USoundWave** OutSoundWave);
bool TryReadAudioImportArgsFromCommandLine(FAudioImportArgs& OutArgs);
bool ImportAudioFromCommandLineIfRequested();
FString GetWorkspaceFilePath(const FString& RelativePath);
bool EnsureTitleUiTextures();
bool EnsureOpeningScenarioUiTextures();
bool EnsureItemIconTextures();
bool ImportIconTextureSources(const TArray<FString>& IconAssetNames, bool bReplaceExisting);
bool EnsureEquipmentIconTextures();
bool EnsureCannedTunaIconTexture();
bool ImportHudStatusIconTexture(const FString& SourceFileName, const FString& AssetName);
bool EnsureHudStatusIconTextures();
void SetListViewEntryWidgetClass(UListViewBase* ListViewBase, TSubclassOf<UUserWidget> EntryWidgetClass);
UWidgetBlueprint* EnsureWidgetBlueprint(const FString& AssetPath, const FString& AssetName, UClass* ParentClass);
bool BuildItemThumbnailSlotWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool BuildHudTopReserveWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool BuildHudBottomStatusWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool BuildHudDebuffBarWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool BuildHudQuickSlotBarWidgetTree(UWidgetBlueprint* WidgetBlueprint);
UBorder* BuildHudSimplePanel(
		UWidgetTree* WidgetTree,
		const FName& PanelName,
		const FText& Title,
		const FVector2D& PanelSize,
		const FLinearColor& AccentColor);
bool BuildHudInventoryAreaWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass);
bool BuildHudItemInfoPanelWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass);
bool BuildLootContainerWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass);
bool BuildWorkbenchRecipeListEntryWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool BuildWorkbenchPanelWidgetTree(
		UWidgetBlueprint* WidgetBlueprint,
		TSubclassOf<UUserWidget> EntryWidgetClass,
		TSubclassOf<UUserWidget> WorkbenchRecipeEntryWidgetClass);
bool BuildHudExternalPanelWidgetTree(
		UWidgetBlueprint* WidgetBlueprint,
		TSubclassOf<UUserWidget> LootContainerWidgetClass,
		TSubclassOf<UUserWidget> StorageContainerWidgetClass,
		TSubclassOf<UUserWidget> ShopContainerWidgetClass,
		TSubclassOf<UUserWidget> WorkbenchPanelWidgetClass);
bool BuildGameHudWidgetTree(
		UWidgetBlueprint* WidgetBlueprint,
		TSubclassOf<UUserWidget> TopReserveWidgetClass,
		TSubclassOf<UUserWidget> BottomStatusWidgetClass,
		TSubclassOf<UUserWidget> DebuffBarWidgetClass,
		TSubclassOf<UUserWidget> QuickSlotBarWidgetClass,
		TSubclassOf<UUserWidget> InventoryAreaWidgetClass,
		TSubclassOf<UUserWidget> ItemInfoPanelWidgetClass,
		TSubclassOf<UUserWidget> ExternalPanelWidgetClass);
bool BuildPickupItemIconWidgetTree(UWidgetBlueprint* WidgetBlueprint);
bool EnsureCommonGameHudAssets();
bool EnsureLootContainerOccupancyHeaderAssets();
bool EnsureBackpackInventoryAssets();
bool BuildInteractionMarkerWidgetTree(UWidgetBlueprint* WidgetBlueprint);
UWidgetBlueprint* EnsureInteractionMarkerWidgetBlueprint();
bool RebuildInteractionMarkerWidgetAlignment();
bool ConfigureIntroMenuWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint);
bool ConfigureLevelTransitionVideoWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint);
bool ConfigureSpeechBubbleWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint);
bool ConfigureQuestWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint);
UTexture2D* EnsureWarpPointNoiseTexture();
UMaterial* EnsureWarpPointEnergyMaterial(UTexture2D* NoiseTexture);
bool ConfigureLevelTravelBlueprint(UBlueprint* LevelTravelBlueprint);
bool ConfigureWarpPointBlueprint(UBlueprint* WarpPointBlueprint);
bool ConfigureSelfDestructBlueprint(UBlueprint* SelfDestructBlueprint);
bool ConfigureSelfDestructActorInstance(AActor* Actor);
bool ConfigurePickupItemIconWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint);
bool ConfigurePickupItemBlueprint(UBlueprint* PickupItemBlueprint, int32 ItemId);
bool ConfigureItemSpawnBlueprint(UBlueprint* ItemSpawnBlueprint);
bool ConfigureLootContainerBlueprint(UBlueprint* LootContainerBlueprint, int32 ContainerDefinitionId, int32 ContentsId);
bool ConfigureLootContainerSpawnBlueprint(UBlueprint* LootContainerSpawnBlueprint);
bool ConfigurePickupItemActorInstance(AActor* Actor, int32 ItemId);
bool ConfigureItemSpawnActorInstance(AActor* Actor);
bool ConfigureLootContainerActorInstance(AActor* Actor, int32 ContainerDefinitionId, int32 ContentsId);
bool ConfigureLootContainerSpawnActorInstance(AActor* Actor);
AActor* FindActorByLabel(UWorld* World, const FString& ActorLabel);
UWorld* LoadEditorMapForSetup(const FString& MapPackagePath);
bool IsEditorWorldReadyForMapSetup();
bool ConfigureEditorMapCaptureActorInstance(AActor* Actor, bool bAutoDetectBounds);
bool PlaceEditorMapCaptureActorInRaidMap(UBlueprint* MapCaptureBlueprint);
bool EnsureEditorMapCaptureBlueprintAndRaidPlacement();
bool EnsureOpeningScenarioMap();
bool EnsureOpeningScenarioPresentationSetup();
bool PlacePickupItemActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location, int32 ItemId);
bool PlaceItemSpawnActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location);
bool PlaceLootContainerActor(
		UWorld* World,
		UBlueprint* ActorBlueprint,
		const FString& ActorLabel,
		const FVector& Location,
		int32 ContainerDefinitionId,
		int32 ContentsId);
bool PlaceLootContainerSpawnActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location);
bool PlaceSelfDestructActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location);
bool PlacePickupItemAndSpawnerActorsInRaidMap(UBlueprint* PickupItemBlueprint, UBlueprint* ItemSpawnBlueprint);
bool EnsurePickupItemAndSpawnerAssetsAndMapPlacement();
bool PlaceLootContainerAndSpawnerActorsInRaidMap(UBlueprint* LootContainerBlueprint, UBlueprint* LootContainerSpawnBlueprint);
bool EnsureLootContainerAndSpawnerAssetsAndMapPlacement();
bool PlaceSelfDestructActorInRaidMap(UBlueprint* SelfDestructBlueprint);
bool EnsureSelfDestructInteractionSetup();
bool EnsureIntroMenuAndLevelTravelSetup();
bool EnsureIntroMenuGraphicsSettingsSetup();
bool EnsureBunkerToRaidTransitionVideoSetup();
bool EnsureFirstOutingQuestSetup();
bool EnsureWorldProgressInteractionAssets();
bool EnsureWarpPointInteractionAssets();
void RunEditorOneShotSetup_ToCleanupOnExplicitRequest();
void SchedulePickupItemAndSpawnerAssetsAndMapPlacement();
void ScheduleLootContainerAndSpawnerAssetsAndMapPlacement();
void ScheduleEditorMapCaptureSetup();
void ScheduleLookdevFluidExplosionSetup();
void ScheduleIntroMenuAndLevelTravelSetup();
void ScheduleOpeningScenarioPresentationSetup();
void ScheduleBunkerToRaidTransitionVideoSetup();
void ScheduleFirstOutingQuestSetup();
void ScheduleSelfDestructInteractionSetup();
}
