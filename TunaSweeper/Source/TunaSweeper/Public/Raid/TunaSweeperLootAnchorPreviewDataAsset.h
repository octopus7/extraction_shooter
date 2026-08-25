#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperLootAnchorPreviewDataAsset.generated.h"

class UStaticMesh;

/** One editor-only representative mesh choice for a placed loot placement anchor. */
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperLootAnchorPreviewDefinition
{
	GENERATED_BODY()

	/** Stable option key shown by the anchor's editor combo box. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Loot Anchor Preview")
	FName PreviewId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Loot Anchor Preview")
	TSoftObjectPtr<UStaticMesh> PreviewMesh;

	/** Scale applied only to the editor preview component. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Loot Anchor Preview")
	FVector RelativeScale = FVector::OneVector;

	/** Keeps a preview's base on the anchor plane when meshes have different bounds. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Loot Anchor Preview")
	FVector RelativeLocation = FVector::ZeroVector;
};

/**
 * Global, BP-referenced editor preview catalog. It has no runtime spawn authority; add entries
 * here to extend the combo choices shown on ATunaSweeperRaidPlacementAnchor instances.
 */
UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperLootAnchorPreviewDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	const FTunaSweeperLootAnchorPreviewDefinition* FindPreview(FName PreviewId) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Loot Anchor Preview")
	TArray<FTunaSweeperLootAnchorPreviewDefinition> PreviewDefinitions;
};
