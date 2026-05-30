#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperHousingTypes.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperHousingFacilityBuildState : uint8
{
	Buildable UMETA(DisplayName = "Buildable"),
	InsufficientMaterials UMETA(DisplayName = "Insufficient Materials"),
	Stored UMETA(DisplayName = "Stored"),
	Placed UMETA(DisplayName = "Placed")
};

UENUM(BlueprintType)
enum class ETunaSweeperHousingPlacementStatus : uint8
{
	None UMETA(DisplayName = "None"),
	Valid UMETA(DisplayName = "Valid"),
	OutsideArea UMETA(DisplayName = "Outside Area"),
	Occupied UMETA(DisplayName = "Occupied"),
	UnknownFacility UMETA(DisplayName = "Unknown Facility")
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperHousingMaterialCost
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing", meta = (ClampMin = "1", UIMin = "1"))
	int32 ItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperHousingFacilityDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FName FacilityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FName DisplayNameStringKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FName DescriptionStringKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FText FallbackDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FText FallbackDescription;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing", meta = (ClampMin = "1", UIMin = "1"))
	int32 SizeX = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing", meta = (ClampMin = "1", UIMin = "1"))
	int32 SizeY = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	TArray<FTunaSweeperHousingMaterialCost> RequiredMaterials;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	TArray<int32> UnlockWhenEverAcquiredItemIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FString ActorClassPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FString StaticMeshPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FString MaterialPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing", meta = (ClampMin = "0", UIMin = "0"))
	int32 SortOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	bool bFunctionUnlockedByDefault = true;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperHousingPlacedFacilitySaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FGuid InstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FName FacilityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FIntPoint AnchorCell = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing", meta = (ClampMin = "0", ClampMax = "3", UIMin = "0", UIMax = "3"))
	int32 RotationQuarterTurns = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	bool bStored = true;

	bool IsValid() const
	{
		return InstanceId.IsValid() && !FacilityId.IsNone();
	}
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperHousingFacilityView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	FGuid InstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	FName FacilityId;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	FText Description;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	FText MaterialsText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	FText StateText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	ETunaSweeperHousingFacilityBuildState BuildState = ETunaSweeperHousingFacilityBuildState::InsufficientMaterials;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	int32 SizeX = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	int32 SizeY = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	bool bCanStartPlacement = false;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Housing")
	bool bCanStore = false;
};
