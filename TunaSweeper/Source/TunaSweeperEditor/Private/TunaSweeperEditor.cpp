#include "Modules/ModuleManager.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperCoverPointActor.h"
#include "AutomatedAssetImportData.h"
#include "Blueprint/WidgetTree.h"
#include "Character/TunaSweeperQuestNpcActor.h"
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
#include "Components/ListViewBase.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Containers/Ticker.h"
#include "Effect/TunaSweeperProjectileHitBurstActor.h"
#include "Effect/TunaSweeperProjectileHitEffectDataAsset.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
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
#include "Interaction/TunaSweeperExplosiveBarrelActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperItemSpawnInteractableActor.h"
#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Interaction/TunaSweeperLootContainerSpawnInteractableActor.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "Interaction/TunaSweeperSandbagCoverActor.h"
#include "Interaction/TunaSweeperSelfDestructInteractableActor.h"
#include "Interaction/TunaSweeperTransparentObstacleActor.h"
#include "Interaction/TunaSweeperWarpPointActor.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
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
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionLength.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "MeshDescription.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "NiagaraEmitter.h"
#include "NiagaraParameterStore.h"
#include "NiagaraScript.h"
#include "NiagaraSystem.h"
#include "NiagaraSystemImpl.h"
#include "NiagaraTypes.h"
#include "ObjectTools.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Sound/SoundWave.h"
#include "StaticMeshAttributes.h"
#include "TunaSweeperExperimentalVegetation.h"
#include "TunaSweeperFMSoundTool.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "TunaSweeperEditorRunOnce.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"
#include "UI/TunaSweeperGameHudWidget.h"
#include "UI/TunaSweeperHudBottomStatusWidget.h"
#include "UI/TunaSweeperHudExternalPanelWidget.h"
#include "UI/TunaSweeperHudInventoryAreaWidget.h"
#include "UI/TunaSweeperHudItemInfoPanelWidget.h"
#include "UI/TunaSweeperHudQuickSlotBarWidget.h"
#include "UI/TunaSweeperHudTopReserveWidget.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperItemThumbnailSlotWidget.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "UI/TunaSweeperLevelTransitionWidget.h"
#include "UI/TunaSweeperLootContainerWidget.h"
#include "UI/TunaSweeperPickupItemIconWidget.h"
#include "UI/TunaSweeperQuestWidget.h"
#include "UI/TunaSweeperReloadRingWidget.h"
#include "UI/TunaSweeperSpeechBubbleWidget.h"
#include "UI/TunaSweeperWorkbenchPanelWidget.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "Weapon/TunaSweeperProjectile.h"
#include "Weapon/TunaSweeperWeapon.h"
#include "Weapon/TunaSweeperWeaponSpreadRecoilDataAsset.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperEditor, Log, All);

namespace TunaSweeperEditorSetup
{
	const FString GameInstanceTaskId = TEXT("2026-05-10_CreateGameInstanceBlueprint");
	const FString TopDownShooterTaskId = TEXT("2026-05-10_CreateTopDownShooterAssets");
	const FString InteractionInputTaskId = TEXT("2026-05-11_SetInteractInputToFKey");
	const FString InteractionMarkerAlignmentTaskId = TEXT("2026-05-25_RebuildInteractionMarkerRequirementPreviewV1");
	const FString PickupItemAndSpawnerTaskId = TEXT("2026-05-11_CreatePickupItemAndSpawnerAssetsV3");
	const FString CommonGameHudTaskId = TEXT("2026-05-28_AddMeleeQuickSlotHudV1");
	const FString WorkbenchPanelWidgetTaskId = TEXT("2026-05-29_CreateWorkbenchPanelWidgetV1");
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
	const FString LootContainerOccupancyHeaderTaskId = TEXT("2026-05-18_AddLootContainerOccupancyHeaderV1");
	const FString CannedTunaIconImportTaskId = TEXT("2026-05-11_ImportCannedTunaIconV1");
	const FString BackpackInventoryTaskId = TEXT("2026-05-16_CreateEquipmentInventoryAssetsV3");
	const FString IntroMenuAndLevelTravelTaskId = TEXT("2026-05-24_CreateTitleIntroMenuPersistentSaveSlotSelectionLevelTravelLadderInitialScaleV1");
	const FString IntroMenuGraphicsSettingsTaskId = TEXT("2026-05-28_AddTitleFullscreenSettingsLayoutV1");
	const FString OpeningScenarioPresentationTaskId = TEXT("2026-05-19_CreateOpeningScenarioPresentationV2");
	const FString LevelTransitionVideoTaskId = TEXT("2026-05-16_AddBidirectionalLevelTransitionVideoV3");
	const FString FirstOutingQuestTaskId = TEXT("2026-05-15_CreateFirstOutingQuestNpcV2");
	const FString SelfDestructInteractionTaskId = TEXT("2026-05-16_CreateSelfDestructInteractionV1");
	const FString WorldProgressInteractionTaskId = TEXT("2026-05-19_CreateWorldProgressObstacleAssetsV1");
	const FString WarpPointInteractionTaskId = TEXT("2026-05-25_CreateWarpPointInteractionAssetsV1");
	const FString EnemyVisualMaterialTaskId = TEXT("2026-05-19_CreateEnemyAndContainerVisualMaterialsV3");
	const FString ExplosiveBarrelTaskId = TEXT("2026-05-29_CreateExplosiveBarrelAssetsV8");
	const FString RollingBomberBodyMaterialTaskId = TEXT("2026-05-28_CreateRollingBomberBodyGrayMaterialV1");
	const FString RollingBomberLegMaterialTaskId = TEXT("2026-05-28_CreateRollingBomberLegMetalMaterialV1");
	const FString RollingBomberChargeCylinderEffectTaskId = TEXT("2026-05-28_CreateRollingBomberChargeCylinderEffectV1");
	const FString LocalExplosionEffectTaskId = TEXT("2026-05-29_CreateLocalExplosionFlipbookEffectV3");
	const FString ExtractionSmokeSignalNiagaraSystemTaskId = TEXT("2026-05-29_CreateExtractionSmokeSignalNiagaraSystemV4");
	const FString ProjectileHitEffectAssetTaskId = TEXT("2026-05-28_CreateProjectileHitEffectAssetsV1");
	const FString WeaponSpreadRecoilAssetTaskId = TEXT("2026-05-28_CreateWeaponSpreadRecoilAssetsV1");
	const FString BaseballBatAssetTaskId = TEXT("2026-05-28_CreateBaseballBatStaticMeshAssetsV1");
	const FString SandbagCoverAssetTaskId = TEXT("2026-05-29_CreateSandbagCoverAssetsV1");
	const FString VoxelMeshAssetTaskId = TEXT("2026-05-19_CreateSharedVoxelMeshAssetsV1");
	const FString LumberjackMeleeSwingArcAssetTaskId = TEXT("2026-05-20_CreateLumberjackMeleeSwingArcAssetsV2");
	const FString LedExpressionMaterialTaskId = TEXT("2026-05-26_CreateLedExpressionMaterialV1");
	const FString ExperimentalVegetationAssetTaskId = TEXT("2026-05-24_CreateExperimentalVegetationStaticMeshV4");
	const FString CoverPointAssetTaskId = TEXT("2026-05-16_CreateCoverPointBlueprintV1");
	const FString CanBotBlueprintTaskId = TEXT("2026-05-25_CreateCanBotBlueprintV1");
	const FString GameInstanceAssetPath = TEXT("/Game/Core");
	const FString GameInstanceAssetName = TEXT("BP_TunaSweeperGameInstance");
	const FString GameModeAssetName = TEXT("BP_TunaSweeperGameMode");
	const FString PlayerAssetPath = TEXT("/Game/Characters/Player");
	const FString PlayerAssetName = TEXT("BP_TunaSweeperPlayerCharacter");
	const FString NpcAssetPath = TEXT("/Game/Characters/NPC");
	const FString InstructorQuestNpcAssetName = TEXT("BP_NPC_InstructorQuest");
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
	const FString BaseballBatWoodTextureAssetName = TEXT("T_BaseballBat_WoodGrain");
	const FString BaseballBatMaterialAssetName = TEXT("M_BaseballBat_Wood");
	const FString BaseballBatMeshAssetName = TEXT("SM_BaseballBat");
	const FString InputAssetPath = TEXT("/Game/Input");
	const FString MoveActionName = TEXT("IA_Move");
	const FString FireActionName = TEXT("IA_Fire");
	const FString AimActionName = TEXT("IA_Aim");
	const FString InteractActionName = TEXT("IA_Interact");
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
	const FString RollingBomberChargeCylinderMaskTextureAssetName = TEXT("T_RollingBomberChargeCylinderMask");
	const FString RollingBomberChargeCylinderMaterialAssetName = TEXT("M_RollingBomberChargeCylinder");
	const FString RollingBomberChargeCylinderMeshAssetName = TEXT("SM_RollingBomberChargeCylinder_Open");
	const FString LocalExplosionFlipbookTextureAssetName = TEXT("T_LocalExplosionFlipbook");
	const FString LocalExplosionFlipbookMaterialAssetName = TEXT("M_LocalExplosionFlipbook");
	const FString LocalExplosionDistortionMaterialAssetName = TEXT("M_LocalExplosionDistortion");
	const FString LocalExplosionSmokeMaterialAssetName = TEXT("M_LocalExplosionSmoke");
	const FString ExtractionSmokeSignalNiagaraSystemAssetName = TEXT("NS_ExtractionSmokeSignal");
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
	const FString HudQuickSlotBarWidgetAssetName = TEXT("WBP_HudQuickSlotBar");
	const FString HudInventoryAreaWidgetAssetName = TEXT("WBP_HudInventoryArea");
	const FString HudItemInfoPanelWidgetAssetName = TEXT("WBP_HudItemInfoPanel");
	const FString HudExternalPanelWidgetAssetName = TEXT("WBP_HudExternalPanel");
	const FString ItemThumbnailSlotWidgetAssetName = TEXT("WBP_ItemThumbnailSlot");
	const FString LootContainerWidgetAssetName = TEXT("WBP_LootContainerPanel");
	const FString WorkbenchPanelWidgetAssetName = TEXT("WBP_WorkbenchPanel");
	const FString IntroMenuWidgetAssetName = TEXT("WBP_IntroMenu");
	const FString LevelTransitionVideoWidgetAssetName = TEXT("WBP_LevelTransitionVideo");
	const FString QuestWidgetAssetName = TEXT("WBP_Quest");
	const FString SpeechBubbleWidgetAssetName = TEXT("WBP_SpeechBubble");
	constexpr float HudTopModeTabButtonWidth = 52.0f;
	constexpr float HudTopModeTabButtonHeight = 46.0f;
	constexpr float HudTopModeTabGap = 8.0f;
	constexpr float HudTopModeTabPaddingX = 12.0f;
	constexpr float HudTopModeTabPaddingY = 10.0f;
	constexpr float HudTopModeTabPanelWidth =
		4.0f * HudTopModeTabButtonWidth + 3.0f * HudTopModeTabGap + 2.0f * HudTopModeTabPaddingX;
	constexpr float HudTopModeTabPanelHeight = HudTopModeTabButtonHeight + 2.0f * HudTopModeTabPaddingY;
	constexpr int32 InventoryTileColumnCount = 5;
	constexpr int32 EquipmentReserveColumnCount = 4;
	constexpr float InventoryTileWidth = 96.0f;
	constexpr float InventoryTileHeight = 96.0f;
	constexpr float InventoryPanelPadding = 14.0f;
	constexpr float InventoryTileViewScrollbarReserveWidth = 22.0f;
	constexpr float InventoryTileViewWidth = InventoryTileColumnCount * InventoryTileWidth + InventoryTileViewScrollbarReserveWidth;
	constexpr float InventoryPanelWidth = InventoryPanelPadding * 2.0f + InventoryTileViewWidth;
	constexpr float EquipmentReserveEntryWidth = InventoryTileViewWidth / EquipmentReserveColumnCount;
	constexpr float EquipmentReserveHeight = 2.0f * InventoryTileHeight;
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
	const FString SandbagCoverOutlineMaterialAssetName = TEXT("M_SandbagCover_Outline");
	const FString SandbagCoverAssetName = TEXT("BP_SandbagCover");
	const FString TransparentObstacleAssetName = TEXT("BP_TransparentObstacle");
	const FString WorldProgressBrokenBridgeAssetName = TEXT("BP_WorldProgress_BrokenBridge");
	const FString WorldProgressRepairedBridgeAssetName = TEXT("BP_WorldProgress_RepairedBridge");
	const FString BrokenBridgeVoxelMeshAssetName = TEXT("SM_Bridge_Broken_Voxel");
	const FString RepairedBridgeVoxelMeshAssetName = TEXT("SM_Bridge_Repaired_Voxel");
	const FString ExplosiveBarrelAssetName = TEXT("BP_ExplosiveBarrel");
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
		const FVector3f& Normal)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		const FVector3f Positions[] = { A, B, C, D };
		const FVector2f UVs[] = {
			FVector2f(0.0f, 0.0f),
			FVector2f(1.0f, 0.0f),
			FVector2f(1.0f, 1.0f),
			FVector2f(0.0f, 1.0f)
		};

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = Normal;
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void AddBoxToMesh(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& Min,
		const FVector3f& Max)
	{
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(0.0f, 0.0f, -1.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(0.0f, 0.0f, 1.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(1.0f, 0.0f, 0.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(-1.0f, 0.0f, 0.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(0.0f, 1.0f, 0.0f));
		AddBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(0.0f, -1.0f, 0.0f));
	}

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

#include "EnemyVoxelBodyShape.inl"
#include "EnemyVoxelForwardMarkerShape.inl"
#include "BridgeVoxelBrokenShape.inl"
#include "BridgeVoxelRepairedShape.inl"

	void AddColoredBoxQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& A,
		const FVector3f& B,
		const FVector3f& C,
		const FVector3f& D,
		const FVector3f& Normal,
		const FLinearColor& Color)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = Attributes.GetVertexInstanceColors();

		const FVector3f Positions[] = { A, B, C, D };
		const FVector2f UVs[] = {
			FVector2f(0.0f, 0.0f),
			FVector2f(1.0f, 0.0f),
			FVector2f(1.0f, 1.0f),
			FVector2f(0.0f, 1.0f)
		};
		const FVector4f VertexColor(Color.R, Color.G, Color.B, Color.A);

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = Normal;
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstanceColors[VertexInstanceId] = VertexColor;
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	FVector3f ConvertVoxelGridPointToMeshPosition(
		int32 X,
		int32 Y,
		int32 Z,
		const FVector3f& Dimensions)
	{
		constexpr float VoxelResolution = 32.0f;
		return FVector3f(
			(static_cast<float>(X) / VoxelResolution - 0.5f) * Dimensions.X,
			(static_cast<float>(Y) / VoxelResolution - 0.5f) * Dimensions.Y,
			(static_cast<float>(Z) / VoxelResolution - 0.5f) * Dimensions.Z);
	}

	void AddVoxelBoxToMesh(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FEnemyVoxelBox& Box,
		const FVector3f& Dimensions)
	{
		const int32 X0 = FMath::Clamp(Box.X0, 0, 32);
		const int32 Y0 = FMath::Clamp(Box.Y0, 0, 32);
		const int32 Z0 = FMath::Clamp(Box.Z0, 0, 32);
		const int32 X1 = FMath::Clamp(Box.X1, 0, 32);
		const int32 Y1 = FMath::Clamp(Box.Y1, 0, 32);
		const int32 Z1 = FMath::Clamp(Box.Z1, 0, 32);
		if (X0 >= X1 || Y0 >= Y1 || Z0 >= Z1)
		{
			return;
		}

		const FVector3f Min = ConvertVoxelGridPointToMeshPosition(X0, Y0, Z0, Dimensions);
		const FVector3f Max = ConvertVoxelGridPointToMeshPosition(X1, Y1, Z1, Dimensions);

		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(0.0f, 0.0f, -1.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(0.0f, 0.0f, 1.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(1.0f, 0.0f, 0.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(-1.0f, 0.0f, 0.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Max.Y, Min.Z),
			FVector3f(Min.X, Max.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Max.Z),
			FVector3f(Max.X, Max.Y, Min.Z),
			FVector3f(0.0f, 1.0f, 0.0f),
			Box.Color);
		AddColoredBoxQuad(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(Min.X, Min.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Min.Z),
			FVector3f(Max.X, Min.Y, Max.Z),
			FVector3f(Min.X, Min.Y, Max.Z),
			FVector3f(0.0f, -1.0f, 0.0f),
			Box.Color);
	}

	void BuildVoxelMeshDescription(
		FMeshDescription& MeshDescription,
		const FName& MaterialSlotName,
		const FVector3f& Dimensions,
		TFunctionRef<void(TArray<FEnemyVoxelBox>&)> AppendBoxes)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = MaterialSlotName;

		TArray<FEnemyVoxelBox> Boxes;
		AppendBoxes(Boxes);
		for (const FEnemyVoxelBox& Box : Boxes)
		{
			AddVoxelBoxToMesh(MeshDescription, Attributes, PolygonGroupId, Box, Dimensions);
		}
	}

	void BuildLevelTravelLadderMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Ladder"));

		constexpr float HalfLength = 130.0f;
		constexpr float RailHalfWidth = 5.0f;
		constexpr float RailCenterOffset = 38.0f;
		constexpr float RungHalfLength = 34.0f;
		constexpr float RungHalfWidth = 5.0f;
		constexpr float Thickness = 8.0f;

		AddBoxToMesh(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(-HalfLength, -RailCenterOffset - RailHalfWidth, 0.0f),
			FVector3f(HalfLength, -RailCenterOffset + RailHalfWidth, Thickness));
		AddBoxToMesh(
			MeshDescription,
			Attributes,
			PolygonGroupId,
			FVector3f(-HalfLength, RailCenterOffset - RailHalfWidth, 0.0f),
			FVector3f(HalfLength, RailCenterOffset + RailHalfWidth, Thickness));

		constexpr int32 RungCount = 7;
		for (int32 RungIndex = 0; RungIndex < RungCount; ++RungIndex)
		{
			const float Alpha = RungCount == 1 ? 0.5f : static_cast<float>(RungIndex) / static_cast<float>(RungCount - 1);
			const float X = FMath::Lerp(-HalfLength + 24.0f, HalfLength - 24.0f, Alpha);
			AddBoxToMesh(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				FVector3f(X - RungHalfWidth, -RungHalfLength, Thickness),
				FVector3f(X + RungHalfWidth, RungHalfLength, Thickness * 2.0f));
		}
	}

	UStaticMesh* EnsureLevelTravelLadderMeshAsset()
	{
		const FString AssetObjectPath = GetAssetObjectPath(InteractionAssetPath, LevelTravelLadderMeshAssetName);
		if (UStaticMesh* ExistingMesh = LoadObject<UStaticMesh>(nullptr, *AssetObjectPath))
		{
			return ExistingMesh;
		}

		const FString PackageName = FString::Printf(TEXT("%s/%s"), *InteractionAssetPath, *LevelTravelLadderMeshAssetName);
		UPackage* Package = CreatePackage(*PackageName);
		if (!Package)
		{
			return nullptr;
		}

		UStaticMesh* StaticMesh = NewObject<UStaticMesh>(
			Package,
			*LevelTravelLadderMeshAssetName,
			RF_Public | RF_Standalone | RF_Transactional);
		if (!StaticMesh)
		{
			return nullptr;
		}

		FMeshDescription MeshDescription;
		BuildLevelTravelLadderMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Add(FStaticMaterial());

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
		StaticMesh->MarkPackageDirty();

		FAssetRegistryModule::AssetCreated(StaticMesh);
		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
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

	UMaterial* EnsureSolidColorMaterial(
		const FString& AssetPath,
		const FString& AssetName,
		const FLinearColor& BaseColor,
		float Roughness = 0.65f,
		float Metallic = 0.0f,
		float Specular = 0.25f)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				AssetName,
				AssetPath,
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

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionConstant3Vector* BaseColorExpression = NewObject<UMaterialExpressionConstant3Vector>(Material);
		BaseColorExpression->Material = Material;
		BaseColorExpression->Constant = BaseColor;
		BaseColorExpression->MaterialExpressionEditorX = -240;
		BaseColorExpression->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(BaseColorExpression);
		MaterialEditorOnly->BaseColor.Connect(0, BaseColorExpression);

		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = Roughness;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = Metallic;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = Specular;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UMaterial* EnsureVoxelVertexColorMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(VoxelAssetPath, VoxelVertexColorMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				VoxelVertexColorMaterialAssetName,
				VoxelAssetPath,
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

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -240;
		VertexColorExpression->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);
		MaterialEditorOnly->BaseColor.Connect(0, VertexColorExpression);

		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.8f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.2f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UStaticMesh* EnsureVoxelStaticMeshAsset(
		const FString& AssetPath,
		const FString& AssetName,
		const FName& MaterialSlotName,
		TFunctionRef<void(FMeshDescription&)> BuildMeshDescription,
		UMaterialInterface* VoxelMaterial)
	{
		const FString ObjectPath = GetAssetObjectPath(AssetPath, AssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *AssetPath, *AssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*AssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FMeshDescription MeshDescription;
			BuildMeshDescription(MeshDescription);

			StaticMesh->GetStaticMaterials().Reset();
			StaticMesh->GetStaticMaterials().Add(FStaticMaterial(VoxelMaterial, MaterialSlotName));

			TArray<const FMeshDescription*> MeshDescriptions;
			MeshDescriptions.Add(&MeshDescription);
			StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
			FAssetRegistryModule::AssetCreated(StaticMesh);
		}
		else
		{
			StaticMesh->Modify();
			if (StaticMesh->GetStaticMaterials().Num() == 0)
			{
				StaticMesh->GetStaticMaterials().Add(FStaticMaterial(VoxelMaterial, MaterialSlotName));
			}
			else
			{
				StaticMesh->GetStaticMaterials()[0] = FStaticMaterial(VoxelMaterial, MaterialSlotName);
			}
		}

		StaticMesh->MarkPackageDirty();
		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	float ComputeSwingArcVertexAlpha(float U, float V)
	{
		const float AlongArcFade = FMath::Pow(FMath::Clamp(FMath::Sin(U * PI), 0.0f, 1.0f), 0.55f);
		const float WidthFade = FMath::Lerp(0.38f, 1.0f, FMath::Clamp(V, 0.0f, 1.0f));
		return FMath::Clamp(AlongArcFade * WidthFade, 0.0f, 1.0f);
	}

	FVertexInstanceID AddSwingArcVertex(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		const FVector3f& Position,
		const FVector2f& UV,
		const FLinearColor& Color)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();
		TVertexInstanceAttributesRef<FVector4f> VertexInstanceColors = Attributes.GetVertexInstanceColors();

		const FVertexID VertexId = MeshDescription.CreateVertex();
		VertexPositions[VertexId] = Position;

		const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
		VertexInstanceNormals[VertexInstanceId] = FVector3f(0.0f, 0.0f, 1.0f);
		VertexInstanceUVs.Set(VertexInstanceId, 0, UV);
		VertexInstanceColors[VertexInstanceId] = FVector4f(Color.R, Color.G, Color.B, Color.A);
		return VertexInstanceId;
	}

	void AddSwingArcQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FVector3f& InnerA,
		const FVector3f& OuterA,
		const FVector3f& OuterB,
		const FVector3f& InnerB,
		float U0,
		float U1)
	{
		const FLinearColor InnerColorA(0.0f, 0.95f, 1.0f, ComputeSwingArcVertexAlpha(U0, 0.0f));
		const FLinearColor OuterColorA(0.0f, 0.95f, 1.0f, ComputeSwingArcVertexAlpha(U0, 1.0f));
		const FLinearColor OuterColorB(0.0f, 0.95f, 1.0f, ComputeSwingArcVertexAlpha(U1, 1.0f));
		const FLinearColor InnerColorB(0.0f, 0.95f, 1.0f, ComputeSwingArcVertexAlpha(U1, 0.0f));

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(4);
		VertexInstances.Add(AddSwingArcVertex(MeshDescription, Attributes, InnerA, FVector2f(U0, 0.0f), InnerColorA));
		VertexInstances.Add(AddSwingArcVertex(MeshDescription, Attributes, OuterA, FVector2f(U0, 1.0f), OuterColorA));
		VertexInstances.Add(AddSwingArcVertex(MeshDescription, Attributes, OuterB, FVector2f(U1, 1.0f), OuterColorB));
		VertexInstances.Add(AddSwingArcVertex(MeshDescription, Attributes, InnerB, FVector2f(U1, 0.0f), InnerColorB));
		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void BuildLumberjackMeleeSwingArcMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		constexpr int32 SegmentCount = 18;
		constexpr float StartDegrees = -66.0f;
		constexpr float EndDegrees = 66.0f;
		constexpr float InnerRadius = 48.0f;
		constexpr float OuterRadius = 118.0f;
		constexpr float Height = 64.0f;
		const FVector2f ArcCenter(18.0f, 0.0f);

		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = FMath::DegreesToRadians(FMath::Lerp(StartDegrees, EndDegrees, U0));
			const float Angle1 = FMath::DegreesToRadians(FMath::Lerp(StartDegrees, EndDegrees, U1));
			const FVector2f Direction0(FMath::Cos(Angle0), FMath::Sin(Angle0));
			const FVector2f Direction1(FMath::Cos(Angle1), FMath::Sin(Angle1));

			const FVector2f Inner0 = ArcCenter + Direction0 * InnerRadius;
			const FVector2f Outer0 = ArcCenter + Direction0 * OuterRadius;
			const FVector2f Outer1 = ArcCenter + Direction1 * OuterRadius;
			const FVector2f Inner1 = ArcCenter + Direction1 * InnerRadius;

			AddSwingArcQuad(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				FVector3f(Inner0.X, Inner0.Y, Height),
				FVector3f(Outer0.X, Outer0.Y, Height),
				FVector3f(Outer1.X, Outer1.Y, Height),
				FVector3f(Inner1.X, Inner1.Y, Height),
				U0,
				U1);
		}
	}

	UMaterial* EnsureLumberjackMeleeSwingArcMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, LumberjackMeleeSwingArcMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				LumberjackMeleeSwingArcMaterialAssetName,
				EffectsAssetPath,
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
		Material->MaterialDomain = MD_PostProcess;
		Material->BlendableLocation = BL_SceneColorAfterTonemapping;
		Material->BlendMode = BLEND_Opaque;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = false;
		Material->bUsedWithNiagaraMeshParticles = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -520;
		VertexColorExpression->MaterialExpressionEditorY = -20;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);

		UMaterialExpressionScalarParameter* IntensityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		IntensityParameter->Material = Material;
		IntensityParameter->ParameterName = TEXT("Intensity");
		IntensityParameter->DefaultValue = 5.2f;
		IntensityParameter->MaterialExpressionEditorX = -520;
		IntensityParameter->MaterialExpressionEditorY = 180;
		Material->GetExpressionCollection().AddExpression(IntensityParameter);

		UMaterialExpressionMultiply* EmissiveMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveMultiply->Material = Material;
		EmissiveMultiply->A.Connect(0, VertexColorExpression);
		EmissiveMultiply->B.Connect(0, IntensityParameter);
		EmissiveMultiply->MaterialExpressionEditorX = -220;
		EmissiveMultiply->MaterialExpressionEditorY = 60;
		Material->GetExpressionCollection().AddExpression(EmissiveMultiply);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->Material = Material;
		OpacityParameter->ParameterName = TEXT("Opacity");
		OpacityParameter->DefaultValue = 1.0f;
		OpacityParameter->MaterialExpressionEditorX = -520;
		OpacityParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		UMaterialExpressionScalarParameter* DissolveParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		DissolveParameter->Material = Material;
		DissolveParameter->ParameterName = TEXT("Dissolve");
		DissolveParameter->DefaultValue = 0.0f;
		DissolveParameter->MaterialExpressionEditorX = -520;
		DissolveParameter->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(DissolveParameter);

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -820;
		TextureCoordinateExpression->MaterialExpressionEditorY = 600;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionComponentMask* UvUMask = NewObject<UMaterialExpressionComponentMask>(Material);
		UvUMask->Material = Material;
		UvUMask->Input.Connect(0, TextureCoordinateExpression);
		UvUMask->R = 1;
		UvUMask->G = 0;
		UvUMask->B = 0;
		UvUMask->A = 0;
		UvUMask->MaterialExpressionEditorX = -640;
		UvUMask->MaterialExpressionEditorY = 600;
		Material->GetExpressionCollection().AddExpression(UvUMask);

		UMaterialExpressionSubtract* UvDissolveSubtract = NewObject<UMaterialExpressionSubtract>(Material);
		UvDissolveSubtract->Material = Material;
		UvDissolveSubtract->A.Connect(0, UvUMask);
		UvDissolveSubtract->B.Connect(0, DissolveParameter);
		UvDissolveSubtract->MaterialExpressionEditorX = -340;
		UvDissolveSubtract->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(UvDissolveSubtract);

		UMaterialExpressionMultiply* UvWipeScale = NewObject<UMaterialExpressionMultiply>(Material);
		UvWipeScale->Material = Material;
		UvWipeScale->A.Connect(0, UvDissolveSubtract);
		UvWipeScale->ConstB = 6.0f;
		UvWipeScale->MaterialExpressionEditorX = -140;
		UvWipeScale->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(UvWipeScale);

		UMaterialExpressionSaturate* UvWipeMask = NewObject<UMaterialExpressionSaturate>(Material);
		UvWipeMask->Material = Material;
		UvWipeMask->Input.Connect(0, UvWipeScale);
		UvWipeMask->MaterialExpressionEditorX = 60;
		UvWipeMask->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(UvWipeMask);

		UMaterialExpressionMultiply* VertexOpacityMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		VertexOpacityMultiply->Material = Material;
		VertexOpacityMultiply->A.Connect(4, VertexColorExpression);
		VertexOpacityMultiply->B.Connect(0, OpacityParameter);
		VertexOpacityMultiply->MaterialExpressionEditorX = -220;
		VertexOpacityMultiply->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(VertexOpacityMultiply);

		UMaterialExpressionMultiply* FinalOpacityMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		FinalOpacityMultiply->Material = Material;
		FinalOpacityMultiply->A.Connect(0, VertexOpacityMultiply);
		FinalOpacityMultiply->B.Connect(0, UvWipeMask);
		FinalOpacityMultiply->MaterialExpressionEditorX = 240;
		FinalOpacityMultiply->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(FinalOpacityMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, VertexColorExpression);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveMultiply);
		MaterialEditorOnly->Opacity.Connect(0, FinalOpacityMultiply);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UStaticMesh* EnsureLumberjackMeleeSwingArcMeshAsset(UMaterialInterface* SwingArcMaterial)
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, LumberjackMeleeSwingArcMeshAssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *EffectsAssetPath, *LumberjackMeleeSwingArcMeshAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*LumberjackMeleeSwingArcMeshAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		StaticMesh->Modify();

		FMeshDescription MeshDescription;
		BuildLumberjackMeleeSwingArcMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(SwingArcMaterial, FName(TEXT("SwingArc"))));

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool EnsureLumberjackMeleeSwingArcAssets()
	{
		UMaterial* SwingArcMaterial = EnsureLumberjackMeleeSwingArcMaterial();
		if (!SwingArcMaterial)
		{
			return false;
		}

		return EnsureLumberjackMeleeSwingArcMeshAsset(SwingArcMaterial) != nullptr;
	}

	struct FBaseballBatRing
	{
		float X = 0.0f;
		float Radius = 0.0f;
	};

	FVertexInstanceID AddBaseballBatVertex(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		const FVector3f& Position,
		const FVector3f& Normal,
		const FVector2f& UV)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		const FVertexID VertexId = MeshDescription.CreateVertex();
		VertexPositions[VertexId] = Position;

		const FVector3f SafeNormal = Normal.IsNearlyZero() ? FVector3f(0.0f, 0.0f, 1.0f) : Normal.GetSafeNormal();
		const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
		VertexInstanceNormals[VertexInstanceId] = SafeNormal;
		VertexInstanceUVs.Set(VertexInstanceId, 0, UV);
		return VertexInstanceId;
	}

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
		float V1)
	{
		const FVector3f Left0(LeftRing.X, FMath::Cos(Angle0) * LeftRing.Radius, FMath::Sin(Angle0) * LeftRing.Radius);
		const FVector3f Left1(LeftRing.X, FMath::Cos(Angle1) * LeftRing.Radius, FMath::Sin(Angle1) * LeftRing.Radius);
		const FVector3f Right0(RightRing.X, FMath::Cos(Angle0) * RightRing.Radius, FMath::Sin(Angle0) * RightRing.Radius);
		const FVector3f Right1(RightRing.X, FMath::Cos(Angle1) * RightRing.Radius, FMath::Sin(Angle1) * RightRing.Radius);
		const FVector3f Normal0(0.0f, FMath::Cos(Angle0), FMath::Sin(Angle0));
		const FVector3f Normal1(0.0f, FMath::Cos(Angle1), FMath::Sin(Angle1));

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(4);
		VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Left0, Normal0, FVector2f(U0, V0)));
		VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Left1, Normal1, FVector2f(U0, V1)));
		VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Right1, Normal1, FVector2f(U1, V1)));
		VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Right0, Normal0, FVector2f(U1, V0)));
		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void AddBaseballBatCap(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		const FBaseballBatRing& Ring,
		bool bRightCap)
	{
		constexpr int32 SegmentCount = 24;
		const FVector3f CapNormal = bRightCap ? FVector3f(1.0f, 0.0f, 0.0f) : FVector3f(-1.0f, 0.0f, 0.0f);
		const FVector3f Center(Ring.X, 0.0f, 0.0f);

		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float V0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float V1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = V0 * 2.0f * UE_PI;
			const float Angle1 = V1 * 2.0f * UE_PI;
			const FVector3f Edge0(Ring.X, FMath::Cos(Angle0) * Ring.Radius, FMath::Sin(Angle0) * Ring.Radius);
			const FVector3f Edge1(Ring.X, FMath::Cos(Angle1) * Ring.Radius, FMath::Sin(Angle1) * Ring.Radius);
			const FVector2f CenterUV(0.5f, 0.5f);
			const FVector2f EdgeUV0(0.5f + FMath::Cos(Angle0) * 0.5f, 0.5f + FMath::Sin(Angle0) * 0.5f);
			const FVector2f EdgeUV1(0.5f + FMath::Cos(Angle1) * 0.5f, 0.5f + FMath::Sin(Angle1) * 0.5f);

			TArray<FVertexInstanceID> VertexInstances;
			VertexInstances.Reserve(3);
			VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Center, CapNormal, CenterUV));
			if (bRightCap)
			{
				VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Edge0, CapNormal, EdgeUV0));
				VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Edge1, CapNormal, EdgeUV1));
			}
			else
			{
				VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Edge1, CapNormal, EdgeUV1));
				VertexInstances.Add(AddBaseballBatVertex(MeshDescription, Attributes, Edge0, CapNormal, EdgeUV0));
			}
			MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
		}
	}

	void BuildBaseballBatMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Wood"));

		const TArray<FBaseballBatRing> Rings = {
			{ -62.0f, 5.2f },
			{ -57.0f, 8.0f },
			{ -50.0f, 5.4f },
			{ -38.0f, 3.1f },
			{ 20.0f, 3.6f },
			{ 35.0f, 5.4f },
			{ 50.0f, 7.4f },
			{ 89.0f, 8.8f },
			{ 99.0f, 8.1f }
		};
		const float MinX = Rings[0].X;
		const float Length = FMath::Max(1.0f, Rings.Last().X - MinX);

		constexpr int32 SegmentCount = 24;
		for (int32 RingIndex = 0; RingIndex < Rings.Num() - 1; ++RingIndex)
		{
			const FBaseballBatRing& LeftRing = Rings[RingIndex];
			const FBaseballBatRing& RightRing = Rings[RingIndex + 1];
			const float U0 = (LeftRing.X - MinX) / Length;
			const float U1 = (RightRing.X - MinX) / Length;

			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				const float V0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
				const float V1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
				const float Angle0 = V0 * 2.0f * UE_PI;
				const float Angle1 = V1 * 2.0f * UE_PI;
				AddBaseballBatSideQuad(
					MeshDescription,
					Attributes,
					PolygonGroupId,
					LeftRing,
					RightRing,
					Angle0,
					Angle1,
					U0,
					U1,
					V0,
					V1);
			}
		}

		AddBaseballBatCap(MeshDescription, Attributes, PolygonGroupId, Rings[0], false);
		AddBaseballBatCap(MeshDescription, Attributes, PolygonGroupId, Rings.Last(), true);
	}

	UTexture2D* EnsureBaseballBatWoodTexture()
	{
		const FString ObjectPath = GetAssetObjectPath(WeaponAssetPath, BaseballBatWoodTextureAssetName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!Texture)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *WeaponAssetPath, *BaseballBatWoodTextureAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			Texture = NewObject<UTexture2D>(
				Package,
				*BaseballBatWoodTextureAssetName,
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
				const float U = static_cast<float>(X) / static_cast<float>(TextureSize - 1);
				const float V = static_cast<float>(Y) / static_cast<float>(TextureSize - 1);
				const float GrainNoise = FMath::PerlinNoise2D(FVector2D(U * 7.0f, V * 5.5f)) * 0.5f + 0.5f;
				const float FineNoise = FMath::PerlinNoise2D(FVector2D(U * 44.0f + 31.0f, V * 12.0f - 17.0f)) * 0.5f + 0.5f;
				const float GrainWave = FMath::Sin((V * 17.0f + GrainNoise * 1.7f + U * 0.8f) * 2.0f * UE_PI) * 0.5f + 0.5f;
				const float RingLine = FMath::Clamp((GrainWave - 0.72f) / 0.28f, 0.0f, 1.0f);
				const float Shade = FMath::Clamp(0.54f + GrainWave * 0.25f + FineNoise * 0.15f, 0.0f, 1.0f);

				float Red = FMath::Lerp(132.0f, 236.0f, Shade);
				float Green = FMath::Lerp(76.0f, 174.0f, Shade);
				float Blue = FMath::Lerp(34.0f, 82.0f, Shade);
				Red = FMath::Lerp(Red, 92.0f, RingLine * 0.28f);
				Green = FMath::Lerp(Green, 48.0f, RingLine * 0.28f);
				Blue = FMath::Lerp(Blue, 20.0f, RingLine * 0.28f);

				const int32 PixelIndex = (Y * TextureSize + X) * 4;
				Pixels[PixelIndex + 0] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Blue), 0, 255));
				Pixels[PixelIndex + 1] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Green), 0, 255));
				Pixels[PixelIndex + 2] = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Red), 0, 255));
				Pixels[PixelIndex + 3] = 255;
			}
		}

		Texture->Modify();
		Texture->Source.Init(TextureSize, TextureSize, 1, 1, TSF_BGRA8, Pixels.GetData());
		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
		Texture->LODGroup = TEXTUREGROUP_World;
		Texture->PostEditChange();
		Texture->MarkPackageDirty();

		if (!SaveAsset(Texture))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Texture;
	}

	UMaterial* EnsureBaseballBatWoodMaterial(UTexture2D* WoodTexture)
	{
		if (!WoodTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(WeaponAssetPath, BaseballBatMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				BaseballBatMaterialAssetName,
				WeaponAssetPath,
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
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->UTiling = 1.6f;
		TextureCoordinateExpression->VTiling = 1.0f;
		TextureCoordinateExpression->MaterialExpressionEditorX = -720;
		TextureCoordinateExpression->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionTextureSampleParameter2D* WoodSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		WoodSample->Material = Material;
		WoodSample->ParameterName = TEXT("WoodTexture");
		WoodSample->Texture = WoodTexture;
		WoodSample->SamplerType = SAMPLERTYPE_Color;
		WoodSample->Coordinates.Connect(0, TextureCoordinateExpression);
		WoodSample->MaterialExpressionEditorX = -460;
		WoodSample->MaterialExpressionEditorY = 0;
		Material->GetExpressionCollection().AddExpression(WoodSample);

		UMaterialExpressionScalarParameter* RoughnessParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		RoughnessParameter->Material = Material;
		RoughnessParameter->ParameterName = TEXT("Roughness");
		RoughnessParameter->DefaultValue = 0.68f;
		RoughnessParameter->MaterialExpressionEditorX = -460;
		RoughnessParameter->MaterialExpressionEditorY = 220;
		Material->GetExpressionCollection().AddExpression(RoughnessParameter);

		UMaterialExpressionScalarParameter* SpecularParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		SpecularParameter->Material = Material;
		SpecularParameter->ParameterName = TEXT("Specular");
		SpecularParameter->DefaultValue = 0.24f;
		SpecularParameter->MaterialExpressionEditorX = -460;
		SpecularParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(SpecularParameter);

		MaterialEditorOnly->BaseColor.Connect(0, WoodSample);
		MaterialEditorOnly->Roughness.Connect(0, RoughnessParameter);
		MaterialEditorOnly->Specular.Connect(0, SpecularParameter);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UStaticMesh* EnsureBaseballBatStaticMeshAsset(UMaterialInterface* WoodMaterial)
	{
		const FString ObjectPath = GetAssetObjectPath(WeaponAssetPath, BaseballBatMeshAssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *WeaponAssetPath, *BaseballBatMeshAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*BaseballBatMeshAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		StaticMesh->Modify();

		FMeshDescription MeshDescription;
		BuildBaseballBatMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(WoodMaterial, FName(TEXT("Wood"))));

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool EnsureBaseballBatAssets()
	{
		UTexture2D* WoodTexture = EnsureBaseballBatWoodTexture();
		UMaterial* WoodMaterial = EnsureBaseballBatWoodMaterial(WoodTexture);
		if (!WoodTexture || !WoodMaterial)
		{
			return false;
		}

		return EnsureBaseballBatStaticMeshAsset(WoodMaterial) != nullptr;
	}

	UMaterial* EnsureLedExpressionMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, LedExpressionMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				LedExpressionMaterialAssetName,
				EffectsAssetPath,
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
		Material->BlendMode = BLEND_Additive;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -520;
		VertexColorExpression->MaterialExpressionEditorY = -20;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);

		UMaterialExpressionScalarParameter* IntensityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		IntensityParameter->Material = Material;
		IntensityParameter->ParameterName = TEXT("Intensity");
		IntensityParameter->DefaultValue = 3.0f;
		IntensityParameter->MaterialExpressionEditorX = -520;
		IntensityParameter->MaterialExpressionEditorY = 180;
		Material->GetExpressionCollection().AddExpression(IntensityParameter);

		UMaterialExpressionMultiply* EmissiveMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveMultiply->Material = Material;
		EmissiveMultiply->A.Connect(0, VertexColorExpression);
		EmissiveMultiply->B.Connect(0, IntensityParameter);
		EmissiveMultiply->MaterialExpressionEditorX = -220;
		EmissiveMultiply->MaterialExpressionEditorY = 60;
		Material->GetExpressionCollection().AddExpression(EmissiveMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, VertexColorExpression);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveMultiply);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
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

	FString GetQuickSlotActionName(int32 SlotNumber)
	{
		return FString::Printf(TEXT("%s%d"), *QuickSlotActionNamePrefix, SlotNumber);
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

	bool EnsureQuickSlotInputAssets()
	{
		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!MappingContext)
		{
			return false;
		}

		static const FKey QuickSlotKeys[8] = {
			EKeys::One,
			EKeys::Two,
			EKeys::Three,
			EKeys::Four,
			EKeys::Five,
			EKeys::Six,
			EKeys::Seven,
			EKeys::Eight
		};

		bool bAllActionsCreated = true;
		for (int32 SlotIndex = 0; SlotIndex < UE_ARRAY_COUNT(QuickSlotKeys); ++SlotIndex)
		{
			const int32 SlotNumber = SlotIndex + 1;
			UInputAction* QuickSlotAction = EnsureInputAction(
				GetQuickSlotActionName(SlotNumber),
				EInputActionValueType::Boolean,
				EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

			bAllActionsCreated = bAllActionsCreated && QuickSlotAction;
			if (QuickSlotAction && !HasInputMapping(MappingContext, QuickSlotAction, QuickSlotKeys[SlotIndex]))
			{
				MappingContext->MapKey(QuickSlotAction, QuickSlotKeys[SlotIndex]);
			}
		}

		UInputAction* MeleeQuickSlotAction = EnsureInputAction(
			MeleeQuickSlotActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		bAllActionsCreated = bAllActionsCreated && MeleeQuickSlotAction;
		if (MeleeQuickSlotAction && !HasInputMapping(MappingContext, MeleeQuickSlotAction, EKeys::V))
		{
			MappingContext->MapKey(MeleeQuickSlotAction, EKeys::V);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, and quick slot input."));
		MappingContext->MarkPackageDirty();
		return bAllActionsCreated && SaveAsset(MappingContext);
	}

	bool EnsureDropInputAssets()
	{
		UInputAction* DropAction = EnsureInputAction(
			DropActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!DropAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, DropAction, EKeys::X))
		{
			MappingContext->MapKey(DropAction, EKeys::X);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, and item drop input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureAmmoReloadInputAssets()
	{
		UInputAction* ReloadAction = EnsureInputAction(
			ReloadActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputAction* AmmoSelectAction = EnsureInputAction(
			AmmoSelectActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);
		UInputAction* AmmoFocusAction = EnsureInputAction(
			AmmoFocusActionName,
			EInputActionValueType::Axis1D,
			EInputActionAccumulationBehavior::Cumulative);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!ReloadAction || !AmmoSelectAction || !AmmoFocusAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, ReloadAction, EKeys::R))
		{
			MappingContext->MapKey(ReloadAction, EKeys::R);
		}

		if (!HasInputMapping(MappingContext, AmmoSelectAction, EKeys::T))
		{
			MappingContext->MapKey(AmmoSelectAction, EKeys::T);
		}

		if (!HasInputMapping(MappingContext, AmmoFocusAction, EKeys::MouseWheelAxis))
		{
			MappingContext->MapKey(AmmoFocusAction, EKeys::MouseWheelAxis);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, and reload input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureCameraModeInputAssets()
	{
		UInputAction* CameraModeAction = EnsureInputAction(
			CameraModeActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!CameraModeAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, CameraModeAction, EKeys::Y))
		{
			MappingContext->MapKey(CameraModeAction, EKeys::Y);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, reload, and camera mode input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureSprintInputAssets()
	{
		UInputAction* SprintAction = EnsureInputAction(
			SprintActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!SprintAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, SprintAction, EKeys::LeftShift))
		{
			MappingContext->MapKey(SprintAction, EKeys::LeftShift);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, reload, camera mode, and sprint input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureRollInputAssets()
	{
		UInputAction* RollAction = EnsureInputAction(
			RollActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!RollAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, RollAction, EKeys::SpaceBar))
		{
			MappingContext->MapKey(RollAction, EKeys::SpaceBar);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, reload, camera mode, sprint, and roll input."));
		MappingContext->MarkPackageDirty();
		return SaveAsset(MappingContext);
	}

	bool EnsureMapInputAssets()
	{
		UInputAction* MapAction = EnsureInputAction(
			MapActionName,
			EInputActionValueType::Boolean,
			EInputActionAccumulationBehavior::TakeHighestAbsoluteValue);

		UInputMappingContext* MappingContext = LoadObject<UInputMappingContext>(
			nullptr,
			*GetAssetObjectPath(InputAssetPath, MappingContextName));

		if (!MapAction || !MappingContext)
		{
			return false;
		}

		if (!HasInputMapping(MappingContext, MapAction, EKeys::M))
		{
			MappingContext->MapKey(MapAction, EKeys::M);
		}

		MappingContext->ContextDescription = FText::FromString(TEXT("TunaSweeper player movement, combat, interaction, inventory, quick slot, ammo, reload, camera mode, sprint, roll, and map input."));
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

	bool SetProjectStartupMapsToIntro()
	{
		const FString IntroMapObjectPath = FString::Printf(TEXT("%s.IntroMap"), *IntroMapPackagePath);

		if (UGameMapsSettings* GameMapsSettings = GetMutableDefault<UGameMapsSettings>())
		{
			UGameMapsSettings::SetGameDefaultMap(IntroMapObjectPath);
			GameMapsSettings->EditorStartupMap = FSoftObjectPath(IntroMapObjectPath);
			GameMapsSettings->SaveConfig();
		}

		const FString DefaultEngineIni = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultEngine.ini"));
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameDefaultMap"),
			*IntroMapObjectPath,
			DefaultEngineIni);
		GConfig->SetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("EditorStartupMap"),
			*IntroMapObjectPath,
			DefaultEngineIni);
		GConfig->Flush(false, DefaultEngineIni);

		FString SavedGameDefaultMap;
		FString SavedEditorStartupMap;
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("GameDefaultMap"),
			SavedGameDefaultMap,
			DefaultEngineIni);
		GConfig->GetString(
			TEXT("/Script/EngineSettings.GameMapsSettings"),
			TEXT("EditorStartupMap"),
			SavedEditorStartupMap,
			DefaultEngineIni);

		return SavedGameDefaultMap == IntroMapObjectPath && SavedEditorStartupMap == IntroMapObjectPath;
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

	bool EnsureProjectileHitEffectAssets()
	{
		UBlueprint* HitEffectBlueprint = EnsureBlueprint(
			EffectsAssetPath,
			ProjectileHitRedBurstActorAssetName,
			ATunaSweeperProjectileHitBurstActor::StaticClass());
		UTunaSweeperProjectileHitEffectDataAsset* HitEffectDataAsset =
			EnsureDataAsset<UTunaSweeperProjectileHitEffectDataAsset>(
				EffectsAssetPath,
				ProjectileHitEffectDataAssetName);
		if (!HitEffectBlueprint || !HitEffectDataAsset)
		{
			return false;
		}

		HitEffectDataAsset->Modify();
		HitEffectDataAsset->HitEffects.Reset();

		FTunaSweeperProjectileHitEffectDefinition RedBurstDefinition;
		RedBurstDefinition.EffectId = FName(TEXT("hit.red_burst"));
		RedBurstDefinition.EffectActorClass =
			TSoftClassPtr<ATunaSweeperProjectileHitBurstActor>(
				FSoftObjectPath(GetAssetClassPath(EffectsAssetPath, ProjectileHitRedBurstActorAssetName)));
		RedBurstDefinition.BurstColor = FLinearColor(1.0f, 0.03f, 0.0f, 1.0f);
		RedBurstDefinition.SpawnScale = FVector::OneVector;
		RedBurstDefinition.SurfaceOffsetCm = 1.0f;
		HitEffectDataAsset->HitEffects.Add(RedBurstDefinition);
		HitEffectDataAsset->MarkPackageDirty();
		if (!SaveAsset(HitEffectDataAsset))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *GetAssetObjectPath(EffectsAssetPath, ProjectileHitEffectDataAssetName));
			return false;
		}

		UBlueprint* GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint && !EnsureGameInstanceBlueprint())
		{
			return false;
		}

		GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint)
		{
			return false;
		}

		if (!GameInstanceBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);
		}

		UTunaSweeperGameInstance* GameInstanceDefaults = GameInstanceBlueprint->GeneratedClass
			? Cast<UTunaSweeperGameInstance>(GameInstanceBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!GameInstanceDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure projectile hit effect mapping on %s."), *GetGameInstanceObjectPath());
			return false;
		}

		GameInstanceBlueprint->Modify();
		GameInstanceDefaults->Modify();
		GameInstanceDefaults->ProjectileHitEffectDataAsset =
			TSoftObjectPtr<UTunaSweeperProjectileHitEffectDataAsset>(
				FSoftObjectPath(GetAssetObjectPath(EffectsAssetPath, ProjectileHitEffectDataAssetName)));
		GameInstanceBlueprint->MarkPackageDirty();
		FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);

		return SaveAsset(GameInstanceBlueprint);
	}

	bool EnsureWeaponSpreadRecoilAssets()
	{
		UTunaSweeperWeaponSpreadRecoilDataAsset* RecoilDataAsset =
			EnsureDataAsset<UTunaSweeperWeaponSpreadRecoilDataAsset>(
				WeaponAssetPath,
				WeaponSpreadRecoilDataAssetName);
		if (!RecoilDataAsset)
		{
			return false;
		}

		RecoilDataAsset->Modify();
		RecoilDataAsset->WeaponTypeDefinitions.Reset();
		auto AddRecoilDefinition = [RecoilDataAsset](
			const TCHAR* WeaponTypeTag,
			float IncreasePerShot,
			float MinimumSpreadHalfAngleDegrees,
			float MaximumSpreadHalfAngleDegrees,
			float DecreasePerSecond)
		{
			FTunaSweeperWeaponSpreadRecoilDefinition Definition;
			Definition.WeaponTypeTag = FName(WeaponTypeTag);
			Definition.IncreasePerShot = IncreasePerShot;
			Definition.MinimumSpreadHalfAngleDegrees = MinimumSpreadHalfAngleDegrees;
			Definition.MaximumSpreadHalfAngleDegrees = MaximumSpreadHalfAngleDegrees;
			Definition.DecreasePerSecond = DecreasePerSecond;
			RecoilDataAsset->WeaponTypeDefinitions.Add(Definition);
		};

		AddRecoilDefinition(TEXT("weapon.type.pistol"), 1.2f, 1.4f, 7.0f, 5.0f);
		AddRecoilDefinition(TEXT("weapon.type.rifle"), 0.8f, 1.0f, 6.0f, 6.5f);
		AddRecoilDefinition(TEXT("weapon.type.shotgun"), 2.0f, 4.5f, 12.0f, 4.5f);

		RecoilDataAsset->MarkPackageDirty();
		if (!SaveAsset(RecoilDataAsset))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *GetAssetObjectPath(WeaponAssetPath, WeaponSpreadRecoilDataAssetName));
			return false;
		}

		UBlueprint* GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint && !EnsureGameInstanceBlueprint())
		{
			return false;
		}

		GameInstanceBlueprint = LoadObject<UBlueprint>(nullptr, *GetGameInstanceObjectPath());
		if (!GameInstanceBlueprint)
		{
			return false;
		}

		if (!GameInstanceBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);
		}

		UTunaSweeperGameInstance* GameInstanceDefaults = GameInstanceBlueprint->GeneratedClass
			? Cast<UTunaSweeperGameInstance>(GameInstanceBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!GameInstanceDefaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure weapon spread recoil mapping on %s."), *GetGameInstanceObjectPath());
			return false;
		}

		GameInstanceBlueprint->Modify();
		GameInstanceDefaults->Modify();
		GameInstanceDefaults->WeaponSpreadRecoilDataAsset =
			TSoftObjectPtr<UTunaSweeperWeaponSpreadRecoilDataAsset>(
				FSoftObjectPath(GetAssetObjectPath(WeaponAssetPath, WeaponSpreadRecoilDataAssetName)));
		GameInstanceBlueprint->MarkPackageDirty();
		FKismetEditorUtilities::CompileBlueprint(GameInstanceBlueprint);

		return SaveAsset(GameInstanceBlueprint);
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

	bool EnsureCanBotBlueprint()
	{
		return EnsureBlueprint(CanBotAssetPath, CanBotAssetName, ATunaSweeperLedRobotCharacterActor::StaticClass()) != nullptr;
	}

	bool EnsureEnemyVisualMaterialAssets()
	{
		UMaterial* RedMaterial = EnsureSolidColorMaterial(
			EnemyAssetPath,
			EnemyBodyMaterialAssetName,
			FLinearColor(0.9f, 0.02f, 0.015f, 1.0f),
			0.7f);
		UMaterial* GreenMaterial = EnsureSolidColorMaterial(
			EnemyAssetPath,
			EnemyGreenMaterialAssetName,
			FLinearColor(0.05f, 0.72f, 0.16f, 1.0f),
			0.7f);
		UMaterial* BlueMaterial = EnsureSolidColorMaterial(
			EnemyAssetPath,
			EnemyBlueMaterialAssetName,
			FLinearColor(0.04f, 0.24f, 0.95f, 1.0f),
			0.7f);
		UMaterial* SightlineMaterial = EnsureSolidColorMaterial(
			EnemyAssetPath,
			EnemySightlineMaterialAssetName,
			FLinearColor(0.35f, 0.0f, 0.0f, 1.0f),
			0.75f);
		UMaterial* CardboardMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			CardboardContainerMaterialAssetName,
			FLinearColor(0.64f, 0.42f, 0.22f, 1.0f),
			0.85f);
		UMaterial* WoodMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			WoodContainerMaterialAssetName,
			FLinearColor(0.45f, 0.24f, 0.11f, 1.0f),
			0.8f);
		UMaterial* MetalMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			MetalContainerMaterialAssetName,
			FLinearColor(0.55f, 0.57f, 0.6f, 1.0f),
			0.35f,
			0.65f);
		UMaterial* SupplyMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			SupplyContainerMaterialAssetName,
			FLinearColor(0.22f, 0.32f, 0.24f, 1.0f),
			0.85f);

		return RedMaterial && GreenMaterial && BlueMaterial && SightlineMaterial &&
			CardboardMaterial && WoodMaterial && MetalMaterial && SupplyMaterial;
	}

	bool EnsureRollingBomberBodyGrayMaterial()
	{
		return EnsureSolidColorMaterial(
			EnemyAssetPath,
			RollingBomberBodyGrayMaterialAssetName,
			FLinearColor(0.5f, 0.5f, 0.5f, 1.0f),
			0.82f,
			0.0f) != nullptr;
	}

	bool EnsureRollingBomberLegMetalMaterial()
	{
		return EnsureSolidColorMaterial(
			EnemyAssetPath,
			RollingBomberLegMetalMaterialAssetName,
			FLinearColor(0.48f, 0.52f, 0.54f, 1.0f),
			0.32f,
			1.0f) != nullptr;
	}

	FVector3f MakeBarrelVertex(float AngleRadians, float Radius, float Height)
	{
		return FVector3f(
			FMath::Cos(AngleRadians) * Radius,
			FMath::Sin(AngleRadians) * Radius,
			Height);
	}

	FVector3f MakeBarrelRadialNormal(const FVector3f& Position)
	{
		const FVector3f Radial(Position.X, Position.Y, 0.0f);
		return Radial.IsNearlyZero() ? FVector3f(1.0f, 0.0f, 0.0f) : Radial.GetSafeNormal();
	}

	FVector3f MakeBarrelRadialTangent(const FVector3f& Position)
	{
		const FVector3f Tangent(-Position.Y, Position.X, 0.0f);
		return Tangent.IsNearlyZero() ? FVector3f(0.0f, 1.0f, 0.0f) : Tangent.GetSafeNormal();
	}

	FVector3f MakeBarrelSafeTangent(const FVector3f& Normal, const FVector3f& PreferredTangent)
	{
		FVector3f Tangent = PreferredTangent - Normal * FVector3f::DotProduct(Normal, PreferredTangent);
		if (Tangent.IsNearlyZero())
		{
			const FVector3f FallbackTangent = FMath::Abs(Normal.Z) < 0.8f
				? FVector3f(0.0f, 0.0f, 1.0f)
				: FVector3f(1.0f, 0.0f, 0.0f);
			Tangent = FallbackTangent - Normal * FVector3f::DotProduct(Normal, FallbackTangent);
		}

		return Tangent.IsNearlyZero() ? FVector3f(1.0f, 0.0f, 0.0f) : Tangent.GetSafeNormal();
	}

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
		const FVector3f& SurfaceNormal)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		const FVector3f Positions[] = { A, C, B };
		const FVector2f UVs[] = { UvA, UvC, UvB };
		const FVector3f SurfaceTangent = MakeBarrelSafeTangent(SurfaceNormal, FVector3f(1.0f, 0.0f, 0.0f));

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = SurfaceNormal;
			VertexInstanceTangents[VertexInstanceId] = SurfaceTangent;
			VertexInstanceBinormalSigns[VertexInstanceId] = 1.0f;
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

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
		float V1)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceTangents = Attributes.GetVertexInstanceTangents();
		TVertexInstanceAttributesRef<float> VertexInstanceBinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		const FVector3f Positions[] = { A, D, C, B };
		const FVector3f Normals[] = {
			MakeBarrelRadialNormal(A),
			MakeBarrelRadialNormal(D),
			MakeBarrelRadialNormal(C),
			MakeBarrelRadialNormal(B)
		};
		const FVector3f Tangents[] = {
			MakeBarrelRadialTangent(A),
			MakeBarrelRadialTangent(D),
			MakeBarrelRadialTangent(C),
			MakeBarrelRadialTangent(B)
		};
		const FVector2f UVs[] = {
			FVector2f(U0, V0),
			FVector2f(U0, V1),
			FVector2f(U1, V1),
			FVector2f(U1, V0)
		};

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = Normals[Index];
			VertexInstanceTangents[VertexInstanceId] = MakeBarrelSafeTangent(Normals[Index], Tangents[Index]);
			VertexInstanceBinormalSigns[VertexInstanceId] = 1.0f;
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void BuildExplosiveBarrelIntactMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Barrel"));
		const FPolygonGroupID DetailPolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[DetailPolygonGroupId] = FName(TEXT("BarrelDetail"));

		constexpr int32 SegmentCount = 32;
		const float Heights[] = { 0.0f, 6.0f, 13.0f, 25.0f, 58.0f, 95.0f, 107.0f, 114.0f, 120.0f };
		const float Radii[] = { 31.0f, 35.0f, 35.0f, 32.5f, 34.0f, 32.5f, 35.0f, 35.0f, 31.0f };
		constexpr int32 RingCount = UE_ARRAY_COUNT(Heights);
		for (int32 RingIndex = 0; RingIndex < RingCount - 1; ++RingIndex)
		{
			const float V0 = Heights[RingIndex] / Heights[RingCount - 1];
			const float V1 = Heights[RingIndex + 1] / Heights[RingCount - 1];
			for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
			{
				const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
				const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
				const float Angle0 = U0 * 2.0f * UE_PI;
				const float Angle1 = U1 * 2.0f * UE_PI;
				AddBarrelQuad(
					MeshDescription,
					Attributes,
					PolygonGroupId,
					MakeBarrelVertex(Angle0, Radii[RingIndex], Heights[RingIndex]),
					MakeBarrelVertex(Angle1, Radii[RingIndex], Heights[RingIndex]),
					MakeBarrelVertex(Angle1, Radii[RingIndex + 1], Heights[RingIndex + 1]),
					MakeBarrelVertex(Angle0, Radii[RingIndex + 1], Heights[RingIndex + 1]),
					U0,
					U1,
					V0,
					V1);
			}
		}

		const FVector3f BottomCenter(0.0f, 0.0f, Heights[0]);
		const FVector3f TopCenter(0.0f, 0.0f, Heights[RingCount - 1]);
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = U0 * 2.0f * UE_PI;
			const float Angle1 = U1 * 2.0f * UE_PI;
			const FVector3f Bottom0 = MakeBarrelVertex(Angle0, Radii[0], Heights[0]);
			const FVector3f Bottom1 = MakeBarrelVertex(Angle1, Radii[0], Heights[0]);
			const FVector3f Top0 = MakeBarrelVertex(Angle0, Radii[RingCount - 1], Heights[RingCount - 1]);
			const FVector3f Top1 = MakeBarrelVertex(Angle1, Radii[RingCount - 1], Heights[RingCount - 1]);
			AddBarrelTriangle(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				BottomCenter,
				Bottom1,
				Bottom0,
				FVector2f(0.5f, 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle1) * 0.5f, 0.5f + FMath::Sin(Angle1) * 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle0) * 0.5f, 0.5f + FMath::Sin(Angle0) * 0.5f),
				FVector3f(0.0f, 0.0f, -1.0f));
			AddBarrelTriangle(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				TopCenter,
				Top0,
				Top1,
				FVector2f(0.5f, 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle0) * 0.5f, 0.5f + FMath::Sin(Angle0) * 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle1) * 0.5f, 0.5f + FMath::Sin(Angle1) * 0.5f),
				FVector3f(0.0f, 0.0f, 1.0f));
		}
	}

	void BuildExplosiveBarrelDestroyedMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("Barrel"));
		const FPolygonGroupID DetailPolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[DetailPolygonGroupId] = FName(TEXT("BarrelDetail"));

		constexpr int32 SegmentCount = 32;
		const float BaseRadius = 32.0f;
		const float RimRadius = 35.0f;
		const float BottomHeight = 0.0f;
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const int32 NextSegmentIndex = SegmentIndex + 1;
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(NextSegmentIndex) / static_cast<float>(SegmentCount);
			const float Angle0 = U0 * 2.0f * UE_PI;
			const float Angle1 = U1 * 2.0f * UE_PI;
			const float TopHeight0 = 20.0f + static_cast<float>((SegmentIndex * 7) % 9) * 1.7f;
			const float TopHeight1 = 20.0f + static_cast<float>((NextSegmentIndex * 7) % 9) * 1.7f;

			AddBarrelQuad(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				MakeBarrelVertex(Angle0, BaseRadius, BottomHeight),
				MakeBarrelVertex(Angle1, BaseRadius, BottomHeight),
				MakeBarrelVertex(Angle1, RimRadius, TopHeight1),
				MakeBarrelVertex(Angle0, RimRadius, TopHeight0),
				U0,
				U1,
				0.0f,
				1.0f);
		}

		const FVector3f BottomCenter(0.0f, 0.0f, BottomHeight);
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = U0 * 2.0f * UE_PI;
			const float Angle1 = U1 * 2.0f * UE_PI;
			AddBarrelTriangle(
				MeshDescription,
				Attributes,
				PolygonGroupId,
				BottomCenter,
				MakeBarrelVertex(Angle1, BaseRadius, BottomHeight),
				MakeBarrelVertex(Angle0, BaseRadius, BottomHeight),
				FVector2f(0.5f, 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle1) * 0.5f, 0.5f + FMath::Sin(Angle1) * 0.5f),
				FVector2f(0.5f + FMath::Cos(Angle0) * 0.5f, 0.5f + FMath::Sin(Angle0) * 0.5f),
				FVector3f(0.0f, 0.0f, -1.0f));
		}
	}

	UStaticMesh* EnsureExplosiveBarrelStaticMesh(
		const FString& AssetName,
		UMaterialInterface* BarrelMaterial,
		UMaterialInterface* DetailMaterial,
		TFunctionRef<void(FMeshDescription&)> BuildMeshDescription)
	{
		if (!BarrelMaterial)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, AssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *InteractionAssetPath, *AssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*AssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		StaticMesh->Modify();
		FMeshDescription MeshDescription;
		BuildMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(BarrelMaterial, FName(TEXT("Barrel"))));
		if (DetailMaterial)
		{
			StaticMesh->GetStaticMaterials().Add(FStaticMaterial(DetailMaterial, FName(TEXT("BarrelDetail"))));
		}

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bFastBuild = true;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = true;
		BuildParams.bUseHashAsGuid = false;
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams);
		StaticMesh->PostEditChange();
		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool ConfigureExplosiveBarrelBlueprint(UBlueprint* ExplosiveBarrelBlueprint)
	{
		if (!ExplosiveBarrelBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(ExplosiveBarrelBlueprint);

		ATunaSweeperExplosiveBarrelActor* Defaults = ExplosiveBarrelBlueprint->GeneratedClass
			? Cast<ATunaSweeperExplosiveBarrelActor>(ExplosiveBarrelBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(ExplosiveBarrelBlueprint));
			return false;
		}

		ExplosiveBarrelBlueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigureExplosiveBarrelDefaults(
			NAME_None,
			30.0f,
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(GetAssetObjectPath(InteractionAssetPath, ExplosiveBarrelIntactMeshAssetName))),
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(GetAssetObjectPath(InteractionAssetPath, ExplosiveBarrelDestroyedMeshAssetName))),
			TSoftObjectPtr<UNiagaraSystem>(),
			TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor>(
				FSoftObjectPath(TEXT("/Script/TunaSweeper.TunaSweeperLocalExplosionEffectActor"))),
			210.0f,
			0.72f);
		FBlueprintEditorUtils::MarkBlueprintAsModified(ExplosiveBarrelBlueprint);
		FKismetEditorUtilities::CompileBlueprint(ExplosiveBarrelBlueprint);
		ExplosiveBarrelBlueprint->MarkPackageDirty();
		return SaveAsset(ExplosiveBarrelBlueprint);
	}

	bool EnsureExplosiveBarrelAssets()
	{
		UMaterial* IntactMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			ExplosiveBarrelGrayMaterialAssetName,
			FLinearColor(0.42f, 0.44f, 0.43f, 1.0f),
			0.92f,
			0.0f,
			0.06f);
		UMaterial* DestroyedMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			ExplosiveBarrelCharredMaterialAssetName,
			FLinearColor(0.12f, 0.12f, 0.11f, 1.0f),
			0.86f,
			0.0f,
			0.05f);
		UMaterial* DetailMaterial = EnsureSolidColorMaterial(
			InteractionAssetPath,
			ExplosiveBarrelDetailMaterialAssetName,
			FLinearColor(0.22f, 0.23f, 0.22f, 1.0f),
			0.94f,
			0.0f,
			0.04f);
		UStaticMesh* IntactMesh = EnsureExplosiveBarrelStaticMesh(
			ExplosiveBarrelIntactMeshAssetName,
			IntactMaterial,
			DetailMaterial,
			[](FMeshDescription& MeshDescription)
			{
				BuildExplosiveBarrelIntactMeshDescription(MeshDescription);
			});
		UStaticMesh* DestroyedMesh = EnsureExplosiveBarrelStaticMesh(
			ExplosiveBarrelDestroyedMeshAssetName,
			DestroyedMaterial,
			DetailMaterial,
			[](FMeshDescription& MeshDescription)
			{
				BuildExplosiveBarrelDestroyedMeshDescription(MeshDescription);
			});
		UBlueprint* ExplosiveBarrelBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			ExplosiveBarrelAssetName,
			ATunaSweeperExplosiveBarrelActor::StaticClass());

		return IntactMaterial && DestroyedMaterial && DetailMaterial && IntactMesh && DestroyedMesh &&
			ConfigureExplosiveBarrelBlueprint(ExplosiveBarrelBlueprint);
	}

	bool EnsureSharedVoxelMeshAssets()
	{
		UMaterial* VoxelMaterial = EnsureVoxelVertexColorMaterial();
		if (!VoxelMaterial)
		{
			return false;
		}

		UStaticMesh* EnemyBodyMesh = EnsureVoxelStaticMeshAsset(
			EnemyAssetPath,
			EnemyVoxelBodyMeshAssetName,
			FName(TEXT("VoxelVertexColor")),
			[](FMeshDescription& MeshDescription)
			{
				BuildVoxelMeshDescription(
					MeshDescription,
					FName(TEXT("VoxelVertexColor")),
					FVector3f(100.0f, 100.0f, 100.0f),
					[](TArray<FEnemyVoxelBox>& OutBoxes)
					{
						AppendEnemyBodyVoxelBoxes(OutBoxes);
					});
			},
			VoxelMaterial);

		UStaticMesh* EnemyForwardMarkerMesh = EnsureVoxelStaticMeshAsset(
			EnemyAssetPath,
			EnemyVoxelForwardMarkerMeshAssetName,
			FName(TEXT("VoxelVertexColor")),
			[](FMeshDescription& MeshDescription)
			{
				BuildVoxelMeshDescription(
					MeshDescription,
					FName(TEXT("VoxelVertexColor")),
					FVector3f(70.0f, 28.0f, 18.0f),
					[](TArray<FEnemyVoxelBox>& OutBoxes)
					{
						AppendEnemyForwardMarkerVoxelBoxes(OutBoxes);
					});
			},
			VoxelMaterial);

		UStaticMesh* BrokenBridgeMesh = EnsureVoxelStaticMeshAsset(
			InteractionAssetPath,
			BrokenBridgeVoxelMeshAssetName,
			FName(TEXT("VoxelVertexColor")),
			[](FMeshDescription& MeshDescription)
			{
				BuildVoxelMeshDescription(
					MeshDescription,
					FName(TEXT("VoxelVertexColor")),
					FVector3f(540.0f, 110.0f, 80.0f),
					[](TArray<FEnemyVoxelBox>& OutBoxes)
					{
						AppendBrokenBridgeVoxelBoxes(OutBoxes);
					});
			},
			VoxelMaterial);

		UStaticMesh* RepairedBridgeMesh = EnsureVoxelStaticMeshAsset(
			InteractionAssetPath,
			RepairedBridgeVoxelMeshAssetName,
			FName(TEXT("VoxelVertexColor")),
			[](FMeshDescription& MeshDescription)
			{
				BuildVoxelMeshDescription(
					MeshDescription,
					FName(TEXT("VoxelVertexColor")),
					FVector3f(540.0f, 110.0f, 80.0f),
					[](TArray<FEnemyVoxelBox>& OutBoxes)
					{
						AppendRepairedBridgeVoxelBoxes(OutBoxes);
					});
			},
			VoxelMaterial);

		return EnemyBodyMesh && EnemyForwardMarkerMesh && BrokenBridgeMesh && RepairedBridgeMesh;
	}

	bool EnsureCoverPointAssets()
	{
		UBlueprint* CoverPointBlueprint = EnsureBlueprint(
			CoverAssetPath,
			CoverPointAssetName,
			ATunaSweeperCoverPointActor::StaticClass());

		return CoverPointBlueprint != nullptr;
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

	void RegisterAllWidgetsInTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return;
		}

		TArray<UWidget*> Widgets;
		WidgetBlueprint->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			RegisterWidgetVariable(WidgetBlueprint, Widget);
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

		TArray<FName> ExistingVariableNames;
		WidgetBlueprint->WidgetVariableNameToGuidMap.GenerateKeyArray(ExistingVariableNames);
		for (const FName& ExistingVariableName : ExistingVariableNames)
		{
			UnregisterWidgetVariable(WidgetBlueprint, ExistingVariableName);
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

		TextBlock->SetText(Text);
		TunaSweeperUIFont::ApplyFont(TextBlock, FontSize);
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

	FSlateBrush MakeRoundedBoxBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float OutlineWidth,
		float CornerRadius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(CornerRadius, FSlateColor(OutlineColor), OutlineWidth);
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

	bool BuildIntroMenuWidgetTree(UWidgetBlueprint* WidgetBlueprint)
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
		UVerticalBox* MenuStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MenuStack"));
		UVerticalBox* MainMenuPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenuPanel"));
		UHorizontalBox* MainMenuRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("MainMenuRow"));
		USizeBox* CurrentSaveSlotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CurrentSaveSlotBox"));
		UBorder* CurrentSaveSlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CurrentSaveSlotBorder"));
		UTextBlock* CurrentSaveSlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CurrentSaveSlotText"));
		USizeBox* StartButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StartButtonBox"));
		UButton* StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
		UTextBlock* StartButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartButtonText"));
		USizeBox* SlotSelectButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotSelectButtonBox"));
		UButton* SlotSelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SlotSelectButton"));
		UTextBlock* SlotSelectButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotSelectButtonText"));
		UVerticalBox* SaveSlotPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SaveSlotPanel"));
		UHorizontalBox* SaveSlotButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SaveSlotButtonRow"));
		USizeBox* SaveSlot1ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot1ButtonBox"));
		UButton* SaveSlot1Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot1Button"));
		UTextBlock* SaveSlot1Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot1Text"));
		USizeBox* SaveSlot2ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot2ButtonBox"));
		UButton* SaveSlot2Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot2Button"));
		UTextBlock* SaveSlot2Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot2Text"));
		USizeBox* SaveSlot3ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot3ButtonBox"));
		UButton* SaveSlot3Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot3Button"));
		UTextBlock* SaveSlot3Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot3Text"));
		UHorizontalBox* SaveSlotActionRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SaveSlotActionRow"));
		USizeBox* PrimarySaveSlotButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PrimarySaveSlotButtonBox"));
		UButton* PrimarySaveSlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PrimarySaveSlotButton"));
		UTextBlock* PrimarySaveSlotButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrimarySaveSlotButtonText"));
		USizeBox* DeleteSaveSlotButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DeleteSaveSlotButtonBox"));
		UButton* DeleteSaveSlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeleteSaveSlotButton"));
		UTextBlock* DeleteSaveSlotButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteSaveSlotButtonText"));
		USizeBox* QuitButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuitButtonBox"));
		UButton* QuitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuitButton"));
		UTextBlock* QuitButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuitButtonText"));

		if (!RootCanvas || !MenuStack || !MainMenuPanel || !MainMenuRow || !CurrentSaveSlotBox ||
			!CurrentSaveSlotBorder || !CurrentSaveSlotText || !StartButtonBox || !StartButton || !StartButtonText ||
			!SlotSelectButtonBox || !SlotSelectButton || !SlotSelectButtonText ||
			!SaveSlotPanel || !SaveSlotButtonRow || !SaveSlot1ButtonBox || !SaveSlot1Button || !SaveSlot1Text ||
			!SaveSlot2ButtonBox || !SaveSlot2Button || !SaveSlot2Text || !SaveSlot3ButtonBox || !SaveSlot3Button ||
			!SaveSlot3Text || !SaveSlotActionRow || !PrimarySaveSlotButtonBox || !PrimarySaveSlotButton ||
			!PrimarySaveSlotButtonText || !DeleteSaveSlotButtonBox || !DeleteSaveSlotButton || !DeleteSaveSlotButtonText ||
			!QuitButtonBox || !QuitButton || !QuitButtonText)
		{
			return false;
		}

		auto ConfigureMenuButton = [](UButton* Button, const FVector2D& ButtonSize, const FLinearColor& FillColor, const FLinearColor& HoveredColor)
		{
			if (!Button)
			{
				return;
			}

			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(ButtonSize, FillColor, FLinearColor(0.78f, 0.84f, 0.90f, 0.95f), 1.5f));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(ButtonSize, HoveredColor, FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), 2.0f));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(ButtonSize, FillColor * 0.78f, FLinearColor(0.68f, 0.75f, 0.84f, 1.0f), 1.0f));
			ButtonStyle.SetNormalPadding(FMargin(10.0f, 4.0f));
			ButtonStyle.SetPressedPadding(FMargin(10.0f, 5.0f, 10.0f, 3.0f));
			Button->SetStyle(ButtonStyle);
			Button->SetClickMethod(EButtonClickMethod::DownAndUp);
		};

		WidgetTree->RootWidget = RootCanvas;

		UCanvasPanelSlot* MenuStackSlot = RootCanvas->AddChildToCanvas(MenuStack);
		if (MenuStackSlot)
		{
			MenuStackSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			MenuStackSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			MenuStackSlot->SetPosition(FVector2D(0.0f, 80.0f));
			MenuStackSlot->SetSize(FVector2D(920.0f, 340.0f));
		}

		CurrentSaveSlotBox->SetWidthOverride(330.0f);
		CurrentSaveSlotBox->SetHeightOverride(120.0f);
		CurrentSaveSlotBorder->SetBrush(MakeRoundedBoxBrush(
			FVector2D(330.0f, 120.0f),
			FLinearColor(0.035f, 0.045f, 0.052f, 0.96f),
			FLinearColor(0.28f, 0.34f, 0.38f, 1.0f),
			1.2f));
		CurrentSaveSlotBorder->SetPadding(FMargin(16.0f, 10.0f));
		ConfigureTextBlock(
			CurrentSaveSlotText,
			FText::FromString(TEXT("\uC2AC\uB86F 1\n\uBE48 \uC2AC\uB86F")),
			FLinearColor(0.82f, 0.88f, 0.92f, 1.0f),
			19);
		CurrentSaveSlotText->SetAutoWrapText(true);
		CurrentSaveSlotBorder->SetContent(CurrentSaveSlotText);
		CurrentSaveSlotBox->SetContent(CurrentSaveSlotBorder);

		StartButtonBox->SetWidthOverride(230.0f);
		StartButtonBox->SetHeightOverride(120.0f);
		StartButtonBox->SetContent(StartButton);
		ConfigureMenuButton(
			StartButton,
			FVector2D(230.0f, 120.0f),
			FLinearColor(0.10f, 0.18f, 0.22f, 0.96f),
			FLinearColor(0.14f, 0.27f, 0.33f, 0.98f));
		ConfigureTextBlock(StartButtonText, FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30")), FLinearColor::White, 26);
		StartButton->SetContent(StartButtonText);

		SlotSelectButtonBox->SetWidthOverride(210.0f);
		SlotSelectButtonBox->SetHeightOverride(120.0f);
		SlotSelectButtonBox->SetContent(SlotSelectButton);
		ConfigureMenuButton(
			SlotSelectButton,
			FVector2D(210.0f, 120.0f),
			FLinearColor(0.055f, 0.075f, 0.085f, 0.96f),
			FLinearColor(0.10f, 0.16f, 0.19f, 0.98f));
		ConfigureTextBlock(SlotSelectButtonText, FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD")), FLinearColor(0.90f, 0.94f, 0.96f, 1.0f), 24);
		SlotSelectButton->SetContent(SlotSelectButtonText);

		for (UWidget* MainMenuItem : { static_cast<UWidget*>(CurrentSaveSlotBox), static_cast<UWidget*>(StartButtonBox), static_cast<UWidget*>(SlotSelectButtonBox) })
		{
			UHorizontalBoxSlot* MainMenuItemSlot = MainMenuRow->AddChildToHorizontalBox(MainMenuItem);
			if (MainMenuItemSlot)
			{
				MainMenuItemSlot->SetHorizontalAlignment(HAlign_Center);
				MainMenuItemSlot->SetVerticalAlignment(VAlign_Center);
				MainMenuItemSlot->SetPadding(FMargin(8.0f, 0.0f));
			}
		}

		UVerticalBoxSlot* MainMenuRowSlot = MainMenuPanel->AddChildToVerticalBox(MainMenuRow);
		if (MainMenuRowSlot)
		{
			MainMenuRowSlot->SetHorizontalAlignment(HAlign_Center);
			MainMenuRowSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* MainMenuPanelSlot = MenuStack->AddChildToVerticalBox(MainMenuPanel);
		if (MainMenuPanelSlot)
		{
			MainMenuPanelSlot->SetHorizontalAlignment(HAlign_Center);
			MainMenuPanelSlot->SetVerticalAlignment(VAlign_Center);
			MainMenuPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
		}

		auto ConfigureSaveSlotButton = [&ConfigureMenuButton](USizeBox* ButtonBox, UButton* Button, UTextBlock* TextBlock, int32 SlotIndex)
		{
			ButtonBox->SetWidthOverride(260.0f);
			ButtonBox->SetHeightOverride(120.0f);
			ButtonBox->SetContent(Button);
			ConfigureMenuButton(
				Button,
				FVector2D(260.0f, 120.0f),
				FLinearColor(0.055f, 0.075f, 0.085f, 0.96f),
				FLinearColor(0.10f, 0.16f, 0.19f, 0.98f));
			ConfigureTextBlock(
				TextBlock,
				FText::FromString(FString::Printf(TEXT("\uC2AC\uB86F %d\n\uBE48 \uC2AC\uB86F"), SlotIndex)),
				FLinearColor(0.82f, 0.88f, 0.92f, 1.0f),
				18);
			TextBlock->SetAutoWrapText(true);
			Button->SetContent(TextBlock);
		};

		ConfigureSaveSlotButton(SaveSlot1ButtonBox, SaveSlot1Button, SaveSlot1Text, 1);
		ConfigureSaveSlotButton(SaveSlot2ButtonBox, SaveSlot2Button, SaveSlot2Text, 2);
		ConfigureSaveSlotButton(SaveSlot3ButtonBox, SaveSlot3Button, SaveSlot3Text, 3);

		for (USizeBox* SlotButtonBox : { SaveSlot1ButtonBox, SaveSlot2ButtonBox, SaveSlot3ButtonBox })
		{
			UHorizontalBoxSlot* SlotButtonSlot = SaveSlotButtonRow->AddChildToHorizontalBox(SlotButtonBox);
			if (SlotButtonSlot)
			{
				SlotButtonSlot->SetHorizontalAlignment(HAlign_Center);
				SlotButtonSlot->SetVerticalAlignment(VAlign_Center);
				SlotButtonSlot->SetPadding(FMargin(8.0f, 0.0f));
			}
		}

		UVerticalBoxSlot* SaveSlotButtonRowSlot = SaveSlotPanel->AddChildToVerticalBox(SaveSlotButtonRow);
		if (SaveSlotButtonRowSlot)
		{
			SaveSlotButtonRowSlot->SetHorizontalAlignment(HAlign_Center);
			SaveSlotButtonRowSlot->SetVerticalAlignment(VAlign_Center);
		}

		PrimarySaveSlotButtonBox->SetWidthOverride(250.0f);
		PrimarySaveSlotButtonBox->SetHeightOverride(56.0f);
		PrimarySaveSlotButtonBox->SetContent(PrimarySaveSlotButton);
		ConfigureMenuButton(
			PrimarySaveSlotButton,
			FVector2D(250.0f, 56.0f),
			FLinearColor(0.10f, 0.18f, 0.22f, 0.96f),
			FLinearColor(0.14f, 0.27f, 0.33f, 0.98f));
		ConfigureTextBlock(
			PrimarySaveSlotButtonText,
			FText::FromString(TEXT("\uC138\uC774\uBE0C \uC2AC\uB86F \uC120\uD0DD")),
			FLinearColor::White,
			22);
		PrimarySaveSlotButton->SetContent(PrimarySaveSlotButtonText);

		DeleteSaveSlotButtonBox->SetWidthOverride(190.0f);
		DeleteSaveSlotButtonBox->SetHeightOverride(56.0f);
		DeleteSaveSlotButtonBox->SetContent(DeleteSaveSlotButton);
		ConfigureMenuButton(
			DeleteSaveSlotButton,
			FVector2D(190.0f, 56.0f),
			FLinearColor(0.42f, 0.045f, 0.04f, 0.96f),
			FLinearColor(0.62f, 0.07f, 0.06f, 0.98f));
		ConfigureTextBlock(
			DeleteSaveSlotButtonText,
			FText::FromString(TEXT("\uC0AD\uC81C\uD558\uAE30")),
			FLinearColor::White,
			22);
		DeleteSaveSlotButton->SetContent(DeleteSaveSlotButtonText);

		UHorizontalBoxSlot* PrimaryActionSlot = SaveSlotActionRow->AddChildToHorizontalBox(PrimarySaveSlotButtonBox);
		if (PrimaryActionSlot)
		{
			PrimaryActionSlot->SetHorizontalAlignment(HAlign_Center);
			PrimaryActionSlot->SetVerticalAlignment(VAlign_Center);
			PrimaryActionSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
		}

		UHorizontalBoxSlot* DeleteActionSlot = SaveSlotActionRow->AddChildToHorizontalBox(DeleteSaveSlotButtonBox);
		if (DeleteActionSlot)
		{
			DeleteActionSlot->SetHorizontalAlignment(HAlign_Center);
			DeleteActionSlot->SetVerticalAlignment(VAlign_Center);
		}
		SaveSlotActionRow->SetVisibility(ESlateVisibility::Collapsed);

		UVerticalBoxSlot* ActionRowSlot = SaveSlotPanel->AddChildToVerticalBox(SaveSlotActionRow);
		if (ActionRowSlot)
		{
			ActionRowSlot->SetHorizontalAlignment(HAlign_Center);
			ActionRowSlot->SetVerticalAlignment(VAlign_Center);
			ActionRowSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
		}

		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
		UVerticalBoxSlot* SaveSlotPanelSlot = MenuStack->AddChildToVerticalBox(SaveSlotPanel);
		if (SaveSlotPanelSlot)
		{
			SaveSlotPanelSlot->SetHorizontalAlignment(HAlign_Center);
			SaveSlotPanelSlot->SetVerticalAlignment(VAlign_Center);
			SaveSlotPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
		}

		QuitButtonBox->SetWidthOverride(220.0f);
		QuitButtonBox->SetHeightOverride(54.0f);
		QuitButtonBox->SetContent(QuitButton);
		ConfigureMenuButton(
			QuitButton,
			FVector2D(220.0f, 54.0f),
			FLinearColor(0.055f, 0.065f, 0.075f, 0.92f),
			FLinearColor(0.12f, 0.14f, 0.16f, 0.96f));
		ConfigureTextBlock(QuitButtonText, FText::FromString(TEXT("\uC885\uB8CC")), FLinearColor(0.90f, 0.94f, 0.96f, 1.0f), 20);
		QuitButton->SetContent(QuitButtonText);

		UVerticalBoxSlot* QuitSlot = MenuStack->AddChildToVerticalBox(QuitButtonBox);
		if (QuitSlot)
		{
			QuitSlot->SetHorizontalAlignment(HAlign_Center);
			QuitSlot->SetVerticalAlignment(VAlign_Center);
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildTitleIntroMenuWidgetTree(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
		{
			return false;
		}

		WidgetBlueprint->Modify();
		WidgetBlueprint->WidgetTree->Modify();
		ClearWidgetTreeForRebuild(WidgetBlueprint);

		UTexture2D* BackgroundTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UITitleTextureAssetPath, TitleBackgroundTextureAssetName));
		UTexture2D* LogoTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UITitleTextureAssetPath, TitleLogoTextureAssetName));

		UWidgetTree* WidgetTree = WidgetBlueprint->WidgetTree;
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UImage* BackgroundImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BackgroundImage"));
		UBorder* LeftScrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftScrim"));
		UImage* LogoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("LogoImage"));
		UVerticalBox* MainMenuPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("MainMenuPanel"));
		USizeBox* StartButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StartButtonBox"));
		UButton* StartButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StartButton"));
		UTextBlock* StartButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StartButtonText"));
		USizeBox* CurrentSaveSlotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CurrentSaveSlotBox"));
		UBorder* CurrentSaveSlotBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CurrentSaveSlotBorder"));
		UTextBlock* CurrentSaveSlotText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CurrentSaveSlotText"));
		USizeBox* SlotSelectButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SlotSelectButtonBox"));
		UButton* SlotSelectButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SlotSelectButton"));
		UTextBlock* SlotSelectButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotSelectButtonText"));
		USizeBox* SettingsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SettingsButtonBox"));
		UButton* SettingsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsButton"));
		UTextBlock* SettingsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsButtonText"));
		USizeBox* CreditsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsButtonBox"));
		UButton* CreditsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CreditsButton"));
		UTextBlock* CreditsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsButtonText"));
		USizeBox* QuitButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuitButtonBox"));
		UButton* QuitButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuitButton"));
		UTextBlock* QuitButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuitButtonText"));
		UCanvasPanel* SaveSlotPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SaveSlotPanel"));
		UBorder* SaveSlotBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SaveSlotBackdrop"));
		UBorder* SaveSlotContentBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SaveSlotContentBackground"));
		UVerticalBox* SaveSlotContentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SaveSlotContentStack"));
		UTextBlock* SaveSlotPanelTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlotPanelTitleText"));
		USizeBox* SaveSlot1ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot1ButtonBox"));
		UButton* SaveSlot1Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot1Button"));
		UTextBlock* SaveSlot1Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot1Text"));
		USizeBox* SaveSlot2ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot2ButtonBox"));
		UButton* SaveSlot2Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot2Button"));
		UTextBlock* SaveSlot2Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot2Text"));
		USizeBox* SaveSlot3ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SaveSlot3ButtonBox"));
		UButton* SaveSlot3Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SaveSlot3Button"));
		UTextBlock* SaveSlot3Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SaveSlot3Text"));
		UVerticalBox* SaveSlotActionRow = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SaveSlotActionRow"));
		USizeBox* PrimarySaveSlotButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PrimarySaveSlotButtonBox"));
		UButton* PrimarySaveSlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PrimarySaveSlotButton"));
		UTextBlock* PrimarySaveSlotButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PrimarySaveSlotButtonText"));
		USizeBox* DeleteSaveSlotButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DeleteSaveSlotButtonBox"));
		UButton* DeleteSaveSlotButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DeleteSaveSlotButton"));
		UTextBlock* DeleteSaveSlotButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteSaveSlotButtonText"));
		UHorizontalBox* DeleteSaveSlotButtonContent = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DeleteSaveSlotButtonContent"));
		USizeBox* DeleteHoldGaugeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DeleteHoldGaugeBox"));
		UOverlay* DeleteHoldGaugeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("DeleteHoldGaugeOverlay"));
		UBorder* DeleteHoldGaugeRing = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DeleteHoldGaugeRing"));
		UImage* DeleteHoldGaugeFill = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DeleteHoldGaugeFill"));
		USizeBox* BackToMainMenuButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackToMainMenuButtonBox"));
		UButton* BackToMainMenuButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackToMainMenuButton"));
		UTextBlock* BackToMainMenuButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackToMainMenuButtonText"));
		UBorder* DeleteConfirmPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DeleteConfirmPanel"));
		UVerticalBox* DeleteConfirmStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DeleteConfirmStack"));
		UTextBlock* DeleteConfirmTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteConfirmTitleText"));
		UTextBlock* DeleteConfirmMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DeleteConfirmMessageText"));
		UHorizontalBox* DeleteConfirmButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DeleteConfirmButtonRow"));
		USizeBox* ConfirmDeleteButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ConfirmDeleteButtonBox"));
		UButton* ConfirmDeleteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmDeleteButton"));
		UTextBlock* ConfirmDeleteButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmDeleteButtonText"));
		USizeBox* CancelDeleteButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CancelDeleteButtonBox"));
		UButton* CancelDeleteButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelDeleteButton"));
		UTextBlock* CancelDeleteButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelDeleteButtonText"));
		UCanvasPanel* SettingsPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("SettingsPanel"));
		UBorder* SettingsBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsBackdrop"));
		UBorder* SettingsContentBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsContentBackground"));
		UVerticalBox* SettingsContentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SettingsContentStack"));
		UTextBlock* SettingsTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsTitleText"));
		UTextBlock* SettingsStatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsStatusText"));
		UHorizontalBox* SettingsTabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SettingsTabRow"));
		USizeBox* GraphicsTabButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("GraphicsTabButtonBox"));
		UButton* SettingsGraphicsTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsGraphicsTabButton"));
		UTextBlock* SettingsGraphicsTabButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsGraphicsTabButtonText"));
		USizeBox* InterfaceTabButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InterfaceTabButtonBox"));
		UButton* SettingsInterfaceTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SettingsInterfaceTabButton"));
		UTextBlock* SettingsInterfaceTabButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SettingsInterfaceTabButtonText"));
		UVerticalBox* GraphicsSettingsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("GraphicsSettingsPanel"));
		UVerticalBox* InterfaceSettingsPanel = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InterfaceSettingsPanel"));
		UTextBlock* WindowModeLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WindowModeLabelText"));
		UHorizontalBox* WindowModeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WindowModeRow"));
		USizeBox* WindowedModeButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("WindowedModeButtonBox"));
		UButton* WindowedModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("WindowedModeButton"));
		UTextBlock* WindowedModeButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WindowedModeButtonText"));
		USizeBox* BorderlessWindowModeButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BorderlessWindowModeButtonBox"));
		UButton* BorderlessWindowModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BorderlessWindowModeButton"));
		UTextBlock* BorderlessWindowModeButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BorderlessWindowModeButtonText"));
		USizeBox* FullscreenModeButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("FullscreenModeButtonBox"));
		UButton* FullscreenModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("FullscreenModeButton"));
		UTextBlock* FullscreenModeButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FullscreenModeButtonText"));
		UTextBlock* ResolutionLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ResolutionLabelText"));
		UVerticalBox* ResolutionButtonStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ResolutionButtonStack"));
		USizeBox* Resolution1280ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution1280ButtonBox"));
		UButton* Resolution1280Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution1280Button"));
		UTextBlock* Resolution1280ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution1280ButtonText"));
		USizeBox* Resolution1600ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution1600ButtonBox"));
		UButton* Resolution1600Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution1600Button"));
		UTextBlock* Resolution1600ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution1600ButtonText"));
		USizeBox* Resolution1920ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution1920ButtonBox"));
		UButton* Resolution1920Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution1920Button"));
		UTextBlock* Resolution1920ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution1920ButtonText"));
		USizeBox* Resolution2560ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution2560ButtonBox"));
		UButton* Resolution2560Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution2560Button"));
		UTextBlock* Resolution2560ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution2560ButtonText"));
		USizeBox* Resolution3840ButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Resolution3840ButtonBox"));
		UButton* Resolution3840Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Resolution3840Button"));
		UTextBlock* Resolution3840ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Resolution3840ButtonText"));
		UTextBlock* DLSSLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSLabelText"));
		UHorizontalBox* DLSSButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DLSSButtonRow"));
		USizeBox* DLSSOffButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DLSSOffButtonBox"));
		UButton* DLSSOffButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DLSSOffButton"));
		UTextBlock* DLSSOffButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSOffButtonText"));
		USizeBox* DLSSQualityButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DLSSQualityButtonBox"));
		UButton* DLSSQualityButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DLSSQualityButton"));
		UTextBlock* DLSSQualityButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSQualityButtonText"));
		USizeBox* DLSSBalancedButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DLSSBalancedButtonBox"));
		UButton* DLSSBalancedButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DLSSBalancedButton"));
		UTextBlock* DLSSBalancedButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSBalancedButtonText"));
		USizeBox* DLSSPerformanceButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DLSSPerformanceButtonBox"));
		UButton* DLSSPerformanceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DLSSPerformanceButton"));
		UTextBlock* DLSSPerformanceButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DLSSPerformanceButtonText"));
		USizeBox* BackFromSettingsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackFromSettingsButtonBox"));
		UButton* BackFromSettingsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackFromSettingsButton"));
		UTextBlock* BackFromSettingsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackFromSettingsButtonText"));
		UTextBlock* LanguageLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LanguageLabelText"));
		UVerticalBox* LanguageButtonStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("LanguageButtonStack"));
		USizeBox* LanguageEnglishButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LanguageEnglishButtonBox"));
		UButton* LanguageEnglishButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LanguageEnglishButton"));
		UTextBlock* LanguageEnglishButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LanguageEnglishButtonText"));
		USizeBox* LanguageKoreanButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LanguageKoreanButtonBox"));
		UButton* LanguageKoreanButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LanguageKoreanButton"));
		UTextBlock* LanguageKoreanButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LanguageKoreanButtonText"));
		USizeBox* LanguageJapaneseButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("LanguageJapaneseButtonBox"));
		UButton* LanguageJapaneseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("LanguageJapaneseButton"));
		UTextBlock* LanguageJapaneseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LanguageJapaneseButtonText"));
		UHorizontalBox* InterfaceActionButtonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InterfaceActionButtonRow"));
		USizeBox* ConfirmInterfaceSettingsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ConfirmInterfaceSettingsButtonBox"));
		UButton* ConfirmInterfaceSettingsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmInterfaceSettingsButton"));
		UTextBlock* ConfirmInterfaceSettingsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ConfirmInterfaceSettingsButtonText"));
		USizeBox* CancelInterfaceSettingsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CancelInterfaceSettingsButtonBox"));
		UButton* CancelInterfaceSettingsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelInterfaceSettingsButton"));
		UTextBlock* CancelInterfaceSettingsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CancelInterfaceSettingsButtonText"));
		UCanvasPanel* CreditsPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CreditsPanel"));
		UBorder* CreditsBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CreditsBackdrop"));
		UVerticalBox* CreditsContentStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CreditsContentStack"));
		UTextBlock* CreditsTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsTitleText"));
		UHorizontalBox* CreditsColumnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CreditsColumnRow"));
		USizeBox* CreditsScrollBoxFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsScrollBoxFrame"));
		UScrollBox* CreditsScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CreditsScrollBox"));
		UTextBlock* CreditsText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsText"));
		USizeBox* CreditsScrollBoxFrame2 = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsScrollBoxFrame2"));
		UScrollBox* CreditsScrollBox2 = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CreditsScrollBox2"));
		UTextBlock* CreditsText2 = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsText2"));
		USizeBox* CreditsScrollBoxFrame3 = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CreditsScrollBoxFrame3"));
		UScrollBox* CreditsScrollBox3 = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("CreditsScrollBox3"));
		UTextBlock* CreditsText3 = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CreditsText3"));
		USizeBox* BackFromCreditsButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BackFromCreditsButtonBox"));
		UButton* BackFromCreditsButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackFromCreditsButton"));
		UTextBlock* BackFromCreditsButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackFromCreditsButtonText"));
		UTextBlock* VersionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VersionText"));

		if (!RootCanvas || !BackgroundImage || !LeftScrim || !LogoImage || !MainMenuPanel || !StartButtonBox ||
			!StartButton || !StartButtonText || !CurrentSaveSlotBox || !CurrentSaveSlotBorder || !CurrentSaveSlotText ||
			!SlotSelectButtonBox || !SlotSelectButton || !SlotSelectButtonText || !SettingsButtonBox || !SettingsButton ||
			!SettingsButtonText || !CreditsButtonBox || !CreditsButton || !CreditsButtonText || !QuitButtonBox ||
			!QuitButton || !QuitButtonText || !SaveSlotPanel || !SaveSlotBackdrop || !SaveSlotContentBackground ||
			!SaveSlotContentStack || !SaveSlotPanelTitleText || !SaveSlot1ButtonBox || !SaveSlot1Button || !SaveSlot1Text || !SaveSlot2ButtonBox || !SaveSlot2Button || !SaveSlot2Text ||
			!SaveSlot3ButtonBox || !SaveSlot3Button || !SaveSlot3Text || !SaveSlotActionRow || !PrimarySaveSlotButtonBox ||
			!PrimarySaveSlotButton || !PrimarySaveSlotButtonText || !DeleteSaveSlotButtonBox || !DeleteSaveSlotButton ||
			!DeleteSaveSlotButtonText || !DeleteSaveSlotButtonContent || !DeleteHoldGaugeBox || !DeleteHoldGaugeOverlay ||
			!DeleteHoldGaugeRing || !DeleteHoldGaugeFill || !BackToMainMenuButtonBox || !BackToMainMenuButton ||
			!BackToMainMenuButtonText || !DeleteConfirmPanel || !DeleteConfirmStack || !DeleteConfirmTitleText ||
			!DeleteConfirmMessageText || !DeleteConfirmButtonRow || !ConfirmDeleteButtonBox || !ConfirmDeleteButton ||
			!ConfirmDeleteButtonText || !CancelDeleteButtonBox || !CancelDeleteButton || !CancelDeleteButtonText ||
			!SettingsPanel || !SettingsBackdrop || !SettingsContentBackground || !SettingsContentStack ||
			!SettingsTitleText || !SettingsStatusText || !SettingsTabRow || !GraphicsTabButtonBox ||
			!SettingsGraphicsTabButton || !SettingsGraphicsTabButtonText || !InterfaceTabButtonBox ||
			!SettingsInterfaceTabButton || !SettingsInterfaceTabButtonText || !GraphicsSettingsPanel ||
			!InterfaceSettingsPanel || !WindowModeLabelText || !WindowModeRow ||
			!WindowedModeButtonBox || !WindowedModeButton || !WindowedModeButtonText ||
			!BorderlessWindowModeButtonBox || !BorderlessWindowModeButton || !BorderlessWindowModeButtonText ||
			!FullscreenModeButtonBox || !FullscreenModeButton || !FullscreenModeButtonText ||
			!ResolutionLabelText || !ResolutionButtonStack || !Resolution1280ButtonBox || !Resolution1280Button ||
			!Resolution1280ButtonText || !Resolution1600ButtonBox || !Resolution1600Button || !Resolution1600ButtonText ||
			!Resolution1920ButtonBox || !Resolution1920Button || !Resolution1920ButtonText || !Resolution2560ButtonBox ||
			!Resolution2560Button || !Resolution2560ButtonText || !Resolution3840ButtonBox || !Resolution3840Button ||
			!Resolution3840ButtonText || !DLSSLabelText || !DLSSButtonRow || !DLSSOffButtonBox || !DLSSOffButton ||
			!DLSSOffButtonText || !DLSSQualityButtonBox || !DLSSQualityButton || !DLSSQualityButtonText ||
			!DLSSBalancedButtonBox || !DLSSBalancedButton || !DLSSBalancedButtonText ||
			!DLSSPerformanceButtonBox || !DLSSPerformanceButton || !DLSSPerformanceButtonText ||
			!BackFromSettingsButtonBox || !BackFromSettingsButton ||
			!BackFromSettingsButtonText || !LanguageLabelText || !LanguageButtonStack || !LanguageEnglishButtonBox ||
			!LanguageEnglishButton || !LanguageEnglishButtonText || !LanguageKoreanButtonBox || !LanguageKoreanButton ||
			!LanguageKoreanButtonText || !LanguageJapaneseButtonBox || !LanguageJapaneseButton || !LanguageJapaneseButtonText ||
			!InterfaceActionButtonRow || !ConfirmInterfaceSettingsButtonBox || !ConfirmInterfaceSettingsButton ||
			!ConfirmInterfaceSettingsButtonText || !CancelInterfaceSettingsButtonBox || !CancelInterfaceSettingsButton ||
			!CancelInterfaceSettingsButtonText || !CreditsPanel || !CreditsBackdrop || !CreditsContentStack || !CreditsTitleText ||
			!CreditsColumnRow || !CreditsScrollBoxFrame || !CreditsScrollBox || !CreditsText ||
			!CreditsScrollBoxFrame2 || !CreditsScrollBox2 || !CreditsText2 || !CreditsScrollBoxFrame3 ||
			!CreditsScrollBox3 || !CreditsText3 ||
			!BackFromCreditsButtonBox || !BackFromCreditsButton || !BackFromCreditsButtonText || !VersionText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		auto FillCanvas = [](UCanvasPanelSlot* Slot)
		{
			if (!Slot)
			{
				return;
			}

			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetAlignment(FVector2D::ZeroVector);
		};

		auto ConfigureButtonStyle = [](UButton* Button, const FVector2D& ButtonSize, bool bPrimary)
		{
			const FLinearColor FillColor = bPrimary
				? FLinearColor(0.025f, 0.045f, 0.050f, 0.76f)
				: FLinearColor(0.025f, 0.045f, 0.050f, 0.56f);
			const FLinearColor HoveredColor = bPrimary
				? FLinearColor(0.075f, 0.13f, 0.14f, 0.88f)
				: FLinearColor(0.055f, 0.095f, 0.105f, 0.76f);
			const float CornerRadius = bPrimary ? 14.0f : 11.0f;

			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(ButtonSize, FillColor, FLinearColor(0.78f, 0.84f, 0.82f, 0.88f), 1.3f, CornerRadius));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(ButtonSize, HoveredColor, FLinearColor(0.96f, 0.98f, 0.95f, 1.0f), 1.7f, CornerRadius));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(ButtonSize, FillColor * 0.75f, FLinearColor(0.60f, 0.68f, 0.68f, 0.90f), 1.0f, CornerRadius));
			ButtonStyle.SetNormalPadding(FMargin(0.0f));
			ButtonStyle.SetPressedPadding(FMargin(0.0f, 1.0f, 0.0f, 0.0f));
			Button->SetStyle(ButtonStyle);
			Button->SetClickMethod(EButtonClickMethod::DownAndUp);
		};

		auto MakeButtonContent = [WidgetTree](
			const FString& NamePrefix,
			const FText& Icon,
			UTextBlock* LabelText,
			const FText& Label,
			int32 LabelFontSize,
			int32 IconFontSize)
		{
			UHorizontalBox* Content = WidgetTree->ConstructWidget<UHorizontalBox>(
				UHorizontalBox::StaticClass(),
				FName(*(NamePrefix + TEXT("Content"))));
			USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(NamePrefix + TEXT("IconBox"))));
			UTextBlock* IconText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(NamePrefix + TEXT("IconText"))));
			USizeBox* BalanceBox = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(NamePrefix + TEXT("BalanceBox"))));

			if (!Content || !IconBox || !IconText || !BalanceBox)
			{
				return static_cast<UWidget*>(LabelText);
			}

			ConfigureTextBlock(IconText, Icon, FLinearColor(0.94f, 0.92f, 0.84f, 1.0f), IconFontSize);
			ConfigureTextBlock(LabelText, Label, FLinearColor(0.94f, 0.92f, 0.84f, 1.0f), LabelFontSize);

			const float IconLaneWidth = IconFontSize >= 28 ? 58.0f : 46.0f;
			IconBox->SetWidthOverride(IconLaneWidth);
			IconBox->SetHeightOverride(IconFontSize + 8.0f);
			IconBox->SetContent(IconText);
			BalanceBox->SetWidthOverride(IconLaneWidth);
			BalanceBox->SetHeightOverride(IconFontSize + 8.0f);

			UHorizontalBoxSlot* IconSlot = Content->AddChildToHorizontalBox(IconBox);
			if (IconSlot)
			{
				IconSlot->SetHorizontalAlignment(HAlign_Center);
				IconSlot->SetVerticalAlignment(VAlign_Center);
				IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}

			UHorizontalBoxSlot* LabelSlot = Content->AddChildToHorizontalBox(LabelText);
			if (LabelSlot)
			{
				LabelSlot->SetHorizontalAlignment(HAlign_Center);
				LabelSlot->SetVerticalAlignment(VAlign_Center);
				LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}

			UHorizontalBoxSlot* BalanceSlot = Content->AddChildToHorizontalBox(BalanceBox);
			if (BalanceSlot)
			{
				BalanceSlot->SetHorizontalAlignment(HAlign_Center);
				BalanceSlot->SetVerticalAlignment(VAlign_Center);
				BalanceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			}

			return static_cast<UWidget*>(Content);
		};

		auto ConfigureMenuButton = [&ConfigureButtonStyle, &MakeButtonContent](
			USizeBox* ButtonBox,
			UButton* Button,
			UTextBlock* ButtonText,
			const FString& NamePrefix,
			const FText& Icon,
			const FText& Label,
			const FVector2D& ButtonSize,
			bool bPrimary)
		{
			ButtonBox->SetWidthOverride(ButtonSize.X);
			ButtonBox->SetHeightOverride(ButtonSize.Y);
			ButtonBox->SetContent(Button);
			ConfigureButtonStyle(Button, ButtonSize, bPrimary);
			Button->SetContent(MakeButtonContent(
				NamePrefix,
				Icon,
				ButtonText,
				Label,
				bPrimary ? 28 : 20,
				bPrimary ? 28 : 20));
		};

		auto ConfigurePlainButton = [&ConfigureButtonStyle](
			USizeBox* ButtonBox,
			UButton* Button,
			UTextBlock* ButtonText,
			const FText& Label,
			const FVector2D& ButtonSize,
			bool bPrimary)
		{
			ButtonBox->SetWidthOverride(ButtonSize.X);
			ButtonBox->SetHeightOverride(ButtonSize.Y);
			ButtonBox->SetContent(Button);
			ConfigureButtonStyle(Button, ButtonSize, bPrimary);
			ConfigureTextBlock(ButtonText, Label, FLinearColor::White, bPrimary ? 19 : 17);
			Button->SetContent(ButtonText);
		};

		if (BackgroundTexture)
		{
			BackgroundImage->SetBrushFromTexture(BackgroundTexture, false);
			FSlateBrush BackgroundBrush = BackgroundImage->GetBrush();
			BackgroundBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));
			BackgroundImage->SetBrush(BackgroundBrush);
		}
		BackgroundImage->SetColorAndOpacity(FLinearColor::White);
		FillCanvas(RootCanvas->AddChildToCanvas(BackgroundImage));

		FSlateBrush ScrimBrush;
		ScrimBrush.DrawAs = ESlateBrushDrawType::Box;
		ScrimBrush.TintColor = FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.42f));
		LeftScrim->SetBrush(ScrimBrush);
		UCanvasPanelSlot* ScrimSlot = RootCanvas->AddChildToCanvas(LeftScrim);
		if (ScrimSlot)
		{
			ScrimSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			ScrimSlot->SetAlignment(FVector2D::ZeroVector);
			ScrimSlot->SetPosition(FVector2D(0.0f, 0.0f));
			ScrimSlot->SetSize(FVector2D(720.0f, 1080.0f));
		}

		if (LogoTexture)
		{
			LogoImage->SetBrushFromTexture(LogoTexture, false);
			FSlateBrush LogoBrush = LogoImage->GetBrush();
			LogoBrush.SetImageSize(FVector2D(400.0f, 162.0f));
			LogoImage->SetBrush(LogoBrush);
		}
		LogoImage->SetColorAndOpacity(FLinearColor::White);
		UCanvasPanelSlot* LogoSlot = RootCanvas->AddChildToCanvas(LogoImage);
		if (LogoSlot)
		{
			LogoSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			LogoSlot->SetAlignment(FVector2D::ZeroVector);
			LogoSlot->SetPosition(FVector2D(80.0f, 86.0f));
			LogoSlot->SetSize(FVector2D(400.0f, 162.0f));
		}

		ConfigureMenuButton(
			StartButtonBox,
			StartButton,
			StartButtonText,
			TEXT("StartButton"),
			FText::FromString(TEXT("\u25B6")),
			FText::FromString(TEXT("\uACC4\uC18D\uD558\uAE30")),
			FVector2D(440.0f, 72.0f),
			true);

		CurrentSaveSlotBox->SetWidthOverride(220.0f);
		CurrentSaveSlotBox->SetHeightOverride(38.0f);
		CurrentSaveSlotBox->SetContent(CurrentSaveSlotBorder);
		CurrentSaveSlotBorder->SetPadding(FMargin(14.0f, 4.0f));
		CurrentSaveSlotBorder->SetBrush(MakeRoundedBoxBrush(
			FVector2D(220.0f, 38.0f),
			FLinearColor(0.025f, 0.045f, 0.050f, 0.52f),
			FLinearColor(0.78f, 0.84f, 0.82f, 0.74f),
			1.0f,
			8.0f));
		ConfigureTextBlockLeft(
			CurrentSaveSlotText,
			FText::FromString(TEXT("\uC2AC\uB86F 1 - \uBE48 \uC2AC\uB86F")),
			FLinearColor(0.82f, 0.86f, 0.84f, 1.0f),
			14);
		CurrentSaveSlotBorder->SetContent(CurrentSaveSlotText);

		ConfigureMenuButton(
			SlotSelectButtonBox,
			SlotSelectButton,
			SlotSelectButtonText,
			TEXT("SlotSelectButton"),
			FText::FromString(TEXT("\u25A6")),
			FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD")),
			FVector2D(380.0f, 52.0f),
			false);
		ConfigureMenuButton(
			SettingsButtonBox,
			SettingsButton,
			SettingsButtonText,
			TEXT("SettingsButton"),
			FText::FromString(TEXT("\u2699")),
			FText::FromString(TEXT("\uC124\uC815")),
			FVector2D(380.0f, 52.0f),
			false);
		ConfigureMenuButton(
			CreditsButtonBox,
			CreditsButton,
			CreditsButtonText,
			TEXT("CreditsButton"),
			FText::FromString(TEXT("\u24D8")),
			FText::FromString(TEXT("\uD06C\uB808\uB527")),
			FVector2D(380.0f, 52.0f),
			false);
		ConfigureMenuButton(
			QuitButtonBox,
			QuitButton,
			QuitButtonText,
			TEXT("QuitButton"),
			FText::FromString(TEXT("\u00D7")),
			FText::FromString(TEXT("\uC885\uB8CC")),
			FVector2D(380.0f, 52.0f),
			false);

		for (UWidget* MenuItem : {
				static_cast<UWidget*>(StartButtonBox),
				static_cast<UWidget*>(CurrentSaveSlotBox),
				static_cast<UWidget*>(SlotSelectButtonBox),
				static_cast<UWidget*>(SettingsButtonBox),
				static_cast<UWidget*>(CreditsButtonBox),
				static_cast<UWidget*>(QuitButtonBox) })
		{
			UVerticalBoxSlot* ItemSlot = MainMenuPanel->AddChildToVerticalBox(MenuItem);
			if (ItemSlot)
			{
				ItemSlot->SetHorizontalAlignment(HAlign_Left);
				ItemSlot->SetVerticalAlignment(VAlign_Center);
				ItemSlot->SetPadding(MenuItem == CurrentSaveSlotBox ? FMargin(0.0f, 12.0f, 0.0f, 22.0f) : FMargin(0.0f, 0.0f, 0.0f, 12.0f));
			}
		}

		UCanvasPanelSlot* MainMenuSlot = RootCanvas->AddChildToCanvas(MainMenuPanel);
		if (MainMenuSlot)
		{
			MainMenuSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			MainMenuSlot->SetAlignment(FVector2D::ZeroVector);
			MainMenuSlot->SetPosition(FVector2D(92.0f, 310.0f));
			MainMenuSlot->SetSize(FVector2D(460.0f, 440.0f));
		}

		SaveSlotPanel->SetVisibility(ESlateVisibility::Collapsed);
		FillCanvas(RootCanvas->AddChildToCanvas(SaveSlotPanel));

		SaveSlotBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(1920.0f, 1080.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.58f),
			FLinearColor::Transparent,
			0.0f,
			0.0f));
		FillCanvas(SaveSlotPanel->AddChildToCanvas(SaveSlotBackdrop));

		SaveSlotContentBackground->SetPadding(FMargin(36.0f, 32.0f));
		SaveSlotContentBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(780.0f, 710.0f),
			FLinearColor(0.018f, 0.030f, 0.034f, 0.88f),
			FLinearColor(0.70f, 0.78f, 0.76f, 0.72f),
			1.2f,
			16.0f));
		SaveSlotContentBackground->SetContent(SaveSlotContentStack);
		UCanvasPanelSlot* SaveContentSlot = SaveSlotPanel->AddChildToCanvas(SaveSlotContentBackground);
		if (SaveContentSlot)
		{
			SaveContentSlot->SetAnchors(FAnchors(0.0f, 0.5f));
			SaveContentSlot->SetAlignment(FVector2D(0.0f, 0.5f));
			SaveContentSlot->SetPosition(FVector2D(88.0f, 14.0f));
			SaveContentSlot->SetSize(FVector2D(780.0f, 710.0f));
		}

		ConfigureTextBlockLeft(
			SaveSlotPanelTitleText,
			FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD")),
			FLinearColor(0.94f, 0.92f, 0.84f, 1.0f),
			30);
		UVerticalBoxSlot* SaveTitleSlot = SaveSlotContentStack->AddChildToVerticalBox(SaveSlotPanelTitleText);
		if (SaveTitleSlot)
		{
			SaveTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}

		auto ConfigureSaveSlotButton = [&ConfigureButtonStyle](USizeBox* ButtonBox, UButton* Button, UTextBlock* TextBlock, int32 SlotIndex)
		{
			ButtonBox->SetWidthOverride(700.0f);
			ButtonBox->SetHeightOverride(112.0f);
			ButtonBox->SetContent(Button);
			ConfigureButtonStyle(Button, FVector2D(700.0f, 112.0f), false);
			ConfigureTextBlockLeft(
				TextBlock,
				FText::FromString(FString::Printf(TEXT("\uC2AC\uB86F %d\n\uBE48 \uC2AC\uB86F\n\uC0C8 \uAC8C\uC784 \uC2DC\uC791"), SlotIndex)),
				FLinearColor(0.82f, 0.86f, 0.84f, 1.0f),
				18);
			TextBlock->SetMargin(FMargin(26.0f, 10.0f));
			TextBlock->SetAutoWrapText(true);
			Button->SetContent(TextBlock);
		};

		ConfigureSaveSlotButton(SaveSlot1ButtonBox, SaveSlot1Button, SaveSlot1Text, 1);
		ConfigureSaveSlotButton(SaveSlot2ButtonBox, SaveSlot2Button, SaveSlot2Text, 2);
		ConfigureSaveSlotButton(SaveSlot3ButtonBox, SaveSlot3Button, SaveSlot3Text, 3);

		for (UWidget* SlotButtonBox : { static_cast<UWidget*>(SaveSlot1ButtonBox), static_cast<UWidget*>(SaveSlot2ButtonBox), static_cast<UWidget*>(SaveSlot3ButtonBox) })
		{
			UVerticalBoxSlot* SlotButtonSlot = SaveSlotContentStack->AddChildToVerticalBox(SlotButtonBox);
			if (SlotButtonSlot)
			{
				SlotButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
			}
		}

		PrimarySaveSlotButtonBox->SetWidthOverride(420.0f);
		PrimarySaveSlotButtonBox->SetHeightOverride(56.0f);
		PrimarySaveSlotButtonBox->SetContent(PrimarySaveSlotButton);
		ConfigureButtonStyle(PrimarySaveSlotButton, FVector2D(420.0f, 56.0f), true);
		ConfigureTextBlock(PrimarySaveSlotButtonText, FText::FromString(TEXT("\uC2AC\uB86F \uC120\uD0DD")), FLinearColor::White, 19);
		PrimarySaveSlotButton->SetContent(PrimarySaveSlotButtonText);

		DeleteSaveSlotButtonBox->SetWidthOverride(420.0f);
		DeleteSaveSlotButtonBox->SetHeightOverride(56.0f);
		DeleteSaveSlotButtonBox->SetContent(DeleteSaveSlotButton);
		ConfigureButtonStyle(DeleteSaveSlotButton, FVector2D(420.0f, 56.0f), false);
		ConfigureTextBlockLeft(
			DeleteSaveSlotButtonText,
			FText::FromString(TEXT("\uAE38\uAC8C \uB20C\uB7EC \uC0AD\uC81C\uD558\uAE30")),
			FLinearColor::White,
			18);

		DeleteHoldGaugeBox->SetWidthOverride(36.0f);
		DeleteHoldGaugeBox->SetHeightOverride(36.0f);
		DeleteHoldGaugeBox->SetContent(DeleteHoldGaugeOverlay);
		DeleteHoldGaugeRing->SetBrush(MakeCircularBrush(
			FVector2D(34.0f, 34.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.0f),
			FLinearColor(0.95f, 0.92f, 0.84f, 0.72f),
			1.6f));
		DeleteHoldGaugeFill->SetBrush(MakeCircularBrush(
			FVector2D(34.0f, 34.0f),
			FLinearColor(0.86f, 0.30f, 0.24f, 0.86f),
			FLinearColor::Transparent,
			0.0f));
		DeleteHoldGaugeFill->SetRenderOpacity(0.0f);
		DeleteHoldGaugeFill->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		DeleteHoldGaugeFill->SetRenderScale(FVector2D::ZeroVector);
		UOverlaySlot* GaugeFillSlot = DeleteHoldGaugeOverlay->AddChildToOverlay(DeleteHoldGaugeFill);
		if (GaugeFillSlot)
		{
			GaugeFillSlot->SetHorizontalAlignment(HAlign_Center);
			GaugeFillSlot->SetVerticalAlignment(VAlign_Center);
		}
		UOverlaySlot* GaugeRingSlot = DeleteHoldGaugeOverlay->AddChildToOverlay(DeleteHoldGaugeRing);
		if (GaugeRingSlot)
		{
			GaugeRingSlot->SetHorizontalAlignment(HAlign_Center);
			GaugeRingSlot->SetVerticalAlignment(VAlign_Center);
		}

		UHorizontalBoxSlot* DeleteGaugeSlot = DeleteSaveSlotButtonContent->AddChildToHorizontalBox(DeleteHoldGaugeBox);
		if (DeleteGaugeSlot)
		{
			DeleteGaugeSlot->SetHorizontalAlignment(HAlign_Center);
			DeleteGaugeSlot->SetVerticalAlignment(VAlign_Center);
			DeleteGaugeSlot->SetPadding(FMargin(22.0f, 0.0f, 16.0f, 0.0f));
			DeleteGaugeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
		UHorizontalBoxSlot* DeleteLabelSlot = DeleteSaveSlotButtonContent->AddChildToHorizontalBox(DeleteSaveSlotButtonText);
		if (DeleteLabelSlot)
		{
			DeleteLabelSlot->SetHorizontalAlignment(HAlign_Fill);
			DeleteLabelSlot->SetVerticalAlignment(VAlign_Center);
			DeleteLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		DeleteSaveSlotButton->SetContent(DeleteSaveSlotButtonContent);

		SaveSlotActionRow->SetVisibility(ESlateVisibility::Collapsed);
		for (UWidget* ActionButton : { static_cast<UWidget*>(PrimarySaveSlotButtonBox), static_cast<UWidget*>(DeleteSaveSlotButtonBox) })
		{
			UVerticalBoxSlot* ActionButtonSlot = SaveSlotActionRow->AddChildToVerticalBox(ActionButton);
			if (ActionButtonSlot)
			{
				ActionButtonSlot->SetHorizontalAlignment(HAlign_Left);
				ActionButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
			}
		}
		UVerticalBoxSlot* ActionSlot = SaveSlotContentStack->AddChildToVerticalBox(SaveSlotActionRow);
		if (ActionSlot)
		{
			ActionSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 2.0f));
		}

		BackToMainMenuButtonBox->SetWidthOverride(420.0f);
		BackToMainMenuButtonBox->SetHeightOverride(54.0f);
		BackToMainMenuButtonBox->SetContent(BackToMainMenuButton);
		ConfigureButtonStyle(BackToMainMenuButton, FVector2D(420.0f, 54.0f), false);
		ConfigureTextBlock(BackToMainMenuButtonText, FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30")), FLinearColor::White, 18);
		BackToMainMenuButton->SetContent(BackToMainMenuButtonText);
		UVerticalBoxSlot* BackSlot = SaveSlotContentStack->AddChildToVerticalBox(BackToMainMenuButtonBox);
		if (BackSlot)
		{
			BackSlot->SetHorizontalAlignment(HAlign_Left);
			BackSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
		}

		DeleteConfirmPanel->SetPadding(FMargin(30.0f, 26.0f));
		DeleteConfirmPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(520.0f, 220.0f),
			FLinearColor(0.012f, 0.018f, 0.022f, 0.96f),
			FLinearColor(0.92f, 0.36f, 0.30f, 0.88f),
			1.4f,
			14.0f));
		DeleteConfirmPanel->SetContent(DeleteConfirmStack);
		DeleteConfirmPanel->SetVisibility(ESlateVisibility::Collapsed);
		ConfigureTextBlockLeft(
			DeleteConfirmTitleText,
			FText::FromString(TEXT("\uC2AC\uB86F \uC0AD\uC81C")),
			FLinearColor::White,
			24);
		ConfigureTextBlockLeft(
			DeleteConfirmMessageText,
			FText::FromString(TEXT("\uC120\uD0DD\uD55C \uC800\uC7A5 \uB370\uC774\uD130\uB97C \uC0AD\uC81C\uD560\uAE4C\uC694?")),
			FLinearColor(0.82f, 0.86f, 0.84f, 1.0f),
			17);
		DeleteConfirmStack->AddChildToVerticalBox(DeleteConfirmTitleText);
		UVerticalBoxSlot* ConfirmMessageSlot = DeleteConfirmStack->AddChildToVerticalBox(DeleteConfirmMessageText);
		if (ConfirmMessageSlot)
		{
			ConfirmMessageSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 22.0f));
		}

		ConfirmDeleteButtonBox->SetWidthOverride(188.0f);
		ConfirmDeleteButtonBox->SetHeightOverride(48.0f);
		ConfirmDeleteButtonBox->SetContent(ConfirmDeleteButton);
		ConfigureButtonStyle(ConfirmDeleteButton, FVector2D(188.0f, 48.0f), true);
		ConfigureTextBlock(ConfirmDeleteButtonText, FText::FromString(TEXT("\uC0AD\uC81C\uD558\uAE30")), FLinearColor::White, 17);
		ConfirmDeleteButton->SetContent(ConfirmDeleteButtonText);

		CancelDeleteButtonBox->SetWidthOverride(188.0f);
		CancelDeleteButtonBox->SetHeightOverride(48.0f);
		CancelDeleteButtonBox->SetContent(CancelDeleteButton);
		ConfigureButtonStyle(CancelDeleteButton, FVector2D(188.0f, 48.0f), false);
		ConfigureTextBlock(CancelDeleteButtonText, FText::FromString(TEXT("\uCDE8\uC18C")), FLinearColor::White, 17);
		CancelDeleteButton->SetContent(CancelDeleteButtonText);

		UHorizontalBoxSlot* ConfirmButtonSlot = DeleteConfirmButtonRow->AddChildToHorizontalBox(ConfirmDeleteButtonBox);
		if (ConfirmButtonSlot)
		{
			ConfirmButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
		}
		DeleteConfirmButtonRow->AddChildToHorizontalBox(CancelDeleteButtonBox);
		DeleteConfirmStack->AddChildToVerticalBox(DeleteConfirmButtonRow);

		UCanvasPanelSlot* ConfirmSlot = SaveSlotPanel->AddChildToCanvas(DeleteConfirmPanel);
		if (ConfirmSlot)
		{
			ConfirmSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			ConfirmSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			ConfirmSlot->SetPosition(FVector2D(0.0f, 0.0f));
			ConfirmSlot->SetSize(FVector2D(520.0f, 220.0f));
		}

		SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
		FillCanvas(RootCanvas->AddChildToCanvas(SettingsPanel));
		SettingsBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(1920.0f, 1080.0f),
			FLinearColor(0.006f, 0.010f, 0.012f, 0.96f),
			FLinearColor::Transparent,
			0.0f,
			0.0f));
		FillCanvas(SettingsPanel->AddChildToCanvas(SettingsBackdrop));

		SettingsContentBackground->SetPadding(FMargin(72.0f, 42.0f, 56.0f, 44.0f));
		SettingsContentBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(1920.0f, 1080.0f),
			FLinearColor::Transparent,
			FLinearColor::Transparent,
			0.0f,
			0.0f));
		SettingsContentBackground->SetContent(SettingsContentStack);
		UCanvasPanelSlot* SettingsContentSlot = SettingsPanel->AddChildToCanvas(SettingsContentBackground);
		if (SettingsContentSlot)
		{
			FillCanvas(SettingsContentSlot);
		}

		ConfigurePlainButton(BackFromSettingsButtonBox, BackFromSettingsButton, BackFromSettingsButtonText, FText::FromString(TEXT("\u2190")), FVector2D(52.0f, 52.0f), false);
		UVerticalBoxSlot* SettingsBackSlot = SettingsContentStack->AddChildToVerticalBox(BackFromSettingsButtonBox);
		if (SettingsBackSlot)
		{
			SettingsBackSlot->SetHorizontalAlignment(HAlign_Left);
			SettingsBackSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
		}

		ConfigureTextBlockLeft(SettingsTitleText, FText::FromString(TEXT("\uC124\uC815")), FLinearColor::White, 30);
		UVerticalBoxSlot* SettingsTitleSlot = SettingsContentStack->AddChildToVerticalBox(SettingsTitleText);
		if (SettingsTitleSlot)
		{
			SettingsTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}
		ConfigureTextBlockLeft(SettingsStatusText, FText::FromString(TEXT("\uD604\uC7AC: --")), FLinearColor(0.82f, 0.86f, 0.84f, 1.0f), 17);
		UVerticalBoxSlot* SettingsStatusSlot = SettingsContentStack->AddChildToVerticalBox(SettingsStatusText);
		if (SettingsStatusSlot)
		{
			SettingsStatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		ConfigurePlainButton(GraphicsTabButtonBox, SettingsGraphicsTabButton, SettingsGraphicsTabButtonText, FText::FromString(TEXT("\uADF8\uB798\uD53D")), FVector2D(158.0f, 44.0f), true);
		ConfigurePlainButton(InterfaceTabButtonBox, SettingsInterfaceTabButton, SettingsInterfaceTabButtonText, FText::FromString(TEXT("\uC778\uD130\uD398\uC774\uC2A4")), FVector2D(158.0f, 44.0f), false);
		for (UWidget* TabButtonBox : { static_cast<UWidget*>(GraphicsTabButtonBox), static_cast<UWidget*>(InterfaceTabButtonBox) })
		{
			UHorizontalBoxSlot* TabButtonSlot = SettingsTabRow->AddChildToHorizontalBox(TabButtonBox);
			if (TabButtonSlot)
			{
				TabButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			}
		}
		UVerticalBoxSlot* SettingsTabSlot = SettingsContentStack->AddChildToVerticalBox(SettingsTabRow);
		if (SettingsTabSlot)
		{
			SettingsTabSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
		}
		UVerticalBoxSlot* GraphicsSettingsSlot = SettingsContentStack->AddChildToVerticalBox(GraphicsSettingsPanel);
		if (GraphicsSettingsSlot)
		{
			GraphicsSettingsSlot->SetPadding(FMargin(0.0f));
		}

		ConfigureTextBlockLeft(WindowModeLabelText, FText::FromString(TEXT("\uD654\uBA74 \uBAA8\uB4DC")), FLinearColor(0.72f, 0.80f, 0.78f, 1.0f), 15);
		UVerticalBoxSlot* WindowModeLabelSlot = GraphicsSettingsPanel->AddChildToVerticalBox(WindowModeLabelText);
		if (WindowModeLabelSlot)
		{
			WindowModeLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
		ConfigurePlainButton(WindowedModeButtonBox, WindowedModeButton, WindowedModeButtonText, FText::FromString(TEXT("\uCC3D\uBAA8\uB4DC")), FVector2D(160.0f, 48.0f), false);
		ConfigurePlainButton(BorderlessWindowModeButtonBox, BorderlessWindowModeButton, BorderlessWindowModeButtonText, FText::FromString(TEXT("\uD14C\uB450\uB9AC \uC5C6\uB294 \uCC3D\uBAA8\uB4DC")), FVector2D(252.0f, 48.0f), false);
		ConfigurePlainButton(FullscreenModeButtonBox, FullscreenModeButton, FullscreenModeButtonText, FText::FromString(TEXT("\uC804\uCCB4\uD654\uBA74\uBAA8\uB4DC")), FVector2D(190.0f, 48.0f), false);
		for (UWidget* WindowModeButtonBox : {
				static_cast<UWidget*>(WindowedModeButtonBox),
				static_cast<UWidget*>(BorderlessWindowModeButtonBox),
				static_cast<UWidget*>(FullscreenModeButtonBox) })
		{
			UHorizontalBoxSlot* ModeSlot = WindowModeRow->AddChildToHorizontalBox(WindowModeButtonBox);
			if (ModeSlot)
			{
				ModeSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			}
		}
		UVerticalBoxSlot* WindowModeSlot = GraphicsSettingsPanel->AddChildToVerticalBox(WindowModeRow);
		if (WindowModeSlot)
		{
			WindowModeSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
		}

		ConfigureTextBlockLeft(ResolutionLabelText, FText::FromString(TEXT("\uD574\uC0C1\uB3C4")), FLinearColor(0.72f, 0.80f, 0.78f, 1.0f), 15);
		UVerticalBoxSlot* ResolutionLabelSlot = GraphicsSettingsPanel->AddChildToVerticalBox(ResolutionLabelText);
		if (ResolutionLabelSlot)
		{
			ResolutionLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
		ConfigurePlainButton(Resolution1280ButtonBox, Resolution1280Button, Resolution1280ButtonText, FText::FromString(TEXT("1280 x 720")), FVector2D(940.0f, 40.0f), false);
		ConfigurePlainButton(Resolution1600ButtonBox, Resolution1600Button, Resolution1600ButtonText, FText::FromString(TEXT("1600 x 900")), FVector2D(940.0f, 40.0f), false);
		ConfigurePlainButton(Resolution1920ButtonBox, Resolution1920Button, Resolution1920ButtonText, FText::FromString(TEXT("1920 x 1080")), FVector2D(940.0f, 40.0f), false);
		ConfigurePlainButton(Resolution2560ButtonBox, Resolution2560Button, Resolution2560ButtonText, FText::FromString(TEXT("2560 x 1440")), FVector2D(940.0f, 40.0f), false);
		ConfigurePlainButton(Resolution3840ButtonBox, Resolution3840Button, Resolution3840ButtonText, FText::FromString(TEXT("3840 x 2160")), FVector2D(940.0f, 40.0f), false);
		for (UWidget* ResolutionButtonBox : {
				static_cast<UWidget*>(Resolution1280ButtonBox),
				static_cast<UWidget*>(Resolution1600ButtonBox),
				static_cast<UWidget*>(Resolution1920ButtonBox),
				static_cast<UWidget*>(Resolution2560ButtonBox),
				static_cast<UWidget*>(Resolution3840ButtonBox) })
		{
			UVerticalBoxSlot* ResolutionSlot = ResolutionButtonStack->AddChildToVerticalBox(ResolutionButtonBox);
			if (ResolutionSlot)
			{
				ResolutionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
			}
		}
		UVerticalBoxSlot* ResolutionStackSlot = GraphicsSettingsPanel->AddChildToVerticalBox(ResolutionButtonStack);
		if (ResolutionStackSlot)
		{
			ResolutionStackSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
		}

		ConfigureTextBlockLeft(DLSSLabelText, FText::FromString(TEXT("DLSS")), FLinearColor(0.72f, 0.80f, 0.78f, 1.0f), 15);
		UVerticalBoxSlot* DLSSLabelSlot = GraphicsSettingsPanel->AddChildToVerticalBox(DLSSLabelText);
		if (DLSSLabelSlot)
		{
			DLSSLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 6.0f));
		}
		ConfigurePlainButton(DLSSOffButtonBox, DLSSOffButton, DLSSOffButtonText, FText::FromString(TEXT("\uB044\uAE30")), FVector2D(142.0f, 44.0f), false);
		ConfigurePlainButton(DLSSQualityButtonBox, DLSSQualityButton, DLSSQualityButtonText, FText::FromString(TEXT("\uD488\uC9C8")), FVector2D(142.0f, 44.0f), false);
		ConfigurePlainButton(DLSSBalancedButtonBox, DLSSBalancedButton, DLSSBalancedButtonText, FText::FromString(TEXT("\uADE0\uD615")), FVector2D(142.0f, 44.0f), false);
		ConfigurePlainButton(DLSSPerformanceButtonBox, DLSSPerformanceButton, DLSSPerformanceButtonText, FText::FromString(TEXT("\uC131\uB2A5")), FVector2D(142.0f, 44.0f), false);
		for (UWidget* DLSSButtonBox : {
				static_cast<UWidget*>(DLSSOffButtonBox),
				static_cast<UWidget*>(DLSSQualityButtonBox),
				static_cast<UWidget*>(DLSSBalancedButtonBox),
				static_cast<UWidget*>(DLSSPerformanceButtonBox) })
		{
			UHorizontalBoxSlot* DLSSSlot = DLSSButtonRow->AddChildToHorizontalBox(DLSSButtonBox);
			if (DLSSSlot)
			{
				DLSSSlot->SetPadding(FMargin(0.0f, 0.0f, 10.0f, 0.0f));
			}
		}
		UVerticalBoxSlot* DLSSRowSlot = GraphicsSettingsPanel->AddChildToVerticalBox(DLSSButtonRow);
		if (DLSSRowSlot)
		{
			DLSSRowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
		}

		InterfaceSettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
		UVerticalBoxSlot* InterfaceSettingsSlot = SettingsContentStack->AddChildToVerticalBox(InterfaceSettingsPanel);
		if (InterfaceSettingsSlot)
		{
			InterfaceSettingsSlot->SetPadding(FMargin(0.0f));
		}

		ConfigureTextBlockLeft(LanguageLabelText, FText::FromString(TEXT("\uC5B8\uC5B4")), FLinearColor(0.72f, 0.80f, 0.78f, 1.0f), 15);
		UVerticalBoxSlot* LanguageLabelSlot = InterfaceSettingsPanel->AddChildToVerticalBox(LanguageLabelText);
		if (LanguageLabelSlot)
		{
			LanguageLabelSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		ConfigurePlainButton(LanguageEnglishButtonBox, LanguageEnglishButton, LanguageEnglishButtonText, FText::FromString(TEXT("[x] English")), FVector2D(940.0f, 48.0f), false);
		ConfigurePlainButton(LanguageKoreanButtonBox, LanguageKoreanButton, LanguageKoreanButtonText, FText::FromString(TEXT("[ ] \uD55C\uAD6D\uC5B4")), FVector2D(940.0f, 48.0f), false);
		ConfigurePlainButton(LanguageJapaneseButtonBox, LanguageJapaneseButton, LanguageJapaneseButtonText, FText::FromString(TEXT("[ ] \u65E5\u672C\u8A9E")), FVector2D(940.0f, 48.0f), false);
		for (UWidget* LanguageButtonBox : {
				static_cast<UWidget*>(LanguageEnglishButtonBox),
				static_cast<UWidget*>(LanguageKoreanButtonBox),
				static_cast<UWidget*>(LanguageJapaneseButtonBox) })
		{
			UVerticalBoxSlot* LanguageButtonSlot = LanguageButtonStack->AddChildToVerticalBox(LanguageButtonBox);
			if (LanguageButtonSlot)
			{
				LanguageButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
			}
		}
		UVerticalBoxSlot* LanguageStackSlot = InterfaceSettingsPanel->AddChildToVerticalBox(LanguageButtonStack);
		if (LanguageStackSlot)
		{
			LanguageStackSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
		}

		ConfigurePlainButton(ConfirmInterfaceSettingsButtonBox, ConfirmInterfaceSettingsButton, ConfirmInterfaceSettingsButtonText, FText::FromString(TEXT("\uACB0\uC815")), FVector2D(180.0f, 50.0f), true);
		ConfigurePlainButton(CancelInterfaceSettingsButtonBox, CancelInterfaceSettingsButton, CancelInterfaceSettingsButtonText, FText::FromString(TEXT("\uCDE8\uC18C")), FVector2D(180.0f, 50.0f), false);
		UHorizontalBoxSlot* ConfirmInterfaceSlot = InterfaceActionButtonRow->AddChildToHorizontalBox(ConfirmInterfaceSettingsButtonBox);
		if (ConfirmInterfaceSlot)
		{
			ConfirmInterfaceSlot->SetPadding(FMargin(0.0f, 0.0f, 12.0f, 0.0f));
		}
		InterfaceActionButtonRow->AddChildToHorizontalBox(CancelInterfaceSettingsButtonBox);
		UVerticalBoxSlot* InterfaceActionSlot = InterfaceSettingsPanel->AddChildToVerticalBox(InterfaceActionButtonRow);
		if (InterfaceActionSlot)
		{
			InterfaceActionSlot->SetHorizontalAlignment(HAlign_Left);
		}

		CreditsPanel->SetVisibility(ESlateVisibility::Collapsed);
		FillCanvas(RootCanvas->AddChildToCanvas(CreditsPanel));
		CreditsBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(1920.0f, 1080.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.46f),
			FLinearColor::Transparent,
			0.0f,
			0.0f));
		FillCanvas(CreditsPanel->AddChildToCanvas(CreditsBackdrop));

		CreditsContentStack->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* LegacyCreditsStackSlot = CreditsPanel->AddChildToCanvas(CreditsContentStack);
		if (LegacyCreditsStackSlot)
		{
			LegacyCreditsStackSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			LegacyCreditsStackSlot->SetAlignment(FVector2D::ZeroVector);
			LegacyCreditsStackSlot->SetPosition(FVector2D::ZeroVector);
			LegacyCreditsStackSlot->SetSize(FVector2D::ZeroVector);
		}

		ConfigurePlainButton(BackFromCreditsButtonBox, BackFromCreditsButton, BackFromCreditsButtonText, FText::FromString(TEXT("\uB3CC\uC544\uAC00\uAE30")), FVector2D(380.0f, 52.0f), false);
		UCanvasPanelSlot* BackFromCreditsSlot = CreditsPanel->AddChildToCanvas(BackFromCreditsButtonBox);
		if (BackFromCreditsSlot)
		{
			BackFromCreditsSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			BackFromCreditsSlot->SetAlignment(FVector2D::ZeroVector);
			BackFromCreditsSlot->SetPosition(FVector2D(92.0f, 310.0f));
			BackFromCreditsSlot->SetSize(FVector2D(380.0f, 52.0f));
		}

		ConfigureTextBlockLeft(CreditsTitleText, FText::FromString(TEXT("\uD06C\uB808\uB527")), FLinearColor::White, 30);
		UCanvasPanelSlot* CreditsTitleSlot = CreditsPanel->AddChildToCanvas(CreditsTitleText);
		if (CreditsTitleSlot)
		{
			CreditsTitleSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CreditsTitleSlot->SetAlignment(FVector2D::ZeroVector);
			CreditsTitleSlot->SetPosition(FVector2D(560.0f, 116.0f));
			CreditsTitleSlot->SetSize(FVector2D(1180.0f, 42.0f));
		}

		auto ConfigureCreditsColumn = [](USizeBox* Frame, UScrollBox* ScrollBox, UTextBlock* TextBlock, const FText& PreviewText)
		{
			Frame->SetWidthOverride(380.0f);
			Frame->SetHeightOverride(700.0f);
			Frame->SetContent(ScrollBox);
			ConfigureTextBlock(
				TextBlock,
				PreviewText,
				FLinearColor(0.90f, 0.94f, 0.92f, 1.0f),
				17);
			TextBlock->SetAutoWrapText(true);
			TextBlock->SetShadowOffset(FVector2D(1.0f, 1.0f));
			TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
			ScrollBox->AddChild(TextBlock);
		};

		ConfigureCreditsColumn(CreditsScrollBoxFrame, CreditsScrollBox, CreditsText, FText::FromString(TEXT("Tuna Sweeper\n\nBlenG")));
		ConfigureCreditsColumn(CreditsScrollBoxFrame2, CreditsScrollBox2, CreditsText2, FText::FromString(TEXT("BlenG")));
		ConfigureCreditsColumn(CreditsScrollBoxFrame3, CreditsScrollBox3, CreditsText3, FText::FromString(TEXT("BlenG")));

		for (UWidget* CreditsColumn : {
				static_cast<UWidget*>(CreditsScrollBoxFrame),
				static_cast<UWidget*>(CreditsScrollBoxFrame2),
				static_cast<UWidget*>(CreditsScrollBoxFrame3) })
		{
			UHorizontalBoxSlot* ColumnSlot = CreditsColumnRow->AddChildToHorizontalBox(CreditsColumn);
			if (ColumnSlot)
			{
				ColumnSlot->SetHorizontalAlignment(HAlign_Fill);
				ColumnSlot->SetVerticalAlignment(VAlign_Fill);
				ColumnSlot->SetPadding(FMargin(0.0f, 0.0f, 32.0f, 0.0f));
				ColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		}
		UCanvasPanelSlot* CreditsColumnSlot = CreditsPanel->AddChildToCanvas(CreditsColumnRow);
		if (CreditsColumnSlot)
		{
			CreditsColumnSlot->SetAnchors(FAnchors(0.0f, 0.0f));
			CreditsColumnSlot->SetAlignment(FVector2D::ZeroVector);
			CreditsColumnSlot->SetPosition(FVector2D(560.0f, 176.0f));
			CreditsColumnSlot->SetSize(FVector2D(1240.0f, 720.0f));
		}

		ConfigureTextBlock(VersionText, FText::FromString(TEXT("v0.1")), FLinearColor(1.0f, 1.0f, 1.0f, 0.86f), 14);
		UCanvasPanelSlot* VersionSlot = RootCanvas->AddChildToCanvas(VersionText);
		if (VersionSlot)
		{
			VersionSlot->SetAnchors(FAnchors(1.0f, 1.0f));
			VersionSlot->SetAlignment(FVector2D(1.0f, 1.0f));
			VersionSlot->SetPosition(FVector2D(-28.0f, -22.0f));
			VersionSlot->SetSize(FVector2D(80.0f, 24.0f));
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildLevelTransitionVideoWidgetTree(UWidgetBlueprint* WidgetBlueprint)
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
		UImage* VideoImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("VideoImage"));
		UBorder* LetterboxTopPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LetterboxTopPanel"));
		UBorder* LetterboxBottomPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LetterboxBottomPanel"));
		UBorder* MessageBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MessageBackground"));
		UTextBlock* TransitionMessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TransitionMessageText"));
		UBorder* BlackFadePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BlackFadePanel"));

		if (!RootCanvas || !VideoImage || !LetterboxTopPanel || !LetterboxBottomPanel || !MessageBackground || !TransitionMessageText || !BlackFadePanel)
		{
			return false;
		}

		auto FillCanvas = [](UCanvasPanelSlot* Slot)
		{
			if (!Slot)
			{
				return;
			}

			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetAlignment(FVector2D(0.0f, 0.0f));
		};

		auto ConfigureLetterboxSlot = [](UCanvasPanelSlot* Slot, bool bTop)
		{
			if (!Slot)
			{
				return;
			}

			Slot->SetAnchors(bTop
				? FAnchors(0.0f, 0.0f, 1.0f, 0.10f)
				: FAnchors(0.0f, 0.90f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetAlignment(FVector2D(0.0f, 0.0f));
			Slot->SetZOrder(10);
		};

		FSlateBrush VideoBrush;
		VideoBrush.DrawAs = ESlateBrushDrawType::Image;
		VideoBrush.TintColor = FSlateColor(FLinearColor::White);
		VideoBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));

		FSlateBrush BlackBrush;
		BlackBrush.DrawAs = ESlateBrushDrawType::Box;
		BlackBrush.TintColor = FSlateColor(FLinearColor::Black);
		BlackBrush.SetImageSize(FVector2D(1920.0f, 1080.0f));

		WidgetTree->RootWidget = RootCanvas;
		VideoImage->SetBrush(VideoBrush);
		VideoImage->SetVisibility(ESlateVisibility::Collapsed);
		if (UCanvasPanelSlot* VideoSlot = RootCanvas->AddChildToCanvas(VideoImage))
		{
			FillCanvas(VideoSlot);
			VideoSlot->SetZOrder(0);
		}

		LetterboxTopPanel->SetBrush(BlackBrush);
		LetterboxTopPanel->SetVisibility(ESlateVisibility::Collapsed);
		ConfigureLetterboxSlot(RootCanvas->AddChildToCanvas(LetterboxTopPanel), true);

		LetterboxBottomPanel->SetBrush(BlackBrush);
		LetterboxBottomPanel->SetVisibility(ESlateVisibility::Collapsed);
		ConfigureLetterboxSlot(RootCanvas->AddChildToCanvas(LetterboxBottomPanel), false);

		MessageBackground->SetPadding(FMargin(18.0f, 7.0f));
		MessageBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(520.0f, 48.0f),
			FLinearColor(0.015f, 0.018f, 0.022f, 0.72f),
			FLinearColor(0.65f, 0.72f, 0.78f, 0.45f),
			1.0f));
		MessageBackground->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ConfigureTextBlock(TransitionMessageText, FText::GetEmpty(), FLinearColor(0.88f, 0.92f, 0.95f, 1.0f), 18);
		MessageBackground->SetContent(TransitionMessageText);

		UCanvasPanelSlot* MessageSlot = RootCanvas->AddChildToCanvas(MessageBackground);
		if (MessageSlot)
		{
			MessageSlot->SetAnchors(FAnchors(0.5f, 1.0f));
			MessageSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			MessageSlot->SetPosition(FVector2D(0.0f, -58.0f));
			MessageSlot->SetSize(FVector2D(520.0f, 48.0f));
			MessageSlot->SetZOrder(20);
		}

		BlackFadePanel->SetBrush(BlackBrush);
		BlackFadePanel->SetRenderOpacity(0.0f);
		BlackFadePanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UCanvasPanelSlot* FadeSlot = RootCanvas->AddChildToCanvas(BlackFadePanel))
		{
			FillCanvas(FadeSlot);
			FadeSlot->SetZOrder(30);
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildSpeechBubbleWidgetTree(UWidgetBlueprint* WidgetBlueprint)
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
		UBorder* BubbleTail = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BubbleTail"));
		UBorder* BubbleBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BubbleBackground"));
		UTextBlock* BubbleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BubbleText"));

		if (!RootCanvas || !BubbleTail || !BubbleBackground || !BubbleText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		const FLinearColor FillColor(0.96f, 0.98f, 1.0f, 0.96f);
		const FLinearColor OutlineColor(0.08f, 0.10f, 0.12f, 0.92f);
		BubbleTail->SetBrush(MakeRoundedBoxBrush(FVector2D(18.0f, 18.0f), FillColor, OutlineColor, 1.0f));
		BubbleTail->SetRenderTransformAngle(45.0f);

		UCanvasPanelSlot* TailSlot = RootCanvas->AddChildToCanvas(BubbleTail);
		if (TailSlot)
		{
			TailSlot->SetAnchors(FAnchors(0.5f, 0.0f));
			TailSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			TailSlot->SetPosition(FVector2D(0.0f, 40.0f));
			TailSlot->SetSize(FVector2D(18.0f, 18.0f));
		}

		BubbleBackground->SetPadding(FMargin(12.0f, 6.0f));
		BubbleBackground->SetBrush(MakeRoundedBoxBrush(FVector2D(160.0f, 48.0f), FillColor, OutlineColor, 1.5f));
		ConfigureTextBlock(BubbleText, FText::FromString(TEXT("3")), FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 28);
		BubbleBackground->SetContent(BubbleText);

		UCanvasPanelSlot* BackgroundSlot = RootCanvas->AddChildToCanvas(BubbleBackground);
		if (BackgroundSlot)
		{
			BackgroundSlot->SetAnchors(FAnchors(0.5f, 0.0f));
			BackgroundSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			BackgroundSlot->SetPosition(FVector2D(0.0f, 0.0f));
			BackgroundSlot->SetSize(FVector2D(160.0f, 48.0f));
		}

		RegisterWidgetVariable(WidgetBlueprint, BubbleText);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildQuestWidgetTree(UWidgetBlueprint* WidgetBlueprint)
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
		UBorder* PanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBackground"));
		UVerticalBox* PanelStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PanelStack"));
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
		UTextBlock* QuestTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestTitleText"));
		UButton* QuestCloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestCloseButton"));
		UTextBlock* QuestCloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestCloseButtonText"));
		UTextBlock* QuestDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestDescriptionText"));
		UTextBlock* QuestObjectiveText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestObjectiveText"));
		UTextBlock* QuestRewardText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestRewardText"));
		UTextBlock* QuestStateText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestStateText"));
		USizeBox* QuestPrimaryButtonBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuestPrimaryButtonBox"));
		UButton* QuestPrimaryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestPrimaryButton"));
		UTextBlock* QuestPrimaryButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("QuestPrimaryButtonText"));

		if (!RootCanvas || !PanelBackground || !PanelStack || !HeaderRow || !QuestTitleText || !QuestCloseButton ||
			!QuestCloseButtonText || !QuestDescriptionText || !QuestObjectiveText || !QuestRewardText ||
			!QuestStateText || !QuestPrimaryButtonBox || !QuestPrimaryButton || !QuestPrimaryButtonText)
		{
			return false;
		}

		auto ConfigureQuestButton = [](UButton* Button, const FVector2D& ButtonSize, const FLinearColor& FillColor, const FLinearColor& HoveredColor)
		{
			if (!Button)
			{
				return;
			}

			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(ButtonSize, FillColor, FLinearColor(0.64f, 0.72f, 0.76f, 0.90f), 1.2f));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(ButtonSize, HoveredColor, FLinearColor(0.92f, 0.96f, 1.0f, 0.96f), 1.8f));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(ButtonSize, FillColor * 0.78f, FLinearColor(0.56f, 0.64f, 0.70f, 1.0f), 1.0f));
			ButtonStyle.SetNormalPadding(FMargin(10.0f, 4.0f));
			ButtonStyle.SetPressedPadding(FMargin(10.0f, 5.0f, 10.0f, 3.0f));
			Button->SetStyle(ButtonStyle);
			Button->SetClickMethod(EButtonClickMethod::DownAndUp);
		};

		WidgetTree->RootWidget = RootCanvas;

		UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBackground);
		if (PanelSlot)
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetPosition(FVector2D(0.0f, 0.0f));
			PanelSlot->SetSize(FVector2D(560.0f, 380.0f));
		}

		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(560.0f, 380.0f),
			FLinearColor(0.055f, 0.065f, 0.075f, 0.96f),
			FLinearColor(0.40f, 0.48f, 0.54f, 0.85f),
			1.5f));
		PanelBackground->SetPadding(FMargin(24.0f, 20.0f));
		PanelBackground->SetContent(PanelStack);

		UVerticalBoxSlot* HeaderSlot = PanelStack->AddChildToVerticalBox(HeaderRow);
		if (HeaderSlot)
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}

		ConfigureTextBlockLeft(QuestTitleText, FText::FromString(TEXT("\uCCAB \uC678\uCD9C")), FLinearColor::White, 28);
		UHorizontalBoxSlot* TitleSlot = HeaderRow->AddChildToHorizontalBox(QuestTitleText);
		if (TitleSlot)
		{
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureQuestButton(
			QuestCloseButton,
			FVector2D(78.0f, 38.0f),
			FLinearColor(0.12f, 0.14f, 0.16f, 0.94f),
			FLinearColor(0.18f, 0.21f, 0.24f, 0.98f));
		ConfigureTextBlock(QuestCloseButtonText, FText::FromString(TEXT("\uB2EB\uAE30")), FLinearColor(0.90f, 0.94f, 0.96f, 1.0f), 15);
		QuestCloseButton->SetContent(QuestCloseButtonText);
		UHorizontalBoxSlot* CloseSlot = HeaderRow->AddChildToHorizontalBox(QuestCloseButton);
		if (CloseSlot)
		{
			CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			CloseSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlockLeft(QuestDescriptionText, FText::FromString(TEXT("\uC774\uC81C \uB4E4\uC5B4\uC654\uC73C\uB2C8 \uB098\uAC00\uC11C \uD55C\uBC88 \uC0B0\uCC45\uD558\uACE0 \uB4E4\uC5B4\uC640")), FLinearColor(0.83f, 0.88f, 0.91f, 1.0f), 18);
		QuestDescriptionText->SetAutoWrapText(true);
		UVerticalBoxSlot* DescriptionSlot = PanelStack->AddChildToVerticalBox(QuestDescriptionText);
		if (DescriptionSlot)
		{
			DescriptionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 26.0f));
		}

		ConfigureTextBlockLeft(QuestObjectiveText, FText::FromString(TEXT("\uBAA9\uD45C: \uBC99\uCEE4 \uBC16\uC73C\uB85C \uC774\uB3D9")), FLinearColor(0.97f, 0.91f, 0.72f, 1.0f), 18);
		UVerticalBoxSlot* ObjectiveSlot = PanelStack->AddChildToVerticalBox(QuestObjectiveText);
		if (ObjectiveSlot)
		{
			ObjectiveSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		ConfigureTextBlockLeft(QuestRewardText, FText::FromString(TEXT("\uBCF4\uC0C1: \uCF54\uC778 100")), FLinearColor(0.95f, 0.78f, 0.36f, 1.0f), 17);
		UVerticalBoxSlot* RewardSlot = PanelStack->AddChildToVerticalBox(QuestRewardText);
		if (RewardSlot)
		{
			RewardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
		}

		ConfigureTextBlockLeft(QuestStateText, FText::FromString(TEXT("\uC0C1\uD0DC: \uBC1B\uAE30 \uAC00\uB2A5")), FLinearColor(0.72f, 0.80f, 0.86f, 1.0f), 16);
		UVerticalBoxSlot* StateSlot = PanelStack->AddChildToVerticalBox(QuestStateText);
		if (StateSlot)
		{
			StateSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));
		}

		QuestPrimaryButtonBox->SetWidthOverride(220.0f);
		QuestPrimaryButtonBox->SetHeightOverride(54.0f);
		QuestPrimaryButtonBox->SetContent(QuestPrimaryButton);
		ConfigureQuestButton(
			QuestPrimaryButton,
			FVector2D(220.0f, 54.0f),
			FLinearColor(0.14f, 0.27f, 0.22f, 0.96f),
			FLinearColor(0.18f, 0.37f, 0.30f, 0.98f));
		ConfigureTextBlock(QuestPrimaryButtonText, FText::FromString(TEXT("\uC218\uB77D")), FLinearColor::White, 20);
		QuestPrimaryButton->SetContent(QuestPrimaryButtonText);

		UVerticalBoxSlot* PrimaryButtonSlot = PanelStack->AddChildToVerticalBox(QuestPrimaryButtonBox);
		if (PrimaryButtonSlot)
		{
			PrimaryButtonSlot->SetHorizontalAlignment(HAlign_Right);
			PrimaryButtonSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	const TArray<FString>& GetItemIconAssetNames()
	{
		static const TArray<FString> IconAssetNames = {
			TEXT("T_UIIcon_Pistol"),
			TEXT("T_UIIcon_Rifle"),
			TEXT("T_UIIcon_Shotgun"),
			TEXT("T_UIIcon_PistolAmmo"),
			TEXT("T_UIIcon_RifleAmmo"),
			TEXT("T_UIIcon_ShotgunAmmo"),
			TEXT("T_UIIcon_RifleExtendedMagazine"),
			TEXT("T_UIIcon_RedDotOptic"),
			TEXT("T_UIIcon_CannedFood"),
			TEXT("T_UIIcon_CannedTuna"),
			TEXT("T_UIIcon_WaterBottle"),
			TEXT("T_UIIcon_EnergyBar"),
			TEXT("T_UIIcon_Bandage"),
			TEXT("T_UIIcon_FirstAidKit"),
			TEXT("T_UIIcon_Painkillers"),
			TEXT("T_UIIcon_Antibiotics"),
			TEXT("T_UIIcon_CombatKnife"),
			TEXT("T_UIIcon_BallisticHelmet"),
			TEXT("T_UIIcon_BodyArmor"),
			TEXT("T_UIIcon_TacticalSunglasses"),
			TEXT("T_UIIcon_TacticalEarphones_Tier1"),
			TEXT("T_UIIcon_TacticalEarphones_Tier2"),
			TEXT("T_UIIcon_TacticalJacket"),
			TEXT("T_UIIcon_Backpack"),
			TEXT("T_UIIcon_Backpack_Tier1"),
			TEXT("T_UIIcon_Backpack_Tier2"),
			TEXT("T_UIIcon_Backpack_Tier3"),
			TEXT("T_UIIcon_Backpack_Tier4"),
			TEXT("T_UIIcon_ValuablesCrate")
		};

		return IconAssetNames;
	}

	const TArray<FString>& GetEquipmentIconAssetNames()
	{
		static const TArray<FString> IconAssetNames = {
			TEXT("T_UIIcon_CombatKnife"),
			TEXT("T_UIIcon_BallisticHelmet"),
			TEXT("T_UIIcon_BodyArmor"),
			TEXT("T_UIIcon_TacticalSunglasses"),
			TEXT("T_UIIcon_TacticalEarphones_Tier1"),
			TEXT("T_UIIcon_TacticalEarphones_Tier2"),
			TEXT("T_UIIcon_TacticalJacket"),
			TEXT("T_UIIcon_RifleExtendedMagazine"),
			TEXT("T_UIIcon_RedDotOptic"),
			TEXT("T_UIIcon_Backpack_Tier1"),
			TEXT("T_UIIcon_Backpack_Tier2"),
			TEXT("T_UIIcon_Backpack_Tier3"),
			TEXT("T_UIIcon_Backpack_Tier4")
		};

		return IconAssetNames;
	}

	FString GetItemIconSourcePath(const FString& IconAssetName)
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

	void ConfigureImportedUiTexture(UTexture2D* Texture)
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

	void ConfigureImportedWorldTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->LODGroup = TEXTUREGROUP_World;
		Texture->SRGB = true;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
	}

	void ConfigureImportedEffectTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->LODGroup = TEXTUREGROUP_Effects;
		Texture->SRGB = true;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
	}

	void ConfigureImportedMaskTexture(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return;
		}

		Texture->Modify();
		Texture->CompressionSettings = TC_Masks;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->LODGroup = TEXTUREGROUP_Effects;
		Texture->SRGB = false;
		Texture->UpdateResource();
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		SaveAsset(Texture);
	}

	bool ImportWorldTexture(
		const FString& InSourceFile,
		const FString& DestinationPath,
		const FString& AssetName,
		UTexture2D** OutTexture = nullptr)
	{
		if (OutTexture)
		{
			*OutTexture = nullptr;
		}

		FString SourceFile = InSourceFile;
		FPaths::NormalizeFilename(SourceFile);
		SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
		FPaths::CollapseRelativeDirectories(SourceFile);

		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing world texture source: %s"), *SourceFile);
			return false;
		}

		if (AssetName.IsEmpty() || !FPackageName::IsValidLongPackageName(DestinationPath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Invalid world texture destination: path=%s asset=%s"), *DestinationPath, *AssetName);
			return false;
		}

		const FString ObjectPath = GetAssetObjectPath(DestinationPath, AssetName);
		FString ImportFile = SourceFile;
		if (FPaths::GetBaseFilename(SourceFile) != AssetName)
		{
			const FString ImportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TunaSweeperWorldTextureImport"));
			IFileManager::Get().MakeDirectory(*ImportDirectory, true);
			ImportFile = FPaths::Combine(ImportDirectory, AssetName + TEXT(".") + FPaths::GetExtension(SourceFile));
			if (IFileManager::Get().Copy(*ImportFile, *SourceFile, true, true) != COPY_OK)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to stage world texture import source: %s -> %s"), *SourceFile, *ImportFile);
				return false;
			}
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(ImportFile);
		ImportData->bReplaceExisting = true;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import world texture: %s"), *ImportFile);
			return false;
		}

		UTexture2D* ImportedTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!ImportedTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported world texture: %s"), *ObjectPath);
			return false;
		}

		ConfigureImportedWorldTexture(ImportedTexture);
		if (OutTexture)
		{
			*OutTexture = ImportedTexture;
		}
		return true;
	}

	FString GetRollingBomberChargeCylinderMaskSourcePath()
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT(".."),
			TEXT("GeneratedImages"),
			TEXT("Effects"),
			TEXT("T_RollingBomberChargeCylinderMask.png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	void AddRollingBomberChargeCylinderQuad(
		FMeshDescription& MeshDescription,
		FStaticMeshAttributes& Attributes,
		FPolygonGroupID PolygonGroupId,
		float Angle0,
		float Angle1,
		float U0,
		float U1)
	{
		TVertexAttributesRef<FVector3f> VertexPositions = Attributes.GetVertexPositions();
		TVertexInstanceAttributesRef<FVector3f> VertexInstanceNormals = Attributes.GetVertexInstanceNormals();
		TVertexInstanceAttributesRef<FVector2f> VertexInstanceUVs = Attributes.GetVertexInstanceUVs();

		constexpr float HalfLength = 50.0f;
		constexpr float Radius = 50.0f;
		const FVector3f Radial0(0.0f, FMath::Cos(Angle0), FMath::Sin(Angle0));
		const FVector3f Radial1(0.0f, FMath::Cos(Angle1), FMath::Sin(Angle1));
		const FVector3f Positions[] = {
			FVector3f(-HalfLength, Radial0.Y * Radius, Radial0.Z * Radius),
			FVector3f(-HalfLength, Radial1.Y * Radius, Radial1.Z * Radius),
			FVector3f(HalfLength, Radial1.Y * Radius, Radial1.Z * Radius),
			FVector3f(HalfLength, Radial0.Y * Radius, Radial0.Z * Radius)
		};
		const FVector3f Normals[] = { Radial0, Radial1, Radial1, Radial0 };
		const FVector2f UVs[] = {
			FVector2f(U0, 0.0f),
			FVector2f(U1, 0.0f),
			FVector2f(U1, 1.0f),
			FVector2f(U0, 1.0f)
		};

		TArray<FVertexInstanceID> VertexInstances;
		VertexInstances.Reserve(UE_ARRAY_COUNT(Positions));
		for (int32 Index = 0; Index < UE_ARRAY_COUNT(Positions); ++Index)
		{
			const FVertexID VertexId = MeshDescription.CreateVertex();
			VertexPositions[VertexId] = Positions[Index];

			const FVertexInstanceID VertexInstanceId = MeshDescription.CreateVertexInstance(VertexId);
			VertexInstanceNormals[VertexInstanceId] = Normals[Index];
			VertexInstanceUVs.Set(VertexInstanceId, 0, UVs[Index]);
			VertexInstances.Add(VertexInstanceId);
		}

		MeshDescription.CreatePolygon(PolygonGroupId, VertexInstances);
	}

	void BuildRollingBomberChargeCylinderMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		Attributes.GetVertexInstanceUVs().SetNumChannels(1);

		const FPolygonGroupID PolygonGroupId = MeshDescription.CreatePolygonGroup();
		Attributes.GetPolygonGroupMaterialSlotNames()[PolygonGroupId] = FName(TEXT("ChargeCylinder"));

		constexpr int32 SegmentCount = 32;
		for (int32 SegmentIndex = 0; SegmentIndex < SegmentCount; ++SegmentIndex)
		{
			const float U0 = static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount);
			const float U1 = static_cast<float>(SegmentIndex + 1) / static_cast<float>(SegmentCount);
			const float Angle0 = U0 * 2.0f * UE_PI;
			const float Angle1 = U1 * 2.0f * UE_PI;
			AddRollingBomberChargeCylinderQuad(MeshDescription, Attributes, PolygonGroupId, Angle0, Angle1, U0, U1);
		}
	}

	UMaterial* EnsureRollingBomberChargeCylinderMaterial(UTexture2D* MaskTexture)
	{
		if (!MaskTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, RollingBomberChargeCylinderMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				RollingBomberChargeCylinderMaterialAssetName,
				EffectsAssetPath,
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
		Material->BlendMode = BLEND_Additive;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -900;
		TextureCoordinateExpression->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionPanner* PannerExpression = NewObject<UMaterialExpressionPanner>(Material);
		PannerExpression->Material = Material;
		PannerExpression->SpeedX = 2.8f;
		PannerExpression->SpeedY = 0.0f;
		PannerExpression->Coordinate.Connect(0, TextureCoordinateExpression);
		PannerExpression->MaterialExpressionEditorX = -680;
		PannerExpression->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(PannerExpression);

		UMaterialExpressionTextureSampleParameter2D* MaskSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		MaskSample->Material = Material;
		MaskSample->ParameterName = TEXT("MaskTexture");
		MaskSample->Texture = MaskTexture;
		MaskSample->SamplerType = SAMPLERTYPE_Masks;
		MaskSample->Coordinates.Connect(0, PannerExpression);
		MaskSample->MaterialExpressionEditorX = -440;
		MaskSample->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(MaskSample);

		UMaterialExpressionComponentMask* MaskRedChannel = NewObject<UMaterialExpressionComponentMask>(Material);
		MaskRedChannel->Material = Material;
		MaskRedChannel->Input.Connect(0, MaskSample);
		MaskRedChannel->R = 1;
		MaskRedChannel->G = 0;
		MaskRedChannel->B = 0;
		MaskRedChannel->A = 0;
		MaskRedChannel->MaterialExpressionEditorX = -180;
		MaskRedChannel->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(MaskRedChannel);

		UMaterialExpressionVectorParameter* ColorParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParameter->Material = Material;
		ColorParameter->ParameterName = TEXT("ChargeColor");
		ColorParameter->DefaultValue = FLinearColor(1.0f, 0.035f, 0.0f, 1.0f);
		ColorParameter->MaterialExpressionEditorX = -440;
		ColorParameter->MaterialExpressionEditorY = -180;
		Material->GetExpressionCollection().AddExpression(ColorParameter);

		UMaterialExpressionScalarParameter* IntensityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		IntensityParameter->Material = Material;
		IntensityParameter->ParameterName = TEXT("Intensity");
		IntensityParameter->DefaultValue = 7.0f;
		IntensityParameter->MaterialExpressionEditorX = -440;
		IntensityParameter->MaterialExpressionEditorY = -20;
		Material->GetExpressionCollection().AddExpression(IntensityParameter);

		UMaterialExpressionMultiply* ColorIntensityMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		ColorIntensityMultiply->Material = Material;
		ColorIntensityMultiply->A.Connect(0, ColorParameter);
		ColorIntensityMultiply->B.Connect(0, IntensityParameter);
		ColorIntensityMultiply->MaterialExpressionEditorX = -180;
		ColorIntensityMultiply->MaterialExpressionEditorY = -120;
		Material->GetExpressionCollection().AddExpression(ColorIntensityMultiply);

		UMaterialExpressionMultiply* EmissiveMaskMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveMaskMultiply->Material = Material;
		EmissiveMaskMultiply->A.Connect(0, ColorIntensityMultiply);
		EmissiveMaskMultiply->B.Connect(0, MaskRedChannel);
		EmissiveMaskMultiply->MaterialExpressionEditorX = 100;
		EmissiveMaskMultiply->MaterialExpressionEditorY = -80;
		Material->GetExpressionCollection().AddExpression(EmissiveMaskMultiply);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->Material = Material;
		OpacityParameter->ParameterName = TEXT("Opacity");
		OpacityParameter->DefaultValue = 0.7f;
		OpacityParameter->MaterialExpressionEditorX = -180;
		OpacityParameter->MaterialExpressionEditorY = 260;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		UMaterialExpressionMultiply* OpacityMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		OpacityMultiply->Material = Material;
		OpacityMultiply->A.Connect(0, MaskRedChannel);
		OpacityMultiply->B.Connect(0, OpacityParameter);
		OpacityMultiply->MaterialExpressionEditorX = 100;
		OpacityMultiply->MaterialExpressionEditorY = 180;
		Material->GetExpressionCollection().AddExpression(OpacityMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, ColorParameter);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveMaskMultiply);
		MaterialEditorOnly->Opacity.Connect(0, OpacityMultiply);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UStaticMesh* EnsureRollingBomberChargeCylinderMesh(UMaterialInterface* ChargeMaterial)
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, RollingBomberChargeCylinderMeshAssetName);
		UStaticMesh* StaticMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!StaticMesh)
		{
			const FString PackageName = FString::Printf(TEXT("%s/%s"), *EffectsAssetPath, *RollingBomberChargeCylinderMeshAssetName);
			UPackage* Package = CreatePackage(*PackageName);
			if (!Package)
			{
				return nullptr;
			}

			StaticMesh = NewObject<UStaticMesh>(
				Package,
				*RollingBomberChargeCylinderMeshAssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			if (!StaticMesh)
			{
				return nullptr;
			}

			FAssetRegistryModule::AssetCreated(StaticMesh);
		}

		StaticMesh->Modify();

		FMeshDescription MeshDescription;
		BuildRollingBomberChargeCylinderMeshDescription(MeshDescription);

		StaticMesh->GetStaticMaterials().Reset();
		StaticMesh->GetStaticMaterials().Add(FStaticMaterial(ChargeMaterial, FName(TEXT("ChargeCylinder"))));

		TArray<const FMeshDescription*> MeshDescriptions;
		MeshDescriptions.Add(&MeshDescription);
		StaticMesh->BuildFromMeshDescriptions(MeshDescriptions);
		StaticMesh->MarkPackageDirty();

		return SaveAsset(StaticMesh) ? StaticMesh : nullptr;
	}

	bool EnsureRollingBomberChargeCylinderEffectAssets()
	{
		UTexture2D* MaskTexture = nullptr;
		const FString SourcePath = GetRollingBomberChargeCylinderMaskSourcePath();
		if (FPaths::FileExists(SourcePath))
		{
			if (!ImportWorldTexture(SourcePath, EffectsAssetPath, RollingBomberChargeCylinderMaskTextureAssetName, &MaskTexture))
			{
				return false;
			}
		}
		else
		{
			MaskTexture = LoadObject<UTexture2D>(
				nullptr,
				*GetAssetObjectPath(EffectsAssetPath, RollingBomberChargeCylinderMaskTextureAssetName));
		}

		if (!MaskTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing RollingBomber charge cylinder mask source: %s"), *SourcePath);
			return false;
		}

		ConfigureImportedMaskTexture(MaskTexture);
		UMaterial* ChargeMaterial = EnsureRollingBomberChargeCylinderMaterial(MaskTexture);
		return ChargeMaterial && EnsureRollingBomberChargeCylinderMesh(ChargeMaterial);
	}

	FString GetLocalExplosionFlipbookSourcePath()
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT(".."),
			TEXT("GeneratedImages"),
			TEXT("Effects"),
			TEXT("T_LocalExplosionFlipbook.png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	UMaterial* EnsureLocalExplosionFlipbookMaterial(
		UTexture2D* FlipbookTexture,
		const FString& MaterialAssetName,
		bool bSmokeMaterial)
	{
		if (!FlipbookTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, MaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				MaterialAssetName,
				EffectsAssetPath,
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
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -1040;
		TextureCoordinateExpression->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionScalarParameter* FrameScaleParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		FrameScaleParameter->Material = Material;
		FrameScaleParameter->ParameterName = TEXT("FrameScale");
		FrameScaleParameter->DefaultValue = 0.25f;
		FrameScaleParameter->MaterialExpressionEditorX = -1040;
		FrameScaleParameter->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(FrameScaleParameter);

		UMaterialExpressionMultiply* ScaledUv = NewObject<UMaterialExpressionMultiply>(Material);
		ScaledUv->Material = Material;
		ScaledUv->A.Connect(0, TextureCoordinateExpression);
		ScaledUv->B.Connect(0, FrameScaleParameter);
		ScaledUv->MaterialExpressionEditorX = -800;
		ScaledUv->MaterialExpressionEditorY = 160;
		Material->GetExpressionCollection().AddExpression(ScaledUv);

		UMaterialExpressionScalarParameter* FrameUParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		FrameUParameter->Material = Material;
		FrameUParameter->ParameterName = TEXT("FrameU");
		FrameUParameter->DefaultValue = 0.0f;
		FrameUParameter->MaterialExpressionEditorX = -1040;
		FrameUParameter->MaterialExpressionEditorY = 480;
		Material->GetExpressionCollection().AddExpression(FrameUParameter);

		UMaterialExpressionScalarParameter* FrameVParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		FrameVParameter->Material = Material;
		FrameVParameter->ParameterName = TEXT("FrameV");
		FrameVParameter->DefaultValue = 0.0f;
		FrameVParameter->MaterialExpressionEditorX = -1040;
		FrameVParameter->MaterialExpressionEditorY = 640;
		Material->GetExpressionCollection().AddExpression(FrameVParameter);

		UMaterialExpressionAppendVector* FrameOffset = NewObject<UMaterialExpressionAppendVector>(Material);
		FrameOffset->Material = Material;
		FrameOffset->A.Connect(0, FrameUParameter);
		FrameOffset->B.Connect(0, FrameVParameter);
		FrameOffset->MaterialExpressionEditorX = -800;
		FrameOffset->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(FrameOffset);

		UMaterialExpressionAdd* FrameUv = NewObject<UMaterialExpressionAdd>(Material);
		FrameUv->Material = Material;
		FrameUv->A.Connect(0, ScaledUv);
		FrameUv->B.Connect(0, FrameOffset);
		FrameUv->MaterialExpressionEditorX = -560;
		FrameUv->MaterialExpressionEditorY = 260;
		Material->GetExpressionCollection().AddExpression(FrameUv);

		UMaterialExpressionTextureSampleParameter2D* FlipbookSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		FlipbookSample->Material = Material;
		FlipbookSample->ParameterName = TEXT("ExplosionTexture");
		FlipbookSample->Texture = FlipbookTexture;
		FlipbookSample->SamplerType = SAMPLERTYPE_Color;
		FlipbookSample->Coordinates.Connect(0, FrameUv);
		FlipbookSample->MaterialExpressionEditorX = -300;
		FlipbookSample->MaterialExpressionEditorY = 200;
		Material->GetExpressionCollection().AddExpression(FlipbookSample);

		UMaterialExpressionVectorParameter* TintColorParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		TintColorParameter->Material = Material;
		TintColorParameter->ParameterName = TEXT("TintColor");
		TintColorParameter->DefaultValue = bSmokeMaterial
			? FLinearColor(0.48f, 0.43f, 0.36f, 1.0f)
			: FLinearColor(1.0f, 0.72f, 0.42f, 1.0f);
		TintColorParameter->MaterialExpressionEditorX = -300;
		TintColorParameter->MaterialExpressionEditorY = -120;
		Material->GetExpressionCollection().AddExpression(TintColorParameter);

		UMaterialExpressionMultiply* TintedTexture = NewObject<UMaterialExpressionMultiply>(Material);
		TintedTexture->Material = Material;
		TintedTexture->A.Connect(0, FlipbookSample);
		TintedTexture->B.Connect(0, TintColorParameter);
		TintedTexture->MaterialExpressionEditorX = -20;
		TintedTexture->MaterialExpressionEditorY = 20;
		Material->GetExpressionCollection().AddExpression(TintedTexture);

		UMaterialExpressionScalarParameter* EmissiveStrengthParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		EmissiveStrengthParameter->Material = Material;
		EmissiveStrengthParameter->ParameterName = TEXT("EmissiveStrength");
		EmissiveStrengthParameter->DefaultValue = bSmokeMaterial ? 0.0f : 3.5f;
		EmissiveStrengthParameter->MaterialExpressionEditorX = -20;
		EmissiveStrengthParameter->MaterialExpressionEditorY = -200;
		Material->GetExpressionCollection().AddExpression(EmissiveStrengthParameter);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->Material = Material;
		OpacityParameter->ParameterName = TEXT("Opacity");
		OpacityParameter->DefaultValue = 1.0f;
		OpacityParameter->MaterialExpressionEditorX = -20;
		OpacityParameter->MaterialExpressionEditorY = 500;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		UMaterialExpressionMultiply* EmissiveStrengthMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveStrengthMultiply->Material = Material;
		EmissiveStrengthMultiply->A.Connect(0, TintedTexture);
		EmissiveStrengthMultiply->B.Connect(0, EmissiveStrengthParameter);
		EmissiveStrengthMultiply->MaterialExpressionEditorX = 220;
		EmissiveStrengthMultiply->MaterialExpressionEditorY = -80;
		Material->GetExpressionCollection().AddExpression(EmissiveStrengthMultiply);

		UMaterialExpressionMultiply* FinalEmissive = NewObject<UMaterialExpressionMultiply>(Material);
		FinalEmissive->Material = Material;
		FinalEmissive->A.Connect(0, EmissiveStrengthMultiply);
		FinalEmissive->B.Connect(0, OpacityParameter);
		FinalEmissive->MaterialExpressionEditorX = 460;
		FinalEmissive->MaterialExpressionEditorY = -60;
		Material->GetExpressionCollection().AddExpression(FinalEmissive);

		UMaterialExpressionComponentMask* RedChannel = NewObject<UMaterialExpressionComponentMask>(Material);
		RedChannel->Material = Material;
		RedChannel->Input.Connect(0, FlipbookSample);
		RedChannel->R = 1;
		RedChannel->G = 0;
		RedChannel->B = 0;
		RedChannel->A = 0;
		RedChannel->MaterialExpressionEditorX = -20;
		RedChannel->MaterialExpressionEditorY = 700;
		Material->GetExpressionCollection().AddExpression(RedChannel);

		UMaterialExpressionComponentMask* GreenChannel = NewObject<UMaterialExpressionComponentMask>(Material);
		GreenChannel->Material = Material;
		GreenChannel->Input.Connect(0, FlipbookSample);
		GreenChannel->R = 0;
		GreenChannel->G = 1;
		GreenChannel->B = 0;
		GreenChannel->A = 0;
		GreenChannel->MaterialExpressionEditorX = -20;
		GreenChannel->MaterialExpressionEditorY = 860;
		Material->GetExpressionCollection().AddExpression(GreenChannel);

		UMaterialExpressionComponentMask* BlueChannel = NewObject<UMaterialExpressionComponentMask>(Material);
		BlueChannel->Material = Material;
		BlueChannel->Input.Connect(0, FlipbookSample);
		BlueChannel->R = 0;
		BlueChannel->G = 0;
		BlueChannel->B = 1;
		BlueChannel->A = 0;
		BlueChannel->MaterialExpressionEditorX = -20;
		BlueChannel->MaterialExpressionEditorY = 1020;
		Material->GetExpressionCollection().AddExpression(BlueChannel);

		UMaterialExpressionAdd* RedGreenSum = NewObject<UMaterialExpressionAdd>(Material);
		RedGreenSum->Material = Material;
		RedGreenSum->A.Connect(0, RedChannel);
		RedGreenSum->B.Connect(0, GreenChannel);
		RedGreenSum->MaterialExpressionEditorX = 220;
		RedGreenSum->MaterialExpressionEditorY = 780;
		Material->GetExpressionCollection().AddExpression(RedGreenSum);

		UMaterialExpressionAdd* RgbSum = NewObject<UMaterialExpressionAdd>(Material);
		RgbSum->Material = Material;
		RgbSum->A.Connect(0, RedGreenSum);
		RgbSum->B.Connect(0, BlueChannel);
		RgbSum->MaterialExpressionEditorX = 460;
		RgbSum->MaterialExpressionEditorY = 860;
		Material->GetExpressionCollection().AddExpression(RgbSum);

		UMaterialExpressionMultiply* LuminanceScale = NewObject<UMaterialExpressionMultiply>(Material);
		LuminanceScale->Material = Material;
		LuminanceScale->A.Connect(0, RgbSum);
		LuminanceScale->ConstB = 0.42f;
		LuminanceScale->MaterialExpressionEditorX = 700;
		LuminanceScale->MaterialExpressionEditorY = 860;
		Material->GetExpressionCollection().AddExpression(LuminanceScale);

		UMaterialExpressionSaturate* LuminanceMask = NewObject<UMaterialExpressionSaturate>(Material);
		LuminanceMask->Material = Material;
		LuminanceMask->Input.Connect(0, LuminanceScale);
		LuminanceMask->MaterialExpressionEditorX = 940;
		LuminanceMask->MaterialExpressionEditorY = 860;
		Material->GetExpressionCollection().AddExpression(LuminanceMask);

		UMaterialExpressionScalarParameter* AlphaBoostParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		AlphaBoostParameter->Material = Material;
		AlphaBoostParameter->ParameterName = TEXT("AlphaBoost");
		AlphaBoostParameter->DefaultValue = bSmokeMaterial ? 2.65f : 1.0f;
		AlphaBoostParameter->MaterialExpressionEditorX = 940;
		AlphaBoostParameter->MaterialExpressionEditorY = 1040;
		Material->GetExpressionCollection().AddExpression(AlphaBoostParameter);

		UMaterialExpressionMultiply* BoostedLuminanceMask = NewObject<UMaterialExpressionMultiply>(Material);
		BoostedLuminanceMask->Material = Material;
		BoostedLuminanceMask->A.Connect(0, LuminanceMask);
		BoostedLuminanceMask->B.Connect(0, AlphaBoostParameter);
		BoostedLuminanceMask->MaterialExpressionEditorX = 1180;
		BoostedLuminanceMask->MaterialExpressionEditorY = 920;
		Material->GetExpressionCollection().AddExpression(BoostedLuminanceMask);

		UMaterialExpressionSaturate* BoostedOpacityMask = NewObject<UMaterialExpressionSaturate>(Material);
		BoostedOpacityMask->Material = Material;
		BoostedOpacityMask->Input.Connect(0, BoostedLuminanceMask);
		BoostedOpacityMask->MaterialExpressionEditorX = 1420;
		BoostedOpacityMask->MaterialExpressionEditorY = 920;
		Material->GetExpressionCollection().AddExpression(BoostedOpacityMask);

		UMaterialExpressionMultiply* FinalOpacity = NewObject<UMaterialExpressionMultiply>(Material);
		FinalOpacity->Material = Material;
		FinalOpacity->A.Connect(0, BoostedOpacityMask);
		FinalOpacity->B.Connect(0, OpacityParameter);
		FinalOpacity->MaterialExpressionEditorX = 1660;
		FinalOpacity->MaterialExpressionEditorY = 780;
		Material->GetExpressionCollection().AddExpression(FinalOpacity);

		MaterialEditorOnly->BaseColor.Connect(0, TintedTexture);
		MaterialEditorOnly->EmissiveColor.Connect(0, FinalEmissive);
		MaterialEditorOnly->Opacity.Connect(0, FinalOpacity);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UMaterial* EnsureLocalExplosionDistortionMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, LocalExplosionDistortionMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				LocalExplosionDistortionMaterialAssetName,
				EffectsAssetPath,
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
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->bTangentSpaceNormal = true;
		Material->RefractionMethod = RM_PixelNormalOffset;
		Material->RefractionDepthBias = 8.0f;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -1240;
		TextureCoordinateExpression->MaterialExpressionEditorY = 40;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionConstant2Vector* CenterVector = NewObject<UMaterialExpressionConstant2Vector>(Material);
		CenterVector->Material = Material;
		CenterVector->R = 0.5f;
		CenterVector->G = 0.5f;
		CenterVector->MaterialExpressionEditorX = -1240;
		CenterVector->MaterialExpressionEditorY = 240;
		Material->GetExpressionCollection().AddExpression(CenterVector);

		UMaterialExpressionSubtract* CenteredUv = NewObject<UMaterialExpressionSubtract>(Material);
		CenteredUv->Material = Material;
		CenteredUv->A.Connect(0, TextureCoordinateExpression);
		CenteredUv->B.Connect(0, CenterVector);
		CenteredUv->MaterialExpressionEditorX = -980;
		CenteredUv->MaterialExpressionEditorY = 100;
		Material->GetExpressionCollection().AddExpression(CenteredUv);

		UMaterialExpressionLength* RadialDistance = NewObject<UMaterialExpressionLength>(Material);
		RadialDistance->Material = Material;
		RadialDistance->Input.Connect(0, CenteredUv);
		RadialDistance->MaterialExpressionEditorX = -740;
		RadialDistance->MaterialExpressionEditorY = 100;
		Material->GetExpressionCollection().AddExpression(RadialDistance);

		UMaterialExpressionScalarParameter* WavePositionParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		WavePositionParameter->Material = Material;
		WavePositionParameter->ParameterName = TEXT("WavePosition");
		WavePositionParameter->DefaultValue = 0.22f;
		WavePositionParameter->MaterialExpressionEditorX = -740;
		WavePositionParameter->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(WavePositionParameter);

		UMaterialExpressionSubtract* RadiusFromWave = NewObject<UMaterialExpressionSubtract>(Material);
		RadiusFromWave->Material = Material;
		RadiusFromWave->A.Connect(0, RadialDistance);
		RadiusFromWave->B.Connect(0, WavePositionParameter);
		RadiusFromWave->MaterialExpressionEditorX = -500;
		RadiusFromWave->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(RadiusFromWave);

		UMaterialExpressionAbs* WaveDelta = NewObject<UMaterialExpressionAbs>(Material);
		WaveDelta->Material = Material;
		WaveDelta->Input.Connect(0, RadiusFromWave);
		WaveDelta->MaterialExpressionEditorX = -260;
		WaveDelta->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(WaveDelta);

		UMaterialExpressionScalarParameter* WaveWidthParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		WaveWidthParameter->Material = Material;
		WaveWidthParameter->ParameterName = TEXT("WaveWidth");
		WaveWidthParameter->DefaultValue = 0.15f;
		WaveWidthParameter->MaterialExpressionEditorX = -260;
		WaveWidthParameter->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(WaveWidthParameter);

		UMaterialExpressionDivide* NormalizedWaveDelta = NewObject<UMaterialExpressionDivide>(Material);
		NormalizedWaveDelta->Material = Material;
		NormalizedWaveDelta->A.Connect(0, WaveDelta);
		NormalizedWaveDelta->B.Connect(0, WaveWidthParameter);
		NormalizedWaveDelta->MaterialExpressionEditorX = -20;
		NormalizedWaveDelta->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(NormalizedWaveDelta);

		UMaterialExpressionOneMinus* WaveFalloff = NewObject<UMaterialExpressionOneMinus>(Material);
		WaveFalloff->Material = Material;
		WaveFalloff->Input.Connect(0, NormalizedWaveDelta);
		WaveFalloff->MaterialExpressionEditorX = 220;
		WaveFalloff->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(WaveFalloff);

		UMaterialExpressionSaturate* RingMask = NewObject<UMaterialExpressionSaturate>(Material);
		RingMask->Material = Material;
		RingMask->Input.Connect(0, WaveFalloff);
		RingMask->MaterialExpressionEditorX = 460;
		RingMask->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(RingMask);

		UMaterialExpressionScalarParameter* DistortionStrengthParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		DistortionStrengthParameter->Material = Material;
		DistortionStrengthParameter->ParameterName = TEXT("DistortionStrength");
		DistortionStrengthParameter->DefaultValue = 0.32f;
		DistortionStrengthParameter->MaterialExpressionEditorX = 220;
		DistortionStrengthParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(DistortionStrengthParameter);

		UMaterialExpressionMultiply* MaskedStrength = NewObject<UMaterialExpressionMultiply>(Material);
		MaskedStrength->Material = Material;
		MaskedStrength->A.Connect(0, RingMask);
		MaskedStrength->B.Connect(0, DistortionStrengthParameter);
		MaskedStrength->MaterialExpressionEditorX = 700;
		MaskedStrength->MaterialExpressionEditorY = 220;
		Material->GetExpressionCollection().AddExpression(MaskedStrength);

		UMaterialExpressionMultiply* DistortedNormalXY = NewObject<UMaterialExpressionMultiply>(Material);
		DistortedNormalXY->Material = Material;
		DistortedNormalXY->A.Connect(0, CenteredUv);
		DistortedNormalXY->B.Connect(0, MaskedStrength);
		DistortedNormalXY->MaterialExpressionEditorX = 940;
		DistortedNormalXY->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(DistortedNormalXY);

		UMaterialExpressionConstant* NormalZ = NewObject<UMaterialExpressionConstant>(Material);
		NormalZ->Material = Material;
		NormalZ->R = 1.0f;
		NormalZ->MaterialExpressionEditorX = 940;
		NormalZ->MaterialExpressionEditorY = 320;
		Material->GetExpressionCollection().AddExpression(NormalZ);

		UMaterialExpressionAppendVector* DistortedNormalVector = NewObject<UMaterialExpressionAppendVector>(Material);
		DistortedNormalVector->Material = Material;
		DistortedNormalVector->A.Connect(0, DistortedNormalXY);
		DistortedNormalVector->B.Connect(0, NormalZ);
		DistortedNormalVector->MaterialExpressionEditorX = 1180;
		DistortedNormalVector->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(DistortedNormalVector);

		UMaterialExpressionNormalize* DistortionNormal = NewObject<UMaterialExpressionNormalize>(Material);
		DistortionNormal->Material = Material;
		DistortionNormal->VectorInput.Connect(0, DistortedNormalVector);
		DistortionNormal->MaterialExpressionEditorX = 1420;
		DistortionNormal->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(DistortionNormal);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->Material = Material;
		OpacityParameter->ParameterName = TEXT("Opacity");
		OpacityParameter->DefaultValue = 0.26f;
		OpacityParameter->MaterialExpressionEditorX = 700;
		OpacityParameter->MaterialExpressionEditorY = 520;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		UMaterialExpressionMultiply* FinalOpacity = NewObject<UMaterialExpressionMultiply>(Material);
		FinalOpacity->Material = Material;
		FinalOpacity->A.Connect(0, RingMask);
		FinalOpacity->B.Connect(0, OpacityParameter);
		FinalOpacity->MaterialExpressionEditorX = 940;
		FinalOpacity->MaterialExpressionEditorY = 500;
		Material->GetExpressionCollection().AddExpression(FinalOpacity);

		UMaterialExpressionScalarParameter* RefractionAmountParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		RefractionAmountParameter->Material = Material;
		RefractionAmountParameter->ParameterName = TEXT("RefractionAmount");
		RefractionAmountParameter->DefaultValue = 1.18f;
		RefractionAmountParameter->MaterialExpressionEditorX = 1420;
		RefractionAmountParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(RefractionAmountParameter);

		UMaterialExpressionConstant3Vector* BaseColor = NewObject<UMaterialExpressionConstant3Vector>(Material);
		BaseColor->Material = Material;
		BaseColor->Constant = FLinearColor::Black;
		BaseColor->MaterialExpressionEditorX = 1420;
		BaseColor->MaterialExpressionEditorY = 560;
		Material->GetExpressionCollection().AddExpression(BaseColor);

		MaterialEditorOnly->BaseColor.Connect(0, BaseColor);
		MaterialEditorOnly->Normal.Connect(0, DistortionNormal);
		MaterialEditorOnly->Opacity.Connect(0, FinalOpacity);
		MaterialEditorOnly->Refraction.Connect(0, RefractionAmountParameter);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	bool EnsureLocalExplosionEffectAssets()
	{
		UTexture2D* FlipbookTexture = nullptr;
		const FString SourcePath = GetLocalExplosionFlipbookSourcePath();
		if (FPaths::FileExists(SourcePath))
		{
			if (!ImportWorldTexture(SourcePath, EffectsAssetPath, LocalExplosionFlipbookTextureAssetName, &FlipbookTexture))
			{
				return false;
			}
		}
		else
		{
			FlipbookTexture = LoadObject<UTexture2D>(
				nullptr,
				*GetAssetObjectPath(EffectsAssetPath, LocalExplosionFlipbookTextureAssetName));
		}

		if (!FlipbookTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing local explosion flipbook source: %s"), *SourcePath);
			return false;
		}

		ConfigureImportedEffectTexture(FlipbookTexture);
		return EnsureLocalExplosionFlipbookMaterial(FlipbookTexture, LocalExplosionFlipbookMaterialAssetName, false) != nullptr
			&& EnsureLocalExplosionFlipbookMaterial(FlipbookTexture, LocalExplosionSmokeMaterialAssetName, true) != nullptr
			&& EnsureLocalExplosionDistortionMaterial() != nullptr;
	}

	bool SetNiagaraStoreBoolByName(FNiagaraParameterStore& Store, FName ParameterName, bool bValue)
	{
		bool bApplied = false;
		TArray<FNiagaraVariable> Parameters;
		Store.GetParameters(Parameters);
		for (const FNiagaraVariable& Parameter : Parameters)
		{
			if (Parameter.GetName() == ParameterName &&
				Parameter.GetType() == FNiagaraTypeDefinition::GetBoolDef())
			{
				bApplied |= Store.SetParameterValue(FNiagaraBool(bValue), Parameter);
			}
		}
		return bApplied;
	}

	bool SetNiagaraStoreFloatByName(FNiagaraParameterStore& Store, FName ParameterName, float Value)
	{
		bool bApplied = false;
		TArray<FNiagaraVariable> Parameters;
		Store.GetParameters(Parameters);
		for (const FNiagaraVariable& Parameter : Parameters)
		{
			if (Parameter.GetName() == ParameterName &&
				Parameter.GetType() == FNiagaraTypeDefinition::GetFloatDef())
			{
				bApplied |= Store.SetParameterValue(Value, Parameter);
			}
		}
		return bApplied;
	}

	bool SetNiagaraStoreVec3ByName(FNiagaraParameterStore& Store, FName ParameterName, const FVector& Value)
	{
		bool bApplied = false;
		TArray<FNiagaraVariable> Parameters;
		Store.GetParameters(Parameters);
		for (const FNiagaraVariable& Parameter : Parameters)
		{
			if (Parameter.GetName() != ParameterName)
			{
				continue;
			}

			if (Parameter.GetType() == FNiagaraTypeDefinition::GetPositionDef())
			{
				bApplied |= Store.SetPositionParameterValue(Value, ParameterName);
			}
			else if (Parameter.GetType() == FNiagaraTypeDefinition::GetVec3Def())
			{
				bApplied |= Store.SetParameterValue(FVector3f(Value), Parameter);
			}
		}
		return bApplied;
	}

	bool SetNiagaraStoreColorByName(FNiagaraParameterStore& Store, FName ParameterName, const FLinearColor& Value)
	{
		bool bApplied = false;
		TArray<FNiagaraVariable> Parameters;
		Store.GetParameters(Parameters);
		for (const FNiagaraVariable& Parameter : Parameters)
		{
			if (Parameter.GetName() == ParameterName &&
				Parameter.GetType() == FNiagaraTypeDefinition::GetColorDef())
			{
				bApplied |= Store.SetParameterValue(Value, Parameter);
			}
		}
		return bApplied;
	}

	int32 ConfigureExtractionSmokeSignalNiagaraScript(UNiagaraScript* Script)
	{
		if (!Script)
		{
			return 0;
		}

		FNiagaraParameterStore& Store = Script->RapidIterationParameters;
		const FVector WorldSpaceSize(420.0f, 305.0f, 460.0f);
		const FVector SourceOffset(0.0f, 0.0f, 8.0f);
		const FVector SourceScale(2.8f, 1.45f, 0.16f);
		const FVector SourceVelocity(30.0f, 11.0f, 185.0f);
		const FLinearColor SmokeBaseColor(0.02f, 1.0f, 0.18f, 1.0f);
		const FLinearColor SmokeTopColor(0.035f, 0.04f, 0.035f, 1.0f);

		int32 AppliedCount = 0;
		auto ApplyBool = [&Store, &AppliedCount](const TCHAR* Name, bool bValue)
		{
			AppliedCount += SetNiagaraStoreBoolByName(Store, FName(Name), bValue) ? 1 : 0;
		};
		auto ApplyFloat = [&Store, &AppliedCount](const TCHAR* Name, float Value)
		{
			AppliedCount += SetNiagaraStoreFloatByName(Store, FName(Name), Value) ? 1 : 0;
		};
		auto ApplyVec3 = [&Store, &AppliedCount](const TCHAR* Name, const FVector& Value)
		{
			AppliedCount += SetNiagaraStoreVec3ByName(Store, FName(Name), Value) ? 1 : 0;
		};
		auto ApplyColor = [&Store, &AppliedCount](const TCHAR* Name, const FLinearColor& Value)
		{
			AppliedCount += SetNiagaraStoreColorByName(Store, FName(Name), Value) ? 1 : 0;
		};

		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Debug Draw"), false);
		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.Debug Collision Volume"), false);
		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_DebugDisplay.Debug Collision Volume"), false);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_InitializeEmitter.World Size"), WorldSpaceSize);
		ApplyBool(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Enable"), true);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Emit Position"), SourceOffset);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Non Uniform Scale"), SourceScale);
		ApplyVec3(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Velocity"), SourceVelocity);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Emit Radius"), 27.0f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Density"), 1.35f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Temperature"), 0.18f);
		ApplyColor(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_SphereSource.Color"), SmokeBaseColor);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_Buoyancy.TemperatureBuoyancy"), 1.1f);
		ApplyFloat(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_Buoyancy.DensityBuoyancy"), 0.05f);
		ApplyColor(TEXT("Grid3D_Gas_Master_Emitter.Grid3D_Gas_MaterialControls.Smoke Color"), SmokeTopColor);

		if (AppliedCount > 0)
		{
			Script->Modify();
		}

		return AppliedCount;
	}

	bool ConfigureExtractionSmokeSignalNiagaraSystem(UNiagaraSystem* System)
	{
		if (!System)
		{
			return false;
		}

		System->Modify();

		FNiagaraUserRedirectionParameterStore& UserParameters = System->GetExposedParameters();
		SetNiagaraStoreBoolByName(UserParameters, FName(TEXT("User.DrawBounds")), false);
		SetNiagaraStoreVec3ByName(UserParameters, FName(TEXT("User.WorldSpaceSize")), FVector(420.0f, 305.0f, 460.0f));
		SetNiagaraStoreFloatByName(UserParameters, FName(TEXT("User.ResolutionMaxAxis")), 96.0f);

		for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
		{
			EmitterHandle.SetDebugShowBounds(false);
			if (FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData())
			{
				EmitterData->bLocalSpace = false;
				EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
				EmitterData->FixedBounds = FBox(FVector(-230.0f, -170.0f, 0.0f), FVector(230.0f, 170.0f, 460.0f));
			}
		}

		int32 AppliedScriptParameterCount = 0;
		System->ForEachScript(
			[&AppliedScriptParameterCount](UNiagaraScript* Script)
			{
				AppliedScriptParameterCount += ConfigureExtractionSmokeSignalNiagaraScript(Script);
			});

		System->InvalidateCachedData();
		System->RequestCompile(true);
		System->PollForCompilationComplete(true);
		System->PostEditChange();
		System->MarkPackageDirty();

		UE_LOG(
			LogTunaSweeperEditor,
			Log,
			TEXT("Configured extraction smoke Niagara system. Applied %d rapid iteration parameter updates."),
			AppliedScriptParameterCount);
		return true;
	}

	UObject* LoadExtractionSmokeSignalSourceTemplate()
	{
		const TCHAR* SourceSystemPath =
			TEXT("/NiagaraFluids/Templates/Gas/3D/Systems/Grid3D_Gas_SimpleParticleSource.Grid3D_Gas_SimpleParticleSource");
		UObject* SourceSystem = LoadObject<UObject>(nullptr, SourceSystemPath);
		if (!SourceSystem)
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Failed to load extraction smoke source template: %s"),
				SourceSystemPath);
		}
		return SourceSystem;
	}

	bool DeleteExistingExtractionSmokeSignalNiagaraSystem(const FString& ObjectPath)
	{
		UObject* ExistingSystem = LoadObject<UObject>(nullptr, *ObjectPath);
		if (!ExistingSystem)
		{
			return true;
		}

		TArray<UObject*> ObjectsToDelete;
		ObjectsToDelete.Add(ExistingSystem);
		const int32 DeletedCount = ObjectTools::ForceDeleteObjects(ObjectsToDelete, false);
		if (DeletedCount != ObjectsToDelete.Num())
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Failed to recreate %s because the existing asset could not be deleted."),
				*ObjectPath);
			return false;
		}

		CollectGarbage(RF_NoFlags);
		return true;
	}

	bool EnsureExtractionSmokeSignalNiagaraSystem()
	{
		const FString ObjectPath = GetAssetObjectPath(EffectsAssetPath, ExtractionSmokeSignalNiagaraSystemAssetName);
		if (!DeleteExistingExtractionSmokeSignalNiagaraSystem(ObjectPath))
		{
			return false;
		}

		UObject* SourceSystem = LoadExtractionSmokeSignalSourceTemplate();
		if (!SourceSystem)
		{
			return false;
		}

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		UObject* DuplicatedSystem = AssetToolsModule.Get().DuplicateAsset(
			ExtractionSmokeSignalNiagaraSystemAssetName,
			EffectsAssetPath,
			SourceSystem);
		if (!DuplicatedSystem)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to duplicate %s."), *ObjectPath);
			return false;
		}

		if (UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(DuplicatedSystem))
		{
			ConfigureExtractionSmokeSignalNiagaraSystem(NiagaraSystem);
		}
		else
		{
			DuplicatedSystem->Modify();
			DuplicatedSystem->MarkPackageDirty();
		}
		return SaveAsset(DuplicatedSystem);
	}

	UMaterial* EnsureMemoStorageDeviceMaterial(UTexture2D* StorageTexture)
	{
		if (!StorageTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, MemoStorageDeviceMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				MemoStorageDeviceMaterialAssetName,
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
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = false;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -620;
		TextureCoordinateExpression->MaterialExpressionEditorY = 120;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionTextureSampleParameter2D* TextureSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		TextureSample->Material = Material;
		TextureSample->ParameterName = TEXT("StorageDeviceTexture");
		TextureSample->Texture = StorageTexture;
		TextureSample->SamplerType = SAMPLERTYPE_Color;
		TextureSample->Coordinates.Connect(0, TextureCoordinateExpression);
		TextureSample->MaterialExpressionEditorX = -360;
		TextureSample->MaterialExpressionEditorY = 40;
		TextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(TextureSample);

		UMaterialExpressionScalarParameter* EmissiveStrengthParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		EmissiveStrengthParameter->Material = Material;
		EmissiveStrengthParameter->ParameterName = TEXT("EmissiveStrength");
		EmissiveStrengthParameter->DefaultValue = 0.14f;
		EmissiveStrengthParameter->MaterialExpressionEditorX = -360;
		EmissiveStrengthParameter->MaterialExpressionEditorY = 260;
		Material->GetExpressionCollection().AddExpression(EmissiveStrengthParameter);

		UMaterialExpressionMultiply* EmissiveMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		EmissiveMultiply->Material = Material;
		EmissiveMultiply->A.Connect(0, TextureSample);
		EmissiveMultiply->B.Connect(0, EmissiveStrengthParameter);
		EmissiveMultiply->MaterialExpressionEditorX = -80;
		EmissiveMultiply->MaterialExpressionEditorY = 170;
		Material->GetExpressionCollection().AddExpression(EmissiveMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, TextureSample);
		MaterialEditorOnly->EmissiveColor.Connect(0, EmissiveMultiply);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.78f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.25f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	bool ImportMemoStorageDeviceTextureFromCommandLineIfRequested()
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportMemoStorageTextureSource="), SourceFile))
		{
			return false;
		}

		UTexture2D* ImportedTexture = nullptr;
		const bool bImported = ImportWorldTexture(
			SourceFile,
			InteractionAssetPath,
			MemoStorageDeviceTextureAssetName,
			&ImportedTexture);
		UMaterial* Material = bImported ? EnsureMemoStorageDeviceMaterial(ImportedTexture) : nullptr;
		const bool bSucceeded = ImportedTexture && Material;
		if (!bSucceeded)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import memo storage device texture/material."));
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportMemoStorageTextureQuit")))
		{
			FPlatformMisc::RequestExit(false);
		}

		return bSucceeded;
	}

	UMaterial* EnsureRollingBomberSpawnerMaterial(UTexture2D* SpawnerTexture)
	{
		if (!SpawnerTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, RollingBomberSpawnerMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				RollingBomberSpawnerMaterialAssetName,
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
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = false;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -760;
		TextureCoordinateExpression->MaterialExpressionEditorY = 60;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionTextureSampleParameter2D* TextureSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		TextureSample->Material = Material;
		TextureSample->ParameterName = TEXT("SpawnerMechanicTexture");
		TextureSample->Texture = SpawnerTexture;
		TextureSample->SamplerType = SAMPLERTYPE_Color;
		TextureSample->Coordinates.Connect(0, TextureCoordinateExpression);
		TextureSample->MaterialExpressionEditorX = -500;
		TextureSample->MaterialExpressionEditorY = -20;
		TextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(TextureSample);

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -500;
		VertexColorExpression->MaterialExpressionEditorY = 210;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);

		UMaterialExpressionMultiply* BaseColorMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		BaseColorMultiply->Material = Material;
		BaseColorMultiply->A.Connect(0, TextureSample);
		BaseColorMultiply->B.Connect(0, VertexColorExpression);
		BaseColorMultiply->MaterialExpressionEditorX = -180;
		BaseColorMultiply->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(BaseColorMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, BaseColorMultiply);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.72f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.12f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.35f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	bool ImportRollingBomberSpawnerTextureFromCommandLineIfRequested()
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportRollingBomberSpawnerTextureSource="), SourceFile))
		{
			return false;
		}

		UTexture2D* ImportedTexture = nullptr;
		const bool bImported = ImportWorldTexture(
			SourceFile,
			InteractionAssetPath,
			RollingBomberSpawnerTextureAssetName,
			&ImportedTexture);
		UMaterial* Material = bImported ? EnsureRollingBomberSpawnerMaterial(ImportedTexture) : nullptr;
		const bool bSucceeded = ImportedTexture && Material;
		if (!bSucceeded)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import rolling bomber spawner texture/material."));
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportRollingBomberSpawnerTextureQuit")))
		{
			FPlatformMisc::RequestExit(false);
		}

		return bSucceeded;
	}

	FString GetSandbagCoverTextureSourcePath()
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectContentDir(),
			TEXT("SourceArt"),
			TEXT("SandbagCover"),
			TEXT("T_SandbagCover_Burlap_Source.png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	UMaterial* EnsureSandbagCoverMaterial(UTexture2D* SandbagTexture)
	{
		if (!SandbagTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, SandbagCoverMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				SandbagCoverMaterialAssetName,
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
		Material->SetShadingModel(MSM_DefaultLit);
		Material->TwoSided = false;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinateExpression = NewObject<UMaterialExpressionTextureCoordinate>(Material);
		TextureCoordinateExpression->Material = Material;
		TextureCoordinateExpression->CoordinateIndex = 0;
		TextureCoordinateExpression->MaterialExpressionEditorX = -860;
		TextureCoordinateExpression->MaterialExpressionEditorY = 40;
		Material->GetExpressionCollection().AddExpression(TextureCoordinateExpression);

		UMaterialExpressionTextureSampleParameter2D* TextureSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(Material);
		TextureSample->Material = Material;
		TextureSample->ParameterName = TEXT("SandbagTexture");
		TextureSample->Texture = SandbagTexture;
		TextureSample->SamplerType = SAMPLERTYPE_Color;
		TextureSample->Coordinates.Connect(0, TextureCoordinateExpression);
		TextureSample->MaterialExpressionEditorX = -620;
		TextureSample->MaterialExpressionEditorY = -20;
		TextureSample->AutoSetSampleType();
		Material->GetExpressionCollection().AddExpression(TextureSample);

		UMaterialExpressionVertexColor* VertexColorExpression = NewObject<UMaterialExpressionVertexColor>(Material);
		VertexColorExpression->Material = Material;
		VertexColorExpression->MaterialExpressionEditorX = -620;
		VertexColorExpression->MaterialExpressionEditorY = 190;
		Material->GetExpressionCollection().AddExpression(VertexColorExpression);

		UMaterialExpressionMultiply* BaseColorMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		BaseColorMultiply->Material = Material;
		BaseColorMultiply->A.Connect(0, TextureSample);
		BaseColorMultiply->B.Connect(0, VertexColorExpression);
		BaseColorMultiply->MaterialExpressionEditorX = -360;
		BaseColorMultiply->MaterialExpressionEditorY = 80;
		Material->GetExpressionCollection().AddExpression(BaseColorMultiply);

		UMaterialExpressionVectorParameter* DamageTintParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		DamageTintParameter->Material = Material;
		DamageTintParameter->ParameterName = TEXT("DamageTint");
		DamageTintParameter->DefaultValue = FLinearColor(0.46f, 0.37f, 0.26f, 1.0f);
		DamageTintParameter->MaterialExpressionEditorX = -360;
		DamageTintParameter->MaterialExpressionEditorY = 300;
		Material->GetExpressionCollection().AddExpression(DamageTintParameter);

		UMaterialExpressionMultiply* DamagedColorMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		DamagedColorMultiply->Material = Material;
		DamagedColorMultiply->A.Connect(0, BaseColorMultiply);
		DamagedColorMultiply->B.Connect(0, DamageTintParameter);
		DamagedColorMultiply->MaterialExpressionEditorX = -80;
		DamagedColorMultiply->MaterialExpressionEditorY = 210;
		Material->GetExpressionCollection().AddExpression(DamagedColorMultiply);

		UMaterialExpressionScalarParameter* DamageAlphaParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		DamageAlphaParameter->Material = Material;
		DamageAlphaParameter->ParameterName = TEXT("DamageAlpha");
		DamageAlphaParameter->DefaultValue = 0.0f;
		DamageAlphaParameter->MaterialExpressionEditorX = -80;
		DamageAlphaParameter->MaterialExpressionEditorY = 390;
		Material->GetExpressionCollection().AddExpression(DamageAlphaParameter);

		UMaterialExpressionLinearInterpolate* DamageLerp = NewObject<UMaterialExpressionLinearInterpolate>(Material);
		DamageLerp->Material = Material;
		DamageLerp->A.Connect(0, BaseColorMultiply);
		DamageLerp->B.Connect(0, DamagedColorMultiply);
		DamageLerp->Alpha.Connect(0, DamageAlphaParameter);
		DamageLerp->MaterialExpressionEditorX = 170;
		DamageLerp->MaterialExpressionEditorY = 110;
		Material->GetExpressionCollection().AddExpression(DamageLerp);

		UMaterialExpressionScalarParameter* AmbientLiftParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		AmbientLiftParameter->Material = Material;
		AmbientLiftParameter->ParameterName = TEXT("AmbientLift");
		AmbientLiftParameter->DefaultValue = 0.18f;
		AmbientLiftParameter->MaterialExpressionEditorX = 170;
		AmbientLiftParameter->MaterialExpressionEditorY = 360;
		Material->GetExpressionCollection().AddExpression(AmbientLiftParameter);

		UMaterialExpressionMultiply* AmbientLiftMultiply = NewObject<UMaterialExpressionMultiply>(Material);
		AmbientLiftMultiply->Material = Material;
		AmbientLiftMultiply->A.Connect(0, DamageLerp);
		AmbientLiftMultiply->B.Connect(0, AmbientLiftParameter);
		AmbientLiftMultiply->MaterialExpressionEditorX = 430;
		AmbientLiftMultiply->MaterialExpressionEditorY = 260;
		Material->GetExpressionCollection().AddExpression(AmbientLiftMultiply);

		MaterialEditorOnly->BaseColor.Connect(0, DamageLerp);
		MaterialEditorOnly->EmissiveColor.Connect(0, AmbientLiftMultiply);
		MaterialEditorOnly->Roughness.UseConstant = true;
		MaterialEditorOnly->Roughness.Constant = 0.88f;
		MaterialEditorOnly->Metallic.UseConstant = true;
		MaterialEditorOnly->Metallic.Constant = 0.0f;
		MaterialEditorOnly->Specular.UseConstant = true;
		MaterialEditorOnly->Specular.Constant = 0.18f;

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	UMaterial* EnsureSandbagCoverOutlineMaterial()
	{
		const FString ObjectPath = GetAssetObjectPath(InteractionAssetPath, SandbagCoverOutlineMaterialAssetName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();

			FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			UObject* CreatedAsset = AssetToolsModule.Get().CreateAsset(
				SandbagCoverOutlineMaterialAssetName,
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
		Material->BlendMode = BLEND_Additive;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = false;

		UMaterialEditorOnlyData* MaterialEditorOnly = Material->GetEditorOnlyData();
		if (!MaterialEditorOnly)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to edit %s."), *ObjectPath);
			return nullptr;
		}

		UMaterialExpressionVectorParameter* ColorParameter = NewObject<UMaterialExpressionVectorParameter>(Material);
		ColorParameter->Material = Material;
		ColorParameter->ParameterName = TEXT("OutlineColor");
		ColorParameter->DefaultValue = FLinearColor(0.09f, 0.13f, 0.13f, 1.0f);
		ColorParameter->MaterialExpressionEditorX = -360;
		ColorParameter->MaterialExpressionEditorY = -90;
		Material->GetExpressionCollection().AddExpression(ColorParameter);

		UMaterialExpressionScalarParameter* StencilParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		StencilParameter->Material = Material;
		StencilParameter->ParameterName = TEXT("StencilValue");
		StencilParameter->DefaultValue = 3.0f;
		StencilParameter->MaterialExpressionEditorX = -360;
		StencilParameter->MaterialExpressionEditorY = 70;
		Material->GetExpressionCollection().AddExpression(StencilParameter);

		UMaterialExpressionScalarParameter* ThicknessParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		ThicknessParameter->Material = Material;
		ThicknessParameter->ParameterName = TEXT("OutlineThickness");
		ThicknessParameter->DefaultValue = 2.0f;
		ThicknessParameter->MaterialExpressionEditorX = -360;
		ThicknessParameter->MaterialExpressionEditorY = 230;
		Material->GetExpressionCollection().AddExpression(ThicknessParameter);

		UMaterialExpressionScalarParameter* OpacityParameter = NewObject<UMaterialExpressionScalarParameter>(Material);
		OpacityParameter->Material = Material;
		OpacityParameter->ParameterName = TEXT("OutlineOpacity");
		OpacityParameter->DefaultValue = 0.92f;
		OpacityParameter->MaterialExpressionEditorX = -360;
		OpacityParameter->MaterialExpressionEditorY = 390;
		Material->GetExpressionCollection().AddExpression(OpacityParameter);

		UMaterialExpressionCustom* OutlineExpression = NewObject<UMaterialExpressionCustom>(Material);
		OutlineExpression->Material = Material;
		OutlineExpression->Description = TEXT("Sandbag custom stencil outline");
		OutlineExpression->OutputType = CMOT_Float4;
		OutlineExpression->MaterialExpressionEditorX = -40;
		OutlineExpression->MaterialExpressionEditorY = 40;
		OutlineExpression->Code = TEXT(
			"float2 uv = GetDefaultSceneTextureUV(Parameters, 14);\n"
			"float2 texel = View.ViewSizeAndInvSize.zw * max(1.0, OutlineThickness);\n"
			"float targetRaw = StencilValue;\n"
			"float targetNorm = StencilValue / 255.0;\n"
			"float4 sceneColor = SceneTextureLookup(uv, 14, false);\n"
			"#define MATCH_STENCIL(Value) max(1.0 - step(0.5, abs((Value) - targetRaw)), 1.0 - step(0.5 / 255.0, abs((Value) - targetNorm)))\n"
			"float centerMask = MATCH_STENCIL(SceneTextureLookup(uv, 25, false).r);\n"
			"float neighborMask = 0.0;\n"
			"neighborMask = max(neighborMask, MATCH_STENCIL(SceneTextureLookup(uv + float2(texel.x, 0.0), 25, false).r));\n"
			"neighborMask = max(neighborMask, MATCH_STENCIL(SceneTextureLookup(uv + float2(-texel.x, 0.0), 25, false).r));\n"
			"neighborMask = max(neighborMask, MATCH_STENCIL(SceneTextureLookup(uv + float2(0.0, texel.y), 25, false).r));\n"
			"neighborMask = max(neighborMask, MATCH_STENCIL(SceneTextureLookup(uv + float2(0.0, -texel.y), 25, false).r));\n"
			"neighborMask = max(neighborMask, MATCH_STENCIL(SceneTextureLookup(uv + float2(texel.x, texel.y), 25, false).r));\n"
			"neighborMask = max(neighborMask, MATCH_STENCIL(SceneTextureLookup(uv + float2(-texel.x, texel.y), 25, false).r));\n"
			"neighborMask = max(neighborMask, MATCH_STENCIL(SceneTextureLookup(uv + float2(texel.x, -texel.y), 25, false).r));\n"
			"neighborMask = max(neighborMask, MATCH_STENCIL(SceneTextureLookup(uv + float2(-texel.x, -texel.y), 25, false).r));\n"
			"float edgeMask = saturate(neighborMask - centerMask);\n"
			"#undef MATCH_STENCIL\n"
			"return lerp(sceneColor, float4(OutlineColor.rgb, 1.0), edgeMask * saturate(OutlineOpacity));\n");

		FCustomInput OutlineColorInput;
		OutlineColorInput.InputName = TEXT("OutlineColor");
		OutlineColorInput.Input.Connect(0, ColorParameter);
		OutlineExpression->Inputs.Add(OutlineColorInput);

		FCustomInput StencilInput;
		StencilInput.InputName = TEXT("StencilValue");
		StencilInput.Input.Connect(0, StencilParameter);
		OutlineExpression->Inputs.Add(StencilInput);

		FCustomInput ThicknessInput;
		ThicknessInput.InputName = TEXT("OutlineThickness");
		ThicknessInput.Input.Connect(0, ThicknessParameter);
		OutlineExpression->Inputs.Add(ThicknessInput);

		FCustomInput OpacityInput;
		OpacityInput.InputName = TEXT("OutlineOpacity");
		OpacityInput.Input.Connect(0, OpacityParameter);
		OutlineExpression->Inputs.Add(OpacityInput);

		Material->GetExpressionCollection().AddExpression(OutlineExpression);
		MaterialEditorOnly->EmissiveColor.Connect(0, OutlineExpression);

		Material->PostEditChange();
		Material->MarkPackageDirty();

		if (!SaveAsset(Material))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to save %s."), *ObjectPath);
			return nullptr;
		}

		return Material;
	}

	bool EnsureSandbagCoverAssets()
	{
		UTexture2D* SandbagTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(InteractionAssetPath, SandbagCoverTextureAssetName));
		if (!SandbagTexture)
		{
			const FString SourcePath = GetSandbagCoverTextureSourcePath();
			if (!FPaths::FileExists(SourcePath) ||
				!ImportWorldTexture(SourcePath, InteractionAssetPath, SandbagCoverTextureAssetName, &SandbagTexture))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import sandbag cover texture source: %s"), *SourcePath);
				return false;
			}
		}

		UMaterial* SandbagMaterial = EnsureSandbagCoverMaterial(SandbagTexture);
		UMaterial* OutlineMaterial = EnsureSandbagCoverOutlineMaterial();
		UBlueprint* SandbagBlueprint = EnsureBlueprint(
			InteractionAssetPath,
			SandbagCoverAssetName,
			ATunaSweeperSandbagCoverActor::StaticClass());
		if (!SandbagMaterial || !OutlineMaterial || !SandbagBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(SandbagBlueprint);
		ATunaSweeperSandbagCoverActor* Defaults = SandbagBlueprint->GeneratedClass
			? Cast<ATunaSweeperSandbagCoverActor>(SandbagBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(SandbagBlueprint));
			return false;
		}

		SandbagBlueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigureCoverDefaults(
			FName(TEXT("TS_SandbagCover_Default")),
			FVector(75.0f, 320.0f, 90.0f),
			70.0f,
			125.0f);
		Defaults->ConfigureCoverVisualDefaults(
			TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(GetAssetObjectPath(InteractionAssetPath, SandbagCoverMaterialAssetName))),
			TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(GetAssetObjectPath(InteractionAssetPath, SandbagCoverOutlineMaterialAssetName))));
		FBlueprintEditorUtils::MarkBlueprintAsModified(SandbagBlueprint);
		FKismetEditorUtilities::CompileBlueprint(SandbagBlueprint);
		SandbagBlueprint->MarkPackageDirty();
		return SaveAsset(SandbagBlueprint);
	}

	bool ImportSandbagCoverTextureFromCommandLineIfRequested()
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportSandbagCoverTextureSource="), SourceFile))
		{
			return false;
		}

		UTexture2D* ImportedTexture = nullptr;
		const bool bImported = ImportWorldTexture(
			SourceFile,
			InteractionAssetPath,
			SandbagCoverTextureAssetName,
			&ImportedTexture);
		const bool bSucceeded = bImported && EnsureSandbagCoverAssets();
		if (!bSucceeded)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import sandbag cover texture/material/blueprint."));
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportSandbagCoverTextureQuit")))
		{
			FPlatformMisc::RequestExit(false);
		}

		return bSucceeded;
	}

	bool ImportUiTexture(const FUiTextureImportArgs& Args, UTexture2D** OutTexture = nullptr)
	{
		if (OutTexture)
		{
			*OutTexture = nullptr;
		}

		FString SourceFile = Args.SourceFile;
		FPaths::NormalizeFilename(SourceFile);
		SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
		FPaths::CollapseRelativeDirectories(SourceFile);

		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing UI texture source: %s"), *SourceFile);
			return false;
		}

		const FString DestinationPath = Args.DestinationPath.IsEmpty() ? UITitleTextureAssetPath : Args.DestinationPath;
		FString AssetName = Args.AssetName.IsEmpty() ? FPaths::GetBaseFilename(SourceFile) : Args.AssetName;
		AssetName = FPaths::GetBaseFilename(AssetName);

		if (AssetName.IsEmpty() || !FPackageName::IsValidLongPackageName(DestinationPath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Invalid UI texture destination: path=%s asset=%s"), *DestinationPath, *AssetName);
			return false;
		}

		const FString ObjectPath = GetAssetObjectPath(DestinationPath, AssetName);
		if (!Args.bReplaceExisting)
		{
			if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath))
			{
				ConfigureImportedUiTexture(ExistingTexture);
				if (OutTexture)
				{
					*OutTexture = ExistingTexture;
				}
				return true;
			}
		}

		FString ImportFile = SourceFile;
		if (FPaths::GetBaseFilename(SourceFile) != AssetName)
		{
			const FString ImportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TunaSweeperUiTextureImport"));
			IFileManager::Get().MakeDirectory(*ImportDirectory, true);
			ImportFile = FPaths::Combine(ImportDirectory, AssetName + TEXT(".") + FPaths::GetExtension(SourceFile));
			if (IFileManager::Get().Copy(*ImportFile, *SourceFile, true, true) != COPY_OK)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to stage UI texture import source: %s -> %s"), *SourceFile, *ImportFile);
				return false;
			}
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(ImportFile);
		ImportData->bReplaceExisting = Args.bReplaceExisting;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import UI texture: %s"), *ImportFile);
			return false;
		}

		UTexture2D* ImportedTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath);
		if (!ImportedTexture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported UI texture: %s"), *ObjectPath);
			return false;
		}

		ConfigureImportedUiTexture(ImportedTexture);
		if (OutTexture)
		{
			*OutTexture = ImportedTexture;
		}
		return true;
	}

	bool TryReadUiTextureImportArgsFromCommandLine(FUiTextureImportArgs& OutArgs)
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureSource="), SourceFile))
		{
			return false;
		}

		OutArgs.SourceFile = SourceFile;
		FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureDest="), OutArgs.DestinationPath);
		FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureName="), OutArgs.AssetName);
		OutArgs.bReplaceExisting = !FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureNoReplace"));
		return true;
	}

	bool ImportUiTextureFromCommandLineIfRequested()
	{
		FUiTextureImportArgs Args;
		if (!TryReadUiTextureImportArgsFromCommandLine(Args))
		{
			return false;
		}

		const bool bImported = ImportUiTexture(Args);
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureQuit")))
		{
			FPlatformMisc::RequestExit(false);
		}
		return bImported;
	}

	void ConfigureImportedBgmSound(USoundWave* SoundWave, bool bLooping)
	{
		if (!SoundWave)
		{
			return;
		}

		SoundWave->Modify();
		SoundWave->bLooping = bLooping;
		SoundWave->PostEditChange();
		SoundWave->MarkPackageDirty();
		SaveAsset(SoundWave);
	}

	bool ImportAudioAsset(const FAudioImportArgs& Args, USoundWave** OutSoundWave = nullptr)
	{
		if (OutSoundWave)
		{
			*OutSoundWave = nullptr;
		}

		FString SourceFile = Args.SourceFile;
		FPaths::NormalizeFilename(SourceFile);
		SourceFile = FPaths::ConvertRelativePathToFull(SourceFile);
		FPaths::CollapseRelativeDirectories(SourceFile);

		if (!FPaths::FileExists(SourceFile))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing audio source: %s"), *SourceFile);
			return false;
		}

		const FString DestinationPath = Args.DestinationPath.IsEmpty() ? AudioBgmAssetPath : Args.DestinationPath;
		FString AssetName = Args.AssetName.IsEmpty() ? FPaths::GetBaseFilename(SourceFile) : Args.AssetName;
		AssetName = FPaths::GetBaseFilename(AssetName);

		if (AssetName.IsEmpty() || !FPackageName::IsValidLongPackageName(DestinationPath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Invalid audio destination: path=%s asset=%s"), *DestinationPath, *AssetName);
			return false;
		}

		const FString ObjectPath = GetAssetObjectPath(DestinationPath, AssetName);
		if (!Args.bReplaceExisting)
		{
			if (USoundWave* ExistingSoundWave = LoadObject<USoundWave>(nullptr, *ObjectPath))
			{
				ConfigureImportedBgmSound(ExistingSoundWave, Args.bLooping);
				if (OutSoundWave)
				{
					*OutSoundWave = ExistingSoundWave;
				}
				return true;
			}
		}

		FString ImportFile = SourceFile;
		if (FPaths::GetBaseFilename(SourceFile) != AssetName)
		{
			const FString ImportDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("TunaSweeperAudioImport"));
			IFileManager::Get().MakeDirectory(*ImportDirectory, true);
			ImportFile = FPaths::Combine(ImportDirectory, AssetName + TEXT(".") + FPaths::GetExtension(SourceFile));
			if (IFileManager::Get().Copy(*ImportFile, *SourceFile, true, true) != COPY_OK)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to stage audio import source: %s -> %s"), *SourceFile, *ImportFile);
				return false;
			}
		}

		FModuleManager::Get().LoadModule(TEXT("AudioEditor"));

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = DestinationPath;
		ImportData->Filenames.Add(ImportFile);
		ImportData->bReplaceExisting = Args.bReplaceExisting;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import audio: %s"), *ImportFile);
			return false;
		}

		USoundWave* ImportedSoundWave = LoadObject<USoundWave>(nullptr, *ObjectPath);
		if (!ImportedSoundWave)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported audio: %s"), *ObjectPath);
			return false;
		}

		ConfigureImportedBgmSound(ImportedSoundWave, Args.bLooping);
		if (OutSoundWave)
		{
			*OutSoundWave = ImportedSoundWave;
		}
		return true;
	}

	bool TryReadAudioImportArgsFromCommandLine(FAudioImportArgs& OutArgs)
	{
		FString SourceFile;
		if (!FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportAudioSource="), SourceFile))
		{
			return false;
		}

		OutArgs.SourceFile = SourceFile;
		FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportAudioDest="), OutArgs.DestinationPath);
		FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportAudioName="), OutArgs.AssetName);
		OutArgs.bReplaceExisting = !FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportAudioNoReplace"));
		OutArgs.bLooping = !FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportAudioNoLoop"));
		return true;
	}

	bool ImportAudioFromCommandLineIfRequested()
	{
		FAudioImportArgs Args;
		if (!TryReadAudioImportArgsFromCommandLine(Args))
		{
			return false;
		}

		const bool bImported = ImportAudioAsset(Args);
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportAudioQuit")))
		{
			FPlatformMisc::RequestExit(true);
		}
		return bImported;
	}

	FString GetWorkspaceFilePath(const FString& RelativePath)
	{
		FString FilePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT(".."), RelativePath));
		FPaths::CollapseRelativeDirectories(FilePath);
		return FilePath;
	}

	bool EnsureTitleUiTextures()
	{
		UTexture2D* BackgroundTexture = nullptr;
		UTexture2D* LogoTexture = nullptr;

		const bool bBackgroundImported = ImportUiTexture(
			FUiTextureImportArgs{
				GetWorkspaceFilePath(TEXT("chatgpt/Title_C1.png")),
				UITitleTextureAssetPath,
				TitleBackgroundTextureAssetName,
				true
			},
			&BackgroundTexture);

		const bool bLogoImported = ImportUiTexture(
			FUiTextureImportArgs{
				GetWorkspaceFilePath(TEXT("Docs/Story/tuna_sweeper_logo_transparent.png")),
				UITitleTextureAssetPath,
				TitleLogoTextureAssetName,
				true
			},
			&LogoTexture);

		return bBackgroundImported && bLogoImported && BackgroundTexture && LogoTexture;
	}

	bool EnsureOpeningScenarioUiTextures()
	{
		UTexture2D* BackgroundTexture = nullptr;
		const bool bBackgroundImported = ImportUiTexture(
			FUiTextureImportArgs{
				GetWorkspaceFilePath(TEXT("chatgpt/opening_light_particles.png")),
				UIStoryTextureAssetPath,
				OpeningScenarioBackgroundTextureAssetName,
				true
			},
			&BackgroundTexture);

		return bBackgroundImported && BackgroundTexture;
	}

	bool EnsureItemIconTextures()
	{
		TArray<FString> FilesToImport;

		for (const FString& IconAssetName : GetItemIconAssetNames())
		{
			const FString ObjectPath = GetAssetObjectPath(UIIconAssetPath, IconAssetName);
			if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *ObjectPath))
			{
				ConfigureImportedIconTexture(ExistingTexture);
				continue;
			}

			const FString SourcePath = GetItemIconSourcePath(IconAssetName);
			if (!FPaths::FileExists(SourcePath))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing item icon source: %s"), *SourcePath);
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
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import item icons."));
				return false;
			}
		}

		for (const FString& IconAssetName : GetItemIconAssetNames())
		{
			UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *GetAssetObjectPath(UIIconAssetPath, IconAssetName));
			if (!Texture)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load imported item icon: %s"), *IconAssetName);
				return false;
			}

			ConfigureImportedIconTexture(Texture);
		}

		return true;
	}

	bool ImportIconTextureSources(const TArray<FString>& IconAssetNames, bool bReplaceExisting)
	{
		TArray<FString> FilesToImport;
		for (const FString& IconAssetName : IconAssetNames)
		{
			const FString SourcePath = GetItemIconSourcePath(IconAssetName);
			if (!FPaths::FileExists(SourcePath))
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Missing icon source: %s"), *SourcePath);
				return false;
			}

			FilesToImport.Add(SourcePath);
		}

		UAutomatedAssetImportData* ImportData = NewObject<UAutomatedAssetImportData>();
		ImportData->DestinationPath = UIIconAssetPath;
		ImportData->Filenames = FilesToImport;
		ImportData->bReplaceExisting = bReplaceExisting;
		ImportData->bSkipReadOnly = true;

		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		const TArray<UObject*> ImportedAssets = AssetToolsModule.Get().ImportAssetsAutomated(ImportData);
		if (ImportedAssets.Num() == 0)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import icon textures."));
			return false;
		}

		for (const FString& IconAssetName : IconAssetNames)
		{
			UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *GetAssetObjectPath(UIIconAssetPath, IconAssetName));
			if (!Texture)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load icon texture: %s"), *IconAssetName);
				return false;
			}

			ConfigureImportedIconTexture(Texture);
		}

		return true;
	}

	bool EnsureEquipmentIconTextures()
	{
		return ImportIconTextureSources(GetEquipmentIconAssetNames(), true);
	}

	bool EnsureCannedTunaIconTexture()
	{
		const FString IconAssetName = TEXT("T_UIIcon_CannedTuna");
		if (UTexture2D* ExistingTexture = LoadObject<UTexture2D>(nullptr, *GetAssetObjectPath(UIIconAssetPath, IconAssetName)))
		{
			ConfigureImportedIconTexture(ExistingTexture);
			return true;
		}

		const FString SourcePath = GetItemIconSourcePath(IconAssetName);
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
		UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("SlotOverlay"));
		USizeBox* IconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("IconBox"));
		UImage* ItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ItemIconImage"));
		UTextBlock* ItemQuantityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemQuantityText"));
		UTextBlock* AttachmentSlotIndicatorText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("AttachmentSlotIndicatorText"));
		UBorder* ItemNamePlate = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ItemNamePlate"));
		UTextBlock* ItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ItemNameText"));

		if (!RootSizeBox || !SlotBackground || !SlotOverlay || !IconBox || !ItemIconImage ||
			!ItemQuantityText || !AttachmentSlotIndicatorText || !ItemNamePlate || !ItemNameText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(96.0f);
		RootSizeBox->SetHeightOverride(96.0f);
		RootSizeBox->SetContent(SlotBackground);

		SlotBackground->SetPadding(FMargin(5.0f));
		SlotBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(96.0f, 96.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.24f, 0.27f, 0.31f, 0.95f),
			1.0f));
		SlotBackground->SetContent(SlotOverlay);

		IconBox->SetWidthOverride(86.0f);
		IconBox->SetHeightOverride(86.0f);
		IconBox->SetContent(ItemIconImage);
		ItemIconImage->SetColorAndOpacity(FLinearColor::White);

		UOverlaySlot* IconSlot = SlotOverlay->AddChildToOverlay(IconBox);
		if (IconSlot)
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlock(ItemQuantityText, FText::FromString(TEXT("x1")), FLinearColor::White, 13);
		UOverlaySlot* QuantitySlot = SlotOverlay->AddChildToOverlay(ItemQuantityText);
		if (QuantitySlot)
		{
			QuantitySlot->SetHorizontalAlignment(HAlign_Right);
			QuantitySlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlock(AttachmentSlotIndicatorText, FText::GetEmpty(), FLinearColor::White, 13);
		UOverlaySlot* AttachmentIndicatorSlot = SlotOverlay->AddChildToOverlay(AttachmentSlotIndicatorText);
		if (AttachmentIndicatorSlot)
		{
			AttachmentIndicatorSlot->SetHorizontalAlignment(HAlign_Left);
			AttachmentIndicatorSlot->SetVerticalAlignment(VAlign_Top);
		}

		ItemNamePlate->SetPadding(FMargin(3.0f, 1.0f));
		ItemNamePlate->SetBrush(MakeRoundedBoxBrush(
			FVector2D(86.0f, 18.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.62f),
			FLinearColor::Transparent,
			0.0f));
		ConfigureTextBlock(ItemNameText, FText::FromString(TEXT("Item")), FLinearColor(0.82f, 0.88f, 0.94f, 1.0f), 10);
		ItemNameText->SetAutoWrapText(false);
		ItemNamePlate->SetContent(ItemNameText);
		UOverlaySlot* NameSlot = SlotOverlay->AddChildToOverlay(ItemNamePlate);
		if (NameSlot)
		{
			NameSlot->SetHorizontalAlignment(HAlign_Fill);
			NameSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		RegisterWidgetVariable(WidgetBlueprint, SlotBackground);
		RegisterWidgetVariable(WidgetBlueprint, ItemIconImage);
		RegisterWidgetVariable(WidgetBlueprint, ItemQuantityText);
		RegisterWidgetVariable(WidgetBlueprint, AttachmentSlotIndicatorText);
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
		UHorizontalBox* ModeTabRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ModeTabRow"));
		UButton* InventoryModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("InventoryModeButton"));
		UButton* QuestModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("QuestModeButton"));
		UButton* MapModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MapModeButton"));
		UButton* MemoModeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("MemoModeButton"));
		USizeBox* InventoryModeButtonFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InventoryModeButtonFrame"));
		USizeBox* QuestModeButtonFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("QuestModeButtonFrame"));
		USizeBox* MapModeButtonFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MapModeButtonFrame"));
		USizeBox* MemoModeButtonFrame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MemoModeButtonFrame"));
		UImage* InventoryModeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("InventoryModeIcon"));
		UImage* QuestModeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("QuestModeIcon"));
		UImage* MapModeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MapModeIcon"));
		UImage* MemoModeIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MemoModeIcon"));
		if (!RootSizeBox || !ReservedBackground || !ModeTabRow ||
			!InventoryModeButton || !QuestModeButton || !MapModeButton || !MemoModeButton ||
			!InventoryModeButtonFrame || !QuestModeButtonFrame || !MapModeButtonFrame || !MemoModeButtonFrame ||
			!InventoryModeIcon || !QuestModeIcon || !MapModeIcon || !MemoModeIcon)
		{
			return false;
		}

		UTexture2D* InventoryModeTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudModeInventoryIconAssetName));
		UTexture2D* QuestModeTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudModeQuestIconAssetName));
		UTexture2D* MapModeTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudModeMapIconAssetName));
		UTexture2D* MemoModeTexture = LoadObject<UTexture2D>(
			nullptr,
			*GetAssetObjectPath(UIIconAssetPath, HudModeMemoIconAssetName));

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(HudTopModeTabPanelWidth);
		RootSizeBox->SetHeightOverride(HudTopModeTabPanelHeight);
		RootSizeBox->SetContent(ReservedBackground);

		ReservedBackground->SetPadding(FMargin(HudTopModeTabPaddingX, HudTopModeTabPaddingY));
		ReservedBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(HudTopModeTabPanelWidth, HudTopModeTabPanelHeight),
			FLinearColor(0.005f, 0.006f, 0.008f, 0.48f),
			FLinearColor(0.15f, 0.17f, 0.19f, 0.42f),
			1.0f,
			5.0f));
		ReservedBackground->SetContent(ModeTabRow);

		auto ConfigureModeButton = [=](UButton* Button)
		{
			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(
				FVector2D(HudTopModeTabButtonWidth, HudTopModeTabButtonHeight),
				FLinearColor(0.030f, 0.036f, 0.038f, 0.76f),
				FLinearColor(0.20f, 0.24f, 0.25f, 0.82f),
				1.0f,
				4.0f));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(
				FVector2D(HudTopModeTabButtonWidth, HudTopModeTabButtonHeight),
				FLinearColor(0.070f, 0.085f, 0.083f, 0.90f),
				FLinearColor(0.58f, 0.70f, 0.62f, 0.92f),
				1.5f,
				4.0f));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(
				FVector2D(HudTopModeTabButtonWidth, HudTopModeTabButtonHeight),
				FLinearColor(0.020f, 0.026f, 0.028f, 0.96f),
				FLinearColor(0.48f, 0.64f, 0.54f, 0.94f),
				1.0f,
				4.0f));
			Button->SetStyle(ButtonStyle);
		};

		auto ConfigureModeIcon = [](UImage* Icon, UTexture2D* Texture)
		{
			if (Texture)
			{
				Icon->SetBrushFromTexture(Texture, true);
			}
			Icon->SetDesiredSizeOverride(FVector2D(28.0f, 28.0f));
			Icon->SetColorAndOpacity(FLinearColor(0.74f, 0.80f, 0.82f, 1.0f));
		};

		auto AddModeTab = [&ModeTabRow, &ConfigureModeButton, &ConfigureModeIcon](
			USizeBox* Frame,
			UButton* Button,
			UImage* Icon,
			UTexture2D* Texture,
			bool bFirst)
		{
			Frame->SetWidthOverride(HudTopModeTabButtonWidth);
			Frame->SetHeightOverride(HudTopModeTabButtonHeight);
			Frame->SetContent(Button);
			ConfigureModeButton(Button);
			ConfigureModeIcon(Icon, Texture);
			Button->SetContent(Icon);
			if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Icon->Slot))
			{
				ButtonSlot->SetHorizontalAlignment(HAlign_Center);
				ButtonSlot->SetVerticalAlignment(VAlign_Center);
				ButtonSlot->SetPadding(FMargin(0.0f));
			}
			UHorizontalBoxSlot* Slot = ModeTabRow->AddChildToHorizontalBox(Frame);
			if (Slot)
			{
				Slot->SetPadding(FMargin(bFirst ? 0.0f : HudTopModeTabGap, 0.0f, 0.0f, 0.0f));
				Slot->SetVerticalAlignment(VAlign_Center);
			}
		};

		AddModeTab(InventoryModeButtonFrame, InventoryModeButton, InventoryModeIcon, InventoryModeTexture, true);
		AddModeTab(QuestModeButtonFrame, QuestModeButton, QuestModeIcon, QuestModeTexture, false);
		AddModeTab(MapModeButtonFrame, MapModeButton, MapModeIcon, MapModeTexture, false);
		AddModeTab(MemoModeButtonFrame, MemoModeButton, MemoModeIcon, MemoModeTexture, false);

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, ReservedBackground);
		RegisterWidgetVariable(WidgetBlueprint, ModeTabRow);
		RegisterWidgetVariable(WidgetBlueprint, InventoryModeButton);
		RegisterWidgetVariable(WidgetBlueprint, QuestModeButton);
		RegisterWidgetVariable(WidgetBlueprint, MapModeButton);
		RegisterWidgetVariable(WidgetBlueprint, MemoModeButton);
		RegisterWidgetVariable(WidgetBlueprint, InventoryModeButtonFrame);
		RegisterWidgetVariable(WidgetBlueprint, QuestModeButtonFrame);
		RegisterWidgetVariable(WidgetBlueprint, MapModeButtonFrame);
		RegisterWidgetVariable(WidgetBlueprint, MemoModeButtonFrame);
		RegisterWidgetVariable(WidgetBlueprint, InventoryModeIcon);
		RegisterWidgetVariable(WidgetBlueprint, QuestModeIcon);
		RegisterWidgetVariable(WidgetBlueprint, MapModeIcon);
		RegisterWidgetVariable(WidgetBlueprint, MemoModeIcon);
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
		UHorizontalBox* VitalsRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("VitalsRow"));
		USizeBox* HealthBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HealthBox"));
		UOverlay* HealthOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HealthOverlay"));
		UProgressBar* HealthGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HealthGauge"));
		UTextBlock* HealthText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HealthText"));
		USizeBox* HydrationBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HydrationBox"));
		UOverlay* HydrationOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HydrationOverlay"));
		UProgressBar* HydrationGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HydrationGauge"));
		UTextBlock* HydrationText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HydrationText"));
		USizeBox* HungerBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("HungerBox"));
		UOverlay* HungerOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HungerOverlay"));
		UProgressBar* HungerGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("HungerGauge"));
		UTextBlock* HungerText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("HungerText"));

		if (!RootSizeBox || !VitalsRow || !HealthBox || !HealthOverlay || !HealthGauge || !HealthText ||
			!HydrationBox || !HydrationOverlay || !HydrationGauge || !HydrationText ||
			!HungerBox || !HungerOverlay || !HungerGauge || !HungerText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(430.0f);
		RootSizeBox->SetHeightOverride(42.0f);
		RootSizeBox->SetContent(VitalsRow);

		HealthBox->SetWidthOverride(210.0f);
		HealthBox->SetHeightOverride(34.0f);
		HealthBox->SetContent(HealthOverlay);
		HealthGauge->SetPercent(1.0f);
		HealthGauge->SetFillColorAndOpacity(FLinearColor(0.96f, 0.32f, 0.36f, 1.0f));
		UOverlaySlot* HealthGaugeSlot = HealthOverlay->AddChildToOverlay(HealthGauge);
		if (HealthGaugeSlot)
		{
			HealthGaugeSlot->SetHorizontalAlignment(HAlign_Fill);
			HealthGaugeSlot->SetVerticalAlignment(VAlign_Fill);
		}
		ConfigureTextBlock(HealthText, FText::FromString(TEXT("HP 100 / 100")), FLinearColor::White, 16);
		UOverlaySlot* HealthTextSlot = HealthOverlay->AddChildToOverlay(HealthText);
		if (HealthTextSlot)
		{
			HealthTextSlot->SetHorizontalAlignment(HAlign_Center);
			HealthTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		HydrationBox->SetWidthOverride(96.0f);
		HydrationBox->SetHeightOverride(34.0f);
		HydrationBox->SetContent(HydrationOverlay);
		HydrationGauge->SetPercent(1.0f);
		HydrationGauge->SetFillColorAndOpacity(FLinearColor(0.30f, 0.65f, 0.98f, 1.0f));
		UOverlaySlot* HydrationGaugeSlot = HydrationOverlay->AddChildToOverlay(HydrationGauge);
		if (HydrationGaugeSlot)
		{
			HydrationGaugeSlot->SetHorizontalAlignment(HAlign_Fill);
			HydrationGaugeSlot->SetVerticalAlignment(VAlign_Fill);
		}
		ConfigureTextBlock(HydrationText, FText::FromString(TEXT("수분 100")), FLinearColor::White, 14);
		UOverlaySlot* HydrationTextSlot = HydrationOverlay->AddChildToOverlay(HydrationText);
		if (HydrationTextSlot)
		{
			HydrationTextSlot->SetHorizontalAlignment(HAlign_Center);
			HydrationTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		HungerBox->SetWidthOverride(112.0f);
		HungerBox->SetHeightOverride(34.0f);
		HungerBox->SetContent(HungerOverlay);
		HungerGauge->SetPercent(1.0f);
		HungerGauge->SetFillColorAndOpacity(FLinearColor(0.92f, 0.58f, 0.22f, 1.0f));
		UOverlaySlot* HungerGaugeSlot = HungerOverlay->AddChildToOverlay(HungerGauge);
		if (HungerGaugeSlot)
		{
			HungerGaugeSlot->SetHorizontalAlignment(HAlign_Fill);
			HungerGaugeSlot->SetVerticalAlignment(VAlign_Fill);
		}
		ConfigureTextBlock(HungerText, FText::FromString(TEXT("배부름 100")), FLinearColor::White, 14);
		UOverlaySlot* HungerTextSlot = HungerOverlay->AddChildToOverlay(HungerText);
		if (HungerTextSlot)
		{
			HungerTextSlot->SetHorizontalAlignment(HAlign_Center);
			HungerTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		UHorizontalBoxSlot* HealthBoxSlot = VitalsRow->AddChildToHorizontalBox(HealthBox);
		if (HealthBoxSlot)
		{
			HealthBoxSlot->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* HydrationBoxSlot = VitalsRow->AddChildToHorizontalBox(HydrationBox);
		if (HydrationBoxSlot)
		{
			HydrationBoxSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
			HydrationBoxSlot->SetVerticalAlignment(VAlign_Center);
		}
		UHorizontalBoxSlot* HungerBoxSlot = VitalsRow->AddChildToHorizontalBox(HungerBox);
		if (HungerBoxSlot)
		{
			HungerBoxSlot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
			HungerBoxSlot->SetVerticalAlignment(VAlign_Center);
		}

		RegisterWidgetVariable(WidgetBlueprint, HealthText);
		RegisterWidgetVariable(WidgetBlueprint, HungerText);
		RegisterWidgetVariable(WidgetBlueprint, HydrationText);
		RegisterWidgetVariable(WidgetBlueprint, HealthGauge);
		RegisterWidgetVariable(WidgetBlueprint, HungerGauge);
		RegisterWidgetVariable(WidgetBlueprint, HydrationGauge);
		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
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
		UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RootCanvas"));
		UHorizontalBox* AmmoSelectorPanel = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AmmoSelectorPanel"));
		UBorder* AmmoSelectorPromptBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AmmoSelectorPromptBackground"));
		UTextBlock* AmmoSelectorPromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoSelectorPromptText"));
		UBorder* AmmoSelectorKeyBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AmmoSelectorKeyBackground"));
		UTextBlock* AmmoSelectorKeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AmmoSelectorKeyText"));
		USizeBox* ReloadProgressPanel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ReloadProgressPanel"));
		UProgressBar* ReloadProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ReloadProgressBar"));
		UHorizontalBox* SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SlotRow"));
		if (!RootSizeBox || !RootCanvas || !AmmoSelectorPanel || !AmmoSelectorPromptBackground || !AmmoSelectorPromptText ||
			!AmmoSelectorKeyBackground || !AmmoSelectorKeyText || !ReloadProgressPanel || !ReloadProgressBar || !SlotRow)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(694.0f);
		RootSizeBox->SetHeightOverride(174.0f);
		RootSizeBox->SetContent(RootCanvas);

		AmmoSelectorPanel->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* AmmoSelectorSlot = RootCanvas->AddChildToCanvas(AmmoSelectorPanel);
		if (AmmoSelectorSlot)
		{
			AmmoSelectorSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			AmmoSelectorSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			AmmoSelectorSlot->SetPosition(FVector2D(0.0f, 28.0f));
			AmmoSelectorSlot->SetAutoSize(true);
			AmmoSelectorSlot->SetSize(FVector2D(0.0f, 26.0f));
		}

		AmmoSelectorPromptBackground->SetPadding(FMargin(8.0f, 3.0f));
		AmmoSelectorPromptBackground->SetVisibility(ESlateVisibility::Collapsed);
		AmmoSelectorPromptBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(98.0f, 24.0f),
			FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
			FLinearColor(0.7f, 0.85f, 0.55f, 1.0f),
			1.0f));
		ConfigureTextBlock(AmmoSelectorPromptText, FText::FromString(TEXT("탄약 미지정")), FLinearColor(0.9f, 0.96f, 0.88f, 1.0f), 11);
		AmmoSelectorPromptBackground->SetContent(AmmoSelectorPromptText);
		UHorizontalBoxSlot* PromptSlot = AmmoSelectorPanel->AddChildToHorizontalBox(AmmoSelectorPromptBackground);
		if (PromptSlot)
		{
			PromptSlot->SetVerticalAlignment(VAlign_Center);
		}

		for (int32 OptionNumber = 1; OptionNumber <= 6; ++OptionNumber)
		{
			UBorder* OptionBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*FString::Printf(TEXT("AmmoOption%dBackground"), OptionNumber)));
			UTextBlock* OptionText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*FString::Printf(TEXT("AmmoOption%dText"), OptionNumber)));
			if (!OptionBackground || !OptionText)
			{
				return false;
			}

			OptionBackground->SetPadding(FMargin(8.0f, 3.0f));
			OptionBackground->SetVisibility(ESlateVisibility::Collapsed);
			OptionBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(96.0f, 24.0f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				FLinearColor(0.7f, 0.85f, 0.55f, 1.0f),
				1.0f));
			ConfigureTextBlock(OptionText, FText::GetEmpty(), FLinearColor(0.9f, 0.96f, 0.88f, 1.0f), 11);
			OptionBackground->SetContent(OptionText);

			UHorizontalBoxSlot* OptionSlot = AmmoSelectorPanel->AddChildToHorizontalBox(OptionBackground);
			if (OptionSlot)
			{
				OptionSlot->SetPadding(FMargin(OptionNumber == 1 ? 0.0f : 4.0f, 0.0f, 0.0f, 0.0f));
				OptionSlot->SetVerticalAlignment(VAlign_Center);
			}

			RegisterWidgetVariable(WidgetBlueprint, OptionBackground);
			RegisterWidgetVariable(WidgetBlueprint, OptionText);
		}

		AmmoSelectorKeyBackground->SetPadding(FMargin(7.0f, 2.0f));
		AmmoSelectorKeyBackground->SetVisibility(ESlateVisibility::Collapsed);
		AmmoSelectorKeyBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(22.0f, 22.0f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
			0.0f));
		ConfigureTextBlock(AmmoSelectorKeyText, FText::FromString(TEXT("T")), FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 11);
		AmmoSelectorKeyBackground->SetContent(AmmoSelectorKeyText);
		UHorizontalBoxSlot* KeySlot = AmmoSelectorPanel->AddChildToHorizontalBox(AmmoSelectorKeyBackground);
		if (KeySlot)
		{
			KeySlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 0.0f));
			KeySlot->SetVerticalAlignment(VAlign_Center);
		}

		ReloadProgressPanel->SetWidthOverride(420.0f);
		ReloadProgressPanel->SetHeightOverride(10.0f);
		ReloadProgressPanel->SetVisibility(ESlateVisibility::Collapsed);
		ReloadProgressBar->SetPercent(0.0f);
		ReloadProgressBar->SetFillColorAndOpacity(FLinearColor(0.62f, 0.98f, 0.62f, 1.0f));
		ReloadProgressPanel->SetContent(ReloadProgressBar);
		UCanvasPanelSlot* ReloadProgressSlot = RootCanvas->AddChildToCanvas(ReloadProgressPanel);
		if (ReloadProgressSlot)
		{
			ReloadProgressSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			ReloadProgressSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			ReloadProgressSlot->SetPosition(FVector2D(0.0f, 34.0f));
			ReloadProgressSlot->SetSize(FVector2D(420.0f, 10.0f));
		}

		UCanvasPanelSlot* SlotRowCanvasSlot = RootCanvas->AddChildToCanvas(SlotRow);
		if (SlotRowCanvasSlot)
		{
			SlotRowCanvasSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
			SlotRowCanvasSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			SlotRowCanvasSlot->SetPosition(FVector2D(0.0f, 0.0f));
			SlotRowCanvasSlot->SetSize(FVector2D(690.0f, 126.0f));
		}

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
		const FString MeleeDefaultIconPath = TEXT("/Game/UI/Icons/T_UIIcon_CombatKnife.T_UIIcon_CombatKnife");

		for (int32 DisplaySlotIndex = 0; DisplaySlotIndex < 9; ++DisplaySlotIndex)
		{
			const bool bMeleeSlot = DisplaySlotIndex == 2;
			const int32 SlotNumber = bMeleeSlot ? INDEX_NONE : (DisplaySlotIndex < 2 ? DisplaySlotIndex + 1 : DisplaySlotIndex);
			const FString SlotWidgetPrefix = bMeleeSlot
				? FString(TEXT("QuickSlotMelee"))
				: FString::Printf(TEXT("QuickSlot%d"), SlotNumber);
			const bool bWeaponSlot = !bMeleeSlot && SlotNumber <= 2;
			const float SlotSize = bWeaponSlot ? 82.0f : 66.0f;
			const float IconSize = bWeaponSlot ? 68.0f : 54.0f;
			const FString DefaultIconPath = bMeleeSlot ? MeleeDefaultIconPath : DefaultIconPaths[SlotNumber - 1];
			const FText SlotLabelText = bMeleeSlot ? FText::FromString(TEXT("V")) : FText::AsNumber(SlotNumber);

			UVerticalBox* SlotStack = WidgetTree->ConstructWidget<UVerticalBox>(
				UVerticalBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("Stack"))));
			USizeBox* SlotAmmoTypeContainer = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTypeContainer"))));
			UHorizontalBox* SlotAmmoTypeRow = WidgetTree->ConstructWidget<UHorizontalBox>(
				UHorizontalBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTypeRow"))));
			UBorder* SlotAmmoTypeBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTypeBackground"))));
			UTextBlock* SlotAmmoTypeText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTypeText"))));
			UBorder* SlotAmmoKeyBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoKeyBackground"))));
			UTextBlock* SlotAmmoKeyText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoKeyText"))));
			USizeBox* SlotAmmoTextContainer = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTextContainer"))));
			UBorder* SlotAmmoTextBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoTextBackground"))));
			UTextBlock* SlotAmmoText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("AmmoText"))));
			USizeBox* SlotSizeBox = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("SizeBox"))));
			UBorder* SlotBackground = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("Background"))));
			UOverlay* SlotOverlay = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("Overlay"))));
			UImage* SlotIcon = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("Icon"))));
			UTextBlock* SlotNumberText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("NumberText"))));
			UBorder* SelectionFrame = WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(),
				FName(*(SlotWidgetPrefix + TEXT("SelectionFrame"))));

			if (!SlotStack || !SlotAmmoTypeContainer || !SlotAmmoTypeRow || !SlotAmmoTypeBackground || !SlotAmmoTypeText ||
				!SlotAmmoKeyBackground || !SlotAmmoKeyText || !SlotAmmoTextContainer || !SlotAmmoTextBackground || !SlotAmmoText ||
				!SlotSizeBox || !SlotBackground || !SlotOverlay || !SlotIcon || !SlotNumberText || !SelectionFrame)
			{
				return false;
			}

			SlotAmmoTypeContainer->SetWidthOverride(SlotSize);
			SlotAmmoTypeContainer->SetHeightOverride(18.0f);
			SlotAmmoTypeContainer->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoTypeContainer->SetClipping(EWidgetClipping::ClipToBounds);
			SlotAmmoTypeBackground->SetPadding(FMargin(5.0f, 2.0f));
			SlotAmmoTypeBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(62.0f, 17.0f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				0.0f));
			ConfigureTextBlock(SlotAmmoTypeText, FText::GetEmpty(), FLinearColor(0.92f, 0.96f, 0.88f, 1.0f), 8);
			SlotAmmoTypeText->SetShadowOffset(FVector2D(0.0f, 1.0f));
			SlotAmmoTypeText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
			SlotAmmoTypeText->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoTypeBackground->SetContent(SlotAmmoTypeText);
			UHorizontalBoxSlot* AmmoTypeTextSlot = SlotAmmoTypeRow->AddChildToHorizontalBox(SlotAmmoTypeBackground);
			if (AmmoTypeTextSlot)
			{
				AmmoTypeTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
				AmmoTypeTextSlot->SetVerticalAlignment(VAlign_Center);
			}
			SlotAmmoKeyBackground->SetPadding(FMargin(4.0f, 1.0f));
			SlotAmmoKeyBackground->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoKeyBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(15.0f, 15.0f),
				FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
				FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
				0.0f));
			ConfigureTextBlock(SlotAmmoKeyText, FText::FromString(TEXT("T")), FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 8);
			SlotAmmoKeyText->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoKeyBackground->SetContent(SlotAmmoKeyText);
			UHorizontalBoxSlot* AmmoKeySlot = SlotAmmoTypeRow->AddChildToHorizontalBox(SlotAmmoKeyBackground);
			if (AmmoKeySlot)
			{
				AmmoKeySlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
				AmmoKeySlot->SetVerticalAlignment(VAlign_Center);
				AmmoKeySlot->SetPadding(FMargin(2.0f, 0.0f, 0.0f, 0.0f));
			}
			SlotAmmoTypeContainer->SetContent(SlotAmmoTypeRow);
			UVerticalBoxSlot* AmmoTypeSlot = SlotStack->AddChildToVerticalBox(SlotAmmoTypeContainer);
			if (AmmoTypeSlot)
			{
				AmmoTypeSlot->SetHorizontalAlignment(HAlign_Center);
				AmmoTypeSlot->SetPadding(FMargin(0.0f));
			}

			SlotAmmoTextContainer->SetWidthOverride(bWeaponSlot ? 64.0f : SlotSize);
			SlotAmmoTextContainer->SetHeightOverride(18.0f);
			SlotAmmoTextContainer->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoTextContainer->SetClipping(EWidgetClipping::ClipToBounds);
			SlotAmmoTextBackground->SetPadding(FMargin(6.0f, 2.0f));
			SlotAmmoTextBackground->SetBrush(MakeRoundedBoxBrush(
				FVector2D(64.0f, 18.0f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				FLinearColor(0.018f, 0.022f, 0.028f, 0.92f),
				0.0f));
			ConfigureTextBlock(SlotAmmoText, FText::GetEmpty(), FLinearColor(0.95f, 0.97f, 1.0f, 1.0f), 11);
			SlotAmmoText->SetShadowOffset(FVector2D(0.0f, 1.0f));
			SlotAmmoText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
			SlotAmmoText->SetVisibility(ESlateVisibility::Collapsed);
			SlotAmmoTextBackground->SetContent(SlotAmmoText);
			SlotAmmoTextContainer->SetContent(SlotAmmoTextBackground);
			UVerticalBoxSlot* AmmoTextSlot = SlotStack->AddChildToVerticalBox(SlotAmmoTextContainer);
			if (AmmoTextSlot)
			{
				AmmoTextSlot->SetHorizontalAlignment(HAlign_Center);
				AmmoTextSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
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

			if (UTexture2D* DefaultIcon = LoadObject<UTexture2D>(nullptr, *DefaultIconPath))
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

			ConfigureTextBlock(SlotNumberText, SlotLabelText, FLinearColor(0.82f, 0.88f, 0.94f, 1.0f), bWeaponSlot ? 16 : 13);
			UOverlaySlot* NumberSlot = SlotOverlay->AddChildToOverlay(SlotNumberText);
			if (NumberSlot)
			{
				NumberSlot->SetHorizontalAlignment(HAlign_Left);
				NumberSlot->SetVerticalAlignment(VAlign_Top);
				NumberSlot->SetPadding(FMargin(3.0f, 1.0f, 0.0f, 0.0f));
			}

			SelectionFrame->SetVisibility(!bMeleeSlot && SlotNumber == 1 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
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

			UVerticalBoxSlot* SlotBoxStackSlot = SlotStack->AddChildToVerticalBox(SlotSizeBox);
			if (SlotBoxStackSlot)
			{
				SlotBoxStackSlot->SetHorizontalAlignment(HAlign_Center);
			}

			UHorizontalBoxSlot* SlotRowSlot = SlotRow->AddChildToHorizontalBox(SlotStack);
			if (SlotRowSlot)
			{
				SlotRowSlot->SetPadding(FMargin(DisplaySlotIndex == 0 ? 0.0f : 8.0f, 0.0f, 0.0f, 0.0f));
				SlotRowSlot->SetVerticalAlignment(VAlign_Bottom);
			}

			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTypeContainer);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTypeRow);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTypeBackground);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTypeText);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoKeyBackground);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoKeyText);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTextContainer);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoTextBackground);
			RegisterWidgetVariable(WidgetBlueprint, SlotAmmoText);
			RegisterWidgetVariable(WidgetBlueprint, SlotIcon);
			RegisterWidgetVariable(WidgetBlueprint, SelectionFrame);
		}

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorPanel);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorPromptBackground);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorPromptText);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorKeyBackground);
		RegisterWidgetVariable(WidgetBlueprint, AmmoSelectorKeyText);
		RegisterWidgetVariable(WidgetBlueprint, ReloadProgressPanel);
		RegisterWidgetVariable(WidgetBlueprint, ReloadProgressBar);
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
		UHorizontalBox* RootRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RootRow"));
		USizeBox* MainInventorySizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MainInventorySizeBox"));
		UBorder* InventoryPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryPanel"));
		UVerticalBox* InventoryStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryStack"));
		UHorizontalBox* InventoryHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryHeaderRow"));
		UTextBlock* InventoryTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryTitleText"));
		UButton* SortInventoryButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("SortInventoryButton"));
		UTextBlock* SortInventoryButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SortInventoryButtonText"));
		USizeBox* EquipmentReserveSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("EquipmentReserveSizeBox"));
		UTileView* EquipmentReserveTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("EquipmentReserveTileView"));
		USizeBox* AuxiliaryBagPanel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AuxiliaryBagPanel"));
		UBorder* AuxiliaryBagBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AuxiliaryBagBackground"));
		UTileView* AuxiliaryBagTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("AuxiliaryBagTileView"));
		UTileView* InventoryTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("InventoryTileView"));
		UBorder* InventoryWeightPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryWeightPanel"));
		UHorizontalBox* InventoryWeightRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InventoryWeightRow"));
		UTextBlock* InventoryWeightLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightLabelText"));
		USizeBox* InventoryWeightGaugeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InventoryWeightGaugeBox"));
		UProgressBar* InventoryWeightGauge = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("InventoryWeightGauge"));
		UTextBlock* InventoryWeightText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightText"));
		UBorder* InventoryWeightWarningIcon = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InventoryWeightWarningIcon"));
		UTextBlock* InventoryWeightWarningText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InventoryWeightWarningText"));

		if (!RootSizeBox || !RootRow || !MainInventorySizeBox || !InventoryPanel || !InventoryStack || !InventoryHeaderRow ||
			!InventoryTitleText || !SortInventoryButton || !SortInventoryButtonText || !EquipmentReserveSizeBox ||
			!EquipmentReserveTileView || !AuxiliaryBagPanel || !AuxiliaryBagBackground || !AuxiliaryBagTileView || !InventoryTileView ||
			!InventoryWeightPanel || !InventoryWeightRow || !InventoryWeightLabelText || !InventoryWeightGaugeBox ||
			!InventoryWeightGauge || !InventoryWeightText || !InventoryWeightWarningIcon || !InventoryWeightWarningText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(InventoryAreaPanelWidth);
		RootSizeBox->SetContent(RootRow);

		MainInventorySizeBox->SetWidthOverride(InventoryPanelWidth);
		MainInventorySizeBox->SetContent(InventoryPanel);
		UHorizontalBoxSlot* MainInventorySlot = RootRow->AddChildToHorizontalBox(MainInventorySizeBox);
		if (MainInventorySlot)
		{
			MainInventorySlot->SetVerticalAlignment(VAlign_Fill);
		}

		InventoryPanel->SetPadding(FMargin(InventoryPanelPadding));
		InventoryPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(InventoryPanelWidth, 620.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.28f, 0.36f, 0.44f, 1.0f),
			1.0f));
		InventoryPanel->SetContent(InventoryStack);

		ConfigureTextBlockLeft(InventoryTitleText, FText::FromString(TEXT("Inventory")), FLinearColor::White, 18);
		UHorizontalBoxSlot* TitleSlot = InventoryHeaderRow->AddChildToHorizontalBox(InventoryTitleText);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Center);
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		FButtonStyle SortButtonStyle;
		SortButtonStyle.SetNormal(MakeRoundedBoxBrush(
			FVector2D(76.0f, 30.0f),
			FLinearColor(0.10f, 0.17f, 0.20f, 0.96f),
			FLinearColor(0.34f, 0.48f, 0.56f, 1.0f),
			1.0f));
		SortButtonStyle.SetHovered(MakeRoundedBoxBrush(
			FVector2D(76.0f, 30.0f),
			FLinearColor(0.14f, 0.25f, 0.29f, 0.98f),
			FLinearColor(0.56f, 0.74f, 0.80f, 1.0f),
			1.5f));
		SortButtonStyle.SetPressed(MakeRoundedBoxBrush(
			FVector2D(76.0f, 30.0f),
			FLinearColor(0.07f, 0.12f, 0.15f, 1.0f),
			FLinearColor(0.28f, 0.40f, 0.48f, 1.0f),
			1.0f));
		SortButtonStyle.SetNormalPadding(FMargin(8.0f, 3.0f));
		SortButtonStyle.SetPressedPadding(FMargin(8.0f, 4.0f, 8.0f, 2.0f));
		SortInventoryButton->SetStyle(SortButtonStyle);
		SortInventoryButton->SetClickMethod(EButtonClickMethod::DownAndUp);
		ConfigureTextBlock(SortInventoryButtonText, FText::FromString(TEXT("\uC815\uB9AC")), FLinearColor::White, 14);
		SortInventoryButton->SetContent(SortInventoryButtonText);
		UHorizontalBoxSlot* SortButtonSlot = InventoryHeaderRow->AddChildToHorizontalBox(SortInventoryButton);
		if (SortButtonSlot)
		{
			SortButtonSlot->SetHorizontalAlignment(HAlign_Right);
			SortButtonSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* HeaderSlot = InventoryStack->AddChildToVerticalBox(InventoryHeaderRow);
		if (HeaderSlot)
		{
			HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
			HeaderSlot->SetVerticalAlignment(VAlign_Top);
		}

		EquipmentReserveTileView->SetEntryWidth(EquipmentReserveEntryWidth);
		EquipmentReserveTileView->SetEntryHeight(InventoryTileHeight);
		SetListViewEntryWidgetClass(EquipmentReserveTileView, EntryWidgetClass);
		EquipmentReserveSizeBox->SetHeightOverride(EquipmentReserveHeight);
		EquipmentReserveSizeBox->SetContent(EquipmentReserveTileView);
		UVerticalBoxSlot* ReserveRowSlot = InventoryStack->AddChildToVerticalBox(EquipmentReserveSizeBox);
		if (ReserveRowSlot)
		{
			ReserveRowSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 12.0f));
			ReserveRowSlot->SetHorizontalAlignment(HAlign_Fill);
			ReserveRowSlot->SetVerticalAlignment(VAlign_Top);
		}

		AuxiliaryBagPanel->SetWidthOverride(AuxiliaryBagPanelWidth);
		AuxiliaryBagPanel->SetHeightOverride(AuxiliaryBagPanelHeight);
		AuxiliaryBagPanel->SetContent(AuxiliaryBagBackground);
		AuxiliaryBagBackground->SetPadding(FMargin(AuxiliaryBagPanelPadding));
		AuxiliaryBagBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(AuxiliaryBagPanelWidth, AuxiliaryBagPanelHeight),
			FLinearColor(0.02f, 0.025f, 0.03f, 0.88f),
			FLinearColor(0.28f, 0.44f, 0.36f, 1.0f),
			1.0f));
		AuxiliaryBagTileView->SetEntryWidth(InventoryTileWidth);
		AuxiliaryBagTileView->SetEntryHeight(InventoryTileHeight);
		SetListViewEntryWidgetClass(AuxiliaryBagTileView, EntryWidgetClass);
		AuxiliaryBagBackground->SetContent(AuxiliaryBagTileView);
		UHorizontalBoxSlot* BagSlot = RootRow->AddChildToHorizontalBox(AuxiliaryBagPanel);
		if (BagSlot)
		{
			BagSlot->SetPadding(FMargin(AuxiliaryBagPanelGap, 0.0f, 0.0f, 0.0f));
			BagSlot->SetVerticalAlignment(VAlign_Top);
		}

		InventoryTileView->SetEntryWidth(InventoryTileWidth);
		InventoryTileView->SetEntryHeight(InventoryTileHeight);
		SetListViewEntryWidgetClass(InventoryTileView, EntryWidgetClass);
		UVerticalBoxSlot* InventoryTileSlot = InventoryStack->AddChildToVerticalBox(InventoryTileView);
		if (InventoryTileSlot)
		{
			InventoryTileSlot->SetHorizontalAlignment(HAlign_Fill);
			InventoryTileSlot->SetVerticalAlignment(VAlign_Fill);
			InventoryTileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		InventoryWeightPanel->SetPadding(FMargin(10.0f, 6.0f));
		InventoryWeightPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(InventoryTileViewWidth, 36.0f),
			FLinearColor(0.005f, 0.008f, 0.010f, 0.72f),
			FLinearColor(0.18f, 0.24f, 0.26f, 0.85f),
			1.0f));
		InventoryWeightPanel->SetContent(InventoryWeightRow);

		ConfigureTextBlockLeft(InventoryWeightLabelText, FText::FromString(TEXT("소지 중량")), FLinearColor(0.92f, 0.96f, 0.94f, 1.0f), 13);
		UHorizontalBoxSlot* WeightLabelSlot = InventoryWeightRow->AddChildToHorizontalBox(InventoryWeightLabelText);
		if (WeightLabelSlot)
		{
			WeightLabelSlot->SetVerticalAlignment(VAlign_Center);
			WeightLabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		InventoryWeightGauge->SetPercent(0.0f);
		InventoryWeightGauge->SetFillColorAndOpacity(FLinearColor(0.60f, 0.84f, 0.36f, 1.0f));
		InventoryWeightGaugeBox->SetHeightOverride(16.0f);
		InventoryWeightGaugeBox->SetContent(InventoryWeightGauge);
		UHorizontalBoxSlot* WeightGaugeSlot = InventoryWeightRow->AddChildToHorizontalBox(InventoryWeightGaugeBox);
		if (WeightGaugeSlot)
		{
			WeightGaugeSlot->SetPadding(FMargin(10.0f, 0.0f, 10.0f, 0.0f));
			WeightGaugeSlot->SetVerticalAlignment(VAlign_Center);
			WeightGaugeSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ConfigureTextBlock(InventoryWeightText, FText::FromString(TEXT("0/50kg")), FLinearColor::White, 13);
		UHorizontalBoxSlot* WeightTextSlot = InventoryWeightRow->AddChildToHorizontalBox(InventoryWeightText);
		if (WeightTextSlot)
		{
			WeightTextSlot->SetVerticalAlignment(VAlign_Center);
			WeightTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		InventoryWeightWarningIcon->SetVisibility(ESlateVisibility::Hidden);
		InventoryWeightWarningIcon->SetPadding(FMargin(6.0f, 2.0f));
		InventoryWeightWarningIcon->SetBrush(MakeRoundedBoxBrush(
			FVector2D(58.0f, 22.0f),
			FLinearColor(0.86f, 0.18f, 0.08f, 0.95f),
			FLinearColor(1.0f, 0.70f, 0.20f, 1.0f),
			1.0f));
		ConfigureTextBlock(InventoryWeightWarningText, FText::FromString(TEXT("과중량")), FLinearColor::White, 11);
		InventoryWeightWarningIcon->SetContent(InventoryWeightWarningText);
		UHorizontalBoxSlot* WeightWarningSlot = InventoryWeightRow->AddChildToHorizontalBox(InventoryWeightWarningIcon);
		if (WeightWarningSlot)
		{
			WeightWarningSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			WeightWarningSlot->SetVerticalAlignment(VAlign_Center);
			WeightWarningSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		UVerticalBoxSlot* WeightPanelSlot = InventoryStack->AddChildToVerticalBox(InventoryWeightPanel);
		if (WeightPanelSlot)
		{
			WeightPanelSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
			WeightPanelSlot->SetHorizontalAlignment(HAlign_Fill);
			WeightPanelSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		RegisterWidgetVariable(WidgetBlueprint, RootSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, RootRow);
		RegisterWidgetVariable(WidgetBlueprint, MainInventorySizeBox);
		RegisterWidgetVariable(WidgetBlueprint, InventoryPanel);
		RegisterWidgetVariable(WidgetBlueprint, InventoryStack);
		RegisterWidgetVariable(WidgetBlueprint, InventoryHeaderRow);
		RegisterWidgetVariable(WidgetBlueprint, InventoryTitleText);
		RegisterWidgetVariable(WidgetBlueprint, SortInventoryButton);
		RegisterWidgetVariable(WidgetBlueprint, SortInventoryButtonText);
		RegisterWidgetVariable(WidgetBlueprint, EquipmentReserveSizeBox);
		RegisterWidgetVariable(WidgetBlueprint, AuxiliaryBagPanel);
		RegisterWidgetVariable(WidgetBlueprint, AuxiliaryBagBackground);
		RegisterWidgetVariable(WidgetBlueprint, EquipmentReserveTileView);
		RegisterWidgetVariable(WidgetBlueprint, AuxiliaryBagTileView);
		RegisterWidgetVariable(WidgetBlueprint, InventoryTileView);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightPanel);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightText);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightGauge);
		RegisterWidgetVariable(WidgetBlueprint, InventoryWeightWarningIcon);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudItemInfoPanelWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass)
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
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderRow"));
		USizeBox* SelectedItemIconContainer = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SelectedItemIconContainer"));
		UImage* SelectedItemIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SelectedItemIconImage"));
		UTextBlock* SelectedItemNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemNameText"));
		UButton* CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* CloseButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseButtonText"));
		UTextBlock* SelectedItemDescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SelectedItemDescriptionText"));
		UBorder* ModdingPanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModdingPanel"));
		UVerticalBox* ModdingStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ModdingStack"));
		UTextBlock* ModdingText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModdingText"));
		UTileView* AttachmentSlotTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("AttachmentSlotTileView"));

		if (!RootSizeBox || !PanelBackground || !PanelStack || !HeaderRow ||
			!SelectedItemIconContainer || !SelectedItemIconImage || !SelectedItemNameText || !CloseButton || !CloseButtonText ||
			!SelectedItemDescriptionText || !ModdingPanel || !ModdingStack || !ModdingText ||
			!AttachmentSlotTileView)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(330.0f);
		RootSizeBox->SetContent(PanelBackground);

		PanelBackground->SetPadding(FMargin(LootContainerPanelPadding));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(330.0f, 620.0f),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.36f, 0.34f, 0.54f, 1.0f),
			1.0f));
		PanelBackground->SetContent(PanelStack);

		ConfigureTextBlockLeft(SelectedItemNameText, FText::FromString(TEXT("No Item")), FLinearColor::White, 20);
		UHorizontalBoxSlot* NameSlot = HeaderRow->AddChildToHorizontalBox(SelectedItemNameText);
		if (NameSlot)
		{
			NameSlot->SetHorizontalAlignment(HAlign_Fill);
			NameSlot->SetVerticalAlignment(VAlign_Center);
			NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		FButtonStyle CloseButtonStyle;
		CloseButtonStyle.SetNormal(MakeRoundedBoxBrush(
			FVector2D(58.0f, 28.0f),
			FLinearColor(0.10f, 0.12f, 0.16f, 0.96f),
			FLinearColor(0.42f, 0.42f, 0.58f, 1.0f),
			1.0f));
		CloseButtonStyle.SetHovered(MakeRoundedBoxBrush(
			FVector2D(58.0f, 28.0f),
			FLinearColor(0.18f, 0.20f, 0.28f, 0.98f),
			FLinearColor(0.72f, 0.72f, 0.90f, 1.0f),
			1.5f));
		CloseButtonStyle.SetPressed(MakeRoundedBoxBrush(
			FVector2D(58.0f, 28.0f),
			FLinearColor(0.07f, 0.08f, 0.12f, 1.0f),
			FLinearColor(0.34f, 0.34f, 0.48f, 1.0f),
			1.0f));
		CloseButtonStyle.SetNormalPadding(FMargin(8.0f, 2.0f));
		CloseButtonStyle.SetPressedPadding(FMargin(8.0f, 3.0f, 8.0f, 1.0f));
		CloseButton->SetStyle(CloseButtonStyle);
		CloseButton->SetClickMethod(EButtonClickMethod::DownAndUp);
		ConfigureTextBlock(CloseButtonText, FText::FromString(TEXT("\uB2EB\uAE30")), FLinearColor(0.90f, 0.94f, 0.96f, 1.0f), 13);
		CloseButton->SetContent(CloseButtonText);
		UHorizontalBoxSlot* CloseSlot = HeaderRow->AddChildToHorizontalBox(CloseButton);
		if (CloseSlot)
		{
			CloseSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			CloseSlot->SetHorizontalAlignment(HAlign_Right);
			CloseSlot->SetVerticalAlignment(VAlign_Center);
		}

		UVerticalBoxSlot* HeaderSlot = PanelStack->AddChildToVerticalBox(HeaderRow);
		if (HeaderSlot)
		{
			HeaderSlot->SetHorizontalAlignment(HAlign_Fill);
			HeaderSlot->SetVerticalAlignment(VAlign_Top);
		}

		SelectedItemIconContainer->SetWidthOverride(132.0f);
		SelectedItemIconContainer->SetHeightOverride(132.0f);
		SelectedItemIconImage->SetOpacity(0.0f);
		SelectedItemIconContainer->SetContent(SelectedItemIconImage);
		UVerticalBoxSlot* IconSlot = PanelStack->AddChildToVerticalBox(SelectedItemIconContainer);
		if (IconSlot)
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
			IconSlot->SetVerticalAlignment(VAlign_Top);
			IconSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			IconSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 8.0f));
		}

		ConfigureTextBlockLeft(SelectedItemDescriptionText, FText::GetEmpty(), FLinearColor(0.75f, 0.8f, 0.86f, 1.0f), 15);
		SelectedItemDescriptionText->SetAutoWrapText(true);
		UVerticalBoxSlot* DescriptionSlot = PanelStack->AddChildToVerticalBox(SelectedItemDescriptionText);
		if (DescriptionSlot)
		{
			DescriptionSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
			DescriptionSlot->SetHorizontalAlignment(HAlign_Fill);
			DescriptionSlot->SetVerticalAlignment(VAlign_Top);
			DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ModdingPanel->SetVisibility(ESlateVisibility::Collapsed);
		ModdingPanel->SetPadding(FMargin(10.0f));
		ModdingPanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(300.0f, 150.0f),
			FLinearColor(0.03f, 0.034f, 0.04f, 0.92f),
			FLinearColor(0.56f, 0.50f, 0.78f, 1.0f),
			1.0f));
		ConfigureTextBlockLeft(ModdingText, FText::FromString(TEXT("Modding")), FLinearColor::White, 15);
		ModdingPanel->SetContent(ModdingStack);
		UVerticalBoxSlot* ModdingTextSlot = ModdingStack->AddChildToVerticalBox(ModdingText);
		if (ModdingTextSlot)
		{
			ModdingTextSlot->SetHorizontalAlignment(HAlign_Fill);
			ModdingTextSlot->SetVerticalAlignment(VAlign_Top);
		}
		AttachmentSlotTileView->SetEntryWidth(96.0f);
		AttachmentSlotTileView->SetEntryHeight(96.0f);
		SetListViewEntryWidgetClass(AttachmentSlotTileView, EntryWidgetClass);
		UVerticalBoxSlot* AttachmentSlot = ModdingStack->AddChildToVerticalBox(AttachmentSlotTileView);
		if (AttachmentSlot)
		{
			AttachmentSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
			AttachmentSlot->SetHorizontalAlignment(HAlign_Fill);
			AttachmentSlot->SetVerticalAlignment(VAlign_Top);
		}

		UVerticalBoxSlot* ModdingSlot = PanelStack->AddChildToVerticalBox(ModdingPanel);
		if (ModdingSlot)
		{
			ModdingSlot->SetHorizontalAlignment(HAlign_Fill);
			ModdingSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		RegisterWidgetVariable(WidgetBlueprint, PanelStack);
		RegisterWidgetVariable(WidgetBlueprint, HeaderRow);
		RegisterWidgetVariable(WidgetBlueprint, SelectedItemIconContainer);
		RegisterWidgetVariable(WidgetBlueprint, SelectedItemIconImage);
		RegisterWidgetVariable(WidgetBlueprint, SelectedItemNameText);
		RegisterWidgetVariable(WidgetBlueprint, CloseButton);
		RegisterWidgetVariable(WidgetBlueprint, CloseButtonText);
		RegisterWidgetVariable(WidgetBlueprint, SelectedItemDescriptionText);
		RegisterWidgetVariable(WidgetBlueprint, ModdingPanel);
		RegisterWidgetVariable(WidgetBlueprint, ModdingText);
		RegisterWidgetVariable(WidgetBlueprint, AttachmentSlotTileView);
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
		UHorizontalBox* ContainerHeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ContainerHeaderRow"));
		UTextBlock* ContainerTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContainerTitleText"));
		UTextBlock* ContainerOccupancyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ContainerOccupancyText"));
		UTileView* ContainerTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("ContainerTileView"));

		if (!RootSizeBox || !PanelBackground || !PanelStack || !ContainerHeaderRow ||
			!ContainerTitleText || !ContainerOccupancyText || !ContainerTileView)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(LootContainerPanelWidth);
		RootSizeBox->SetHeightOverride(LootContainerPanelHeaderHeight + 2.0f * LootContainerTileHeight);
		RootSizeBox->SetContent(PanelBackground);

		PanelBackground->SetPadding(FMargin(14.0f));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(LootContainerPanelWidth, LootContainerPanelHeaderHeight + 2.0f * LootContainerTileHeight),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.90f),
			FLinearColor(0.44f, 0.34f, 0.26f, 1.0f),
			1.0f));
		PanelBackground->SetContent(PanelStack);

		ConfigureTextBlockLeft(ContainerTitleText, FText::FromString(TEXT("Container")), FLinearColor::White, 18);
		ConfigureTextBlockLeft(ContainerOccupancyText, FText::FromString(TEXT("(0/0)")), FLinearColor(0.92f, 0.94f, 0.96f, 1.0f), 18);

		UHorizontalBoxSlot* TitleTextSlot = ContainerHeaderRow->AddChildToHorizontalBox(ContainerTitleText);
		if (TitleTextSlot)
		{
			TitleTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			TitleTextSlot->SetHorizontalAlignment(HAlign_Left);
			TitleTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		UHorizontalBoxSlot* OccupancyTextSlot = ContainerHeaderRow->AddChildToHorizontalBox(ContainerOccupancyText);
		if (OccupancyTextSlot)
		{
			OccupancyTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			OccupancyTextSlot->SetHorizontalAlignment(HAlign_Left);
			OccupancyTextSlot->SetVerticalAlignment(VAlign_Center);
			OccupancyTextSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
		}

		UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(ContainerHeaderRow);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Top);
		}

		ContainerTileView->SetEntryWidth(LootContainerTileWidth);
		ContainerTileView->SetEntryHeight(LootContainerTileHeight);
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
		RegisterWidgetVariable(WidgetBlueprint, ContainerHeaderRow);
		RegisterWidgetVariable(WidgetBlueprint, ContainerTitleText);
		RegisterWidgetVariable(WidgetBlueprint, ContainerOccupancyText);
		RegisterWidgetVariable(WidgetBlueprint, ContainerTileView);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildWorkbenchPanelWidgetTree(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UUserWidget> EntryWidgetClass)
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
		UTextBlock* WorkbenchTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WorkbenchTitleText"));
		UHorizontalBox* BodyRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BodyRow"));
		UBorder* LeftPanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LeftPanelBackground"));
		UOverlay* LeftModeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("LeftModeOverlay"));
		UBorder* RightPanelBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RightPanelBackground"));
		UOverlay* RightModeOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("RightModeOverlay"));

		UVerticalBox* CraftLeftStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CraftLeftStack"));
		UTileView* CraftRecipeTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("CraftRecipeTileView"));
		UVerticalBox* DismantleLeftStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DismantleLeftStack"));
		UTextBlock* DismantleInventoryHeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleInventoryHeaderText"));
		UTileView* DismantleInventoryTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("DismantleInventoryTileView"));
		UTextBlock* DismantleStorageHeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleStorageHeaderText"));
		UTileView* DismantleStorageTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("DismantleStorageTileView"));
		UVerticalBox* BlueprintLeftStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BlueprintLeftStack"));
		UTileView* BlueprintItemTileView = WidgetTree->ConstructWidget<UTileView>(UTileView::StaticClass(), TEXT("BlueprintItemTileView"));

		UVerticalBox* CraftRightStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CraftRightStack"));
		UTextBlock* CraftMaterialsTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftMaterialsTitleText"));
		UVerticalBox* CraftIngredientList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CraftIngredientList"));
		UTextBlock* CraftArrowText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftArrowText"));
		UHorizontalBox* CraftOutputRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CraftOutputRow"));
		USizeBox* CraftOutputImageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CraftOutputImageBox"));
		UImage* CraftOutputImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CraftOutputImage"));
		UTextBlock* CraftOutputText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftOutputText"));
		UButton* CraftButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CraftButton"));
		UTextBlock* CraftButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CraftButtonText"));

		UVerticalBox* DismantleRightStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DismantleRightStack"));
		UTextBlock* DismantleResultTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleResultTitleText"));
		UTextBlock* DismantleResultText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleResultText"));
		UButton* DismantleButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DismantleButton"));
		UTextBlock* DismantleButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DismantleButtonText"));

		UVerticalBox* BlueprintRightStack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BlueprintRightStack"));
		UTextBlock* BlueprintGuideText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BlueprintGuideText"));
		UTextBlock* BlueprintRegisterText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BlueprintRegisterText"));
		UButton* BlueprintRegisterButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BlueprintRegisterButton"));
		UTextBlock* BlueprintRegisterButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BlueprintRegisterButtonText"));

		if (!RootSizeBox || !PanelBackground || !PanelStack || !WorkbenchTitleText || !BodyRow ||
			!LeftPanelBackground || !LeftModeOverlay || !RightPanelBackground || !RightModeOverlay ||
			!CraftLeftStack || !CraftRecipeTileView || !DismantleLeftStack || !DismantleInventoryHeaderText ||
			!DismantleInventoryTileView || !DismantleStorageHeaderText || !DismantleStorageTileView ||
			!BlueprintLeftStack || !BlueprintItemTileView || !CraftRightStack || !CraftMaterialsTitleText ||
			!CraftIngredientList || !CraftArrowText || !CraftOutputRow || !CraftOutputImageBox || !CraftOutputImage ||
			!CraftOutputText || !CraftButton || !CraftButtonText || !DismantleRightStack || !DismantleResultTitleText ||
			!DismantleResultText || !DismantleButton || !DismantleButtonText || !BlueprintRightStack ||
			!BlueprintGuideText || !BlueprintRegisterText || !BlueprintRegisterButton || !BlueprintRegisterButtonText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(WorkbenchPanelWidth);
		RootSizeBox->SetHeightOverride(WorkbenchPanelHeight);
		RootSizeBox->SetContent(PanelBackground);

		PanelBackground->SetPadding(FMargin(WorkbenchPanelPadding));
		PanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(WorkbenchPanelWidth, WorkbenchPanelHeight),
			FLinearColor(0.012f, 0.014f, 0.017f, 0.92f),
			FLinearColor(0.22f, 0.42f, 0.56f, 1.0f),
			1.0f));
		PanelBackground->SetContent(PanelStack);

		ConfigureTextBlockLeft(WorkbenchTitleText, FText::FromString(TEXT("\uC81C\uC870")), FLinearColor::White, 22);
		UVerticalBoxSlot* TitleSlot = PanelStack->AddChildToVerticalBox(WorkbenchTitleText);
		if (TitleSlot)
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Fill);
			TitleSlot->SetVerticalAlignment(VAlign_Top);
		}

		UVerticalBoxSlot* BodySlot = PanelStack->AddChildToVerticalBox(BodyRow);
		if (BodySlot)
		{
			BodySlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			BodySlot->SetHorizontalAlignment(HAlign_Fill);
			BodySlot->SetVerticalAlignment(VAlign_Fill);
			BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		LeftPanelBackground->SetPadding(FMargin(10.0f));
		LeftPanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(WorkbenchLeftPanelWidth, WorkbenchPanelHeight - 72.0f),
			FLinearColor(0.025f, 0.030f, 0.034f, 0.94f),
			FLinearColor(0.18f, 0.22f, 0.26f, 1.0f),
			1.0f));
		LeftPanelBackground->SetContent(LeftModeOverlay);

		RightPanelBackground->SetPadding(FMargin(14.0f));
		RightPanelBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(WorkbenchPanelWidth - WorkbenchLeftPanelWidth - 54.0f, WorkbenchPanelHeight - 72.0f),
			FLinearColor(0.020f, 0.024f, 0.028f, 0.94f),
			FLinearColor(0.18f, 0.22f, 0.26f, 1.0f),
			1.0f));
		RightPanelBackground->SetContent(RightModeOverlay);

		UHorizontalBoxSlot* LeftPanelSlot = BodyRow->AddChildToHorizontalBox(LeftPanelBackground);
		if (LeftPanelSlot)
		{
			LeftPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			LeftPanelSlot->SetHorizontalAlignment(HAlign_Left);
			LeftPanelSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UHorizontalBoxSlot* RightPanelSlot = BodyRow->AddChildToHorizontalBox(RightPanelBackground);
		if (RightPanelSlot)
		{
			RightPanelSlot->SetPadding(FMargin(14.0f, 0.0f, 0.0f, 0.0f));
			RightPanelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			RightPanelSlot->SetHorizontalAlignment(HAlign_Fill);
			RightPanelSlot->SetVerticalAlignment(VAlign_Fill);
		}

		auto ConfigureWorkbenchTileView = [EntryWidgetClass](UTileView* TileView, float Height)
		{
			TileView->SetEntryWidth(WorkbenchTileWidth);
			TileView->SetEntryHeight(WorkbenchTileHeight);
			TileView->SetWheelScrollMultiplier(0.55f);
			SetListViewEntryWidgetClass(TileView, EntryWidgetClass);
			if (USizeBox* TileViewSizeBox = Cast<USizeBox>(TileView->GetParent()))
			{
				TileViewSizeBox->SetWidthOverride(WorkbenchTileViewWidth);
				TileViewSizeBox->SetHeightOverride(Height);
			}
		};

		auto AddModeStackToOverlay = [](UOverlay* Overlay, UWidget* Stack)
		{
			UOverlaySlot* StackSlot = Overlay->AddChildToOverlay(Stack);
			if (StackSlot)
			{
				StackSlot->SetHorizontalAlignment(HAlign_Fill);
				StackSlot->SetVerticalAlignment(VAlign_Fill);
			}
		};

		USizeBox* CraftTileViewBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CraftTileViewBox"));
		CraftTileViewBox->SetContent(CraftRecipeTileView);
		UVerticalBoxSlot* CraftTileSlot = CraftLeftStack->AddChildToVerticalBox(CraftTileViewBox);
		if (CraftTileSlot)
		{
			CraftTileSlot->SetHorizontalAlignment(HAlign_Fill);
			CraftTileSlot->SetVerticalAlignment(VAlign_Fill);
			CraftTileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		ConfigureWorkbenchTileView(CraftRecipeTileView, WorkbenchTileViewHeight);

		ConfigureTextBlockLeft(DismantleInventoryHeaderText, FText::FromString(TEXT("\uC778\uBCA4\uD1A0\uB9AC")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 16);
		ConfigureTextBlockLeft(DismantleStorageHeaderText, FText::FromString(TEXT("\uCC3D\uACE0")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 16);
		DismantleLeftStack->AddChildToVerticalBox(DismantleInventoryHeaderText);
		USizeBox* DismantleInventoryBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DismantleInventoryBox"));
		DismantleInventoryBox->SetContent(DismantleInventoryTileView);
		UVerticalBoxSlot* DismantleInventorySlot = DismantleLeftStack->AddChildToVerticalBox(DismantleInventoryBox);
		if (DismantleInventorySlot)
		{
			DismantleInventorySlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 12.0f));
			DismantleInventorySlot->SetHorizontalAlignment(HAlign_Fill);
			DismantleInventorySlot->SetVerticalAlignment(VAlign_Top);
		}
		DismantleLeftStack->AddChildToVerticalBox(DismantleStorageHeaderText);
		USizeBox* DismantleStorageBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DismantleStorageBox"));
		DismantleStorageBox->SetContent(DismantleStorageTileView);
		UVerticalBoxSlot* DismantleStorageSlot = DismantleLeftStack->AddChildToVerticalBox(DismantleStorageBox);
		if (DismantleStorageSlot)
		{
			DismantleStorageSlot->SetPadding(FMargin(0.0f, 6.0f, 0.0f, 0.0f));
			DismantleStorageSlot->SetHorizontalAlignment(HAlign_Fill);
			DismantleStorageSlot->SetVerticalAlignment(VAlign_Top);
		}
		ConfigureWorkbenchTileView(DismantleInventoryTileView, 204.0f);
		ConfigureWorkbenchTileView(DismantleStorageTileView, 204.0f);

		USizeBox* BlueprintTileViewBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BlueprintTileViewBox"));
		BlueprintTileViewBox->SetContent(BlueprintItemTileView);
		UVerticalBoxSlot* BlueprintTileSlot = BlueprintLeftStack->AddChildToVerticalBox(BlueprintTileViewBox);
		if (BlueprintTileSlot)
		{
			BlueprintTileSlot->SetHorizontalAlignment(HAlign_Fill);
			BlueprintTileSlot->SetVerticalAlignment(VAlign_Fill);
			BlueprintTileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		ConfigureWorkbenchTileView(BlueprintItemTileView, WorkbenchTileViewHeight);

		AddModeStackToOverlay(LeftModeOverlay, CraftLeftStack);
		AddModeStackToOverlay(LeftModeOverlay, DismantleLeftStack);
		AddModeStackToOverlay(LeftModeOverlay, BlueprintLeftStack);

		auto ConfigureActionButton = [](UButton* Button, UTextBlock* ButtonText, const FText& Text)
		{
			FButtonStyle ButtonStyle;
			ButtonStyle.SetNormal(MakeRoundedBoxBrush(FVector2D(180.0f, 44.0f), FLinearColor(0.05f, 0.33f, 0.78f, 1.0f), FLinearColor(0.45f, 0.68f, 0.95f, 1.0f), 1.0f));
			ButtonStyle.SetHovered(MakeRoundedBoxBrush(FVector2D(180.0f, 44.0f), FLinearColor(0.08f, 0.42f, 0.92f, 1.0f), FLinearColor(0.70f, 0.86f, 1.0f, 1.0f), 1.5f));
			ButtonStyle.SetPressed(MakeRoundedBoxBrush(FVector2D(180.0f, 44.0f), FLinearColor(0.04f, 0.24f, 0.62f, 1.0f), FLinearColor(0.36f, 0.56f, 0.84f, 1.0f), 1.0f));
			Button->SetStyle(ButtonStyle);
			Button->SetContent(ButtonText);
			ConfigureTextBlock(ButtonText, Text, FLinearColor::White, 17);
		};

		ConfigureTextBlockLeft(CraftMaterialsTitleText, FText::FromString(TEXT("\uC7AC\uB8CC")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 18);
		CraftRightStack->AddChildToVerticalBox(CraftMaterialsTitleText);
		UVerticalBoxSlot* IngredientSlot = CraftRightStack->AddChildToVerticalBox(CraftIngredientList);
		if (IngredientSlot)
		{
			IngredientSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
			IngredientSlot->SetHorizontalAlignment(HAlign_Fill);
			IngredientSlot->SetVerticalAlignment(VAlign_Top);
		}
		ConfigureTextBlock(CraftArrowText, FText::FromString(TEXT("\u2193")), FLinearColor(0.68f, 0.82f, 0.96f, 1.0f), 34);
		UVerticalBoxSlot* ArrowSlot = CraftRightStack->AddChildToVerticalBox(CraftArrowText);
		if (ArrowSlot)
		{
			ArrowSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 12.0f));
			ArrowSlot->SetHorizontalAlignment(HAlign_Center);
			ArrowSlot->SetVerticalAlignment(VAlign_Top);
		}
		CraftOutputImageBox->SetWidthOverride(84.0f);
		CraftOutputImageBox->SetHeightOverride(84.0f);
		CraftOutputImageBox->SetContent(CraftOutputImage);
		CraftOutputImage->SetOpacity(0.0f);
		UHorizontalBoxSlot* OutputImageSlot = CraftOutputRow->AddChildToHorizontalBox(CraftOutputImageBox);
		if (OutputImageSlot)
		{
			OutputImageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			OutputImageSlot->SetHorizontalAlignment(HAlign_Left);
			OutputImageSlot->SetVerticalAlignment(VAlign_Center);
		}
		ConfigureTextBlockLeft(CraftOutputText, FText::GetEmpty(), FLinearColor::White, 18);
		UHorizontalBoxSlot* OutputTextSlot = CraftOutputRow->AddChildToHorizontalBox(CraftOutputText);
		if (OutputTextSlot)
		{
			OutputTextSlot->SetPadding(FMargin(14.0f, 0.0f, 0.0f, 0.0f));
			OutputTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			OutputTextSlot->SetHorizontalAlignment(HAlign_Fill);
			OutputTextSlot->SetVerticalAlignment(VAlign_Center);
		}
		CraftRightStack->AddChildToVerticalBox(CraftOutputRow);
		ConfigureActionButton(CraftButton, CraftButtonText, FText::FromString(TEXT("\uC81C\uC870")));
		UVerticalBoxSlot* CraftButtonSlot = CraftRightStack->AddChildToVerticalBox(CraftButton);
		if (CraftButtonSlot)
		{
			CraftButtonSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
			CraftButtonSlot->SetHorizontalAlignment(HAlign_Right);
			CraftButtonSlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlockLeft(DismantleResultTitleText, FText::FromString(TEXT("\uBD84\uD574 \uACB0\uACFC")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 18);
		DismantleRightStack->AddChildToVerticalBox(DismantleResultTitleText);
		ConfigureTextBlockLeft(DismantleResultText, FText::GetEmpty(), FLinearColor::White, 17);
		UVerticalBoxSlot* DismantleResultSlot = DismantleRightStack->AddChildToVerticalBox(DismantleResultText);
		if (DismantleResultSlot)
		{
			DismantleResultSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			DismantleResultSlot->SetHorizontalAlignment(HAlign_Fill);
			DismantleResultSlot->SetVerticalAlignment(VAlign_Top);
		}
		ConfigureActionButton(DismantleButton, DismantleButtonText, FText::FromString(TEXT("\uBD84\uD574")));
		UVerticalBoxSlot* DismantleButtonSlot = DismantleRightStack->AddChildToVerticalBox(DismantleButton);
		if (DismantleButtonSlot)
		{
			DismantleButtonSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
			DismantleButtonSlot->SetHorizontalAlignment(HAlign_Right);
			DismantleButtonSlot->SetVerticalAlignment(VAlign_Top);
		}

		ConfigureTextBlockLeft(BlueprintGuideText, FText::FromString(TEXT("\uC124\uACC4\uB3C4 \uC544\uC774\uD15C")), FLinearColor(0.84f, 0.90f, 0.94f, 1.0f), 18);
		BlueprintRightStack->AddChildToVerticalBox(BlueprintGuideText);
		ConfigureTextBlockLeft(BlueprintRegisterText, FText::GetEmpty(), FLinearColor::White, 17);
		UVerticalBoxSlot* BlueprintTextSlot = BlueprintRightStack->AddChildToVerticalBox(BlueprintRegisterText);
		if (BlueprintTextSlot)
		{
			BlueprintTextSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			BlueprintTextSlot->SetHorizontalAlignment(HAlign_Fill);
			BlueprintTextSlot->SetVerticalAlignment(VAlign_Top);
		}
		ConfigureActionButton(BlueprintRegisterButton, BlueprintRegisterButtonText, FText::FromString(TEXT("\uC124\uACC4\uB3C4 \uB4F1\uB85D")));
		UVerticalBoxSlot* BlueprintButtonSlot = BlueprintRightStack->AddChildToVerticalBox(BlueprintRegisterButton);
		if (BlueprintButtonSlot)
		{
			BlueprintButtonSlot->SetPadding(FMargin(0.0f, 24.0f, 0.0f, 0.0f));
			BlueprintButtonSlot->SetHorizontalAlignment(HAlign_Right);
			BlueprintButtonSlot->SetVerticalAlignment(VAlign_Top);
		}

		AddModeStackToOverlay(RightModeOverlay, CraftRightStack);
		AddModeStackToOverlay(RightModeOverlay, DismantleRightStack);
		AddModeStackToOverlay(RightModeOverlay, BlueprintRightStack);

		RegisterAllWidgetsInTree(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return true;
	}

	bool BuildHudExternalPanelWidgetTree(
		UWidgetBlueprint* WidgetBlueprint,
		TSubclassOf<UUserWidget> LootContainerWidgetClass,
		TSubclassOf<UUserWidget> WorkbenchPanelWidgetClass)
	{
		if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !LootContainerWidgetClass || !WorkbenchPanelWidgetClass)
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
		UOverlay* WorkbenchPanel = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("WorkbenchPanel"));
		UUserWidget* WorkbenchPanelWidget = WidgetTree->ConstructWidget<UUserWidget>(WorkbenchPanelWidgetClass, TEXT("WorkbenchPanelWidget"));
		UBorder* ShopPanel = BuildHudSimplePanel(
			WidgetTree,
			TEXT("ShopPanel"),
			FText::FromString(TEXT("Shop")),
			FVector2D(LootContainerPanelWidth, 620.0f),
			FLinearColor(0.28f, 0.40f, 0.50f, 1.0f));
		UBorder* StoragePanel = BuildHudSimplePanel(
			WidgetTree,
			TEXT("StoragePanel"),
			FText::FromString(TEXT("Storage")),
			FVector2D(LootContainerPanelWidth, 620.0f),
			FLinearColor(0.38f, 0.42f, 0.32f, 1.0f));

		if (!RootSizeBox || !PanelOverlay || !LootingBoxPanel || !LootContainerWidget ||
			!WorkbenchPanel || !WorkbenchPanelWidget || !ShopPanel || !StoragePanel)
		{
			return false;
		}

		WidgetTree->RootWidget = RootSizeBox;
		RootSizeBox->SetWidthOverride(WorkbenchPanelWidth);
		RootSizeBox->SetContent(PanelOverlay);

		UOverlaySlot* LootContainerSlot = LootingBoxPanel->AddChildToOverlay(LootContainerWidget);
		if (LootContainerSlot)
		{
			LootContainerSlot->SetHorizontalAlignment(HAlign_Fill);
			LootContainerSlot->SetVerticalAlignment(VAlign_Top);
		}

		UOverlaySlot* WorkbenchSlot = WorkbenchPanel->AddChildToOverlay(WorkbenchPanelWidget);
		if (WorkbenchSlot)
		{
			WorkbenchSlot->SetHorizontalAlignment(HAlign_Fill);
			WorkbenchSlot->SetVerticalAlignment(VAlign_Top);
		}

		TArray<UWidget*> ExternalPanels = { LootingBoxPanel, WorkbenchPanel, ShopPanel, StoragePanel };
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
		RegisterWidgetVariable(WidgetBlueprint, WorkbenchPanel);
		RegisterWidgetVariable(WidgetBlueprint, ShopPanel);
		RegisterWidgetVariable(WidgetBlueprint, StoragePanel);
		RegisterWidgetVariable(WidgetBlueprint, LootContainerWidget);
		RegisterWidgetVariable(WidgetBlueprint, WorkbenchPanelWidget);
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
		UCanvasPanel* CenterContentPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CenterContentPanel"));
		UUserWidget* InventoryAreaWidget = WidgetTree->ConstructWidget<UUserWidget>(InventoryAreaWidgetClass, TEXT("InventoryAreaWidget"));
		UUserWidget* ItemInfoPanelWidget = WidgetTree->ConstructWidget<UUserWidget>(ItemInfoPanelWidgetClass, TEXT("ItemInfoPanelWidget"));
		UUserWidget* ExternalPanelWidget = WidgetTree->ConstructWidget<UUserWidget>(ExternalPanelWidgetClass, TEXT("ExternalPanelWidget"));
		UBorder* UnsupportedModePanel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("UnsupportedModePanel"));
		UTextBlock* UnsupportedModeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("UnsupportedModeText"));
		UTextBlock* ModeTitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ModeTitleText"));
		UHorizontalBox* BottomRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BottomRow"));
		UUserWidget* BottomStatusWidget = WidgetTree->ConstructWidget<UUserWidget>(BottomStatusWidgetClass, TEXT("BottomStatusWidget"));
		USizeBox* BottomGap = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BottomGap"));
		UUserWidget* QuickSlotBarWidget = WidgetTree->ConstructWidget<UUserWidget>(QuickSlotBarWidgetClass, TEXT("QuickSlotBarWidget"));
		USizeBox* CenterReloadGaugeRoot = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CenterReloadGaugeRoot"));
		UCanvasPanel* CenterReloadGaugeCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CenterReloadGaugeCanvas"));
		UBorder* CenterReloadGaugeBackdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CenterReloadGaugeBackdrop"));
		UTunaSweeperReloadRingWidget* CenterReloadRingWidget = WidgetTree->ConstructWidget<UTunaSweeperReloadRingWidget>(
			UTunaSweeperReloadRingWidget::StaticClass(),
			TEXT("CenterReloadRingWidget"));
		UTextBlock* CenterReloadPercentText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CenterReloadPercentText"));
		UHorizontalBox* CenterReloadPromptRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CenterReloadPromptRoot"));
		UTextBlock* CenterReloadPromptText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CenterReloadPromptText"));
		UBorder* CenterReloadPromptKeyBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CenterReloadPromptKeyBackground"));
		UTextBlock* CenterReloadPromptKeyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CenterReloadPromptKeyText"));

		if (!RootCanvas || !TopStatusReserveWidget || !CenterContentPanel || !InventoryAreaWidget || !ItemInfoPanelWidget ||
			!ExternalPanelWidget || !UnsupportedModePanel || !UnsupportedModeText || !ModeTitleText ||
			!BottomRow || !BottomStatusWidget || !BottomGap || !QuickSlotBarWidget ||
			!CenterReloadGaugeRoot || !CenterReloadGaugeCanvas || !CenterReloadGaugeBackdrop || !CenterReloadRingWidget || !CenterReloadPercentText ||
			!CenterReloadPromptRoot || !CenterReloadPromptText || !CenterReloadPromptKeyBackground || !CenterReloadPromptKeyText)
		{
			return false;
		}

		WidgetTree->RootWidget = RootCanvas;

		TopStatusReserveWidget->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* TopSlot = RootCanvas->AddChildToCanvas(TopStatusReserveWidget);
		if (TopSlot)
		{
			TopSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			TopSlot->SetOffsets(FMargin(0.0f, 16.0f, HudTopModeTabPanelWidth, HudTopModeTabPanelHeight));
			TopSlot->SetAlignment(FVector2D(0.5f, 0.0f));
		}

		ModeTitleText->SetVisibility(ESlateVisibility::Collapsed);
		ConfigureTextBlockLeft(ModeTitleText, FText::GetEmpty(), FLinearColor(0.92f, 0.98f, 1.0f, 0.96f), 52);
		TunaSweeperUIFont::ApplyFont(ModeTitleText, 52.0f, ETunaSweeperUIFontWeight::Bold);
		ModeTitleText->SetShadowOffset(FVector2D(2.0f, 2.0f));
		ModeTitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.86f));
		UCanvasPanelSlot* ModeTitleSlot = RootCanvas->AddChildToCanvas(ModeTitleText);
		if (ModeTitleSlot)
		{
			ModeTitleSlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			ModeTitleSlot->SetAlignment(FVector2D(0.0f, 0.0f));
			ModeTitleSlot->SetPosition(FVector2D(42.0f, 92.0f));
			ModeTitleSlot->SetSize(FVector2D(420.0f, 84.0f));
			ModeTitleSlot->SetZOrder(20);
		}

		CenterContentPanel->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CenterSlot = RootCanvas->AddChildToCanvas(CenterContentPanel);
		if (CenterSlot)
		{
			CenterSlot->SetAnchors(FAnchors(0.0f, 0.5f, 1.0f, 0.5f));
			CenterSlot->SetAlignment(FVector2D(0.0f, 0.5f));
			CenterSlot->SetOffsets(FMargin(34.0f, -20.0f, 34.0f, 620.0f));
		}

		UCanvasPanelSlot* InventorySlot = CenterContentPanel->AddChildToCanvas(InventoryAreaWidget);
		if (InventorySlot)
		{
			InventorySlot->SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
			InventorySlot->SetAlignment(FVector2D(0.0f, 0.0f));
			InventorySlot->SetPosition(FVector2D(0.0f, 0.0f));
			InventorySlot->SetSize(FVector2D(InventoryAreaPanelWidth, 620.0f));
		}

		ItemInfoPanelWidget->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* ItemInfoSlot = CenterContentPanel->AddChildToCanvas(ItemInfoPanelWidget);
		if (ItemInfoSlot)
		{
			ItemInfoSlot->SetAnchors(FAnchors(0.5f, 0.0f, 0.5f, 0.0f));
			ItemInfoSlot->SetAlignment(FVector2D(0.5f, 0.0f));
			ItemInfoSlot->SetPosition(FVector2D(0.0f, 0.0f));
			ItemInfoSlot->SetSize(FVector2D(330.0f, 620.0f));
		}

		UCanvasPanelSlot* ExternalSlot = CenterContentPanel->AddChildToCanvas(ExternalPanelWidget);
		if (ExternalSlot)
		{
			ExternalSlot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
			ExternalSlot->SetAlignment(FVector2D(1.0f, 0.0f));
			ExternalSlot->SetPosition(FVector2D(0.0f, 0.0f));
			ExternalSlot->SetSize(FVector2D(WorkbenchPanelWidth, 620.0f));
		}

		UnsupportedModePanel->SetVisibility(ESlateVisibility::Collapsed);
		UnsupportedModePanel->SetPadding(FMargin(24.0f, 12.0f));
		UnsupportedModePanel->SetBrush(MakeRoundedBoxBrush(
			FVector2D(144.0f, 52.0f),
			FLinearColor(0.0f, 0.0f, 0.0f, 0.62f),
			FLinearColor(0.28f, 0.32f, 0.34f, 0.52f),
			1.0f,
			6.0f));
		ConfigureTextBlock(UnsupportedModeText, FText::FromString(TEXT("\uBBF8\uAD6C\uD604")), FLinearColor(0.88f, 0.92f, 0.94f, 0.96f), 18);
		UnsupportedModePanel->SetContent(UnsupportedModeText);
		UCanvasPanelSlot* UnsupportedSlot = CenterContentPanel->AddChildToCanvas(UnsupportedModePanel);
		if (UnsupportedSlot)
		{
			UnsupportedSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			UnsupportedSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			UnsupportedSlot->SetPosition(FVector2D(0.0f, 0.0f));
			UnsupportedSlot->SetSize(FVector2D(144.0f, 52.0f));
		}

		UCanvasPanelSlot* BottomSlot = RootCanvas->AddChildToCanvas(BottomRow);
		if (BottomSlot)
		{
			BottomSlot->SetAnchors(FAnchors(0.5f, 1.0f, 0.5f, 1.0f));
			BottomSlot->SetAlignment(FVector2D(0.5f, 1.0f));
			BottomSlot->SetPosition(FVector2D(0.0f, -20.0f));
			BottomSlot->SetSize(FVector2D(980.0f, 172.0f));
		}

		CenterReloadGaugeRoot->SetWidthOverride(96.0f);
		CenterReloadGaugeRoot->SetHeightOverride(96.0f);
		CenterReloadGaugeRoot->SetContent(CenterReloadGaugeCanvas);
		CenterReloadGaugeRoot->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CenterReloadSlot = RootCanvas->AddChildToCanvas(CenterReloadGaugeRoot);
		if (CenterReloadSlot)
		{
			CenterReloadSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterReloadSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterReloadSlot->SetPosition(FVector2D(0.0f, 0.0f));
			CenterReloadSlot->SetSize(FVector2D(96.0f, 96.0f));
		}

		CenterReloadGaugeBackdrop->SetBrush(MakeRoundedBoxBrush(
			FVector2D(58.0f, 58.0f),
			FLinearColor(0.012f, 0.016f, 0.018f, 0.68f),
			FLinearColor(0.48f, 0.66f, 0.46f, 0.3f),
			1.0f));
		UCanvasPanelSlot* CenterReloadBackdropSlot = CenterReloadGaugeCanvas->AddChildToCanvas(CenterReloadGaugeBackdrop);
		if (CenterReloadBackdropSlot)
		{
			CenterReloadBackdropSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterReloadBackdropSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterReloadBackdropSlot->SetPosition(FVector2D(0.0f, 0.0f));
			CenterReloadBackdropSlot->SetSize(FVector2D(58.0f, 58.0f));
		}

		CenterReloadRingWidget->SetReloadProgress(0.0f, true);
		UCanvasPanelSlot* CenterReloadRingSlot = CenterReloadGaugeCanvas->AddChildToCanvas(CenterReloadRingWidget);
		if (CenterReloadRingSlot)
		{
			CenterReloadRingSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterReloadRingSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterReloadRingSlot->SetPosition(FVector2D(0.0f, 0.0f));
			CenterReloadRingSlot->SetSize(FVector2D(90.0f, 90.0f));
		}

		ConfigureTextBlock(CenterReloadPercentText, FText::GetEmpty(), FLinearColor(0.9f, 1.0f, 0.88f, 1.0f), 13);
		UCanvasPanelSlot* CenterReloadTextSlot = CenterReloadGaugeCanvas->AddChildToCanvas(CenterReloadPercentText);
		if (CenterReloadTextSlot)
		{
			CenterReloadTextSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterReloadTextSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterReloadTextSlot->SetPosition(FVector2D(0.0f, 0.0f));
			CenterReloadTextSlot->SetSize(FVector2D(54.0f, 24.0f));
		}

		CenterReloadPromptRoot->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* CenterReloadPromptSlot = RootCanvas->AddChildToCanvas(CenterReloadPromptRoot);
		if (CenterReloadPromptSlot)
		{
			CenterReloadPromptSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
			CenterReloadPromptSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CenterReloadPromptSlot->SetPosition(FVector2D(0.0f, 92.0f));
			CenterReloadPromptSlot->SetAutoSize(true);
		}

		ConfigureTextBlock(CenterReloadPromptText, FText::FromString(TEXT("재장전")), FLinearColor(0.92f, 0.96f, 1.0f, 1.0f), 18);
		UHorizontalBoxSlot* CenterReloadPromptTextSlot = CenterReloadPromptRoot->AddChildToHorizontalBox(CenterReloadPromptText);
		if (CenterReloadPromptTextSlot)
		{
			CenterReloadPromptTextSlot->SetVerticalAlignment(VAlign_Center);
		}

		CenterReloadPromptKeyBackground->SetPadding(FMargin(8.0f, 2.0f));
		CenterReloadPromptKeyBackground->SetBrush(MakeRoundedBoxBrush(
			FVector2D(28.0f, 24.0f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
			FLinearColor(1.0f, 1.0f, 1.0f, 0.96f),
			0.0f));
		ConfigureTextBlock(CenterReloadPromptKeyText, FText::FromString(TEXT("R")), FLinearColor(0.02f, 0.025f, 0.03f, 1.0f), 13);
		CenterReloadPromptKeyBackground->SetContent(CenterReloadPromptKeyText);
		UHorizontalBoxSlot* CenterReloadPromptKeySlot = CenterReloadPromptRoot->AddChildToHorizontalBox(CenterReloadPromptKeyBackground);
		if (CenterReloadPromptKeySlot)
		{
			CenterReloadPromptKeySlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			CenterReloadPromptKeySlot->SetVerticalAlignment(VAlign_Center);
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
		RegisterWidgetVariable(WidgetBlueprint, CenterReloadGaugeRoot);
		RegisterWidgetVariable(WidgetBlueprint, CenterReloadRingWidget);
		RegisterWidgetVariable(WidgetBlueprint, CenterReloadPromptRoot);
		RegisterWidgetVariable(WidgetBlueprint, CenterReloadPercentText);
		RegisterWidgetVariable(WidgetBlueprint, CenterContentPanel);
		RegisterWidgetVariable(WidgetBlueprint, InventoryAreaWidget);
		RegisterWidgetVariable(WidgetBlueprint, ItemInfoPanelWidget);
		RegisterWidgetVariable(WidgetBlueprint, ExternalPanelWidget);
		RegisterWidgetVariable(WidgetBlueprint, UnsupportedModePanel);
		RegisterWidgetVariable(WidgetBlueprint, UnsupportedModeText);
		RegisterWidgetVariable(WidgetBlueprint, ModeTitleText);
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
		UWidgetBlueprint* WorkbenchPanelWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			WorkbenchPanelWidgetAssetName,
			UTunaSweeperWorkbenchPanelWidget::StaticClass());

		if (!ItemThumbnailWidgetBlueprint || !TopReserveWidgetBlueprint || !BottomStatusWidgetBlueprint || !QuickSlotWidgetBlueprint ||
			!InventoryAreaWidgetBlueprint || !ItemInfoPanelWidgetBlueprint || !ExternalPanelWidgetBlueprint ||
			!LootContainerWidgetBlueprint || !WorkbenchPanelWidgetBlueprint)
		{
			return false;
		}

		if (!BuildItemThumbnailSlotWidgetTree(ItemThumbnailWidgetBlueprint))
		{
			return false;
		}
		RegisterAllWidgetsInTree(ItemThumbnailWidgetBlueprint);
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
			BuildHudItemInfoPanelWidgetTree(ItemInfoPanelWidgetBlueprint, ItemThumbnailWidgetClass) &&
			BuildLootContainerWidgetTree(LootContainerWidgetBlueprint, ItemThumbnailWidgetClass) &&
			BuildWorkbenchPanelWidgetTree(WorkbenchPanelWidgetBlueprint, ItemThumbnailWidgetClass);

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
			LootContainerWidgetBlueprint,
			WorkbenchPanelWidgetBlueprint
		})
		{
			RegisterAllWidgetsInTree(ChildWidgetBlueprint);
			FKismetEditorUtilities::CompileBlueprint(ChildWidgetBlueprint);
			ChildWidgetBlueprint->MarkPackageDirty();
			if (!SaveAsset(ChildWidgetBlueprint))
			{
				return false;
			}
		}

		if (!BuildHudExternalPanelWidgetTree(
			ExternalPanelWidgetBlueprint,
			LootContainerWidgetBlueprint->GeneratedClass.Get(),
			WorkbenchPanelWidgetBlueprint->GeneratedClass.Get()))
		{
			return false;
		}
		RegisterAllWidgetsInTree(ExternalPanelWidgetBlueprint);
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

		RegisterAllWidgetsInTree(GameHudWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(GameHudWidgetBlueprint);
		GameHudWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(GameHudWidgetBlueprint);
	}

	bool EnsureLootContainerOccupancyHeaderAssets()
	{
		UWidgetBlueprint* ItemThumbnailWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			ItemThumbnailSlotWidgetAssetName,
			UTunaSweeperItemThumbnailSlotWidget::StaticClass());
		UWidgetBlueprint* LootContainerWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			LootContainerWidgetAssetName,
			UTunaSweeperLootContainerWidget::StaticClass());
		if (!ItemThumbnailWidgetBlueprint || !LootContainerWidgetBlueprint)
		{
			return false;
		}

		if (!ItemThumbnailWidgetBlueprint->GeneratedClass)
		{
			FKismetEditorUtilities::CompileBlueprint(ItemThumbnailWidgetBlueprint);
		}

		const TSubclassOf<UUserWidget> ItemThumbnailWidgetClass = ItemThumbnailWidgetBlueprint->GeneratedClass.Get();
		if (!ItemThumbnailWidgetClass || !BuildLootContainerWidgetTree(LootContainerWidgetBlueprint, ItemThumbnailWidgetClass))
		{
			return false;
		}

		RegisterAllWidgetsInTree(LootContainerWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(LootContainerWidgetBlueprint);
		LootContainerWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(LootContainerWidgetBlueprint);
	}

	bool EnsureBackpackInventoryAssets()
	{
		if (!EnsureItemIconTextures() || !EnsureEquipmentIconTextures())
		{
			return false;
		}

		UWidgetBlueprint* ItemThumbnailWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			ItemThumbnailSlotWidgetAssetName,
			UTunaSweeperItemThumbnailSlotWidget::StaticClass());
		UWidgetBlueprint* InventoryAreaWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			HudInventoryAreaWidgetAssetName,
			UTunaSweeperHudInventoryAreaWidget::StaticClass());
		if (!ItemThumbnailWidgetBlueprint || !InventoryAreaWidgetBlueprint)
		{
			return false;
		}

		if (!BuildItemThumbnailSlotWidgetTree(ItemThumbnailWidgetBlueprint))
		{
			return false;
		}
		RegisterAllWidgetsInTree(ItemThumbnailWidgetBlueprint);
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

		if (!BuildHudInventoryAreaWidgetTree(InventoryAreaWidgetBlueprint, ItemThumbnailWidgetClass))
		{
			return false;
		}
		RegisterAllWidgetsInTree(InventoryAreaWidgetBlueprint);
		FKismetEditorUtilities::CompileBlueprint(InventoryAreaWidgetBlueprint);
		InventoryAreaWidgetBlueprint->MarkPackageDirty();
		return SaveAsset(InventoryAreaWidgetBlueprint);
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
		UHorizontalBox* LabelContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LabelContentRow"));
		UTextBlock* DisplayNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DisplayNameText"));
		UHorizontalBox* RequirementRoot = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RequirementRoot"));
		USizeBox* RequirementIconBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RequirementIconBox"));
		UImage* RequirementIconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RequirementIconImage"));
		UTextBlock* RequirementQuantityText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RequirementQuantityText"));

		if (!RootCanvas || !MarkerRoot || !MarkerSizeBox || !MarkerOverlay || !RingImage || !RingBrushImage ||
			!FilledImage || !FilledBrushImage || !LabelBackground || !LabelContentRow || !DisplayNameText ||
			!RequirementRoot || !RequirementIconBox || !RequirementIconImage || !RequirementQuantityText)
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
			RootSlot->SetSize(FVector2D(360.0f, 56.0f));
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
		LabelBackground->SetPadding(FMargin(12.0f, 4.0f, 10.0f, 4.0f));

		ConfigureTextBlock(DisplayNameText, FText::FromString(TEXT("Interact")), FLinearColor::Black, 18);
		LabelBackground->SetContent(LabelContentRow);

		if (UHorizontalBoxSlot* DisplayNameSlot = LabelContentRow->AddChildToHorizontalBox(DisplayNameText))
		{
			DisplayNameSlot->SetHorizontalAlignment(HAlign_Left);
			DisplayNameSlot->SetVerticalAlignment(VAlign_Center);
		}

		RequirementIconBox->SetWidthOverride(22.0f);
		RequirementIconBox->SetHeightOverride(22.0f);
		RequirementIconBox->SetContent(RequirementIconImage);
		RequirementIconImage->SetColorAndOpacity(FLinearColor::White);
		RequirementIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		RequirementIconImage->SetOpacity(0.0f);

		if (UHorizontalBoxSlot* RequirementIconSlot = RequirementRoot->AddChildToHorizontalBox(RequirementIconBox))
		{
			RequirementIconSlot->SetHorizontalAlignment(HAlign_Center);
			RequirementIconSlot->SetVerticalAlignment(VAlign_Center);
		}

		ConfigureTextBlock(RequirementQuantityText, FText::FromString(TEXT("x0")), FLinearColor::Black, 17);
		if (UHorizontalBoxSlot* RequirementQuantitySlot = RequirementRoot->AddChildToHorizontalBox(RequirementQuantityText))
		{
			RequirementQuantitySlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
			RequirementQuantitySlot->SetHorizontalAlignment(HAlign_Left);
			RequirementQuantitySlot->SetVerticalAlignment(VAlign_Center);
		}

		RequirementRoot->SetVisibility(ESlateVisibility::Collapsed);
		if (UHorizontalBoxSlot* RequirementSlot = LabelContentRow->AddChildToHorizontalBox(RequirementRoot))
		{
			RequirementSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
			RequirementSlot->SetHorizontalAlignment(HAlign_Left);
			RequirementSlot->SetVerticalAlignment(VAlign_Center);
		}

		UHorizontalBoxSlot* LabelSlot = MarkerRoot->AddChildToHorizontalBox(LabelBackground);
		if (LabelSlot)
		{
			LabelSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			LabelSlot->SetHorizontalAlignment(HAlign_Left);
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		RegisterAllWidgetsInTree(WidgetBlueprint);

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

	bool ConfigureIntroMenuWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !BuildTitleIntroMenuWidgetTree(WidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return SaveAsset(WidgetBlueprint);
	}

	bool ConfigureLevelTransitionVideoWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !BuildLevelTransitionVideoWidgetTree(WidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return SaveAsset(WidgetBlueprint);
	}

	bool ConfigureSpeechBubbleWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !BuildSpeechBubbleWidgetTree(WidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return SaveAsset(WidgetBlueprint);
	}

	bool ConfigureQuestWidgetBlueprint(UWidgetBlueprint* WidgetBlueprint)
	{
		if (!WidgetBlueprint || !BuildQuestWidgetTree(WidgetBlueprint))
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(WidgetBlueprint);
		WidgetBlueprint->MarkPackageDirty();
		return SaveAsset(WidgetBlueprint);
	}

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

	bool ConfigureLevelTravelActorInstance(
		AActor* Actor,
		FName TargetLevelName,
		const FText& DisplayName,
		TSoftObjectPtr<UMediaSource> TransitionMediaSource = TSoftObjectPtr<UMediaSource>(),
		const FText& TransitionMessage = FText::GetEmpty(),
		TSoftObjectPtr<UStaticMesh> VisualMeshOverride = TSoftObjectPtr<UStaticMesh>(),
		const FVector& VisualMeshScale = FVector(0.75f, 0.75f, 0.75f),
		const FVector& VisualMeshRelativeLocation = FVector::ZeroVector)
	{
		ATunaSweeperLevelTravelInteractableActor* LevelTravelActor = Cast<ATunaSweeperLevelTravelInteractableActor>(Actor);
		if (!LevelTravelActor)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s is not an ATunaSweeperLevelTravelInteractableActor."), *GetNameSafe(Actor));
			return false;
		}

		LevelTravelActor->Modify();
		LevelTravelActor->ConfigureLevelTravelDefaults(
			TargetLevelName,
			DisplayName,
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))),
			TransitionMediaSource,
			TSoftClassPtr<UTunaSweeperLevelTransitionWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, LevelTransitionVideoWidgetAssetName))),
			TransitionMessage);
		LevelTravelActor->ConfigureLevelTravelVisualDefaults(
			VisualMeshOverride,
			VisualMeshScale,
			VisualMeshRelativeLocation);
		LevelTravelActor->MarkPackageDirty();
		return true;
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

	bool ConfigureQuestNpcBlueprint(UBlueprint* QuestNpcBlueprint)
	{
		if (!QuestNpcBlueprint)
		{
			return false;
		}

		FKismetEditorUtilities::CompileBlueprint(QuestNpcBlueprint);

		ATunaSweeperQuestNpcActor* Defaults = QuestNpcBlueprint->GeneratedClass
			? Cast<ATunaSweeperQuestNpcActor>(QuestNpcBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to configure %s defaults."), *GetNameSafe(QuestNpcBlueprint));
			return false;
		}

		QuestNpcBlueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigureQuestNpcDefaults(
			UTunaSweeperQuestSubsystem::GetFirstOutingQuestId(),
			FText::FromString(TEXT("\uAD50\uAD00")),
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))),
			UTunaSweeperQuestSubsystem::GetInstructorProviderId());
		FBlueprintEditorUtils::MarkBlueprintAsModified(QuestNpcBlueprint);
		FKismetEditorUtilities::CompileBlueprint(QuestNpcBlueprint);
		QuestNpcBlueprint->MarkPackageDirty();
		return SaveAsset(QuestNpcBlueprint);
	}

	bool ConfigureQuestNpcActorInstance(AActor* Actor)
	{
		ATunaSweeperQuestNpcActor* QuestNpcActor = Cast<ATunaSweeperQuestNpcActor>(Actor);
		if (!QuestNpcActor)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("%s is not an ATunaSweeperQuestNpcActor."), *GetNameSafe(Actor));
			return false;
		}

		QuestNpcActor->Modify();
		QuestNpcActor->ConfigureQuestNpcDefaults(
			UTunaSweeperQuestSubsystem::GetFirstOutingQuestId(),
			FText::FromString(TEXT("\uAD50\uAD00")),
			TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
				FSoftObjectPath(GetAssetClassPath(UIAssetPath, InteractionMarkerAssetName))),
			UTunaSweeperQuestSubsystem::GetInstructorProviderId());
		QuestNpcActor->MarkPackageDirty();
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

	bool PlaceLevelTravelActor(
		UWorld* World,
		UBlueprint* ActorBlueprint,
		const FString& ActorLabel,
		const FVector& Location,
		FName TargetLevelName,
		const FText& DisplayName,
		TSoftObjectPtr<UMediaSource> TransitionMediaSource = TSoftObjectPtr<UMediaSource>(),
		const FText& TransitionMessage = FText::GetEmpty(),
		TSoftObjectPtr<UStaticMesh> VisualMeshOverride = TSoftObjectPtr<UStaticMesh>(),
		const FVector& VisualMeshScale = FVector(0.75f, 0.75f, 0.75f),
		const FVector& VisualMeshRelativeLocation = FVector::ZeroVector)
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
			return ConfigureLevelTravelActorInstance(
				ExistingActor,
				TargetLevelName,
				DisplayName,
				TransitionMediaSource,
				TransitionMessage,
				VisualMeshOverride,
				VisualMeshScale,
				VisualMeshRelativeLocation);
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
		if (!ConfigureLevelTravelActorInstance(
			SpawnedActor,
			TargetLevelName,
			DisplayName,
			TransitionMediaSource,
			TransitionMessage,
			VisualMeshOverride,
			VisualMeshScale,
			VisualMeshRelativeLocation))
		{
			return false;
		}
		SpawnedActor->MarkPackageDirty();
		return true;
	}

	bool PlaceQuestNpcActor(UWorld* World, UBlueprint* ActorBlueprint, const FString& ActorLabel, const FVector& Location)
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
			return ConfigureQuestNpcActorInstance(ExistingActor);
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
		if (!ConfigureQuestNpcActorInstance(SpawnedActor))
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

	bool PlaceLevelTravelActorsInBunkerAndRaidMaps(UBlueprint* LevelTravelBlueprint)
	{
		if (!LevelTravelBlueprint)
		{
			return false;
		}

		const TSoftObjectPtr<UMediaSource> BunkerToRaidMediaSource(
			FSoftObjectPath(GetAssetObjectPath(VideoAssetPath, BunkerToRaidMediaSourceAssetName)));
		if (!EnsureLevelTravelLadderMeshAsset())
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to create %s."), *LevelTravelLadderMeshAssetName);
			return false;
		}
		const TSoftObjectPtr<UStaticMesh> LevelTravelLadderMesh(
			FSoftObjectPath(GetAssetObjectPath(InteractionAssetPath, LevelTravelLadderMeshAssetName)));

		UWorld* BunkerWorld = LoadEditorMapForSetup(BunkerMapPackagePath);
		const bool bBunkerPlaced =
			BunkerWorld &&
			PlaceLevelTravelActor(
				BunkerWorld,
				LevelTravelBlueprint,
				TEXT("TS_Travel_DeployToRaid"),
				FVector(220.0f, -220.0f, 4.0f),
				FName(TEXT("RaidMap")),
				FText::FromString(TEXT("Deploy")),
				BunkerToRaidMediaSource,
				FText::FromString(TEXT("Deploying to Raid")),
				LevelTravelLadderMesh,
				FVector::OneVector,
				FVector::ZeroVector) &&
			UEditorLoadingAndSavingUtils::SaveMap(BunkerWorld, BunkerMapPackagePath);

		LoadEditorMapForSetup(IntroMapPackagePath);
		return bBunkerPlaced;
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
			ConfigureLevelTravelBlueprint(LevelTravelBlueprint);

		return bConfigured && PlaceLevelTravelActorsInBunkerAndRaidMaps(LevelTravelBlueprint);
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

		return bConfigured && PlaceLevelTravelActorsInBunkerAndRaidMaps(LevelTravelBlueprint);
	}

	bool PlaceFirstOutingQuestNpcInBunkerMap(UBlueprint* QuestNpcBlueprint)
	{
		if (!QuestNpcBlueprint)
		{
			return false;
		}

		UWorld* BunkerWorld = LoadEditorMapForSetup(BunkerMapPackagePath);
		const bool bNpcPlaced =
			BunkerWorld &&
			PlaceQuestNpcActor(
				BunkerWorld,
				QuestNpcBlueprint,
				TEXT("TS_NPC_Instructor"),
				FVector(700.0f, 0.0f, 100.0f)) &&
			UEditorLoadingAndSavingUtils::SaveMap(BunkerWorld, BunkerMapPackagePath);

		LoadEditorMapForSetup(IntroMapPackagePath);
		return bNpcPlaced;
	}

	bool EnsureFirstOutingQuestSetup()
	{
		UWidgetBlueprint* QuestWidgetBlueprint = EnsureWidgetBlueprint(
			UIAssetPath,
			QuestWidgetAssetName,
			UTunaSweeperQuestWidget::StaticClass());
		UBlueprint* QuestNpcBlueprint = EnsureBlueprint(
			NpcAssetPath,
			InstructorQuestNpcAssetName,
			ATunaSweeperQuestNpcActor::StaticClass());

		if (!QuestWidgetBlueprint || !QuestNpcBlueprint)
		{
			return false;
		}

		const bool bConfigured =
			ConfigureQuestWidgetBlueprint(QuestWidgetBlueprint) &&
			ConfigureQuestNpcBlueprint(QuestNpcBlueprint);

		return bConfigured && PlaceFirstOutingQuestNpcInBunkerMap(QuestNpcBlueprint);
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

	void ScheduleEditorMapCaptureSetup()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(EditorMapCaptureTaskId))
		{
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperMapCaptureSetupQuit")))
			{
				FPlatformMisc::RequestExit(false);
			}
			return;
		}

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[](float)
				{
					if (!IsEditorWorldReadyForMapSetup())
					{
						return true;
					}

					FTunaSweeperEditorRunOnce::Run(
						EditorMapCaptureTaskId,
						[]()
						{
							return EnsureEditorMapCaptureBlueprintAndRaidPlacement();
						});

					const bool bCompleted = FTunaSweeperEditorRunOnce::HasCompleted(EditorMapCaptureTaskId);
					if (bCompleted && FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperMapCaptureSetupQuit")))
					{
						FPlatformMisc::RequestExit(false);
					}

					return !bCompleted;
				}),
			1.0f);
	}

	void ScheduleIntroMenuAndLevelTravelSetup()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(IntroMenuAndLevelTravelTaskId))
		{
			return;
		}

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[](float)
				{
					UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
					if (!EditorWorld)
					{
						return true;
					}

					FTunaSweeperEditorRunOnce::Run(
						IntroMenuAndLevelTravelTaskId,
						[]()
						{
							return EnsureIntroMenuAndLevelTravelSetup();
						});

					const bool bCompleted = FTunaSweeperEditorRunOnce::HasCompleted(IntroMenuAndLevelTravelTaskId);
					if (bCompleted &&
						(FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperSetupQuit")) ||
							FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperIntroSetupQuit"))))
					{
						FPlatformMisc::RequestExit(false);
					}

					return !bCompleted;
				}),
			1.0f);
	}

	void ScheduleOpeningScenarioPresentationSetup()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(OpeningScenarioPresentationTaskId))
		{
			return;
		}

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[](float)
				{
					UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
					if (!EditorWorld)
					{
						return true;
					}

					FTunaSweeperEditorRunOnce::Run(
						OpeningScenarioPresentationTaskId,
						[]()
						{
							return EnsureOpeningScenarioPresentationSetup();
						});

					return !FTunaSweeperEditorRunOnce::HasCompleted(OpeningScenarioPresentationTaskId);
				}),
			1.0f);
	}

	void ScheduleBunkerToRaidTransitionVideoSetup()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(LevelTransitionVideoTaskId))
		{
			return;
		}

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[](float)
				{
					UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
					if (!EditorWorld)
					{
						return true;
					}

					FTunaSweeperEditorRunOnce::Run(
						LevelTransitionVideoTaskId,
						[]()
						{
							return EnsureBunkerToRaidTransitionVideoSetup();
						});

					const bool bCompleted = FTunaSweeperEditorRunOnce::HasCompleted(LevelTransitionVideoTaskId);
					if (bCompleted && FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperSetupQuit")))
					{
						FPlatformMisc::RequestExit(false);
					}

					return !bCompleted;
				}),
			1.0f);
	}

	void ScheduleFirstOutingQuestSetup()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(FirstOutingQuestTaskId))
		{
			return;
		}

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[](float)
				{
					UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
					if (!EditorWorld)
					{
						return true;
					}

					FTunaSweeperEditorRunOnce::Run(
						FirstOutingQuestTaskId,
						[]()
						{
							return EnsureFirstOutingQuestSetup();
						});

					const bool bCompleted = FTunaSweeperEditorRunOnce::HasCompleted(FirstOutingQuestTaskId);
					if (bCompleted && FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperSetupQuit")))
					{
						FPlatformMisc::RequestExit(false);
					}

					return !bCompleted;
				}),
			1.0f);
	}

	void ScheduleSelfDestructInteractionSetup()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(SelfDestructInteractionTaskId))
		{
			return;
		}

		FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda(
				[](float)
				{
					UWorld* EditorWorld = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
					if (!EditorWorld)
					{
						return true;
					}

					FTunaSweeperEditorRunOnce::Run(
						SelfDestructInteractionTaskId,
						[]()
						{
							return EnsureSelfDestructInteractionSetup();
						});

					const bool bCompleted = FTunaSweeperEditorRunOnce::HasCompleted(SelfDestructInteractionTaskId);
					if (bCompleted && FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperSetupQuit")))
					{
						FPlatformMisc::RequestExit(false);
					}

					return !bCompleted;
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

		FString UiTextureImportSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureSource="), UiTextureImportSource))
		{
			TunaSweeperEditorSetup::ImportUiTextureFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportUiTextureQuit")))
			{
				return;
			}
		}

		FString AudioImportSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportAudioSource="), AudioImportSource))
		{
			TunaSweeperEditorSetup::ImportAudioFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportAudioQuit")))
			{
				return;
			}
		}

		FString MemoStorageTextureSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportMemoStorageTextureSource="), MemoStorageTextureSource))
		{
			TunaSweeperEditorSetup::ImportMemoStorageDeviceTextureFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportMemoStorageTextureQuit")))
			{
				return;
			}
		}

		FString RollingBomberSpawnerTextureSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportRollingBomberSpawnerTextureSource="), RollingBomberSpawnerTextureSource))
		{
			TunaSweeperEditorSetup::ImportRollingBomberSpawnerTextureFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportRollingBomberSpawnerTextureQuit")))
			{
				return;
			}
		}

		FString SandbagCoverTextureSource;
		if (FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperImportSandbagCoverTextureSource="), SandbagCoverTextureSource))
		{
			TunaSweeperEditorSetup::ImportSandbagCoverTextureFromCommandLineIfRequested();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImportSandbagCoverTextureQuit")))
			{
				return;
			}
		}

		FMSoundTool = MakeUnique<FTunaSweeperFMSoundTool>();
		FMSoundTool->Startup();

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperCommonHudSetupQuit")))
		{
			FTunaSweeperEditorRunOnce::Run(
				TunaSweeperEditorSetup::CommonGameHudTaskId,
				[]()
				{
					return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
				});
			FTunaSweeperEditorRunOnce::Run(
				TunaSweeperEditorSetup::WorkbenchPanelWidgetTaskId,
				[]()
				{
					return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
				});
			FPlatformMisc::RequestExit(false);
			return;
		}

		FString ExperimentalVegetationPaintSource;
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRebuildExperimentalVegetation")) ||
			FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperExperimentalVegetationPaintSource="), ExperimentalVegetationPaintSource))
		{
			if (TunaSweeperExperimentalVegetation::EnsureExperimentalVegetationAssets())
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(TunaSweeperEditorSetup::ExperimentalVegetationAssetTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperExperimentalVegetationQuit")))
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
		}

		FString TurbulentConiferTextureSource;
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRebuildTurbulentConifer")) ||
			FParse::Value(FCommandLine::Get(), TEXT("TunaSweeperTurbulentConiferTextureSource="), TurbulentConiferTextureSource))
		{
			TunaSweeperExperimentalVegetation::EnsureTurbulentConiferPrototypeAssets();

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperTurbulentConiferQuit")))
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRebuildExplosiveBarrel")))
		{
			if (TunaSweeperEditorSetup::EnsureExplosiveBarrelAssets())
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(TunaSweeperEditorSetup::ExplosiveBarrelTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperExplosiveBarrelSetupQuit")))
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
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

		const bool bCanBotBlueprintTaskRan = FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::CanBotBlueprintTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCanBotBlueprint();
			});
		if ((bCanBotBlueprintTaskRan || FTunaSweeperEditorRunOnce::HasCompleted(TunaSweeperEditorSetup::CanBotBlueprintTaskId)) &&
			FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperCanBotSetupQuit")))
		{
			FPlatformMisc::RequestExit(false);
			return;
		}

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::EnemyVisualMaterialTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureEnemyVisualMaterialAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::ExplosiveBarrelTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureExplosiveBarrelAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::RollingBomberBodyMaterialTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureRollingBomberBodyGrayMaterial();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::RollingBomberLegMaterialTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureRollingBomberLegMetalMaterial();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::VoxelMeshAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureSharedVoxelMeshAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::LumberjackMeleeSwingArcAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureLumberjackMeleeSwingArcAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::RollingBomberChargeCylinderEffectTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureRollingBomberChargeCylinderEffectAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::LocalExplosionEffectTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureLocalExplosionEffectAssets();
			});

		const bool bExtractionSmokeSignalNiagaraTaskRan = FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::ExtractionSmokeSignalNiagaraSystemTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureExtractionSmokeSignalNiagaraSystem();
			});
		if ((bExtractionSmokeSignalNiagaraTaskRan ||
				FTunaSweeperEditorRunOnce::HasCompleted(TunaSweeperEditorSetup::ExtractionSmokeSignalNiagaraSystemTaskId)) &&
			FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperExtractionSmokeSignalNiagaraSetupQuit")))
		{
			FPlatformMisc::RequestExit(false);
			return;
		}

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::ProjectileHitEffectAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureProjectileHitEffectAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::WeaponSpreadRecoilAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureWeaponSpreadRecoilAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::BaseballBatAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureBaseballBatAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::SandbagCoverAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureSandbagCoverAssets();
			});

		const bool bLedExpressionMaterialTaskRan = FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::LedExpressionMaterialTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureLedExpressionMaterial() != nullptr;
			});
		if ((bLedExpressionMaterialTaskRan || FTunaSweeperEditorRunOnce::HasCompleted(TunaSweeperEditorSetup::LedExpressionMaterialTaskId)) &&
			FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLedExpressionMaterialSetupQuit")))
		{
			FPlatformMisc::RequestExit(false);
			return;
		}

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::ExperimentalVegetationAssetTaskId,
			[]()
			{
				return TunaSweeperExperimentalVegetation::EnsureExperimentalVegetationAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::CoverPointAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCoverPointAssets();
			});
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperCoverPointSetupQuit")) &&
			FTunaSweeperEditorRunOnce::HasCompleted(TunaSweeperEditorSetup::CoverPointAssetTaskId))
		{
			FPlatformMisc::RequestExit(false);
		}

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
			TunaSweeperEditorSetup::QuickSlotInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureQuickSlotInputAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::DropInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureDropInputAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::AmmoReloadInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureAmmoReloadInputAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::CameraModeInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCameraModeInputAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::SprintInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureSprintInputAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::RollInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureRollInputAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::MapInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureMapInputAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::WorldProgressInteractionTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureWorldProgressInteractionAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::WarpPointInteractionTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureWarpPointInteractionAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::InteractionMarkerAlignmentTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::RebuildInteractionMarkerWidgetAlignment();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::CommonGameHudTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::WorkbenchPanelWidgetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::LootContainerOccupancyHeaderTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureLootContainerOccupancyHeaderAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::CannedTunaIconImportTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCannedTunaIconTexture();
			});

		const bool bBackpackInventoryTaskRan = FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::BackpackInventoryTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureBackpackInventoryAssets();
			});
		if ((bBackpackInventoryTaskRan || FTunaSweeperEditorRunOnce::HasCompleted(TunaSweeperEditorSetup::BackpackInventoryTaskId)) &&
			FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperSetupQuit")))
		{
			FPlatformMisc::RequestExit(false);
		}

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::IntroMenuGraphicsSettingsTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureIntroMenuGraphicsSettingsSetup();
			});

		TunaSweeperEditorSetup::SchedulePickupItemAndSpawnerAssetsAndMapPlacement();
		TunaSweeperEditorSetup::ScheduleLootContainerAndSpawnerAssetsAndMapPlacement();
		TunaSweeperEditorSetup::ScheduleEditorMapCaptureSetup();
		TunaSweeperEditorSetup::ScheduleIntroMenuAndLevelTravelSetup();
		TunaSweeperEditorSetup::ScheduleOpeningScenarioPresentationSetup();
		TunaSweeperEditorSetup::ScheduleBunkerToRaidTransitionVideoSetup();
		TunaSweeperEditorSetup::ScheduleFirstOutingQuestSetup();
		TunaSweeperEditorSetup::ScheduleSelfDestructInteractionSetup();
	}

	virtual void ShutdownModule() override
	{
		if (FMSoundTool)
		{
			FMSoundTool->Shutdown();
			FMSoundTool.Reset();
		}
	}

private:
	TUniquePtr<FTunaSweeperFMSoundTool> FMSoundTool;
};

IMPLEMENT_MODULE(FTunaSweeperEditorModule, TunaSweeperEditor)
