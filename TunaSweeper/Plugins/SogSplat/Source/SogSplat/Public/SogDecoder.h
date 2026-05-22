#pragma once

#include "CoreMinimal.h"
#include "SogSplatTypes.h"

class USogAsset;

class SOGSPLAT_API FSogDecoder
{
public:
	static bool DecodeFileToAsset(const FString& FilePath, const FSogDecodeOptions& Options, USogAsset* TargetAsset, FText& OutError);
};
