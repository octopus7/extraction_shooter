#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "IconEditorTerminal.generated.h"

class UIconEditorGPT6Astra;

/** Placeable editor terminal. All terminals edit the same account-global icon. */
UCLASS(Blueprintable)
class TUNASWEEPER_API AIconEditorTerminal : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	AIconEditorTerminal();
	bool OpenFor(APawn* Player);
	TSubclassOf<UIconEditorGPT6Astra> ResolveEditorWidgetClass() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Icon Editor")
	TSoftClassPtr<UIconEditorGPT6Astra> EditorWidgetClass;
};
