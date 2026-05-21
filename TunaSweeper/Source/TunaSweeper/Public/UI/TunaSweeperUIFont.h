#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

class UTextBlock;
class UUserWidget;

enum class ETunaSweeperUIFontWeight : uint8
{
	Preserve,
	Regular,
	Bold
};

namespace TunaSweeperUIFont
{
	TUNASWEEPER_API FSlateFontInfo MakeFont(
		const UTextBlock* TextBlock,
		float Size,
		ETunaSweeperUIFontWeight Weight = ETunaSweeperUIFontWeight::Preserve);

	TUNASWEEPER_API void ApplyFont(
		UTextBlock* TextBlock,
		float Size,
		ETunaSweeperUIFontWeight Weight = ETunaSweeperUIFontWeight::Preserve);

	TUNASWEEPER_API void ApplyFontToWidgetTree(UUserWidget* UserWidget);
}
