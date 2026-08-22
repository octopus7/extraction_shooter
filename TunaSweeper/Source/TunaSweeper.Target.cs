// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class TunaSweeperTarget : TargetRules
{
	public TunaSweeperTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		bOverrideBuildEnvironment = true;
		CustomConfig = "Full";
		GlobalDefinitions.Add("TUNASWEEPER_DEMO=0");
		GlobalDefinitions.Add("CUSTOM_CONFIG=\"Full\"");
		ExtraModuleNames.Add("TunaSweeper");
	}
}
