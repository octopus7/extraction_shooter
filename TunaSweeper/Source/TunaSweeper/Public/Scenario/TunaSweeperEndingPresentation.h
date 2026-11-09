#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/TunaSweeperDialogueWidget.h"
#include "TunaSweeperEndingPresentation.generated.h"

/** Replaceable placeholder story. All display text resolves through the project string table. */
UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperEndingPresentation : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ending")
    FName TitleStringKey;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Ending")
    TArray<FName> LineStringKeys;
    bool ResolveDialogue(class UTunaSweeperGameInstance* Game, TArray<FTunaSweeperDialogueLine>& OutLines) const;
};
