#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperOcclusionReveal
{
	inline const TCHAR* ParameterCollectionObjectPath()
	{
		return TEXT("/Game/Effects/MPC_TunaSweeperOcclusionReveal.MPC_TunaSweeperOcclusionReveal");
	}

	inline FName CharacterCenterParameterName()
	{
		return FName(TEXT("CharacterRevealCenter"));
	}

	inline FName CursorCenterParameterName()
	{
		return FName(TEXT("CursorRevealCenter"));
	}

	inline FName CharacterRadiusParameterName()
	{
		return FName(TEXT("CharacterRevealRadiusCm"));
	}

	inline FName CursorRadiusParameterName()
	{
		return FName(TEXT("CursorRevealRadiusCm"));
	}

	inline FName RevealFeatherParameterName()
	{
		return FName(TEXT("RevealFeatherCm"));
	}

	inline FName RevealStrengthParameterName()
	{
		return FName(TEXT("RevealStrength"));
	}

	inline FName CursorValidParameterName()
	{
		return FName(TEXT("CursorRevealValid"));
	}

	constexpr int32 RevealIntensityPrimitiveDataIndex = 0;
	constexpr int32 CharacterRadiusScalePrimitiveDataIndex = 1;
	constexpr int32 CursorRadiusScalePrimitiveDataIndex = 2;
	constexpr int32 PatternScalePrimitiveDataIndex = 3;
}
