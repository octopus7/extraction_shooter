#include "IconEditor/IconEditorTerminal.h"

#include "GameFramework/Pawn.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/Astra/IconEditorGPT6Astra.h"

AIconEditorTerminal::AIconEditorTerminal()
{
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::IconEditor, FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(
			TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))), TEXT("ui.icon_editor.title"));
	EditorWidgetClass = TSoftClassPtr<UIconEditorGPT6Astra>(FSoftObjectPath(
		TEXT("/Game/UI/IconEditor/WBP_IconEditor_GPT6Astra.WBP_IconEditor_GPT6Astra_C")));
}

bool AIconEditorTerminal::OpenFor(APawn* Player)
{
	auto* Controller = Player ? Cast<ATunaSweeperPlayerController>(Player->GetController()) : nullptr;
	return Controller && IsWithinInteractionDistance(Player) && Controller->OpenIconEditorPanel(this);
}

TSubclassOf<UIconEditorGPT6Astra> AIconEditorTerminal::ResolveEditorWidgetClass() const
{
	return EditorWidgetClass.LoadSynchronous();
}
