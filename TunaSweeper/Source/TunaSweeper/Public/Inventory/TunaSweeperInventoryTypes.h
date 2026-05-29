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
	Storage UMETA(DisplayName = "Storage"),
	Shop UMETA(DisplayName = "Shop"),
	WorkbenchRecipe UMETA(DisplayName = "Workbench Recipe"),
	WorkbenchDismantleItem UMETA(DisplayName = "Workbench Dismantle Item"),
	WorkbenchBlueprintItem UMETA(DisplayName = "Workbench Blueprint Item"),
	SelectedWeaponAttachment UMETA(DisplayName = "Selected Weapon Attachment"),
	UsableQuickSlot UMETA(DisplayName = "Usable Quick Slot")
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	int32 LoadedAmmoItemId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory", meta = (ClampMin = "0", UIMin = "0"))
	int32 LoadedAmmoCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	int32 SelectedAmmoItemId = INDEX_NONE;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Inventory")
	bool bSortLocked = false;

	bool IsEmpty() const
	{
		return !ItemUid.IsValid();
	}

	void Clear()
	{
		ItemUid.Invalidate();
		bSortLocked = false;
	}
};
