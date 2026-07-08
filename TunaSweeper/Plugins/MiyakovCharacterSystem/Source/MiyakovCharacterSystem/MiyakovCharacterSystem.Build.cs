// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MiyakovCharacterSystem : ModuleRules
{
	public MiyakovCharacterSystem(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});
	}
}
