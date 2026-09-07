#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperDifficultySubsystem.generated.h"

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperDifficultyDefinition
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Difficulty")
	int32 DifficultyStage = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Difficulty")
	FName TitleStringKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Difficulty")
	FName DescriptionStringKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Difficulty")
	int32 EnemyIncomingDamageMultiplier = 10000;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperDifficultySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Difficulty")
	bool LoadDifficultyData(bool bForceReload = false);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Difficulty")
	bool TryGetDifficultyDefinition(int32 DifficultyStage, FTunaSweeperDifficultyDefinition& OutDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Difficulty")
	int32 GetEnemyIncomingDamageMultiplier(int32 DifficultyStage);

	static float ScaleEnemyIncomingDamage(float RawDamage, int32 RawMultiplier);
	static float ResolveAppliedPlayerDamage(
		float RawDamage,
		int32 DefenseValue,
		bool bEnemyAttributed,
		int32 EnemyIncomingDamageMultiplier);

private:
	TMap<int32, FTunaSweeperDifficultyDefinition> DefinitionsByStage;
	bool bDifficultyDataLoaded = false;
};
