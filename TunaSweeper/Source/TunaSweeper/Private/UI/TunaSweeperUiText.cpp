#include "UI/TunaSweeperUiText.h"

#include "Game/TunaSweeperGameInstance.h"

namespace TunaSweeperUiText
{
	FText ResolveUiText(
		const UTunaSweeperGameInstance* TunaGameInstance,
		const TCHAR* StringKey,
		const TCHAR* Fallback)
	{
		const FText FallbackText = FText::FromString(Fallback ? Fallback : TEXT(""));
		return TunaGameInstance && StringKey && *StringKey != TEXT('\0')
			? TunaGameInstance->ResolveLocalizedText(FName(StringKey), FallbackText)
			: FallbackText;
	}
}
