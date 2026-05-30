#pragma once

#include "CoreMinimal.h"
#include "Debuff/TunaSweeperDebuffTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperDebuffDataSubsystem.generated.h"

UCLASS()
class TUNASWEEPER_API UTunaSweeperDebuffDataSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debuff Data")
	bool LoadDebuffData(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debuff Data")
	bool IsDebuffDataLoaded() const { return bDebuffDataLoaded; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Debuff Data")
	bool TryGetDebuffDefinition(FName DebuffId, FTunaSweeperDebuffDefinition& OutDefinition);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debuff Data")
	float GetGlobalTickIntervalSeconds() const { return GlobalTickIntervalSeconds; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debuff Data")
	FTunaSweeperCarryWeightDebuffSettings GetCarryWeightSettings();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Debuff Data")
	FString BuildDebuffIconObjectPath(const FTunaSweeperDebuffDefinition& Definition) const;

private:
	bool EnsureDebuffDataLoaded();
	bool LoadDebuffDefinitionsJson();
	void ResetLoadedDebuffData();
	void InstallFallbackDefinitions();
	FString GetDebuffDefinitionsJsonPath() const;

	UPROPERTY(Transient)
	TMap<FName, FTunaSweeperDebuffDefinition> DebuffDefinitionsById;

	bool bDebuffDataLoaded = false;
	float GlobalTickIntervalSeconds = 2.0f;
	FTunaSweeperCarryWeightDebuffSettings CarryWeightSettings;
};
