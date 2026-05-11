#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperGameInstance.generated.h"

class UTexture2D;

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

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperTempOpenLootItemData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot")
	FText DisplayName;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot")
	int32 Quantity = 1;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Temp Open Loot")
	TSoftObjectPtr<UTexture2D> IconTexture;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperPlayerHudState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float CurrentCarryWeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MaxCarryWeight = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float MovementBlockedWeight = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Health = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Hunger = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD", meta = (ClampMin = "0.0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0"))
	float Hydration = 100.0f;

	void NormalizeWeightLimits();
	bool IsCarryWeightOverLimit() const;
	bool IsCarryWeightMovementBlocked() const;
	float GetCarryWeightMovementSpeedMultiplier() const;
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD")
	FTunaSweeperPlayerHudState PlayerHudState;

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

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetPlayerHudState(const FTunaSweeperPlayerHudState& InHudState);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetCarryWeight(float CurrentCarryWeight, float MaxCarryWeight, float MovementBlockedWeight);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	float GetCarryWeightMovementSpeedMultiplier() const;

	const TArray<FTunaSweeperTempOpenLootItemData>& GetOrCreateTempOpenLootItems();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Temp Open Loot")
	void GetTempOpenLootItems(TArray<FTunaSweeperTempOpenLootItemData>& OutItems);

	const TArray<FTunaSweeperItemStack>& GetOrCreatePlayerInventoryItems();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Inventory")
	void GetPlayerInventoryItems(TArray<FTunaSweeperItemStack>& OutItems);

private:
	void GenerateTempOpenLootItems();
	void GeneratePlayerInventoryItems();

	UPROPERTY()
	TArray<FTunaSweeperTempOpenLootItemData> TempOpenLootItems;

	UPROPERTY()
	bool bHasGeneratedTempOpenLootItems = false;

	UPROPERTY()
	TArray<FTunaSweeperItemStack> PlayerInventoryItems;

	UPROPERTY()
	bool bHasGeneratedPlayerInventoryItems = false;
};
