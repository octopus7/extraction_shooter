# Editor One-Shot Cleanup List

이 문서는 에디터 시작 시 한 번 실행하고 끝나는 자동 생성/보정 코드를 추적한다.

정리 규칙:

- 아래 코드는 사용자가 명시적으로 "에디터 one-shot 정리", "RunOnce 정리", "cleanup" 등을 요청할 때만 제거한다.
- RunOnce 키 문자열은 완료 상태 판정에 쓰이므로, 정리 전에는 이름이나 값을 바꾸지 않는다.
- 코드 마커는 `TunaSweeperEditorOneShot_ToCleanupOnExplicitRequest.cpp` 파일명과 `RunEditorOneShotSetup_ToCleanupOnExplicitRequest` 함수명이다.
- 정리 시에는 호출부, 관련 `TaskId` 상수, 더 이상 참조되지 않는 `Ensure*` 생성 함수까지 함께 확인한다.

## 코드 마커

- `TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperEditorOneShot_ToCleanupOnExplicitRequest.cpp`
- `TunaSweeperEditorSetup::RunEditorOneShotSetup_ToCleanupOnExplicitRequest()`
- `TunaSweeperEditorSetup::SchedulePickupItemAndSpawnerAssetsAndMapPlacement()`
- `TunaSweeperEditorSetup::ScheduleLootContainerAndSpawnerAssetsAndMapPlacement()`
- `TunaSweeperEditorSetup::ScheduleEditorMapCaptureSetup()`
- `TunaSweeperEditorSetup::ScheduleLookdevFluidExplosionSetup()`
- `TunaSweeperEditorSetup::ScheduleIntroMenuAndLevelTravelSetup()`
- `TunaSweeperEditorSetup::ScheduleOpeningScenarioPresentationSetup()`
- `TunaSweeperEditorSetup::ScheduleBunkerToRaidTransitionVideoSetup()`
- `TunaSweeperEditorSetup::ScheduleFirstOutingQuestSetup()`
- `TunaSweeperEditorSetup::ScheduleSelfDestructInteractionSetup()`

## RunOnce TaskId 목록

- `GameInstanceTaskId`: `2026-05-10_CreateGameInstanceBlueprint`
- `TopDownShooterTaskId`: `2026-05-10_CreateTopDownShooterAssets`
- `InteractionInputTaskId`: `2026-05-29_SetInteractInputAndFocusWheelV1`
- `InteractionMarkerAlignmentTaskId`: `2026-05-25_RebuildInteractionMarkerRequirementPreviewV1`
- `PickupItemAndSpawnerTaskId`: `2026-05-11_CreatePickupItemAndSpawnerAssetsV3`
- `CommonGameHudTaskId`: `2026-06-02_BottomActionProgressCuteLayoutV1`
- `WorkbenchPanelWidgetTaskId`: `2026-05-29_CreateWorkbenchPanelWidgetV6`
- `ShopRefreshStockButtonTaskId`: `2026-05-29_AddShopRefreshStockButtonV1`
- `SplitExternalContainerPanelTaskId`: `2026-05-30_SplitExternalContainerPanelsV1`
- `CurrencyCoinUiTaskId`: `2026-05-30_AddCurrencyCoinUiV1`
- `HudDebuffBarWidgetTaskId`: `2026-05-30_HudDebuffBarWidgetV2`
- `ItemThumbnailSlotLayoutTaskId`: `2026-05-30_RebuildItemThumbnailSlotLayoutV1`
- `ItemContainerScrollbarStyleTaskId`: `2026-05-30_ItemContainerScrollbarStyleV1`
- `InventoryInputTaskId`: `2026-05-11_AddInventoryInput`
- `QuickSlotInputTaskId`: `2026-05-28_AddMeleeQuickSlotInputV1`
- `DropInputTaskId`: `2026-05-18_AddDropInputAction`
- `AmmoReloadInputTaskId`: `2026-05-19_AddAmmoReloadInputActionsV1`
- `CameraModeInputTaskId`: `2026-05-26_AddCameraModeInputV1`
- `SprintInputTaskId`: `2026-05-28_AddSprintInputV1`
- `RollInputTaskId`: `2026-05-28_AddRollInputV1`
- `MapInputTaskId`: `2026-05-28_AddMapInputV1`
- `EditorMapCaptureTaskId`: `2026-05-28_CreateEditorMapCaptureBlueprintAndRaidPlacementV1`
- `LootContainerAndSpawnerTaskId`: `2026-05-11_CreateLootContainerAndSpawnerAssetsV1`
- `LootContainerOccupancyHeaderTaskId`: `2026-05-30_StorageFilterHeaderAboveGridV1`
- `CannedTunaIconImportTaskId`: `2026-05-11_ImportCannedTunaIconV1`
- `BackpackInventoryTaskId`: `2026-05-16_CreateEquipmentInventoryAssetsV3`
- `IntroMenuAndLevelTravelTaskId`: `2026-05-24_CreateTitleIntroMenuPersistentSaveSlotSelectionLevelTravelLadderInitialScaleV1`
- `IntroMenuGraphicsSettingsTaskId`: `2026-07-08_CompactTitleSettingsUiV1`
- `OpeningScenarioPresentationTaskId`: `2026-05-19_CreateOpeningScenarioPresentationV2`
- `LevelTransitionVideoTaskId`: `2026-05-16_AddBidirectionalLevelTransitionVideoV3`
- `FirstOutingQuestTaskId`: `2026-05-30_UpdateQuestPanelEmptyStateSelectionV2`
- `SelfDestructInteractionTaskId`: `2026-05-16_CreateSelfDestructInteractionV1`
- `WorldProgressInteractionTaskId`: `2026-05-19_CreateWorldProgressObstacleAssetsV1`
- `WarpPointInteractionTaskId`: `2026-05-25_CreateWarpPointInteractionAssetsV1`
- `EnemyVisualMaterialTaskId`: `2026-05-19_CreateEnemyAndContainerVisualMaterialsV3`
- `ExplosiveBarrelTaskId`: `2026-05-29_CreateExplosiveBarrelAssetsV8`
- `BreakableAppleCrateTaskId`: `2026-07-07_CreateBreakableAppleCrateAssetsV5`
- `RollingBomberBodyMaterialTaskId`: `2026-05-28_CreateRollingBomberBodyGrayMaterialV1`
- `RollingBomberLegMaterialTaskId`: `2026-05-28_CreateRollingBomberLegMetalMaterialV1`
- `RollingBomberChargeCylinderEffectTaskId`: `2026-05-28_CreateRollingBomberChargeCylinderEffectV1`
- `LocalExplosionEffectTaskId`: `2026-05-29_CreateLocalExplosionFlipbookEffectV3`
- `ExtractionSmokeSignalNiagaraSystemTaskId`: `2026-05-29_CreateExtractionSmokeSignalNiagaraSystemV4`
- `LookdevFluidExplosionTaskId`: `2026-06-14_CreateLookdevFluidExplosionNiagaraLevelV1`
- `ProjectileHitEffectAssetTaskId`: `2026-05-28_CreateProjectileHitEffectAssetsV1`
- `WeaponSpreadRecoilAssetTaskId`: `2026-05-28_CreateWeaponSpreadRecoilAssetsV1`
- `BaseballBatAssetTaskId`: `2026-05-28_CreateBaseballBatStaticMeshAssetsV1`
- `SandbagCoverAssetTaskId`: `2026-06-02_SandbagFourLayerCoverV1`
- `VoxelMeshAssetTaskId`: `2026-05-19_CreateSharedVoxelMeshAssetsV1`
- `LumberjackMeleeSwingArcAssetTaskId`: `2026-05-20_CreateLumberjackMeleeSwingArcAssetsV2`
- `LedExpressionMaterialTaskId`: `2026-05-26_CreateLedExpressionMaterialV1`
- `ExperimentalVegetationAssetTaskId`: `2026-05-24_CreateExperimentalVegetationStaticMeshV4`
- `TurbulentConiferOcclusionRevealTaskId`: `2026-06-08_UpdateTurbulentConiferOcclusionRevealV1`
- `CoverPointAssetTaskId`: `2026-05-16_CreateCoverPointBlueprintV1`
- `CanBotBlueprintTaskId`: `2026-05-25_CreateCanBotBlueprintV1`

## Inline literal RunOnce keys

- `2026-07-06_PuddleSkyReflectionMaterialNoRippleV2`

