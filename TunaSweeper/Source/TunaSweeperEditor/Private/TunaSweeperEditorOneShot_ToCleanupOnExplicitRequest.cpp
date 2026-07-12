#include "TunaSweeperEditorSetupShared.h"

namespace TunaSweeperEditorSetup
{

	// One-shot editor bootstrap. Remove only after an explicit cleanup request; see Docs/editor_one_shot_cleanup.md.
	void RunEditorOneShotSetup_ToCleanupOnExplicitRequest()
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperWeaponPresentationSetup")))
		{
			if (TunaSweeperEditorSetup::EnsureWeaponPresentationAssets())
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(
					TunaSweeperEditorSetup::WeaponPresentationAssetTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperWeaponPresentationSetupQuit")))
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
		}

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
			FTunaSweeperEditorRunOnce::Run(
				TunaSweeperEditorSetup::ShopRefreshStockButtonTaskId,
				[]()
				{
					return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
				});
			FTunaSweeperEditorRunOnce::Run(
				TunaSweeperEditorSetup::SplitExternalContainerPanelTaskId,
				[]()
				{
					return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
				});
			FTunaSweeperEditorRunOnce::Run(
				TunaSweeperEditorSetup::CurrencyCoinUiTaskId,
				[]()
				{
					return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
				});
			FTunaSweeperEditorRunOnce::Run(
				TunaSweeperEditorSetup::HudDebuffBarWidgetTaskId,
				[]()
				{
					return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
				});
			FTunaSweeperEditorRunOnce::Run(
				TunaSweeperEditorSetup::ItemContainerScrollbarStyleTaskId,
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

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRebuildBreakableAppleCrate")))
		{
			if (TunaSweeperEditorSetup::EnsureBreakableAppleCrateAssets())
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(TunaSweeperEditorSetup::BreakableAppleCrateTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperBreakableAppleCrateSetupQuit")))
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRebuildProceduralTerrainTest")))
		{
			FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateLambda(
					[](float)
					{
						const bool bCompleted = TunaSweeperProceduralTerrainTest::EnsureProceduralTerrainTestLevel();
						if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperProceduralTerrainTestQuit")))
						{
							FPlatformMisc::RequestExit(false);
						}
						return !bCompleted;
					}),
				1.0f);
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperCheckProceduralTerrainWater")))
		{
			TSharedRef<int32> WaterCheckTickCount = MakeShared<int32>(0);
			FTSTicker::GetCoreTicker().AddTicker(
				FTickerDelegate::CreateLambda(
					[WaterCheckTickCount](float)
					{
						if (*WaterCheckTickCount == 0)
						{
							TunaSweeperProceduralTerrainTest::LoadProceduralTerrainTestLevelForWaterCheck();
							++(*WaterCheckTickCount);
							return true;
						}

						if (*WaterCheckTickCount < 8)
						{
							++(*WaterCheckTickCount);
							return true;
						}

						TunaSweeperProceduralTerrainTest::RunProceduralTerrainWaterVisualCheck();
						if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperProceduralTerrainWaterCheckQuit")))
						{
							FPlatformMisc::RequestExit(false);
						}
						return false;
					}),
				1.0f);
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRebuildPuddleSkyReflectionMaterial")))
		{
			TunaSweeperPuddleSkyReflectionMaterial::EnsureAssets();

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperPuddleSkyReflectionMaterialQuit")))
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
		}

		FTunaSweeperEditorRunOnce::Run(
			TEXT("2026-07-06_PuddleSkyReflectionMaterialNoRippleV2"),
			[]()
			{
				return TunaSweeperPuddleSkyReflectionMaterial::EnsureAssets();
			});

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
			TunaSweeperEditorSetup::BreakableAppleCrateTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureBreakableAppleCrateAssets();
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
			TunaSweeperEditorSetup::EnemyCombatDebugAssetsTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureEnemyCombatDebugInputAssets() &&
					TunaSweeperEditorSetup::EnsureEnemySensorDebugMaterial() != nullptr;
			});

		const bool bWeaponPresentationTaskRan = FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::WeaponPresentationAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureWeaponPresentationAssets();
			});
		if ((bWeaponPresentationTaskRan ||
				FTunaSweeperEditorRunOnce::HasCompleted(TunaSweeperEditorSetup::WeaponPresentationAssetTaskId)) &&
			FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperWeaponPresentationSetupQuit")))
		{
			FPlatformMisc::RequestExit(false);
			return;
		}

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
			TunaSweeperEditorSetup::TurbulentConiferOcclusionRevealTaskId,
			[]()
			{
				return TunaSweeperExperimentalVegetation::EnsureTurbulentConiferOcclusionRevealAssets();
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
			TunaSweeperEditorSetup::ShopRefreshStockButtonTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::SplitExternalContainerPanelTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::CurrencyCoinUiTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::HudDebuffBarWidgetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::ItemThumbnailSlotLayoutTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::ItemContainerScrollbarStyleTaskId,
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

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::IntroMenuDevelopmentSettingsTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureIntroMenuGraphicsSettingsSetup();
			});

		TunaSweeperEditorSetup::SchedulePickupItemAndSpawnerAssetsAndMapPlacement();
		TunaSweeperEditorSetup::ScheduleLootContainerAndSpawnerAssetsAndMapPlacement();
		TunaSweeperEditorSetup::ScheduleEditorMapCaptureSetup();
		TunaSweeperEditorSetup::ScheduleLookdevFluidExplosionSetup();
		TunaSweeperEditorSetup::ScheduleIntroMenuAndLevelTravelSetup();
		TunaSweeperEditorSetup::ScheduleOpeningScenarioPresentationSetup();
		TunaSweeperEditorSetup::ScheduleBunkerToRaidTransitionVideoSetup();
		TunaSweeperEditorSetup::ScheduleFirstOutingQuestSetup();
		TunaSweeperEditorSetup::ScheduleSelfDestructInteractionSetup();
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

	void ScheduleLookdevFluidExplosionSetup()
	{
		if (FTunaSweeperEditorRunOnce::HasCompleted(LookdevFluidExplosionTaskId))
		{
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLookdevFluidExplosionSetupQuit")))
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
						LookdevFluidExplosionTaskId,
						[]()
						{
							return EnsureLookdevFluidExplosionAssetsAndLevel();
						});

					const bool bCompleted = FTunaSweeperEditorRunOnce::HasCompleted(LookdevFluidExplosionTaskId);
					if (bCompleted && FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLookdevFluidExplosionSetupQuit")))
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
