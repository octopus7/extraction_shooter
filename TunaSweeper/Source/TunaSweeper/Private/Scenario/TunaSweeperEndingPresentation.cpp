#include "Scenario/TunaSweeperEndingPresentation.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"

bool UTunaSweeperEndingPresentation::ResolveDialogue(UTunaSweeperGameInstance* Game,
    TArray<FTunaSweeperDialogueLine>& OutLines) const
{
    OutLines.Reset();
    const auto* Strings = Game ? Game->GetSubsystem<UTunaSweeperTextSubsystem>() : nullptr;
    if (!Strings || LineStringKeys.IsEmpty()) return false;
    TArray<FName> Keys{TitleStringKey};
    Keys.Append(LineStringKeys);
    for (FName Key : Keys)
    {
        FTunaSweeperDialogueLine Line;
        if (!Strings->TryGetTextByKey(Key, Game->GetCurrentTextLanguage(), Line.DialogueText) || Line.DialogueText.IsEmpty())
        {
            OutLines.Reset();
            return false;
        }
        // A story title gets its own card; it must not overflow the compact NPC-name tab.
        OutLines.Add(MoveTemp(Line));
    }
    return true;
}
