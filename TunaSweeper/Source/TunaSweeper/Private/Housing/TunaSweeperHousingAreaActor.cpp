#include "Housing/TunaSweeperHousingAreaActor.h"

#include "Engine/GameInstance.h"
#include "Subsystem/TunaSweeperHousingSubsystem.h"

ATunaSweeperHousingAreaActor::ATunaSweeperHousingAreaActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
}

void ATunaSweeperHousingAreaActor::BeginPlay()
{
	Super::BeginPlay();

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GameInstance->GetSubsystem<UTunaSweeperHousingSubsystem>())
		{
			HousingSubsystem->RegisterHousingArea(this);
		}
	}
}

void ATunaSweeperHousingAreaActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperHousingSubsystem* HousingSubsystem = GameInstance->GetSubsystem<UTunaSweeperHousingSubsystem>())
		{
			HousingSubsystem->UnregisterHousingArea(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool ATunaSweeperHousingAreaActor::IsCellRectInside(const FIntPoint& AnchorCell, const FIntPoint& SizeCells) const
{
	const int32 SafeGridSizeX = FMath::Max(1, GridSizeX);
	const int32 SafeGridSizeY = FMath::Max(1, GridSizeY);
	const int32 SafeSizeX = FMath::Max(1, SizeCells.X);
	const int32 SafeSizeY = FMath::Max(1, SizeCells.Y);

	return AnchorCell.X >= 0 &&
		AnchorCell.Y >= 0 &&
		AnchorCell.X + SafeSizeX <= SafeGridSizeX &&
		AnchorCell.Y + SafeSizeY <= SafeGridSizeY;
}

bool ATunaSweeperHousingAreaActor::TryGetAnchorCellForWorldLocation(
	const FVector& WorldLocation,
	const FIntPoint& FootprintSize,
	FIntPoint& OutAnchorCell) const
{
	const float SafeCellSize = FMath::Max(1.0f, CellSize);
	const FVector LocalLocation = GetActorTransform().InverseTransformPosition(WorldLocation);
	const float HalfWidth = static_cast<float>(FMath::Max(1, GridSizeX)) * SafeCellSize * 0.5f;
	const float HalfHeight = static_cast<float>(FMath::Max(1, GridSizeY)) * SafeCellSize * 0.5f;
	const FVector2D FractionalCell(
		(LocalLocation.X + HalfWidth) / SafeCellSize,
		(LocalLocation.Y + HalfHeight) / SafeCellSize);
	const FIntPoint SafeFootprint(
		FMath::Max(1, FootprintSize.X),
		FMath::Max(1, FootprintSize.Y));

	OutAnchorCell = FIntPoint(
		FMath::FloorToInt(FractionalCell.X - static_cast<float>(SafeFootprint.X) * 0.5f),
		FMath::FloorToInt(FractionalCell.Y - static_cast<float>(SafeFootprint.Y) * 0.5f));
	return IsCellRectInside(OutAnchorCell, SafeFootprint);
}

FVector ATunaSweeperHousingAreaActor::GetWorldLocationForFootprintCenter(
	const FIntPoint& AnchorCell,
	const FIntPoint& FootprintSize) const
{
	const float SafeCellSize = FMath::Max(1.0f, CellSize);
	const float HalfWidth = static_cast<float>(FMath::Max(1, GridSizeX)) * SafeCellSize * 0.5f;
	const float HalfHeight = static_cast<float>(FMath::Max(1, GridSizeY)) * SafeCellSize * 0.5f;
	const FIntPoint SafeFootprint(
		FMath::Max(1, FootprintSize.X),
		FMath::Max(1, FootprintSize.Y));
	const FVector LocalLocation(
		-HalfWidth + (static_cast<float>(AnchorCell.X) + static_cast<float>(SafeFootprint.X) * 0.5f) * SafeCellSize,
		-HalfHeight + (static_cast<float>(AnchorCell.Y) + static_cast<float>(SafeFootprint.Y) * 0.5f) * SafeCellSize,
		0.0f);

	return GetActorTransform().TransformPosition(LocalLocation);
}

FVector ATunaSweeperHousingAreaActor::GetWorldLocationForCellCorner(const FIntPoint& CellCorner) const
{
	return GetActorTransform().TransformPosition(GetLocalLocationForCellCorner(CellCorner));
}

FRotator ATunaSweeperHousingAreaActor::GetAreaYawRotation() const
{
	const FRotator ActorRotation = GetActorRotation();
	return FRotator(0.0f, ActorRotation.Yaw, 0.0f);
}

FVector ATunaSweeperHousingAreaActor::GetLocalLocationForCellCorner(const FIntPoint& CellCorner) const
{
	const float SafeCellSize = FMath::Max(1.0f, CellSize);
	const float HalfWidth = static_cast<float>(FMath::Max(1, GridSizeX)) * SafeCellSize * 0.5f;
	const float HalfHeight = static_cast<float>(FMath::Max(1, GridSizeY)) * SafeCellSize * 0.5f;
	return FVector(
		-HalfWidth + static_cast<float>(CellCorner.X) * SafeCellSize,
		-HalfHeight + static_cast<float>(CellCorner.Y) * SafeCellSize,
		0.0f);
}
