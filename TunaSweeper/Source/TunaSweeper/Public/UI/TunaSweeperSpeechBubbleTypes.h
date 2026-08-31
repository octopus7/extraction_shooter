#pragma once

#include "CoreMinimal.h"

#include "TunaSweeperSpeechBubbleTypes.generated.h"

/** Direction in which the tail extends away from the speech-bubble body. */
UENUM(BlueprintType)
enum class ETunaSweeperSpeechBubbleTailDirection : uint8
{
	None,
	Up,
	UpRight,
	Right,
	DownRight,
	Down,
	DownLeft,
	Left,
	UpLeft,
};
