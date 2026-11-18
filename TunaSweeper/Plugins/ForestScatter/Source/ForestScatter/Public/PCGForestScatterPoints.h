#pragma once

#include "PCGSettings.h"

#include "PCGForestScatterPoints.generated.h"

/** Generates ground cover and flower points for native static mesh spawners. */
UCLASS(BlueprintType, ClassGroup = (Procedural))
class FORESTSCATTER_API UPCGForestScatterPointsSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	virtual bool UseSeed() const override { return true; }

#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override { return TEXT("ForestScatterPoints"); }
	virtual FText GetDefaultNodeTitle() const override;
	virtual EPCGSettingsType GetType() const override { return EPCGSettingsType::Spatial; }
#endif

protected:
	virtual TArray<FPCGPinProperties> InputPinProperties() const override { return {}; }
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;
	virtual FPCGElementPtr CreateElement() const override;
};
