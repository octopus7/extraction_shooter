#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TunaSweeperBuildTargetSettings.generated.h"

UENUM()
enum class ETunaSweeperBuildTarget : uint8
{
	NoStoreFull UMETA(DisplayName = "No Store - Full Game"),
	NoStoreDemo UMETA(DisplayName = "No Store - Demo"),
	SteamFull UMETA(DisplayName = "Steam - Full Game"),
	SteamDemo UMETA(DisplayName = "Steam - Demo"),
	StoveFull UMETA(DisplayName = "STOVE - Full Game"),
	StoveDemo UMETA(DisplayName = "STOVE - Demo")
};

UCLASS(config = Game, defaultconfig)
class TUNASWEEPER_API UTunaSweeperBuildTargetSettings : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(config)
	ETunaSweeperBuildTarget BuildTarget = ETunaSweeperBuildTarget::NoStoreFull;

	FString GetDistributionChannel() const;
	bool IsDemoBuild() const;
};
