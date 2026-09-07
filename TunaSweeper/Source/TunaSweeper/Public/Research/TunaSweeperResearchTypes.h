#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperResearchTypes.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperResearchEffectType : uint8
{
	MaxHealth, MaxFood, MaxHydration, MaxStamina, CarryStrength,
	WeaponBurnTicks, AmmoBurnTicks, WeaponBurnDamage, AmmoBurnDamage
};

UENUM(BlueprintType)
enum class ETunaSweeperResearchNodeState : uint8 { Locked, Available, Researching, ReadyToClaim, Applied };

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperResearchEffect
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") ETunaSweeperResearchEffectType Type = ETunaSweeperResearchEffectType::MaxHealth;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") float Value = 0.0f;
	// Empty applies to every weapon/ammo type; otherwise match the relevant item's type tag.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") FName TargetTypeTag;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperResearchNodeDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") FName NodeId;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") int32 Row = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") int32 Column = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") int32 RequiredAppliedNodeCount = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") int32 DurationSeconds = 10;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") TArray<FName> ParentNodeIds;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") FName DisplayNameStringKey;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") FName DescriptionStringKey;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") FSoftObjectPath Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Research") TArray<FTunaSweeperResearchEffect> Effects;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperActiveResearchSaveData
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Research") FName NodeId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Research") int64 StartUtcTicks = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Research") int64 FinishUtcTicks = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Research") bool bTimerCompleted = false;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperResearchStatBonuses
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") float MaxHealth = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") float MaxFood = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") float MaxHydration = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") float MaxStamina = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") float CarryStrength = 0.0f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperResearchBurnBonuses
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") int32 AdditionalTickCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") float DamageMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperResearchNodeView
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") FName NodeId;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") FText DisplayName;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") FText Description;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") int32 Row = 0;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") int32 Column = 0;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") int32 RequiredAppliedNodeCount = 0;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") int32 DurationSeconds = 0;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") int32 RemainingSeconds = 0;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") float Progress = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|Research") ETunaSweeperResearchNodeState State = ETunaSweeperResearchNodeState::Locked;
};
