// Copyright Epic Games, Inc. All Rights Reserved.
using UnrealBuildTool;
public class TunaSweeperNoStoreDemoTarget : TargetRules
{
	public TunaSweeperNoStoreDemoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		bOverrideBuildEnvironment = true;
		CustomConfig = "NoStoreDemo";
		GlobalDefinitions.Add("TUNASWEEPER_DEMO=1");
		GlobalDefinitions.Add("CUSTOM_CONFIG=\"NoStoreDemo\"");
		ExtraModuleNames.Add("TunaSweeper");
	}
}
