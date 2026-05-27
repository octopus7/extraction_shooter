#pragma once

#include "CoreMinimal.h"
#include "TunaSweeperMemoTypes.generated.h"

USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperMemoDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo")
	int32 MemoId = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Memo")
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
