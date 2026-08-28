#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperGraphicsSettingsTypes.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperGraphicsPreset : uint8
{
	Auto = 0,
	Low,
	Medium,
	High,
	Epic,
	Custom
};

UENUM(BlueprintType)
enum class ETunaSweeperScalabilityOption : uint8
{
	Texture = 0,
	Shadow,
	GlobalIllumination,
	Reflection,
	ViewDistance,
	Effects,
	PostProcess,
	Foliage,
	Shading,
	Landscape,
	AntiAliasing
};

UENUM(BlueprintType)
enum class ETunaSweeperTitleDLSSMode : uint8
{
	Off = 0,
	Quality,
	Balanced,
	Performance
};
