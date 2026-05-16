#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperInventoryTypes.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperItemSlotSource : uint8
{
	Equipment UMETA(DisplayName = "Equipment"),
	AuxiliaryBag UMETA(DisplayName = "Auxiliary Bag"),
	Inventory UMETA(DisplayName = "Inventory"),
	LootContainer UMETA(DisplayName = "Loot Container"),
	SelectedWeaponAttachment UMETA(DisplayName = "Selected Weapon Attachment")
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperItemSlotReference
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	ETunaSweeperItemSlotSource Source = ETunaSweeperItemSlotSource::Inventory;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	int32 SlotIndex = INDEX_NONE;

	bool IsValid() const
	{
		return SlotIndex != INDEX_NONE;
	}
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperItemInstance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	FGuid Uid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory", meta = (ClampMin = "1", UIMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	TMap<FName, FGuid> AttachmentSlots;

	bool IsValid() const
	{
		return Uid.IsValid() && ItemId != INDEX_NONE && Quantity > 0;
	}
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperInventorySlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	FGuid ItemUid;

	bool IsEmpty() const
	{
		return !ItemUid.IsValid();
	}

	void Clear()
	{
		ItemUid.Invalidate();
	}
};
