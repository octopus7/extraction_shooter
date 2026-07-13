#pragma once

#include "CoreMinimal.h"
#include "AI/TunaSweeperEnemyCombatProfile.h"
#include "Component/TunaSweeperFactionTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperEnemySpawnSubsystem.generated.h"

class AActor;
class ATunaSweeperDifficultyAdjustmentActor;
class ATunaSweeperEnemyCharacter;
class ATunaSweeperExplosiveBarrelActor;
class ATunaSweeperExtractionPointActor;
class ATunaSweeperLocalExplosionEffectActor;
class ATunaSweeperItemSpawnInteractableActor;
class ATunaSweeperLevelTravelInteractableActor;
class ATunaSweeperLootContainerActor;
class ATunaSweeperLootContainerSpawnInteractableActor;
class ATunaSweeperPeriodicNoiseEmitterActor;
class ATunaSweeperPickupItemActor;
class ATunaSweeperRollingBomber;
class ATunaSweeperRollingBomberSpawner;
class ATunaSweeperSandbagCoverActor;
class ATunaSweeperSelfDestructInteractableActor;
class ATunaSweeperShopActor;
class ATunaSweeperTransparentObstacleActor;
class ATunaSweeperWarpPointActor;
class ATunaSweeperWorkbenchActor;
class ATunaSweeperWorldProgressActor;
class UMaterialInterface;
class UMediaSource;
class UNiagaraSystem;
class USoundBase;
class UWorld;
class UStaticMesh;
class UTunaSweeperRevealOccluderComponent;
class UTunaSweeperInteractionMarkerWidget;
class UTunaSweeperLevelTransitionWidget;
class UTunaSweeperPickupItemIconWidget;
class UTunaSweeperSpeechBubbleWidget;

struct TUNASWEEPER_API FTunaSweeperMapOverlayDefinition
{
	FName LevelName = NAME_None;
	FName SpawnId = NAME_None;
	FVector WorldLocation = FVector::ZeroVector;
	FName TextStringKey = NAME_None;
	FName IconId = NAME_None;
	FVector2D TextOffset = FVector2D::ZeroVector;
	FVector2D IconOffset = FVector2D::ZeroVector;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperEnemySpawnSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Enemy Spawn")
	bool EnsureEnemiesSpawnedForWorld(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool EnsureRaidRuntimeActorsSpawnedForWorld(UWorld* World);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Enemy Spawn")
	bool LoadEnemySpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Enemy Spawn")
	bool LoadEnemyCombatProfileData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Enemy Spawn")
	bool TryGetEnemyCombatProfile(FName ProfileId, FTunaSweeperEnemyCombatProfile& OutProfile);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool LoadLootContainerSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool LoadTransparentObstacleSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool LoadWorldProgressObjectSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool LoadWarpPointSpawnData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Raid Runtime Spawn")
	bool LoadGameplayInteractionActorSpawnData(bool bForceReload = false);

	bool GetMapOverlaysForWorld(const UWorld* World, TArray<FTunaSweeperMapOverlayDefinition>& OutMapOverlays);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Enemy Spawn")
	bool IsEnemySpawnDataLoaded() const { return bEnemySpawnDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Enemy Spawn")
	bool IsEnemyCombatProfileDataLoaded() const { return bEnemyCombatProfileDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Runtime Spawn")
	bool IsLootContainerSpawnDataLoaded() const { return bLootContainerSpawnDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Runtime Spawn")
	bool IsTransparentObstacleSpawnDataLoaded() const { return bTransparentObstacleSpawnDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Runtime Spawn")
	bool IsWorldProgressObjectSpawnDataLoaded() const { return bWorldProgressObjectSpawnDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Runtime Spawn")
	bool IsWarpPointSpawnDataLoaded() const { return bWarpPointSpawnDataLoaded; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Runtime Spawn")
	bool IsGameplayInteractionActorSpawnDataLoaded() const { return bGameplayInteractionActorSpawnDataLoaded; }

public:
	enum class EGameplayInteractionActorSpawnType : uint8
	{
		Unknown,
		LevelTravel,
		PickupItem,
		ItemSpawn,
		LootContainer,
		LootContainerSpawn,
		SelfDestruct,
		RollingBomberSpawner,
		ExtractionPoint,
		SandbagCover,
		ExplosiveBarrel,
		StaticMeshProp,
		ShootingPracticeDummy,
		Shop,
		Workbench,
		PiggyBank,
		PeriodicNoiseEmitter,
		DifficultyAdjustment
	};

private:
	struct FEnemySpawnDefinition
	{
		FName LevelName;
		FName EnemyId;
		TSoftClassPtr<ATunaSweeperEnemyCharacter> EnemyClass;
		TSoftObjectPtr<UMaterialInterface> BodyMaterial;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		int32 DropContainerDefinitionId = INDEX_NONE;
		int32 DropContentsId = INDEX_NONE;
		int32 WeaponItemId = INDEX_NONE;
		int32 AmmoItemId = INDEX_NONE;
		int32 ReserveAmmoCount = INDEX_NONE;
		float LootLoadedAmmoDeductionRatio = 0.35f;
		int32 LootLoadedAmmoFlatDeduction = 0;
		int32 ExperienceValue = 30;
		float MaxHealth = 30.0f;
		int32 BleedingChanceBonus = 0;
		float BleedingDurationBonusSeconds = 0.0f;
		FName CombatProfileId;
		FTunaSweeperEnemyCombatProfile CombatProfile;
		uint8 FactionId = TunaSweeperFactionIds::NoFaction;
		FName SquadId;
		int32 SquadSlot = INDEX_NONE;
	};

	struct FLootContainerSpawnDefinition
	{
		FName LevelName;
		TSoftClassPtr<ATunaSweeperLootContainerActor> LootContainerClass;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		int32 ContainerDefinitionId = INDEX_NONE;
		int32 ContentsId = INDEX_NONE;
		bool bEditorOnly = false;
	};

	struct FTransparentObstacleSpawnDefinition
	{
		FName LevelName;
		FName ObstacleId;
		TSoftClassPtr<ATunaSweeperTransparentObstacleActor> ObstacleClass;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector BoxExtent = FVector(260.0f, 45.0f, 140.0f);
	};

	struct FWorldProgressObjectSpawnDefinition
	{
		FName LevelName;
		FName ObjectId;
		FName InfoId;
		TSoftClassPtr<ATunaSweeperWorldProgressActor> ProgressActorClass;
		TSoftClassPtr<AActor> CompletedActorClass;
		FText DisplayName;
		FName DisplayNameStringKey;
		FText InteractionDisplayName;
		FName InteractionDisplayNameStringKey;
		FText RequiredItemDisplayName;
		FName RequiredItemDisplayNameStringKey;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector BoxExtent = FVector(260.0f, 55.0f, 140.0f);
		int32 RequiredItemId = 6002;
		int32 RequiredQuantity = 2;
		int32 InitialProgressQuantity = 0;
	};

	struct FWarpPointSpawnDefinition
	{
		FName LevelName;
		FName WarpPointId;
		FName TargetWarpPointId;
		TSoftClassPtr<ATunaSweeperWarpPointActor> WarpPointClass;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector VisualScale = FVector(1.2f, 1.2f, 1.2f);
		FVector VisualRelativeLocation = FVector::ZeroVector;
		FVector ExitOffset = FVector(160.0f, 0.0f, 0.0f);
		bool bUseTargetRotation = true;
	};

	struct FGameplayInteractionActorSpawnDefinition
	{
		FName LevelName;
		FName SpawnId;
		EGameplayInteractionActorSpawnType SpawnType = EGameplayInteractionActorSpawnType::Unknown;
		TSoftClassPtr<AActor> ActorClass;
		FVector Location = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
		FVector Scale = FVector::OneVector;
		FText InteractionDisplayName;
		FName InteractionDisplayNameStringKey;
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> MarkerWidgetClass;

		FName TargetLevelName;
		TSoftObjectPtr<UMediaSource> TransitionMediaSource;
		TSoftClassPtr<UTunaSweeperLevelTransitionWidget> TransitionWidgetClass;
		FText TransitionMessage;
		FName TransitionMessageStringKey;
		TSoftObjectPtr<UStaticMesh> LevelTravelVisualMesh;
		FVector LevelTravelVisualScale = FVector(0.75f, 0.75f, 0.75f);
		FVector LevelTravelVisualRelativeLocation = FVector::ZeroVector;

		float ExtractionRadius = 300.0f;
		float ExtractionHoldSeconds = 4.0f;
		float ExtractionRadiusRingWidth = 4.8f;
		TSoftObjectPtr<UNiagaraSystem> ExtractionParticleSystem;
		TSoftObjectPtr<UMaterialInterface> ExtractionRadiusVisualMaterial;

		int32 ItemId = 1001;
		int32 ItemQuantity = 1;
		bool bDestroyOnPickup = true;
		TSoftClassPtr<UTunaSweeperPickupItemIconWidget> PickupItemIconWidgetClass;

		TSoftClassPtr<ATunaSweeperPickupItemActor> PickupItemActorClass;
		TSoftClassPtr<ATunaSweeperLootContainerActor> LootContainerActorClass;
		int32 ContainerDefinitionId = INDEX_NONE;
		int32 ContentsId = INDEX_NONE;
		int32 ShopId = 1;
		int32 WorkbenchId = 1;
		int32 CurrencyGrantAmount = 1000;
		float MinSpawnRadius = 160.0f;
		float MaxSpawnRadius = 420.0f;
		float SpawnTraceHeight = 800.0f;

		TSoftClassPtr<UTunaSweeperSpeechBubbleWidget> SpeechBubbleWidgetClass;
		int32 CountdownStartNumber = 3;
		float CountdownStepSeconds = 1.0f;
		float BoomDisplaySeconds = 0.2f;
		float ExplosionRadius = 200.0f;
		float ExplosionDamage = 100.0f;

		TSoftClassPtr<ATunaSweeperRollingBomber> RollingBomberClass;
		TSoftObjectPtr<USoundBase> RollingBomberLaunchSound;
		int32 RollingBomberInitialSpawnCount = 2;
		int32 RollingBomberMaxSpawnCount = 8;
		float RollingBomberWaveIntervalSeconds = 10.0f;
		float RollingBomberSpawnIntervalSeconds = 0.2f;
		float RollingBomberLaunchSpeedMin = 850.0f;
		float RollingBomberLaunchSpeedMax = 1100.0f;
		float RollingBomberLaunchPitchMinDegrees = 38.0f;
		float RollingBomberLaunchPitchMaxDegrees = 58.0f;
		float RollingBomberSpawnerMaxHealth = 80.0f;
		int32 RollingBomberSpawnerExperienceValue = 120;

		FVector SandbagCoverBoxExtent = FVector(37.5f, 160.0f, 60.0f);
		float SandbagCoverMaxHealth = 70.0f;
		float SandbagCoverPassthroughRadius = 62.5f;

		float ExplosiveBarrelMaxHealth = 30.0f;
		TSoftObjectPtr<UStaticMesh> ExplosiveBarrelIntactMesh;
		TSoftObjectPtr<UStaticMesh> ExplosiveBarrelDestroyedMesh;
		TSoftObjectPtr<UNiagaraSystem> ExplosiveBarrelDestroyedLoopEffect;
		TSoftClassPtr<ATunaSweeperLocalExplosionEffectActor> ExplosiveBarrelExplosionEffectClass;
		float ExplosiveBarrelExplosionVisualRadius = 210.0f;
		float ExplosiveBarrelExplosionDurationSeconds = 0.72f;

		TSoftObjectPtr<UStaticMesh> StaticMeshPropMesh;
		TArray<TSoftObjectPtr<UMaterialInterface>> StaticMeshPropMaterials;
		FVector StaticMeshPropRelativeLocation = FVector::ZeroVector;
		FRotator StaticMeshPropRelativeRotation = FRotator::ZeroRotator;
		FVector StaticMeshPropRelativeScale = FVector::OneVector;
		bool bStaticMeshPropCollisionEnabled = true;
		bool bStaticMeshPropRevealOccluder = false;
		float StaticMeshPropRevealIntensity = 1.0f;
		float StaticMeshPropRevealCharacterRadiusScale = 1.0f;
		float StaticMeshPropRevealCursorRadiusScale = 1.0f;
		float StaticMeshPropRevealPatternScale = 1.0f;

		float PracticeDummyMaxHealth = 100.0f;
		float PracticeDummyCriticalDamageMultiplier = 3.0f;
		float PracticeDummyHeadshotDamageMultiplier = 6.0f;
		float PracticeDummyHealthRecoverySeconds = 2.0f;

		FName NoiseEmitterMeshDefinitionId = FName(TEXT("mesh.test_noise_quad_horn"));
		FString NoiseEmitterMeshDefinitionJsonRelativePath = TEXT("Data/PeriodicNoiseEmitterMeshes.json");
		float NoiseEmitterIntervalSeconds = 2.0f;
		float NoiseEmitterLoudness = 1.0f;
		float NoiseEmitterMaxRange = 2600.0f;
		FName NoiseEmitterTag = FName(TEXT("noise.test_periodic"));
		FVector NoiseEmitterSourceLocalOffset = FVector(0.0f, 0.0f, 160.0f);
		bool bNoiseEmitterStartEnabled = true;

		FTunaSweeperMapOverlayDefinition MapOverlay;
		bool bHasMapOverlay = false;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetLoadedEnemySpawnData();
	void ResetLoadedEnemyCombatProfileData();
	void ResetLoadedLootContainerSpawnData();
	void ResetLoadedTransparentObstacleSpawnData();
	void ResetLoadedWorldProgressObjectSpawnData();
	void ResetLoadedWarpPointSpawnData();
	void ResetLoadedGameplayInteractionActorSpawnData();
	FString GetEnemySpawnJsonPath() const;
	FString GetEnemyCombatProfileJsonPath() const;
	FString GetLootContainerSpawnJsonPath() const;
	FString GetTransparentObstacleSpawnJsonPath() const;
	FString GetWorldProgressObjectSpawnJsonPath() const;
	FString GetWarpPointSpawnJsonPath() const;
	FString GetGameplayInteractionActorSpawnJsonPath() const;
	bool DoesLevelNameMatchWorld(FName LevelName, const UWorld* World) const;
	void ConfigureGameplayInteractionActor(
		AActor* SpawnedActor,
		const FGameplayInteractionActorSpawnDefinition& SpawnDefinition) const;

	TArray<FEnemySpawnDefinition> EnemySpawnDefinitions;
	TMap<FName, FTunaSweeperEnemyCombatProfile> EnemyCombatProfilesById;
	TArray<FLootContainerSpawnDefinition> LootContainerSpawnDefinitions;
	TArray<FTransparentObstacleSpawnDefinition> TransparentObstacleSpawnDefinitions;
	TArray<FWorldProgressObjectSpawnDefinition> WorldProgressObjectSpawnDefinitions;
	TArray<FWarpPointSpawnDefinition> WarpPointSpawnDefinitions;
	TArray<FGameplayInteractionActorSpawnDefinition> GameplayInteractionActorSpawnDefinitions;

	TWeakObjectPtr<UWorld> LastSpawnedWorld;
	FDelegateHandle PostLoadMapHandle;
	bool bEnemySpawnDataLoaded = false;
	bool bEnemyCombatProfileDataLoaded = false;
	bool bLootContainerSpawnDataLoaded = false;
	bool bTransparentObstacleSpawnDataLoaded = false;
	bool bWorldProgressObjectSpawnDataLoaded = false;
	bool bWarpPointSpawnDataLoaded = false;
	bool bGameplayInteractionActorSpawnDataLoaded = false;
};
