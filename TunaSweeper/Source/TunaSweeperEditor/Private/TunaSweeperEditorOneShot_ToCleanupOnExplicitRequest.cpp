#include "TunaSweeperEditorSetupShared.h"

namespace TunaSweeperEditorSetup
{

	// One-shot editor bootstrap. Remove only after an explicit cleanup request; see Docs/editor_one_shot_cleanup.md.
	void RunEditorOneShotSetup_ToCleanupOnExplicitRequest()
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperCrowbarWallRackSetupQuit")))
		{
			const bool bSucceeded = TunaSweeperEditorSetup::EnsureCrowbarWallRackBlueprint();
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([bSucceeded](float)
			{
				FPlatformMisc::RequestExitWithStatus(false, bSucceeded ? 0 : 1, TEXT("TunaSweeperCrowbarWallRackSetup"));
				return false;
			}));
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRobotDeathDissolveSetup")))
		{
			const bool bSucceeded = TunaSweeperEditorSetup::EnsureRobotDeathDissolveMaterial();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRobotDeathDissolveSetupQuit")))
			{
				FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([bSucceeded](float)
				{
					FPlatformMisc::RequestExitWithStatus(false, bSucceeded ? 0 : 1, TEXT("TunaSweeperRobotDeathDissolveSetup"));
					return false;
				}));
			}
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperResearchSinkSetupQuit")))
		{
			const bool bSucceeded = TunaSweeperEditorSetup::EnsureResearchSinkInteractionBlueprint();
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([bSucceeded](float)
			{
				FPlatformMisc::RequestExitWithStatus(false, bSucceeded ? 0 : 1, TEXT("TunaSweeperResearchSinkSetup"));
				return false;
			}));
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperResearchSetupQuit")))
		{
			const bool bSucceeded = TunaSweeperEditorSetup::EnsureCommonGameHudAssets();
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([bSucceeded](float)
			{
				FPlatformMisc::RequestExitWithStatus(false, bSucceeded ? 0 : 1, TEXT("TunaSweeperResearchSetup"));
				return false;
			}));
			return;
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperScratchSetup")))
		{
			const bool bSucceeded = TunaSweeperEditorSetup::EnsureScratchPresentationAssets();
			if (bSucceeded)
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(TunaSweeperEditorSetup::ScratchPresentationTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperScratchSetupQuit")))
			{
				FPlatformMisc::RequestExitWithStatus(false, bSucceeded ? 0 : 1, TEXT("TunaSweeperScratchSetup"));
				return;
			}
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImpactPhysicalMaterialSetup")))
		{
			if (TunaSweeperEditorSetup::EnsureImpactPhysicalMaterialAssets())
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(
					TunaSweeperEditorSetup::ImpactPhysicalMaterialAssetTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperImpactPhysicalMaterialSetupQuit")))
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperFootstepPresentationSetup")))
		{
			if (TunaSweeperEditorSetup::EnsurePlayerFootstepPresentationAssets())
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(
					TunaSweeperEditorSetup::PlayerFootstepPresentationAssetTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperFootstepPresentationSetupQuit")))
			{
				FTSTicker::GetCoreTicker().AddTicker(
					FTickerDelegate::CreateLambda([](float)
					{
						FPlatformMisc::RequestExit(false);
						return false;
					}));
				return;
			}
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperWeaponEmptyFireSetup")))
		{
			if (TunaSweeperEditorSetup::EnsureWeaponPresentationEmptyFireSoundAsset())
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(
					TunaSweeperEditorSetup::WeaponPresentationEmptyFireAssetTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperWeaponEmptyFireSetupQuit")))
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperOcclusionRevealSetup")))
		{
			const bool bSucceeded = TunaSweeperEditorSetup::EnsureOcclusionRevealAssets();
			if (bSucceeded)
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(TunaSweeperEditorSetup::OcclusionRevealAssetTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperOcclusionRevealSetupQuit")))
			{
				FPlatformMisc::RequestExitWithStatus(false, bSucceeded ? 0 : 1, TEXT("TunaSweeperOcclusionRevealSetup"));
				return;
			}
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperShellCasingSetup")))
		{
			TunaSweeperEditorSetup::EnsureShellCasingAssets();
			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperShellCasingSetupQuit")))
			{
				FPlatformMisc::RequestExit(false);
				return;
			}
		}

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

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperCreateCookableChickenBlueprint")))
		{
			if (TunaSweeperEditorSetup::EnsureCookableChickenBlueprint())
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(TunaSweeperEditorSetup::CookableChickenTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperCookableChickenBlueprintQuit")))
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
				FTSTicker::GetCoreTicker().AddTicker(
					FTickerDelegate::CreateLambda([](float)
					{
						FPlatformMisc::RequestExit(false);
						return false;
					}));
				return;
			}
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperRebuildBreakableTomato")))
		{
			if (TunaSweeperEditorSetup::EnsureBreakableTomatoAssets())
			{
				FTunaSweeperEditorRunOnce::MarkCompleted(TunaSweeperEditorSetup::BreakableTomatoTaskId);
			}

			if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperBreakableTomatoSetupQuit")))
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

		const bool bMoleBlueprintTaskRan = FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::MoleBlueprintTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureMoleBlueprint();
			});
		if ((bMoleBlueprintTaskRan || FTunaSweeperEditorRunOnce::HasCompleted(TunaSweeperEditorSetup::MoleBlueprintTaskId)) &&
			FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperMoleSetupQuit")))
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

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::BreakableTomatoTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureBreakableTomatoAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::EnemyDeathStrawberryEffectTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureEnemyDeathStrawberryEffectAssets();
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
			TunaSweeperEditorSetup::ImpactPhysicalMaterialAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureImpactPhysicalMaterialAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::WeaponSpreadRecoilAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureWeaponSpreadRecoilAssets();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::PlayerFootstepPresentationAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsurePlayerFootstepPresentationAssets();
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
			TunaSweeperEditorSetup::WeaponPresentationEmptyFireAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureWeaponPresentationEmptyFireSoundAsset();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::EnemyWeaponFallbackPresentationAssetTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureEnemyWeaponFallbackPresentationAsset();
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
			TunaSweeperEditorSetup::JumpInputTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureJumpInputAssets();
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
			TunaSweeperEditorSetup::ScratchPresentationTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureScratchPresentationAssets();
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
			TunaSweeperEditorSetup::IntroMenuVisualRestyleTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureIntroMenuVisualRestyleSetup();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::IntroMenuDevelopmentSettingsTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureIntroMenuGraphicsSettingsSetup();
			});

		FTunaSweeperEditorRunOnce::Run(
			TunaSweeperEditorSetup::IntroMenuDebugDisplayLanguageTaskId,
			[]()
			{
				return TunaSweeperEditorSetup::EnsureIntroMenuGraphicsSettingsSetup();
			});

		FTunaSweeperEditorRunOnce::Run(
			TEXT("2026-09-06_SplitTitleScreensAndFullscreenSettingsV7"),
			[]() { return TunaSweeperEditorSetup::EnsureTitleScreenAssetsSetup(); });

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
					if (bCompleted && FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperLevelTravelPresentationSetupQuit")))
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
