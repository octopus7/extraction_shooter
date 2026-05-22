#pragma once

#include "CoreMinimal.h"
#include "SogSplatTypes.h"

class USogAsset;

class SOGSPLAT_API FSogDecoder
{
public:
	static bool DecodeFileToAsset(const FString& FilePath, const FSogDecodeOptions& Options, USogAsset* TargetAsset, FText& OutError);
	static bool DecodeArchiveBytesToAsset(const FString& SourcePath, const TArray<uint8>& FileBytes, const FSogDecodeOptions& Options, USogAsset* TargetAsset, FText& OutError);
};
