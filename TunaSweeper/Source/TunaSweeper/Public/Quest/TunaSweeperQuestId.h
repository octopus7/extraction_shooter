#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Runtime identity is a positive int32. Zero is reserved for no quest.
namespace TunaSweeperQuestId
{
	inline int32 FromLegacyString(const FString& Text)
	{
		if (Text.IsEmpty()) return 0;
		int64 Result = 0;
		for (const TCHAR Character : Text)
		{
			if (Character < TEXT('0') || Character > TEXT('9')) return 0;
			Result = Result * 10 + Character - TEXT('0');
			if (Result > MAX_int32) return 0;
		}
		return static_cast<int32>(Result);
	}

	inline int32 FromJson(const TSharedPtr<FJsonValue>& Value)
	{
		if (!Value.IsValid()) return 0;
		if (Value->Type == EJson::String) return FromLegacyString(Value->AsString());
		if (Value->Type != EJson::Number) return 0;
		const double Number = Value->AsNumber();
		return FMath::IsFinite(Number) && Number > 0 && Number <= MAX_int32 && FMath::FloorToDouble(Number) == Number
			? static_cast<int32>(Number) : 0;
	}

	inline int32 Read(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field)
	{
		return Object.IsValid() ? FromJson(Object->TryGetField(Field)) : 0;
	}

	inline FString Format(int32 Id)
	{
		return Id > 0 ? FString::Printf(TEXT("%03d"), Id) : FString();
	}
}
