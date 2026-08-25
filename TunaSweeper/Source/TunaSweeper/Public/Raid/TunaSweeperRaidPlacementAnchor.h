#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperRaidPlacementAnchor.generated.h"

class USceneComponent;
class UTunaSweeperLootAnchorPreviewDataAsset;

#if WITH_EDITORONLY_DATA
class UArrowComponent;
class UBillboardComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
#endif

/** The only raid-placement values serialized into a level besides this actor's transform. */
UENUM(BlueprintType)
enum class ETunaSweeperRaidPlacementAnchorKind : uint8
{
	Enemy UMETA(DisplayName = "Enemy"),
	LootContainer UMETA(DisplayName = "Loot Container")
};

/**
 * Lightweight level-authored location for a data-owned raid spawn. This actor has no gameplay
 * collision or authority; runtime actors are created by UTunaSweeperRaidPlacementSubsystem.
 */
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperRaidPlacementAnchor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperRaidPlacementAnchor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Placement")
	int32 GetPlacementId() const { return PlacementId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Raid Placement")
	ETunaSweeperRaidPlacementAnchorKind GetAnchorKind() const { return AnchorKind; }

#if WITH_EDITOR
	/** Names used by the editor details-panel combo; sourced from the BP's preview data asset. */
	UFUNCTION()
	TArray<FString> GetLootPreviewOptions() const;

	/** Used only while creating/updating the anchor Blueprint prefab. */
	void SetLootPreviewDataAssetForEditor(TSoftObjectPtr<UTunaSweeperLootAnchorPreviewDataAsset> InDataAsset);
#endif

private:
	void RefreshEditorPreview();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	/** Stable positive number, unique with the level id across both anchor kinds. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Raid Placement", meta = (AllowPrivateAccess = "true", ClampMin = "1"))
	int32 PlacementId = 1;

	/** Structural anchor type. Runtime data must reference the matching kind. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Raid Placement", meta = (AllowPrivateAccess = "true"))
	ETunaSweeperRaidPlacementAnchorKind AnchorKind = ETunaSweeperRaidPlacementAnchorKind::Enemy;

#if WITH_EDITORONLY_DATA
	/** Set on BP_RaidPlacementAnchor, never consulted by runtime spawning. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Raid Placement|Loot Preview", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UTunaSweeperLootAnchorPreviewDataAsset> LootPreviewDataAsset;

	/** Combo selection for the editor-only loot representative; data stays in the DA, not spawn JSON. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Raid Placement|Loot Preview", meta = (AllowPrivateAccess = "true", GetOptions = "GetLootPreviewOptions", EditCondition = "AnchorKind == ETunaSweeperRaidPlacementAnchorKind::LootContainer", EditConditionHides))
	FName LootPreviewId = NAME_None;

	/** Editor-only visualization components: never used for runtime gameplay or serialized data resolution. */
	UPROPERTY(Transient)
	TObjectPtr<UArrowComponent> EditorPreviewArrow;

	UPROPERTY(Transient)
	TObjectPtr<UBillboardComponent> EditorPreviewBillboard;

	/** Mesh and transform are supplied by the BP-referenced loot preview data asset. */
	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> EditorLootBoxPreview;

	UPROPERTY(Transient)
	TObjectPtr<UTextRenderComponent> EditorPreviewLabel;
#endif
};
