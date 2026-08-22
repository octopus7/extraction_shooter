// Copyright Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
public class TunaSweeperStoveDemoTarget : TargetRules
{
	public TunaSweeperStoveDemoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		bOverrideBuildEnvironment = true;
		CustomConfig = "StoveDemo";
		GlobalDefinitions.Add("TUNASWEEPER_DEMO=1");
		ExtraModuleNames.Add("TunaSweeper");
	}
}