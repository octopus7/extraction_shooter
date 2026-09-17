#pragma once

#include "CoreMinimal.h"

class UButton;
class UTextBlock;
class UWidgetTree;

namespace TunaSweeperUIStyle
{
	enum class EButtonRole : uint8 { Primary, Secondary, Danger, Icon, Tab };
	TUNASWEEPER_API void ApplyButton(UButton* Button, EButtonRole Role = EButtonRole::Primary, bool bSelected = false);
	TUNASWEEPER_API void ApplyLabel(UTextBlock* Label, int32 FontSize = 0);
	TUNASWEEPER_API void SetCheckButton(UWidgetTree* Tree, UButton* Button, UTextBlock* Label, bool bChecked);
}
