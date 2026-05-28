#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperHousingAreaActor.generated.h"

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperHousingAreaActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperHousingAreaActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	FName GetAreaId() const { return AreaId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	int32 GetGridSizeX() const { return GridSizeX; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	int32 GetGridSizeY() const { return GridSizeY; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	float GetCellSize() const { return CellSize; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	bool IsCellRectInside(const FIntPoint& AnchorCell, const FIntPoint& SizeCells) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	bool TryGetAnchorCellForWorldLocation(
		const FVector& WorldLocation,
		const FIntPoint& FootprintSize,
		FIntPoint& OutAnchorCell) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	FVector GetWorldLocationForFootprintCenter(
		const FIntPoint& AnchorCell,
		const FIntPoint& FootprintSize) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	FVector GetWorldLocationForCellCorner(const FIntPoint& CellCorner) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	FRotator GetAreaYawRotation() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing")
	FName AreaId = FName(TEXT("bunker_housing_main"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridSizeX = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing", meta = (ClampMin = "1", UIMin = "1"))
	int32 GridSizeY = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Housing", meta = (ClampMin = "10.0", UIMin = "10.0"))
	float CellSize = 120.0f;

private:
	FVector GetLocalLocationForCellCorner(const FIntPoint& CellCorner) const;
};
