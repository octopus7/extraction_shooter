#pragma once

#include "CoreMinimal.h"

class AActor;

namespace TunaSweeperSpeechBubbleInternal
{
	enum class ETargetType : uint8
	{
		Screen,
		World,
		Actor,
	};

	struct FTargetIdentity
	{
		ETargetType Type = ETargetType::Screen;
		FVector2D ScreenPosition = FVector2D::ZeroVector;
		FVector WorldLocation = FVector::ZeroVector;
		TWeakObjectPtr<AActor> Actor;
	};

	inline bool AreSameTarget(const FTargetIdentity& A, const FTargetIdentity& B)
	{
		if (A.Type != B.Type)
		{
			return false;
		}

		switch (A.Type)
		{
		case ETargetType::Screen:
			return FVector2D::DistSquared(A.ScreenPosition, B.ScreenPosition) <= 1.0;
		case ETargetType::World:
			return FVector::DistSquared(A.WorldLocation, B.WorldLocation) <= 1.0;
		case ETargetType::Actor:
			return A.Actor == B.Actor;
		default:
			return false;
		}
	}

	inline bool HasExpired(const float DurationSeconds, const float ElapsedSeconds)
	{
		return DurationSeconds > 0.0f && ElapsedSeconds >= DurationSeconds;
	}

	inline bool IsValidDuration(const float DurationSeconds)
	{
		return FMath::IsFinite(DurationSeconds);
	}

	template <typename RangeType>
	int32 FindHandleIndex(const RangeType& Items, const FGuid& Handle)
	{
		if (!Handle.IsValid())
		{
			return INDEX_NONE;
		}
		for (int32 Index = 0; Index < Items.Num(); ++Index)
		{
			if (Items[Index].Handle == Handle)
			{
				return Index;
			}
		}
		return INDEX_NONE;
	}
}
