#include "AnimGraphNode_TunaSweeperTitleHeadLook.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"

namespace
{
	FText ResolveNodeText(FName Key)
	{
		const FString Language = FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName();
		const auto TextLanguage = Language == TEXT("ko") ? ETunaSweeperItemTextLanguage::Korean
			: Language == TEXT("ja") ? ETunaSweeperItemTextLanguage::Japanese : ETunaSweeperItemTextLanguage::English;
		return GetDefault<UTunaSweeperTextSubsystem>()->ResolveText(Key, TextLanguage, FText::GetEmpty());
	}
}

FText UAnimGraphNode_TunaSweeperTitleHeadLook::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

FText UAnimGraphNode_TunaSweeperTitleHeadLook::GetControllerDescription() const
{
	return ResolveNodeText(TEXT("editor.anim.title_head_look.title"));
}

FText UAnimGraphNode_TunaSweeperTitleHeadLook::GetTooltipText() const
{
	return ResolveNodeText(TEXT("editor.anim.title_head_look.tooltip"));
}
