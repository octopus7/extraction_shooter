#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "TunaSweeperGameInstance.generated.h"

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperGameplaySettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	float InteractionTraceDistance = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	int32 MaxInventorySlots = 24;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Gameplay")
	bool bEnableDebugGameplay = false;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "TunaSweeper|Settings")
	FTunaSweeperGameplaySettings GameplaySettings;

	UPROPERTY(BlueprintReadWrite, Category = "TunaSweeper|State")
	TMap<FName, FString> GameplayInfo;

	UPROPERTY(BlueprintReadWrite, Category = "TunaSweeper|State")
	TMap<FName, float> NumberSettings;

	UPROPERTY(BlueprintReadWrite, Category = "TunaSweeper|State")
	TMap<FName, bool> BoolSettings;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Gameplay Info")
	void SetGameplayInfo(FName Key, const FString& Value);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Gameplay Info")
	bool TryGetGameplayInfo(FName Key, FString& OutValue) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Settings")
	void SetNumberSetting(FName Key, float Value);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Settings")
	bool TryGetNumberSetting(FName Key, float& OutValue) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Settings")
	void SetBoolSetting(FName Key, bool bValue);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Settings")
	bool TryGetBoolSetting(FName Key, bool& bOutValue) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|State")
	void ClearRuntimeState();
};
