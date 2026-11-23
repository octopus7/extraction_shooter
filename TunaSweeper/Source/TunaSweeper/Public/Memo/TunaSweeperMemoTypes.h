#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperMemoTypes.generated.h"

namespace TunaSweeperMemo
{
    // Temporarily release only the first set of authored memos.
    inline constexpr int32 LastAvailableId = 15;
    constexpr bool IsAvailable(int32 Id) { return Id > 0 && Id <= LastAvailableId; }
}

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperMemoDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo")
	int32 MemoId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo")
	FName BodyStringKey;

	// Derived from the shared localized title format and MemoId.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category = "TunaSweeper|Memo")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo", meta = (MultiLine = "true"))
	FText Body;
};

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperMemoListEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo")
	int32 MemoId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo")
	FText Title;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo", meta = (MultiLine = "true"))
	FText Body;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo")
	bool bAcquired = false;
};
