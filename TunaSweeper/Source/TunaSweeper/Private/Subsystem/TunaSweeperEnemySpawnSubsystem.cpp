#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "AI/TunaSweeperRollingBomber.h"
#include "AI/TunaSweeperRollingBomberSpawner.h"
#include "Components/StaticMeshComponent.h"
#include "Game/TunaSweeperDataValueTypes.h"
#include "Debuff/TunaSweeperDebuffTypes.h"
#include "Dom/JsonObject.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interaction/TunaSweeperDifficultyAdjustmentActor.h"
#include "Interaction/TunaSweeperExplosiveBarrelActor.h"
#include "Interaction/TunaSweeperExtractionPointActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Interaction/TunaSweeperItemSpawnInteractableActor.h"
#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"
#include "Interaction/TunaSweeperLootContainerActor.h"
#include "Interaction/TunaSweeperLootContainerSpawnInteractableActor.h"
#include "Interaction/TunaSweeperPeriodicNoiseEmitterActor.h"
#include "Interaction/TunaSweeperPiggyBankActor.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Interaction/TunaSweeperPickupItemActor.h"
#include "Interaction/TunaSweeperSandbagCoverActor.h"
#include "Interaction/TunaSweeperSelfDestructInteractableActor.h"
#include "Interaction/TunaSweeperShopActor.h"
#include "Interaction/TunaSweeperShootingPracticeDummyActor.h"
#include "Interaction/TunaSweeperTransparentObstacleActor.h"
#include "Interaction/TunaSweeperWarpPointActor.h"
#include "Interaction/TunaSweeperWorkbenchActor.h"
#include "Interaction/TunaSweeperWorldProgressActor.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "NiagaraSystem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"
#include "UObject/UObjectGlobals.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperEnemySpawn, Log, All);

namespace TunaSweeperEnemySpawn
{
	const TCHAR* EnemySpawnsJsonRelativePath = TEXT("Data/EnemySpawns.json");
	const TCHAR* EnemyCombatProfilesJsonRelativePath = TEXT("Data/EnemyCombatProfiles.json");
	const FName DefaultEnemyCombatProfileId(TEXT("enemy.rifle_anchor"));
	const TCHAR* LootContainerSpawnsJsonRelativePath = TEXT("Data/LootContainerSpawns.json");
	const TCHAR* TransparentObstacleSpawnsJsonRelativePath = TEXT("Data/TransparentObstacleSpawns.json");
	const TCHAR* WorldProgressObjectSpawnsJsonRelativePath = TEXT("Data/WorldProgressObjectSpawns.json");
	const TCHAR* WarpPointSpawnsJsonRelativePath = TEXT("Data/WarpPointSpawns.json");
	const TCHAR* GameplayInteractionActorSpawnsJsonRelativePath = TEXT("Data/GameplayInteractionSpawns.json");
	const TCHAR* DefaultEnemyClassPath = TEXT("/Game/Characters/Enemy/BP_TunaSweeperEnemy.BP_TunaSweeperEnemy_C");
	const TCHAR* DefaultLevelTravelClassPath = TEXT("/Game/Interaction/BP_Interact_LevelTravel.BP_Interact_LevelTravel_C");
	const TCHAR* DefaultPickupItemClassPath = TEXT("/Game/Interaction/BP_PickupItem.BP_PickupItem_C");
	const TCHAR* DefaultItemSpawnClassPath = TEXT("/Game/Interaction/BP_Interact_ItemSpawn.BP_Interact_ItemSpawn_C");
	const TCHAR* DefaultLootContainerClassPath = TEXT("/Game/Interaction/BP_LootContainer.BP_LootContainer_C");
	const TCHAR* DefaultLootContainerSpawnClassPath = TEXT("/Game/Interaction/BP_Interact_LootContainerSpawn.BP_Interact_LootContainerSpawn_C");
	const TCHAR* DefaultSelfDestructClassPath = TEXT("/Game/Interaction/BP_Interact_SelfDestruct.BP_Interact_SelfDestruct_C");
	const TCHAR* DefaultRollingBomberSpawnerClassPath = TEXT("/Script/TunaSweeper.TunaSweeperRollingBomberSpawner");
	const TCHAR* DefaultExtractionPointClassPath = TEXT("/Script/TunaSweeper.TunaSweeperExtractionPointActor");
	const TCHAR* DefaultSandbagCoverClassPath = TEXT("/Game/Interaction/BP_SandbagCover.BP_SandbagCover_C");
	const TCHAR* DefaultExplosiveBarrelClassPath = TEXT("/Game/Interaction/BP_ExplosiveBarrel.BP_ExplosiveBarrel_C");
	const TCHAR* DefaultStaticMeshPropClassPath = TEXT("/Script/Engine.StaticMeshActor");
	const TCHAR* DefaultShootingPracticeDummyClassPath = TEXT("/Script/TunaSweeper.TunaSweeperShootingPracticeDummyActor");
	const TCHAR* DefaultShopClassPath = TEXT("/Script/TunaSweeper.TunaSweeperShopActor");
	const TCHAR* DefaultWorkbenchClassPath = TEXT("/Script/TunaSweeper.TunaSweeperWorkbenchActor");
	const TCHAR* DefaultPiggyBankClassPath = TEXT("/Script/TunaSweeper.TunaSweeperPiggyBankActor");
	const TCHAR* DefaultPeriodicNoiseEmitterClassPath = TEXT("/Script/TunaSweeper.TunaSweeperPeriodicNoiseEmitterActor");
	const TCHAR* DefaultDifficultyAdjustmentClassPath = TEXT("/Script/TunaSweeper.TunaSweeperDifficultyAdjustmentActor");
	const TCHAR* DefaultRollingBomberClassPath = TEXT("/Script/TunaSweeper.TunaSweeperRollingBomber");
	const TCHAR* DefaultRollingBomberLaunchSoundPath =
		TEXT("/Game/Audio/SFX/SFX_RollingBomberSpawnerLaunch_FM.SFX_RollingBomberSpawnerLaunch_FM");
	const TCHAR* DefaultTransparentObstacleClassPath = TEXT("/Game/Interaction/BP_TransparentObstacle.BP_TransparentObstacle_C");
	const TCHAR* DefaultWorldProgressActorClassPath = TEXT("/Game/Interaction/BP_WorldProgress_BrokenBridge.BP_WorldProgress_BrokenBridge_C");
	const TCHAR* DefaultWorldProgressCompletedActorClassPath = TEXT("/Game/Interaction/BP_WorldProgress_RepairedBridge.BP_WorldProgress_RepairedBridge_C");
	const TCHAR* DefaultWarpPointClassPath = TEXT("/Game/Interaction/BP_WarpPoint.BP_WarpPoint_C");
	const TCHAR* DefaultWarpPointMaterialPath = TEXT("/Game/Interaction/M_WarpPointEnergy.M_WarpPointEnergy");
	const TCHAR* DefaultWarpPointSphereMeshPath = TEXT("/Engine/BasicShapes/Sphere.Sphere");
	const TCHAR* DefaultInteractionMarkerWidgetClassPath = TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C");
	const TCHAR* DefaultLevelTransitionWidgetClassPath = TEXT("/Game/UI/WBP_LevelTransitionVideo.WBP_LevelTransitionVideo_C");
	const TCHAR* DefaultPickupItemIconWidgetClassPath = TEXT("/Game/UI/WBP_PickupItemIcon.WBP_PickupItemIcon_C");
	const TCHAR* DefaultSpeechBubbleWidgetClassPath = TEXT("/Game/UI/WBP_SpeechBubble.WBP_SpeechBubble_C");
	const TCHAR* DefaultExplosionEffectActorClassPath = TEXT("/Script/TunaSweeper.TunaSweeperLocalExplosionEffectActor");

	float ReadNonNegativeFloatField(
		const TSharedPtr<FJsonObject>& JsonObject,
		const TCHAR* FieldName,
		float DefaultValue)
	{
		double NumericValue = DefaultValue;
		JsonObject->TryGetNumberField(FieldName, NumericValue);
		return FMath::Max(0.0f, static_cast<float>(NumericValue));
	}

	int32 ReadNonNegativeIntField(
		const TSharedPtr<FJsonObject>& JsonObject,
		const TCHAR* FieldName,
		int32 DefaultValue)
	{
		double NumericValue = DefaultValue;
		JsonObject->TryGetNumberField(FieldName, NumericValue);
		return FMath::Max(0, FMath::RoundToInt(NumericValue));
	}

	bool TryResolveEnemyAttackMode(const FString& AttackModeString, ETunaSweeperEnemyAttackMode& OutAttackMode)
	{
		const FString NormalizedAttackMode = AttackModeString.TrimStartAndEnd().ToLower();
		if (NormalizedAttackMode == TEXT("ranged"))
		{
			OutAttackMode = ETunaSweeperEnemyAttackMode::Ranged;
			return true;
		}
		if (NormalizedAttackMode == TEXT("melee"))
		{
			OutAttackMode = ETunaSweeperEnemyAttackMode::Melee;
			return true;
		}

		return false;
	}

	bool TryResolveEnemyCombatRole(const FString& CombatRoleString, ETunaSweeperEnemyCombatRole& OutCombatRole)
	{
		const FString NormalizedCombatRole = CombatRoleString.TrimStartAndEnd().ToLower();
		if (NormalizedCombatRole == TEXT("anchor"))
		{
			OutCombatRole = ETunaSweeperEnemyCombatRole::Anchor;
			return true;
		}
		if (NormalizedCombatRole == TEXT("flanker"))
		{
			OutCombatRole = ETunaSweeperEnemyCombatRole::Flanker;
			return true;
		}
		if (NormalizedCombatRole == TEXT("melee"))
		{
			OutCombatRole = ETunaSweeperEnemyCombatRole::Melee;
			return true;
		}

		return false;
	}

	FString NormalizeLevelName(const FString& RawLevelName)
	{
		FString LevelName = FPackageName::GetShortName(RawLevelName);
		if (LevelName.StartsWith(TEXT("UEDPIE_")))
		{
			const int32 SearchStart = FString(TEXT("UEDPIE_")).Len();
			const int32 SecondUnderscoreIndex = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, SearchStart);
			if (SecondUnderscoreIndex != INDEX_NONE)
			{
				LevelName = LevelName.Mid(SecondUnderscoreIndex + 1);
			}
		}

		return LevelName;
	}

	bool TryReadVectorField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, FVector& OutVector)
	{
		const TArray<TSharedPtr<FJsonValue>>* VectorArray = nullptr;
		if (!JsonObject.IsValid() || !JsonObject->TryGetArrayField(FieldName, VectorArray) || !VectorArray || VectorArray->Num() < 3)
		{
			return false;
		}

		OutVector = FVector(
			static_cast<float>((*VectorArray)[0]->AsNumber()),
			static_cast<float>((*VectorArray)[1]->AsNumber()),
			static_cast<float>((*VectorArray)[2]->AsNumber()));
		return true;
	}

	bool TryReadRotatorField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, FRotator& OutRotator)
	{
		FVector RotationVector = FVector::ZeroVector;
		if (!TryReadVectorField(JsonObject, FieldName, RotationVector))
		{
			return false;
		}

		OutRotator = FRotator(RotationVector.X, RotationVector.Y, RotationVector.Z);
		return true;
	}

	bool TryReadVector2DField(const TSharedPtr<FJsonObject>& JsonObject, const TCHAR* FieldName, FVector2D& OutVector)
	{
		const TArray<TSharedPtr<FJsonValue>>* VectorArray = nullptr;
		if (!JsonObject.IsValid() || !JsonObject->TryGetArrayField(FieldName, VectorArray) || !VectorArray || VectorArray->Num() < 2)
		{
			return false;
		}

		OutVector = FVector2D(
			static_cast<float>((*VectorArray)[0]->AsNumber()),
			static_cast<float>((*VectorArray)[1]->AsNumber()));
		return true;
	}

	FString ReadFirstStringField(
		const TSharedPtr<FJsonObject>& JsonObject,
		const TCHAR* FieldNameA,
		const TCHAR* FieldNameB = nullptr,
		const TCHAR* FieldNameC = nullptr)
	{
		if (!JsonObject.IsValid())
		{
			return FString();
		}

		const TCHAR* FieldNames[] = { FieldNameA, FieldNameB, FieldNameC };
		for (const TCHAR* FieldName : FieldNames)
		{
			if (!FieldName)
			{
				continue;
			}

			FString Value;
			if (JsonObject->TryGetStringField(FieldName, Value))
			{
				return Value.TrimStartAndEnd();
			}
		}

		return FString();
	}

	bool TryReadMapOverlayDefinition(
		const TSharedPtr<FJsonObject>& JsonObject,
		FName LevelName,
		FName SpawnId,
		const FVector& SpawnLocation,
		FTunaSweeperMapOverlayDefinition& OutMapOverlay)
	{
		const TSharedPtr<FJsonObject>* MapOverlayObjectPtr = nullptr;
		if (!JsonObject.IsValid() ||
			(!JsonObject->TryGetObjectField(TEXT("mapOverlay"), MapOverlayObjectPtr) &&
				!JsonObject->TryGetObjectField(TEXT("map_overlay"), MapOverlayObjectPtr)) ||
			!MapOverlayObjectPtr ||
			!MapOverlayObjectPtr->IsValid())
		{
			return false;
		}

		const TSharedPtr<FJsonObject>& MapOverlayObject = *MapOverlayObjectPtr;
		bool bEnabled = true;
		if (MapOverlayObject->TryGetBoolField(TEXT("enabled"), bEnabled) && !bEnabled)
		{
			return false;
		}

		const FString TextStringKey = ReadFirstStringField(
			MapOverlayObject,
			TEXT("text_string_key"),
			TEXT("text_key"),
			TEXT("textKey"));
		const FString IconId = ReadFirstStringField(
			MapOverlayObject,
			TEXT("icon"),
			TEXT("icon_id"),
			TEXT("iconId"));
		if (TextStringKey.IsEmpty() && IconId.IsEmpty())
		{
			return false;
		}

		FVector WorldOffset = FVector::ZeroVector;
		if (!TryReadVectorField(MapOverlayObject, TEXT("world_offset"), WorldOffset))
		{
			TryReadVectorField(MapOverlayObject, TEXT("location_offset"), WorldOffset);
		}

		FVector2D TextOffset = IconId.IsEmpty() ? FVector2D::ZeroVector : FVector2D(0.0f, -34.0f);
		if (!TryReadVector2DField(MapOverlayObject, TEXT("text_offset"), TextOffset) &&
			!TryReadVector2DField(MapOverlayObject, TEXT("text_screen_offset"), TextOffset))
		{
			TryReadVector2DField(MapOverlayObject, TEXT("textOffset"), TextOffset);
		}

		FVector2D IconOffset = FVector2D::ZeroVector;
		if (!TryReadVector2DField(MapOverlayObject, TEXT("icon_offset"), IconOffset) &&
			!TryReadVector2DField(MapOverlayObject, TEXT("icon_screen_offset"), IconOffset))
		{
			TryReadVector2DField(MapOverlayObject, TEXT("iconOffset"), IconOffset);
		}

		OutMapOverlay.LevelName = LevelName;
		OutMapOverlay.SpawnId = SpawnId;
		OutMapOverlay.WorldLocation = SpawnLocation + WorldOffset;
		OutMapOverlay.TextStringKey = TextStringKey.IsEmpty() ? NAME_None : FName(*TextStringKey);
		OutMapOverlay.IconId = IconId.IsEmpty() ? NAME_None : FName(*IconId);
		OutMapOverlay.TextOffset = TextOffset;
		OutMapOverlay.IconOffset = IconOffset;
		return true;
	}

	bool ShouldIncludeEditorOnlySpawn(const UWorld* World)
	{
#if WITH_EDITOR
		return World && World->IsGameWorld();
#else
		(void)World;
		return false;
#endif
	}

	UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType ReadGameplayInteractionActorSpawnType(
		const FString& RawSpawnType)
	{
		FString SpawnType = RawSpawnType.TrimStartAndEnd().ToLower();
		SpawnType.ReplaceInline(TEXT("-"), TEXT("_"));

		if (SpawnType == TEXT("level_travel") || SpawnType == TEXT("leveltravel"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::LevelTravel;
		}
		if (SpawnType == TEXT("pickup_item") || SpawnType == TEXT("pickupitem") || SpawnType == TEXT("item_pickup"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::PickupItem;
		}
		if (SpawnType == TEXT("item_spawn") || SpawnType == TEXT("itemspawn"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ItemSpawn;
		}
		if (SpawnType == TEXT("loot_container") || SpawnType == TEXT("lootcontainer"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::LootContainer;
		}
		if (SpawnType == TEXT("loot_container_spawn") || SpawnType == TEXT("lootcontainerspawn"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::LootContainerSpawn;
		}
		if (SpawnType == TEXT("shop") ||
			SpawnType == TEXT("shop_open") ||
			SpawnType == TEXT("vending_machine") ||
			SpawnType == TEXT("vendingmachine"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::Shop;
		}
		if (SpawnType == TEXT("workbench") ||
			SpawnType == TEXT("workbench_open") ||
			SpawnType == TEXT("crafting_table") ||
			SpawnType == TEXT("craftingtable"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::Workbench;
		}
		if (SpawnType == TEXT("piggy_bank") ||
			SpawnType == TEXT("piggybank") ||
			SpawnType == TEXT("debug_piggy_bank") ||
			SpawnType == TEXT("currency_piggy_bank"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::PiggyBank;
		}
		if (SpawnType == TEXT("periodic_noise_emitter") ||
			SpawnType == TEXT("noise_emitter") ||
			SpawnType == TEXT("test_noise_emitter") ||
			SpawnType == TEXT("speaker_noise_emitter"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::PeriodicNoiseEmitter;
		}
		if (SpawnType == TEXT("difficulty_adjustment") ||
			SpawnType == TEXT("difficulty_adjuster") ||
			SpawnType == TEXT("difficulty_terminal"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::DifficultyAdjustment;
		}
		if (SpawnType == TEXT("self_destruct") || SpawnType == TEXT("selfdestruct"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::SelfDestruct;
		}
		if (SpawnType == TEXT("rolling_bomber_spawner") ||
			SpawnType == TEXT("rollingbomberspawner") ||
			SpawnType == TEXT("rolling_bomber_spawn"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::RollingBomberSpawner;
		}
		if (SpawnType == TEXT("extraction_point") ||
			SpawnType == TEXT("extractionpoint") ||
			SpawnType == TEXT("extraction"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ExtractionPoint;
		}
		if (SpawnType == TEXT("sandbag_cover") ||
			SpawnType == TEXT("sandbagcover") ||
			SpawnType == TEXT("cover_sandbag"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::SandbagCover;
		}
		if (SpawnType == TEXT("explosive_barrel") ||
			SpawnType == TEXT("explosivebarrel") ||
			SpawnType == TEXT("barrel_explosive") ||
			SpawnType == TEXT("drum_barrel"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ExplosiveBarrel;
		}
		if (SpawnType == TEXT("static_mesh_prop") ||
			SpawnType == TEXT("staticmeshprop") ||
			SpawnType == TEXT("static_mesh") ||
			SpawnType == TEXT("mesh_prop") ||
			SpawnType == TEXT("prop_static_mesh"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::StaticMeshProp;
		}
		if (SpawnType == TEXT("shooting_practice_dummy") ||
			SpawnType == TEXT("practice_dummy") ||
			SpawnType == TEXT("target_dummy") ||
			SpawnType == TEXT("shooting_dummy"))
		{
			return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ShootingPracticeDummy;
		}

		return UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::Unknown;
	}

	const TCHAR* GetDefaultGameplayInteractionActorClassPath(
		UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType SpawnType)
	{
		switch (SpawnType)
		{
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::LevelTravel:
			return DefaultLevelTravelClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::PickupItem:
			return DefaultPickupItemClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ItemSpawn:
			return DefaultItemSpawnClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::LootContainer:
			return DefaultLootContainerClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::LootContainerSpawn:
			return DefaultLootContainerSpawnClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::SelfDestruct:
			return DefaultSelfDestructClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::RollingBomberSpawner:
			return DefaultRollingBomberSpawnerClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ExtractionPoint:
			return DefaultExtractionPointClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::SandbagCover:
			return DefaultSandbagCoverClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ExplosiveBarrel:
			return DefaultExplosiveBarrelClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::StaticMeshProp:
			return DefaultStaticMeshPropClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ShootingPracticeDummy:
			return DefaultShootingPracticeDummyClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::Shop:
			return DefaultShopClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::Workbench:
			return DefaultWorkbenchClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::PiggyBank:
			return DefaultPiggyBankClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::PeriodicNoiseEmitter:
			return DefaultPeriodicNoiseEmitterClassPath;
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::DifficultyAdjustment:
			return DefaultDifficultyAdjustmentClassPath;
		default:
			return nullptr;
		}
	}

	FText GetDefaultGameplayInteractionDisplayName(
		UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType SpawnType)
	{
		switch (SpawnType)
		{
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::LevelTravel:
			return FText::FromString(TEXT("Travel"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ItemSpawn:
			return FText::FromString(TEXT("\uC544\uC774\uD15C\uC2A4\uD3F0"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::LootContainerSpawn:
			return FText::FromString(TEXT("\uC0C1\uC790\uC2A4\uD3F0"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::SelfDestruct:
			return FText::FromString(TEXT("\uC790\uD3ED"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::RollingBomberSpawner:
			return FText::FromString(TEXT("Rolling Bomber Spawner"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ExtractionPoint:
			return FText::FromString(TEXT("Extraction"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::SandbagCover:
			return FText::FromString(TEXT("Sandbag Cover"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ExplosiveBarrel:
			return FText::FromString(TEXT("Explosive Barrel"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::StaticMeshProp:
			return FText::GetEmpty();
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::ShootingPracticeDummy:
			return FText::FromString(TEXT("Practice Dummy"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::Shop:
			return FText::FromString(TEXT("\uC0C1\uC810"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::Workbench:
			return FText::FromString(TEXT("\uC791\uC5C5\uB300"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::PiggyBank:
			return FText::FromString(TEXT("\uB3C8\uB0B4\uB194"));
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::PeriodicNoiseEmitter:
			return FText::GetEmpty();
		case UTunaSweeperEnemySpawnSubsystem::EGameplayInteractionActorSpawnType::DifficultyAdjustment:
			return FText::FromString(TEXT("\uB09C\uC774\uB3C4 \uC870\uC815"));
		default:
			return FText::GetEmpty();
		}
	}
}

void UTunaSweeperEnemySpawnSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	PostLoadMapHandle = FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(
		this,
		&UTunaSweeperEnemySpawnSubsystem::HandlePostLoadMapWithWorld);
}

void UTunaSweeperEnemySpawnSubsystem::Deinitialize()
{
	if (PostLoadMapHandle.IsValid())
	{
		FCoreUObjectDelegates::PostLoadMapWithWorld.Remove(PostLoadMapHandle);
		PostLoadMapHandle.Reset();
	}

	ResetLoadedEnemySpawnData();
	ResetLoadedEnemyCombatProfileData();
	ResetLoadedLootContainerSpawnData();
	ResetLoadedTransparentObstacleSpawnData();
	ResetLoadedWorldProgressObjectSpawnData();
	ResetLoadedWarpPointSpawnData();
	ResetLoadedGameplayInteractionActorSpawnData();
	LastSpawnedWorld.Reset();
	Super::Deinitialize();
}

bool UTunaSweeperEnemySpawnSubsystem::EnsureEnemiesSpawnedForWorld(UWorld* World)
{
	return EnsureRaidRuntimeActorsSpawnedForWorld(World);
}

bool UTunaSweeperEnemySpawnSubsystem::EnsureRaidRuntimeActorsSpawnedForWorld(UWorld* World)
{
	if (!World || !World->IsGameWorld())
	{
		return true;
	}

	if (LastSpawnedWorld.Get() == World)
	{
		return true;
	}

	const bool bLoadedEnemies = LoadEnemySpawnData(false);
	const bool bLoadedLootContainers = LoadLootContainerSpawnData(false);
	const bool bLoadedTransparentObstacles = LoadTransparentObstacleSpawnData(false);
	const bool bLoadedWorldProgressObjects = LoadWorldProgressObjectSpawnData(false);
	const bool bLoadedWarpPoints = LoadWarpPointSpawnData(false);
	const bool bLoadedGameplayInteractionActors = LoadGameplayInteractionActorSpawnData(false);
	if (!bLoadedEnemies && !bLoadedLootContainers && !bLoadedTransparentObstacles && !bLoadedWorldProgressObjects && !bLoadedWarpPoints && !bLoadedGameplayInteractionActors)
	{
		return false;
	}

	LastSpawnedWorld = World;

	int32 SpawnedCount = 0;
	if (bLoadedEnemies)
	{
		for (const FEnemySpawnDefinition& SpawnDefinition : EnemySpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperEnemyCharacter> LoadedEnemyClass = SpawnDefinition.EnemyClass.LoadSynchronous();
			if (!LoadedEnemyClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("Enemy class failed to load for level %s. Falling back to native enemy character."),
					*SpawnDefinition.LevelName.ToString());
				LoadedEnemyClass = ATunaSweeperEnemyCharacter::StaticClass();
			}

			const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location);
			ATunaSweeperEnemyCharacter* SpawnedEnemy = World->SpawnActorDeferred<ATunaSweeperEnemyCharacter>(
				LoadedEnemyClass,
				SpawnTransform,
				nullptr,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedEnemy)
			{
				SpawnedEnemy->ConfigureCombatProfile(
					SpawnDefinition.CombatProfile,
					SpawnDefinition.FactionId,
					SpawnDefinition.SquadId,
					SpawnDefinition.SquadSlot);
				SpawnedEnemy->ConfigureSpawnData(
					SpawnDefinition.BodyMaterial,
					SpawnDefinition.EnemyId,
					SpawnDefinition.DropContainerDefinitionId,
					SpawnDefinition.DropContentsId,
					SpawnDefinition.MaxHealth,
					SpawnDefinition.ExperienceValue,
					SpawnDefinition.BleedingChanceBonus,
					SpawnDefinition.BleedingDurationBonusSeconds,
					SpawnDefinition.WeaponItemId,
					SpawnDefinition.AmmoItemId,
					SpawnDefinition.ReserveAmmoCount,
					SpawnDefinition.LootLoadedAmmoDeductionRatio,
					SpawnDefinition.LootLoadedAmmoFlatDeduction);
				UGameplayStatics::FinishSpawningActor(SpawnedEnemy, SpawnTransform);
				++SpawnedCount;
			}
		}
	}

	int32 SpawnedLootContainerCount = 0;
	if (bLoadedLootContainers)
	{
		for (const FLootContainerSpawnDefinition& SpawnDefinition : LootContainerSpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}
			if (SpawnDefinition.bEditorOnly && !TunaSweeperEnemySpawn::ShouldIncludeEditorOnlySpawn(World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperLootContainerActor> LoadedContainerClass = SpawnDefinition.LootContainerClass.LoadSynchronous();
			if (!LoadedContainerClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("Loot container class failed to load for level %s. Falling back to native loot container actor."),
					*SpawnDefinition.LevelName.ToString());
				LoadedContainerClass = ATunaSweeperLootContainerActor::StaticClass();
			}

			FActorSpawnParameters SpawnParameters;
			SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			ATunaSweeperLootContainerActor* SpawnedContainer = World->SpawnActor<ATunaSweeperLootContainerActor>(
				LoadedContainerClass,
				SpawnDefinition.Location,
				SpawnDefinition.Rotation,
				SpawnParameters);
			if (SpawnedContainer)
			{
				SpawnedContainer->SetContainerDataIds(
					SpawnDefinition.ContainerDefinitionId,
					SpawnDefinition.ContentsId);
				++SpawnedLootContainerCount;
			}
		}
	}

	int32 SpawnedTransparentObstacleCount = 0;
	if (bLoadedTransparentObstacles)
	{
		for (const FTransparentObstacleSpawnDefinition& SpawnDefinition : TransparentObstacleSpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperTransparentObstacleActor> LoadedObstacleClass = SpawnDefinition.ObstacleClass.LoadSynchronous();
			if (!LoadedObstacleClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("Transparent obstacle class failed to load for level %s. Falling back to native transparent obstacle actor."),
					*SpawnDefinition.LevelName.ToString());
				LoadedObstacleClass = ATunaSweeperTransparentObstacleActor::StaticClass();
			}

			const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location);
			ATunaSweeperTransparentObstacleActor* SpawnedObstacle =
				World->SpawnActorDeferred<ATunaSweeperTransparentObstacleActor>(
					LoadedObstacleClass,
					SpawnTransform,
					nullptr,
					nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedObstacle)
			{
				SpawnedObstacle->ConfigureObstacleDefaults(SpawnDefinition.ObstacleId, SpawnDefinition.BoxExtent);
				if (!SpawnDefinition.ObstacleId.IsNone())
				{
					SpawnedObstacle->Tags.AddUnique(SpawnDefinition.ObstacleId);
				}
				UGameplayStatics::FinishSpawningActor(SpawnedObstacle, SpawnTransform);
				++SpawnedTransparentObstacleCount;
			}
		}
	}

	int32 SpawnedWorldProgressObjectCount = 0;
	if (bLoadedWorldProgressObjects)
	{
		for (const FWorldProgressObjectSpawnDefinition& SpawnDefinition : WorldProgressObjectSpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperWorldProgressActor> LoadedProgressActorClass =
				SpawnDefinition.ProgressActorClass.LoadSynchronous();
			if (!LoadedProgressActorClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("World progress actor class failed to load for level %s. Falling back to native world progress actor."),
					*SpawnDefinition.LevelName.ToString());
				LoadedProgressActorClass = ATunaSweeperWorldProgressActor::StaticClass();
			}

			const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location);
			ATunaSweeperWorldProgressActor* SpawnedProgressActor =
				World->SpawnActorDeferred<ATunaSweeperWorldProgressActor>(
					LoadedProgressActorClass,
					SpawnTransform,
					nullptr,
					nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedProgressActor)
			{
				SpawnedProgressActor->ConfigureWorldProgressDefaults(
					SpawnDefinition.ObjectId,
					SpawnDefinition.InfoId,
					SpawnDefinition.DisplayName,
					SpawnDefinition.InteractionDisplayName,
					SpawnDefinition.RequiredItemId,
					SpawnDefinition.RequiredQuantity,
					SpawnDefinition.InitialProgressQuantity,
					SpawnDefinition.RequiredItemDisplayName,
					SpawnDefinition.BoxExtent,
					SpawnDefinition.CompletedActorClass,
					SpawnDefinition.DisplayNameStringKey,
					SpawnDefinition.InteractionDisplayNameStringKey,
					SpawnDefinition.RequiredItemDisplayNameStringKey);
				if (!SpawnDefinition.ObjectId.IsNone())
				{
					SpawnedProgressActor->Tags.AddUnique(SpawnDefinition.ObjectId);
				}
				UGameplayStatics::FinishSpawningActor(SpawnedProgressActor, SpawnTransform);
				++SpawnedWorldProgressObjectCount;
			}
		}
	}

	int32 SpawnedWarpPointCount = 0;
	if (bLoadedWarpPoints)
	{
		for (const FWarpPointSpawnDefinition& SpawnDefinition : WarpPointSpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}

			TSubclassOf<ATunaSweeperWarpPointActor> LoadedWarpPointClass =
				SpawnDefinition.WarpPointClass.LoadSynchronous();
			if (!LoadedWarpPointClass)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("Warp point class failed to load for level %s. Falling back to native warp point actor."),
					*SpawnDefinition.LevelName.ToString());
				LoadedWarpPointClass = ATunaSweeperWarpPointActor::StaticClass();
			}

			const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location);
			ATunaSweeperWarpPointActor* SpawnedWarpPoint =
				World->SpawnActorDeferred<ATunaSweeperWarpPointActor>(
					LoadedWarpPointClass,
					SpawnTransform,
					nullptr,
					nullptr,
					ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedWarpPoint)
			{
				SpawnedWarpPoint->ConfigureWarpPointDefaults(
					SpawnDefinition.WarpPointId,
					SpawnDefinition.TargetWarpPointId,
					FText::FromString(TEXT("\uC0C1\uD638\uC791\uC6A9")),
					TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
						FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
					TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TunaSweeperEnemySpawn::DefaultWarpPointMaterialPath)),
					TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TunaSweeperEnemySpawn::DefaultWarpPointSphereMeshPath)),
					SpawnDefinition.VisualScale,
					SpawnDefinition.VisualRelativeLocation,
					SpawnDefinition.ExitOffset,
					SpawnDefinition.bUseTargetRotation);
				if (!SpawnDefinition.WarpPointId.IsNone())
				{
					SpawnedWarpPoint->Tags.AddUnique(SpawnDefinition.WarpPointId);
				}
				UGameplayStatics::FinishSpawningActor(SpawnedWarpPoint, SpawnTransform);
				++SpawnedWarpPointCount;
			}
		}
	}

	int32 SpawnedGameplayInteractionActorCount = 0;
	if (bLoadedGameplayInteractionActors)
	{
		for (const FGameplayInteractionActorSpawnDefinition& SpawnDefinition : GameplayInteractionActorSpawnDefinitions)
		{
			if (!DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
			{
				continue;
			}
			if (SpawnDefinition.bRequiresDeveloperPiggyBank &&
				!ATunaSweeperPlayerController::GetDeveloperPiggyBankPreference())
			{
				continue;
			}

			UClass* LoadedActorClass = SpawnDefinition.ActorClass.LoadSynchronous();
			if (!LoadedActorClass || !LoadedActorClass->IsChildOf(AActor::StaticClass()))
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Warning,
					TEXT("Gameplay interaction actor class failed to load for %s. Skipping spawn %s."),
					*SpawnDefinition.LevelName.ToString(),
					*SpawnDefinition.SpawnId.ToString());
				continue;
			}

			if (!SpawnDefinition.SpawnId.IsNone())
			{
				TArray<AActor*> ExistingActorsWithSpawnId;
				UGameplayStatics::GetAllActorsWithTag(World, SpawnDefinition.SpawnId, ExistingActorsWithSpawnId);
				for (AActor* ExistingActorWithSpawnId : ExistingActorsWithSpawnId)
				{
					if (!IsValid(ExistingActorWithSpawnId))
					{
						continue;
					}

					ExistingActorWithSpawnId->SetActorHiddenInGame(true);
					ExistingActorWithSpawnId->SetActorEnableCollision(false);
					ExistingActorWithSpawnId->Destroy();
				}

				if (ExistingActorsWithSpawnId.Num() > 0)
				{
					UE_LOG(
						LogTunaSweeperEnemySpawn,
						Log,
						TEXT("Removed %d existing gameplay interaction actor(s) with spawn id %s before spawning JSON actor."),
						ExistingActorsWithSpawnId.Num(),
						*SpawnDefinition.SpawnId.ToString());
				}
			}

			TArray<AActor*> ExistingActorsAtSpawnLocation;
			UGameplayStatics::GetAllActorsOfClass(World, LoadedActorClass, ExistingActorsAtSpawnLocation);
			int32 RemovedLegacyActorCount = 0;
			for (AActor* ExistingActor : ExistingActorsAtSpawnLocation)
			{
				if (!IsValid(ExistingActor) || ExistingActor->IsActorBeingDestroyed())
				{
					continue;
				}

				const FString SpawnIdText = SpawnDefinition.SpawnId.ToString();
				const bool bMatchesSpawnIdName = !SpawnDefinition.SpawnId.IsNone() &&
					(ExistingActor->GetFName() == SpawnDefinition.SpawnId || ExistingActor->GetName().StartsWith(SpawnIdText));
#if WITH_EDITOR
				const bool bMatchesSpawnIdLabel = !SpawnDefinition.SpawnId.IsNone() &&
					ExistingActor->GetActorLabel().Equals(SpawnIdText, ESearchCase::CaseSensitive);
#else
				const bool bMatchesSpawnIdLabel = false;
#endif
				const bool bMatchesSpawnLocation = ExistingActor->GetActorLocation().Equals(SpawnDefinition.Location, 2.0f);
				if (!bMatchesSpawnIdName && !bMatchesSpawnIdLabel && !bMatchesSpawnLocation)
				{
					continue;
				}

				ExistingActor->SetActorHiddenInGame(true);
				ExistingActor->SetActorEnableCollision(false);
				ExistingActor->Destroy();
				++RemovedLegacyActorCount;
			}

			if (RemovedLegacyActorCount > 0)
			{
				UE_LOG(
					LogTunaSweeperEnemySpawn,
					Log,
					TEXT("Removed %d existing gameplay interaction actor(s) of class %s before spawning JSON actor %s."),
					RemovedLegacyActorCount,
					*GetNameSafe(LoadedActorClass),
					*SpawnDefinition.SpawnId.ToString());
			}

			const FTransform SpawnTransform(SpawnDefinition.Rotation, SpawnDefinition.Location, SpawnDefinition.Scale);
			AActor* SpawnedActor = World->SpawnActorDeferred<AActor>(
				LoadedActorClass,
				SpawnTransform,
				nullptr,
				nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			if (SpawnedActor)
			{
				ConfigureGameplayInteractionActor(SpawnedActor, SpawnDefinition);
				if (!SpawnDefinition.SpawnId.IsNone())
				{
					SpawnedActor->Tags.AddUnique(SpawnDefinition.SpawnId);
#if WITH_EDITOR
					SpawnedActor->SetActorLabel(SpawnDefinition.SpawnId.ToString());
#endif
				}
				UGameplayStatics::FinishSpawningActor(SpawnedActor, SpawnTransform);
				SpawnedActor->SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);
				++SpawnedGameplayInteractionActorCount;
			}
		}
	}

	UE_LOG(
		LogTunaSweeperEnemySpawn,
		Log,
		TEXT("Spawned %d enemies, %d loot containers, %d transparent obstacles, %d world progress objects, %d warp points, and %d gameplay interaction actors for level %s."),
		SpawnedCount,
		SpawnedLootContainerCount,
		SpawnedTransparentObstacleCount,
		SpawnedWorldProgressObjectCount,
		SpawnedWarpPointCount,
		SpawnedGameplayInteractionActorCount,
		*TunaSweeperEnemySpawn::NormalizeLevelName(World->GetMapName()));
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadEnemyCombatProfileData(bool bForceReload)
{
	if (bEnemyCombatProfileDataLoaded && !bForceReload)
	{
		return true;
	}

	if (bForceReload)
	{
		ResetLoadedEnemySpawnData();
	}
	ResetLoadedEnemyCombatProfileData();

	FString JsonContent;
	const FString EnemyCombatProfileJsonPath = GetEnemyCombatProfileJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *EnemyCombatProfileJsonPath))
	{
		UE_LOG(
			LogTunaSweeperEnemySpawn,
			Error,
			TEXT("Failed to read enemy combat profile JSON: %s"),
			*EnemyCombatProfileJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(
			LogTunaSweeperEnemySpawn,
			Error,
			TEXT("Failed to parse enemy combat profile JSON: %s"),
			*EnemyCombatProfileJsonPath);
		return false;
	}

	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy combat profile row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString ProfileIdString;
		FString AttackModeString;
		FString CombatRoleString;
		if (!JsonObject->TryGetStringField(TEXT("profile_id"), ProfileIdString) ||
			!JsonObject->TryGetStringField(TEXT("attack_mode"), AttackModeString) ||
			!JsonObject->TryGetStringField(TEXT("role"), CombatRoleString))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy combat profile row %d: required field is missing."), RowIndex);
			continue;
		}

		FTunaSweeperEnemyCombatProfile Profile;
		Profile.ProfileId = FName(*ProfileIdString.TrimStartAndEnd());
		if (Profile.ProfileId.IsNone() ||
			!TunaSweeperEnemySpawn::TryResolveEnemyAttackMode(AttackModeString, Profile.AttackMode) ||
			!TunaSweeperEnemySpawn::TryResolveEnemyCombatRole(CombatRoleString, Profile.Role))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy combat profile row %d: profile_id, attack_mode, or role is invalid."), RowIndex);
			continue;
		}
		if (EnemyCombatProfilesById.Contains(Profile.ProfileId))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping duplicate enemy combat profile '%s'."), *Profile.ProfileId.ToString());
			continue;
		}
		if ((Profile.AttackMode == ETunaSweeperEnemyAttackMode::Melee) !=
			(Profile.Role == ETunaSweeperEnemyCombatRole::Melee))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy combat profile '%s': melee attack mode and role must agree."), *Profile.ProfileId.ToString());
			continue;
		}

		Profile.MovementSpeed = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("movement_speed"), Profile.MovementSpeed);
		Profile.TrackingRange = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("tracking_range"), Profile.TrackingRange);
		Profile.PreferredRangeMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("preferred_range_min"), Profile.PreferredRangeMin);
		Profile.PreferredRangeMax = FMath::Max(
			Profile.PreferredRangeMin,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("preferred_range_max"), Profile.PreferredRangeMax));
		Profile.DangerRange = FMath::Min(
			Profile.PreferredRangeMin,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("danger_range"), Profile.DangerRange));
		Profile.AlertSeconds = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("alert_seconds"), Profile.AlertSeconds);
		Profile.AimSecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("aim_seconds_min"), Profile.AimSecondsMin);
		Profile.AimSecondsMax = FMath::Max(
			Profile.AimSecondsMin,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("aim_seconds_max"), Profile.AimSecondsMax));
		Profile.TurnSpeedDegreesPerSecond = FMath::Max(
			1.0f,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(
				JsonObject,
				TEXT("turn_speed_degrees_per_second"),
				Profile.TurnSpeedDegreesPerSecond));
		Profile.AttackFacingToleranceDegrees = FMath::Clamp(
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(
				JsonObject,
				TEXT("attack_facing_tolerance_degrees"),
				Profile.AttackFacingToleranceDegrees),
			0.0f,
			90.0f);
		Profile.WeaponSpreadMultiplier = FMath::Max(
			0.01f,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(
				JsonObject,
				TEXT("weapon_spread_multiplier"),
				Profile.WeaponSpreadMultiplier));
		Profile.FiringShotCount = TunaSweeperEnemySpawn::ReadNonNegativeIntField(JsonObject, TEXT("firing_shot_count"), Profile.FiringShotCount);
		Profile.OpeningFiringShotCount = TunaSweeperEnemySpawn::ReadNonNegativeIntField(JsonObject, TEXT("opening_firing_shot_count"), Profile.OpeningFiringShotCount);
		Profile.ShotIntervalSecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("shot_interval_seconds_min"), Profile.ShotIntervalSecondsMin);
		Profile.ShotIntervalSecondsMax = FMath::Max(
			Profile.ShotIntervalSecondsMin,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("shot_interval_seconds_max"), Profile.ShotIntervalSecondsMax));
		Profile.RecoverSecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("recover_seconds_min"), Profile.RecoverSecondsMin);
		Profile.RecoverSecondsMax = FMath::Max(
			Profile.RecoverSecondsMin,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("recover_seconds_max"), Profile.RecoverSecondsMax));
		Profile.ObserveSecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("observe_seconds_min"), Profile.ObserveSecondsMin);
		Profile.ObserveSecondsMax = FMath::Max(
			Profile.ObserveSecondsMin,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("observe_seconds_max"), Profile.ObserveSecondsMax));
		Profile.ReloadReadySecondsMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("reload_ready_seconds_min"), Profile.ReloadReadySecondsMin);
		Profile.ReloadReadySecondsMax = FMath::Max(
			Profile.ReloadReadySecondsMin,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("reload_ready_seconds_max"), Profile.ReloadReadySecondsMax));
		Profile.PositionFiringBudgetMin = TunaSweeperEnemySpawn::ReadNonNegativeIntField(JsonObject, TEXT("position_firing_budget_min"), Profile.PositionFiringBudgetMin);
		Profile.PositionFiringBudgetMax = FMath::Max(
			Profile.PositionFiringBudgetMin,
			TunaSweeperEnemySpawn::ReadNonNegativeIntField(JsonObject, TEXT("position_firing_budget_max"), Profile.PositionFiringBudgetMax));
		Profile.RepositionDistanceMin = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("reposition_distance_min"), Profile.RepositionDistanceMin);
		Profile.RepositionDistanceMax = FMath::Max(
			Profile.RepositionDistanceMin,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("reposition_distance_max"), Profile.RepositionDistanceMax));
		Profile.CrossRepositionChance = FMath::Clamp(
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("cross_reposition_chance"), Profile.CrossRepositionChance),
			0.0f,
			1.0f);
		Profile.CrossRepositionCooldownSeconds = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(
			JsonObject,
			TEXT("cross_reposition_cooldown_seconds"),
			Profile.CrossRepositionCooldownSeconds);
		Profile.CrossRepositionOrbitRadius = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(
			JsonObject,
			TEXT("cross_reposition_orbit_radius"),
			Profile.CrossRepositionOrbitRadius);
		Profile.MeleeAttackDamage = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("melee_attack_damage"), Profile.MeleeAttackDamage);
		Profile.MeleeApproachStartRange = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("melee_approach_start_range"), Profile.MeleeApproachStartRange);
		Profile.MeleeApproachStopRange = FMath::Min(
			Profile.MeleeApproachStartRange,
			TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("melee_approach_stop_range"), Profile.MeleeApproachStopRange));
		Profile.AttackCooldownSeconds = TunaSweeperEnemySpawn::ReadNonNegativeFloatField(JsonObject, TEXT("attack_cooldown_seconds"), Profile.AttackCooldownSeconds);

		if (Profile.AttackMode == ETunaSweeperEnemyAttackMode::Ranged)
		{
			Profile.FiringShotCount = FMath::Max(1, Profile.FiringShotCount);
			Profile.OpeningFiringShotCount = FMath::Clamp(Profile.OpeningFiringShotCount, 1, Profile.FiringShotCount);
			Profile.PositionFiringBudgetMin = FMath::Max(1, Profile.PositionFiringBudgetMin);
			Profile.PositionFiringBudgetMax = FMath::Max(Profile.PositionFiringBudgetMin, Profile.PositionFiringBudgetMax);
		}
		else
		{
			Profile.FiringShotCount = 0;
			Profile.OpeningFiringShotCount = 0;
			Profile.PositionFiringBudgetMin = 0;
			Profile.PositionFiringBudgetMax = 0;
			Profile.CrossRepositionChance = 0.0f;
		}

		EnemyCombatProfilesById.Add(Profile.ProfileId, Profile);
	}

	if (!EnemyCombatProfilesById.Contains(TunaSweeperEnemySpawn::DefaultEnemyCombatProfileId))
	{
		UE_LOG(
			LogTunaSweeperEnemySpawn,
			Error,
			TEXT("Enemy combat profile JSON is missing required default profile '%s': %s"),
			*TunaSweeperEnemySpawn::DefaultEnemyCombatProfileId.ToString(),
			*EnemyCombatProfileJsonPath);
		ResetLoadedEnemyCombatProfileData();
		return false;
	}

	bEnemyCombatProfileDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::TryGetEnemyCombatProfile(
	FName ProfileId,
	FTunaSweeperEnemyCombatProfile& OutProfile)
{
	OutProfile = FTunaSweeperEnemyCombatProfile();
	if (ProfileId.IsNone() || !LoadEnemyCombatProfileData(false))
	{
		return false;
	}

	const FTunaSweeperEnemyCombatProfile* Profile = EnemyCombatProfilesById.Find(ProfileId);
	if (!Profile)
	{
		return false;
	}

	OutProfile = *Profile;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadEnemySpawnData(bool bForceReload)
{
	if (bEnemySpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	if (!LoadEnemyCombatProfileData(bForceReload))
	{
		return false;
	}

	ResetLoadedEnemySpawnData();

	FString JsonContent;
	const FString EnemySpawnJsonPath = GetEnemySpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *EnemySpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read enemy spawn JSON: %s"), *EnemySpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse enemy spawn JSON: %s"), *EnemySpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		// Anchor-schema rows are owned by UTunaSweeperRaidPlacementSubsystem. Never let a
		// mixed row spawn twice through the coordinate-based legacy path.
		if (JsonObject->HasField(TEXT("placement_id")))
		{
			if (JsonObject->HasField(TEXT("location")) || JsonObject->HasField(TEXT("rotation")) || JsonObject->HasField(TEXT("scale")))
			{
				UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Enemy spawn row %d mixes anchor placement_id with transform fields; skipped."), RowIndex);
			}
			continue;
		}
		FString LevelName;
		FString EnemyId;
		FString EnemyClassPath;
		FString BodyMaterialPath;
		FString CombatProfileIdString;
		FString SquadIdString;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		double NumericDropContainerDefinitionId = INDEX_NONE;
		double NumericDropContentsId = INDEX_NONE;
		double NumericWeaponItemId = INDEX_NONE;
		double NumericAmmoItemId = INDEX_NONE;
		double NumericReserveAmmoCount = INDEX_NONE;
		double NumericLootLoadedAmmoDeductionRatio = 0.35;
		double NumericLootLoadedAmmoFlatDeduction = 0.0;
		double NumericMaxHealth = 30.0;
		double NumericExperienceValue = 30.0;
		double NumericBleedingChanceBonus = 0.0;
		double NumericBleedingDurationBonusSeconds = 0.0;
		double NumericFactionId = TunaSweeperFactionIds::NoFaction;
		double NumericSquadSlot = INDEX_NONE;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("enemy_id"), EnemyId);
		JsonObject->TryGetStringField(TEXT("enemy_class"), EnemyClassPath);
		JsonObject->TryGetStringField(TEXT("body_material"), BodyMaterialPath);
		JsonObject->TryGetStringField(TEXT("combat_profile_id"), CombatProfileIdString);
		JsonObject->TryGetStringField(TEXT("squad_id"), SquadIdString);
		JsonObject->TryGetNumberField(TEXT("drop_container_definition_id"), NumericDropContainerDefinitionId);
		JsonObject->TryGetNumberField(TEXT("drop_contents_id"), NumericDropContentsId);
		JsonObject->TryGetNumberField(TEXT("weapon_item_id"), NumericWeaponItemId);
		JsonObject->TryGetNumberField(TEXT("ammo_item_id"), NumericAmmoItemId);
		JsonObject->TryGetNumberField(TEXT("reserve_ammo_count"), NumericReserveAmmoCount);
		JsonObject->TryGetNumberField(TEXT("loot_loaded_ammo_deduction_ratio"), NumericLootLoadedAmmoDeductionRatio);
		JsonObject->TryGetNumberField(TEXT("loot_loaded_ammo_flat_deduction"), NumericLootLoadedAmmoFlatDeduction);
		JsonObject->TryGetNumberField(TEXT("max_health"), NumericMaxHealth);
		JsonObject->TryGetNumberField(TEXT("experience_value"), NumericExperienceValue);
		JsonObject->TryGetNumberField(TEXT("bleeding_chance_bonus"), NumericBleedingChanceBonus) ||
			JsonObject->TryGetNumberField(TEXT("bleed_chance_bonus"), NumericBleedingChanceBonus);
		JsonObject->TryGetNumberField(TEXT("bleeding_duration_bonus_seconds"), NumericBleedingDurationBonusSeconds) ||
			JsonObject->TryGetNumberField(TEXT("bleed_duration_bonus_seconds"), NumericBleedingDurationBonusSeconds);
		JsonObject->TryGetNumberField(TEXT("faction_id"), NumericFactionId);
		JsonObject->TryGetNumberField(TEXT("squad_slot"), NumericSquadSlot);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);

		FEnemySpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.EnemyId = EnemyId.TrimStartAndEnd().IsEmpty()
			? NAME_None
			: FName(*EnemyId.TrimStartAndEnd());
		SpawnDefinition.EnemyClass = TSoftClassPtr<ATunaSweeperEnemyCharacter>(
			FSoftObjectPath(EnemyClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultEnemyClassPath)
				: EnemyClassPath.TrimStartAndEnd()));
		const FString TrimmedBodyMaterialPath = BodyMaterialPath.TrimStartAndEnd();
		if (!TrimmedBodyMaterialPath.IsEmpty())
		{
			SpawnDefinition.BodyMaterial = TSoftObjectPtr<UMaterialInterface>(
				FSoftObjectPath(TrimmedBodyMaterialPath));
		}
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.DropContainerDefinitionId = static_cast<int32>(NumericDropContainerDefinitionId);
		SpawnDefinition.DropContentsId = static_cast<int32>(NumericDropContentsId);
		SpawnDefinition.WeaponItemId = static_cast<int32>(NumericWeaponItemId);
		SpawnDefinition.AmmoItemId = static_cast<int32>(NumericAmmoItemId);
		SpawnDefinition.ReserveAmmoCount = static_cast<int32>(NumericReserveAmmoCount);
		SpawnDefinition.LootLoadedAmmoDeductionRatio =
			FMath::Clamp(static_cast<float>(NumericLootLoadedAmmoDeductionRatio), 0.0f, 1.0f);
		SpawnDefinition.LootLoadedAmmoFlatDeduction =
			FMath::Max(0, FMath::RoundToInt(NumericLootLoadedAmmoFlatDeduction));
		SpawnDefinition.ExperienceValue = FMath::Max(0, static_cast<int32>(NumericExperienceValue));
		SpawnDefinition.MaxHealth = FMath::Max(1.0f, static_cast<float>(NumericMaxHealth));
		SpawnDefinition.BleedingChanceBonus = TunaSweeperDataValues::ClampProbabilityValue(
			FMath::RoundToInt(NumericBleedingChanceBonus));
		SpawnDefinition.BleedingDurationBonusSeconds = FMath::Max(0.0f, static_cast<float>(NumericBleedingDurationBonusSeconds));
		SpawnDefinition.CombatProfileId = CombatProfileIdString.TrimStartAndEnd().IsEmpty()
			? TunaSweeperEnemySpawn::DefaultEnemyCombatProfileId
			: FName(*CombatProfileIdString.TrimStartAndEnd());
		const FTunaSweeperEnemyCombatProfile* ResolvedCombatProfile =
			EnemyCombatProfilesById.Find(SpawnDefinition.CombatProfileId);
		if (!ResolvedCombatProfile)
		{
			UE_LOG(
				LogTunaSweeperEnemySpawn,
				Warning,
				TEXT("Enemy spawn row %d references unknown combat profile '%s'; using '%s'."),
				RowIndex,
				*SpawnDefinition.CombatProfileId.ToString(),
				*TunaSweeperEnemySpawn::DefaultEnemyCombatProfileId.ToString());
			SpawnDefinition.CombatProfileId = TunaSweeperEnemySpawn::DefaultEnemyCombatProfileId;
			ResolvedCombatProfile = EnemyCombatProfilesById.Find(SpawnDefinition.CombatProfileId);
		}
		if (!ResolvedCombatProfile)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy spawn row %d: no combat profile could be resolved."), RowIndex);
			continue;
		}
		SpawnDefinition.CombatProfile = *ResolvedCombatProfile;

		const int32 ParsedFactionId = FMath::RoundToInt(NumericFactionId);
		SpawnDefinition.FactionId = (ParsedFactionId >= 1 && ParsedFactionId <= 254) ||
			ParsedFactionId == TunaSweeperFactionIds::NoFaction
			? static_cast<uint8>(ParsedFactionId)
			: TunaSweeperFactionIds::NoFaction;
		if (SpawnDefinition.FactionId == TunaSweeperFactionIds::NoFaction &&
			ParsedFactionId != TunaSweeperFactionIds::NoFaction)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Enemy spawn row %d has invalid faction_id %d; using NoFaction (255)."), RowIndex, ParsedFactionId);
		}
		SpawnDefinition.SquadId = SquadIdString.TrimStartAndEnd().IsEmpty()
			? NAME_None
			: FName(*SquadIdString.TrimStartAndEnd());
		SpawnDefinition.SquadSlot = SpawnDefinition.SquadId.IsNone()
			? INDEX_NONE
			: FMath::Max(0, FMath::RoundToInt(NumericSquadSlot));

		if (SpawnDefinition.LevelName.IsNone())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping enemy spawn row %d: level_name is empty."), RowIndex);
			continue;
		}

		EnemySpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows && JsonRows.Num() > 0)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Enemy spawn JSON has no valid rows: %s"), *EnemySpawnJsonPath);
		return false;
	}

	bEnemySpawnDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadLootContainerSpawnData(bool bForceReload)
{
	if (bLootContainerSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedLootContainerSpawnData();

	FString JsonContent;
	const FString LootContainerSpawnJsonPath = GetLootContainerSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *LootContainerSpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read loot container spawn JSON: %s"), *LootContainerSpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse loot container spawn JSON: %s"), *LootContainerSpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping loot container spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		// Anchor-schema rows are resolved by UTunaSweeperRaidPlacementSubsystem.
		if (JsonObject->HasField(TEXT("placement_id")))
		{
			if (JsonObject->HasField(TEXT("location")) || JsonObject->HasField(TEXT("rotation")) || JsonObject->HasField(TEXT("scale")))
			{
				UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Loot spawn row %d mixes anchor placement_id with transform fields; skipped."), RowIndex);
			}
			continue;
		}
		FString LevelName;
		FString LootContainerClassPath;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		bool bEditorOnly = false;
		double NumericContainerDefinitionId = INDEX_NONE;
		double NumericContentsId = INDEX_NONE;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location) ||
			!JsonObject->TryGetNumberField(TEXT("container_definition_id"), NumericContainerDefinitionId) ||
			!JsonObject->TryGetNumberField(TEXT("contents_id"), NumericContentsId))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping loot container spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("loot_container_class"), LootContainerClassPath);
		JsonObject->TryGetBoolField(TEXT("editor_only"), bEditorOnly);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);

		FLootContainerSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.LootContainerClass = TSoftClassPtr<ATunaSweeperLootContainerActor>(
			FSoftObjectPath(LootContainerClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultLootContainerClassPath)
				: LootContainerClassPath.TrimStartAndEnd()));
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.ContainerDefinitionId = static_cast<int32>(NumericContainerDefinitionId);
		SpawnDefinition.ContentsId = static_cast<int32>(NumericContentsId);
		SpawnDefinition.bEditorOnly = bEditorOnly;

		if (SpawnDefinition.LevelName.IsNone() ||
			SpawnDefinition.ContainerDefinitionId == INDEX_NONE ||
			SpawnDefinition.ContentsId == INDEX_NONE)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping loot container spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		LootContainerSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows && JsonRows.Num() > 0)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Loot container spawn JSON has no valid rows: %s"), *LootContainerSpawnJsonPath);
		return false;
	}

	bLootContainerSpawnDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadTransparentObstacleSpawnData(bool bForceReload)
{
	if (bTransparentObstacleSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedTransparentObstacleSpawnData();

	FString JsonContent;
	const FString ObstacleSpawnJsonPath = GetTransparentObstacleSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *ObstacleSpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read transparent obstacle spawn JSON: %s"), *ObstacleSpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse transparent obstacle spawn JSON: %s"), *ObstacleSpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping transparent obstacle spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString ObstacleId;
		FString ObstacleClassPath;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector BoxExtent(260.0f, 45.0f, 140.0f);
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!JsonObject->TryGetStringField(TEXT("obstacle_id"), ObstacleId) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping transparent obstacle spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("obstacle_class"), ObstacleClassPath);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("box_extent"), BoxExtent);

		FTransparentObstacleSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.ObstacleId = FName(*ObstacleId.TrimStartAndEnd());
		SpawnDefinition.ObstacleClass = TSoftClassPtr<ATunaSweeperTransparentObstacleActor>(
			FSoftObjectPath(ObstacleClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultTransparentObstacleClassPath)
				: ObstacleClassPath.TrimStartAndEnd()));
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.BoxExtent = FVector(
			FMath::Max(1.0f, BoxExtent.X),
			FMath::Max(1.0f, BoxExtent.Y),
			FMath::Max(1.0f, BoxExtent.Z));

		if (SpawnDefinition.LevelName.IsNone() || SpawnDefinition.ObstacleId.IsNone())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping transparent obstacle spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		TransparentObstacleSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows && JsonRows.Num() > 0)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Transparent obstacle spawn JSON has no valid rows: %s"), *ObstacleSpawnJsonPath);
		return false;
	}

	bTransparentObstacleSpawnDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadWorldProgressObjectSpawnData(bool bForceReload)
{
	if (bWorldProgressObjectSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedWorldProgressObjectSpawnData();

	FString JsonContent;
	const FString ProgressObjectSpawnJsonPath = GetWorldProgressObjectSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *ProgressObjectSpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read world progress object spawn JSON: %s"), *ProgressObjectSpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse world progress object spawn JSON: %s"), *ProgressObjectSpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping world progress object spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString ObjectId;
		FString InfoId;
		FString ProgressActorClassPath;
		FString CompletedActorClassPath;
		FString DisplayName;
		FString DisplayNameStringKey;
		FString InteractionDisplayName;
		FString InteractionDisplayNameStringKey;
		FString RequiredItemDisplayName;
		FString RequiredItemDisplayNameStringKey;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector BoxExtent(260.0f, 55.0f, 140.0f);
		double NumericRequiredItemId = 6002.0;
		double NumericRequiredQuantity = 2.0;
		double NumericInitialProgressQuantity = 0.0;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!JsonObject->TryGetStringField(TEXT("object_id"), ObjectId) ||
			!JsonObject->TryGetStringField(TEXT("info_id"), InfoId) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping world progress object spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("progress_actor_class"), ProgressActorClassPath);
		JsonObject->TryGetStringField(TEXT("completed_actor_class"), CompletedActorClassPath);
		JsonObject->TryGetStringField(TEXT("display_name"), DisplayName);
		JsonObject->TryGetStringField(TEXT("display_name_key"), DisplayNameStringKey);
		JsonObject->TryGetStringField(TEXT("interaction_display_name"), InteractionDisplayName);
		JsonObject->TryGetStringField(TEXT("interaction_display_name_key"), InteractionDisplayNameStringKey);
		JsonObject->TryGetStringField(TEXT("required_item_display_name"), RequiredItemDisplayName);
		JsonObject->TryGetStringField(TEXT("required_item_display_name_key"), RequiredItemDisplayNameStringKey);
		JsonObject->TryGetNumberField(TEXT("required_item_id"), NumericRequiredItemId);
		JsonObject->TryGetNumberField(TEXT("required_quantity"), NumericRequiredQuantity);
		JsonObject->TryGetNumberField(TEXT("initial_progress_quantity"), NumericInitialProgressQuantity);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("box_extent"), BoxExtent);

		FWorldProgressObjectSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.ObjectId = FName(*ObjectId.TrimStartAndEnd());
		SpawnDefinition.InfoId = FName(*InfoId.TrimStartAndEnd());
		SpawnDefinition.ProgressActorClass = TSoftClassPtr<ATunaSweeperWorldProgressActor>(
			FSoftObjectPath(ProgressActorClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultWorldProgressActorClassPath)
				: ProgressActorClassPath.TrimStartAndEnd()));
		SpawnDefinition.CompletedActorClass = TSoftClassPtr<AActor>(
			FSoftObjectPath(CompletedActorClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultWorldProgressCompletedActorClassPath)
				: CompletedActorClassPath.TrimStartAndEnd()));
		SpawnDefinition.DisplayName = DisplayName.TrimStartAndEnd().IsEmpty()
			? FText::FromString(TEXT("\uBD80\uC11C\uC9C4 \uB2E4\uB9AC"))
			: FText::FromString(DisplayName.TrimStartAndEnd());
		SpawnDefinition.DisplayNameStringKey = FName(*DisplayNameStringKey.TrimStartAndEnd());
		SpawnDefinition.InteractionDisplayName = InteractionDisplayName.TrimStartAndEnd().IsEmpty()
			? FText::FromString(TEXT("\uC218\uB9AC\uD558\uAE30"))
			: FText::FromString(InteractionDisplayName.TrimStartAndEnd());
		SpawnDefinition.InteractionDisplayNameStringKey = FName(*InteractionDisplayNameStringKey.TrimStartAndEnd());
		SpawnDefinition.RequiredItemDisplayName = RequiredItemDisplayName.TrimStartAndEnd().IsEmpty()
			? FText::FromString(TEXT("\uBAA9\uC7AC"))
			: FText::FromString(RequiredItemDisplayName.TrimStartAndEnd());
		SpawnDefinition.RequiredItemDisplayNameStringKey = FName(*RequiredItemDisplayNameStringKey.TrimStartAndEnd());
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.BoxExtent = FVector(
			FMath::Max(1.0f, BoxExtent.X),
			FMath::Max(1.0f, BoxExtent.Y),
			FMath::Max(1.0f, BoxExtent.Z));
		SpawnDefinition.RequiredItemId = static_cast<int32>(NumericRequiredItemId);
		SpawnDefinition.RequiredQuantity = FMath::Max(1, static_cast<int32>(NumericRequiredQuantity));
		SpawnDefinition.InitialProgressQuantity = FMath::Clamp(
			static_cast<int32>(NumericInitialProgressQuantity),
			0,
			SpawnDefinition.RequiredQuantity);

		if (SpawnDefinition.LevelName.IsNone() ||
			SpawnDefinition.ObjectId.IsNone() ||
			SpawnDefinition.InfoId.IsNone() ||
			SpawnDefinition.RequiredItemId == INDEX_NONE)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping world progress object spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		WorldProgressObjectSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows && JsonRows.Num() > 0)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("World progress object spawn JSON has no valid rows: %s"), *ProgressObjectSpawnJsonPath);
		return false;
	}

	bWorldProgressObjectSpawnDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadWarpPointSpawnData(bool bForceReload)
{
	if (bWarpPointSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedWarpPointSpawnData();

	FString JsonContent;
	const FString WarpPointSpawnJsonPath = GetWarpPointSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *WarpPointSpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read warp point spawn JSON: %s"), *WarpPointSpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse warp point spawn JSON: %s"), *WarpPointSpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping warp point spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString WarpPointId;
		FString TargetWarpPointId;
		FString WarpPointClassPath;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector VisualScale(1.2f, 1.2f, 1.2f);
		FVector VisualRelativeLocation = FVector::ZeroVector;
		FVector ExitOffset(160.0f, 0.0f, 0.0f);
		bool bUseTargetRotation = true;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!JsonObject->TryGetStringField(TEXT("warp_point_id"), WarpPointId) ||
			!JsonObject->TryGetStringField(TEXT("target_warp_point_id"), TargetWarpPointId) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping warp point spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("warp_point_class"), WarpPointClassPath);
		JsonObject->TryGetBoolField(TEXT("use_target_rotation"), bUseTargetRotation);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("visual_scale"), VisualScale);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("visual_relative_location"), VisualRelativeLocation);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("exit_offset"), ExitOffset);

		FWarpPointSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.WarpPointId = FName(*WarpPointId.TrimStartAndEnd());
		SpawnDefinition.TargetWarpPointId = FName(*TargetWarpPointId.TrimStartAndEnd());
		SpawnDefinition.WarpPointClass = TSoftClassPtr<ATunaSweeperWarpPointActor>(
			FSoftObjectPath(WarpPointClassPath.TrimStartAndEnd().IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultWarpPointClassPath)
				: WarpPointClassPath.TrimStartAndEnd()));
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.VisualScale = FVector(
			FMath::Max(0.01f, VisualScale.X),
			FMath::Max(0.01f, VisualScale.Y),
			FMath::Max(0.01f, VisualScale.Z));
		SpawnDefinition.VisualRelativeLocation = VisualRelativeLocation;
		SpawnDefinition.ExitOffset = ExitOffset;
		SpawnDefinition.bUseTargetRotation = bUseTargetRotation;

		if (SpawnDefinition.LevelName.IsNone() ||
			SpawnDefinition.WarpPointId.IsNone() ||
			SpawnDefinition.TargetWarpPointId.IsNone())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping warp point spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		WarpPointSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows && JsonRows.Num() > 0)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Warp point spawn JSON has no valid rows: %s"), *WarpPointSpawnJsonPath);
		return false;
	}

	bWarpPointSpawnDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::LoadGameplayInteractionActorSpawnData(bool bForceReload)
{
	if (bGameplayInteractionActorSpawnDataLoaded && !bForceReload)
	{
		return true;
	}

	ResetLoadedGameplayInteractionActorSpawnData();

	FString JsonContent;
	const FString GameplayInteractionActorSpawnJsonPath = GetGameplayInteractionActorSpawnJsonPath();
	if (!FFileHelper::LoadFileToString(JsonContent, *GameplayInteractionActorSpawnJsonPath))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to read gameplay interaction actor spawn JSON: %s"), *GameplayInteractionActorSpawnJsonPath);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> JsonRows;
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonContent);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonRows))
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Failed to parse gameplay interaction actor spawn JSON: %s"), *GameplayInteractionActorSpawnJsonPath);
		return false;
	}

	bool bHasValidRows = false;
	for (int32 RowIndex = 0; RowIndex < JsonRows.Num(); ++RowIndex)
	{
		const TSharedPtr<FJsonObject>* JsonObjectPtr = nullptr;
		if (!JsonRows[RowIndex].IsValid() || !JsonRows[RowIndex]->TryGetObject(JsonObjectPtr) ||
			!JsonObjectPtr || !JsonObjectPtr->IsValid())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping gameplay interaction actor spawn row %d: row is not an object."), RowIndex);
			continue;
		}

		const TSharedPtr<FJsonObject>& JsonObject = *JsonObjectPtr;
		FString LevelName;
		FString SpawnId;
		FString SpawnTypeText;
		FString ActorClassPath;
		FString InteractionDisplayName;
		FString InteractionDisplayNameStringKey;
		FString MarkerWidgetClassPath;
		bool bRequiresDeveloperPiggyBank = false;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector Scale = FVector::OneVector;
		if (!JsonObject->TryGetStringField(TEXT("level_name"), LevelName) ||
			!JsonObject->TryGetStringField(TEXT("spawn_id"), SpawnId) ||
			!JsonObject->TryGetStringField(TEXT("spawn_type"), SpawnTypeText) ||
			!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("location"), Location))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping gameplay interaction actor spawn row %d: required field is missing."), RowIndex);
			continue;
		}

		FGameplayInteractionActorSpawnDefinition SpawnDefinition;
		SpawnDefinition.LevelName = FName(*LevelName.TrimStartAndEnd());
		SpawnDefinition.SpawnId = FName(*SpawnId.TrimStartAndEnd());
		SpawnDefinition.SpawnType = TunaSweeperEnemySpawn::ReadGameplayInteractionActorSpawnType(SpawnTypeText);
		if (SpawnDefinition.LevelName.IsNone() ||
			SpawnDefinition.SpawnId.IsNone() ||
			SpawnDefinition.SpawnType == EGameplayInteractionActorSpawnType::Unknown)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping gameplay interaction actor spawn row %d: row has invalid identifiers."), RowIndex);
			continue;
		}

		const TCHAR* DefaultActorClassPath =
			TunaSweeperEnemySpawn::GetDefaultGameplayInteractionActorClassPath(SpawnDefinition.SpawnType);
		if (!DefaultActorClassPath)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping gameplay interaction actor spawn row %d: no default class for spawn type."), RowIndex);
			continue;
		}

		JsonObject->TryGetStringField(TEXT("actor_class"), ActorClassPath);
		JsonObject->TryGetStringField(TEXT("interaction_display_name"), InteractionDisplayName);
		JsonObject->TryGetStringField(TEXT("interaction_display_name_key"), InteractionDisplayNameStringKey);
		JsonObject->TryGetStringField(TEXT("marker_widget_class"), MarkerWidgetClassPath);
		JsonObject->TryGetBoolField(TEXT("requires_developer_piggy_bank"), bRequiresDeveloperPiggyBank);
		TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("rotation"), Rotation);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("scale"), Scale);

		const FString TrimmedActorClassPath = ActorClassPath.TrimStartAndEnd();
		const FString TrimmedMarkerWidgetClassPath = MarkerWidgetClassPath.TrimStartAndEnd();
		SpawnDefinition.ActorClass = TSoftClassPtr<AActor>(
			FSoftObjectPath(TrimmedActorClassPath.IsEmpty()
				? FString(DefaultActorClassPath)
				: TrimmedActorClassPath));
		SpawnDefinition.Location = Location;
		SpawnDefinition.Rotation = Rotation;
		SpawnDefinition.Scale = FVector(
			FMath::Max(0.01f, Scale.X),
			FMath::Max(0.01f, Scale.Y),
			FMath::Max(0.01f, Scale.Z));
		SpawnDefinition.InteractionDisplayName = InteractionDisplayName.TrimStartAndEnd().IsEmpty()
			? TunaSweeperEnemySpawn::GetDefaultGameplayInteractionDisplayName(SpawnDefinition.SpawnType)
			: FText::FromString(InteractionDisplayName.TrimStartAndEnd());
		SpawnDefinition.InteractionDisplayNameStringKey = FName(*InteractionDisplayNameStringKey.TrimStartAndEnd());
		SpawnDefinition.MarkerWidgetClass = TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TrimmedMarkerWidgetClassPath.IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultInteractionMarkerWidgetClassPath)
				: TrimmedMarkerWidgetClassPath));
		SpawnDefinition.bRequiresDeveloperPiggyBank = bRequiresDeveloperPiggyBank;
		SpawnDefinition.bHasMapOverlay = TunaSweeperEnemySpawn::TryReadMapOverlayDefinition(
			JsonObject,
			SpawnDefinition.LevelName,
			SpawnDefinition.SpawnId,
			SpawnDefinition.Location,
			SpawnDefinition.MapOverlay);

		FString TargetLevelName;
		FString TransitionMediaSourcePath;
		FString TransitionWidgetClassPath;
		FString TransitionMessage;
		FString TransitionMessageStringKey;
		JsonObject->TryGetStringField(TEXT("target_level_name"), TargetLevelName);
		JsonObject->TryGetStringField(TEXT("transition_media_source"), TransitionMediaSourcePath);
		JsonObject->TryGetStringField(TEXT("transition_widget_class"), TransitionWidgetClassPath);
		JsonObject->TryGetStringField(TEXT("transition_message"), TransitionMessage);
		JsonObject->TryGetStringField(TEXT("transition_message_key"), TransitionMessageStringKey);
		SpawnDefinition.TargetLevelName = TargetLevelName.TrimStartAndEnd().IsEmpty()
			? NAME_None
			: FName(*TargetLevelName.TrimStartAndEnd());
		const FString TrimmedTransitionMediaSourcePath = TransitionMediaSourcePath.TrimStartAndEnd();
		if (!TrimmedTransitionMediaSourcePath.IsEmpty())
		{
			SpawnDefinition.TransitionMediaSource = TSoftObjectPtr<UMediaSource>(
				FSoftObjectPath(TrimmedTransitionMediaSourcePath));
		}
		const FString TrimmedTransitionWidgetClassPath = TransitionWidgetClassPath.TrimStartAndEnd();
		SpawnDefinition.TransitionWidgetClass = TSoftClassPtr<UTunaSweeperLevelTransitionWidget>(
			FSoftObjectPath(TrimmedTransitionWidgetClassPath.IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultLevelTransitionWidgetClassPath)
				: TrimmedTransitionWidgetClassPath));
		SpawnDefinition.TransitionMessage = TransitionMessage.TrimStartAndEnd().IsEmpty()
			? FText::GetEmpty()
			: FText::FromString(TransitionMessage.TrimStartAndEnd());
		SpawnDefinition.TransitionMessageStringKey = FName(*TransitionMessageStringKey.TrimStartAndEnd());
		double NumericExtractionRadius = 300.0;
		double NumericExtractionRadiusMeters = 0.0;
		double NumericExtractionHoldSeconds = 4.0;
		double NumericExtractionRadiusRingWidth = 4.8;
		FString ExtractionParticleSystemPath;
		FString ExtractionRadiusVisualMaterialPath;
		JsonObject->TryGetNumberField(TEXT("extraction_radius"), NumericExtractionRadius);
		if (JsonObject->TryGetNumberField(TEXT("extraction_radius_meters"), NumericExtractionRadiusMeters) &&
			NumericExtractionRadiusMeters > 0.0)
		{
			NumericExtractionRadius = NumericExtractionRadiusMeters * 100.0;
		}
		JsonObject->TryGetNumberField(TEXT("extraction_hold_seconds"), NumericExtractionHoldSeconds);
		JsonObject->TryGetNumberField(TEXT("extraction_radius_ring_width"), NumericExtractionRadiusRingWidth);
		JsonObject->TryGetStringField(TEXT("extraction_particle_system"), ExtractionParticleSystemPath);
		JsonObject->TryGetStringField(TEXT("extraction_radius_visual_material"), ExtractionRadiusVisualMaterialPath);
		SpawnDefinition.ExtractionRadius = FMath::Max(1.0f, static_cast<float>(NumericExtractionRadius));
		SpawnDefinition.ExtractionHoldSeconds = FMath::Max(0.1f, static_cast<float>(NumericExtractionHoldSeconds));
		SpawnDefinition.ExtractionRadiusRingWidth = FMath::Max(1.0f, static_cast<float>(NumericExtractionRadiusRingWidth));
		const FString TrimmedExtractionParticleSystemPath = ExtractionParticleSystemPath.TrimStartAndEnd();
		if (!TrimmedExtractionParticleSystemPath.IsEmpty())
		{
			SpawnDefinition.ExtractionParticleSystem = TSoftObjectPtr<UNiagaraSystem>(
				FSoftObjectPath(TrimmedExtractionParticleSystemPath));
		}
		const FString TrimmedExtractionRadiusVisualMaterialPath = ExtractionRadiusVisualMaterialPath.TrimStartAndEnd();
		if (!TrimmedExtractionRadiusVisualMaterialPath.IsEmpty())
		{
			SpawnDefinition.ExtractionRadiusVisualMaterial = TSoftObjectPtr<UMaterialInterface>(
				FSoftObjectPath(TrimmedExtractionRadiusVisualMaterialPath));
		}

		double NumericItemId = 1001.0;
		double NumericItemQuantity = 1.0;
		bool bDestroyOnPickup = true;
		FString PickupItemIconWidgetClassPath;
		FString PickupItemActorClassPath;
		JsonObject->TryGetNumberField(TEXT("item_id"), NumericItemId);
		JsonObject->TryGetNumberField(TEXT("item_quantity"), NumericItemQuantity);
		JsonObject->TryGetBoolField(TEXT("destroy_on_pickup"), bDestroyOnPickup);
		JsonObject->TryGetStringField(TEXT("pickup_item_icon_widget_class"), PickupItemIconWidgetClassPath);
		JsonObject->TryGetStringField(TEXT("pickup_item_actor_class"), PickupItemActorClassPath);
		SpawnDefinition.ItemId = static_cast<int32>(NumericItemId);
		SpawnDefinition.ItemQuantity = FMath::Max(1, static_cast<int32>(NumericItemQuantity));
		SpawnDefinition.bDestroyOnPickup = bDestroyOnPickup;
		const FString TrimmedPickupItemIconWidgetClassPath = PickupItemIconWidgetClassPath.TrimStartAndEnd();
		SpawnDefinition.PickupItemIconWidgetClass = TSoftClassPtr<UTunaSweeperPickupItemIconWidget>(
			FSoftObjectPath(TrimmedPickupItemIconWidgetClassPath.IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultPickupItemIconWidgetClassPath)
				: TrimmedPickupItemIconWidgetClassPath));
		const FString TrimmedPickupItemActorClassPath = PickupItemActorClassPath.TrimStartAndEnd();
		SpawnDefinition.PickupItemActorClass = TSoftClassPtr<ATunaSweeperPickupItemActor>(
			FSoftObjectPath(TrimmedPickupItemActorClassPath.IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultPickupItemClassPath)
				: TrimmedPickupItemActorClassPath));

		double NumericContainerDefinitionId = INDEX_NONE;
		double NumericContentsId = INDEX_NONE;
		double NumericMinSpawnRadius = SpawnDefinition.SpawnType == EGameplayInteractionActorSpawnType::LootContainerSpawn ? 180.0 : 160.0;
		double NumericMaxSpawnRadius = SpawnDefinition.SpawnType == EGameplayInteractionActorSpawnType::LootContainerSpawn ? 440.0 : 420.0;
		double NumericSpawnTraceHeight = 800.0;
		FString LootContainerActorClassPath;
		JsonObject->TryGetNumberField(TEXT("container_definition_id"), NumericContainerDefinitionId);
		JsonObject->TryGetNumberField(TEXT("contents_id"), NumericContentsId);
		JsonObject->TryGetNumberField(TEXT("min_spawn_radius"), NumericMinSpawnRadius);
		JsonObject->TryGetNumberField(TEXT("max_spawn_radius"), NumericMaxSpawnRadius);
		JsonObject->TryGetNumberField(TEXT("spawn_trace_height"), NumericSpawnTraceHeight);
		JsonObject->TryGetStringField(TEXT("loot_container_actor_class"), LootContainerActorClassPath);
		SpawnDefinition.ContainerDefinitionId = static_cast<int32>(NumericContainerDefinitionId);
		SpawnDefinition.ContentsId = static_cast<int32>(NumericContentsId);
		SpawnDefinition.MinSpawnRadius = FMath::Max(0.0f, static_cast<float>(NumericMinSpawnRadius));
		SpawnDefinition.MaxSpawnRadius = FMath::Max(0.0f, static_cast<float>(NumericMaxSpawnRadius));
		SpawnDefinition.SpawnTraceHeight = FMath::Max(0.0f, static_cast<float>(NumericSpawnTraceHeight));
		const FString TrimmedLootContainerActorClassPath = LootContainerActorClassPath.TrimStartAndEnd();
		SpawnDefinition.LootContainerActorClass = TSoftClassPtr<ATunaSweeperLootContainerActor>(
			FSoftObjectPath(TrimmedLootContainerActorClassPath.IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultLootContainerClassPath)
				: TrimmedLootContainerActorClassPath));

		double NumericShopId = 1.0;
		JsonObject->TryGetNumberField(TEXT("shop_id"), NumericShopId);
		SpawnDefinition.ShopId = FMath::Max(1, static_cast<int32>(NumericShopId));

		double NumericWorkbenchId = 1.0;
		JsonObject->TryGetNumberField(TEXT("workbench_id"), NumericWorkbenchId);
		SpawnDefinition.WorkbenchId = FMath::Max(1, static_cast<int32>(NumericWorkbenchId));

		double NumericCurrencyGrantAmount = SpawnDefinition.CurrencyGrantAmount;
		if (!JsonObject->TryGetNumberField(TEXT("currency_grant_amount"), NumericCurrencyGrantAmount))
		{
			JsonObject->TryGetNumberField(TEXT("grant_amount"), NumericCurrencyGrantAmount);
		}
		SpawnDefinition.CurrencyGrantAmount = FMath::Max(1, static_cast<int32>(NumericCurrencyGrantAmount));

		FString SpeechBubbleWidgetClassPath;
		double NumericCountdownStartNumber = 3.0;
		double NumericCountdownStepSeconds = 1.0;
		double NumericBoomDisplaySeconds = 0.2;
		double NumericExplosionRadius = 200.0;
		double NumericExplosionDamage = 100.0;
		JsonObject->TryGetStringField(TEXT("speech_bubble_widget_class"), SpeechBubbleWidgetClassPath);
		JsonObject->TryGetNumberField(TEXT("countdown_start_number"), NumericCountdownStartNumber);
		JsonObject->TryGetNumberField(TEXT("countdown_step_seconds"), NumericCountdownStepSeconds);
		JsonObject->TryGetNumberField(TEXT("boom_display_seconds"), NumericBoomDisplaySeconds);
		JsonObject->TryGetNumberField(TEXT("explosion_radius"), NumericExplosionRadius);
		JsonObject->TryGetNumberField(TEXT("explosion_damage"), NumericExplosionDamage);
		const FString TrimmedSpeechBubbleWidgetClassPath = SpeechBubbleWidgetClassPath.TrimStartAndEnd();
		SpawnDefinition.SpeechBubbleWidgetClass = TSoftClassPtr<UTunaSweeperSpeechBubbleWidget>(
			FSoftObjectPath(TrimmedSpeechBubbleWidgetClassPath.IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultSpeechBubbleWidgetClassPath)
				: TrimmedSpeechBubbleWidgetClassPath));
		SpawnDefinition.CountdownStartNumber = FMath::Max(1, static_cast<int32>(NumericCountdownStartNumber));
		SpawnDefinition.CountdownStepSeconds = FMath::Max(0.01f, static_cast<float>(NumericCountdownStepSeconds));
		SpawnDefinition.BoomDisplaySeconds = FMath::Max(0.0f, static_cast<float>(NumericBoomDisplaySeconds));
		SpawnDefinition.ExplosionRadius = FMath::Max(0.0f, static_cast<float>(NumericExplosionRadius));
		SpawnDefinition.ExplosionDamage = FMath::Max(0.0f, static_cast<float>(NumericExplosionDamage));

		FString RollingBomberClassPath;
		FString RollingBomberLaunchSoundPath;
		double NumericRollingBomberInitialSpawnCount = 2.0;
		double NumericRollingBomberMaxSpawnCount = 8.0;
		double NumericRollingBomberWaveIntervalSeconds = 10.0;
		double NumericRollingBomberSpawnIntervalSeconds = 0.2;
		double NumericRollingBomberLaunchSpeedMin = 850.0;
		double NumericRollingBomberLaunchSpeedMax = 1100.0;
		double NumericRollingBomberLaunchPitchMinDegrees = 38.0;
		double NumericRollingBomberLaunchPitchMaxDegrees = 58.0;
		double NumericRollingBomberSpawnerMaxHealth = 80.0;
		double NumericRollingBomberSpawnerExperienceValue = 120.0;
		JsonObject->TryGetStringField(TEXT("rolling_bomber_class"), RollingBomberClassPath);
		JsonObject->TryGetStringField(TEXT("launch_sound"), RollingBomberLaunchSoundPath);
		JsonObject->TryGetNumberField(TEXT("initial_spawn_count"), NumericRollingBomberInitialSpawnCount);
		JsonObject->TryGetNumberField(TEXT("max_spawn_count"), NumericRollingBomberMaxSpawnCount);
		JsonObject->TryGetNumberField(TEXT("wave_interval_seconds"), NumericRollingBomberWaveIntervalSeconds);
		JsonObject->TryGetNumberField(TEXT("spawn_interval_seconds"), NumericRollingBomberSpawnIntervalSeconds);
		JsonObject->TryGetNumberField(TEXT("launch_speed_min"), NumericRollingBomberLaunchSpeedMin);
		JsonObject->TryGetNumberField(TEXT("launch_speed_max"), NumericRollingBomberLaunchSpeedMax);
		JsonObject->TryGetNumberField(TEXT("launch_pitch_min_degrees"), NumericRollingBomberLaunchPitchMinDegrees);
		JsonObject->TryGetNumberField(TEXT("launch_pitch_max_degrees"), NumericRollingBomberLaunchPitchMaxDegrees);
		JsonObject->TryGetNumberField(TEXT("spawner_max_health"), NumericRollingBomberSpawnerMaxHealth);
		JsonObject->TryGetNumberField(TEXT("experience_value"), NumericRollingBomberSpawnerExperienceValue);
		const FString TrimmedRollingBomberClassPath = RollingBomberClassPath.TrimStartAndEnd();
		const FString TrimmedRollingBomberLaunchSoundPath = RollingBomberLaunchSoundPath.TrimStartAndEnd();
		SpawnDefinition.RollingBomberClass = TSoftClassPtr<ATunaSweeperRollingBomber>(
			FSoftObjectPath(TrimmedRollingBomberClassPath.IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultRollingBomberClassPath)
				: TrimmedRollingBomberClassPath));
		SpawnDefinition.RollingBomberLaunchSound = TSoftObjectPtr<USoundBase>(
			FSoftObjectPath(TrimmedRollingBomberLaunchSoundPath.IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultRollingBomberLaunchSoundPath)
				: TrimmedRollingBomberLaunchSoundPath));
		SpawnDefinition.RollingBomberInitialSpawnCount = FMath::Max(1, static_cast<int32>(NumericRollingBomberInitialSpawnCount));
		SpawnDefinition.RollingBomberMaxSpawnCount = FMath::Max(
			SpawnDefinition.RollingBomberInitialSpawnCount,
			static_cast<int32>(NumericRollingBomberMaxSpawnCount));
		SpawnDefinition.RollingBomberWaveIntervalSeconds = FMath::Max(0.01f, static_cast<float>(NumericRollingBomberWaveIntervalSeconds));
		SpawnDefinition.RollingBomberSpawnIntervalSeconds = FMath::Max(0.01f, static_cast<float>(NumericRollingBomberSpawnIntervalSeconds));
		SpawnDefinition.RollingBomberLaunchSpeedMin = FMath::Max(0.0f, static_cast<float>(NumericRollingBomberLaunchSpeedMin));
		SpawnDefinition.RollingBomberLaunchSpeedMax = FMath::Max(
			SpawnDefinition.RollingBomberLaunchSpeedMin,
			static_cast<float>(NumericRollingBomberLaunchSpeedMax));
		SpawnDefinition.RollingBomberLaunchPitchMinDegrees = FMath::Clamp(
			static_cast<float>(NumericRollingBomberLaunchPitchMinDegrees),
			0.0f,
			89.0f);
		SpawnDefinition.RollingBomberLaunchPitchMaxDegrees = FMath::Clamp(
			static_cast<float>(NumericRollingBomberLaunchPitchMaxDegrees),
			SpawnDefinition.RollingBomberLaunchPitchMinDegrees,
			89.0f);
		SpawnDefinition.RollingBomberSpawnerMaxHealth = FMath::Max(1.0f, static_cast<float>(NumericRollingBomberSpawnerMaxHealth));
		SpawnDefinition.RollingBomberSpawnerExperienceValue = FMath::Max(
			0,
			static_cast<int32>(NumericRollingBomberSpawnerExperienceValue));

		FVector SandbagCoverBoxExtent = SpawnDefinition.SandbagCoverBoxExtent;
		if (!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("sandbag_box_extent"), SandbagCoverBoxExtent))
		{
			TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("box_extent"), SandbagCoverBoxExtent);
		}
		double NumericSandbagCoverMaxHealth = SpawnDefinition.SandbagCoverMaxHealth;
		double NumericSandbagCoverPassthroughRadius = SpawnDefinition.SandbagCoverPassthroughRadius;
		JsonObject->TryGetNumberField(TEXT("cover_max_health"), NumericSandbagCoverMaxHealth);
		JsonObject->TryGetNumberField(TEXT("max_health"), NumericSandbagCoverMaxHealth);
		JsonObject->TryGetNumberField(TEXT("passthrough_radius"), NumericSandbagCoverPassthroughRadius);
		SpawnDefinition.SandbagCoverBoxExtent = FVector(
			FMath::Max(1.0f, SandbagCoverBoxExtent.X),
			FMath::Max(1.0f, SandbagCoverBoxExtent.Y),
			FMath::Max(1.0f, SandbagCoverBoxExtent.Z));
		SpawnDefinition.SandbagCoverMaxHealth = FMath::Max(1.0f, static_cast<float>(NumericSandbagCoverMaxHealth));
		SpawnDefinition.SandbagCoverPassthroughRadius = FMath::Max(
			0.0f,
			static_cast<float>(NumericSandbagCoverPassthroughRadius));

		FString ExplosiveBarrelIntactMeshPath;
		FString ExplosiveBarrelDestroyedMeshPath;
		FString ExplosiveBarrelDestroyedLoopEffectPath;
		FString ExplosiveBarrelExplosionEffectClassPath;
		double NumericExplosiveBarrelMaxHealth = SpawnDefinition.ExplosiveBarrelMaxHealth;
		double NumericExplosiveBarrelExplosionVisualRadius = SpawnDefinition.ExplosiveBarrelExplosionVisualRadius;
		double NumericExplosiveBarrelExplosionDurationSeconds = SpawnDefinition.ExplosiveBarrelExplosionDurationSeconds;
		JsonObject->TryGetNumberField(TEXT("barrel_max_health"), NumericExplosiveBarrelMaxHealth);
		JsonObject->TryGetNumberField(TEXT("max_health"), NumericExplosiveBarrelMaxHealth);
		JsonObject->TryGetStringField(TEXT("intact_mesh"), ExplosiveBarrelIntactMeshPath);
		JsonObject->TryGetStringField(TEXT("destroyed_mesh"), ExplosiveBarrelDestroyedMeshPath);
		JsonObject->TryGetStringField(TEXT("destroyed_loop_effect"), ExplosiveBarrelDestroyedLoopEffectPath);
		JsonObject->TryGetStringField(TEXT("explosion_effect_actor_class"), ExplosiveBarrelExplosionEffectClassPath);
		JsonObject->TryGetNumberField(TEXT("explosion_visual_radius"), NumericExplosiveBarrelExplosionVisualRadius);
		JsonObject->TryGetNumberField(TEXT("explosion_duration_seconds"), NumericExplosiveBarrelExplosionDurationSeconds);
		const FString TrimmedExplosiveBarrelIntactMeshPath = ExplosiveBarrelIntactMeshPath.TrimStartAndEnd();
		const FString TrimmedExplosiveBarrelDestroyedMeshPath = ExplosiveBarrelDestroyedMeshPath.TrimStartAndEnd();
		const FString TrimmedExplosiveBarrelDestroyedLoopEffectPath = ExplosiveBarrelDestroyedLoopEffectPath.TrimStartAndEnd();
		const FString TrimmedExplosiveBarrelExplosionEffectClassPath = ExplosiveBarrelExplosionEffectClassPath.TrimStartAndEnd();
		SpawnDefinition.ExplosiveBarrelMaxHealth = FMath::Max(1.0f, static_cast<float>(NumericExplosiveBarrelMaxHealth));
		SpawnDefinition.ExplosiveBarrelIntactMesh = TrimmedExplosiveBarrelIntactMeshPath.IsEmpty()
			? TSoftObjectPtr<UStaticMesh>()
			: TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TrimmedExplosiveBarrelIntactMeshPath));
		SpawnDefinition.ExplosiveBarrelDestroyedMesh = TrimmedExplosiveBarrelDestroyedMeshPath.IsEmpty()
			? TSoftObjectPtr<UStaticMesh>()
			: TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TrimmedExplosiveBarrelDestroyedMeshPath));
		if (!TrimmedExplosiveBarrelDestroyedLoopEffectPath.IsEmpty())
		{
			SpawnDefinition.ExplosiveBarrelDestroyedLoopEffect = TSoftObjectPtr<UNiagaraSystem>(
				FSoftObjectPath(TrimmedExplosiveBarrelDestroyedLoopEffectPath));
		}
		SpawnDefinition.ExplosiveBarrelExplosionEffectClass = TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor>(
			FSoftObjectPath(TrimmedExplosiveBarrelExplosionEffectClassPath.IsEmpty()
				? FString(TunaSweeperEnemySpawn::DefaultExplosionEffectActorClassPath)
				: TrimmedExplosiveBarrelExplosionEffectClassPath));
		SpawnDefinition.ExplosiveBarrelExplosionVisualRadius = FMath::Max(
			1.0f,
			static_cast<float>(NumericExplosiveBarrelExplosionVisualRadius));
		SpawnDefinition.ExplosiveBarrelExplosionDurationSeconds = FMath::Max(
			0.05f,
			static_cast<float>(NumericExplosiveBarrelExplosionDurationSeconds));

		const FString StaticMeshPropMeshPath = TunaSweeperEnemySpawn::ReadFirstStringField(
			JsonObject,
			TEXT("static_mesh"),
			TEXT("mesh"),
			TEXT("static_mesh_prop_mesh"));
		if (!StaticMeshPropMeshPath.IsEmpty())
		{
			SpawnDefinition.StaticMeshPropMesh = TSoftObjectPtr<UStaticMesh>(
				FSoftObjectPath(StaticMeshPropMeshPath));
		}

		const TArray<TSharedPtr<FJsonValue>>* StaticMeshPropMaterialValues = nullptr;
		if (JsonObject->TryGetArrayField(TEXT("static_mesh_materials"), StaticMeshPropMaterialValues) ||
			JsonObject->TryGetArrayField(TEXT("materials"), StaticMeshPropMaterialValues))
		{
			for (const TSharedPtr<FJsonValue>& MaterialValue : *StaticMeshPropMaterialValues)
			{
				if (!MaterialValue.IsValid())
				{
					continue;
				}

				const FString MaterialPath = MaterialValue->AsString().TrimStartAndEnd();
				if (!MaterialPath.IsEmpty())
				{
					SpawnDefinition.StaticMeshPropMaterials.Add(
						TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(MaterialPath)));
				}
			}
		}
		else
		{
			const FString StaticMeshPropMaterialPath = TunaSweeperEnemySpawn::ReadFirstStringField(
				JsonObject,
				TEXT("static_mesh_material"),
				TEXT("material"));
			if (!StaticMeshPropMaterialPath.IsEmpty())
			{
				SpawnDefinition.StaticMeshPropMaterials.Add(
					TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(StaticMeshPropMaterialPath)));
			}
		}

		FVector StaticMeshPropRelativeLocation = SpawnDefinition.StaticMeshPropRelativeLocation;
		if (!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("static_mesh_relative_location"), StaticMeshPropRelativeLocation))
		{
			TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("mesh_relative_location"), StaticMeshPropRelativeLocation);
		}
		FRotator StaticMeshPropRelativeRotation = SpawnDefinition.StaticMeshPropRelativeRotation;
		if (!TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("static_mesh_relative_rotation"), StaticMeshPropRelativeRotation))
		{
			TunaSweeperEnemySpawn::TryReadRotatorField(JsonObject, TEXT("mesh_relative_rotation"), StaticMeshPropRelativeRotation);
		}
		FVector StaticMeshPropRelativeScale = SpawnDefinition.StaticMeshPropRelativeScale;
		if (!TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("static_mesh_relative_scale"), StaticMeshPropRelativeScale))
		{
			TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("mesh_relative_scale"), StaticMeshPropRelativeScale);
		}
		JsonObject->TryGetBoolField(TEXT("static_mesh_collision_enabled"), SpawnDefinition.bStaticMeshPropCollisionEnabled);
		JsonObject->TryGetBoolField(TEXT("collision_enabled"), SpawnDefinition.bStaticMeshPropCollisionEnabled);
		SpawnDefinition.StaticMeshPropRelativeLocation = StaticMeshPropRelativeLocation;
		SpawnDefinition.StaticMeshPropRelativeRotation = StaticMeshPropRelativeRotation;
		SpawnDefinition.StaticMeshPropRelativeScale = FVector(
			FMath::Max(0.01f, StaticMeshPropRelativeScale.X),
			FMath::Max(0.01f, StaticMeshPropRelativeScale.Y),
			FMath::Max(0.01f, StaticMeshPropRelativeScale.Z));

		double NumericPracticeDummyMaxHealth = SpawnDefinition.PracticeDummyMaxHealth;
		double NumericPracticeDummyCriticalMultiplier =
			TunaSweeperDataValues::RatioIdentity * SpawnDefinition.PracticeDummyCriticalDamageMultiplier;
		double NumericPracticeDummyHeadshotMultiplier =
			TunaSweeperDataValues::RatioIdentity * SpawnDefinition.PracticeDummyHeadshotDamageMultiplier;
		double NumericPracticeDummyHealthRecoverySeconds = SpawnDefinition.PracticeDummyHealthRecoverySeconds;
		if (!JsonObject->TryGetNumberField(TEXT("practice_dummy_max_health"), NumericPracticeDummyMaxHealth))
		{
			if (!JsonObject->TryGetNumberField(TEXT("dummy_max_health"), NumericPracticeDummyMaxHealth))
			{
				JsonObject->TryGetNumberField(TEXT("max_health"), NumericPracticeDummyMaxHealth);
			}
		}
		JsonObject->TryGetNumberField(TEXT("critical_damage_multiplier"), NumericPracticeDummyCriticalMultiplier);
		JsonObject->TryGetNumberField(TEXT("headshot_damage_multiplier"), NumericPracticeDummyHeadshotMultiplier);
		JsonObject->TryGetNumberField(TEXT("health_recovery_seconds"), NumericPracticeDummyHealthRecoverySeconds);
		JsonObject->TryGetNumberField(TEXT("recovery_seconds"), NumericPracticeDummyHealthRecoverySeconds);
		SpawnDefinition.PracticeDummyMaxHealth = FMath::Max(1.0f, static_cast<float>(NumericPracticeDummyMaxHealth));
		SpawnDefinition.PracticeDummyCriticalDamageMultiplier =
			FMath::Max(
				1.0f,
				TunaSweeperDataValues::ToRatioFloat(TunaSweeperDataValues::ClampRatioValue(
					FMath::RoundToInt(NumericPracticeDummyCriticalMultiplier))));
		SpawnDefinition.PracticeDummyHeadshotDamageMultiplier = FMath::Max(
			SpawnDefinition.PracticeDummyCriticalDamageMultiplier,
			TunaSweeperDataValues::ToRatioFloat(TunaSweeperDataValues::ClampRatioValue(
				FMath::RoundToInt(NumericPracticeDummyHeadshotMultiplier))));
		SpawnDefinition.PracticeDummyHealthRecoverySeconds =
			FMath::Max(0.05f, static_cast<float>(NumericPracticeDummyHealthRecoverySeconds));

		FString NoiseEmitterMeshDefinitionId;
		FString NoiseEmitterMeshDefinitionJsonPath;
		FString NoiseEmitterTag;
		double NumericNoiseEmitterIntervalSeconds = SpawnDefinition.NoiseEmitterIntervalSeconds;
		double NumericNoiseEmitterLoudness = SpawnDefinition.NoiseEmitterLoudness;
		double NumericNoiseEmitterMaxRange = SpawnDefinition.NoiseEmitterMaxRange;
		bool bNoiseEmitterStartEnabled = SpawnDefinition.bNoiseEmitterStartEnabled;
		FVector NoiseEmitterSourceLocalOffset = SpawnDefinition.NoiseEmitterSourceLocalOffset;
		JsonObject->TryGetStringField(TEXT("mesh_definition_id"), NoiseEmitterMeshDefinitionId);
		JsonObject->TryGetStringField(TEXT("noise_emitter_mesh_definition_id"), NoiseEmitterMeshDefinitionId);
		JsonObject->TryGetStringField(TEXT("mesh_definition_json"), NoiseEmitterMeshDefinitionJsonPath);
		JsonObject->TryGetStringField(TEXT("mesh_definition_json_path"), NoiseEmitterMeshDefinitionJsonPath);
		JsonObject->TryGetNumberField(TEXT("noise_interval_seconds"), NumericNoiseEmitterIntervalSeconds);
		JsonObject->TryGetNumberField(TEXT("noise_loudness"), NumericNoiseEmitterLoudness);
		JsonObject->TryGetNumberField(TEXT("noise_max_range"), NumericNoiseEmitterMaxRange);
		JsonObject->TryGetStringField(TEXT("noise_tag"), NoiseEmitterTag);
		JsonObject->TryGetBoolField(TEXT("noise_emitter_start_enabled"), bNoiseEmitterStartEnabled);
		JsonObject->TryGetBoolField(TEXT("emit_on_begin_play"), bNoiseEmitterStartEnabled);
		TunaSweeperEnemySpawn::TryReadVectorField(JsonObject, TEXT("noise_source_local_offset"), NoiseEmitterSourceLocalOffset);
		if (!NoiseEmitterMeshDefinitionId.TrimStartAndEnd().IsEmpty())
		{
			SpawnDefinition.NoiseEmitterMeshDefinitionId = FName(*NoiseEmitterMeshDefinitionId.TrimStartAndEnd());
		}
		if (!NoiseEmitterMeshDefinitionJsonPath.TrimStartAndEnd().IsEmpty())
		{
			SpawnDefinition.NoiseEmitterMeshDefinitionJsonRelativePath =
				NoiseEmitterMeshDefinitionJsonPath.TrimStartAndEnd();
		}
		SpawnDefinition.NoiseEmitterIntervalSeconds =
			FMath::Max(0.05f, static_cast<float>(NumericNoiseEmitterIntervalSeconds));
		SpawnDefinition.NoiseEmitterLoudness = FMath::Max(0.0f, static_cast<float>(NumericNoiseEmitterLoudness));
		SpawnDefinition.NoiseEmitterMaxRange = FMath::Max(0.0f, static_cast<float>(NumericNoiseEmitterMaxRange));
		if (!NoiseEmitterTag.TrimStartAndEnd().IsEmpty())
		{
			SpawnDefinition.NoiseEmitterTag = FName(*NoiseEmitterTag.TrimStartAndEnd());
		}
		SpawnDefinition.NoiseEmitterSourceLocalOffset = NoiseEmitterSourceLocalOffset;
		SpawnDefinition.bNoiseEmitterStartEnabled = bNoiseEmitterStartEnabled;

		if (SpawnDefinition.SpawnType == EGameplayInteractionActorSpawnType::LevelTravel)
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping gameplay interaction actor spawn row %d: level travel actors are placed directly in maps."), RowIndex);
			continue;
		}
		if (SpawnDefinition.SpawnType == EGameplayInteractionActorSpawnType::ExtractionPoint &&
			SpawnDefinition.TargetLevelName.IsNone())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping gameplay interaction actor spawn row %d: extraction target is missing."), RowIndex);
			continue;
		}
		if (SpawnDefinition.SpawnType == EGameplayInteractionActorSpawnType::LootContainer &&
			(SpawnDefinition.ContainerDefinitionId == INDEX_NONE || SpawnDefinition.ContentsId == INDEX_NONE))
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping gameplay interaction actor spawn row %d: loot container identifiers are missing."), RowIndex);
			continue;
		}
		if (SpawnDefinition.SpawnType == EGameplayInteractionActorSpawnType::StaticMeshProp &&
			SpawnDefinition.StaticMeshPropMesh.IsNull())
		{
			UE_LOG(LogTunaSweeperEnemySpawn, Warning, TEXT("Skipping gameplay interaction actor spawn row %d: static mesh prop mesh is missing."), RowIndex);
			continue;
		}

		GameplayInteractionActorSpawnDefinitions.Add(SpawnDefinition);
		bHasValidRows = true;
	}

	if (!bHasValidRows && JsonRows.Num() > 0)
	{
		UE_LOG(LogTunaSweeperEnemySpawn, Error, TEXT("Gameplay interaction actor spawn JSON has no valid rows: %s"), *GameplayInteractionActorSpawnJsonPath);
		return false;
	}

	bGameplayInteractionActorSpawnDataLoaded = true;
	return true;
}

bool UTunaSweeperEnemySpawnSubsystem::GetMapOverlaysForWorld(
	const UWorld* World,
	TArray<FTunaSweeperMapOverlayDefinition>& OutMapOverlays)
{
	OutMapOverlays.Reset();

	if (!World || !LoadGameplayInteractionActorSpawnData(false))
	{
		return false;
	}

	for (const FGameplayInteractionActorSpawnDefinition& SpawnDefinition : GameplayInteractionActorSpawnDefinitions)
	{
		if (!SpawnDefinition.bHasMapOverlay || !DoesLevelNameMatchWorld(SpawnDefinition.LevelName, World))
		{
			continue;
		}

		OutMapOverlays.Add(SpawnDefinition.MapOverlay);
	}

	return OutMapOverlays.Num() > 0;
}

void UTunaSweeperEnemySpawnSubsystem::HandlePostLoadMapWithWorld(UWorld* LoadedWorld)
{
	EnsureRaidRuntimeActorsSpawnedForWorld(LoadedWorld);
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedEnemySpawnData()
{
	EnemySpawnDefinitions.Reset();
	bEnemySpawnDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedEnemyCombatProfileData()
{
	EnemyCombatProfilesById.Reset();
	bEnemyCombatProfileDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedLootContainerSpawnData()
{
	LootContainerSpawnDefinitions.Reset();
	bLootContainerSpawnDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedTransparentObstacleSpawnData()
{
	TransparentObstacleSpawnDefinitions.Reset();
	bTransparentObstacleSpawnDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedWorldProgressObjectSpawnData()
{
	WorldProgressObjectSpawnDefinitions.Reset();
	bWorldProgressObjectSpawnDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedWarpPointSpawnData()
{
	WarpPointSpawnDefinitions.Reset();
	bWarpPointSpawnDataLoaded = false;
}

void UTunaSweeperEnemySpawnSubsystem::ResetLoadedGameplayInteractionActorSpawnData()
{
	GameplayInteractionActorSpawnDefinitions.Reset();
	bGameplayInteractionActorSpawnDataLoaded = false;
}

FString UTunaSweeperEnemySpawnSubsystem::GetEnemySpawnJsonPath() const
{
	return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("EnemySpawns.json"));
}

FString UTunaSweeperEnemySpawnSubsystem::GetEnemyCombatProfileJsonPath() const
{
	return FPaths::Combine(FPaths::ProjectContentDir(), TunaSweeperEnemySpawn::EnemyCombatProfilesJsonRelativePath);
}

FString UTunaSweeperEnemySpawnSubsystem::GetLootContainerSpawnJsonPath() const
{
	return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("LootContainerSpawns.json"));
}

FString UTunaSweeperEnemySpawnSubsystem::GetTransparentObstacleSpawnJsonPath() const
{
	return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("TransparentObstacleSpawns.json"));
}

FString UTunaSweeperEnemySpawnSubsystem::GetWorldProgressObjectSpawnJsonPath() const
{
	return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("WorldProgressObjectSpawns.json"));
}

FString UTunaSweeperEnemySpawnSubsystem::GetWarpPointSpawnJsonPath() const
{
	return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("WarpPointSpawns.json"));
}

FString UTunaSweeperEnemySpawnSubsystem::GetGameplayInteractionActorSpawnJsonPath() const
{
	return TunaSweeperBuildFlavor::GetRuntimePlacementDataPath(TEXT("GameplayInteractionSpawns.json"));
}

bool UTunaSweeperEnemySpawnSubsystem::DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const
{
	if (!World || LevelName.IsNone())
	{
		return false;
	}

	const FString SpawnLevelName = TunaSweeperEnemySpawn::NormalizeLevelName(
		TunaSweeperBuildFlavor::ResolveGameplayLevelName(LevelName).ToString());
	const FString WorldMapName = TunaSweeperEnemySpawn::NormalizeLevelName(World->GetMapName());
	const FString WorldPackageName = TunaSweeperEnemySpawn::NormalizeLevelName(World->GetOutermost()->GetName());
	return SpawnLevelName.Equals(WorldMapName, ESearchCase::IgnoreCase) ||
		SpawnLevelName.Equals(WorldPackageName, ESearchCase::IgnoreCase);
}

void UTunaSweeperEnemySpawnSubsystem::ConfigureGameplayInteractionActor(
	AActor* SpawnedActor,
	const FGameplayInteractionActorSpawnDefinition& SpawnDefinition) const
{
	if (!SpawnedActor)
	{
		return;
	}

	switch (SpawnDefinition.SpawnType)
	{
	case EGameplayInteractionActorSpawnType::ExtractionPoint:
		if (ATunaSweeperExtractionPointActor* ExtractionPointActor = Cast<ATunaSweeperExtractionPointActor>(SpawnedActor))
		{
			ExtractionPointActor->ConfigureExtractionPointDefaults(
				TunaSweeperBuildFlavor::ResolveGameplayLevelName(SpawnDefinition.TargetLevelName),
				SpawnDefinition.ExtractionRadius,
				SpawnDefinition.ExtractionHoldSeconds,
				SpawnDefinition.ExtractionRadiusRingWidth,
				SpawnDefinition.ExtractionParticleSystem,
				SpawnDefinition.ExtractionRadiusVisualMaterial,
				SpawnDefinition.TransitionMediaSource,
				SpawnDefinition.TransitionWidgetClass,
				SpawnDefinition.TransitionMessage,
				SpawnDefinition.TransitionMessageStringKey);
		}
		break;
	case EGameplayInteractionActorSpawnType::PickupItem:
		if (ATunaSweeperPickupItemActor* PickupItemActor = Cast<ATunaSweeperPickupItemActor>(SpawnedActor))
		{
			PickupItemActor->ConfigurePickupItemDefaults(
				SpawnDefinition.ItemId,
				SpawnDefinition.ItemQuantity,
				SpawnDefinition.bDestroyOnPickup,
				SpawnDefinition.PickupItemIconWidgetClass);
		}
		break;
	case EGameplayInteractionActorSpawnType::ItemSpawn:
		if (ATunaSweeperItemSpawnInteractableActor* ItemSpawnActor = Cast<ATunaSweeperItemSpawnInteractableActor>(SpawnedActor))
		{
			ItemSpawnActor->ConfigureItemSpawnDefaults(
				SpawnDefinition.PickupItemActorClass,
				SpawnDefinition.MinSpawnRadius,
				SpawnDefinition.MaxSpawnRadius,
				SpawnDefinition.SpawnTraceHeight);
			if (UTunaSweeperInteractableComponent* InteractableComponent = ItemSpawnActor->GetInteractableComponent())
			{
				InteractableComponent->ConfigureInteractionDefaults(
					ETunaSweeperInteractionType::ItemSpawn,
					SpawnDefinition.InteractionDisplayName,
					SpawnDefinition.MarkerWidgetClass,
					SpawnDefinition.InteractionDisplayNameStringKey);
			}
		}
		break;
	case EGameplayInteractionActorSpawnType::LootContainer:
		if (ATunaSweeperLootContainerActor* LootContainerActor = Cast<ATunaSweeperLootContainerActor>(SpawnedActor))
		{
			LootContainerActor->ConfigureLootContainerDefaults(
				SpawnDefinition.ContainerDefinitionId,
				SpawnDefinition.ContentsId);
		}
		break;
	case EGameplayInteractionActorSpawnType::LootContainerSpawn:
		if (ATunaSweeperLootContainerSpawnInteractableActor* LootContainerSpawnActor = Cast<ATunaSweeperLootContainerSpawnInteractableActor>(SpawnedActor))
		{
			LootContainerSpawnActor->ConfigureLootContainerSpawnDefaults(
				SpawnDefinition.LootContainerActorClass,
				SpawnDefinition.MinSpawnRadius,
				SpawnDefinition.MaxSpawnRadius,
				SpawnDefinition.SpawnTraceHeight);
			if (UTunaSweeperInteractableComponent* InteractableComponent = LootContainerSpawnActor->GetInteractableComponent())
			{
				InteractableComponent->ConfigureInteractionDefaults(
					ETunaSweeperInteractionType::LootContainerSpawn,
					SpawnDefinition.InteractionDisplayName,
					SpawnDefinition.MarkerWidgetClass,
					SpawnDefinition.InteractionDisplayNameStringKey);
			}
		}
		break;
	case EGameplayInteractionActorSpawnType::Shop:
		if (ATunaSweeperShopActor* ShopActor = Cast<ATunaSweeperShopActor>(SpawnedActor))
		{
			ShopActor->ConfigureShopDefaults(SpawnDefinition.ShopId);
			if (UTunaSweeperInteractableComponent* InteractableComponent = ShopActor->GetInteractableComponent())
			{
				InteractableComponent->ConfigureInteractionDefaults(
					ETunaSweeperInteractionType::ShopOpen,
					SpawnDefinition.InteractionDisplayName,
					SpawnDefinition.MarkerWidgetClass,
					SpawnDefinition.InteractionDisplayNameStringKey.IsNone()
						? FName(TEXT("ui.interaction.shop_open"))
						: SpawnDefinition.InteractionDisplayNameStringKey);
			}
		}
		break;
	case EGameplayInteractionActorSpawnType::Workbench:
		if (ATunaSweeperWorkbenchActor* WorkbenchActor = Cast<ATunaSweeperWorkbenchActor>(SpawnedActor))
		{
			WorkbenchActor->ConfigureWorkbenchDefaults(SpawnDefinition.WorkbenchId);
			if (UTunaSweeperInteractableComponent* InteractableComponent = WorkbenchActor->GetInteractableComponent())
			{
				InteractableComponent->ConfigureInteractionDefaults(
					ETunaSweeperInteractionType::WorkbenchCraft,
					FText::FromString(TEXT("\uC81C\uC870")),
					SpawnDefinition.MarkerWidgetClass,
					FName(TEXT("ui.interaction.workbench_craft")));
			}
			if (UTunaSweeperInteractableComponent* DismantleComponent = WorkbenchActor->GetDismantleInteractableComponent())
			{
				DismantleComponent->ConfigureInteractionDefaults(
					ETunaSweeperInteractionType::WorkbenchDismantle,
					FText::FromString(TEXT("\uBD84\uD574")),
					SpawnDefinition.MarkerWidgetClass,
					FName(TEXT("ui.interaction.workbench_dismantle")));
			}
			if (UTunaSweeperInteractableComponent* BlueprintRegisterComponent = WorkbenchActor->GetBlueprintRegisterInteractableComponent())
			{
				BlueprintRegisterComponent->ConfigureInteractionDefaults(
					ETunaSweeperInteractionType::WorkbenchBlueprintRegister,
					FText::FromString(TEXT("\uC124\uACC4\uB3C4 \uB4F1\uB85D")),
					SpawnDefinition.MarkerWidgetClass,
					FName(TEXT("ui.interaction.workbench_blueprint_register")));
			}
		}
		break;
	case EGameplayInteractionActorSpawnType::PiggyBank:
		if (ATunaSweeperPiggyBankActor* PiggyBankActor = Cast<ATunaSweeperPiggyBankActor>(SpawnedActor))
		{
			PiggyBankActor->ConfigurePiggyBankDefaults(
				SpawnDefinition.CurrencyGrantAmount,
				SpawnDefinition.InteractionDisplayName,
				SpawnDefinition.MarkerWidgetClass,
				SpawnDefinition.SpawnId);
		}
		break;
	case EGameplayInteractionActorSpawnType::PeriodicNoiseEmitter:
		if (ATunaSweeperPeriodicNoiseEmitterActor* NoiseEmitterActor = Cast<ATunaSweeperPeriodicNoiseEmitterActor>(SpawnedActor))
		{
			NoiseEmitterActor->ConfigureNoiseEmitterDefaults(
				SpawnDefinition.NoiseEmitterMeshDefinitionId,
				SpawnDefinition.NoiseEmitterMeshDefinitionJsonRelativePath,
				SpawnDefinition.NoiseEmitterIntervalSeconds,
				SpawnDefinition.NoiseEmitterLoudness,
				SpawnDefinition.NoiseEmitterMaxRange,
				SpawnDefinition.NoiseEmitterTag,
				SpawnDefinition.NoiseEmitterSourceLocalOffset,
				SpawnDefinition.bNoiseEmitterStartEnabled);
		}
		break;
	case EGameplayInteractionActorSpawnType::DifficultyAdjustment:
		if (ATunaSweeperDifficultyAdjustmentActor* DifficultyAdjustmentActor = Cast<ATunaSweeperDifficultyAdjustmentActor>(SpawnedActor))
		{
			DifficultyAdjustmentActor->ConfigureDifficultyAdjustmentDefaults(
				SpawnDefinition.InteractionDisplayName,
				SpawnDefinition.MarkerWidgetClass,
				SpawnDefinition.InteractionDisplayNameStringKey.IsNone()
					? FName(TEXT("ui.interaction.difficulty_adjustment"))
					: SpawnDefinition.InteractionDisplayNameStringKey);
		}
		break;
	case EGameplayInteractionActorSpawnType::SelfDestruct:
		if (ATunaSweeperSelfDestructInteractableActor* SelfDestructActor = Cast<ATunaSweeperSelfDestructInteractableActor>(SpawnedActor))
		{
			SelfDestructActor->ConfigureSelfDestructDefaults(
				SpawnDefinition.MarkerWidgetClass,
				SpawnDefinition.SpeechBubbleWidgetClass,
				SpawnDefinition.CountdownStartNumber,
				SpawnDefinition.CountdownStepSeconds,
				SpawnDefinition.BoomDisplaySeconds,
				SpawnDefinition.ExplosionRadius,
				SpawnDefinition.ExplosionDamage);
			if (UTunaSweeperInteractableComponent* InteractableComponent = SelfDestructActor->GetInteractableComponent())
			{
				InteractableComponent->ConfigureInteractionDefaults(
					ETunaSweeperInteractionType::SelfDestruct,
					SpawnDefinition.InteractionDisplayName,
					SpawnDefinition.MarkerWidgetClass,
					SpawnDefinition.InteractionDisplayNameStringKey);
			}
		}
		break;
	case EGameplayInteractionActorSpawnType::RollingBomberSpawner:
		if (ATunaSweeperRollingBomberSpawner* RollingBomberSpawner = Cast<ATunaSweeperRollingBomberSpawner>(SpawnedActor))
		{
			RollingBomberSpawner->ConfigureSpawnerDefaults(
				SpawnDefinition.RollingBomberClass,
				SpawnDefinition.RollingBomberLaunchSound,
				SpawnDefinition.RollingBomberInitialSpawnCount,
				SpawnDefinition.RollingBomberMaxSpawnCount,
				SpawnDefinition.RollingBomberWaveIntervalSeconds,
				SpawnDefinition.RollingBomberSpawnIntervalSeconds,
				SpawnDefinition.RollingBomberLaunchSpeedMin,
				SpawnDefinition.RollingBomberLaunchSpeedMax,
				SpawnDefinition.RollingBomberLaunchPitchMinDegrees,
				SpawnDefinition.RollingBomberLaunchPitchMaxDegrees,
				SpawnDefinition.RollingBomberSpawnerMaxHealth,
				SpawnDefinition.RollingBomberSpawnerExperienceValue);
		}
		break;
	case EGameplayInteractionActorSpawnType::SandbagCover:
		if (ATunaSweeperSandbagCoverActor* SandbagCover = Cast<ATunaSweeperSandbagCoverActor>(SpawnedActor))
		{
			SandbagCover->ConfigureCoverDefaults(
				SpawnDefinition.SpawnId,
				SpawnDefinition.SandbagCoverBoxExtent,
				SpawnDefinition.SandbagCoverMaxHealth,
				SpawnDefinition.SandbagCoverPassthroughRadius);
		}
		break;
	case EGameplayInteractionActorSpawnType::ExplosiveBarrel:
		if (ATunaSweeperExplosiveBarrelActor* ExplosiveBarrel = Cast<ATunaSweeperExplosiveBarrelActor>(SpawnedActor))
		{
			ExplosiveBarrel->ConfigureExplosiveBarrelDefaults(
				SpawnDefinition.SpawnId,
				SpawnDefinition.ExplosiveBarrelMaxHealth,
				SpawnDefinition.ExplosiveBarrelIntactMesh,
				SpawnDefinition.ExplosiveBarrelDestroyedMesh,
				SpawnDefinition.ExplosiveBarrelDestroyedLoopEffect,
				SpawnDefinition.ExplosiveBarrelExplosionEffectClass,
				SpawnDefinition.ExplosiveBarrelExplosionVisualRadius,
				SpawnDefinition.ExplosiveBarrelExplosionDurationSeconds);
		}
		break;
	case EGameplayInteractionActorSpawnType::ShootingPracticeDummy:
		if (ATunaSweeperShootingPracticeDummyActor* PracticeDummy = Cast<ATunaSweeperShootingPracticeDummyActor>(SpawnedActor))
		{
			PracticeDummy->ConfigurePracticeDummyDefaults(
				SpawnDefinition.PracticeDummyMaxHealth,
				SpawnDefinition.PracticeDummyCriticalDamageMultiplier,
				SpawnDefinition.PracticeDummyHeadshotDamageMultiplier,
				SpawnDefinition.PracticeDummyHealthRecoverySeconds);
		}
		break;
	case EGameplayInteractionActorSpawnType::StaticMeshProp:
		{
			UStaticMeshComponent* MeshComponent = nullptr;
			if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(SpawnedActor))
			{
				MeshComponent = StaticMeshActor->GetStaticMeshComponent();
			}
			if (!MeshComponent)
			{
				MeshComponent = SpawnedActor->FindComponentByClass<UStaticMeshComponent>();
			}
			if (MeshComponent)
			{
				MeshComponent->SetMobility(EComponentMobility::Movable);
				if (UStaticMesh* StaticMesh = SpawnDefinition.StaticMeshPropMesh.LoadSynchronous())
				{
					MeshComponent->SetStaticMesh(StaticMesh);
				}
				for (int32 MaterialIndex = 0; MaterialIndex < SpawnDefinition.StaticMeshPropMaterials.Num(); ++MaterialIndex)
				{
					if (UMaterialInterface* Material = SpawnDefinition.StaticMeshPropMaterials[MaterialIndex].LoadSynchronous())
					{
						MeshComponent->SetMaterial(MaterialIndex, Material);
					}
				}
				MeshComponent->SetRelativeLocation(SpawnDefinition.StaticMeshPropRelativeLocation);
				MeshComponent->SetRelativeRotation(SpawnDefinition.StaticMeshPropRelativeRotation);
				MeshComponent->SetRelativeScale3D(SpawnDefinition.StaticMeshPropRelativeScale);
				MeshComponent->SetCollisionEnabled(
					SpawnDefinition.bStaticMeshPropCollisionEnabled
						? ECollisionEnabled::QueryAndPhysics
						: ECollisionEnabled::NoCollision);
			}
		}
		break;
	default:
		break;
	}
}
