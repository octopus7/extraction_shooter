#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "IconStorageSubsystem.generated.h"

enum class EIconLoadResult : uint8
{
	Missing,
	Loaded,
	Invalid,
	ReadFailed
};

/** One opaque RGB8 icon per account/build flavor, independent of gameplay save slots. */
UCLASS()
class TUNASWEEPER_API UIconStorageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	static constexpr int32 Width = 32;
	static constexpr int32 Height = 32;
	static constexpr int32 ByteCount = Width * Height * 3;

	/** Always supplies a complete black draft on missing/invalid/unreadable input. Never writes. */
	EIconLoadResult LoadRgbIcon(TArray<uint8>& OutPixels) const;
	/** Explicit confirmation only. Failed writes do not delete the previously saved icon. */
	bool TrySaveIcon(const TArray<uint8>& Pixels) const;
	FString GetIconFilePath() const;
};
