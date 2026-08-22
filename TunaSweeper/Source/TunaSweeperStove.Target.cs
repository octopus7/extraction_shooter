// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TunaSweeperStoveTarget : TargetRules
{
	public TunaSweeperStoveTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		bOverrideBuildEnvironment = true;
		CustomConfig = "Stove";
		GlobalDefinitions.Add("TUNASWEEPER_DEMO=0");
		GlobalDefinitions.Add("CUSTOM_CONFIG=\"Stove\"");
		ExtraModuleNames.Add("TunaSweeper");
	}
}
