#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
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

UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Build Target"))
class TUNASWEEPER_API UTunaSweeperBuildTargetSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, config, Category = "Build Target", meta = (DisplayName = "Target"))
	ETunaSweeperBuildTarget BuildTarget = ETunaSweeperBuildTarget::NoStoreFull;

	virtual FName GetCategoryName() const override { return TEXT("TunaSweeper"); }
	virtual FName GetSectionName() const override { return TEXT("Build Target"); }

	FString GetDistributionChannel() const;
	bool IsDemoBuild() const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
#if WITH_EDITOR
	void ApplyPackagingTarget();
#endif
};
