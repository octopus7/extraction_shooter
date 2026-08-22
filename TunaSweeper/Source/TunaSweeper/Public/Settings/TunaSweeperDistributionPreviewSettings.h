#pragma once
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "TunaSweeperDistributionPreviewSettings.generated.h"
UENUM() enum class ETunaSweeperPreviewBuildType : uint8 { Demo UMETA(DisplayName="Demo"), Full UMETA(DisplayName="Full Game") };
UENUM() enum class ETunaSweeperPreviewDistributionChannel : uint8 { Editor UMETA(DisplayName="Editor"), Steam UMETA(DisplayName="Steam"), Stove UMETA(DisplayName="STOVE") };
UCLASS(config=Game, defaultconfig, meta=(DisplayName="Distribution Preview"))
class TUNASWEEPER_API UTunaSweeperDistributionPreviewSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, config, Category="Preview") ETunaSweeperPreviewBuildType BuildType = ETunaSweeperPreviewBuildType::Full;
	UPROPERTY(EditAnywhere, config, Category="Preview") ETunaSweeperPreviewDistributionChannel DistributionChannel = ETunaSweeperPreviewDistributionChannel::Editor;
	virtual FName GetCategoryName() const override { return TEXT("TunaSweeper"); }
	virtual FName GetSectionName() const override { return TEXT("Distribution Preview"); }
};