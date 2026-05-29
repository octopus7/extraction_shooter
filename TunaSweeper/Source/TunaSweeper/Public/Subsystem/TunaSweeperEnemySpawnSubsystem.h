#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperEnemySpawnSubsystem.generated.h"

class AActor;
class ATunaSweeperEnemyCharacter;
class ATunaSweeperExplosiveBarrelActor;
class ATunaSweeperExtractionPointActor;
class ATunaSweeperLocalExplosionEffectActor;
class ATunaSweeperItemSpawnInteractableActor;
class ATunaSweeperLevelTravelInteractableActor;
class ATunaSweeperLootContainerActor;
class ATunaSweeperLootContainerSpawnInteractableActor;
class ATunaSweeperPickupItemActor;
class ATunaSweeperRollingBomber;
class ATunaSweeperRollingBomberSpawner;
class ATunaSweeperSandbagCoverActor;
class ATunaSweeperSelfDestructInteractableActor;
class ATunaSweeperTransparentObstacleActor;
class ATunaSweeperWarpPointActor;
class ATunaSweeperWorldProgressActor;
class UMaterialInterface;
class UMediaSource;
class UNiagaraSystem;
class USoundBase;
class UWorld;
class UStaticMesh;
class UTunaSweeperExtractionProgressWidget;
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
		StaticMeshProp
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
		int32 ExperienceValue = 30;
		float MaxHealth = 30.0f;
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
		FText InteractionDisplayName;
		FText RequiredItemDisplayName;
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
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> MarkerWidgetClass;

		FName TargetLevelName;
		TSoftObjectPtr<UMediaSource> TransitionMediaSource;
		TSoftClassPtr<UTunaSweeperLevelTransitionWidget> TransitionWidgetClass;
		FText TransitionMessage;
		TSoftObjectPtr<UStaticMesh> LevelTravelVisualMesh;
		FVector LevelTravelVisualScale = FVector(0.75f, 0.75f, 0.75f);
		FVector LevelTravelVisualRelativeLocation = FVector::ZeroVector;

		float ExtractionRadius = 300.0f;
		float ExtractionHoldSeconds = 4.0f;
		float ExtractionRadiusRingWidth = 4.8f;
		TSoftClassPtr<UTunaSweeperExtractionProgressWidget> ExtractionProgressWidgetClass;
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

		FVector SandbagCoverBoxExtent = FVector(75.0f, 320.0f, 90.0f);
		float SandbagCoverMaxHealth = 70.0f;
		float SandbagCoverPassthroughRadius = 125.0f;

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

		FTunaSweeperMapOverlayDefinition MapOverlay;
		bool bHasMapOverlay = false;
	};

	void HandlePostLoadMapWithWorld(UWorld* LoadedWorld);
	void ResetLoadedEnemySpawnData();
	void ResetLoadedLootContainerSpawnData();
	void ResetLoadedTransparentObstacleSpawnData();
	void ResetLoadedWorldProgressObjectSpawnData();
	void ResetLoadedWarpPointSpawnData();
	void ResetLoadedGameplayInteractionActorSpawnData();
	FString GetEnemySpawnJsonPath() const;
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
	TArray<FLootContainerSpawnDefinition> LootContainerSpawnDefinitions;
	TArray<FTransparentObstacleSpawnDefinition> TransparentObstacleSpawnDefinitions;
	TArray<FWorldProgressObjectSpawnDefinition> WorldProgressObjectSpawnDefinitions;
	TArray<FWarpPointSpawnDefinition> WarpPointSpawnDefinitions;
	TArray<FGameplayInteractionActorSpawnDefinition> GameplayInteractionActorSpawnDefinitions;

	TWeakObjectPtr<UWorld> LastSpawnedWorld;
	FDelegateHandle PostLoadMapHandle;
	bool bEnemySpawnDataLoaded = false;
	bool bLootContainerSpawnDataLoaded = false;
	bool bTransparentObstacleSpawnDataLoaded = false;
	bool bWorldProgressObjectSpawnDataLoaded = false;
	bool bWarpPointSpawnDataLoaded = false;
	bool bGameplayInteractionActorSpawnDataLoaded = false;
};
