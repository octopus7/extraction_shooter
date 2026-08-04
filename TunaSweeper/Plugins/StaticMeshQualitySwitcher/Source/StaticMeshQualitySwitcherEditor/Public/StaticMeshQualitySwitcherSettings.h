#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "StaticMeshQualitySwitcherSettings.generated.h"

class UStaticMeshQualityProfile;

UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig, DisplayName = "Static Mesh Quality Switcher")
class STATICMESHQUALITYSWITCHEREDITOR_API UStaticMeshQualitySwitcherSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(Config, EditAnywhere, Category = "Profile")
	TSoftObjectPtr<UStaticMeshQualityProfile> ActiveProfile;

	virtual FName GetCategoryName() const override
	{
		return TEXT("Plugins");
	}
};
