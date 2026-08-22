// Copyright Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
public class TunaSweeperNoStoreTarget : TargetRules
{
	public TunaSweeperNoStoreTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		bOverrideBuildEnvironment = true;
		CustomConfig = "NoStore";
		GlobalDefinitions.Add("TUNASWEEPER_DEMO=0");
		GlobalDefinitions.Add("CUSTOM_CONFIG=\"NoStore\"");
		ExtraModuleNames.Add("TunaSweeper");
	}
}
