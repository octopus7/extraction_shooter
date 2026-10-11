#pragma once

#include "CoreMinimal.h"
#include "Memo/TunaSweeperMemoTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperMemoSubsystem.generated.h"


UCLASS()
class TUNASWEEPER_API UTunaSweeperMemoSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	bool LoadMemoDefinitions(bool bForceReload = false);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Memo")
	bool IsMemoDefinitionDataLoaded() const { return bMemoDefinitionDataLoaded; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	bool TryGetMemoDefinition(int32 MemoId, FTunaSweeperMemoDefinition& OutDefinition);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	void GetMemoListEntries(TArray<FTunaSweeperMemoListEntry>& OutEntries);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	int32 GetFirstAcquiredMemoId();

private:
    void ResetLoadedMemoDefinitions();
    FString GetMemoDefinitionsJsonPath() const;
    TMap<int32, FTunaSweeperMemoDefinition> MemoDefinitionsById;
    bool bMemoDefinitionDataLoaded = false;
};
