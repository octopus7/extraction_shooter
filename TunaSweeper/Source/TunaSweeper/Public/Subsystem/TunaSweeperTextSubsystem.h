#pragma once

#include "CoreMinimal.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperTextSubsystem.generated.h"

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperLocalizedTextString
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Text")
	FName StringKey;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Text")
	FText Korean;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Text")
	FText English;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Text")
	FText Japanese;
};

UCLASS()
class TUNASWEEPER_API UTunaSweeperTextSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Text")
	bool LoadTextData(bool bForceReload = false) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Text")
	bool IsTextDataLoaded() const { return bTextDataLoaded; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Text")
	bool TryGetTextByKey(FName StringKey, ETunaSweeperItemTextLanguage Language, FText& OutText) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Text")
	FText ResolveText(FName StringKey, ETunaSweeperItemTextLanguage Language, const FText& FallbackText) const;

private:
	bool EnsureTextDataLoaded() const;
	bool LoadTextStringsCsv() const;
	void ResetLoadedTextData() const;
	FString GetTextStringsCsvPath() const;

	mutable TMap<FName, FTunaSweeperLocalizedTextString> TextStringsByKey;

	mutable bool bTextDataLoaded = false;
};
